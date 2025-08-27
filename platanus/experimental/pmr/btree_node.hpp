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

#ifndef PLATANUS_EXPERIMENTAL_PMR_BTREE_NODE_H_
#define PLATANUS_EXPERIMENTAL_PMR_BTREE_NODE_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <memory_resource>
#include <type_traits>

#include "../../internal/btree_node_fwd.hpp"
#include "../../internal/btree_util.hpp"
#include "../../internal/btree_base_node.hpp"
#include "../../pmr/polymorphic_allocator.hpp"

namespace platanus {
namespace experimental::pmr {

// A leaf node in the btree holding.
template <typename Params>
class btree_leaf_node : public internal::btree_base_node<Params, btree_leaf_node<Params>> {
 public:
  using self_type  = btree_leaf_node;
  using super_type = internal::btree_base_node<Params, self_type>;

  static constexpr std::size_t kNodeValues   = super_type::kNodeValues;
  static constexpr std::size_t kNodeChildren = super_type::kNodeChildren;

  using key_type           = typename super_type::key_type;
  using mapped_type        = typename super_type::mapped_type;
  using value_type         = typename super_type::value_type;
  using mutable_value_type = typename super_type::mutable_value_type;
  using pointer            = typename super_type::pointer;
  using const_pointer      = typename super_type::const_pointer;
  using reference          = typename super_type::reference;
  using const_reference    = typename super_type::const_reference;
  using key_compare        = typename super_type::key_compare;
  using value_compare      = typename super_type::value_compare;
  using size_type          = typename super_type::size_type;
  using difference_type    = typename super_type::difference_type;

  using allocator_type = platanus::pmr::polymorphic_allocator<std::byte>;

  using search_result     = typename super_type::search_result;
  using count_type        = typename search_result::count_type;
  using signed_count_type = typename search_result::signed_count_type;

  using values_type                   = typename super_type::values_type;
  using values_iterator               = typename values_type::iterator;
  using values_const_iterator         = typename values_type::const_iterator;
  using values_reverse_iterator       = typename values_type::reverse_iterator;
  using values_const_reverse_iterator = typename values_type::const_reverse_iterator;

  using node_borrower          = typename super_type::node_borrower;
  using node_readonly_borrower = typename super_type::node_readonly_borrower;

  btree_leaf_node()                             = default;
  btree_leaf_node(btree_leaf_node&&)            = default;
  btree_leaf_node& operator=(btree_leaf_node&&) = default;
  ~btree_leaf_node()                            = default;

  btree_leaf_node(const btree_leaf_node&) = delete;
  void operator=(const btree_leaf_node&)  = delete;

  explicit btree_leaf_node(btree_leaf_node* parent) : super_type(parent), is_leaf_(true) {}

  using super_type::position;
  using super_type::set_position;

  using super_type::count;
  using super_type::max_count;

  using super_type::borrow_parent;
  using super_type::borrow_readonly_parent;

  using super_type::is_root;
  using super_type::make_root;

  using super_type::key;
  using super_type::value;

  using super_type::value_swap;

  using super_type::lower_bound;
  using super_type::upper_bound;

  using super_type::max_values_count;
  using super_type::values_count;

  using super_type::insert_value;
  using super_type::remove_value;

  using super_type::rebalance_left_to_right;
  using super_type::rebalance_right_to_left;

  // If *this is a leaf node, return true: otherwise, false. This value doesn't
  // change after the node is created.
  bool is_leaf() const noexcept { return is_leaf_; }

  count_type children_count() const noexcept { return is_leaf() ? 0 : count() + 1; }
  count_type max_children_count() const noexcept { return is_leaf() ? 0 : max_count() + 1; }

 protected:
  static void set_parent(node_borrower self, node_borrower parent) noexcept {
    self->parent_ = parent;
  }
  static void set_position(node_borrower self, count_type position) noexcept {
    self->position_ = position;
  }

  using super_type::set_count;

  using super_type::value_init;

  using super_type::extract_value;
  using super_type::replace_value;

  using super_type::binary_search_compare;

  using super_type::begin_values;
  using super_type::end_values;

  using super_type::rbegin_values;
  using super_type::rend_values;

  using super_type::shift_values_left;
  using super_type::shift_values_right;

  template <class P>
  friend void merge(
      internal::btree_node_borrower<btree_leaf_node<P>> left,
      internal::btree_node_borrower<btree_leaf_node<P>> right
  );

