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

#ifndef PLATANUS_INTERNAL_BTREE_NODE_FWD_H_
#define PLATANUS_INTERNAL_BTREE_NODE_FWD_H_

#include <array>
#include <cstddef>

namespace platanus {
namespace internal {
template <class T>
struct btree_node_owner_type {
  // Define a template type named `type`.
};

template <class T>
using btree_node_owner = typename btree_node_owner_type<T>::type;

template <class T>
using btree_node_borrower = T*;

template <class T>
using btree_node_readonly_borrower = const T*;

template <class T>
struct sizeof_leaf_node {};

template <class T>
static constexpr std::size_t sizeof_leaf_node_v = sizeof_leaf_node<T>::value;

template <class T>
struct sizeof_internal_node {};

template <class T>
static constexpr std::size_t sizeof_internal_node_v = sizeof_internal_node<T>::value;

static constexpr std::int_least16_t kMinNumOfValues = 3;
}  // namespace internal

static constexpr std::array<int, 2> cache_line_sizes = {64, 1024};

static constexpr std::int_least16_t kFitL1Cache = -1;
static constexpr std::int_least16_t kFitL2Cache = -2;

}  // namespace platanus

#endif
