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

  class Borrower {
   public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;

    Borrower() = delete;

    Borrower(const Borrower& x) : owner_ptr_(x.owner_ptr_) { owner_ptr_->add_num_of_borrowers(); }
    Borrower& operator=(const Borrower& x) {
      if (owner_ptr_ == x.owner_ptr_) {
        return *this;
      }
      owner_ptr_->sub_num_of_borrowers();
      x.owner_ptr_->add_num_of_borrowers();
      owner_ptr_ = x.owner_ptr_;
    }

    Borrower(Borrower&& b) : owner_ptr_(b.owner_ptr_) { b.owner_ptr_ = nullptr; }
    Borrower& operator=(Borrower&& b) {
      if (this == &b) {
        return *this;
      }
      owner_ptr_   = b.owner_ptr_;
      b.owner_ptr_ = nullptr;
    }

    ~Borrower() { owner_ptr_->sub_num_of_borrowers(); }

    const_reference value() const { return owner_ptr_->value(); }

   private:
    explicit Borrower(Owner* owner_ptr) : owner_ptr_{owner_ptr} { assert(owner_ptr_ != nullptr); }

    friend Owner;

    Owner* owner_ptr_;
  };

  class MutableBorrower {
   public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;

    MutableBorrower()                                    = delete;
    MutableBorrower(const MutableBorrower&)              = delete;
    MutableBorrower& operator=(const MutableBorrower& b) = delete;

    MutableBorrower(MutableBorrower&& b) : owner_ptr_(b.owner_ptr_) { b.owner_ptr_ = nullptr; }
    MutableBorrower& operator=(MutableBorrower&& b) {
      if (this == &b) {
        return *this;
      }
      owner_ptr_   = b.owner_ptr_;
      b.owner_ptr_ = nullptr;
    }

    ~MutableBorrower() { owner_ptr_->end_reading(); }

    reference       value() { return owner_ptr_->value(); }
    const_reference value() const {
      return static_cast<const_reference>(const_cast<MutableBorrower*>(this)->value());
    }

   private:
    explicit MutableBorrower(Owner* owner_ptr) : owner_ptr_(owner_ptr) {
      assert(owner_ptr_ != nullptr);
    }

    friend Owner;

    Owner* owner_ptr_;
  };

  Owner() : v_{}, is_reading_{false}, num_of_borrowers_{0} {}

  Owner(const Owner&)            = delete;
  Owner(Owner&&)                 = delete;
  Owner& operator=(const Owner&) = delete;
  Owner& operator=(Owner&&)      = delete;

  // Owner don't care about waiting, reading or writing borrower because there is no way for owner
  // to check how many such borrowers exist. So, programmers have to manage the lifetime of owner
  // and its borrowers.
  ~Owner() = default;

  explicit Owner(T&& v) : v_{std::move(v)}, is_reading_{false}, num_of_borrowers_{0} {}

  Borrower borrow() {
    // Set `acquire` to ensure the owned value can be read after being written by MutableBorrower.
    if (is_reading_.load(std::memory_order_acquire)) {
      if (try_to_borrow_or_wait()) {
        return Borrower(this);
      }
    }
    assert(not is_reading_.load(std::memory_order_relaxed));
    assert(num_of_borrowers_.load(std::memory_order_relaxed) == 0);
    // Try to get lock.
    bool dummy = false;
    while (not is_reading_.compare_exchange_strong(dummy, true, std::memory_order_relaxed)) {
      if (try_to_borrow_or_wait()) {
        return Borrower(this);
      }
    }
    // In this point, the other thread may see as if this thread references mutably.
    add_num_of_borrowers();
    return Borrower(this);
  }

  MutableBorrower mutable_borrow() {
    // prevent spuriously unblock
    bool dummy = false;
    while (not is_reading_.compare_exchange_strong(
        dummy,
        true,
        std::memory_order_acq_rel,
        std::memory_order_relaxed
    )) {
      is_reading_.wait(true, std::memory_order_relaxed);
    }
    assert(num_of_borrowers_.load(std::memory_order_relaxed) == 0);
    return MutableBorrower(this);
  }

 private:
  reference       value() { return v_; }
  const_reference value() const {
    return static_cast<const_reference>(const_cast<Owner*>(this)->value());
  }

  void end_reading() {
    assert(is_reading_.load(std::memory_order_relaxed));
    is_reading_.store(false, std::memory_order_release);
    is_reading_.notify_all();
  }

  void sub_num_of_borrowers() {
    assert(is_reading_.load(std::memory_order_relaxed));
    num_of_borrowers_.fetch_sub(1, std::memory_order_relaxed);
    assert(num_of_borrowers_.load(std::memory_order_relaxed) >= 0);
    if (num_of_borrowers_.load(std::memory_order_relaxed) == 0) {
      end_reading();
    }
  }

  void add_num_of_borrowers() {
    assert(is_reading_.load(std::memory_order_relaxed));
    num_of_borrowers_.fetch_add(1, std::memory_order_relaxed);
  }

  // If return value is true, success to borrow: otherwise, fail.
  bool try_to_borrow_or_wait() {
    auto current_num_of_borrowers = num_of_borrowers_.load(std::memory_order_relaxed);
    // Try to add 1 to the number of borrowers while current_num_of_borrowers > 0.
    // `current_num_of_borrowers > 0` is required because the other threads may release their
    // borrowers until this thread reaches the following line or simply mutable borrower exists.
    while (current_num_of_borrowers > 0
           || not num_of_borrowers_.compare_exchange_weak(
               current_num_of_borrowers,
               current_num_of_borrowers + 1,
               std::memory_order_relaxed
           )) {
    }
    if (current_num_of_borrowers > 0) {
      return true;
    }
    // prevent spuriously unblock
    while (is_reading_.load(std::memory_order_relaxed)) {
      is_reading_.wait(true, std::memory_order_relaxed);
    }
    return false;
  }

  T                 v_;
  std::atomic<bool> is_reading_;
  // In Ubuntu 24.04, the maximum number of unique process is 4194304, so I use 32bit integer.
  std::atomic<std::int_least32_t> num_of_borrowers_;
};
