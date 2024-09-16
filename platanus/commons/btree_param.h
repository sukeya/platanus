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
// Copyright 2024 Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

#ifndef PLATANUS_BTREE_PARAM_H_
#define PLATANUS_BTREE_PARAM_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

#include "btree_util.h"

namespace platanus {

template <typename Key, typename Compare, typename Alloc, std::size_t MaxNumOfValues>
requires comp_requirement<Key, Compare>
struct btree_common_params {
  using key_compare   = Compare;
  using value_compare = std::false_type;

  using allocator_type  = Alloc;
  using key_type        = Key;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;

  static constexpr std::size_t kMaxNumOfValues = MaxNumOfValues;
};

// A parameters structure for holding the type parameters for a btree_map.
template <typename Key, typename Data, typename Compare, typename Alloc, std::size_t MaxNumOfValues>
struct btree_map_params : public btree_common_params<Key, Compare, Alloc, MaxNumOfValues> {
  using mapped_type        = Data;
  using value_type         = std::pair<const Key, mapped_type>;
  using mutable_value_type = std::pair<Key, mapped_type>;

  using key_compare =
      typename btree_common_params<Key, Compare, Alloc, MaxNumOfValues>::key_compare;
  // TODO implement value_compare
  using value_compare   = std::false_type;
  using pointer         = value_type*;
  using const_pointer   = const value_type*;
  using reference       = value_type&;
  using const_reference = const value_type&;

  static const Key& key(const value_type& x) noexcept { return x.first; }
  static const Key& key(const mutable_value_type& x) noexcept { return x.first; }
};

// A parameters structure for holding the type parameters for a btree_set.
template <typename Key, typename Compare, typename Alloc, std::size_t MaxNumOfValues>
struct btree_set_params : public btree_common_params<Key, Compare, Alloc, MaxNumOfValues> {
  using mapped_type        = std::false_type;
  using value_type         = Key;
  using mutable_value_type = value_type;

  using key_compare =
      typename btree_common_params<Key, Compare, Alloc, MaxNumOfValues>::key_compare;
  using value_compare   = key_compare;
  using pointer         = value_type*;
  using const_pointer   = const value_type*;
  using reference       = value_type&;
  using const_reference = const value_type&;

  static const Key& key(const value_type& x) noexcept { return x; }
};

}  // namespace platanus

#endif  // PLATANUS_BTREE_PARAM_H_
