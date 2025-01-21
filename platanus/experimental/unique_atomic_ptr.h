// Copyright 2025- Yuya Asano <my_favorite_theory@yahoo.co.jp>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PLATANUS_INTERNAL_UNIQUE_ATOMIC_PTR_H_
#define PLATANUS_INTERNAL_UNIQUE_ATOMIC_PTR_H_

#include <atomic>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <memory>
#include <type_traits>

#if __cplusplus >= 202302L
#define PLATANUS_CONSTEXPR = constexpr
#else
#define PLATANUS_CONSTEXPR
#endif

template <class D, class T>
concept is_deleter =
    std::is_nothrow_default_constructible_v<D> && std::is_nothrow_copy_constructible_v<D>
    && std::is_nothrow_move_constructible_v<D> && std::invocable<const D, T*>;

namespace platanus::internal {
template <class T, class D = std::default_delete<T>>
class unique_atomic_ptr {
 public:
  using pointer      = T*;
  using element_type = T;
  using deleter_type = D;

 private:
  using rmref_deleter_type = std::remove_cvref_t<deleter_type>;

 public:
  constexpr unique_atomic_ptr() noexcept
  requires(std::is_nothrow_default_constructible_v<deleter_type>)
      : ptr_and_deleter_() {}

  constexpr unique_atomic_ptr(std::nullptr_t) noexcept : unique_atomic_ptr() {}

  PLATANUS_CONSTEXPR explicit unique_atomic_ptr(std::type_identity_t<pointer> p) noexcept
  requires(std::is_nothrow_default_constructible_v<deleter_type>)
      : ptr_and_deleter_(p, deleter_type()) {}

  PLATANUS_CONSTEXPR unique_atomic_ptr(std::type_identity_t<pointer> p, const deleter_type& dl) noexcept
  requires(not std::is_reference_v<deleter_type> && std::is_nothrow_copy_constructible_v<deleter_type>)
      : ptr_and_deleter_(p, dl) {}

  PLATANUS_CONSTEXPR unique_atomic_ptr(std::type_identity_t<pointer> p, deleter_type dl) noexcept
  requires(not std::is_const_v<deleter_type> && std::is_reference_v<deleter_type> && std::is_nothrow_constructible_v<deleter_type, decltype(dl)>)
      : ptr_and_deleter_(p, dl) {}

  PLATANUS_CONSTEXPR unique_atomic_ptr(std::type_identity_t<pointer> p, deleter_type dl) noexcept
  requires(std::is_const_v<deleter_type> && std::is_reference_v<deleter_type> && std::is_nothrow_constructible_v<deleter_type, decltype(dl)>)
      : ptr_and_deleter_(p, dl) {}

  PLATANUS_CONSTEXPR unique_atomic_ptr(std::type_identity_t<pointer> p, deleter_type&& dl) noexcept
  requires(not std::is_reference_v<deleter_type>)
      : ptr_and_deleter_(p, std::forward<deleter_type>(dl)) {}

  PLATANUS_CONSTEXPR unique_atomic_ptr(pointer p, rmref_deleter_type&& dl) noexcept
  requires(not std::is_const_v<deleter_type> && std::is_reference_v<deleter_type>)
  = delete;

  PLATANUS_CONSTEXPR unique_atomic_ptr(pointer p, const rmref_deleter_type&& dl) noexcept
  requires(std::is_const_v<deleter_type> && std::is_reference_v<deleter_type>)
  = delete;

  PLATANUS_CONSTEXPR unique_atomic_ptr(unique_atomic_ptr&& x) noexcept
  requires(not std::is_reference_v<deleter_type> && std::is_nothrow_move_constructible_v<deleter_type>)
      : ptr_and_deleter_(std::move(x.ptr_and_deleter_)) {}

  PLATANUS_CONSTEXPR unique_atomic_ptr(unique_atomic_ptr&& x) noexcept
  requires(std::is_reference_v<deleter_type> && std::is_move_constructible_v<deleter_type>)
      : ptr_and_deleter_(std::move(x.ptr_and_deleter_)) {}

  template <class U, class E>
  requires(
      std::is_convertible_v<typename unique_atomic_ptr<U, E>::pointer, pointer>
      && (not std::is_array_v<U>) && ((std::is_reference_v<deleter_type> && std::is_same_v<deleter_type, E>) || (
        not std::is_reference_v<deleter_type> && std::is_convertible_v<E, deleter_type>
      ))
  )
  PLATANUS_CONSTEXPR unique_atomic_ptr(unique_atomic_ptr<U, E>&& x) noexcept
      : unique_atomic_ptr{x.release(), x.get_deleter()} {}

