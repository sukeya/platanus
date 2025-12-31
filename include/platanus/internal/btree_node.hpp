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

#ifndef PLATANUS_INTERNAL_BTREE_NODE_H_
#define PLATANUS_INTERNAL_BTREE_NODE_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <memory>

#include "btree_node_fwd.hpp"
#include "btree_base_node.hpp"
#include "btree_util.hpp"

namespace platanus::internal {
// A node in the btree holding. The same node type is used for both internal
// and leaf nodes in the btree, though the nodes are allocated in such a way
// that the children array is only valid in internal nodes.
template <typename Params>
class btree_node : public btree_base_node<Params, btree_node<Params>> {
 public:
  using params_type = Params;
  using self_type   = btree_node<params_type>;
  using super_type  = btree_base_node<Params, self_type>;

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

  using search_result = typename super_type::search_result;
  using count_type    = typename search_result::count_type;

  using values_type                   = typename super_type::values_type;
  using values_iterator               = typename values_type::iterator;
  using values_const_iterator         = typename values_type::const_iterator;
  using values_reverse_iterator       = typename values_type::reverse_iterator;
  using values_const_reverse_iterator = typename values_type::const_reverse_iterator;

  using node_borrower          = typename super_type::node_borrower;
  using node_readonly_borrower = typename super_type::node_readonly_borrower;

  using allocator_type   = typename Params::allocator_type;
  using allocator_traits = std::allocator_traits<allocator_type>;

  using node_allocator_type   = typename allocator_traits::template rebind_alloc<btree_node>;
  using node_allocator_traits = std::allocator_traits<node_allocator_type>;

  class node_deleter : private node_allocator_type {
   public:
    using pointer = typename node_allocator_traits::pointer;

    node_deleter()                               = default;
    node_deleter(const node_deleter&)            = default;
    node_deleter(node_deleter&&)                 = default;
    node_deleter& operator=(const node_deleter&) = default;
    node_deleter& operator=(node_deleter&&)      = default;
    ~node_deleter()                              = default;

    explicit node_deleter(const node_allocator_type& alloc) : node_allocator_type(alloc) {}

    void operator()(pointer p) {
      if (p != nullptr) {
        node_allocator_traits::destroy(*this, p);
        node_allocator_traits::deallocate(*this, p, 1);
      }
    }

    void swap(node_deleter& x) { btree_swap_helper(*this, x); }
  };

  // node_owner owes to allocate and release the memory of the node.
  using node_owner = std::unique_ptr<btree_node, node_deleter>;

  using children_allocator_type   = typename allocator_traits::template rebind_alloc<node_owner>;
  using children_allocator_traits = std::allocator_traits<children_allocator_type>;

  class children_deleter : private children_allocator_type {
   public:
    using pointer = typename children_allocator_traits::pointer;

    children_deleter()                                   = default;
    children_deleter(const children_deleter&)            = default;
    children_deleter(children_deleter&&)                 = default;
    children_deleter& operator=(const children_deleter&) = default;
    children_deleter& operator=(children_deleter&&)      = default;
    ~children_deleter()                                  = default;

    explicit children_deleter(const children_allocator_type& alloc)
        : children_allocator_type(alloc) {}

    void operator()(pointer p) {
      if (p != nullptr) {
        // FIXME some children whose index is over children_count() haven't been freed.
        for (count_type i = 0; i < kNodeChildren; ++i) {
          if (p[i]) {
            children_allocator_traits::destroy(*this, &p[i]);
          }
        }
        children_allocator_traits::deallocate(*this, p, kNodeChildren);
      }
    }

    void swap(children_deleter& x) { btree_swap_helper(*this, x); }
  };

  using children_ptr                    = std::unique_ptr<node_owner[], children_deleter>;
  using children_iterator               = typename children_ptr::pointer;
  using children_const_iterator         = const children_iterator;
  using children_reverse_iterator       = std::reverse_iterator<children_iterator>;
  using children_const_reverse_iterator = std::reverse_iterator<children_const_iterator>;

