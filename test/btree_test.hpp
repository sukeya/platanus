// Copyright 2013 Google Inc. All Rights Reserved.
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
//
// Copyright 2024- Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

#ifndef PLATANUS_BTREE_TEST_H_
#define PLATANUS_BTREE_TEST_H_

#include <cstdio>
#include <algorithm>
#include <type_traits>
#include <iosfwd>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#include "util.hpp"

namespace platanus {

// The number of values to use for tests.
inline static constexpr int test_values = 10'000;

// Counts the number of occurances of "c" in a buffer.
ptrdiff_t strcount(const char* buf_begin, const char* buf_end, char c) {
  if (buf_begin == nullptr) return 0;
  if (buf_end <= buf_begin) return 0;
  ptrdiff_t num = 0;
  for (const char* bp = buf_begin; bp != buf_end; bp++) {
    if (*bp == c) num++;
  }
  return num;
}

// for when the string is not null-terminated.
ptrdiff_t strcount(const char* buf, size_t len, char c) { return strcount(buf, buf + len, c); }

ptrdiff_t strcount(const std::string& buf, char c) { return strcount(buf.c_str(), buf.size(), c); }

// The base class for a sorted associative container checker. TreeType is the
// container type to check and CheckerType is the container type to check
// against. TreeType is expected to be btree_{set,map,multiset,multimap} and
// CheckerType is expected to be {set,map,multiset,multimap}.
template <typename TreeType, typename CheckerType>
class base_checker {
  using self_type = base_checker<TreeType, CheckerType>;

 public:
  using key_type               = typename TreeType::key_type;
  using value_type             = typename TreeType::value_type;
  using key_compare            = typename TreeType::key_compare;
  using pointer                = typename TreeType::pointer;
  using const_pointer          = typename TreeType::const_pointer;
  using reference              = typename TreeType::reference;
  using const_reference        = typename TreeType::const_reference;
  using size_type              = typename TreeType::size_type;
  using difference_type        = typename TreeType::difference_type;
  using iterator               = typename TreeType::iterator;
  using const_iterator         = typename TreeType::const_iterator;
  using reverse_iterator       = typename TreeType::reverse_iterator;
  using const_reverse_iterator = typename TreeType::const_reverse_iterator;

 public:
  // Default constructor.
  base_checker() : const_tree_(tree_) {}
  // Copy constructor.
  base_checker(const self_type& x) : tree_(x.tree_), const_tree_(tree_), checker_(x.checker_) {}
  // Copy assignment.
  base_checker& operator=(const self_type& x) {
    tree_    = x.tree_;
    checker_ = x.checker_;
    return *this;
  }

  // Range constructor.
  template <typename InputIterator>
  base_checker(InputIterator b, InputIterator e)
      : tree_(b, e), const_tree_(tree_), checker_(b, e) {}

  // Iterator routines.
  iterator               begin() { return tree_.begin(); }
  const_iterator         begin() const { return tree_.begin(); }
  iterator               end() { return tree_.end(); }
  const_iterator         end() const { return tree_.end(); }
  reverse_iterator       rbegin() { return tree_.rbegin(); }
  const_reverse_iterator rbegin() const { return tree_.rbegin(); }
  reverse_iterator       rend() { return tree_.rend(); }
  const_reverse_iterator rend() const { return tree_.rend(); }

  // Helper routines.
  template <typename IterType, typename CheckerIterType>
  IterType iter_check(IterType tree_iter, CheckerIterType checker_iter) const {
    if (tree_iter == tree_.end()) {
      EXPECT_EQ(checker_iter, checker_.end());
    } else {
      EXPECT_EQ(*tree_iter, *checker_iter);
    }
    return tree_iter;
  }

  template <typename IterType, typename CheckerIterType>
  IterType riter_check(IterType tree_iter, CheckerIterType checker_iter) const {
    if (tree_iter == tree_.rend()) {
      EXPECT_EQ(checker_iter, checker_.rend());
    } else {
      EXPECT_EQ(*tree_iter, *checker_iter);
    }
    return tree_iter;
  }

  void value_check(const value_type& x) {
    using KeyGetter =
        typename KeyOfValue<typename TreeType::key_type, typename TreeType::value_type>::type;

    const key_type& key = KeyGetter::Get(x);
    EXPECT_EQ(*find(key), x);
    lower_bound(key);
    upper_bound(key);
    equal_range(key);
    count(key);
    contains(key);
  }

  void erase_check(const key_type& key) {
    EXPECT_TRUE(tree_.find(key) == const_tree_.end());
    EXPECT_TRUE(const_tree_.find(key) == tree_.end());
    EXPECT_TRUE(tree_.equal_range(key).first == const_tree_.equal_range(key).second);
  }

  // Lookup routines.
  iterator lower_bound(const key_type& key) {
    return iter_check(tree_.lower_bound(key), checker_.lower_bound(key));
  }

