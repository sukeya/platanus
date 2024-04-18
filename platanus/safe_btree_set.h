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
// The safe_btree_set<> is like btree_set<> except that it removes the caveat
// about insertion and deletion invalidating existing iterators at a small cost
// in making iterators larger and slower.
//
// Revalidation occurs whenever an iterator is accessed.  References
// and pointers returned by safe_btree_map<> iterators are not stable,
// they are potentially invalidated by any non-const method on the set.
//
// BEGIN INCORRECT EXAMPLE
//   for (auto i = safe_set->begin(); i != safe_set->end(); ++i) {
//     const T &value = *i;  // DO NOT DO THIS
//     [code that modifies safe_set and uses value];
//   }
// END INCORRECT EXAMPLE

#ifndef PLATANUS_SAFE_BTREE_SET_H__
#define PLATANUS_SAFE_BTREE_SET_H__

#include <compare>
#include <functional>
#include <memory>

#include "btree_container.h"
#include "btree_set.h"
#include "safe_btree.h"

namespace platanus {

// The safe_btree_set class is needed mainly for its constructors.
template <
    typename Key,
    typename Compare   = DefaultWeakComp,
    typename Alloc     = std::allocator<Key>,
    std::size_t TargetNodeSize = 512>
class safe_btree_set : public btree_unique_container<
                           safe_btree<btree_set_params<Key, Compare, Alloc, TargetNodeSize> > > {
  using self_type   = safe_btree_set<Key, Compare, Alloc, TargetNodeSize>;
  using params_type = btree_set_params<Key, Compare, Alloc, TargetNodeSize>;
  using btree_type  = safe_btree<params_type>;
  using super_type  = btree_unique_container<btree_type>;

 public:
  using value_type = typename btree_type::value_type;
  using key_compare    = typename btree_type::key_compare;
  using value_compare  = typename btree_type::value_compare;
  using allocator_type = typename btree_type::allocator_type;

 public:
  safe_btree_set()                            = default;
  safe_btree_set(const self_type&)            = default;
  safe_btree_set(self_type&&)                 = default;
  safe_btree_set& operator=(const self_type&) = default;
  safe_btree_set& operator=(self_type&&)      = default;
  ~safe_btree_set()                           = default;

  explicit safe_btree_set(
      const key_compare& comp, const allocator_type& alloc = allocator_type()
  )
      : super_type(comp, alloc) {}

  explicit safe_btree_set(
      const allocator_type& alloc
  )
      : super_type(alloc) {}

  // Range constructor.
  template <class InputIterator>
  safe_btree_set(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}

  template <class InputIterator>
  safe_btree_set(
      InputIterator         b,
      InputIterator         e,
      const allocator_type& alloc
  )
      : super_type(b, e, alloc) {}

  safe_btree_set(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  safe_btree_set(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  safe_btree_set(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  safe_btree_set(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}
};

template <typename K, typename C, typename A, std::size_t N>
inline void swap(safe_btree_set<K, C, A, N>& x, safe_btree_set<K, C, A, N>& y) {
  x.swap(y);
}

}  // namespace platanus

#endif  // PLATANUS_SAFE_BTREE_SET_H__
