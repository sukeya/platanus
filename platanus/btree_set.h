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
// A btree_set<> implements the STL unique sorted associative container
// interface (a.k.a set<>) using a btree. A btree_multiset<> implements the STL
// multiple sorted associative container interface (a.k.a multiset<>) using a
// btree. See btree.h for details of the btree implementation and caveats.

#ifndef PLATANUS_BTREE_SET_H__
#define PLATANUS_BTREE_SET_H__

#include <compare>
#include <functional>
#include <memory>
#include <string>

#include "btree.h"
#include "btree_container.h"

namespace platanus {

// The btree_set class is needed mainly for its constructors.
template <
    typename Key,
    typename Compare           = std::less<Key>,
    typename Alloc             = std::allocator<Key>,
    std::size_t TargetNodeSize = 512>
class btree_set : public btree_unique_container<
                      btree<btree_set_params<Key, Compare, Alloc, TargetNodeSize> > > {
  using self_type   = btree_set<Key, Compare, Alloc, TargetNodeSize>;
  using params_type = btree_set_params<Key, Compare, Alloc, TargetNodeSize>;
  using btree_type  = btree<params_type>;
  using super_type  = btree_unique_container<btree_type>;

 public:
  using value_type     = typename btree_type::value_type;
  using key_compare    = typename btree_type::key_compare;
  using value_compare  = typename btree_type::value_compare;
  using allocator_type = typename btree_type::allocator_type;

 public:
  btree_set()                            = default;
  btree_set(const self_type&)            = default;
  btree_set(self_type&&)                 = default;
  btree_set& operator=(const self_type&) = default;
  btree_set& operator=(self_type&&)      = default;
  ~btree_set()                           = default;

  explicit btree_set(const key_compare& comp, const allocator_type& alloc = allocator_type())
      : super_type(comp, alloc) {}

  explicit btree_set(const allocator_type& alloc) : super_type(alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_set(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}

  template <class InputIterator>
  btree_set(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(b, e, alloc) {}

  btree_set(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_set(self_type&& x, const allocator_type& alloc) : super_type(std::move(x), alloc) {}

  btree_set(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_set(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}
};

template <typename K, typename C, typename A, std::size_t N>
inline void swap(btree_set<K, C, A, N>& x, btree_set<K, C, A, N>& y) {
  x.swap(y);
}

// The btree_multiset class is needed mainly for its constructors.
template <
    typename Key,
    typename Compare           = std::less<Key>,
    typename Alloc             = std::allocator<Key>,
    std::size_t TargetNodeSize = 512>
class btree_multiset
    : public btree_multi_container<btree<btree_set_params<Key, Compare, Alloc, TargetNodeSize> > > {
  using self_type   = btree_multiset<Key, Compare, Alloc, TargetNodeSize>;
  using params_type = btree_set_params<Key, Compare, Alloc, TargetNodeSize>;
  using btree_type  = btree<params_type>;
  using super_type  = btree_multi_container<btree_type>;

 public:
  using value_type     = typename btree_type::value_type;
  using key_compare    = typename btree_type::key_compare;
  using allocator_type = typename btree_type::allocator_type;

 public:
  btree_multiset()                            = default;
  btree_multiset(const self_type&)            = default;
  btree_multiset(self_type&&)                 = default;
  btree_multiset& operator=(const self_type&) = default;
  btree_multiset& operator=(self_type&&)      = default;
  ~btree_multiset()                           = default;

  explicit btree_multiset(const key_compare& comp, const allocator_type& alloc = allocator_type())
      : super_type(comp, alloc) {}

  explicit btree_multiset(const allocator_type& alloc) : super_type(alloc) {}

  // Range constructor.
  template <class InputIterator>
  btree_multiset(
      InputIterator         b,
      InputIterator         e,
      const key_compare&    comp  = key_compare(),
      const allocator_type& alloc = allocator_type()
  )
      : super_type(b, e, comp, alloc) {}

  template <class InputIterator>
  btree_multiset(InputIterator b, InputIterator e, const allocator_type& alloc)
      : super_type(b, e, alloc) {}

  btree_multiset(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  btree_multiset(self_type&& x, const allocator_type& alloc) : super_type(std::move(x), alloc) {}

  btree_multiset(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_multiset(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}
};

template <typename K, typename C, typename A, std::size_t N>
inline void swap(btree_multiset<K, C, A, N>& x, btree_multiset<K, C, A, N>& y) {
  x.swap(y);
}

}  // namespace platanus

#endif  // PLATANUS_BTREE_SET_H__
