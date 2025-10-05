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
// Copyright 2024-2025 Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

#ifndef PLATANUS_BTREE_MAP_H_
#define PLATANUS_BTREE_MAP_H_

#include <algorithm>
#include <compare>
#include <functional>
#include <memory>
#include <memory_resource>
#include <string>
#include <utility>

#include "internal/btree_node.hpp"
#include "internal/btree.hpp"
#include "internal/btree_container.hpp"
#include "pmr/polymorphic_allocator.hpp"

namespace platanus {

template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    typename Alloc             = std::allocator<std::pair<const Key, Value>>,
    std::size_t MaxNumOfValues = 64>
class btree_map
    : public internal::btree_map_container<internal::btree<internal::btree_node_and_factory<
          internal::btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues>>>> {
  using self_type   = btree_map<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using params_type = internal::btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using btree_type  = internal::btree<internal::btree_node_and_factory<params_type>>;
  using super_type  = internal::btree_map_container<btree_type>;

 public:
  using key_type               = typename super_type::key_type;
  using value_type             = typename super_type::value_type;
  using mapped_type            = typename super_type::mapped_type;
  using key_compare            = typename super_type::key_compare;
  using value_compare          = typename super_type::value_compare;
  using allocator_type         = typename super_type::allocator_type;
  using pointer                = typename super_type::pointer;
  using const_pointer          = typename super_type::const_pointer;
  using reference              = typename super_type::reference;
  using const_reference        = typename super_type::const_reference;
  using size_type              = typename super_type::size_type;
  using difference_type        = typename super_type::difference_type;
  using iterator               = typename super_type::iterator;
  using const_iterator         = typename super_type::const_iterator;
  using reverse_iterator       = typename super_type::reverse_iterator;
  using const_reverse_iterator = typename super_type::const_reverse_iterator;

  static constexpr std::size_t sizeof_leaf_node() { return super_type::sizeof_leaf_node(); }

  static constexpr std::size_t sizeof_internal_node() { return super_type::sizeof_internal_node(); }

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

  explicit btree_map(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  explicit btree_map(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_map(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_map(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  using super_type::begin;
  using super_type::cbegin;
  using super_type::cend;
  using super_type::crbegin;
  using super_type::crend;
  using super_type::end;
  using super_type::rbegin;
  using super_type::rend;

  using super_type::clear;
  using super_type::dump;
  using super_type::swap;
  using super_type::verify;

  using super_type::average_bytes_per_value;
  using super_type::bytes_used;
  using super_type::empty;
  using super_type::fullness;
  using super_type::height;
  using super_type::internal_nodes;
  using super_type::leaf_nodes;
  using super_type::max_size;
  using super_type::nodes;
  using super_type::overhead;
  using super_type::size;

  using super_type::key_comp;

  using super_type::equal_range;
  using super_type::lower_bound;
  using super_type::upper_bound;

  using super_type::contains;
  using super_type::count;
  using super_type::find;

  using super_type::erase;
  using super_type::insert;

  using super_type::merge;

  using super_type::at;
  using super_type::operator[];
};

template <typename K, typename V, typename C, typename A, std::size_t N>
void swap(btree_map<K, V, C, A, N>& x, btree_map<K, V, C, A, N>& y) {
  x.swap(y);
}

template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    typename Alloc             = std::allocator<std::pair<const Key, Value>>,
    std::size_t MaxNumOfValues = 64>
class btree_multimap
    : public internal::btree_multi_container<internal::btree<internal::btree_node_and_factory<
          internal::btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues>>>> {
  using self_type   = btree_multimap<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using params_type = internal::btree_map_params<Key, Value, Compare, Alloc, MaxNumOfValues>;
  using btree_type  = internal::btree<internal::btree_node_and_factory<params_type>>;
  using super_type  = internal::btree_multi_container<btree_type>;

 public:
  using mapped_type = typename btree_type::mapped_type;

  using key_type               = typename super_type::key_type;
  using value_type             = typename super_type::value_type;
  using key_compare            = typename super_type::key_compare;
  using value_compare          = typename super_type::value_compare;
  using allocator_type         = typename super_type::allocator_type;
  using pointer                = typename super_type::pointer;
  using const_pointer          = typename super_type::const_pointer;
  using reference              = typename super_type::reference;
  using const_reference        = typename super_type::const_reference;
  using size_type              = typename super_type::size_type;
  using difference_type        = typename super_type::difference_type;
  using iterator               = typename super_type::iterator;
  using const_iterator         = typename super_type::const_iterator;
  using reverse_iterator       = typename super_type::reverse_iterator;
  using const_reverse_iterator = typename super_type::const_reverse_iterator;

  static constexpr std::size_t sizeof_leaf_node() { return super_type::sizeof_leaf_node(); }

  static constexpr std::size_t sizeof_internal_node() { return super_type::sizeof_internal_node(); }

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

  explicit btree_multimap(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  explicit btree_multimap(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_multimap(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_multimap(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  using super_type::begin;
  using super_type::cbegin;
  using super_type::cend;
  using super_type::crbegin;
  using super_type::crend;
  using super_type::end;
  using super_type::rbegin;
  using super_type::rend;

  using super_type::clear;
  using super_type::dump;
  using super_type::swap;
  using super_type::verify;

  using super_type::average_bytes_per_value;
  using super_type::bytes_used;
  using super_type::empty;
  using super_type::fullness;
  using super_type::height;
  using super_type::internal_nodes;
  using super_type::leaf_nodes;
  using super_type::max_size;
  using super_type::nodes;
  using super_type::overhead;
  using super_type::size;

  using super_type::key_comp;

  using super_type::equal_range;
  using super_type::lower_bound;
  using super_type::upper_bound;

  using super_type::contains;
  using super_type::count;
  using super_type::find;

  using super_type::erase;
  using super_type::insert;

  using super_type::merge;
};

template <typename K, typename V, typename C, typename A, std::size_t N>
void swap(btree_multimap<K, V, C, A, N>& x, btree_multimap<K, V, C, A, N>& y) {
  x.swap(y);
}

namespace pmr {

template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    std::size_t MaxNumOfValues = 64>
class btree_map : public internal::btree_map_container<
                      internal::btree<internal::btree_node_and_factory<internal::btree_map_params<
                          Key,
                          Value,
                          Compare,
                          pmr::polymorphic_allocator<>,
                          MaxNumOfValues>>>> {
  using self_type = btree_map<Key, Value, Compare, MaxNumOfValues>;
  using params_type =
      internal::btree_map_params<Key, Value, Compare, pmr::polymorphic_allocator<>, MaxNumOfValues>;
  using btree_type = internal::btree<internal::btree_node_and_factory<
      internal::
          btree_map_params<Key, Value, Compare, pmr::polymorphic_allocator<>, MaxNumOfValues>>>;
  using super_type = internal::btree_map_container<btree_type>;

 public:
  using key_type               = typename super_type::key_type;
  using value_type             = typename super_type::value_type;
  using mapped_type            = typename super_type::mapped_type;
  using key_compare            = typename super_type::key_compare;
  using value_compare          = typename super_type::value_compare;
  using allocator_type         = typename super_type::allocator_type;
  using pointer                = typename super_type::pointer;
  using const_pointer          = typename super_type::const_pointer;
  using reference              = typename super_type::reference;
  using const_reference        = typename super_type::const_reference;
  using size_type              = typename super_type::size_type;
  using difference_type        = typename super_type::difference_type;
  using iterator               = typename super_type::iterator;
  using const_iterator         = typename super_type::const_iterator;
  using reverse_iterator       = typename super_type::reverse_iterator;
  using const_reverse_iterator = typename super_type::const_reverse_iterator;

  static constexpr std::size_t sizeof_leaf_node() { return super_type::sizeof_leaf_node(); }

  static constexpr std::size_t sizeof_internal_node() { return super_type::sizeof_internal_node(); }

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

  explicit btree_map(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  explicit btree_map(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_map(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_map(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  using super_type::begin;
  using super_type::cbegin;
  using super_type::cend;
  using super_type::crbegin;
  using super_type::crend;
  using super_type::end;
  using super_type::rbegin;
  using super_type::rend;

  using super_type::clear;
  using super_type::dump;
  using super_type::swap;
  using super_type::verify;

  using super_type::average_bytes_per_value;
  using super_type::bytes_used;
  using super_type::empty;
  using super_type::fullness;
  using super_type::height;
  using super_type::internal_nodes;
  using super_type::leaf_nodes;
  using super_type::max_size;
  using super_type::nodes;
  using super_type::overhead;
  using super_type::size;

  using super_type::key_comp;

  using super_type::equal_range;
  using super_type::lower_bound;
  using super_type::upper_bound;

  using super_type::contains;
  using super_type::count;
  using super_type::find;

  using super_type::erase;
  using super_type::insert;

  using super_type::merge;

  using super_type::at;
  using super_type::operator[];
};

template <typename K, typename V, typename C, std::size_t N>
void swap(btree_map<K, V, C, N>& x, btree_map<K, V, C, N>& y) {
  x.swap(y);
}

template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    std::size_t MaxNumOfValues = 64>
class btree_multimap
    : public internal::btree_multi_container<
          internal::btree<internal::btree_node_and_factory<internal::btree_map_params<
              Key,
              Value,
              Compare,
              pmr::polymorphic_allocator<>,
              MaxNumOfValues>>>> {
  using self_type = btree_multimap<Key, Value, Compare, MaxNumOfValues>;
  using params_type =
      internal::btree_map_params<Key, Value, Compare, pmr::polymorphic_allocator<>, MaxNumOfValues>;
  using btree_type = internal::btree<internal::btree_node_and_factory<
      internal::
          btree_map_params<Key, Value, Compare, pmr::polymorphic_allocator<>, MaxNumOfValues>>>;
  using super_type = internal::btree_multi_container<btree_type>;

 public:
  using mapped_type = typename btree_type::mapped_type;

  using key_type               = typename super_type::key_type;
  using value_type             = typename super_type::value_type;
  using key_compare            = typename super_type::key_compare;
  using value_compare          = typename super_type::value_compare;
  using allocator_type         = typename super_type::allocator_type;
  using pointer                = typename super_type::pointer;
  using const_pointer          = typename super_type::const_pointer;
  using reference              = typename super_type::reference;
  using const_reference        = typename super_type::const_reference;
  using size_type              = typename super_type::size_type;
  using difference_type        = typename super_type::difference_type;
  using iterator               = typename super_type::iterator;
  using const_iterator         = typename super_type::const_iterator;
  using reverse_iterator       = typename super_type::reverse_iterator;
  using const_reverse_iterator = typename super_type::const_reverse_iterator;

  static constexpr std::size_t sizeof_leaf_node() { return super_type::sizeof_leaf_node(); }

  static constexpr std::size_t sizeof_internal_node() { return super_type::sizeof_internal_node(); }

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

  explicit btree_multimap(const self_type& x, const allocator_type& alloc) : super_type(x, alloc) {}
  explicit btree_multimap(self_type&& x, const allocator_type& alloc)
      : super_type(std::move(x), alloc) {}

  btree_multimap(
      std::initializer_list<value_type> init,
      const key_compare&                comp  = key_compare{},
      const allocator_type&             alloc = allocator_type{}
  )
      : self_type{init.begin(), init.end(), comp, alloc} {}
  btree_multimap(std::initializer_list<value_type> init, const allocator_type& alloc)
      : self_type{init.begin(), init.end(), alloc} {}

  using super_type::begin;
  using super_type::cbegin;
  using super_type::cend;
  using super_type::crbegin;
  using super_type::crend;
  using super_type::end;
  using super_type::rbegin;
  using super_type::rend;

  using super_type::clear;
  using super_type::dump;
  using super_type::swap;
  using super_type::verify;

  using super_type::average_bytes_per_value;
  using super_type::bytes_used;
  using super_type::empty;
  using super_type::fullness;
  using super_type::height;
  using super_type::internal_nodes;
  using super_type::leaf_nodes;
  using super_type::max_size;
  using super_type::nodes;
  using super_type::overhead;
  using super_type::size;

  using super_type::key_comp;

  using super_type::equal_range;
  using super_type::lower_bound;
  using super_type::upper_bound;

  using super_type::contains;
  using super_type::count;
  using super_type::find;

  using super_type::erase;
  using super_type::insert;

  using super_type::merge;
};

template <typename K, typename V, typename C, std::size_t N>
void swap(btree_multimap<K, V, C, N>& x, btree_multimap<K, V, C, N>& y) {
  x.swap(y);
}
}  // namespace pmr
}  // namespace platanus

#endif  // PLATANUS_BTREE_MAP_H_
