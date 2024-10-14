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

#ifndef PLATANUS_COMMONS_BTREE_NODE_COMMON_H_
#define PLATANUS_COMMONS_BTREE_NODE_COMMON_H_

#include <bit>
#include <cstdint>
#include <type_traits>

#include "btree_node_decl.h"
#include "btree_util.h"

namespace platanus {

namespace pmr::details {
template <class P>
class btree_leaf_node;
}

namespace commons {
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

template <typename Params, class Node>
class btree_base_node {
 public:
  static constexpr std::size_t kMaxNumOfValues = []() {
    // Available space for values.
    static_assert(
        Params::kMaxNumOfValues >= 3,
        "We need a minimum of 3 values per internal node in order to perform"
        "splitting (1 value for the two nodes involved in the split and 1 value"
        "propagated to the parent as the delimiter for the split)."
    );

    return Params::kMaxNumOfValues;
  }();

  static constexpr std::size_t kNodeValues   = Params::kMaxNumOfValues;
  static constexpr std::size_t kNodeChildren = Params::kMaxNumOfValues + 1;

  using params_type        = Params;
  using key_type           = typename Params::key_type;
  using mapped_type        = typename Params::mapped_type;
  using value_type         = typename Params::value_type;
  using mutable_value_type = typename Params::mutable_value_type;
  using pointer            = typename Params::pointer;
  using const_pointer      = typename Params::const_pointer;
  using reference          = typename Params::reference;
  using const_reference    = typename Params::const_reference;
  using key_compare        = typename Params::key_compare;
  using value_compare      = typename Params::value_compare;
  using size_type          = typename Params::size_type;
  using difference_type    = typename Params::difference_type;

  using search_result =
      btree_node_search_result<std::bit_width(static_cast<std::size_t>(kNodeValues))>;
  using count_type        = typename search_result::count_type;
  using signed_count_type = typename search_result::signed_count_type;

  using values_type                   = std::array<mutable_value_type, kNodeValues>;
  using values_iterator               = typename values_type::iterator;
  using values_const_iterator         = typename values_type::const_iterator;
  using values_reverse_iterator       = typename values_type::reverse_iterator;
  using values_const_reverse_iterator = typename values_type::const_reverse_iterator;

  // node_borrower can change the node's content but not the memory (allocation and release not
  // allowed).
  using node_borrower          = btree_node_borrower<Node>;
  using node_readonly_borrower = btree_node_readonly_borrower<Node>;

  btree_base_node()                                  = default;
  btree_base_node(const btree_base_node&)            = default;
  btree_base_node(btree_base_node&&)                 = default;
  btree_base_node& operator=(const btree_base_node&) = default;
  btree_base_node& operator=(btree_base_node&&)      = default;
  ~btree_base_node()                                 = default;

  explicit btree_base_node(btree_node_borrower<Node> parent)
      : parent_(parent), position_(0), count_(0) {}

  // Getter for the position of this node in its parent.
  count_type position() const noexcept { return position_; }
  void       set_position(count_type v) noexcept {
    assert(borrow_parent() != nullptr);
    assert(0 <= v && v < kNodeChildren);
    position_ = v;
  }

  // Getter/setter for the number of values stored in this node.
  count_type           count() const noexcept { return count_; }
  constexpr count_type max_count() const noexcept { return kNodeValues; }

  // Getter for the parent of this node.
  node_borrower          borrow_parent() const noexcept { return parent_; }
  node_readonly_borrower borrow_readonly_parent() const noexcept { return parent_; }

  // Getter for whether the node is the root of the tree. The parent of the
  // root of the tree is the leftmost node in the tree which is guaranteed to
  // be a leaf.
  bool is_root() const noexcept { return borrow_readonly_parent() == nullptr; }
  void make_root() noexcept {
    assert(borrow_readonly_parent()->is_root());
    parent_ = nullptr;
  }

