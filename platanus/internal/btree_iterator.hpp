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

#ifndef PLATANUS_INTERNAL_BTREE_ITERATOR_H_
#define PLATANUS_INTERNAL_BTREE_ITERATOR_H_

#include <iterator>

#include "btree_node_fwd.hpp"

namespace platanus::internal {
namespace btree_iterator_impl {
template <class Node>
using signed_count_type = typename Node::signed_count_type;

// Increment/decrement the iterator.
template <class Node>
void increment_slow(
    btree_node_readonly_borrower<Node>& node, signed_count_type<Node>& pos
) noexcept {
  if (is_leaf(node)) {
    assert(pos >= count(node));

    auto save_node = node;
    auto save_pos  = pos;

    // Climb the tree.
    while (pos == count(node) && not is_root(node)) {
      assert(borrow_readonly_child(borrow_readonly_parent(node), position(node)) == node);

      pos  = position(node);
      node = borrow_readonly_parent(node);
    }
    // If node is the root, this tree is fully iterated, so set this to the saved end() pos.
    if (pos == count(node)) {
      node = save_node;
      pos  = save_pos;
    }
  } else {
    assert(pos < count(node));

    node = borrow_readonly_child(node, pos + 1);
    // Descend to the leaf node.
    while (not is_leaf(node)) {
      node = borrow_readonly_child(node, 0);
    }
    pos = 0;
  }
}

template <class Node>
void increment(
    btree_node_readonly_borrower<Node>& node, signed_count_type<Node>& position
) noexcept {
  if (is_leaf(node) && ++position < count(node)) {
    return;
  }
  increment_slow(node, position);
}

template <class Node>
void increment(btree_node_borrower<Node>& node, signed_count_type<Node>& position) noexcept {
  auto tmp = static_cast<btree_node_readonly_borrower<Node>>(node);
  increment(tmp, position);
  node = const_cast<btree_node_borrower<Node>>(tmp);
}

template <class Node>
void decrement_slow(
    btree_node_readonly_borrower<Node>& node, signed_count_type<Node>& pos
) noexcept {
  if (is_leaf(node)) {
    assert(pos <= -1);

    auto save_node = node;
    auto save_pos  = pos;

    // Climb the tree while updating the pos to the left sibling of this in its parent node.
    while (pos < 0 && not is_root(node)) {
      assert(borrow_readonly_child(borrow_readonly_parent(node), position(node)) == node);

      pos  = position(node) - 1;
      node = borrow_readonly_parent(node);
    }
    // If node is the root, the previoud *this is the rend() pos, so set *this to the saved
    // rend() pos.
    if (pos < 0) {
      node = save_node;
      pos  = save_pos;
    }
  } else {
    assert(pos >= 0);

    node = borrow_readonly_child(node, pos);
    // Descend to the leaf node.
    while (not is_leaf(node)) {
      node = borrow_readonly_child(node, count(node));
    }
    pos = count(node) - 1;
  }
}

template <class Node>
void decrement(
    btree_node_readonly_borrower<Node>& node, signed_count_type<Node>& position
) noexcept {
  if (is_leaf(node) && --position >= 0) {
    return;
  }
  decrement_slow(node, position);
}

template <class Node>
void decrement(btree_node_borrower<Node>& node, signed_count_type<Node>& position) noexcept {
  auto tmp = static_cast<btree_node_readonly_borrower<Node>>(node);
  decrement(tmp, position);
  node = const_cast<btree_node_borrower<Node>>(tmp);
}
}  // namespace btree_iterator_impl

template <typename Node, bool is_const>
struct btree_iterator;

template <typename Node>
struct btree_iterator<Node, false> {
  using self_type = btree_iterator<Node, false>;

  using node_type              = Node;
  using node_borrower          = btree_node_borrower<Node>;
  using node_readonly_borrower = btree_node_readonly_borrower<Node>;

  using key_type        = typename Node::key_type;
  using value_type      = typename Node::value_type;
  using reference       = typename Node::reference;
  using const_reference = typename Node::const_reference;
  using pointer         = typename Node::pointer;
  using const_pointer   = typename Node::const_pointer;

  using size_type         = typename Node::size_type;
  using difference_type   = typename Node::difference_type;
  using signed_count_type = typename Node::signed_count_type;