  const_iterator lower_bound(const key_type& key) const {
    return iter_check(tree_.lower_bound(key), checker_.lower_bound(key));
  }

  iterator upper_bound(const key_type& key) {
    return iter_check(tree_.upper_bound(key), checker_.upper_bound(key));
  }

  const_iterator upper_bound(const key_type& key) const {
    return iter_check(tree_.upper_bound(key), checker_.upper_bound(key));
  }

  std::pair<iterator, iterator> equal_range(const key_type& key) {
    std::pair<typename CheckerType::iterator, typename CheckerType::iterator> checker_res =
        checker_.equal_range(key);
    std::pair<iterator, iterator> tree_res = tree_.equal_range(key);
    iter_check(tree_res.first, checker_res.first);
    iter_check(tree_res.second, checker_res.second);
    return tree_res;
  }

  std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
    std::pair<typename CheckerType::const_iterator, typename CheckerType::const_iterator>
                                              checker_res = checker_.equal_range(key);
    std::pair<const_iterator, const_iterator> tree_res    = tree_.equal_range(key);
    iter_check(tree_res.first, checker_res.first);
    iter_check(tree_res.second, checker_res.second);
    return tree_res;
  }

  iterator find(const key_type& key) { return iter_check(tree_.find(key), checker_.find(key)); }
  const_iterator find(const key_type& key) const {
    return iter_check(tree_.find(key), checker_.find(key));
  }

  size_type count(const key_type& key) const {
    size_type res = checker_.count(key);
    EXPECT_EQ(res, tree_.count(key));
    return res;
  }

  bool contains(const key_type& key) const {
    bool res = checker_.contains(key);
    EXPECT_EQ(res, tree_.contains(key));
    return res;
  }

  // Deletion routines.
  int erase(const key_type& key) {
    int size = tree_.size();
    int res  = checker_.erase(key);
    EXPECT_EQ(res, tree_.count(key));
    EXPECT_EQ(res, tree_.erase(key));
    EXPECT_EQ(tree_.count(key), 0);
    EXPECT_EQ(tree_.size(), size - res);
    erase_check(key);
    return res;
  }

  iterator erase(iterator iter) {
    key_type                       key          = iter.key();
    int                            size         = tree_.size();
    int                            count        = tree_.count(key);
    typename CheckerType::iterator checker_iter = checker_.find(key);
    for (iterator tmp(tree_.find(key)); tmp != iter; ++tmp) {
      ++checker_iter;
    }
    typename CheckerType::iterator checker_next = checker_iter;
    ++checker_next;
    checker_.erase(checker_iter);
    iter = tree_.erase(iter);
    EXPECT_EQ(tree_.size(), checker_.size());
    EXPECT_EQ(tree_.size(), size - 1);
    EXPECT_EQ(tree_.count(key), count - 1);
    if (count == 1) {
      erase_check(key);
    }
    return iter_check(iter, checker_next);
  }

  void erase(iterator begin, iterator end) {
    int                            size          = tree_.size();
    int                            count         = std::distance(begin, end);
    typename CheckerType::iterator checker_begin = checker_.find(begin.key());
    for (iterator tmp(tree_.find(begin.key())); tmp != begin; ++tmp) {
      ++checker_begin;
    }
    typename CheckerType::iterator checker_end =
        end == tree_.end() ? checker_.end() : checker_.find(end.key());
    if (end != tree_.end()) {
      for (iterator tmp(tree_.find(end.key())); tmp != end; ++tmp) {
        ++checker_end;
      }
    }
    checker_.erase(checker_begin, checker_end);
    tree_.erase(begin, end);
    EXPECT_EQ(tree_.size(), checker_.size());
    EXPECT_EQ(tree_.size(), size - count);
  }

  // Utility routines.
  void clear() {
    tree_.clear();
    checker_.clear();
  }

  void swap(self_type& x) {
    tree_.swap(x.tree_);
    checker_.swap(x.checker_);
  }

  void verify() const {
    tree_.verify();
    EXPECT_EQ(tree_.size(), checker_.size());

    // Move through the forward iterators using increment.
    typename CheckerType::const_iterator checker_iter(checker_.begin());
    const_iterator                       tree_iter(tree_.begin());
    for (; tree_iter != tree_.end(); ++tree_iter, ++checker_iter) {
      EXPECT_EQ(*tree_iter, *checker_iter);
    }

    // Move through the forward iterators using decrement.
    for (int n = tree_.size() - 1; n >= 0; --n) {
      iter_check(tree_iter, checker_iter);
      --tree_iter;
      --checker_iter;
    }
    EXPECT_TRUE(tree_iter == tree_.begin());
    EXPECT_TRUE(checker_iter == checker_.begin());

    // Move through the reverse iterators using increment.
    typename CheckerType::const_reverse_iterator checker_riter(checker_.rbegin());
    const_reverse_iterator                       tree_riter(tree_.rbegin());
    for (; tree_riter != tree_.rend(); ++tree_riter, ++checker_riter) {
      EXPECT_EQ(*tree_riter, *checker_riter);
    }

    // Move through the reverse iterators using decrement.
    for (int n = tree_.size() - 1; n >= 0; --n) {
      riter_check(tree_riter, checker_riter);
      --tree_riter;
      --checker_riter;
    }
    EXPECT_EQ(tree_riter, tree_.rbegin());
    EXPECT_EQ(checker_riter, checker_.rbegin());
  }

