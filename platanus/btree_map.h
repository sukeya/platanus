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
// A btree_map<> implements the STL unique sorted associative container
// interface and the pair associative container interface (a.k.a map<>) using a
// btree. A btree_multimap<> implements the STL multiple sorted associative
// container interface and the pair associtive container interface (a.k.a
// multimap<>) using a btree. See btree.h for details of the btree
// implementation and caveats.

#ifndef PLATANUS_BTREE_MAP_H__
#define PLATANUS_BTREE_MAP_H__

#include <algorithm>
#include <compare>
#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "btree.h"
#include "btree_container.h"

namespace platanus {

// The btree_map class is needed mainly for its constructors.
template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    typename Alloc             = std::allocator<std::pair<const Key, Value> >,
    std::size_t MaxNumOfValues = 64>
class btree_map : public btree_map_container<
                      btree<btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues> > > {
  using self_type   = btree_map<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using params_type = btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using btree_type  = btree<params_type>;
  using super_type  = btree_map_container<btree_type>;

 public:
  using value_type     = typename btree_type::value_type;
  using key_compare    = typename btree_type::key_compare;
  using allocator_type = typename btree_type::allocator_type;

 public:
  btree_map()                            = default;
  btree_map(const self_type&)            = default;
  btree_map(self_type&&)                 = default;
  btree_map& operator=(const self_type&) = default;
  btree_map& operator=(self_type&&)      = default;
  ~btree_map()                           = default;

  explicit btree_map(const key_compare& comp, const allocator_type& alloc = allocator_type())
      : super_type(comp, alloc) {}

  explicit btree_map(const allocator_type& alloc) : super_type(alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_map(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}
  template <class InputIterator>
  btree_map(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(b, e, alloc) {}

  btree_map(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_map(self_type&& x, const allocator_type& alloc) : super_type(std::move(x), alloc) {}

  btree_map(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_map(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}
};

template <typename K, typename V, typename C, typename A, std::size_t N>
void swap(btree_map<K, V, C, A, N>& x, btree_map<K, V, C, A, N>& y) {
  x.swap(y);
}

// The btree_multimap class is needed mainly for its constructors.
template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    typename Alloc             = std::allocator<std::pair<const Key, Value> >,
    std::size_t MaxNumOfValues = 64>
class btree_multimap : public btree_multi_container<
                           btree<btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues> > > {
  using self_type   = btree_multimap<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using params_type = btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using btree_type  = btree<params_type>;
  using super_type  = btree_multi_container<btree_type>;

 public:
  using key_compare    = typename btree_type::key_compare;
  using value_compare  = typename btree_type::value_compare;
  using allocator_type = typename btree_type::allocator_type;
  using mapped_type = typename btree_type::mapped_type;
  using value_type  = typename btree_type::value_type;

 public:
  btree_multimap()                            = default;
  btree_multimap(const self_type&)            = default;
  btree_multimap(self_type&&)                 = default;
  btree_multimap& operator=(const self_type&) = default;
  btree_multimap& operator=(self_type&&)      = default;
  ~btree_multimap()                           = default;

  explicit btree_multimap(const key_compare& comp, const allocator_type& alloc = allocator_type())
      : super_type(comp, alloc) {}

  explicit btree_multimap(const allocator_type& alloc) : super_type(alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_multimap(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}

  template <class InputIterator>
  btree_multimap(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(b, e, alloc) {}

  btree_multimap(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_multimap(self_type&& x, const allocator_type& alloc) : super_type(std::move(x), alloc) {}

  btree_multimap(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_multimap(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}
};

template <typename K, typename V, typename C, typename A, std::size_t N>
void swap(btree_multimap<K, V, C, A, N>& x, btree_multimap<K, V, C, A, N>& y) {
  x.swap(y);
}

}  // namespace platanus

#endif  // PLATANUS_BTREE_MAP_H__
