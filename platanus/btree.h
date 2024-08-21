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

#ifndef PLATANUS_BTREE_H__
#define PLATANUS_BTREE_H__

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <ostream>
#include <string>
#include <sys/types.h>
#include <type_traits>
#include <utility>

#include "btree_iterator.h"
#include "btree_node.h"
#include "btree_param.h"
#include "btree_util.h"

namespace platanus {
template <typename Params>
class btree {
  using self_type = btree<Params>;
  using node_type = btree_node<Params>;

  static constexpr std::size_t kNodeValues    = node_type::kNodeValues;
  static constexpr std::size_t kNodeChildren  = node_type::kNodeChildren;
  static constexpr std::size_t kMinNodeValues = kNodeValues / 2;

  struct node_stats {
    node_stats(std::size_t l, std::size_t i) : leaf_nodes(l), internal_nodes(i) {}

    node_stats& operator+=(const node_stats& x) {
      leaf_nodes += x.leaf_nodes;
      internal_nodes += x.internal_nodes;
      return *this;
    }

    std::size_t leaf_nodes;
    std::size_t internal_nodes;
  };

  using node_owner         = typename node_type::node_owner;
  using node_borrower      = typename node_type::node_borrower;
  using node_search_result = typename node_type::search_result;

 public:
  using params_type            = Params;
  using key_type               = typename Params::key_type;
  using mapped_type            = typename Params::mapped_type;
  using value_type             = typename Params::value_type;
  using key_compare            = typename Params::key_compare;
  using value_compare          = typename Params::value_compare;
  using pointer                = typename Params::pointer;
  using const_pointer          = typename Params::const_pointer;
  using reference              = typename Params::reference;
  using const_reference        = typename Params::const_reference;
  using size_type              = typename Params::size_type;
  using difference_type        = typename Params::difference_type;
  using iterator               = btree_iterator<node_type, false>;
  using const_iterator         = btree_iterator<node_type, true>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator       = std::reverse_iterator<iterator>;

  using allocator_type = typename node_type::allocator_type;

  using node_allocator_type     = typename node_type::node_allocator_type;
  using children_allocator_type = typename node_type::children_allocator_type;

 public:
  btree()                   = default;
  btree(btree&&)            = default;
  btree& operator=(btree&&) = default;
  ~btree()                  = default;

  btree(const btree&);
  btree& operator=(const btree& x) {
    if (&x == this) {
      // Don't copy onto ourselves.
      return *this;
    }
    copy(x);
    return *this;
  }

  btree(const key_compare& comp, const allocator_type& alloc);
  btree(const btree& x, const allocator_type& alloc) : btree(key_compare{}, alloc) { copy(x); }
  btree(btree&& x, const allocator_type& alloc);

  // Iterator routines.
  iterator begin() noexcept {
    if (borrow_leftmost()) {
      return iterator(borrow_leftmost(), 0);
    } else {
      return iterator();
    }
  }
  const_iterator begin() const noexcept { return cbegin(); }
  const_iterator cbegin() const noexcept {
    return static_cast<const_iterator>(const_cast<btree*>(this)->begin());
  }
  iterator end() noexcept {
    if (borrow_rightmost()) {
      return iterator(borrow_rightmost(), borrow_readonly_rightmost()->values_count());
    } else {
      return iterator();
    }
  }
  const_iterator end() const noexcept { return cend(); }
  const_iterator cend() const noexcept {
    return static_cast<const_iterator>(const_cast<btree*>(this)->end());
  }
  reverse_iterator rbegin() noexcept(noexcept(reverse_iterator(end()))) {
    return reverse_iterator(end());
  }
  const_reverse_iterator rbegin() const noexcept(noexcept(crbegin())) { return crbegin(); }
  const_reverse_iterator crbegin() const noexcept(noexcept(const_reverse_iterator(end()))) {
    return const_reverse_iterator(end());
  }
  reverse_iterator rend() noexcept(noexcept(reverse_iterator(begin()))) {
    return reverse_iterator(begin());
  }
  const_reverse_iterator rend() const noexcept(noexcept(crend())) { return crend(); }
  const_reverse_iterator crend() const noexcept(noexcept(const_reverse_iterator(begin()))) {
    return const_reverse_iterator(begin());
  }

  // Finds the first element whose key is not less than key.
  template <bool IsUnique = true>
  iterator internal_lower_bound(const key_type& key) {
    if (not borrow_readonly_root()) {
      return end();
    }
    auto iter = iterator(borrow_root(), 0);
    for (;;) {
      auto result   = iter.node->lower_bound(key, ref_key_comp());
      iter.position = result.index();
      if (iter.node->leaf()) {
        break;
      }
      if constexpr (IsUnique) {
        if (result.is_exact_match()) {
          break;
        }
      }
      iter.node = iter.node->borrow_child(iter.position);
    }
    return internal_end(internal_last(iter));
  }
  iterator       lower_bound_unique(const key_type& key) { return internal_lower_bound(key); }
  const_iterator lower_bound_unique(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree*>(this)->lower_bound_unique(key));
  }

