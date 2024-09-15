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

#ifndef PLATANUS_BTREE_NODE_H_
#define PLATANUS_BTREE_NODE_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <memory>
#include <memory_resource>
#include <type_traits>

#include "btree_util.h"
#include "btree_node_common.h"

namespace platanus {
namespace pmr {

// A leaf node in the btree holding.
template <typename Params>
class btree_leaf_node : public btree_base_node<Params, btree_leaf_node> {
 public:
  using self_type  = btree_leaf_node;
  using super_type = btree_base_node<Params, btree_leaf_node>;

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

  using values_type                   = typename values_type::values_type;
  using values_iterator               = typename values_type::iterator;
  using values_const_iterator         = typename values_type::const_iterator;
  using values_reverse_iterator       = typename values_type::reverse_iterator;
  using values_const_reverse_iterator = typename values_type::const_reverse_iterator;

  using node_borrower          = self_type*;
  using node_readonly_borrower = self_type const*;

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

  using super_type::values_count;
  using super_type::max_values_count;

  using super_type::insert_value;
  using super_type::remove_value;

  using super_type::rebalance_right_to_left;
  using super_type::rebalance_left_to_right;

  using super_type::merge;

  // If *this is a leaf node, return true: otherwise, false. This value doesn't
  // change after the node is created.
  bool is_leaf() const noexcept { return is_leaf_; }

  void swap(btree_leaf_node& x) {
    super_type::swap(x);
    btree_swap_helper(is_leaf_, x.is_leaf_);
  }

 protected:
  using super_type::set_count

  using super_type::value_init

  using super_type::extract_value
  using super_type::replace_value

  using super_type::binary_search_compare;

  using super_type::begin_values;
  using super_type::end_values;

  using super_type::rbegin_values;
  using super_type::rend_values;

  using super_type::shift_values_right;
  using super_type::shift_values_left;

  count_type children_count() const noexcept { return is_leaf() ? 0 : count() + 1; }
  count_type max_children_count() const noexcept { return is_leaf() ? 0 : max_count() + 1; }

  // Indicate whether this node is leaf or not.
  bool is_leaf_;
};

// A internal node in the btree holding.
template <typename Params>
class btree_internal_node : public btree_leaf_node<Params> {
 public:
  using super_type     = btree_leaf_node<Params>;
  using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

  class node_deleter : allocator_type {
   public:
    using super_type = allocator_type;

    node_deleter()                               = default;
    node_deleter(const node_deleter&)            = default;
    node_deleter(node_deleter&&)                 = default;
    node_deleter& operator=(const node_deleter&) = default;
    node_deleter& operator=(node_deleter&&)      = default;
    ~node_deleter()                              = default;

    explicit node_deleter(const allocator_type& alloc) : super_type(alloc) {}

    void operator()(super_type* p) {
      if (p != nullptr) {
        if (p->is_leaf()) {
          this->destroy(p);
          this->deallocate_bytes(p, sizeof(btree_leaf_node), alignof(btree_leaf_node));
        } else {
          auto* ip = static_cast<btree_internal_node*>(p);
          for (count_type i = 0; i < ip->children_count(); ++i) {
            if (p->children_[i]) {
              this->destroy(p->children_[i]);
            }
          }
          this->deallocate_bytes(p, sizeof(btree_internal_node), alignof(btree_internal_node));
        }
      }
    }

    void swap(node_deleter& x) { assert(x == *this); }
  };

  // node_owner owes to allocate and release the memory of the node.
  using node_owner = std::unique_ptr<super_type, node_deleter>;

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

  using values_type                   = typename values_type::values_type;
  using values_iterator               = typename values_type::iterator;
  using values_const_iterator         = typename values_type::const_iterator;
  using values_reverse_iterator       = typename values_type::reverse_iterator;
  using values_const_reverse_iterator = typename values_type::const_reverse_iterator;

  using node_borrower          = super_type*;
  using node_readonly_borrower = super_type const*;

  btree_internal_node()                                 = default;
  btree_internal_node(btree_internal_node&&)            = default;
  btree_internal_node& operator=(btree_internal_node&&) = default;
  ~btree_internal_node()                                = default;

  btree_internal_node(const btree_internal_node&) = delete;
  void operator=(const btree_internal_node&)      = delete;

  explicit btree_internal_node(btree_leaf_node* parent) : super_type(parent) { is_leaf_ = false; }

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

  using super_type::values_count;
  using super_type::max_values_count;

  using super_type::insert_value;
  using super_type::remove_value;

  using super_type::rebalance_right_to_left;
  using super_type::rebalance_left_to_right;

