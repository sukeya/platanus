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
template <typename Node, bool is_const>
struct btree_iterator;

template <typename Node>
struct btree_iterator_base {
  using self_type = btree_iterator_base;

  using node_type     = Node;
  using node_borrower = typename Node::node_borrower;

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

  btree_iterator_base() noexcept : node(nullptr), position(-1) {}

  btree_iterator_base(const btree_iterator_base&)            = default;
  btree_iterator_base(btree_iterator_base&&)                 = default;
  btree_iterator_base& operator=(const btree_iterator_base&) = default;
  btree_iterator_base& operator=(btree_iterator_base&&)      = default;
  ~btree_iterator_base()                                     = default;

  explicit btree_iterator_base(node_borrower n, signed_count_type p) noexcept
      : node(n), position(p) {}

  // Increment/decrement the iterator.
  void increment() noexcept {
    if (node->leaf() && ++position < node->count()) {
      return;
    }
    increment_slow();
  }

  void increment_slow() noexcept;

  void decrement() noexcept {
    if (node->leaf() && --position >= 0) {
      return;
    }
    decrement_slow();
  }

  void decrement_slow() noexcept;

  // Accessors for the key/value the iterator is pointing at.
  const key_type& key() const { return node->key(position); }

  // The node in the tree the iterator is pointing at.
  node_borrower node;
  // The position within the node of the tree the iterator is pointing at.
  signed_count_type position;
};

////
// btree_iterator methods
template <typename N>
void btree_iterator_base<N>::increment_slow() noexcept {
  if (node->leaf()) {
    assert(position >= node->count());

    self_type save(*this);
    // Climb the tree.
    while (position == node->count() && !node->is_root()) {
      assert(node->borrow_parent()->borrow_child(node->position()) == node);

      position = node->position();
      node     = node->borrow_parent();
    }
    // If node is the root, this tree is fully iterated, so set this to the saved end() position.
    if (position == node->count()) {
      *this = save;
    }
  } else {
    assert(position < node->count());

    node = node->borrow_child(position + 1);
    // Descend to the leaf node.
    while (!node->leaf()) {
      node = node->borrow_child(0);
    }
    position = 0;
  }
}

template <typename N>
void btree_iterator_base<N>::decrement_slow() noexcept {
  if (node->leaf()) {
    assert(position <= -1);

    self_type save(*this);
    // Climb the tree while updating the position to the left sibling of this in its parent node.
    while (position < 0 && !node->is_root()) {
      assert(node->borrow_parent()->borrow_child(node->position()) == node);

      position = node->position() - 1;
      node     = node->borrow_parent();
    }
    // If node is the root, the previoud *this is the rend() position, so set *this to the saved
    // rend() position.
    if (position < 0) {
      *this = save;
    }
  } else {
    assert(position >= 0);

    node = node->borrow_child(position);
    // Descend to the leaf node.
    while (!node->leaf()) {
      node = node->borrow_child(node->count());
    }
    position = node->count() - 1;
  }
}

template <typename Node>
bool operator==(
    const btree_iterator_base<Node>& lhd, const btree_iterator_base<Node>& rhd
) noexcept {
  return lhd.node == rhd.node && lhd.position == rhd.position;
}

template <typename Node>
bool operator!=(
    const btree_iterator_base<Node>& lhd, const btree_iterator_base<Node>& rhd
) noexcept {
  return !(lhd == rhd);
}

template <typename Node>
struct btree_iterator<Node, false> : public btree_iterator_base<Node> {
  using self_type  = btree_iterator<Node, false>;
  using super_type = btree_iterator_base<Node>;

  using node_borrower     = typename super_type::node_borrower;
  using signed_count_type = typename super_type::signed_count_type;
  using reference         = typename super_type::reference;
  using pointer           = typename super_type::pointer;

  btree_iterator()                                 = default;
  btree_iterator(const btree_iterator&)            = default;
  btree_iterator(btree_iterator&&)                 = default;
  btree_iterator& operator=(const btree_iterator&) = default;
  btree_iterator& operator=(btree_iterator&&)      = default;
  ~btree_iterator()                                = default;

  explicit btree_iterator(node_borrower n, signed_count_type p) noexcept : super_type(n, p) {}

  reference operator*() const { return this->node->value(this->position); }
  pointer   operator->() const { return &this->node->value(this->position); }

  self_type& operator++() noexcept {
    this->increment();
    return *this;
  }

  self_type& operator--() noexcept {
    this->decrement();
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
};

template <typename Node>
struct btree_iterator<Node, true> : public btree_iterator_base<Node> {
  using self_type  = btree_iterator<Node, true>;
  using super_type = btree_iterator_base<Node>;

  using node_borrower     = typename super_type::node_borrower;
  using signed_count_type = typename super_type::signed_count_type;
  using const_reference   = typename super_type::const_reference;
  using const_pointer     = typename super_type::const_pointer;

  btree_iterator()                                 = default;
  btree_iterator(const btree_iterator&)            = default;
  btree_iterator(btree_iterator&&)                 = default;
  btree_iterator& operator=(const btree_iterator&) = default;
  btree_iterator& operator=(btree_iterator&&)      = default;
  ~btree_iterator()                                = default;

  explicit btree_iterator(node_borrower n, signed_count_type p) noexcept : super_type(n, p) {}

  explicit btree_iterator(const btree_iterator<Node, false>& x) noexcept
      : super_type(x.node, x.position) {}

  const_reference operator*() const { return this->node->value(this->position); }
  const_pointer   operator->() const { return &this->node->value(this->position); }

  self_type& operator++() noexcept {
    this->increment();
    return *this;
  }

  self_type& operator--() noexcept {
    this->decrement();
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
};

}  // namespace platanus

#endif  // PLATANUS_BTREE_ITERATOR_H_