  static node_owner make_node(
      bool                     is_leaf,
      node_borrower            parent,
      node_allocator_type&     node_alloc,
      children_allocator_type& children_alloc
  ) {
    auto node_ptr = node_allocator_traits::allocate(node_alloc, 1);
    node_allocator_traits::construct(node_alloc, node_ptr, is_leaf, parent, children_alloc);
    return node_owner(node_ptr, node_deleter{node_alloc});
  }

  static node_owner make_root_node(
      bool is_leaf, node_allocator_type& node_alloc, children_allocator_type& children_alloc
  ) {
    return make_node(is_leaf, nullptr, node_alloc, children_alloc);
  }

 public:
  btree_node()                        = default;
  btree_node(btree_node&&)            = default;
  btree_node& operator=(btree_node&&) = default;
  ~btree_node()                       = default;

  btree_node(const btree_node&)            = delete;
  btree_node& operator=(const btree_node&) = delete;

  explicit btree_node(bool is_leaf, node_borrower parent, children_allocator_type& children_alloc)
      : super_type(parent), children_ptr_() {
    if (not is_leaf) {
      auto p = children_allocator_traits::allocate(children_alloc, kNodeChildren);
      for (count_type i = 0; i < kNodeChildren; ++i) {
        children_allocator_traits::construct(children_alloc, &p[i], nullptr);
      }
      children_ptr_ = children_ptr{p, children_deleter{children_alloc}};
    }
  }

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

  // If *this is a leaf node, return true: otherwise, false. This value doesn't
  // change after the node is created.
  bool is_leaf() const noexcept { return children_ptr_ ? false : true; }

  // Getters/setter for the child at position i in the node.
  node_borrower borrow_child(count_type i) const noexcept { return children_ptr_[i].get(); }
  node_readonly_borrower borrow_readonly_child(count_type i) const noexcept {
    return children_ptr_[i].get();
  }
  node_owner extract_child(count_type i) noexcept { return std::move(children_ptr_[i]); }
  void       set_child(count_type i, node_owner&& new_child) noexcept {
    children_ptr_[i]              = std::move(new_child);
    auto borrowed_new_child       = borrow_child(i);
    borrowed_new_child->parent_   = this;
    borrowed_new_child->position_ = i;
  }

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(node_borrower sibling, count_type to_move);
  void rebalance_left_to_right(node_borrower sibling, count_type to_move);

  // Splits a node, moving a portion of the node's values to its right sibling.
  void split(node_owner&& sibling, count_type insert_position);

  // Merges a node with its right sibling, moving all of the values and the
  // delimiting key in the parent node onto itself.
  void merge(node_borrower sibling);

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
  children_iterator begin_children() noexcept { return children_ptr_.get(); }

  // Returns the pointer to the back of the children array.
  children_iterator end_children() noexcept(
      noexcept(std::next(std::declval<children_iterator>(), std::declval<int>()))
  ) {
    return std::next(begin_children(), count() + 1);
  }

  children_reverse_iterator rbegin_children() noexcept(
      noexcept(std::reverse_iterator(end_children()))
  ) {
    return std::reverse_iterator(end_children());
  }
  children_reverse_iterator rend_children() noexcept(
      noexcept(std::reverse_iterator(begin_children()))
  ) {
    return std::reverse_iterator(begin_children());
  }

  count_type children_count() const noexcept { return is_leaf() ? 0 : count() + 1; }

  count_type max_children_count() const noexcept { return is_leaf() ? 0 : max_count() + 1; }

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

