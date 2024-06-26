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
// The safe_btree_map<> is like btree_map<> except that it removes the caveat
// about insertion and deletion invalidating existing iterators at a small cost
// in making iterators larger and slower.
//
// Revalidation occurs whenever an iterator is accessed.  References
// and pointers returned by safe_btree_map<> iterators are not stable,
// they are potentially invalidated by any non-const method on the map.
//
// BEGIN INCORRECT EXAMPLE
//   for (auto i = safe_map->begin(); i != safe_map->end(); ++i) {
//     const T *value = &i->second;  // DO NOT DO THIS
//     [code that modifies safe_map and uses value];
//   }
// END INCORRECT EXAMPLE
#ifndef PLATANUS_SAFE_BTREE_MAP_H__
#define PLATANUS_SAFE_BTREE_MAP_H__

#include <functional>
#include <memory>
#include <utility>

#include "btree_container.h"
#include "btree_map.h"
#include "safe_btree.h"

namespace platanus {

// The safe_btree_map class is needed mainly for its constructors.
template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    typename Alloc             = std::allocator<std::pair<const Key, Value> >,
    std::size_t TargetNodeSize = 512>
class safe_btree_map
    : public btree_map_container<
          safe_btree<btree_map_params<Key, Value, Compare, Alloc, TargetNodeSize> > > {
  using self_type   = safe_btree_map<Key, Value, Compare, Alloc, TargetNodeSize>;
  using params_type = btree_map_params<Key, Value, Compare, Alloc, TargetNodeSize>;
  using btree_type  = safe_btree<params_type>;
  using super_type  = btree_map_container<btree_type>;

 public:
  using value_type     = typename btree_type::value_type;
  using key_compare    = typename btree_type::key_compare;
  using value_compare  = typename btree_type::value_compare;
  using allocator_type = typename btree_type::allocator_type;

 public:
  safe_btree_map()                            = default;
  safe_btree_map(const self_type&)            = default;
  safe_btree_map(self_type&&)                 = default;
  safe_btree_map& operator=(const self_type&) = default;
  safe_btree_map& operator=(self_type&&)      = default;
  ~safe_btree_map()                           = default;

  explicit safe_btree_map(const key_compare& comp, const allocator_type& alloc = allocator_type())
      : super_type(comp, alloc) {}

  explicit safe_btree_map(const allocator_type& alloc) : super_type(alloc) {}

  // Range constructor.
  template <class InputIterator>
  safe_btree_map(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}

  template <class InputIterator>
  safe_btree_map(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(b, e, alloc) {}

  safe_btree_map(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  safe_btree_map(self_type&& x, const allocator_type& alloc) : super_type(std::move(x), alloc) {}

  safe_btree_map(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  safe_btree_map(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}
};

template <typename K, typename V, typename C, typename A, std::size_t N>
inline void swap(safe_btree_map<K, V, C, A, N>& x, safe_btree_map<K, V, C, A, N>& y) {
  x.swap(y);
}

}  // namespace platanus

#endif  // PLATANUS_SAFE_BTREE_MAP_H__