  // Getters for the key/value at position i in the node.
  const key_type& key(count_type i) const noexcept { return params_type::key(values_[i]); }
  reference       value(count_type i) noexcept { return reinterpret_cast<reference>(values_[i]); }
  const_reference value(count_type i) const noexcept {
    return reinterpret_cast<const_reference>(values_[i]);
  }

  // Swap value i in this node with value j in node x.
  void
  value_swap(count_type i, btree_node_borrower<btree_base_node> x, count_type j) noexcept(noexcept(
      btree_swap_helper(std::declval<mutable_value_type&>(), std::declval<mutable_value_type&>())
  )) {
    btree_swap_helper(values_[i], x->values_[j]);
  }

  // Returns the position of the first value whose key is not less than k.
  search_result lower_bound(const key_type& k, const key_compare& comp) const
      noexcept(noexcept(binary_search_compare(k, 0, count(), comp))) {
    return binary_search_compare(k, 0, count(), comp);
  }
  // Returns the position of the first value whose key is greater than k.
  search_result upper_bound(const key_type& k, const key_compare& comp) const
      noexcept(noexcept(binary_search_compare<false>(k, 0, count(), comp))) {
    return binary_search_compare<false>(k, 0, count(), comp);
  }

  count_type           values_count() const noexcept { return count(); }
  constexpr count_type max_values_count() const noexcept { return max_count(); }

  // Inserts the value x at position i, shifting all existing values and
  // children at positions >= i to the right by 1.
  template <typename T>
  void insert_value(count_type i, T&& x) {
    shift_values_right(i, values_count(), 1);
    value_init(i, std::forward<T>(x));
    set_count(count() + 1);
  }

  // Removes the value at position i, shifting all existing values and children
  // at positions > i to the left by 1.
  void remove_value(count_type i) {
    assert(count() >= 1);

    auto old_values_count = values_count();
    set_count(count() - 1);
    // Shift values behind the removed value left.
    if (i + 1 < old_values_count) {
      shift_values_left(i + 1, old_values_count, 1);
    }
  }

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(node_borrower sibling, count_type to_move);
  void rebalance_left_to_right(node_borrower sibling, count_type to_move);

 protected:
  void set_count(count_type v) noexcept {
    assert(0 <= v && v <= max_count());
    count_ = v;
  }

  template <typename T>
  void value_init(count_type i, T&& x) {
    values_[i] = std::forward<T>(x);
  }

  mutable_value_type&& extract_value(count_type i) { return std::move(values_[i]); }

  void replace_value(count_type i, mutable_value_type&& v) { values_[i] = std::move(v); }

  // Returns the position of the first value whose key is not less than k using
  // binary search.
  // If WithEqual = false, returns the position of the first value whose key is
  // greater than k using binary search.
  template <bool WithEqual = true>
  search_result binary_search_compare(
      const key_type& k, count_type s, count_type e, const key_compare& comp
  ) const noexcept(noexcept(comp(std::declval<const key_type&>(), k)))
  requires comp_return_weak_ordering<key_type, key_compare>
  {
    bool is_exact_match = false;
    while (s != e) {
      count_type mid = (s + e) / 2;
      auto       c   = comp(key(mid), k);
      if (c < 0) {
        s = mid + 1;
      } else if (c > 0) {
        e = mid;
      } else {
        if constexpr (WithEqual) {
          is_exact_match = true;
          e              = mid;
        } else {
          s = mid + 1;
        }
      }
    }
    return search_result(s, is_exact_match);
  }

  template <bool WithEqual = true>
  search_result binary_search_compare(
      const key_type& k, count_type s, count_type e, const key_compare& comp
  ) const noexcept(noexcept(comp(std::declval<const key_type&>(), k)))
  requires comp_return_bool<key_type, key_compare>
  {
    bool is_exact_match = false;
    while (s != e) {
      count_type  mid     = (s + e) / 2;
      const auto& mid_key = key(mid);
      if constexpr (WithEqual) {
        if (comp(mid_key, k)) {
          s = mid + 1;
        } else {
          // I know mid_key >= k, so check k >= mid_key.
          // If is_exact_match is true, mid_key <= key(e) <= k.
          if ((not is_exact_match) and (not comp(k, mid_key))) {
            is_exact_match = true;
          }
          e = mid;
        }
      } else {
        if (comp(k, mid_key)) {
          e = mid;
        } else {
          s = mid + 1;
        }
      }
    }
    return search_result(s, is_exact_match);
  }