  iterator       lower_bound_multi(const key_type& key) { return internal_lower_bound<false>(key); }
  const_iterator lower_bound_multi(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree*>(this)->lower_bound_multi(key));
  }

  // Finds the first element whose key is greater than key.
  iterator upper_bound(const key_type& key) {
    if (not borrow_readonly_root()) {
      return end();
    }
    auto iter = iterator(borrow_root(), 0);
    for (;;) {
      iter.position = iter.node->upper_bound(key, ref_key_comp()).index();
      if (iter.node->leaf()) {
        break;
      }
      iter.node = iter.node->borrow_child(iter.position);
    }
    return internal_end(internal_last(iter));
  }
  const_iterator upper_bound(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree*>(this)->upper_bound(key));
  }

  // Finds the range of values which compare equal to key. The first member of
  // the returned pair is equal to lower_bound(key). The second member pair of
  // the pair is equal to upper_bound(key).
  std::pair<iterator, iterator> equal_range_unique(const key_type& key) {
    return std::make_pair(lower_bound_unique(key), upper_bound(key));
  }
  std::pair<const_iterator, const_iterator> equal_range_unique(const key_type& key) const {
    return static_cast<std::pair<const_iterator, const_iterator>>(
        const_cast<btree*>(this)->equal_range_unique(key)
    );
  }

  std::pair<iterator, iterator> equal_range_multi(const key_type& key) {
    return std::make_pair(lower_bound_multi(key), upper_bound(key));
  }
  std::pair<const_iterator, const_iterator> equal_range_multi(const key_type& key) const {
    return static_cast<std::pair<const_iterator, const_iterator>>(
        const_cast<btree*>(this)->equal_range_multi(key)
    );
  }

  // Inserts a value into the btree only if it does not already exist. The
  // boolean return value indicates whether insertion succeeded or failed.
  std::pair<iterator, bool> insert_unique(const value_type& v) { return internal_insert_unique(v); }
  std::pair<iterator, bool> insert_unique(value_type&& v) {
    return internal_insert_unique(std::move(v));
  }

  // Insert with hint. Check to see if the value should be placed immediately
  // before position in the tree. If it does, then the insertion will take
  // amortized constant time. If not, the insertion will take amortized
  // logarithmic time as if a call to insert_unique(v) were made.
  iterator insert_unique(iterator hint, const value_type& v) {
    return internal_insert_unique(hint, v);
  }
  iterator insert_unique(iterator hint, value_type&& v) {
    return internal_insert_unique(hint, std::move(v));
  }

  // Insert a range of values into the btree.
  template <typename InputIterator>
  void insert_unique(InputIterator b, InputIterator e) {
    for (; b != e; ++b) {
      insert_unique(*b);
    }
  }

  // Insert initialiser list of values into the btree.
  void insert_unique(std::initializer_list<value_type> list) {
    insert_unique(list.begin(), list.end());
  }

  // Inserts a value into the btree.
  iterator insert_multi(const value_type& v) { return internal_insert_multi(v); }
  iterator insert_multi(value_type&& v) { return internal_insert_multi(std::move(v)); }

  // Insert with hint. Check to see if the value should be placed immediately
  // before hint in the tree. If it does, then the insertion will take
  // amortized constant time. If not, the insertion will take amortized
  // logarithmic time as if a call to insert_multi(v) were made.
  iterator insert_multi(iterator hint, const value_type& v) {
    return internal_insert_multi(hint, v);
  }
  iterator insert_multi(iterator hint, value_type&& v) {
    return internal_insert_multi(hint, std::move(v));
  }

  // Insert a range of values into the btree.
  template <typename InputIterator>
  void insert_multi(InputIterator b, InputIterator e) {
    for (; b != e; ++b) {
      insert_multi(*b);
    }
  }

  // Insert initialiser list of values into the btree.
  void insert_multi(std::initializer_list<value_type> list) {
    insert_multi(list.begin(), list.end());
  }

  void copy(const self_type& x);

  // Erase the specified element from the btree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  iterator erase(iterator iter);

  // Erases range. Returns the number of keys erased.
  size_type erase(iterator begin, iterator end);

  // Erases the specified key from the btree. Returns 1 if an element was
  // erased and 0 otherwise.
  size_type erase_unique(const key_type& key);

  // Erases all of the entries matching the specified key from the
  // btree. Returns the number of elements erased.
  size_type erase_multi(const key_type& key);

  // Finds the iterator corresponding to a key or returns end() if the key is
  // not present.
  iterator find_unique(const key_type& key) {
    return internal_end(internal_find_unique(key, iterator(borrow_root(), 0)));
  }
  const_iterator find_unique(const key_type& key) const {
    return internal_end(internal_find_unique(key, const_iterator(borrow_readonly_root(), 0)));
  }
  iterator find_multi(const key_type& key) {
    iterator iter = lower_bound_multi(key);
    if (iter != end() && !compare_keys(key, iter.key())) {
      return iter;
    } else {
      return end();
    }
  }
  const_iterator find_multi(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree*>(this)->find_multi(key));
  }

  // Returns a count of the number of times the key appears in the btree.
  size_type count_unique(const key_type& key) const {
    const_iterator begin = internal_find_unique(key, const_iterator(borrow_readonly_root(), 0));
    if (!begin.node) {
      // The key doesn't exist in the tree.
      return 0;
    }
    return 1;
  }
  // Returns a count of the number of times the key appears in the btree.
  size_type count_multi(const key_type& key) const {
    return distance(lower_bound_multi(key), upper_bound(key));
  }

  // Clear the btree, deleting all of the values it contains.
  void clear() {
    root_      = nullptr;
    rightmost_ = nullptr;
    leftmost_  = nullptr;
    size_      = 0;
  }

  // Swap the contents of *this and x.
  void swap(self_type& x);

  key_compare key_comp() const noexcept { return comp_; }

  bool compare_keys(const key_type& x, const key_type& y) const {
    const auto& comp = ref_key_comp();
    if constexpr (comp_return_weak_ordering<key_type, key_compare>) {
      return comp(x, y) < 0;
    } else {
      return comp(x, y);
    }
  }

  // Dump the btree to the specified ostream. Requires that operator<< is
  // defined for Key and Value.
  void dump(std::ostream& os) const {
    if (borrow_readonly_root() != nullptr) {
      internal_dump(os, borrow_readonly_root(), 0);
    }
  }

  // Verifies the structure of the btree.
  void verify() const;

  // Size routines.
  size_type size() const noexcept { return size_; }
  size_type max_size() const noexcept { return std::numeric_limits<size_type>::max(); }
  bool      empty() const noexcept {
         assert((size() == 0) == (borrow_readonly_root() == nullptr));
         return size() == 0;
  }

  // The height of the btree. An empty tree will have height 0.
  size_type height() const noexcept {
    size_type h = 0;
    if (borrow_readonly_root()) {
      // Count the length of the chain from the leftmost node up to the
      // root.
      ++h;
      const node_type* n = borrow_readonly_leftmost();
      while (n != borrow_readonly_root()) {
        ++h;
        n = n->borrow_readonly_parent();
      }
    }
    return h;
  }

  // The number of internal, leaf and total nodes used by the btree.
  size_type leaf_nodes() const noexcept {
    return internal_stats(borrow_readonly_root()).leaf_nodes;
  }
  size_type internal_nodes() const noexcept {
    return internal_stats(borrow_readonly_root()).internal_nodes;
  }
  size_type nodes() const noexcept {
    node_stats stats = internal_stats(borrow_readonly_root());
    return stats.leaf_nodes + stats.internal_nodes;
  }

  // The total number of bytes used by the btree.
  size_type bytes_used() const noexcept {
    node_stats stats = internal_stats(borrow_readonly_root());
    if (stats.leaf_nodes == 1 && stats.internal_nodes == 0) {
      return sizeof(*this) + sizeof(node_type);
    } else {
      return sizeof(*this) + sizeof(node_type) * (stats.leaf_nodes + stats.internal_nodes)
             + stats.internal_nodes * sizeof(node_owner) * kNodeChildren;
    }
  }

  // The average number of bytes used per value stored in the btree.
  double average_bytes_per_value() const noexcept { return bytes_used() / size(); }

  // The fullness of the btree. Computed as the number of elements in the btree
  // divided by the maximum number of elements a tree with the current number
  // of nodes could hold. A value of 1 indicates perfect space
  // utilization. Smaller values indicate space wastage.
  double fullness() const noexcept { return double(size()) / (nodes() * kNodeValues); }
  // The overhead of the btree structure in bytes per node. Computed as the
  // total number of bytes used by the btree minus the number of bytes used for
  // storing elements divided by the number of elements.
  double overhead() const noexcept {
    if (empty()) {
      return 0.0;
    }
    return (bytes_used() - size() * sizeof(mapped_type)) / double(size());
  }

  // Merges an B-tree. The given B-tree will have the intersection of two B-trees.
  void merge_unique(self_type& rhd) {
    if (empty()) {
      swap(rhd);
      return;
    }
    if (rhd.empty()) {
      return;
    }

    auto lhd_min = params_type::key(*(begin()));
    auto lhd_max = params_type::key(*(rbegin()));
    auto rhd_min = params_type::key(*(rhd.begin()));
    auto rhd_max = params_type::key(*(rhd.rbegin()));

    const auto& comp = ref_key_comp();

    // If lhd_min >= rhd_min,
    if (!comp(lhd_min, rhd_min)) {
      swap(rhd);
    }

    // If the value range of rhd is included in that of *this,
    // rhd_max <= lhd_max
    if (!comp(lhd_max, rhd_max)) {
      // Store the keys removed from rhd temporarily.
      std::vector<std::size_t> removed_indexes;

      // Insert the intersection to *this.
      std::size_t i = 0;
      for (auto it = rhd.begin(); it != rhd.end(); ++it) {
        auto [result_it, is_inserted] = insert_unique(std::move(*it));
        if (is_inserted) {
          removed_indexes.push_back(i);
        }
        ++i;
      }

      // Remove the keys inserted to *this.
      for (auto it = removed_indexes.rbegin(); it != removed_indexes.rend(); ++it) {
        iterator hint;
        if (*it < rhd.size() / 2) {
          hint = std::next(rhd.begin(), *it);
        } else {
          hint = std::prev(rhd.end(), rhd.size() - *it);
        }
        rhd.erase(hint);
      }

      return;
    }

    // Store the keys removed from rhd temporarily.
    std::vector<std::size_t> removed_indexes;

    auto        it = rhd.begin();
    std::size_t i  = 0;
    // Insert the intersection to *this.
    auto rhd_intersection_end = rhd.upper_bound(lhd_max);
    for (; it != rhd_intersection_end; ++it) {
      auto [result_it, is_inserted] = insert_unique(std::move(*it));
      if (is_inserted) {
        removed_indexes.push_back(i);
      }
      ++i;
    }
    // Insert the extra of rhd.
    for (; it != rhd.end(); ++it) {
      insert_unique(end(), std::move(*it));
    }

    // Remove the extra of rhd.
    while (!rhd.empty() && comp(lhd_max, params_type::key(*(rhd.rbegin())))) {
      rhd.erase(std::prev(rhd.end()));
    }
    // Remove the keys inserted to *this.
    for (auto it = removed_indexes.rbegin(); it != removed_indexes.rend(); ++it) {
      iterator hint;
      if (*it < rhd.size() / 2) {
        hint = std::next(rhd.begin(), *it);
      } else {
        hint = std::prev(rhd.end(), rhd.size() - *it);
      }
      rhd.erase(hint);
    }
  }

  void merge_multi(self_type& rhd) {
    if (empty()) {
      swap(rhd);
      return;
    }
    if (rhd.empty()) {
      return;
    }

    auto lhd_min = params_type::key(*(begin()));
    auto lhd_max = params_type::key(*(rbegin()));
    auto rhd_min = params_type::key(*(rhd.begin()));

    // If lhd_min >= rhd_min,
    if (!comp(lhd_min, rhd_min)) {
      swap(rhd);
    }

    // Insert the intersection to *this.
    auto rhd_intersection_end = rhd.upper_bound(lhd_max);
    auto it                   = rhd.begin();
    for (; it != rhd_intersection_end; ++it) {
      insert_multi(std::move(*it));
    }
    // Insert the extra of rhd.
    for (; it != rhd.end(); ++it) {
      insert_multi(end(), std::move(*it));
    }

    rhd.clear();
  }

 private:
  // Internal accessor routines.
  node_borrower       borrow_root() noexcept { return root_->borrow_myself(); }
  const node_borrower borrow_readonly_root() const noexcept {
    return static_cast<const node_borrower>(const_cast<btree*>(this)->borrow_root());
  }
  node_owner extract_root() noexcept { return std::move(root_); }
  void       set_root(node_owner&& node) noexcept { root_ = std::move(node); }

  // Getter/Setter for the rightmost node in the tree.
  node_borrower       borrow_rightmost() noexcept { return rightmost_; }
  const node_borrower borrow_readonly_rightmost() const noexcept {
    return static_cast<const node_borrower>(const_cast<btree*>(this)->borrow_rightmost());
  }
  void set_rightmost(node_borrower node) noexcept { rightmost_ = node; }
  void set_leftmost(node_borrower node) noexcept { leftmost_ = node; }

  // The leftmost node is stored as the parent of the root node.
  node_borrower       borrow_leftmost() noexcept { return leftmost_; }
  const node_borrower borrow_readonly_leftmost() const noexcept {
    return static_cast<const node_borrower>(const_cast<btree*>(this)->borrow_leftmost());
  }

  // Getter/Setter for the size of the tree.
  void set_size(size_type size) noexcept { size_ = size; }

  node_allocator_type&     ref_node_alloc() noexcept { return node_alloc_; }
  children_allocator_type& ref_children_alloc() noexcept { return children_alloc_; }

  key_compare&       ref_key_comp() noexcept { return comp_; }
  const key_compare& ref_key_comp() const noexcept { return comp_; }

  // Node creation/deletion routines.
  node_owner make_internal_node(node_borrower parent) {
    return node_type::make_node(false, parent, ref_node_alloc(), ref_children_alloc());
  }
  node_owner make_internal_root_node() {
    return node_type::make_root_node(false, ref_node_alloc(), ref_children_alloc());
  }
  node_owner make_leaf_node(node_borrower parent) {
    return node_type::make_node(true, parent, ref_node_alloc(), ref_children_alloc());
  }
  node_owner make_leaf_root_node() {
    return node_type::make_root_node(true, ref_node_alloc(), ref_children_alloc());
  }

  // Rebalances or splits the node iter points to.
  void rebalance_or_split(iterator& iter);

  // Merges the values of left, right and the delimiting key on their parent
  // onto left, removing the delimiting key and deleting right.
  void merge_nodes(node_borrower left, node_borrower right);

  // Tries to merge node with its left or right sibling, and failing that,
  // rebalance with its left or right sibling. Returns true if a merge
  // occurred, at which point it is no longer valid to access node. Returns
  // false if no merging took place.
  bool try_merge_or_rebalance(iterator& iter);

  // Tries to shrink the height of the tree by 1.
  void try_shrink();

  iterator       internal_end(iterator iter) { return iter.node ? iter : end(); }
  const_iterator internal_end(const_iterator iter) const { return iter.node ? iter : end(); }

  // Inserts a value into the btree immediately before iter. Requires that
  // key(v) <= iter.key() and (--iter).key() <= key(v).
  template <typename T>
  iterator internal_insert(iterator iter, T&& v);

  template <typename T>
  std::pair<iterator, bool> internal_insert_unique(T&& value);

  template <typename T>
  iterator internal_insert_unique(iterator position, T&& v);

  template <typename T>
  iterator internal_insert_multi(T&& value);

  template <typename T>
  iterator internal_insert_multi(iterator position, T&& value);

  // Returns an iterator pointing to the first value >= the value "iter" is
  // pointing at. Note that "iter" might be pointing to an invalid location as
  // iter.position == iter.node->count(). This routine simply moves iter up in
  // the tree to a valid location.
  template <typename IterType>
  IterType internal_last(IterType iter);

  // Returns an iterator pointing to the leaf position at which key would
  // reside in the tree. This function returns either true (if the
  // key was found in the tree) or false (if it wasn't) in the second
  // field of the pair.
  template <typename IterType>
  std::pair<IterType, bool> internal_locate(const key_type& key, IterType iter) const;

  // Internal routine which implements find_unique().
  template <typename IterType>
  IterType internal_find_unique(const key_type& key, IterType iter) const;

  // Dumps a node and all of its children to the specified ostream.
  void internal_dump(std::ostream& os, const node_borrower node, int level) const;

  // Verifies the tree structure of node.
  int internal_verify(const node_borrower node, const key_type* lo, const key_type* hi) const;

  node_stats internal_stats(const node_borrower node) const noexcept {
    if (!node) {
      return node_stats(0, 0);
    }
    if (node->leaf()) {
      return node_stats(1, 0);
    }
    node_stats res(0, 1);
    for (size_type i = 0; i <= node->count(); ++i) {
      res += internal_stats(node->borrow_readonly_child(i));
    }
    return res;
  }

 private:
  key_compare             comp_{};
  node_allocator_type     node_alloc_{};
  children_allocator_type children_alloc_{};
  node_owner              root_{};
  // A pointer to the rightmost node of the tree
  node_borrower rightmost_{nullptr};
  // A pointer to the leftmost node of the tree
  node_borrower leftmost_{nullptr};
  // The size of the tree.
  size_type size_{0};
};