  // Access to the underlying btree.
  const TreeType& tree() const { return tree_; }

  // Size routines.
  size_type size() const {
    EXPECT_EQ(tree_.size(), checker_.size());
    return tree_.size();
  }

  size_type max_size() const { return tree_.max_size(); }

  bool empty() const {
    EXPECT_EQ(tree_.empty(), checker_.empty());
    return tree_.empty();
  }

  size_type height() const { return tree_.height(); }
  size_type internal_nodes() const { return tree_.internal_nodes(); }
  size_type leaf_nodes() const { return tree_.leaf_nodes(); }
  size_type nodes() const { return tree_.nodes(); }
  size_type bytes_used() const { return tree_.bytes_used(); }
  double    fullness() const { return tree_.fullness(); }
  double    overhead() const { return tree_.overhead(); }

 protected:
  TreeType        tree_;
  const TreeType& const_tree_;
  CheckerType     checker_;
};

// A checker for unique sorted associative containers. TreeType is expected to
// be btree_{set,map} and CheckerType is expected to be {set,map}.
template <typename TreeType, typename CheckerType>
class unique_checker : public base_checker<TreeType, CheckerType> {
  using super_type = base_checker<TreeType, CheckerType>;
  using self_type  = unique_checker<TreeType, CheckerType>;

 public:
  using iterator   = typename super_type::iterator;
  using value_type = typename super_type::value_type;

 public:
  // Default constructor.
  unique_checker() : super_type() {}
  // Copy constructor.
  unique_checker(const self_type& x) : super_type(x) {}
  // Copy assignment.
  unique_checker& operator=(const self_type& x) {
    super_type::operator=(x);
    return *this;
  }

  // Range constructor.
  template <class InputIterator>
  unique_checker(InputIterator b, InputIterator e) : super_type(b, e) {}

  // Insertion routines.
  template <class T>
  std::pair<iterator, bool> insert_impl(T&& x) {
    auto                                            y           = T{x};
    std::size_t                                     size        = this->tree_.size();
    std::pair<typename CheckerType::iterator, bool> checker_res = this->checker_.insert(y);
    std::pair<iterator, bool> tree_res = this->tree_.insert(std::forward<T>(x));
    EXPECT_EQ(*tree_res.first, *checker_res.first);
    EXPECT_EQ(tree_res.second, checker_res.second);
    EXPECT_EQ(this->tree_.size(), this->checker_.size());
    EXPECT_EQ(this->tree_.size(), size + tree_res.second);
    return tree_res;
  }
  std::pair<iterator, bool> insert(const value_type& x) { return insert_impl(x); }
  std::pair<iterator, bool> insert(value_type&& x) { return insert_impl(std::move(x)); }

  template <class T>
  iterator insert_impl(iterator position, T&& x) {
    auto                                            y           = T{x};
    typename CheckerType::size_type                 size        = this->tree_.size();
    std::pair<typename CheckerType::iterator, bool> checker_res = this->checker_.insert(y);
    iterator tree_res = this->tree_.insert(position, std::forward<T>(x));
    EXPECT_EQ(*tree_res, *checker_res.first);
    EXPECT_EQ(this->tree_.size(), this->checker_.size());
    EXPECT_EQ(this->tree_.size(), size + checker_res.second);
    return tree_res;
  }
  iterator insert(iterator position, const value_type& x) { return insert_impl(position, x); }
  iterator insert(iterator position, value_type&& x) { return insert_impl(position, std::move(x)); }

  template <typename InputIterator>
  void insert(InputIterator b, InputIterator e) {
    for (; b != e; ++b) {
      insert(*b);
    }
  }
};

// A checker for multiple sorted associative containers. TreeType is expected
// to be btree_{multiset,multimap} and CheckerType is expected to be
// {multiset,multimap}.
template <typename TreeType, typename CheckerType>
class multi_checker : public base_checker<TreeType, CheckerType> {
  using super_type = base_checker<TreeType, CheckerType>;
  using self_type  = multi_checker<TreeType, CheckerType>;

 public:
  using iterator   = typename super_type::iterator;
  using value_type = typename super_type::value_type;

