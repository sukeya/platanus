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

#include <atomic>
#include <cassert>
#include <optional>

template <class T>
class Owner {
 public:
  using value_type      = T;
  using reference       = T&;
  using const_reference = const T&;

  class Borrowed {
   public:
    Borrowed() = delete;

    Borrowed(Borrowed&&)              = default;
    Borrowed& operator=(Borrowed&& b) = default;

    Borrowed(const Borrowed& x) : owner_ptr_(x.owner_ptr_) { owner_ptr_->get_cref(); }
    Borrowed& operator=(const Borrowed& x) {
      if (this == &x) {
        return *this;
      }
      if (owner_ptr_ == x.owner_ptr_) {
        return *this;
      }
      owner_ptr_->return_cref();
      x.owner_ptr_->get_cref();
      owner_ptr_ = x.owner_ptr_;
    }

    ~Borrowed() { owner_ptr_->return_cref(); }

    const_reference value() const { return owner_ptr_->value(); }

   private:
    explicit Borrowed(Owner* owner_ptr) : owner_ptr_{owner_ptr} { assert(owner_ptr_ != nullptr); }

    friend Owner;

    Owner* owner_ptr_;
  };

  class Moved {
   public:
    ~Moved() {
      owner_ptr_->return_mut_ref();
    }
   private:
    explicit Moved(Owner* owner_ptr) : owner_ptr_(owner_ptr) { assert(owner_ptr_ != nullptr); }

    friend Owner;

    Owner* owner_ptr_;
  };

  Owner() : v_{}, is_reading_{false}, num_of_crefs_{0} {}

  explicit Owner(T&& v) : v_{std::move(v)}, is_reading_{false}, num_of_crefs_{0} {}

  Borrowed borrow() {
    if (is_reading_.load(std::memory_order_acquire)) {
      auto current = num_of_crefs_.load(std::memory_order_relaxed);
      while (current > 0) {
        num_of_crefs_.compare_exchange_weak(current, current + 1, std::memory_order_relaxed);
      }
      if (current > 0) {
        return Borrowed(this);
      }
    }
    // prevent spuriously unblok
    while (true) {
      bool current = false;
      is_reading_.compare_exchange_strong(current, true, std::memory_order_release, std::memory_order_acquire);
      if (not current) {
        break;
      }
      is_reading_.wait(true, std::memory_order_acquire);
    }
    assert(num_of_crefs_.load(std::memory_order_relaxed) == 0);
    // In this point, the other thread may see as if this thread borrows mutably.
    // However, even if the following threads reach here, they only atomically add num_of_crefs_.
    // So, there is no problem.
    get_cref();
    return Borrowed(this);
  }

  Moved move() {
    // prevent spuriously unblok
    while (true) {
      bool current = false;
      is_reading_.compare_exchange_strong(current, true, std::memory_order_release, std::memory_order_acquire);
      if (not current) {
        break;
      }
      is_reading_.wait(true, std::memory_order_acquire);
    }
    assert(num_of_crefs_.load(std::memory_order_acquire) == 0);
    return Moved(this);
  }

 private:
  const_reference value() const {
    return static_cast<const_reference>(const_cast<Owner*>(this)->value());
  }

  reference value() {
    return v_;
  }

  void return_mut_ref() {
    assert(is_reading_.load(std::memory_order_acquire));
    is_reading_.store(false, std::memory_order_release);
    is_reading_.notify_all();
  }

  void return_cref() {
    assert(is_reading_.load(std::memory_order_acquire));
    num_of_crefs_.fetch_sub(1, std::memory_order_relaxed);
    assert(num_of_crefs_.load(std::memory_order_relaxed) >= 0);
    if (num_of_crefs_.load(std::memory_order_relaxed) == 0) {
      is_reading_.store(false, std::memory_order_release);
      is_reading_.notify_all();
    }
  }

  void get_cref() {
    assert(is_reading_.load(std::memory_order_acquire));
    num_of_crefs_.fetch_add(1, std::memory_order_relaxed);
  }

  T          v_;
  std::atomic<bool>         is_reading_;
  // In Ubuntu 24.04, the maximum number of unique process is 4194304, so I use 32bit integer.
  std::atomic<std::int_least32_t> num_of_crefs_;
};