////
// btree methods
template <typename P>
btree<P>::btree(const key_compare& comp, const allocator_type& alloc)
    : comp_(comp),
      node_alloc_(alloc),
      children_alloc_(alloc),
      root_(),
      rightmost_(nullptr),
      leftmost_(nullptr),
      size_(0) {}

template <typename P>
btree<P>::btree(const self_type& x)
    : comp_(x.comp_),
      node_alloc_(std::allocator_traits<node_allocator_type>::select_on_container_copy_construction(
          x.node_alloc_
      )),
      children_alloc_(
          std::allocator_traits<children_allocator_type>::select_on_container_copy_construction(
              x.children_alloc_
          )
      ),
      root_(),
      rightmost_(x.rightmost_),
      leftmost_(x.leftmost_),
      size_(x.size_) {
  copy(x);
}

template <typename P>
btree<P>::btree(self_type&& x, const allocator_type& alloc) : btree(std::move(x)) {
  node_alloc_     = alloc;
  children_alloc_ = alloc;
}

template <typename P>
template <typename T>
std::pair<typename btree<P>::iterator, bool> btree<P>::internal_insert_unique(T&& value) {
  if (empty()) {
    set_root(make_leaf_root_node());
    set_rightmost(borrow_root());
    set_leftmost(borrow_root());
  }

  const auto&               key  = params_type::key(value);
  std::pair<iterator, bool> res  = internal_locate(key, iterator(borrow_root(), 0));
  iterator&                 iter = res.first;
  if (res.second) {
    // The key already exists in the tree, do nothing.
    return std::make_pair(iter, false);
  }

  return std::make_pair(internal_insert(iter, std::forward<T>(value)), true);
}

