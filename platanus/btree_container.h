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

#ifndef PLATANUS_BTREE_CONTAINER_H__
#define PLATANUS_BTREE_CONTAINER_H__

#include <iosfwd>
#include <initializer_list>
#include <utility>

#include "btree.h"

namespace platanus {

// A common base class for btree_set, btree_map, btree_multiset and
// btree_multimap.
template <typename Tree>
class btree_container {
  using self_type = btree_container<Tree>;

 public:
  using params_type            = typename Tree::params_type;
  using key_type               = typename Tree::key_type;
  using value_type             = typename Tree::value_type;
  using key_compare            = typename Tree::key_compare;
  using value_compare          = typename Tree::value_compare;
  using allocator_type         = typename Tree::allocator_type;
  using pointer                = typename Tree::pointer;
  using const_pointer          = typename Tree::const_pointer;
  using reference              = typename Tree::reference;
  using const_reference        = typename Tree::const_reference;
  using size_type              = typename Tree::size_type;
  using difference_type        = typename Tree::difference_type;
  using iterator               = typename Tree::iterator;
  using const_iterator         = typename Tree::const_iterator;
  using reverse_iterator       = typename Tree::reverse_iterator;
  using const_reverse_iterator = typename Tree::const_reverse_iterator;

 public:
  btree_container()                              = default;
  btree_container(const self_type& x)            = default;
  btree_container(self_type&& x)                 = default;
  btree_container& operator=(const self_type& x) = default;
  btree_container& operator=(self_type&& x)      = default;
  ~btree_container()                             = default;

  explicit btree_container(const key_compare& comp, const allocator_type& alloc)
      : tree_(comp, alloc) {}

  btree_container(const self_type& x, const allocator_type& alloc) : tree_(x.tree_, alloc) {}
  btree_container(self_type&& x, const allocator_type& alloc) : tree_(std::move(x.tree_), alloc) {}

  allocator_type get_allocator() const { return tree_.get_allocator(); }

  // Iterator routines.
  iterator               begin() { return tree_.begin(); }
  const_iterator         begin() const { return cbegin(); }
  const_iterator         cbegin() const { return tree_.cbegin(); }
  iterator               end() { return tree_.end(); }
  const_iterator         end() const { return cend(); }
  const_iterator         cend() const { return tree_.cend(); }
  reverse_iterator       rbegin() { return tree_.rbegin(); }
  const_reverse_iterator rbegin() const { return crbegin(); }
  const_reverse_iterator crbegin() const { return tree_.crbegin(); }
  reverse_iterator       rend() { return tree_.rend(); }
  const_reverse_iterator rend() const { return crend(); }
  const_reverse_iterator crend() const { return tree_.crend(); }

  // Utility routines.
  void clear() { tree_.clear(); }
  void swap(self_type& x) { tree_.swap(x.tree_); }
  void dump(std::ostream& os) const { tree_.dump(os); }
  void verify() const { tree_.verify(); }

  // Size routines.
  size_type     size() const { return tree_.size(); }
  size_type     max_size() const { return tree_.max_size(); }
  bool          empty() const { return tree_.empty(); }
  size_type     height() const { return tree_.height(); }
  size_type     internal_nodes() const { return tree_.internal_nodes(); }
  size_type     leaf_nodes() const { return tree_.leaf_nodes(); }
  size_type     nodes() const { return tree_.nodes(); }
  size_type     bytes_used() const { return tree_.bytes_used(); }
  static double average_bytes_per_value() { return Tree::average_bytes_per_value(); }
  double        fullness() const { return tree_.fullness(); }
  double        overhead() const { return tree_.overhead(); }

  bool operator==(const self_type& x) const {
    if (size() != x.size()) {
      return false;
    }
    for (const_iterator i = cbegin(), xi = x.cbegin(); i != cend(); ++i, ++xi) {
      if (*i != *xi) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const self_type& other) const { return !operator==(other); }

 protected:
  Tree tree_;
};

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const btree_container<T>& b) {
  b.dump(os);
  return os;
}

// A common base class for btree_set and safe_btree_set.
template <typename Tree>
class btree_unique_container : public btree_container<Tree> {
  using self_type  = btree_unique_container<Tree>;
  using super_type = btree_container<Tree>;

