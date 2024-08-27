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

#ifndef PLATANUS_BTREE_ITERATOR_H_
#define PLATANUS_BTREE_ITERATOR_H_

namespace platanus {
namespace detail {
template <class Node>
using node_borrower = typename Node::node_borrower;

template <class Node>
using node_readonly_borrower = typename Node::node_readonly_borrower;

template <class Node>
using signed_count_type = typename Node::signed_count_type;

// Increment/decrement the iterator.
template <class Node>
void increment_slow(
    node_readonly_borrower<Node>& node, signed_count_type<Node>& position
) noexcept {
  if (node->leaf()) {
    assert(position >= node->count());

    node_readonly_borrower<Node> save_node     = node;
    signed_count_type<Node>      save_position = position;

    // Climb the tree.
    while (position == node->count() && !node->is_root()) {
      assert(node->borrow_readonly_parent()->borrow_readonly_child(node->position()) == node);

      position = node->position();
      node     = node->borrow_readonly_parent();
    }
    // If node is the root, this tree is fully iterated, so set this to the saved end() position.
    if (position == node->count()) {
      node     = save_node;
      position = save_position;
    }
  } else {
    assert(position < node->count());

    node = node->borrow_readonly_child(position + 1);
    // Descend to the leaf node.
    while (!node->leaf()) {
      node = node->borrow_readonly_child(0);
    }
    position = 0;
  }
}

template <class Node>
void increment(node_readonly_borrower<Node>& node, signed_count_type<Node>& position) noexcept {
  if (node->leaf() && ++position < node->count()) {
    return;
  }
  increment_slow<Node>(node, position);
}

template <class Node>
void increment(node_borrower<Node>& node, signed_count_type<Node>& position) noexcept {
  node_readonly_borrower<Node> tmp = static_cast<node_readonly_borrower<Node>>(node);
  increment<Node>(tmp, position);
  node = const_cast<node_borrower<Node>>(tmp);
}

template <class Node>
void decrement_slow(
    node_readonly_borrower<Node>& node, signed_count_type<Node>& position
) noexcept {
  if (node->leaf()) {
    assert(position <= -1);

    node_readonly_borrower<Node> save_node     = node;
    signed_count_type<Node>      save_position = position;

    // Climb the tree while updating the position to the left sibling of this in its parent node.
    while (position < 0 && !node->is_root()) {
      assert(node->borrow_readonly_parent()->borrow_readonly_child(node->position()) == node);

      position = node->position() - 1;
      node     = node->borrow_readonly_parent();
    }
    // If node is the root, the previoud *this is the rend() position, so set *this to the saved
    // rend() position.
    if (position < 0) {
      node     = save_node;
      position = save_position;
    }
  } else {
    assert(position >= 0);

    node = node->borrow_readonly_child(position);
    // Descend to the leaf node.
    while (!node->leaf()) {
      node = node->borrow_readonly_child(node->count());
    }
    position = node->count() - 1;
  }
}

template <class Node>
void decrement(node_readonly_borrower<Node>& node, signed_count_type<Node>& position) noexcept {
  if (node->leaf() && --position >= 0) {
    return;
  }
  decrement_slow<Node>(node, position);
}

template <class Node>
void decrement(node_borrower<Node>& node, signed_count_type<Node>& position) noexcept {
  node_readonly_borrower<Node> tmp = static_cast<node_readonly_borrower<Node>>(node);
  decrement<Node>(tmp, position);
  node = const_cast<node_borrower<Node>>(tmp);
}
}  // namespace detail

template <typename Node, bool is_const>
struct btree_iterator;

template <typename Node>
struct btree_iterator<Node, false> {
  using self_type = btree_iterator<Node, false>;

  using node_type              = Node;
  using node_borrower          = typename Node::node_borrower;
  using node_readonly_borrower = typename Node::node_readonly_borrower;

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
  const key_type& key() const { return node->key(position); }

  reference operator*() const { return node->value(position); }
  pointer   operator->() const { return &node->value(position); }

  self_type& operator++() noexcept {
    detail::increment<self_type>(node, position);
    return *this;
  }

  self_type& operator--() noexcept {
    detail::decrement<self_type>(node, position);
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
  using node_borrower          = typename Node::node_borrower;
  using node_readonly_borrower = typename Node::node_readonly_borrower;

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
  const key_type& key() const { return node->key(position); }

  const_reference operator*() const { return node->value(position); }
  const_pointer   operator->() const { return &node->value(position); }

  self_type& operator++() noexcept {
    detail::increment<self_type>(node, position);
    return *this;
  }

  self_type& operator--() noexcept {
    detail::decrement<self_type>(node, position);
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

}  // namespace platanus

#endif  // PLATANUS_BTREE_ITERATOR_H_