template <typename P>
template <typename T>
typename btree<P>::iterator btree<P>::internal_insert_unique(iterator hint, T&& v) {
  if (!empty()) {
    const key_type& key = params_type::key(v);
    if (hint == end() || compare_keys(key, hint.key())) {
      iterator prev = hint;
      if (hint == begin() || compare_keys((--prev).key(), key)) {
        // prev.key() < key < hint.key()
        return internal_insert(hint, std::forward<T>(v));
      }
    } else if (compare_keys(hint.key(), key)) {
      iterator next = hint;
      ++next;
      if (next == end() || compare_keys(key, next.key())) {
        // hint.key() < key < next.key()
        return internal_insert(next, std::forward<T>(v));
      }
    } else {
      // hint.key() == key
      return hint;
    }
  }
  return insert_unique(std::forward<T>(v)).first;
}

template <typename P>
template <typename T>
typename btree<P>::iterator btree<P>::internal_insert_multi(T&& value) {
  if (empty()) {
    set_root(make_leaf_root_node());
    set_rightmost(borrow_root());
    set_leftmost(borrow_root());
  }

  return internal_insert(upper_bound(params_type::key(value)), std::forward<T>(value));
}

template <typename P>
template <typename T>
typename btree<P>::iterator btree<P>::internal_insert_multi(iterator hint, T&& v) {
  if (!empty()) {
    const key_type& key = params_type::key(v);
    if (hint == end() || !compare_keys(hint.key(), key)) {
      iterator prev = hint;
      if (hint == begin() || !compare_keys(key, (--prev).key())) {
        // prev.key() <= key <= hint.key()
        return internal_insert(hint, std::forward<T>(v));
      }
    } else {
      iterator next = hint;
      ++next;
      if (next == end() || !compare_keys(next.key(), key)) {
        // hint.key() < key <= next.key()
        return internal_insert(next, std::forward<T>(v));
      }
    }
  }
  return insert_multi(std::forward<T>(v));
}

