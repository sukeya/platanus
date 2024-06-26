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
// A safe_btree<> wraps around a btree<> and removes the caveat that insertion
// and deletion invalidate iterators. A safe_btree<> maintains a generation
// number that is incremented on every mutation. A safe_btree<>::iterator keeps
// a pointer to the safe_btree<> it came from, the generation of the tree when
// it was last validated and the key the underlying btree<>::iterator points
// to. If an iterator is accessed and its generation differs from the tree
// generation it is revalidated.
//
// References and pointers returned by safe_btree iterators are not safe.
//
// See the incorrect usage examples mentioned in safe_btree_set.h and
// safe_btree_map.h.

#ifndef PLATANUS_SAFE_BTREE_H__
#define PLATANUS_SAFE_BTREE_H__

#include <cstdint>
#include <iosfwd>
#include <initializer_list>
#include <utility>

#include "btree.h"

namespace platanus {

template <typename Tree, typename Iterator>
class safe_btree_iterator {
 public:
  using key_type          = typename Iterator::key_type;
  using value_type        = typename Iterator::value_type;
  using size_type         = typename Iterator::size_type;
  using difference_type   = typename Iterator::difference_type;
  using pointer           = typename Iterator::pointer;
  using reference         = typename Iterator::reference;
  using const_pointer     = typename Iterator::const_pointer;
  using const_reference   = typename Iterator::const_reference;
  using iterator_category = typename Iterator::iterator_category;
  using iterator          = typename Tree::iterator;
  using const_iterator    = typename Tree::const_iterator;
  using self_type         = safe_btree_iterator<Tree, Iterator>;

  void update() const {
    if (iter_ != tree_->internal_btree()->end()) {
      // A positive generation indicates a valid key.
      generation_ = tree_->generation();
      key_        = iter_.key();
    } else {
      // Use a negative generation to indicate iter_ points to end().
      generation_ = -tree_->generation();
    }
  }

 public:
  safe_btree_iterator() : generation_(0), key_(), iter_(), tree_(nullptr) {}
  safe_btree_iterator(const iterator& x)
      : generation_(x.generation()), key_(x.key()), iter_(x.iter()), tree_(x.tree()) {}
  safe_btree_iterator(Tree* tree, const Iterator& iter)
      : generation_(), key_(), iter_(iter), tree_(tree) {
    update();
  }

  Tree*              tree() const { return tree_; }
  std::int_least64_t generation() const { return generation_; }

  Iterator* mutable_iter() const {
    if (generation_ != tree_->generation()) {
      if (generation_ > 0) {
        // This does the wrong thing for a multi{set,map}. If my iter was
        // pointing to the 2nd of 2 values with the same key, then this will
        // reset it to point to the first. This is why we don't provide a
        // safe_btree_multi{set,map}.
        iter_ = tree_->internal_btree()->lower_bound_unique(key_);
        update();
      } else if (-generation_ != tree_->generation()) {
        iter_       = tree_->internal_btree()->end();
        generation_ = -tree_->generation();
      }
    }
    return &iter_;
  }
  const Iterator& iter() const { return *mutable_iter(); }

  // Equality/inequality operators.
  bool operator==(const const_iterator& x) const { return iter() == x.iter(); }
  bool operator!=(const const_iterator& x) const { return iter() != x.iter(); }

  // Accessors for the key/value the iterator is pointing at.
  const key_type& key() const { return key_; }
  // This reference value is potentially invalidated by any non-const
  // method on the tree; it is NOT safe.
  reference operator*() const {
    assert(generation_ > 0);
    return iter().operator*();
  }
  // This pointer value is potentially invalidated by any non-const
  // method on the tree; it is NOT safe.
  pointer operator->() const {
    assert(generation_ > 0);
    return iter().operator->();
  }

  // Increment/decrement operators.
  self_type& operator++() {
    ++(*mutable_iter());
    update();
    return *this;
  }
  self_type& operator--() {
    --(*mutable_iter());
    update();
    return *this;
  }
  self_type operator++(int) {
    self_type tmp = *this;
    ++*this;
    return tmp;
  }
  self_type operator--(int) {
    self_type tmp = *this;
    --*this;
    return tmp;
  }