  // Returns the pointer to the front of the values array.
  values_iterator begin_values() noexcept { return values_.begin(); }

  // Returns the pointer to the back of the values array.
  values_iterator end_values(
  ) noexcept(noexcept(std::next(std::declval<values_iterator>(), std::declval<count_type>()))) {
    return std::next(begin_values(), count());
  }

  values_reverse_iterator rbegin_values() noexcept(noexcept(std::reverse_iterator(end_values()))) {
    return std::reverse_iterator(end_values());
  }
  values_reverse_iterator rend_values() noexcept { return std::reverse_iterator(begin_values()); }

  // Shift values from first to last by shift toward right.
  void shift_values_right(count_type first, count_type last, count_type shift) {
    assert(0 <= first);
    assert(first <= last);
    assert(0 <= shift);
    assert(last + shift <= max_values_count());
    auto begin = std::next(begin_values(), first);
    auto end   = std::next(begin_values(), last);
    std::move_backward(begin, end, std::next(end, shift));
  }

  // Shift values from first to last by shift toward left.
  void shift_values_left(count_type first, count_type last, count_type shift) {
    assert(0 <= first);
    assert(first <= last);
    assert(last <= max_values_count());
    assert(0 <= shift);
    assert(0 <= first - shift);
    auto begin = std::next(begin_values(), first);
    auto end   = std::next(begin_values(), last);
    std::move(begin, end, std::prev(begin, shift));
  }

  template <class P>
  friend void merge(
      btree_node_borrower<pmr::details::btree_leaf_node<P>> left,
      btree_node_borrower<pmr::details::btree_leaf_node<P>> right
  );

  template <class P>
  friend void split(
      btree_node_borrower<pmr::details::btree_leaf_node<P>> left,
      btree_node_owner<pmr::details::btree_leaf_node<P>>&&  right,
      typename pmr::details::btree_leaf_node<P>::count_type insert_position
  );

