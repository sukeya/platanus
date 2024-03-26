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

#ifndef CPPBTREE_BTREE_UTIL_H_
#define CPPBTREE_BTREE_UTIL_H_

#include <compare>
#include <utility>

namespace std {
inline void swap(const decltype(std::compare_weak_order_fallback)&, const decltype(std::compare_weak_order_fallback)&) {}
}

namespace cppbtree {

// Inside a btree method, if we just call swap(), it will choose the
// btree::swap method, which we don't want. And we can't say ::swap
// because then MSVC won't pickup any std::swap() implementations. We
// can't just use std::swap() directly because then we don't get the
// specialization for types outside the std namespace. So the solution
// is to have a special swap helper function whose name doesn't
// collide with other swap functions defined by the btree classes.
template <typename T>
inline void btree_swap_helper(T& a, T& b) {
  using std::swap;
  swap(a, b);
}

template <class Key, class Compare>
concept is_comp_weak_order = requires(Key lhd, Key rhd, Compare comp) {
  {comp(lhd, rhd)} -> std::convertible_to<std::weak_ordering>;
};

using DefaultWeakComp = decltype(std::compare_weak_order_fallback);

}  // namespace cppbtree

#endif  // CPPBTREE_BTREE_UTIL_H_