template <typename P>
void btree<P>::copy(const self_type& x) {
  clear();

  // Assignment can avoid key comparisons because we know the order of the
  // values is the same order we'll store them in.
  const_iterator iter = x.cbegin();
  if (iter != x.cend()) {
    insert_multi(*iter);
    ++iter;
  }
  while (iter != x.cend()) {
    // If the btree is not empty, we can just insert the new value at the end
    // of the tree!
    internal_insert(end(), *iter);
    ++iter;
  }
}

template <typename P>
typename btree<P>::iterator btree<P>::erase(iterator iter) {
  bool internal_delete = false;
  if (!iter.node->leaf()) {
    // Deletion of a value on an internal node. Swap the key with the largest
    // value of our left child. This is easy, we just decrement iter.
    iterator tmp_iter(iter--);
    assert(iter.node->leaf());
    assert(!compare_keys(tmp_iter.key(), iter.key()));
    iter.node->value_swap(iter.position, tmp_iter.node, tmp_iter.position);
    internal_delete = true;
  }
  set_size(size() - 1);

  // Delete the key from the leaf.
  iter.node->remove_value(iter.position);

  // We want to return the next value after the one we just erased. If we
  // erased from an internal node (internal_delete == true), then the next
  // value is ++(++iter). If we erased from a leaf node (internal_delete ==
  // false) then the next value is ++iter. Note that ++iter may point to an
  // internal node and the value in the internal node may move to a leaf node
  // (iter.node) when rebalancing is performed at the leaf level.

  // Merge/rebalance as we walk back up the tree.
  iterator res(iter);
  for (;;) {
    if (iter.node == borrow_root()) {
      try_shrink();
      if (empty()) {
        return end();
      }
      break;
    }
    if (iter.node->count() >= kMinNodeValues) {
      break;
    }
    bool merged = try_merge_or_rebalance(iter);
    if (iter.node->leaf()) {
      res = iter;
    }
    if (!merged) {
      break;
    }
    iter.node = iter.node->borrow_parent();
  }

  // Adjust our return value. If we're pointing at the end of a node, advance
  // the iterator.
  if (res.position == res.node->count()) {
    res.position = res.node->count() - 1;
    ++res;
  }
  // If we erased from an internal node, advance the iterator.
  if (internal_delete) {
    ++res;
  }
  return res;
}