 public:
  // Default constructor.
  multi_checker() : super_type() {}
  // Copy constructor.
  multi_checker(const self_type& x) : super_type(x) {}
  // Copy assignment.
  multi_checker& operator=(const self_type& x) {
    super_type::operator=(x);
    return *this;
  }

  // Range constructor.
  template <class InputIterator>
  multi_checker(InputIterator b, InputIterator e) : super_type(b, e) {}

  // Insertion routines.
  template <class T>
  iterator insert_impl(T&& x) {
    typename CheckerType::size_type size        = this->tree_.size();
    typename CheckerType::iterator  checker_res = this->checker_.insert(T{x});
    iterator                        tree_res    = this->tree_.insert(std::forward<T>(x));
    EXPECT_EQ(*tree_res, *checker_res);
    EXPECT_EQ(this->tree_.size(), this->checker_.size());
    EXPECT_EQ(this->tree_.size(), size + 1);
    return tree_res;
  }
  iterator insert(const value_type& x) { return insert_impl(x); }
  iterator insert(value_type&& x) { return insert_impl(std::move(x)); }

  template <class T>
  iterator insert_impl(iterator position, T&& x) {
    typename CheckerType::size_type size        = this->tree_.size();
    typename CheckerType::iterator  checker_res = this->checker_.insert(
        std::next(this->checker_.begin(), std::distance(this->tree_.begin(), position)),
        T{x}
    );
    iterator tree_res = this->tree_.insert(position, std::forward<T>(x));
    EXPECT_EQ(*tree_res, *checker_res);
    EXPECT_EQ(this->tree_.size(), this->checker_.size());
    EXPECT_EQ(this->tree_.size(), size + 1);
    return tree_res;
  }
  iterator insert(iterator position, const value_type& x) { return insert_impl(position, x); }
  iterator insert(iterator position, value_type&& x) { return insert_impl(position, std::move(x)); }

  template <typename InputIterator>
  void insert(InputIterator b, InputIterator e) {
    for (; b != e; ++b) {
      insert(*b);
    }
  }
};