  using super_type::merge;

  // Getters/setter for the child at position i in the node.
  node_borrower          borrow_child(count_type i) const noexcept { return children_[i].get(); }
  node_readonly_borrower borrow_readonly_child(count_type i) const noexcept {
    return children_[i].get();
  }

  node_owner extract_child(count_type i) noexcept { return std::move(children_[i]); }
  void       set_child(count_type i, node_owner&& new_child) noexcept {
    children_[i]                  = std::move(new_child);
    auto borrowed_new_child       = borrow_child(i);
    borrowed_new_child->parent_   = this;
    borrowed_new_child->position_ = i;
  }

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(node_borrower sibling, count_type to_move) {
    // Move the child pointers from the right to the left node.
    receive_children_n(end_children(), src, src->begin_children(), to_move);
    src->shift_children_left(to_move, src->children_count(), to_move);
    super_type::rebalance_right_to_left(sibling, to_move);
  }
  void rebalance_left_to_right(node_borrower sibling, count_type to_move) {
    // Move the child pointers from the left node to the right node.
    dest->shift_children_right(0, dest->children_count(), to_move);
    dest->receive_children_n(
        dest->begin_children(),
        this,
        begin_children() + children_count() - to_move,
        to_move
    );
    super_type::rebalance_left_to_right(sibling, to_move);
  }

  // Merges a node with its right sibling, moving all of the values and the
  // delimiting key in the parent node onto itself.
  void merge(node_borrower sibling) {
    // Move the child pointers from the right to the left node.
    receive_children(end_children(), src, src->begin_children(), src->end_children());
    super_type::merge(sibling);
  }

  // Swap the contents of "this" and "x".
  void swap(btree_internal_node& x) {
    super_type::swap(x);

    // Swap the child.
    btree_swap_helper(children_, x.children_);
    // Reset the parent of children.
    std::for_each_n(x.begin_children(), children_count(), [x](node_borrower np) {
      np->parent_ = &x;
    });
    std::for_each_n(begin_children(), x.children_count(), [this](node_borrower np) {
      np->parent_ = this;
    });
  }

 private:
  using super_type::set_count

  using super_type::value_init

  using super_type::extract_value
  using super_type::replace_value

  using super_type::binary_search_compare;

  using super_type::begin_values;
  using super_type::end_values;

  using super_type::rbegin_values;
  using super_type::rend_values;

  using super_type::shift_values_right;
  using super_type::shift_values_left;

  using super_type::children_count;
  using super_type::max_children_count;

  // Returns the pointer to the front of the children array.
  children_iterator begin_children() noexcept { return children_.get(); }

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
    auto dest_idx  = dest - begin_children();
    auto first_idx = first - src->begin_children();
    assert(0 <= dest_idx && 0 <= first_idx);
    assert(
        0 <= n && dest_idx + n <= max_children_count() && first_idx + n <= src->max_children_count()
    );
    for (count_type i = 0; i < n; ++i) {
      set_child(dest_idx + i, src->extract_child(first_idx + i));
    }
  }

  // The array of child node. The keys in children_[i] are all less than
  // key(i). The keys in children_[i + 1] are all greater than key(i). There
  // are always count + 1 children.
  children_type children_;
};
}  // namespace pmr

template <typename Params>
typename pmr::btree_internal_node<Params>::node_owner make_node(
    bool                                                       is_leaf,
    typename pmr::btree_leaf_node<Params>::node_borrower       parent,
    typename pmr::btree_internal_node<Params>::allocator_type& alloc
) {
  using leaf_node     = pmr::btree_leaf_node<Params>;
  using internal_node = pmr::btree_internal_node<Params>;
  using node_deleter  = typename internal_node::node_owner<Node>::deleter_type;

  leaf_node* node_ptr;
  if (is_leaf) {
    node_ptr = static_cast<leaf_node*>(alloc.allocate_bytes(sizeof(leaf_node), alignof(leaf_node)));
    alloc.construct(node_ptr, parent);
  } else {
    auto* internal_node_ptr = static_cast<internal_node*>(
        alloc.allocate_bytes(sizeof(internal_node), alignof(internal_node))
    );
    alloc.construct(internal_node_ptr, parent);
    node_ptr = static_cast<leaf_node*>(internal_node_ptr);
  }
  return node_owner(node_ptr, node_deleter{alloc});
}

template <typename Params>
typename pmr::btree_internal_node<Params>::node_owner make_root_node(
    bool is_leaf, typename pmr::btree_internal_node<Params>::allocator_type& alloc
) {
  return make_node<Params>(is_leaf, nullptr, alloc);
}