  template <class P>
  friend void split(
      internal::btree_node_borrower<btree_leaf_node<P>> left,
      internal::btree_node_owner<btree_leaf_node<P>>&&  right,
      typename btree_leaf_node<P>::count_type          insert_position
  );

  // Indicate whether this node is leaf or not.
  bool is_leaf_;
};

// A internal node in the btree holding.
template <typename Params>
class btree_internal_node : public btree_leaf_node<Params> {
 public:
  using self_type      = btree_internal_node<Params>;
  using super_type     = btree_leaf_node<Params>;
  using allocator_type = typename super_type::allocator_type;

  using node_borrower          = internal::btree_node_borrower<super_type>;
  using node_readonly_borrower = internal::btree_node_readonly_borrower<super_type>;

  class node_deleter {
   public:
    using super_type       = allocator_type;
    using allocator_traits = std::allocator_traits<allocator_type>;

    node_deleter()                               = default;
    node_deleter(const node_deleter&)            = default;
    node_deleter(node_deleter&&)                 = default;
    node_deleter& operator=(const node_deleter&) = default;
    node_deleter& operator=(node_deleter&&)      = default;
    ~node_deleter()                              = default;

    explicit node_deleter(allocator_type alloc) : alloc_(alloc) {}

    void operator()(node_borrower p) {
      if (p != nullptr) {
        if (p->is_leaf()) {
          allocator_traits::destroy(alloc_, p);
          alloc_.deallocate_bytes(p, sizeof(super_type), alignof(super_type));
        } else {
          auto* ip = static_cast<btree_internal_node*>(p);
          // TODO Why memory leak happens when using children_count()?
          // When using kNodeChildren, this deleter don't leak memory.
          for (count_type i = 0; i < kNodeChildren; ++i) {
            if (ip->children_[i]) {
              allocator_traits::destroy(alloc_, &(ip->children_[i]));
            }
          }
          alloc_.deallocate_bytes(ip, sizeof(btree_internal_node), alignof(btree_internal_node));
        }
      }
    }

    void swap(node_deleter& x) noexcept { alloc_.swap_resource(x.alloc_); }

   private:
    allocator_type alloc_;
  };

  // node_owner owes to allocate and release the memory of the node.
  using node_owner = std::unique_ptr<super_type, node_deleter>;

  static constexpr std::size_t kNodeValues   = super_type::kNodeValues;
  static constexpr std::size_t kNodeChildren = super_type::kNodeChildren;

  using children_type                   = std::array<node_owner, kNodeChildren>;
  using children_iterator               = typename children_type::iterator;
  using children_const_iterator         = typename children_type::const_iterator;
  using children_reverse_iterator       = std::reverse_iterator<children_iterator>;
  using children_const_reverse_iterator = std::reverse_iterator<children_const_iterator>;

  using key_type           = typename super_type::key_type;
  using mapped_type        = typename super_type::mapped_type;
  using value_type         = typename super_type::value_type;
  using mutable_value_type = typename super_type::mutable_value_type;
  using pointer            = typename super_type::pointer;
  using const_pointer      = typename super_type::const_pointer;
  using reference          = typename super_type::reference;
  using const_reference    = typename super_type::const_reference;
  using key_compare        = typename super_type::key_compare;
  using value_compare      = typename super_type::value_compare;
  using size_type          = typename super_type::size_type;
  using difference_type    = typename super_type::difference_type;

  using search_result     = typename super_type::search_result;
  using count_type        = typename search_result::count_type;
  using signed_count_type = typename search_result::signed_count_type;

  using values_type                   = typename super_type::values_type;
  using values_iterator               = typename values_type::iterator;
  using values_const_iterator         = typename values_type::const_iterator;
  using values_reverse_iterator       = typename values_type::reverse_iterator;
  using values_const_reverse_iterator = typename values_type::const_reverse_iterator;

  btree_internal_node()                                 = default;
  btree_internal_node(btree_internal_node&&)            = default;
  btree_internal_node& operator=(btree_internal_node&&) = default;
  ~btree_internal_node()                                = default;

  btree_internal_node(const btree_internal_node&) = delete;
  void operator=(const btree_internal_node&)      = delete;

  explicit btree_internal_node(node_borrower parent) : super_type(parent) {
    this->is_leaf_ = false;
  }

