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

#ifndef CPPBTREE_BTREE_COMPARER_H_
#define CPPBTREE_BTREE_COMPARER_H_

#include <functional>
#include <string>
#include <type_traits>

namespace cppbtree {

// Dispatch helper class for using linear search.
template <typename K, typename N, typename Compare>
struct btree_linear_search_compare{
  using node_search_result = typename N::search_result;

  static node_search_result lower_bound(const K& k, const N& n, const Compare& comp) {
    return n.linear_search_compare(k, 0, n.count(), comp);
  }
  static node_search_result upper_bound(const K& k, const N& n, const Compare& comp) {
    return n.template linear_search_compare<false>(k, 0, n.count(), comp);
  }
};

// Dispatch helper class for using binary search.
template <typename K, typename N, typename Compare>
struct btree_binary_search_compare {
  using node_search_result = typename N::search_result;

  static node_search_result lower_bound(const K& k, const N& n, const Compare& comp) {
    return n.binary_search_compare(k, 0, n.count(), comp);
  }
  static node_search_result upper_bound(const K& k, const N& n, const Compare& comp) {
    return n.template binary_search_compare<false>(k, 0, n.count(), comp);
  }
};

}  // namespace cppbtree

#endif  // CPPBTREE_BTREE_COMPARER_H_