 public:
  using key_type       = typename Tree::key_type;
  using value_type     = typename Tree::value_type;
  using size_type      = typename Tree::size_type;
  using key_compare    = typename Tree::key_compare;
  using value_compare  = typename Tree::value_compare;
  using allocator_type = typename Tree::allocator_type;
  using iterator       = typename Tree::iterator;
  using const_iterator = typename Tree::const_iterator;

 public:
  btree_unique_container()                            = default;
  btree_unique_container(const self_type&)            = default;
  btree_unique_container(self_type&&)                 = default;
  btree_unique_container& operator=(const self_type&) = default;
  btree_unique_container& operator=(self_type&&)      = default;
  ~btree_unique_container()                           = default;

  explicit btree_unique_container(
      const key_compare& comp, const allocator_type& alloc = allocator_type{}
  )
      : super_type(comp, alloc) {}

  explicit btree_unique_container(const allocator_type& alloc) : super_type(key_compare{}, alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_unique_container(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare{},
      const allocator_type& alloc = allocator_type{}
  )
      : super_type(comp, alloc) {
    insert(b, e);
  }

  template <class InputIterator>
  btree_unique_container(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(key_compare{}, alloc) {
    insert(b, e);
  }

  btree_unique_container(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_unique_container(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_unique_container(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_unique_container(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  // Lookup routines.
  iterator       lower_bound(const key_type& key) { return this->tree_.lower_bound_unique(key); }
  const_iterator lower_bound(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree_unique_container*>(this)->lower_bound(key));
  }
  iterator       upper_bound(const key_type& key) { return this->tree_.upper_bound(key); }
  const_iterator upper_bound(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree_unique_container*>(this)->upper_bound(key));
  }
  std::pair<iterator, iterator> equal_range(const key_type& key) { return this->tree_.equal_range_unique(key); }
  std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
    return static_cast<std::pair<const_iterator, const_iterator>>(const_cast<btree_unique_container*>(this)->equal_range(key));
  }

  iterator       find(const key_type& key) { return this->tree_.find_unique(key); }
  const_iterator find(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree_unique_container*>(this)->find(key));
  }
  size_type      count(const key_type& key) const { return this->tree_.count_unique(key); }

  // Insertion routines.
  std::pair<iterator, bool> insert(const value_type& x) { return this->tree_.insert_unique(x); }
  std::pair<iterator, bool> insert(value_type&& x) {
    return this->tree_.insert_unique(std::move(x));
  }
  iterator insert(iterator hint, const value_type& x) { return this->tree_.insert_unique(hint, x); }
  iterator insert(iterator hint, value_type&& x) {
    return this->tree_.insert_unique(hint, std::move(x));
  }
  template <typename InputIterator>
  void insert(InputIterator b, InputIterator e) {
    this->tree_.insert_unique(b, e);
  }

  void insert(std::initializer_list<value_type> list) { this->tree_.insert_unique(list); }

  // Deletion routines.
  size_type erase(const key_type& key) { return this->tree_.erase_unique(key); }
  // Erase the specified iterator from the btree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  iterator erase(const iterator& iter) { return this->tree_.erase(iter); }
  void     erase(const iterator& first, const iterator& last) { this->tree_.erase(first, last); }
};

// A common base class for btree_map and safe_btree_map.
template <typename Tree>
class btree_map_container : public btree_unique_container<Tree> {
  using self_type  = btree_map_container<Tree>;
  using super_type = btree_unique_container<Tree>;

 public:
  using key_type = typename Tree::key_type;
  using value_type     = typename Tree::value_type;
  using mapped_type    = typename Tree::mapped_type;
  using key_compare    = typename Tree::key_compare;
  using value_compare  = typename Tree::value_compare;
  using allocator_type = typename Tree::allocator_type;

 public:
  btree_map_container()                            = default;
  btree_map_container(const self_type&)            = default;
  btree_map_container(self_type&&)                 = default;
  btree_map_container& operator=(const self_type&) = default;
  btree_map_container& operator=(self_type&&)      = default;
  ~btree_map_container()                           = default;

  explicit btree_map_container(
      const key_compare& comp, const allocator_type& alloc = allocator_type{}
  )
      : super_type(comp, alloc) {}

  explicit btree_map_container(const allocator_type& alloc) : super_type(key_compare{}, alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_map_container(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}

  template <class InputIterator>
  btree_map_container(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(b, e, alloc) {}

  btree_map_container(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_map_container(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_map_container(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_map_container(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  // Insertion routines.
  template <class T>
  mapped_type& internal_operator(T&& key) {
    auto iter = this->lower_bound(key);
    if (iter != this->end() && key == iter->first) {
      return iter->second;
    } else {
      return this->insert(iter, std::make_pair(std::forward<T>(key), mapped_type{}))->second;
    }
  }
  mapped_type& operator[](const key_type& key) {
    return internal_operator(key);
  }
  mapped_type& operator[](key_type&& key) {
    return internal_operator(std::move(key));
  }
};

// A common base class for btree_multiset and btree_multimap.
template <typename Tree>
class btree_multi_container : public btree_container<Tree> {
  using self_type  = btree_multi_container<Tree>;
  using super_type = btree_container<Tree>;

 public:
  using key_type       = typename Tree::key_type;
  using value_type     = typename Tree::value_type;
  using size_type      = typename Tree::size_type;
  using key_compare    = typename Tree::key_compare;
  using value_compare  = typename Tree::value_compare;
  using allocator_type = typename Tree::allocator_type;
  using iterator       = typename Tree::iterator;
  using const_iterator = typename Tree::const_iterator;

 public:
  btree_multi_container()                              = default;
  btree_multi_container(const self_type& x)            = default;
  btree_multi_container(self_type&& x)                 = default;
  btree_multi_container& operator=(const self_type& x) = default;
  btree_multi_container& operator=(self_type&& x)      = default;
  ~btree_multi_container()                             = default;

  explicit btree_multi_container(
      const key_compare& comp, const allocator_type& alloc = allocator_type{}
  )
      : super_type(comp, alloc) {}

  explicit btree_multi_container(const allocator_type& alloc) : super_type(key_compare{}, alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_multi_container(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(comp, alloc) {
    insert(b, e);
  }

  template <class InputIterator>
  btree_multi_container(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(key_compare{}, alloc) {
    insert(b, e);
  }

  btree_multi_container(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_multi_container(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_multi_container(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_multi_container(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  // Lookup routines.
  iterator       lower_bound(const key_type& key) { return this->tree_.lower_bound_multi(key); }
  const_iterator lower_bound(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree_multi_container*>(this)->lower_bound(key));
  }
  iterator       upper_bound(const key_type& key) { return this->tree_.upper_bound(key); }
  const_iterator upper_bound(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree_multi_container*>(this)->upper_bound(key));
  }
  std::pair<iterator, iterator> equal_range(const key_type& key) { return this->tree_.equal_range_multi(key); }
  std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
    return static_cast<std::pair<const_iterator, const_iterator>>(const_cast<btree_multi_container*>(this)->equal_range(key));
  }

  iterator       find(const key_type& key) { return this->tree_.find_multi(key); }
  const_iterator find(const key_type& key) const {
    return static_cast<const_iterator>(const_cast<btree_multi_container*>(this)->find(key));
  }
  size_type      count(const key_type& key) const { return this->tree_.count_multi(key); }

  // Insertion routines.
  iterator insert(const value_type& x) { return this->tree_.insert_multi(x); }
  iterator insert(value_type&& x) { return this->tree_.insert_multi(std::move(x)); }
  iterator insert(iterator position, const value_type& x) {
    return this->tree_.insert_multi(position, x);
  }
  iterator insert(iterator position, value_type&& x) {
    return this->tree_.insert_multi(position, std::move(x));
  }
  template <typename InputIterator>
  void insert(InputIterator b, InputIterator e) {
    this->tree_.insert_multi(b, e);
  }
  void insert(std::initializer_list<value_type> list) { this->tree_.insert_multi(list); }

  // Deletion routines.
  size_type erase(const key_type& key) { return this->tree_.erase_multi(key); }
  // Erase the specified iterator from the btree. The iterator must be valid
  // (i.e. not equal to end()).  Return an iterator pointing to the node after
  // the one that was erased (or end() if none exists).
  iterator erase(const iterator& iter) { return this->tree_.erase(iter); }
  void     erase(const iterator& first, const iterator& last) { this->tree_.erase(first, last); }
};

}  // namespace platanus

#endif  // PLATANUS_BTREE_CONTAINER_H__