  using super_type::is_leaf;

  using super_type::position;
  using super_type::set_position;

  using super_type::count;
  using super_type::max_count;

  using super_type::borrow_parent;
  using super_type::borrow_readonly_parent;

  using super_type::is_root;
  using super_type::make_root;

  using super_type::key;
  using super_type::value;

  using super_type::value_swap;

  using super_type::lower_bound;
  using super_type::upper_bound;

  using super_type::max_values_count;
  using super_type::values_count;

  using super_type::children_count;
  using super_type::max_children_count;

  using super_type::insert_value;
  using super_type::remove_value;

  // Getters/setter for the child at position i in the node.
  node_borrower          borrow_child(count_type i) const noexcept { return children_[i].get(); }
  node_readonly_borrower borrow_readonly_child(count_type i) const noexcept {
    return children_[i].get();
  }

  node_owner extract_child(count_type i) noexcept { return std::move(children_[i]); }
  void       set_child(count_type i, node_owner&& new_child) noexcept {
    children_[i]            = std::move(new_child);
    auto borrowed_new_child = borrow_child(i);
    super_type::set_parent(borrowed_new_child, this);
    super_type::set_position(borrowed_new_child, i);
  }

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(node_borrower base_right, count_type to_move) {
    assert(not base_right->is_leaf());
    auto right = static_cast<internal::btree_node_borrower<self_type>>(base_right);

    // Move the child pointers from the right to the left node.
    receive_children_n(end_children(), right, right->begin_children(), to_move);
    right->shift_children_left(to_move, right->children_count(), to_move);
    super_type::rebalance_right_to_left(right, to_move);
  }
  void rebalance_left_to_right(node_borrower base_right, count_type to_move) {
    assert(not base_right->is_leaf());
    auto right = static_cast<internal::btree_node_borrower<self_type>>(base_right);

    // Move the child pointers from the left node to the right node.
    right->shift_children_right(0, right->children_count(), to_move);
    right->receive_children_n(
        right->begin_children(),
        this,
        begin_children() + children_count() - to_move,
        to_move
    );
    super_type::rebalance_left_to_right(right, to_move);
  }

 private:
  using super_type::set_count;

  using super_type::value_init;

  using super_type::extract_value;
  using super_type::replace_value;

  using super_type::binary_search_compare;

  using super_type::begin_values;
  using super_type::end_values;

  using super_type::rbegin_values;
  using super_type::rend_values;

  using super_type::shift_values_left;
  using super_type::shift_values_right;

  // Returns the pointer to the front of the children array.
  children_iterator begin_children() noexcept { return children_.begin(); }

  // Returns the pointer to the back of the children array.
  children_iterator end_children(
  ) noexcept(noexcept(std::next(std::declval<children_iterator>(), std::declval<int>()))) {
    return std::next(begin_children(), count() + 1);
  }

  children_reverse_iterator rbegin_children() noexcept(noexcept(std::reverse_iterator(end_children()
  ))) {
    return std::reverse_iterator(end_children());
  }
  children_reverse_iterator rend_children() noexcept(noexcept(std::reverse_iterator(begin_children()
  ))) {
    return std::reverse_iterator(begin_children());
  }

  // Shift the children in the range [first, last) to the right by shift.
  // CAUTION: This function  uninitializes [first, first + shift) in *this.
  void shift_children_right(count_type first, count_type last, count_type shift) {
    assert(0 <= first && first <= last && 0 <= shift && last + shift <= max_children_count());

    auto begin = begin_children() + first;
    auto end   = begin_children() + last;

    std::move_backward(begin, end, end + shift);
    std::for_each(begin + shift, end + shift, [shift](node_owner& np) {
      np->set_position(np->position() + shift);
    });
  }

  // Shift the children in the range [first, last) to the left by shift.
  // CAUTION: This function  uninitializes [last - shift, last) in *this.
  void shift_children_left(count_type first, count_type last, count_type shift) {
    assert(0 <= first && first <= last && last <= max_children_count());
    assert(0 <= shift && 0 <= first - shift);

    auto begin = begin_children() + first;
    auto end   = begin_children() + last;

    std::move(begin, end, begin - shift);
    std::for_each(begin - shift, end - shift, [shift](node_owner& np) {
      np->set_position(np->position() - shift);
    });
  }

