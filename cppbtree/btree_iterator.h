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

#ifndef CPPBTREE_BTREE_ITERATOR_H_
#define CPPBTREE_BTREE_ITERATOR_H_

namespace cppbtree {

template <typename Node, typename Reference, typename Pointer>
struct btree_iterator {
  using key_type        = typename Node::key_type;
  using size_type       = typename Node::size_type;
  using difference_type = typename Node::difference_type;
  using params_type     = typename Node::params_type;

  using node_type        = Node;
  using normal_node      = typename std::remove_const<Node>::type;
  using const_node       = const Node;
  using value_type       = typename params_type::value_type;
  using normal_pointer   = typename params_type::pointer;
  using normal_reference = typename params_type::reference;
  using const_pointer    = typename params_type::const_pointer;
  using const_reference  = typename params_type::const_reference;

  using pointer           = Pointer;
  using reference         = Reference;
  using iterator_category = std::bidirectional_iterator_tag;

  using iterator       = btree_iterator<normal_node, normal_reference, normal_pointer>;
  using const_iterator = btree_iterator<const_node, const_reference, const_pointer>;
  using self_type      = btree_iterator<Node, Reference, Pointer>;

  btree_iterator() noexcept : node(nullptr), position(-1) {}
  btree_iterator(Node* n, int p) noexcept : node(n), position(p) {}
  btree_iterator(const iterator& x) noexcept : node(x.node), position(x.position) {}

  // Increment/decrement the iterator.
  void increment() {
    if (node->leaf() && ++position < node->count()) {
      return;
    }
    increment_slow();
  }
  void increment_by(int count);
  void increment_slow();

  void decrement() {
    if (node->leaf() && --position >= 0) {
      return;
    }
    decrement_slow();
  }
  void decrement_slow();

  bool operator==(const const_iterator& x) const {
    return node == x.node && position == x.position;
  }
  bool operator!=(const const_iterator& x) const {
    return node != x.node || position != x.position;
  }

  // Accessors for the key/value the iterator is pointing at.
  const key_type& key() const { return node->key(position); }
  reference       operator*() const { return node->value(position); }
  pointer         operator->() const { return &node->value(position); }

  self_type& operator++() {
    increment();
    return *this;
  }
  self_type& operator--() {
    decrement();
    return *this;
  }
  self_type operator++(int) {
    self_type tmp = *this;
    ++*this;
    return tmp;
  }
  self_type operator--(int) {
    self_type tmp = *this;
    --*this;
    return tmp;
  }

  // The node in the tree the iterator is pointing at.
  Node* node;
  // The position within the node of the tree the iterator is pointing at.
  int position;
};

////
// btree_iterator methods
template <typename N, typename R, typename P>
void btree_iterator<N, R, P>::increment_slow() {
  if (node->leaf()) {
    assert(position >= node->count());
    self_type save(*this);
    // Climb the tree.
    while (position == node->count() && !node->is_root()) {
      assert(node->parent()->child(node->position()) == node);
      position = node->position();
      node     = node->parent();
    }
    // If node is the root, this tree is fully iterated, so set this to the saved end() position.
    if (position == node->count()) {
      *this = save;
    }
  } else {
    assert(position < node->count());
    node = node->child(position + 1);
    // Descend to the leaf node.
    while (!node->leaf()) {
      node = node->child(0);
    }
    position = 0;
  }
}

template <typename N, typename R, typename P>
void btree_iterator<N, R, P>::increment_by(int count) {
  while (count > 0) {
    if (node->leaf()) {
      int rest = node->count() - position;
      position += std::min(rest, count);
      count = count - rest;
      if (position < node->count()) {
        // In this case, count is less than rest, so we are done.
        return;
      }
    } else {
      --count;
    }
    increment_slow();
  }
}

template <typename N, typename R, typename P>
void btree_iterator<N, R, P>::decrement_slow() {
  if (node->leaf()) {
    assert(position <= -1);
    self_type save(*this);
    // Climb the tree while updating the position to the left sibling of this in its parent node.
    while (position < 0 && !node->is_root()) {
      assert(node->parent()->child(node->position()) == node);
      position = node->position() - 1;
      node     = node->parent();
    }
    // If node is the root, this is the rend() position, so set this to the saved rend() position.
    if (position < 0) {
      *this = save;
    }
  } else {
    assert(position >= 0);
    node = node->child(position);
    // Descend to the leaf node.
    while (!node->leaf()) {
      node = node->child(node->count());
    }
    position = node->count() - 1;
  }
}

} // namespace cppbtree

#endif // CPPBTREE_BTREE_ITERATOR_H_