template <typename P>
typename btree<P>::size_type btree<P>::erase(iterator begin, iterator end) {
  size_type count = distance(begin, end);
  for (size_type i = 0; i < count; i++) {
    begin = erase(begin);
  }
  return count;
}

template <typename P>
typename btree<P>::size_type btree<P>::erase_unique(const key_type& key) {
  iterator iter = internal_find_unique(key, iterator(borrow_root(), 0));
  if (!iter.node) {
    // The key doesn't exist in the tree, return nothing done.
    return 0;
  }
  erase(iter);
  return 1;
}

template <typename P>
typename btree<P>::size_type btree<P>::erase_multi(const key_type& key) {
  iterator begin = lower_bound_multi(key);
  if (begin == end()) {
    // The key doesn't exist in the tree, return nothing done.
    return 0;
  }
  // Delete all of the keys between begin and upper_bound(key).
  return erase(begin, upper_bound(key));
}

template <typename P>
void btree<P>::swap(self_type& x) {
  btree_swap_helper(comp_, x.comp_);
  btree_swap_helper(root_, x.root_);
  btree_swap_helper(node_alloc_, x.node_alloc_);
  btree_swap_helper(children_alloc_, x.children_alloc_);
  btree_swap_helper(rightmost_, x.rightmost_);
  btree_swap_helper(leftmost_, x.leftmost_);
  btree_swap_helper(size_, x.size_);
}