  using iterator_type     = btree_iterator<Node, false>;
  using iterator_category = std::bidirectional_iterator_tag;

  btree_iterator() noexcept : node(nullptr), position(-1) {}

  btree_iterator(const btree_iterator&)            = default;
  btree_iterator(btree_iterator&&)                 = default;
  btree_iterator& operator=(const btree_iterator&) = default;
  btree_iterator& operator=(btree_iterator&&)      = default;
  ~btree_iterator()                                = default;

  explicit btree_iterator(node_borrower n, signed_count_type p) noexcept : node(n), position(p) {}

  // Accessors for the key/value the iterator is pointing at.
  const key_type& key() const {
    using internal::key;
    return key(node, position);
  }

  reference operator*() const {
    using internal::value;
    return value(node, position);
  }
  pointer operator->() const { return &(this->operator*()); }

  self_type& operator++() noexcept {
    btree_iterator_impl::increment(node, position);
    return *this;
  }

  self_type& operator--() noexcept {
    btree_iterator_impl::decrement(node, position);
    return *this;
  }

  self_type operator++(int) noexcept {
    self_type tmp = *this;
    ++*this;
    return tmp;
  }

  self_type operator--(int) noexcept {
    self_type tmp = *this;
    --*this;
    return tmp;
  }

  // The node in the tree the iterator is pointing at.
  node_borrower node;
  // The position within the node of the tree the iterator is pointing at.
  signed_count_type position;
};

template <typename Node>
struct btree_iterator<Node, true> {
  using self_type = btree_iterator<Node, true>;

  using node_type              = Node;
  using node_borrower          = btree_node_borrower<Node>;
  using node_readonly_borrower = btree_node_readonly_borrower<Node>;

  using key_type        = typename Node::key_type;
  using value_type      = typename Node::value_type;
  using reference       = typename Node::reference;
  using const_reference = typename Node::const_reference;
  using pointer         = typename Node::pointer;
  using const_pointer   = typename Node::const_pointer;

  using size_type         = typename Node::size_type;
  using difference_type   = typename Node::difference_type;
  using signed_count_type = typename Node::signed_count_type;

  using iterator_type     = btree_iterator<Node, false>;
  using iterator_category = std::bidirectional_iterator_tag;

  btree_iterator() noexcept : node(nullptr), position(-1) {}

  btree_iterator(const btree_iterator&)            = default;
  btree_iterator(btree_iterator&&)                 = default;
  btree_iterator& operator=(const btree_iterator&) = default;
  btree_iterator& operator=(btree_iterator&&)      = default;
  ~btree_iterator()                                = default;

  explicit btree_iterator(node_readonly_borrower n, signed_count_type p) noexcept
      : node(n), position(p) {}

  btree_iterator(const btree_iterator<Node, false>& x) noexcept
      : btree_iterator(static_cast<node_readonly_borrower>(x.node), x.position) {}

  // Accessors for the key/value the iterator is pointing at.
  const key_type& key() const {
    using internal::key;
    return key(node, position);
  }

  const_reference operator*() const {
    using internal::value;
    return value(node, position);
  }
  const_pointer operator->() const { return &(this->operator*()); }

  self_type& operator++() noexcept {
    btree_iterator_impl::increment(node, position);
    return *this;
  }

  self_type& operator--() noexcept {
    btree_iterator_impl::decrement(node, position);
    return *this;
  }

  self_type operator++(int) noexcept {
    self_type tmp = *this;
    ++*this;
    return tmp;
  }

  self_type operator--(int) noexcept {
    self_type tmp = *this;
    --*this;
    return tmp;
  }

  // The node in the tree the iterator is pointing at.
  node_readonly_borrower node;
  // The position within the node of the tree the iterator is pointing at.
  signed_count_type position;
};

template <typename Node, bool lb, bool rb>
bool operator==(const btree_iterator<Node, lb>& lhd, const btree_iterator<Node, rb>& rhd) noexcept {
  return lhd.node == rhd.node && lhd.position == rhd.position;
}

template <typename Node, bool lb, bool rb>
bool operator!=(const btree_iterator<Node, lb>& lhd, const btree_iterator<Node, rb>& rhd) noexcept {
  return !(lhd == rhd);
}
}  // namespace platanus::internal

#endif  // PLATANUS_INTERNAL_BTREE_ITERATOR_H_