 private:
  // The pointer to the array of child pointers. The keys in children_[i] are all less than
  // key(i). The keys in children_[i + 1] are all greater than key(i). There
  // are always count + 1 children.
  children_ptr children_ptr_;
};

template <typename P>
void btree_node<P>::rebalance_right_to_left(node_borrower right, count_type to_move) {
  if (!is_leaf()) {
    // Move the child pointers from the right to the left node.
    receive_children_n(end_children(), right, right->begin_children(), to_move);
    right->shift_children_left(to_move, right->children_count(), to_move);
  }

  super_type::rebalance_right_to_left(right, to_move);
}

template <typename P>
void btree_node<P>::rebalance_left_to_right(node_borrower right, count_type to_move) {
  if (!is_leaf()) {
    // Move the child pointers from the left to the right node.
    right->shift_children_right(0, right->children_count(), to_move);
    right->receive_children_n(
        right->begin_children(),
        this,
        begin_children() + children_count() - to_move,
        to_move
    );
  }

  super_type::rebalance_left_to_right(right, to_move);
}

template <typename P>
void btree_node<P>::split(node_owner&& dest, count_type insert_position) {
  assert(dest->count() == 0);
  assert(borrow_readonly_parent() != nullptr);

  // We bias the split based on the position being inserted. If we're
  // inserting at the beginning of the left node then bias the split to put
  // more values on the right node. If we're inserting at the end of the
  // right node then bias the split to put more values on the left node.
  count_type to_move = 0;
  if (insert_position == 0) {
    to_move = count() - 1;
  } else if (insert_position != max_children_count() - 1) {
    to_move = count() / 2;
  }
  assert(count() - to_move >= 1);

  std::move(end_values() - to_move, end_values(), dest->begin_values());
  set_count(count() - to_move);
  dest->set_count(to_move);

  // The split key is the largest value in the left sibling.
  set_count(count() - 1);
  borrow_parent()->insert_value(position(), this->extract_value(values_count()));
  // Insert dest as a child of parent.
  borrow_parent()->shift_children_right(position() + 1, borrow_parent()->children_count() - 1, 1);
  borrow_parent()->set_child(position() + 1, std::move(dest));

  if (!is_leaf()) {
    auto dest = borrow_parent()->borrow_child(position() + 1);
    for (count_type i = 0; i <= dest->count(); ++i) {
      assert(borrow_child(children_count() + i) != nullptr);
    }
    dest->receive_children_n(dest->begin_children(), this, end_children(), dest->count() + 1);
  }
}

template <typename P>
void btree_node<P>::merge(node_borrower src) {
  assert(borrow_readonly_parent() == src->borrow_readonly_parent());
  assert(borrow_readonly_parent() != nullptr);
  assert(position() + 1 == src->position());

  // Move the delimiting value to the left node.
  this->replace_value(values_count(), borrow_parent()->extract_value(position()));

  // Move the values from the right to the left node.
  std::move(src->begin_values(), src->end_values(), end_values() + 1);

  if (!is_leaf()) {
    // Move the child pointers from the right to the left node.
    receive_children(end_children(), src, src->begin_children(), src->end_children());
  }

  // Fixup the counts on the src and dest nodes.
  set_count(1 + count() + src->count());
  src->set_count(0);

  // Shift children behind the removed child left.
  if (position() + 2 < borrow_parent()->children_count()) {
    borrow_parent()->shift_children_left(position() + 2, borrow_parent()->children_count(), 1);
  }

  // Remove the value on the parent node.
  borrow_parent()->remove_value(position());
}

template <class Params>
struct btree_node_owner_type<btree_node<Params>> {
  using type = typename btree_node<Params>::node_owner;
};

template <class Params>
struct sizeof_leaf_node<btree_node<Params>> {
  static constexpr std::size_t value = sizeof(btree_node<Params>);
};

template <class Params>
struct sizeof_internal_node<btree_node<Params>> {
  static constexpr std::size_t value =
      sizeof(btree_node<Params>)
      + sizeof(btree_node_owner<btree_node<Params>>) * btree_node<Params>::kNodeChildren;
};

template <class Params>
bool is_leaf(btree_node_readonly_borrower<btree_node<Params>> n) noexcept {
  assert(n != nullptr);
  return n->is_leaf();
}

template <class Params>
btree_node_borrower<btree_node<Params>> borrow_child(
    btree_node_readonly_borrower<btree_node<Params>> n, typename btree_node<Params>::count_type i
) noexcept {
  assert(n != nullptr);
  return n->borrow_child(i);
}

template <class Params>
btree_node_readonly_borrower<btree_node<Params>> borrow_readonly_child(
    btree_node_readonly_borrower<btree_node<Params>> n, typename btree_node<Params>::count_type i
) noexcept {
  assert(n != nullptr);
  return n->borrow_readonly_child(i);
}

template <class Params>
btree_node_owner<btree_node<Params>> extract_child(
    btree_node_borrower<btree_node<Params>> n, typename btree_node<Params>::count_type i
) noexcept {
  assert(n != nullptr);
  return n->extract_child(i);
}

template <class Params>
void set_child(
    btree_node_borrower<btree_node<Params>> n,
    typename btree_node<Params>::count_type i,
    btree_node_owner<btree_node<Params>>&&  new_child
) noexcept {
  assert(n != nullptr);
  n->set_child(i, std::move(new_child));
}

template <class Params>
void rebalance_right_to_left(
    btree_node_borrower<btree_node<Params>> n,
    btree_node_borrower<btree_node<Params>> sibling,
    typename btree_node<Params>::count_type to_move
) {
  assert(n != nullptr);
  n->rebalance_right_to_left(sibling, to_move);
}

template <class Params>
void rebalance_left_to_right(
    btree_node_borrower<btree_node<Params>> n,
    btree_node_borrower<btree_node<Params>> sibling,
    typename btree_node<Params>::count_type to_move
) {
  assert(n != nullptr);
  n->rebalance_left_to_right(sibling, to_move);
}

template <class Params>
void split(
    btree_node_borrower<btree_node<Params>> n,
    btree_node_owner<btree_node<Params>>&&  sibling,
    typename btree_node<Params>::count_type insert_position
) {
  assert(n != nullptr);
  n->split(std::move(sibling), insert_position);
}

template <class Params>
void merge(
    btree_node_borrower<btree_node<Params>> n, btree_node_borrower<btree_node<Params>> sibling
) {
  assert(n != nullptr);
  n->merge(sibling);
}
}  // namespace platanus::internal

