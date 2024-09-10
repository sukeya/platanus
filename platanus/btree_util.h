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

#ifndef PLATANUS_BTREE_UTIL_H_
#define PLATANUS_BTREE_UTIL_H_

#include <concepts>
#include <compare>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace platanus {

// Inside a btree method, if we just call swap(), it will choose the
// btree::swap method, which we don't want. And we can't say ::swap
// because then MSVC won't pickup any std::swap() implementations. We
// can't just use std::swap() directly because then we don't get the
// specialization for types outside the std namespace. So the solution
// is to have a special swap helper function whose name doesn't
// collide with other swap functions defined by the btree classes.
template <typename T>
void btree_swap_helper(T& a, T& b) {
  using std::swap;
  swap(a, b);
}

template <class T>
concept bool_or_weak_ordering = std::same_as<T, bool> || std::convertible_to<T, std::weak_ordering>;

template <class Key, class Compare>
concept comp_requirement = requires(Key lhd, Key rhd, Compare comp) {
  { comp(lhd, rhd) } -> bool_or_weak_ordering;
};

template <class Key, class Compare>
concept comp_return_weak_ordering = requires(Key lhd, Key rhd, Compare comp) {
  { comp(lhd, rhd) } -> std::convertible_to<std::weak_ordering>;
};

template <class Key, class Compare>
concept comp_return_bool = requires(Key lhd, Key rhd, Compare comp) {
  { comp(lhd, rhd) } -> std::same_as<bool>;
};

template <std::size_t BitWidth>
class btree_node_search_result {
 public:
  static_assert(BitWidth > 0 && BitWidth < 16, "BitWidth must be in the range [1, 15].");

  static constexpr std::size_t bit_width         = BitWidth;
  static constexpr bool        is_less_than_8bit = bit_width < 8;

  using count_type = std::conditional_t<is_less_than_8bit, std::uint_least8_t, std::uint_least16_t>;
  using signed_count_type =
      std::conditional_t<is_less_than_8bit, std::int_least16_t, std::int_least32_t>;

  btree_node_search_result()                                           = default;
  btree_node_search_result(const btree_node_search_result&)            = default;
  btree_node_search_result& operator=(const btree_node_search_result&) = default;
  btree_node_search_result(btree_node_search_result&&)                 = default;
  btree_node_search_result& operator=(btree_node_search_result&&)      = default;
  ~btree_node_search_result()                                          = default;

  explicit btree_node_search_result(count_type index, bool is_exact_match) noexcept
      : index_(index), exact_match_(is_exact_match ? 1 : 0) {
    assert(index < (1 << bit_width));
  }

  count_type index() const noexcept { return index_; }
  bool       is_exact_match() const noexcept { return exact_match_ == 1; }

 private:
  count_type index_ : bit_width{0};
  count_type exact_match_ : 1 {0};
};
}  // namespace platanus

#endif  // PLATANUS_BTREE_UTIL_H_