  // Receive the children from [first, last) in *src and set them to [dest, dest + last - first) in
  // *this. CAUTION: This function doesn't uninitialize [first, last) in *src.
  void receive_children(
      children_iterator dest, node_borrower src, children_iterator first, children_iterator last
  ) {
    auto n = last - first;
    assert(0 <= n && n <= std::numeric_limits<int>::max());
    receive_children_n(dest, src, first, static_cast<int>(n));
  }

  // Receive the children from [first, first + n) in *src and set them to [dest, dest + n) in *this.
  // CAUTION: This function doesn't uninitialize [first, last) in *src.
  void receive_children_n(
      children_iterator dest, node_borrower src, children_iterator first, count_type n
  ) {
    assert(not src->is_leaf());
    auto casted_src = static_cast<internal::btree_node_borrower<self_type>>(src);

    auto dest_idx  = dest - begin_children();
    auto first_idx = first - casted_src->begin_children();
    assert(0 <= dest_idx && 0 <= first_idx);
    assert(
        0 <= n && dest_idx + n <= max_children_count()
        && first_idx + n <= casted_src->max_children_count()
    );
    for (count_type i = 0; i < n; ++i) {
      set_child(dest_idx + i, casted_src->extract_child(first_idx + i));
    }
  }

  template <class P>
  friend void merge(
      internal::btree_node_borrower<btree_leaf_node<P>> left,
      internal::btree_node_borrower<btree_leaf_node<P>> right
  );

  template <class P>
  friend void split(
      internal::btree_node_borrower<btree_leaf_node<P>> left,
      internal::btree_node_owner<btree_leaf_node<P>>&&  right,
      typename btree_leaf_node<P>::count_type          insert_position
  );

