// Copyright 2024-2025 Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

#ifndef PLATANUS_PMR_POLYMORPHIC_ALLOCATOR_H_
#define PLATANUS_PMR_POLYMORPHIC_ALLOCATOR_H_

#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <memory_resource>

namespace platanus::pmr {

template <class Tp = std::byte>
class polymorphic_allocator {
  using memory_resource = std::pmr::memory_resource;

 public:
  using value_type = Tp;

  polymorphic_allocator() noexcept : mem_res_ptr_(std::pmr::get_default_resource()) {}
  polymorphic_allocator(const polymorphic_allocator&) = default;
  polymorphic_allocator(polymorphic_allocator&&)      = default;
  // Allow move assignment
  // This can cause memory leak, but is only used by the move assignment of std::unique_ptr as
  // deleter. It's guaranteed that the resource of the assigned value is released, so this is safe.
  polymorphic_allocator& operator=(polymorphic_allocator&&) = default;
  ~polymorphic_allocator()                                  = default;

  polymorphic_allocator& operator=(const polymorphic_allocator&) = delete;

  polymorphic_allocator(memory_resource* mem_res_ptr) : mem_res_ptr_(mem_res_ptr) {
    assert(mem_res_ptr_);
  }

  template <class U>
  polymorphic_allocator(std::pmr::polymorphic_allocator<U> poly_alloc) noexcept : mem_res_ptr_(poly_alloc.resource()) {
    assert(mem_res_ptr_);
  }

  template <class U>
  polymorphic_allocator(const polymorphic_allocator<U>& other) noexcept
      : mem_res_ptr_(other.resource()) {}

  [[nodiscard]] Tp* allocate(std::size_t n) {
    constexpr std::size_t max = std::numeric_limits<std::size_t>::max();
    return static_cast<Tp*>(
        mem_res_ptr_->allocate((max / sizeof(Tp) < n) ? max : (n * sizeof(Tp)), alignof(Tp))
    );
  }

  void deallocate(Tp* p, std::size_t n) {
    mem_res_ptr_->deallocate(p, n * sizeof(Tp), alignof(Tp));
  }

  [[nodiscard]] void* allocate_bytes(
      std::size_t nbytes, std::size_t alignment = alignof(std::max_align_t)
  ) {
    return mem_res_ptr_->allocate(nbytes, alignment);
  }

  void deallocate_bytes(
      void* p, std::size_t nbytes, std::size_t alignment = alignof(std::max_align_t)
  ) {
    mem_res_ptr_->deallocate(p, nbytes, alignment);
  }

  template <class T>
  [[nodiscard]] T* allocate_object(std::size_t n = 1) {
    constexpr std::size_t max = std::numeric_limits<std::size_t>::max();
    return static_cast<T*>(allocate_bytes((max / sizeof(T) < n) ? max : (n * sizeof(T)), alignof(T))
    );
  }

  template <class T>
  void deallocate_object(T* p, std::size_t n = 1) {
    deallocate_bytes(p, n * sizeof(T), alignof(T));
  }

  template <class T, class... Args>
  void construct(T* p, Args&&... args) {
    std::uninitialized_construct_using_allocator(p, *this, std::forward<Args>(args)...);
  }

  template <class T>
  void destroy(T* p) {
    p->~T();
  }

  template <class T, class... Args>
  [[nodiscard]] T* new_object(Args&&... args) {
    T* p = allocate_object<T>();
    try {
      construct(p, std::forward<T>(args...));
    } catch (...) {
      deallocate_object(p);
      throw;
    }
    return p;
  }

  template <class T>
  void delete_object(T* p) {
    destroy(p);
    deallocate_object(p);
  }

  polymorphic_allocator select_on_container_copy_construction() const noexcept {
    return polymorphic_allocator();
  }

  memory_resource* resource() const noexcept { return mem_res_ptr_; }

  void swap_resource(polymorphic_allocator& other) noexcept {
    auto* tmp          = mem_res_ptr_;
    mem_res_ptr_       = other.mem_res_ptr_;
    other.mem_res_ptr_ = tmp;
  }

 private:
  memory_resource* mem_res_ptr_;
};

template <class T1, class T2>
bool operator==(
    const polymorphic_allocator<T1>& left, const polymorphic_allocator<T2>& right
) noexcept {
  return *(left.resource()) == *(right.resource());
}

}  // namespace platanus::pmr

#endif