  // The array of values.
  values_type values_;
  // A pointer to the node's parent.
  node_borrower parent_;
  // The position of the node in the node's parent.
  count_type position_;
  // The count of the number of values in the node.
  count_type count_;
};

template <typename P, typename N>
void btree_base_node<P, N>::rebalance_right_to_left(
    typename btree_base_node<P, N>::node_borrower right,
    typename btree_base_node<P, N>::count_type    to_move
) {
  assert(borrow_readonly_parent() == right->borrow_readonly_parent());
  assert(borrow_readonly_parent() != nullptr);
  assert(position() + 1 == right->position());
  assert(right->count() >= count());
  assert(to_move >= 1);
  assert(to_move <= right->count());

  auto base_right  = static_cast<btree_node_borrower<btree_base_node>>(right);
  auto base_parent = static_cast<btree_node_borrower<btree_base_node>>(borrow_parent());

  // Move the delimiting value to the left node.
  this->replace_value(values_count(), base_parent->extract_value(position()));

  // Move the values from the right to the left node.
  std::move(base_right->begin_values(), base_right->begin_values() + to_move - 1, end_values() + 1);
  // Move the new delimiting value from the right node.
  base_parent->replace_value(position(), base_right->extract_value(to_move - 1));
  // Shift the values in the right node to their correct position.
  base_right->shift_values_left(to_move, base_right->values_count(), to_move);

  // Fixup the counts on the right and left nodes.
  set_count(count() + to_move);
  base_right->set_count(base_right->count() - to_move);
}

template <typename P, typename N>
void btree_base_node<P, N>::rebalance_left_to_right(
    typename btree_base_node<P, N>::node_borrower right,
    typename btree_base_node<P, N>::count_type    to_move
) {
  assert(borrow_readonly_parent() == right->borrow_readonly_parent());
  assert(borrow_readonly_parent() != nullptr);
  assert(position() + 1 == right->position());
  assert(count() >= right->count());
  assert(to_move >= 1);
  assert(to_move <= count());

  auto base_right  = static_cast<btree_node_borrower<btree_base_node>>(right);
  auto base_parent = static_cast<btree_node_borrower<btree_base_node>>(borrow_parent());

  base_right->shift_values_right(0, base_right->values_count(), to_move);

  // Move the delimiting value to the right node.
  base_right->replace_value(to_move - 1, base_parent->extract_value(position()));

  // Move the values from the left to the right node.
  std::move(end_values() - to_move + 1, end_values(), base_right->begin_values());
  // Move the new delimiting value from the left node.
  base_parent->replace_value(position(), this->extract_value(values_count() - to_move));

  // Fixup the counts on the left and right nodes.
  set_count(count() - to_move);
  base_right->set_count(base_right->count() + to_move);
}

// Free functions
template <class Params, class Node>
typename btree_base_node<Params, Node>::count_type position(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->position();
}

template <class Params, class Node>
void set_position(
    btree_node_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type v
) noexcept {
  assert(n != nullptr);
  n->set_position(v);
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::count_type count(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->count();
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::count_type max_count(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->max_count();
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::node_borrower borrow_parent(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->borrow_parent();
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::node_readonly_borrower borrow_readonly_parent(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->borrow_readonly_parent();
}

template <class Params, class Node>
bool is_root(btree_node_readonly_borrower<btree_base_node<Params, Node>> n) noexcept {
  assert(n != nullptr);
  return n->is_root();
}

template <class Params, class Node>
void make_root(btree_node_borrower<btree_base_node<Params, Node>> n) noexcept {
  assert(n != nullptr);
  n->make_root();
}

template <class Params, class Node>
const typename btree_base_node<Params, Node>::key_type& key(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type          i
) noexcept {
  assert(n != nullptr);
  return n->key(i);
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::reference value(
    btree_node_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type i
) noexcept {
  assert(n != nullptr);
  return n->value(i);
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::const_reference value(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type          i
) noexcept {
  assert(n != nullptr);
  return n->value(i);
}

template <class Params, class Node>
void value_swap(
    btree_node_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type i,
    btree_node_borrower<btree_base_node<Params, Node>> x,
    typename btree_base_node<Params, Node>::count_type j
) {
  assert(n != nullptr);
  n->value_swap(i, x, j);
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::search_result lower_bound(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n,
    const typename btree_base_node<Params, Node>::key_type&     k,
    const typename btree_base_node<Params, Node>::key_compare&  comp
) {
  assert(n != nullptr);
  return n->lower_bound(k, comp);
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::search_result upper_bound(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n,
    const typename btree_base_node<Params, Node>::key_type&     k,
    const typename btree_base_node<Params, Node>::key_compare&  comp
) {
  assert(n != nullptr);
  return n->upper_bound(k, comp);
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::count_type values_count(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->values_count();
}

template <class Params, class Node>
typename btree_base_node<Params, Node>::count_type max_values_count(
    btree_node_readonly_borrower<btree_base_node<Params, Node>> n
) noexcept {
  assert(n != nullptr);
  return n->max_values_count();
}

template <class Params, class Node, class T>
void insert_value(
    btree_node_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type i,
    T&&                                                x
) {
  assert(n != nullptr);
  n->insert_value(i, x);
}

template <class Params, class Node>
void remove_value(
    btree_node_borrower<btree_base_node<Params, Node>> n,
    typename btree_base_node<Params, Node>::count_type i
) {
  assert(n != nullptr);
  n->remove_value(i);
}
}  // namespace commons
}  // namespace platanus

#endif