 private:
  // The generation of the tree when "iter" was updated.
  mutable std::int_least64_t generation_;
  // The key the iterator points to.
  mutable key_type key_;
  // The underlying iterator.
  mutable Iterator iter_;
  // The tree the iterator is associated with.
  Tree* tree_;
};

template <typename Params>
class safe_btree {
  using self_type = safe_btree<Params>;

  using btree_type          = btree<Params>;
  using tree_iterator       = typename btree_type::iterator;
  using tree_const_iterator = typename btree_type::const_iterator;

 public:
  using params_type = typename btree_type::params_type;
  using key_type    = typename btree_type::key_type;
  using mapped_type            = typename btree_type::mapped_type;
  using value_type             = typename btree_type::value_type;
  using key_compare            = typename btree_type::key_compare;
  using value_compare          = typename btree_type::value_compare;
  using allocator_type         = typename btree_type::allocator_type;
  using pointer                = typename btree_type::pointer;
  using const_pointer          = typename btree_type::const_pointer;
  using reference              = typename btree_type::reference;
  using const_reference        = typename btree_type::const_reference;
  using size_type              = typename btree_type::size_type;
  using difference_type        = typename btree_type::difference_type;
  using iterator               = safe_btree_iterator<self_type, tree_iterator>;
  using const_iterator         = safe_btree_iterator<const self_type, tree_const_iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator       = std::reverse_iterator<iterator>;

 public:
  safe_btree() : tree_(), generation_(1) {}
  safe_btree(const self_type& x) : tree_(x.tree_), generation_(1) {}
  safe_btree& operator=(const self_type& x) {
    if (&x == this) {
      // Don't copy onto ourselves.
      return *this;
    }
    ++generation_;
    tree_ = x.tree_;
    return *this;
  }

  safe_btree(self_type&&)            = default;
  safe_btree& operator=(self_type&&) = default;
  ~safe_btree()                      = default;

  safe_btree(const key_compare& comp, const allocator_type& alloc)
      : tree_(comp, alloc), generation_(1) {}

  safe_btree(const self_type& x, const allocator_type& alloc)
      : tree_(x.tree_, alloc), generation_(1) {}
  safe_btree(self_type&& x, const allocator_type& alloc)
      : tree_(std::move(x.tree_), alloc), generation_(x.generation_) {}

  allocator_type get_allocator() const { return tree_.get_allocator(); }

  iterator               begin() { return iterator(this, tree_.begin()); }
  const_iterator         begin() const { return cbegin(); }
  const_iterator         cbegin() const { return const_iterator(this, tree_.cbegin()); }
  iterator               end() { return iterator(this, tree_.end()); }
  const_iterator         end() const { return cend(); }
  const_iterator         cend() const { return const_iterator(this, tree_.cend()); }
  reverse_iterator       rbegin() { return reverse_iterator(end()); }
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator crbegin() const { return const_reverse_iterator(cend()); }
  reverse_iterator       rend() { return reverse_iterator(begin()); }
  const_reverse_iterator rend() const { return crend(); }
  const_reverse_iterator crend() const { return const_reverse_iterator(cbegin()); }

  // Lookup routines.
  iterator       lower_bound_unique(const key_type& key) { return iterator(this, tree_.lower_bound_unique(key)); }
  const_iterator lower_bound_unique(const key_type& key) const {
    return const_iterator(this, tree_.lower_bound_unique(key));
  }
  iterator       upper_bound(const key_type& key) { return iterator(this, tree_.upper_bound(key)); }
  const_iterator upper_bound(const key_type& key) const {
    return const_iterator(this, tree_.upper_bound(key));
  }
  std::pair<iterator, iterator> equal_range_unique(const key_type& key) {
    std::pair<tree_iterator, tree_iterator> p = tree_.equal_range_unique(key);
    return std::make_pair(iterator(this, p.first), iterator(this, p.second));
  }
  std::pair<const_iterator, const_iterator> equal_range_unique(const key_type& key) const {
    std::pair<tree_const_iterator, tree_const_iterator> p = tree_.equal_range_unique(key);
    return std::make_pair(const_iterator(this, p.first), const_iterator(this, p.second));
  }
  iterator       find_unique(const key_type& key) { return iterator(this, tree_.find_unique(key)); }
  const_iterator find_unique(const key_type& key) const {
    return const_iterator(this, tree_.find_unique(key));
  }
  iterator       find_multi(const key_type& key) { return iterator(this, tree_.find_multi(key)); }
  const_iterator find_multi(const key_type& key) const {
    return const_iterator(this, tree_.find_multi(key));
  }
  size_type count_unique(const key_type& key) const { return tree_.count_unique(key); }
  size_type count_multi(const key_type& key) const { return tree_.count_multi(key); }