  // The array of child node. The keys in children_[i] are all less than
  // key(i). The keys in children_[i + 1] are all greater than key(i). There
  // are always count + 1 children.
  children_type children_;
};

// Merges a node with its right sibling, moving all of the values and the
// delimiting key in the parent node onto itself.
template <class Params>
void merge(
    internal::btree_node_borrower<btree_leaf_node<Params>> left,
    internal::btree_node_borrower<btree_leaf_node<Params>> right
) {
  assert(left->borrow_readonly_parent() == right->borrow_readonly_parent());
  assert(left->borrow_readonly_parent() != nullptr);
  assert(left->position() + 1 == right->position());

  // Move the delimiting value to the left node.
  left->replace_value(left->values_count(), left->borrow_parent()->extract_value(left->position()));

  // Move the values from the right to the left node.
  std::move(right->begin_values(), right->end_values(), left->end_values() + 1);

  if (not left->is_leaf()) {
    assert(not right->is_leaf());
    auto casted_left = static_cast<internal::btree_node_borrower<btree_internal_node<Params>>>(left);
    auto casted_right =
        static_cast<internal::btree_node_borrower<btree_internal_node<Params>>>(right);
    // Move the child pointers from the right to the left node.
    casted_left->receive_children(
        casted_left->end_children(),
        casted_right,
        casted_right->begin_children(),
        casted_right->end_children()
    );
  }

  // Fixup the counts on the right and dest nodes.
  left->set_count(1 + left->count() + right->count());
  right->set_count(0);

  // Shift children behind the removed child left.
  if (left->position() + 2 < left->borrow_parent()->children_count()) {
    assert(not left->borrow_parent()->is_leaf());
    auto casted_parent =
        static_cast<internal::btree_node_borrower<btree_internal_node<Params>>>(left->borrow_parent()
        );

    casted_parent
        ->shift_children_left(left->position() + 2, left->borrow_parent()->children_count(), 1);
  }

  // Remove the value on the parent node.
  left->borrow_parent()->remove_value(left->position());
}

// Splits a node, moving a portion of the node's values to its right sibling.
template <class Params>
void split(
    internal::btree_node_borrower<btree_leaf_node<Params>> left,
    internal::btree_node_owner<btree_leaf_node<Params>>&&  right,
    typename btree_leaf_node<Params>::count_type          insert_position
) {
  using internal_node = btree_internal_node<Params>;
  using count_type    = typename internal_node::count_type;

  using internal_node_borrower = internal::btree_node_borrower<internal_node>;

  assert(left != nullptr);
  assert(right.get() != nullptr);
  assert(right->count() == 0);
  assert(left->borrow_readonly_parent() != nullptr);
  assert(not left->borrow_readonly_parent()->is_leaf());

  // We bias the split based on the position being inserted. If we're
  // inserting at the beginning of the left node then bias the split to put
  // more values on the right node. If we're inserting at the end of the
  // right node then bias the split to put more values on the left node.
  count_type to_move = 0;
  if (insert_position == 0) {
    to_move = left->count() - 1;
  } else if (insert_position != left->max_children_count() - 1) {
    to_move = left->count() / 2;
  }
  assert(left->count() - to_move >= 1);

  std::move(left->end_values() - to_move, left->end_values(), right->begin_values());
  left->set_count(left->count() - to_move);
  right->set_count(to_move);

  // The split key is the largest value in the left sibling.
  left->set_count(left->count() - 1);
  auto parent_node = static_cast<internal_node_borrower>(left->borrow_parent());
  parent_node->insert_value(left->position(), left->extract_value(left->values_count()));
  // Insert dest as a child of parent.
  parent_node
      ->shift_children_right(left->position() + 1, left->borrow_parent()->children_count() - 1, 1);
  parent_node->set_child(left->position() + 1, std::move(right));

  if (not left->is_leaf()) {
    auto casted_left = static_cast<internal_node_borrower>(left);

    auto right = parent_node->borrow_child(left->position() + 1);
    assert(not right->is_leaf());
    auto casted_right = static_cast<internal_node_borrower>(right);

    for (count_type i = 0; i <= right->count(); ++i) {
      assert(casted_left->borrow_child(casted_left->children_count() + i) != nullptr);
    }
    casted_right->receive_children_n(
        casted_right->begin_children(),
        casted_left,
        casted_left->end_children(),
        casted_right->count() + 1
    );
  }
}
}  // namespace experimental::pmr

namespace internal {
template <class Params>
struct btree_node_owner_type<experimental::pmr::btree_leaf_node<Params>> {
  using type = typename experimental::pmr::btree_internal_node<Params>::node_owner;
};

template <class Params>
struct sizeof_leaf_node<experimental::pmr::btree_leaf_node<Params>> {
  static constexpr std::size_t value = sizeof(experimental::pmr::btree_leaf_node<Params>);
};

template <class Params>
struct sizeof_internal_node<experimental::pmr::btree_leaf_node<Params>> {
  static constexpr std::size_t value = sizeof(experimental::pmr::btree_internal_node<Params>);
};

template <class Params>
bool is_leaf(btree_node_readonly_borrower<experimental::pmr::btree_leaf_node<Params>> n) noexcept {
  return n->is_leaf();
}

template <class Params>
btree_node_borrower<experimental::pmr::btree_leaf_node<Params>> borrow_child(
    btree_node_readonly_borrower<experimental::pmr::btree_leaf_node<Params>> n,
    typename experimental::pmr::btree_internal_node<Params>::count_type      i
) noexcept {
  assert(n != nullptr);
  assert(not n->is_leaf());

  auto in = static_cast<btree_node_readonly_borrower<experimental::pmr::btree_internal_node<Params>>>(n);
  return in->borrow_child(i);
}

template <class Params>
btree_node_readonly_borrower<experimental::pmr::btree_leaf_node<Params>> borrow_readonly_child(
    btree_node_readonly_borrower<experimental::pmr::btree_leaf_node<Params>> n,
    typename experimental::pmr::btree_internal_node<Params>::count_type      i
) noexcept {
  assert(n != nullptr);
  assert(not n->is_leaf());

  auto in = static_cast<btree_node_readonly_borrower<experimental::pmr::btree_internal_node<Params>>>(n);
  return in->borrow_readonly_child(i);
}

template <class Params>
btree_node_owner<experimental::pmr::btree_leaf_node<Params>> extract_child(
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     n,
    typename experimental::pmr::btree_internal_node<Params>::count_type i
) noexcept {
  assert(n != nullptr);
  assert(not n->is_leaf());

  auto in = static_cast<btree_node_borrower<experimental::pmr::btree_internal_node<Params>>>(n);
  return in->extract_child(i);
}

template <class Params>
void set_child(
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     n,
    typename experimental::pmr::btree_internal_node<Params>::count_type i,
    btree_node_owner<experimental::pmr::btree_leaf_node<Params>>&&      new_child
) noexcept {
  assert(n != nullptr);
  assert(not n->is_leaf());

  auto in = static_cast<btree_node_borrower<experimental::pmr::btree_internal_node<Params>>>(n);
  in->set_child(i, std::move(new_child));
}

template <class Params>
void rebalance_right_to_left(
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     n,
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     sibling,
    typename experimental::pmr::btree_internal_node<Params>::count_type to_move
) {
  assert(n != nullptr);
  if (n->is_leaf()) {
    n->rebalance_right_to_left(sibling, to_move);
  } else {
    auto in = static_cast<btree_node_borrower<experimental::pmr::btree_internal_node<Params>>>(n);
    in->rebalance_right_to_left(sibling, to_move);
  }
}

template <class Params>
void rebalance_left_to_right(
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     n,
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     sibling,
    typename experimental::pmr::btree_internal_node<Params>::count_type to_move
) {
  assert(n != nullptr);
  if (n->is_leaf()) {
    n->rebalance_left_to_right(sibling, to_move);
  } else {
    auto in = static_cast<btree_node_borrower<experimental::pmr::btree_internal_node<Params>>>(n);
    in->rebalance_left_to_right(sibling, to_move);
  }
}

template <class Params>
void split(
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>>     left,
    btree_node_owner<experimental::pmr::btree_leaf_node<Params>>&&      right,
    typename experimental::pmr::btree_internal_node<Params>::count_type insert_position
) {
  experimental::pmr::split(left, std::move(right), insert_position);
}

template <class Params>
void merge(
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>> left,
    btree_node_borrower<experimental::pmr::btree_leaf_node<Params>> right
) {
  experimental::pmr::merge(left, std::move(right));
}
}  // namespace internal
}  // namespace platanus

