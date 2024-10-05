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

#ifndef PLATANUS_COMMONS_BTREE_NODE_DECL_H__
#define PLATANUS_COMMONS_BTREE_NODE_DECL_H__

#include <cstddef>
#include <limits>

namespace platanus::commons {
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
}

#endif
