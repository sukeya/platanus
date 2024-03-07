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

#ifndef CPPBTREE_BTREE_PARAM_H_
#define CPPBTREE_BTREE_PARAM_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <sys/types.h>
#include <type_traits>
#include <utility>

#include "btree_comparer.h"
#include "btree_util.h"

namespace cppbtree {

template <typename Key, typename Compare, typename Alloc, int TargetNodeSize, int ValueSize>
struct btree_common_params {
  // If Compare is derived from btree_key_compare_to_tag then use it as the
  // key_compare type. Otherwise, use btree_key_compare_to_adapter<> which will
  // fall-back to Compare if we don't have an appropriate specialization.
  using key_compare = typename std::conditional_t<
      btree_is_key_compare_to<Compare>::value,
      Compare,
      btree_key_compare_to_adapter<Compare>>;
  // A type which indicates if we have a key-compare-to functor or a plain old
  // key-compare functor.
  using is_key_compare_to = btree_is_key_compare_to<key_compare>;

  using value_compare   = std::false_type;

  using allocator_type  = Alloc;
  using key_type        = Key;
  using size_type       = ssize_t;
  using difference_type = std::ptrdiff_t;

  static constexpr int kTargetNodeSize = TargetNodeSize;

  // Available space for values.  This is largest for leaf nodes,
  // which has overhead no fewer than two pointers.
  static_assert(
      TargetNodeSize >= 2 * sizeof(void*), "ValueSize must be no less than 2 * sizeof(void*)"
  );
  static constexpr int kNodeValueSpace = TargetNodeSize - 2 * sizeof(void*);

  // This is an integral type large enough to hold as many
  // ValueSize-values as will fit a node of TargetNodeSize bytes.
  static_assert(
      kNodeValueSpace / ValueSize <= std::numeric_limits<std::uint_fast16_t>::max(),
      "The total of nodes exceeds supported size (max of uint16_t)."
  );
  using node_count_type = typename std::conditional<
      (kNodeValueSpace / ValueSize) >= 256,
      std::uint_least16_t,
      std::uint_least8_t>::type;
};

// A parameters structure for holding the type parameters for a btree_map.
template <typename Key, typename Data, typename Compare, typename Alloc, int TargetNodeSize>
struct btree_map_params
    : public btree_common_params<Key, Compare, Alloc, TargetNodeSize, sizeof(Key) + sizeof(Data)> {
  // Deprecated: use mapped_type instead.
  using data_type          = Data;
  using mapped_type        = Data;
  using value_type         = std::pair<const Key, mapped_type>;
  using mutable_value_type = std::pair<Key, mapped_type>;
  using key_compare        = typename btree_common_params<Key, Compare, Alloc, TargetNodeSize, sizeof(Key) + sizeof(Data)>::key_compare;
  // TODO
  using value_compare      = std::false_type;
  using pointer            = value_type*;
  using const_pointer      = const value_type*;
  using reference          = value_type&;
  using const_reference    = const value_type&;

  static_assert(
      sizeof(Key) + sizeof(mapped_type) <= std::numeric_limits<int>::max(),
      "The total size of Key and mapped_type must be less than the max of int."
  );
  static constexpr int kValueSize = sizeof(Key) + sizeof(mapped_type);

  static const Key& key(const value_type& x) noexcept { return x.first; }
  static const Key& key(const mutable_value_type& x) noexcept { return x.first; }
  static void       swap(mutable_value_type* a, mutable_value_type* b) {
          btree_swap_helper(a->first, b->first);
          btree_swap_helper(a->second, b->second);
  }
};

// A parameters structure for holding the type parameters for a btree_set.
template <typename Key, typename Compare, typename Alloc, int TargetNodeSize>
struct btree_set_params
    : public btree_common_params<Key, Compare, Alloc, TargetNodeSize, sizeof(Key)> {
  // Deprecated: use mapped_type instead.
  using data_type          = std::false_type;
  using mapped_type        = std::false_type;
  using value_type         = Key;
  using mutable_value_type = value_type;
  using key_compare        = typename btree_common_params<Key, Compare, Alloc, TargetNodeSize, sizeof(Key)>::key_compare;
  using value_compare      = key_compare;
  using pointer            = value_type*;
  using const_pointer      = const value_type*;
  using reference          = value_type&;
  using const_reference    = const value_type&;

  static_assert(
      sizeof(Key) <= std::numeric_limits<int>::max(),
      "The size of Key must be less than the max of int."
  );
  static constexpr std::size_t kValueSize = sizeof(Key);

  static const Key& key(const value_type& x) noexcept { return x; }
  static void       swap(mutable_value_type* a, mutable_value_type* b) {
          btree_swap_helper<mutable_value_type>(*a, *b);
  }
};

} // namespace cppbtree

#endif  // CPPBTREE_BTREE_PARAM_H_