  // Insertion routines.
  template <class T>
  std::pair<iterator, bool> insert_unique_impl(T&& v) {
    std::pair<tree_iterator, bool> p = tree_.insert_unique(std::forward<T>(v));
    generation_ += p.second;
    return std::make_pair(iterator(this, p.first), p.second);
  }
  std::pair<iterator, bool> insert_unique(const value_type& v) { return insert_unique_impl(v); }
  std::pair<iterator, bool> insert_unique(value_type&& v) {
    return insert_unique_impl(std::move(v));
  }
  template <class T>
  iterator insert_unique_impl(iterator hint, T&& v) {
    tree_iterator tree_pos = hint.iter();
    ++generation_;
    return iterator(this, tree_.insert_unique(tree_pos, std::forward<T>(v)));
  }
  iterator insert_unique(iterator hint, const value_type& v) { return insert_unique_impl(hint, v); }
  iterator insert_unique(iterator hint, value_type&& v) {
    return insert_unique_impl(hint, std::forward<value_type>(v));
  }
  template <typename InputIterator>
  void insert_unique(InputIterator b, InputIterator e) {
    for (; b != e; ++b) {
      insert_unique(*b);
    }
  }
  void insert_unique(std::initializer_list<value_type> list) {
    insert_unique(list.begin(), list.end());
  }
  iterator insert_multi(const value_type& v) {
    ++generation_;
    return iterator(this, tree_.insert_multi(v));
  }
  iterator insert_multi(iterator position, const value_type& v) {
    tree_iterator tree_pos = position.iter();
    ++generation_;
    return iterator(this, tree_.insert_multi(tree_pos, v));
  }
  template <typename InputIterator>
  void insert_multi(InputIterator b, InputIterator e) {
    for (; b != e; ++b) {
      insert_multi(*b);
    }
  }
  void insert_multi(std::initializer_list<value_type> list) {
    insert_multi(list.begin(), list.end());
  }

  // Deletion routines.
  void erase(const iterator& begin, const iterator& end) {
    tree_.erase(begin.iter(), end.iter());
    ++generation_;
  }
  // Erase the specified iterator from the btree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  iterator erase(iterator iter) {
    tree_iterator res = tree_.erase(iter.iter());
    ++generation_;
    return iterator(this, res);
  }
  size_type erase_unique(const key_type& key) {
    size_type res = tree_.erase_unique(key);
    generation_ += res;
    return res;
  }
  size_type erase_multi(const key_type& key) {
    size_type res = tree_.erase_multi(key);
    generation_ += res;
    return res;
  }

  // Access to the underlying btree.
  btree_type*       internal_btree() { return &tree_; }
  const btree_type* internal_btree() const { return &tree_; }

  // Utility routines.
  void clear() {
    ++generation_;
    tree_.clear();
  }
  void swap(self_type& x) {
    ++generation_;
    ++x.generation_;
    tree_.swap(x.tree_);
  }
  void               dump(std::ostream& os) const { tree_.dump(os); }
  void               verify() const { tree_.verify(); }
  std::int_least64_t generation() const { return generation_; }
  key_compare        key_comp() const { return tree_.key_comp(); }

  // Size routines.
  size_type     size() const { return tree_.size(); }
  size_type     max_size() const { return tree_.max_size(); }
  bool          empty() const { return tree_.empty(); }
  size_type     height() const { return tree_.height(); }
  size_type     internal_nodes() const { return tree_.internal_nodes(); }
  size_type     leaf_nodes() const { return tree_.leaf_nodes(); }
  size_type     nodes() const { return tree_.nodes(); }
  size_type     bytes_used() const { return tree_.bytes_used(); }
  static double average_bytes_per_value() { return btree_type::average_bytes_per_value(); }
  double        fullness() const { return tree_.fullness(); }
  double        overhead() const { return tree_.overhead(); }

 private:
  btree_type         tree_;
  std::int_least64_t generation_;
};

}  // namespace platanus

#endif  // PLATANUS_SAFE_BTREE_H__