template <typename T, typename V>
void DoTest(const char* name, T* b, const std::vector<V>& values) {
  using KeyGetter = typename KeyOfValue<typename T::key_type, V>::type;

  T&       mutable_b = *b;
  const T& const_b   = *b;

  // Test insert.
  for (const auto& v : values) {
    mutable_b.insert(v);
    mutable_b.value_check(v);
  }
  assert(mutable_b.size() == values.size());

  const_b.verify();
  printf(
      "    %s fullness=%0.2f  overhead=%0.2f  bytes-per-value=%0.2f\n",
      name,
      const_b.fullness(),
      const_b.overhead(),
      double(const_b.bytes_used()) / const_b.size()
  );

  // Test copy constructor.
  T b_copy(const_b);
  EXPECT_EQ(b_copy.size(), const_b.size());
  EXPECT_LE(b_copy.height(), const_b.height());
  EXPECT_LE(b_copy.internal_nodes(), const_b.internal_nodes());
  EXPECT_LE(b_copy.leaf_nodes(), const_b.leaf_nodes());
  for (const auto& v : values) {
    EXPECT_EQ(*b_copy.find(KeyGetter::Get(v)), static_cast<typename T::value_type>(v));
  }

  // Test range constructor.
  T b_range(const_b.begin(), const_b.end());
  EXPECT_EQ(b_range.size(), const_b.size());
  EXPECT_LE(b_range.height(), const_b.height());
  EXPECT_LE(b_range.internal_nodes(), const_b.internal_nodes());
  EXPECT_LE(b_range.leaf_nodes(), const_b.leaf_nodes());
  for (const auto& v : values) {
    EXPECT_EQ(*b_range.find(KeyGetter::Get(v)), static_cast<typename T::value_type>(v));
  }

  // Test range insertion for values that already exist.
  b_range.insert(b_copy.begin(), b_copy.end());
  b_range.verify();

  // Test range insertion for new values.
  b_range.clear();
  b_range.insert(b_copy.begin(), b_copy.end());
  EXPECT_EQ(b_range.size(), b_copy.size());
  EXPECT_EQ(b_range.height(), b_copy.height());
  EXPECT_EQ(b_range.internal_nodes(), b_copy.internal_nodes());
  EXPECT_EQ(b_range.leaf_nodes(), b_copy.leaf_nodes());
  for (const auto& v : values) {
    EXPECT_EQ(*b_range.find(KeyGetter::Get(v)), static_cast<typename T::value_type>(v));
  }

  // Test assignment to self. Nothing should change.
  b_range = b_range;
  EXPECT_EQ(b_range.size(), b_copy.size());
  EXPECT_EQ(b_range.height(), b_copy.height());
  EXPECT_EQ(b_range.internal_nodes(), b_copy.internal_nodes());
  EXPECT_EQ(b_range.leaf_nodes(), b_copy.leaf_nodes());

  // Test assignment of new values.
  b_range.clear();
  b_range = b_copy;
  EXPECT_EQ(b_range.size(), b_copy.size());
  EXPECT_EQ(b_range.height(), b_copy.height());
  EXPECT_EQ(b_range.internal_nodes(), b_copy.internal_nodes());
  EXPECT_EQ(b_range.leaf_nodes(), b_copy.leaf_nodes());

  // Test swap.
  b_range.clear();
  b_range.swap(b_copy);
  EXPECT_EQ(b_copy.size(), 0);
  EXPECT_EQ(b_range.size(), const_b.size());
  for (const auto& v : values) {
    EXPECT_EQ(*b_range.find(KeyGetter::Get(v)), static_cast<typename T::value_type>(v));
  }
  b_range.swap(b_copy);

  // Test erase via values.
  for (const auto& v : values) {
    mutable_b.erase(KeyGetter::Get(v));
    // Erasing a non-existent key should have no effect.
    EXPECT_EQ(mutable_b.erase(KeyGetter::Get(v)), 0);
  }

  const_b.verify();
  EXPECT_EQ(const_b.internal_nodes(), 0);
  EXPECT_EQ(const_b.leaf_nodes(), 0);
  EXPECT_EQ(const_b.size(), 0);

  // Test erase via iterators.
  mutable_b = b_copy;
  for (const auto& v : values) {
    mutable_b.erase(mutable_b.find(KeyGetter::Get(v)));
  }

  const_b.verify();
  EXPECT_EQ(const_b.internal_nodes(), 0);
  EXPECT_EQ(const_b.leaf_nodes(), 0);
  EXPECT_EQ(const_b.size(), 0);

  // Test insert with hint.
  for (const auto& v : values) {
    mutable_b.insert(mutable_b.upper_bound(KeyGetter::Get(v)), v);
  }

  const_b.verify();

  // Test insert rvalues.
  T b_rvalues;
  for (const auto& v : values) {
    b_rvalues.insert(V(v));
  }
  b_rvalues.verify();

  // Test insert rvalues with hint.
  b_rvalues.clear();
  for (const auto& v : values) {
    b_rvalues.insert(b_rvalues.upper_bound(KeyGetter::Get(v)), V(v));
  }
  b_rvalues.verify();

  // Test dumping of the btree to an ostream. There should be 1 line for each
  // value.
  std::stringstream strm;
  strm << mutable_b.tree();
  EXPECT_EQ(mutable_b.size(), strcount(strm.str(), '\n'));

  // Test range erase.
  mutable_b.erase(mutable_b.begin(), mutable_b.end());
  EXPECT_EQ(mutable_b.size(), 0);
  const_b.verify();

  // First half.
  mutable_b                             = b_copy;
  typename T::iterator mutable_iter_end = mutable_b.begin();
  for (std::size_t i = 0; i < values.size() / 2; ++i) {
    ++mutable_iter_end;
  }
  mutable_b.erase(mutable_b.begin(), mutable_iter_end);
  EXPECT_EQ(mutable_b.size(), values.size() - values.size() / 2);
  const_b.verify();

  // Second half.
  mutable_b                               = b_copy;
  typename T::iterator mutable_iter_begin = mutable_b.begin();
  for (std::size_t i = 0; i < values.size() / 2; ++i) {
    ++mutable_iter_begin;
  }
  mutable_b.erase(mutable_iter_begin, mutable_b.end());
  EXPECT_EQ(mutable_b.size(), values.size() / 2);
  const_b.verify();

  // Second quarter.
  mutable_b          = b_copy;
  mutable_iter_begin = mutable_b.begin();
  for (std::size_t i = 0; i < values.size() / 4; ++i) {
    ++mutable_iter_begin;
  }
  mutable_iter_end = mutable_iter_begin;
  for (std::size_t i = 0; i < values.size() / 4; ++i) {
    ++mutable_iter_end;
  }
  mutable_b.erase(mutable_iter_begin, mutable_iter_end);
  EXPECT_EQ(mutable_b.size(), values.size() - values.size() / 4);
  const_b.verify();

  mutable_b.clear();
}