template <typename P>
void btree<P>::verify() const {
  if (borrow_readonly_root() != nullptr) {
    assert(size() == internal_verify(borrow_readonly_root(), nullptr, nullptr));
    auto p = (++const_iterator(borrow_readonly_root(), -1)).node;
    assert(borrow_readonly_leftmost() == p);
    assert(
        borrow_readonly_rightmost()
        == (--const_iterator(borrow_readonly_root(), borrow_readonly_root()->count())).node
    );
    assert(borrow_readonly_leftmost()->leaf());
    assert(borrow_readonly_rightmost()->leaf());
  } else {
    assert(size() == 0);
    assert(borrow_readonly_leftmost() == nullptr);
    assert(borrow_readonly_rightmost() == nullptr);
  }
}

template <typename P>
void btree<P>::rebalance_or_split(iterator& iter) {
  node_borrower& node            = iter.node;
  auto&          insert_position = iter.position;
  assert(node->values_count() == node->max_values_count());

  // First try to make room on the node by rebalancing.
  auto parent = node->borrow_parent();
  if (node != borrow_readonly_root()) {
    if (node->position() > 0) {
      // Try rebalancing with our left sibling.
      auto left = parent->borrow_child(node->position() - 1);
      if (left->values_count() < left->max_values_count()) {
        // We bias rebalancing based on the position being inserted. If we're
        // inserting at the end of the right node then we bias rebalancing to
        // fill up the left node.
        auto to_move = (left->max_values_count() - left->values_count())
                       / (1 + (insert_position < node->max_values_count() ? 1 : 0));
        to_move = std::max(1, to_move);

        if (((insert_position - to_move) >= 0)
            || ((left->values_count() + to_move) < left->max_values_count())) {
          left->rebalance_right_to_left(node, to_move);

          assert(node->max_values_count() - node->values_count() == to_move);
          insert_position = insert_position - to_move;
          if (insert_position < 0) {
            insert_position = insert_position + left->values_count() + 1;
            node            = left;
          }

          assert(node->values_count() < node->max_values_count());
          return;
        }
      }
    }

    if (node->position() < parent->values_count()) {
      // Try rebalancing with our right sibling.
      auto right = parent->borrow_child(node->position() + 1);
      if (right->values_count() < right->max_values_count()) {
        // We bias rebalancing based on the position being inserted. If we're
        // inserting at the beginning of the left node then we bias rebalancing
        // to fill up the right node.
        auto to_move = (right->max_values_count() - right->values_count())
                       / (1 + (insert_position > 0 ? 1 : 0));
        to_move = std::max(1, to_move);

        if ((insert_position <= (node->values_count() - to_move))
            || ((right->values_count() + to_move) < right->max_values_count())) {
          node->rebalance_left_to_right(right, to_move);

          if (insert_position > node->values_count()) {
            insert_position = insert_position - node->values_count() - 1;
            node            = right;
          }

          assert(node->values_count() < node->max_values_count());
          return;
        }
      }
    }

    // Rebalancing failed, make sure there is room on the parent node for a new
    // value.
    if (parent->values_count() == parent->max_values_count()) {
      iterator parent_iter(node->borrow_parent(), node->position());
      rebalance_or_split(parent_iter);
    }
  } else {
    // Rebalancing not possible because this is the root node.
    auto new_root = make_internal_root_node();
    new_root->set_child(0, extract_root());
    set_root(std::move(new_root));
    parent = borrow_root();
    assert(node == borrow_root()->borrow_child(0));
  }

  // Split the node.
  node_owner split_node;
  if (node->leaf()) {
    split_node = make_leaf_node(parent);
    node->split(std::move(split_node), insert_position);
    if (borrow_readonly_rightmost() == node) {
      set_rightmost(node->borrow_parent()->borrow_child(node->position() + 1));
    }
  } else {
    split_node = make_internal_node(parent);
    node->split(std::move(split_node), insert_position);
  }

  if (insert_position > node->values_count()) {
    insert_position = insert_position - node->values_count() - 1;
    node            = node->borrow_parent()->borrow_child(node->position() + 1);
  }
}

template <typename P>
void btree<P>::merge_nodes(node_borrower left, node_borrower right) {
  if (borrow_readonly_rightmost() == right) {
    assert(right->leaf());
    set_rightmost(left);
  }
  left->merge(right);
}