template <class Params>
typename pmr::btree_internal_node<Params>::node_borrower borrow_child(
    const pmr::btree_leaf_node<Params>& n, typename pmr::btree_internal_node<Params>::count_type i
) noexcept {
  assert(not n.is_leaf());
  const auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
  return in.borrow_child(i);
}

template <class Params>
typename pmr::btree_internal_node<Params>::node_readonly_borrower borrow_readonly_child(
    const pmr::btree_leaf_node<Params>& n, typename pmr::btree_internal_node<Params>::count_type i
) noexcept {
  assert(not n.is_leaf());
  const auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
  return in.borrow_readonly_child(i);
}

template <class Params>
typename pmr::btree_internal_node<Params>::node_owner extract_child(
    pmr::btree_leaf_node<Params>& n, typename pmr::btree_internal_node<Params>::count_type i
) noexcept {
  assert(not n.is_leaf());
  auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
  return in.extract_child(i);
}

template <class Params>
void set_child(
    typename pmr::btree_leaf_node<Params>&                  n,
    typename pmr::btree_internal_node<Params>::count_type   i,
    typename pmr::btree_internal_node<Params>::node_owner&& new_child
) noexcept {
  assert(not n.is_leaf());
  auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
  in.set_child(i, std::move(new_child));
}

template <class Params>
void rebalance_right_to_left(
    typename pmr::btree_leaf_node<Params>&                   n,
    typename pmr::btree_internal_node<Params>::node_borrower sibling,
    typename pmr::btree_internal_node<Params>::count_type    to_move
) {
  if(n.is_leaf()) {
    n.rebalance_right_to_left(sibling, to_move);
  } else {
    auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
    in.rebalance_right_to_left(sibling, to_move);
  }
}

template <class Params>
void rebalance_left_to_right(
    typename pmr::btree_leaf_node<Params>&                   n,
    typename pmr::btree_internal_node<Params>::node_borrower sibling,
    typename pmr::btree_internal_node<Params>::count_type    to_move
) {
  if(n.is_leaf()) {
    n.rebalance_left_to_right(sibling, to_move);
  } else {
    auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
    in.rebalance_left_to_right(sibling, to_move);
  }
}

template <class Params>
void merge(
    typename pmr::btree_leaf_node<Params>&                   n,
    typename pmr::btree_internal_node<Params>::node_borrower sibling
) {
  if(n.is_leaf()) {
    n.merge(sibling);
  } else {
    auto& in = static_cast<pmr::btree_internal_node<Params>>(n);
    in.merge(sibling);
  }
}

// Splits a node, moving a portion of the node's values to its right sibling.
template <class Params>
void split(pmr::btree_leaf_node<Params>& left, typename pmr::btree_internal_node<Params>::node_owner&& right, typename pmr::btree_internal_node<Params>::count_type insert_position) {
  using internal_node = pmr::btree_internal_node<Params>;
  using count_type = typename internal_node::count_type;

  assert(right->count() == 0);
  assert(borrow_readonly_parent() != nullptr);

  // We bias the split based on the position being inserted. If we're
  // inserting at the beginning of the left node then bias the split to put
  // more values on the right node. If we're inserting at the end of the
  // right node then bias the split to put more values on the left node.
  count_type to_move = 0;
  if (insert_position == 0) {
    to_move = left.count() - 1;
  } else if (insert_position != left.max_children_count() - 1) {
    to_move = left.count() / 2;
  }
  assert(count() - to_move >= 1);

  std::move(left.end_values() - to_move, left.end_values(), right->begin_values());
  left.set_count(left.count() - to_move);
  right->set_count(to_move);

  // The split key is the largest value in the left sibling.
  left.set_count(left.count() - 1);
  assert(not left.borrow_parent()->is_leaf());
  auto& parent_node = static_cast<internal_node&>(*(left.borrow_parent()));
  parent_node.insert_value(left.position(), left.extract_value(left.values_count()));
  // Insert dest as a child of parent.
  parent_node.shift_children_right(left.position() + 1, left.borrow_parent()->children_count() - 1, 1);
  parent_node.set_child(left.position() + 1, std::move(right));

  if (!n.is_leaf()) {
    auto right = parent_node.borrow_child(left.position() + 1);
    for (count_type i = 0; i <= right->count(); ++i) {
      assert(left.borrow_child(left.children_count() + i) != nullptr);
    }
    right->receive_children_n(right->begin_children(), &left, left.end_children(), right->count() + 1);
  }
}
}  // namespace platanus

#endif  // PLATANUS_BTREE_NODE_H_