namespace std {
template <class Params, class Alloc>
struct uses_allocator<platanus::experimental::pmr::btree_leaf_node<Params>, Alloc>
    : public std::false_type {};

template <class Params, class Alloc>
struct uses_allocator<platanus::experimental::pmr::btree_internal_node<Params>, Alloc>
    : public std::false_type {};
}  // namespace std

namespace platanus::experimental::pmr {
template <class Params>
class btree_node_factory {
 public:
  using leaf_node     = btree_leaf_node<Params>;
  using internal_node = btree_internal_node<Params>;
  using node_deleter  = typename internal_node::node_owner::deleter_type;
  using node_owner    = internal::btree_node_owner<leaf_node>;
  using node_borrower = internal::btree_node_borrower<leaf_node>;

  using allocator_type = typename internal_node::allocator_type;

  btree_node_factory()                                     = default;
  btree_node_factory(btree_node_factory&&)                 = default;
  btree_node_factory& operator=(const btree_node_factory&) = default;
  btree_node_factory& operator=(btree_node_factory&&)      = default;
  ~btree_node_factory()                                    = default;

  btree_node_factory(const btree_node_factory& x)
      : alloc_(std::allocator_traits<allocator_type>::select_on_container_copy_construction(x.alloc_
        )) {}

  explicit btree_node_factory(const allocator_type& alloc) : alloc_(alloc) {}

  node_owner make_node(bool is_leaf, node_borrower parent) {
    leaf_node* node_ptr;
    if (is_leaf) {
      node_ptr =
          static_cast<leaf_node*>(alloc_.allocate_bytes(sizeof(leaf_node), alignof(leaf_node)));
      alloc_.construct(node_ptr, parent);
    } else {
      auto* internal_node_ptr = static_cast<internal_node*>(
          alloc_.allocate_bytes(sizeof(internal_node), alignof(internal_node))
      );
      alloc_.construct(internal_node_ptr, parent);
      node_ptr = static_cast<leaf_node*>(internal_node_ptr);
    }
    return node_owner(node_ptr, node_deleter{alloc_});
  }

  node_owner make_root_node(bool is_leaf) { return make_node(is_leaf, nullptr); }

  void swap(btree_node_factory& other) noexcept { alloc_.swap_resource(other.alloc_); }

 private:
  allocator_type alloc_;
};

template <class Params>
void swap(btree_node_factory<Params>& left, btree_node_factory<Params>& right) noexcept {
  left.swap(right);
}
}  // namespace platanus::experimental::pmr

#endif  // PLATANUS_PMR_DETAILS_BTREE_NODE_H_