template <typename T>
void ConstTest() {
  using value_type = typename T::value_type;
  using KeyGetter  = typename KeyOfValue<typename T::key_type, value_type>::type;

  T        mutable_b;
  const T& const_b = mutable_b;

  // Insert a single value into the container and test looking it up.
  value_type value = Generator<value_type>::Generate(2);
  mutable_b.insert(value);
  EXPECT_TRUE(mutable_b.find(KeyGetter::Get(value)) != const_b.end());
  EXPECT_TRUE(const_b.find(KeyGetter::Get(value)) != mutable_b.end());
  EXPECT_EQ(*const_b.lower_bound(KeyGetter::Get(value)), value);
  EXPECT_TRUE(const_b.upper_bound(KeyGetter::Get(value)) == const_b.end());
  EXPECT_EQ(*const_b.equal_range(KeyGetter::Get(value)).first, value);

  // We can only create a non-const iterator from a non-const container.
  typename T::iterator mutable_iter(mutable_b.begin());
  EXPECT_TRUE(mutable_iter == const_b.begin());
  EXPECT_TRUE(mutable_iter != const_b.end());
  EXPECT_TRUE(const_b.begin() == mutable_iter);
  EXPECT_TRUE(const_b.end() != mutable_iter);
  typename T::reverse_iterator mutable_riter(mutable_b.rbegin());
  EXPECT_TRUE(mutable_riter == const_b.rbegin());
  EXPECT_TRUE(mutable_riter != const_b.rend());
  EXPECT_TRUE(const_b.rbegin() == mutable_riter);
  EXPECT_TRUE(const_b.rend() != mutable_riter);

  // We can create a const iterator from a non-const iterator.
  typename T::const_iterator const_iter(mutable_iter);
  EXPECT_TRUE(const_iter == mutable_b.begin());
  EXPECT_TRUE(const_iter != mutable_b.end());
  EXPECT_TRUE(mutable_b.begin() == const_iter);
  EXPECT_TRUE(mutable_b.end() != const_iter);
  typename T::const_reverse_iterator const_riter(mutable_riter);
  EXPECT_EQ(const_riter, mutable_b.rbegin());
  EXPECT_TRUE(const_riter != mutable_b.rend());
  EXPECT_EQ(mutable_b.rbegin(), const_riter);
  EXPECT_TRUE(mutable_b.rend() != const_riter);

  // Make sure various methods can be invoked on a const container.
  const_b.verify();
  EXPECT_FALSE(const_b.empty());
  EXPECT_EQ(const_b.size(), 1);
  EXPECT_GT(const_b.max_size(), 0);
  EXPECT_EQ(const_b.height(), 1);
  EXPECT_EQ(const_b.count(KeyGetter::Get(value)), 1);
  EXPECT_EQ(const_b.contains(KeyGetter::Get(value)), true);
  EXPECT_EQ(const_b.internal_nodes(), 0);
  EXPECT_EQ(const_b.leaf_nodes(), 1);
  EXPECT_EQ(const_b.nodes(), 1);
  EXPECT_GT(const_b.bytes_used(), 0);
  EXPECT_GT(const_b.fullness(), 0);
  EXPECT_GT(const_b.overhead(), 0);
}

template <typename T, typename C, typename V>
void MergeTest(const std::vector<V>& values) {
  T former, later;
  C ans_former, ans_later;

  for (std::size_t i = 0; i < values.size() / 2; ++i) {
    former.insert(former.end(), values[i]);
    ans_former.insert(ans_former.end(), values[i]);
  }

  // empty test
  EXPECT_EQ(later.size(), 0);

  former.merge(later);
  EXPECT_EQ(former.size(), values.size() / 2);
  {
    auto it     = former.begin();
    auto ans_it = ans_former.begin();
    while (it != former.end()) {
      EXPECT_EQ(*it, *ans_it);
      ++it;
      ++ans_it;
    }
  }
  EXPECT_EQ(later.size(), 0);

  later.merge(former);
  EXPECT_EQ(later.size(), values.size() / 2);
  {
    auto it     = later.begin();
    auto ans_it = ans_former.begin();
    while (it != later.end()) {
      EXPECT_EQ(*it, *ans_it);
      ++it;
      ++ans_it;
    }
  }
  EXPECT_EQ(former.size(), 0);

  // reset
  former.swap(later);

  for (std::size_t i = values.size() / 2; i < values.size(); ++i) {
    later.insert(later.end(), values[i]);
    ans_later.insert(ans_later.end(), values[i]);
  }

  printf(
      "      %s fullness=%0.2f  overhead=%0.2f  bytes-per-value=%0.2f\n",
      "merge 1st half:",
      former.fullness(),
      former.overhead(),
      double(former.bytes_used()) / former.size()
  );
  printf(
      "      %s fullness=%0.2f  overhead=%0.2f  bytes-per-value=%0.2f\n",
      "merge 2st half:",
      later.fullness(),
      later.overhead(),
      double(later.bytes_used()) / later.size()
  );

  former.merge(later);
  ans_former.merge(ans_later);

  assert(former.size() == ans_former.size());
  {
    auto it     = former.begin();
    auto ans_it = ans_former.begin();
    while (it != former.end()) {
      EXPECT_EQ(*it, *ans_it);
      ++it;
      ++ans_it;
    }
  }
  assert(later.size() == ans_later.size());
  {
    auto it     = later.begin();
    auto ans_it = ans_later.begin();
    while (it != later.end()) {
      EXPECT_EQ(*it, *ans_it);
      ++it;
      ++ans_it;
    }
  }

  printf(
      "      %s fullness=%0.2f  overhead=%0.2f  bytes-per-value=%0.2f\n",
      "merged:    ",
      former.fullness(),
      former.overhead(),
      double(former.bytes_used()) / former.size()
  );
}