  unique_atomic_ptr(const unique_atomic_ptr&) = delete;

  // TODO add requires section for deleter_type.
  PLATANUS_CONSTEXPR unique_atomic_ptr& operator=(unique_atomic_ptr&& x) noexcept {
    if (&x == this) {
      return *this;
    }
    reset(x.release());
    assert(get_deleter() == x.get_deleter());
    return *this;
  }

  // TODO add requires section for E.
  template <class U, class E>
  requires(not std::is_reference_v<U> && std::is_convertible_v<typename unique_atomic_ptr<U, E>::pointer, pointer>)
  PLATANUS_CONSTEXPR unique_atomic_ptr& operator=(unique_atomic_ptr<U, E>&& u) noexcept {
    if (&x == this) {
      return *this;
    }
    reset(u.release());
    assert(get_deleter() == x.get_deleter());
    return *this;
  }

  PLATANUS_CONSTEXPR unique_atomic_ptr& operator=(std::nullptr_t) noexcept {
    reset();
    return *this;
  }

  unique_atomic_ptr& operator=(const unique_atomic_ptr&) = delete;

  PLATANUS_CONSTEXPR ~unique_atomic_ptr() requires(std::is_nothrow_invocable_v<const D, pointer>) {
    if (get() == nullptr) {
      return;
    }
    get_deleter()(release());
  }

  PLATANUS_CONSTEXPR pointer release() noexcept {
    return ptr_.exchange(nullptr, std::memory_order_relaxed);
  }

  PLATANUS_CONSTEXPR void reset(pointer p = pointer()) noexcept requires(std::is_nothrow_invocable_v<const D, pointer>) {
    auto old_ptr = ptr_.exchange(p, std::memory_order_relaxed);
    if (old_ptr) {
      get_deleter()(old_ptr);
    }
  }

  PLATANUS_CONSTEXPR void reset(std::nullptr_t = nullptr) noexcept {
    reset(pointer());
  }

  void swap(unique_atomic_ptr& x) noexcept {
    ptr_and_deleter_.swap(x.ptr_and_deleter_);
  }

  PLATANUS_CONSTEXPR pointer get() const noexcept { return ptr_.load(std::memory_order_relaxed); }

  PLATANUS_CONSTEXPR deleter_type&       get_deleter() noexcept { return *this; }
  PLATANUS_CONSTEXPR const deleter_type& get_deleter() const noexcept {
    return static_cast<const deleter_type&>(const_cast<unique_atomic_ptr*>(this)->get_deleter());
  }

  PLATANUS_CONSTEXPR explicit operator bool() const noexcept { return get() != nullptr; }

  PLATANUS_CONSTEXPR typename std::add_lvalue_reference<T>::type operator*() const noexcept(noexcept(*std::declval<pointer>())) {
    return *get();
  }

  PLATANUS_CONSTEXPR pointer operator->() const noexcept {
    return get();
  }

 private:
  std::pair<std::atomic<T*>, deleter_type> ptr_and_deleter_;
};

template <class T, class D>
class unique_atomic_ptr<T[], D> : private D {
 public:
  template <class U>
  PLATANUS_CONSTEXPR explicit unique_atomic_ptr(U p) noexcept : ptr_(p) {}

  template <class U>
  PLATANUS_CONSTEXPR unique_atomic_ptr(U p, const deleter_type& dl) noexcept
      : deleter_type(dl), ptr_(p) {}

  template <class U>
  PLATANUS_CONSTEXPR unique_atomic_ptr(U p, deleter_type&& dl) noexcept
      : deleter_type(std::move(dl)), ptr_(p) {}

  template <class U, class E>
  PLATANUS_CONSTEXPR unique_atomic_ptr(unique_atomic_ptr<U, E>&& x) noexcept
      : deleter_type(x.get_deleter()), ptr_(x.get()) {}

  template <class U, class E>
  unique_atomic_ptr& operator=(unique_atomic_ptr<U, E>&& x) noexcept {
    *this = static_cast<deleter_type>(x);
    ptr_.exchange(x.get());
  }
};

}  // namespace platanus::internal

#undef PLATANUS_CONSTEXPR

#endif