template <typename P>
bool btree<P>::try_merge_or_rebalance(iterator& iter) {
  assert(iter.node != borrow_readonly_root());
  assert(iter.node->count() < kMinNodeValues);

  node_borrower parent = iter.node->borrow_parent();
  if (iter.node->position() > 0) {
    // Try merging with our left sibling.
    node_borrower left = parent->borrow_child(iter.node->position() - 1);
    if ((1 + left->count() + iter.node->count()) <= left->max_count()) {
      iter.position += 1 + left->count();
      merge_nodes(left, iter.node);
      iter.node = left;
      return true;
    }
  }
  if (iter.node->position() < parent->count()) {
    // Try merging with our right sibling.
    node_borrower right = parent->borrow_child(iter.node->position() + 1);
    if ((1 + iter.node->count() + right->count()) <= iter.node->max_count()) {
      merge_nodes(iter.node, right);
      return true;
    }
    // Try rebalancing with our right sibling. We don't perform rebalancing if
    // we deleted the first element from iter.node and the node is not
    // empty. This is a small optimization for the common pattern of deleting
    // from the front of the tree.
    if ((right->count() > kMinNodeValues) && ((iter.node->count() == 0) || (iter.position > 0))) {
      auto to_move = (right->count() - iter.node->count()) / 2;
      to_move      = std::min(to_move, right->count() - 1);
      iter.node->rebalance_right_to_left(right, to_move);
      return false;
    }
  }
  if (iter.node->position() > 0) {
    // Try rebalancing with our left sibling. We don't perform rebalancing if
    // we deleted the last element from iter.node and the node is not
    // empty. This is a small optimization for the common pattern of deleting
    // from the back of the tree.
    node_borrower left = parent->borrow_child(iter.node->position() - 1);
    if ((left->count() > kMinNodeValues)
        && ((iter.node->count() == 0) || (iter.position < iter.node->count()))) {
      auto to_move = (left->count() - iter.node->count()) / 2;
      to_move      = std::min(to_move, left->count() - 1);
      left->rebalance_left_to_right(iter.node, to_move);
      iter.position += to_move;
      return false;
    }
  }
  return false;
}

template <typename P>
void btree<P>::try_shrink() {
  if (borrow_readonly_root()->count() > 0) {
    return;
  }
  // Deleted the last item on the root node, shrink the height of the tree.
  if (borrow_readonly_root()->leaf()) {
    assert(size() == 0);
    clear();
  } else {
    auto child = borrow_root()->extract_child(0);
    // Set parent of child to leftmost node.
    child->make_root();
    // We want to keep the existing root node
    // so we move all of the values from the child node into the existing
    // (empty) root node.
    set_root(std::move(child));
  }
}

template <typename P>
template <typename IterType>
IterType btree<P>::internal_last(IterType iter) {
  while (iter.node && iter.position == iter.node->count()) {
    iter.position = iter.node->position();
    iter.node     = iter.node->borrow_parent();
  }
  return iter;
}

template <typename P>
template <typename T>
typename btree<P>::iterator btree<P>::internal_insert(iterator iter, T&& v) {
  if (!iter.node->leaf()) {
    // We can't insert on an internal node. Instead, we'll insert after the
    // previous value which is guaranteed to be on a leaf node.
    --iter;
    ++iter.position;
  }
  if (iter.node->count() == iter.node->max_count()) {
    // Make a room in the leaf for the new item.
    rebalance_or_split(iter);
  }
  set_size(size() + 1);
  iter.node->insert_value(iter.position, std::forward<T>(v));
  return iter;
}

template <typename P>
template <typename IterType>
std::pair<IterType, bool> btree<P>::internal_locate(const key_type& key, IterType iter) const {
  for (;;) {
    node_search_result res = iter.node->lower_bound(key, ref_key_comp());
    iter.position          = res.index();
    if (res.is_exact_match()) {
      return std::make_pair(iter, true);
    }
    if (iter.node->leaf()) {
      break;
    }
    iter.node = iter.node->borrow_child(iter.position);
  }
  return std::make_pair(iter, false);
}

template <typename P>
template <typename IterType>
IterType btree<P>::internal_find_unique(const key_type& key, IterType iter) const {
  if (iter.node) {
    std::pair<IterType, bool> res = internal_locate(key, iter);
    if (res.second) {
      return res.first;
    }
  }
  return IterType(nullptr, 0);
}

template <typename P>
void btree<P>::internal_dump(std::ostream& os, const node_borrower node, int level) const {
  for (int i = 0; i < node->count(); ++i) {
    if (!node->leaf()) {
      internal_dump(os, node->borrow_readonly_child(i), level + 1);
    }
    for (int j = 0; j < level; ++j) {
      os << "  ";
    }
    os << node->key(i) << " [" << level << "]\n";
  }
  if (!node->leaf()) {
    internal_dump(os, node->borrow_readonly_child(node->count()), level + 1);
  }
}

template <typename P>
int btree<P>::internal_verify(const node_borrower node, const key_type* lo, const key_type* hi)
    const {
  assert(node->count() > 0);
  assert(node->count() <= node->max_count());
  if (lo) {
    assert(!compare_keys(node->key(0), *lo));
  }
  if (hi) {
    assert(!compare_keys(*hi, node->key(node->count() - 1)));
  }
  for (int i = 1; i < node->count(); ++i) {
    assert(!compare_keys(node->key(i), node->key(i - 1)));
  }
  int count = node->count();
  if (!node->leaf()) {
    for (int i = 0; i <= node->count(); ++i) {
      assert(node->borrow_readonly_child(i) != nullptr);
      assert(node->borrow_readonly_child(i)->borrow_readonly_parent() == node);
      assert(node->borrow_readonly_child(i)->position() == i);
      count += internal_verify(
          node->borrow_readonly_child(i),
          (i == 0) ? lo : &node->key(i - 1),
          (i == node->count()) ? hi : &node->key(i)
      );
    }
  }
  return count;
}

}  // namespace platanus

#endif  // PLATANUS_BTREE_H__