template <typename T, typename C>
void BtreeTest() {
  ConstTest<T>();

  using V                      = typename std::remove_const<typename T::value_type>::type;
  std::vector<V> random_values = GenerateValues<V>(test_values);

  unique_checker<T, C> container;

  // TODO
  auto comp = C{}.value_comp();

  // Test key insertion/deletion in sorted order.
  std::vector<V> sorted_values(random_values);
  sort(sorted_values.begin(), sorted_values.end(), comp);
  DoTest("sorted:    ", &container, sorted_values);

  // Test key insertion/deletion in reverse sorted order.
  reverse(sorted_values.begin(), sorted_values.end());
  DoTest("rsorted:   ", &container, sorted_values);

  // Test key insertion/deletion in random order.
  DoTest("random:    ", &container, random_values);

  // Test merging a B-tree with unique values.
  printf("    sorted:\n");
  MergeTest<T, C>(sorted_values);

  std::vector<V> duplicate_values;
  duplicate_values.reserve(sorted_values.size());
  for (std::size_t i = 0; i < sorted_values.size() / 2; ++i) {
    duplicate_values.push_back(sorted_values[i]);
  }
  for (std::size_t i = sorted_values.size() / 2; i < sorted_values.size(); ++i) {
    duplicate_values.push_back(sorted_values[i]);
  }

  // Test merging a B-tree with duplicate values.
  printf("    duplicated:\n");
  MergeTest<T, C>(duplicate_values);
}

template <typename T, typename C>
void BtreeMultiTest() {
  ConstTest<T>();

  using V                             = typename std::remove_const<typename T::value_type>::type;
  const std::vector<V>& random_values = GenerateValues<V>(test_values);

  multi_checker<T, C> container;

  // TODO
  auto comp = C{}.value_comp();

  // Test keys in sorted order.
  std::vector<V> sorted_values(random_values);
  sort(sorted_values.begin(), sorted_values.end(), comp);
  DoTest("sorted:    ", &container, sorted_values);

  // Test keys in reverse sorted order.
  reverse(sorted_values.begin(), sorted_values.end());
  DoTest("rsorted:   ", &container, sorted_values);

  // Test keys in random order.
  DoTest("random:    ", &container, random_values);

  // Test keys in random order w/ duplicates.
  std::vector<V> duplicate_values(random_values);
  duplicate_values.insert(duplicate_values.end(), random_values.begin(), random_values.end());
  DoTest("duplicates:", &container, duplicate_values);

  // Test all identical keys.
  std::vector<V> identical_values(100);
  fill(identical_values.begin(), identical_values.end(), Generator<V>::Generate(2));
  DoTest("identical: ", &container, identical_values);

  // Test merging a B-tree with unique values.
  printf("    sorted:\n");
  MergeTest<T, C>(sorted_values);

  // Test merging a B-tree with duplicate values.
  printf("    duplicated:\n");
  MergeTest<T, C>(duplicate_values);
}

template <typename T, typename Alloc = std::allocator<T>>
class TestAllocator : public Alloc {
 public:
  using pointer   = T*;
  using size_type = typename Alloc::size_type;

  TestAllocator() : bytes_used_(nullptr) {}
  TestAllocator(int64_t* bytes_used) : bytes_used_(bytes_used) {}

  // Constructor used for rebinding
  template <class U>
  TestAllocator(const TestAllocator<U>& x) : Alloc(x), bytes_used_(x.bytes_used()) {}

  pointer allocate(size_type n) {
    EXPECT_TRUE(bytes_used_ != nullptr);
    *bytes_used_ += n * sizeof(T);
    return Alloc::allocate(n);
  }

  void deallocate(pointer p, size_type n) {
    Alloc::deallocate(p, n);
    EXPECT_TRUE(bytes_used_ != nullptr);
    *bytes_used_ -= n * sizeof(T);
  }

  // Rebind allows an allocator<T> to be used for a different type
  template <class U>
  struct rebind {
    using other = TestAllocator<U, typename std::allocator_traits<Alloc>::template rebind_alloc<U>>;
  };

  int64_t* bytes_used() const { return bytes_used_; }

 private:
  int64_t* bytes_used_;
};