namespace std {
template <class Params, class Alloc>
struct uses_allocator<platanus::internal::btree_node<Params>, Alloc> : public std::false_type {};
}  // namespace std

namespace platanus::internal {
template <class Params>
class btree_node_factory {
 public:
  using node_type               = btree_node<Params>;
  using allocator_type          = typename node_type::allocator_type;
  using node_allocator_type     = typename node_type::node_allocator_type;
  using children_allocator_type = typename node_type::children_allocator_type;

  btree_node_factory()                                     = default;
  btree_node_factory(btree_node_factory&&)                 = default;
  btree_node_factory& operator=(const btree_node_factory&) = default;
  btree_node_factory& operator=(btree_node_factory&&)      = default;
  ~btree_node_factory()                                    = default;

  btree_node_factory(const btree_node_factory& x)
      : node_alloc_(
            std::allocator_traits<node_allocator_type>::select_on_container_copy_construction(
                x.node_alloc_
            )
        ),
        children_alloc_(
            std::allocator_traits<children_allocator_type>::select_on_container_copy_construction(
                x.children_alloc_
            )
        ) {}

  explicit btree_node_factory(const allocator_type& alloc)
      : node_alloc_(alloc), children_alloc_(alloc) {}

  btree_node_owner<node_type> make_node(bool is_leaf, btree_node_borrower<node_type> parent) {
    return node_type::make_node(is_leaf, parent, node_alloc_, children_alloc_);
  }

  btree_node_owner<node_type> make_root_node(bool is_leaf) {
    return node_type::make_root_node(is_leaf, node_alloc_, children_alloc_);
  }

  node_allocator_type get_node_allocator() const { return node_alloc_; }

 private:
  node_allocator_type     node_alloc_;
  children_allocator_type children_alloc_;
};

template <class Params>
struct btree_node_and_factory {
  using node_type    = btree_node<Params>;
  using node_factory = btree_node_factory<Params>;
};
}  // namespace platanus::internal

#endif  // PLATANUS_BTREE_INTERNAL_NODE_H_