template <typename T>
void BtreeAllocatorTest() {
  using value_type     = typename T::value_type;
  using allocator_type = typename T::allocator_type;

  int64_t alloc1 = 0;
  int64_t alloc2 = 0;
  auto    b1     = T{allocator_type{&alloc1}};
  auto    b2     = T{allocator_type{&alloc2}};

  // This should swap the allocators!
  swap(b1, b2);

  for (std::size_t i = 0; i < 1000; i++) {
    b1.insert(Generator<value_type>::Generate(i));
  }

  // We should have allocated out of alloc2!
  EXPECT_LE(b1.bytes_used(), alloc2 + sizeof(b1));
  EXPECT_GT(alloc2, alloc1);
}

template <typename T>
void BtreeMapTest() {
  using value_type  = typename T::value_type;
  using mapped_type = typename T::mapped_type;

  mapped_type m = Generator<mapped_type>::Generate(0);
  (void)m;

  T b;

  // Verify we can insert using operator[].
  std::pair<std::remove_const_t<typename value_type::first_type>, mapped_type> min, max;
  auto                                                                         comp = b.key_comp();
  for (std::size_t i = 0; i < 1000; i++) {
    value_type v = Generator<value_type>::Generate(i);
    if (i == 0) {
      min = v;
      max = v;
    } else if (comp(v.first, min.first)) {
      min = v;
    } else if (comp(max.first, v.first)) {
      max = v;
    }
    b[v.first] = v.second;
  }
  EXPECT_EQ(b.size(), 1000);

  // Test whether we can use the "->" operator on iterators and
  // reverse_iterators. This stresses the btree_map_params::pair_pointer
  // mechanism.
  EXPECT_EQ(b.begin()->first, min.first);
  EXPECT_EQ(b.begin()->second, min.second);
  EXPECT_EQ(b.rbegin()->first, max.first);
  EXPECT_EQ(b.rbegin()->second, max.second);
}

template <typename T>
void BtreeMultiMapTest() {
  using mapped_type = typename T::mapped_type;
  mapped_type m     = Generator<mapped_type>::Generate(0);
  (void)m;
}

// Verify that swapping btrees swaps the key comparision functors.
struct SubstringLess {
  SubstringLess() : n(2) {}
  SubstringLess(size_t length) : n(length) {}
  auto operator()(const std::string& a, const std::string& b) const {
    std::string_view as(a.data(), std::min(n, a.size()));
    std::string_view bs(b.data(), std::min(n, b.size()));
    return as <=> bs;
  }
  size_t n;
};

// Test using a class that doesn't implement any comparison operator as key.
struct Vec2i {
  static constexpr std::size_t N = 2;

  int a[N];
};

bool operator==(const Vec2i& lhd, const Vec2i& rhd) {
  for (std::size_t i = 0; i < Vec2i::N; ++i) {
    if (lhd.a[i] != rhd.a[i]) {
      return false;
    }
  }
  return true;
}

bool operator!=(const Vec2i& lhd, const Vec2i& rhd) { return not(lhd == rhd); }

template <>
struct Generator<Vec2i> {
  static Vec2i Generate(std::size_t n) {
    Vec2i v;
    for (std::size_t i = 0; i < Vec2i::N; ++i) {
      v.a[i] = n;
    }
    return v;
  }
};

struct Vec2iComp {
  static constexpr int N = Vec2i::N;

  bool comp(const Vec2i& lhd, const Vec2i& rhd, int i) const noexcept {
    assert(0 <= i && i <= N);

    if (i == N) {
      return false;
    }
    if (lhd.a[i] < rhd.a[i]) {
      return true;
    } else if (lhd.a[i] == rhd.a[i]) {
      return comp(lhd, rhd, i + 1);
    } else {
      return false;
    }
  }

  bool operator()(const Vec2i& lhd, const Vec2i& rhd) const noexcept { return comp(lhd, rhd, 0); }
};

#define BTREE_TEST(test_func, name)                                  \
  TEST(Btree##name, int32_3) { test_func<int32_t, 3>(); }            \
  TEST(Btree##name, int64_3) { test_func<int64_t, 3>(); }            \
  TEST(Btree##name, string_3) { test_func<std::string, 3>(); }       \
  TEST(Btree##name, pair_3) { test_func<std::pair<int, int>, 3>(); } \
  TEST(Btree##name, int32_64) { test_func<int32_t, 64>(); }          \
  TEST(Btree##name, int32_128) { test_func<int32_t, 128>(); }        \
  TEST(Btree##name, int32_256) { test_func<int32_t, 256>(); }        \
  TEST(Btree##name, string_64) { test_func<std::string, 64>(); }     \
  TEST(Btree##name, string_128) { test_func<std::string, 128>(); }   \
  TEST(Btree##name, string_256) { test_func<std::string, 256>(); }
}  // namespace platanus

namespace std {
ostream& operator<<(ostream& os, const platanus::Vec2i& v) {
  os << "(" << v.a[0] << "," << v.a[1] << ")";
  return os;
}
}  // namespace std

#endif  // PLATANUS_BTREE_TEST_H_
