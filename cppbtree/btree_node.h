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

#ifndef CPPBTREE_BTREE_NODE_H_
#define CPPBTREE_BTREE_NODE_H_

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>

#include "btree_comparer.h"
#include "btree_util.h"

#ifndef NDEBUG
#define NDEBUG 1
#endif

namespace cppbtree {

// A node in the btree holding. The same node type is used for both internal
// and leaf nodes in the btree, though the nodes are allocated in such a way
// that the children array is only valid in internal nodes.
template <typename Params>
class btree_node {
 public:
  using params_type        = Params;
  using self_type          = btree_node<Params>;
  using key_type           = typename Params::key_type;
  // Deprecated: use mapped_type instead.
  using data_type          = typename Params::data_type;
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
  // Typedefs for the various types of node searches.
  using linear_search_plain_compare_type =
      btree_linear_search_plain_compare<key_type, self_type, key_compare>;
  using linear_search_compare_to_type =
      btree_linear_search_compare_to<key_type, self_type, key_compare>;
  using binary_search_plain_compare_type =
      btree_binary_search_plain_compare<key_type, self_type, key_compare>;
  using binary_search_compare_to_type =
      btree_binary_search_compare_to<key_type, self_type, key_compare>;
  // If we have a valid key-compare-to type, use linear_search_compare_to,
  // otherwise use linear_search_plain_compare.
  using linear_search_type = typename std::conditional_t<
      Params::is_key_compare_to::value,
      linear_search_compare_to_type,
      linear_search_plain_compare_type>;
  // If we have a valid key-compare-to type, use binary_search_compare_to,
  // otherwise use binary_search_plain_compare.
  using binary_search_type = typename std::conditional_t<
      Params::is_key_compare_to::value,
      binary_search_compare_to_type,
      binary_search_plain_compare_type>;
  // If the key is an integral or floating point type, use linear search which
  // is faster than binary search for such types. Might be wise to also
  // configure linear search based on node-size.
  using search_type = typename std::conditional_t<
      std::is_integral_v<key_type> || std::is_floating_point_v<key_type>,
      linear_search_type,
      binary_search_type>;

  struct base_fields {
    using field_type = typename Params::node_count_type;

    // A boolean indicating whether the node is a leaf or not.
    bool leaf;
    // The position of the node in the node's parent.
    field_type position;
    // The maximum number of values the node can hold.
    field_type max_count;
    // The count of the number of values in the node.
    field_type count;
    // A pointer to the node's parent.
    btree_node* parent;
  };

  static constexpr int kValueSize      = params_type::kValueSize;
  static constexpr int kTargetNodeSize = params_type::kTargetNodeSize;

  // Compute how many values we can fit onto a leaf node.
  static_assert(kTargetNodeSize >= sizeof(base_fields), "target node size too small.");
  static constexpr int kNodeTargetValues = (kTargetNodeSize - sizeof(base_fields)) / kValueSize;
  // We need a minimum of 3 values per internal node in order to perform
  // splitting (1 value for the two nodes involved in the split and 1 value
  // propagated to the parent as the delimiter for the split).
  static constexpr int kNodeValues = kNodeTargetValues >= 3 ? kNodeTargetValues : 3;

  static_assert(
      std::numeric_limits<int>::digits >= 31, "This program requires int to have 32 bit at least."
  );
  static constexpr int kExactMatch = 1 << 30;
  static constexpr int kMatchMask  = kExactMatch - 1;

  struct leaf_fields : public base_fields {
    using values_type = std::array<mutable_value_type, kNodeValues>;
    // The array of values. Only the first count of these values have been
    // constructed and are valid.
    values_type values;
  };

  using values_iterator = typename leaf_fields::values_type::iterator;
  using values_const_iterator = typename leaf_fields::values_type::const_iterator;
  using values_reverse_iterator = typename leaf_fields::values_type::reverse_iterator;
  using values_const_reverse_iterator = typename leaf_fields::values_type::const_reverse_iterator;

  struct internal_fields : public leaf_fields {
    using children_type = std::array<btree_node*, kNodeValues + 1>;
    // The array of child pointers. The keys in children_[i] are all less than
    // key(i). The keys in children_[i + 1] are all greater than key(i). There
    // are always count + 1 children.
    children_type children;
  };

  using children_iterator = typename internal_fields::children_type::iterator;
  using children_const_iterator = typename internal_fields::children_type::const_iterator;
  using children_reverse_iterator = typename internal_fields::children_type::reverse_iterator;
  using children_const_reverse_iterator = typename internal_fields::children_type::const_reverse_iterator;

  struct root_fields : public internal_fields {
    btree_node* rightmost;
    size_type   size;
  };

 public:
  btree_node()                        = default;
  btree_node(btree_node&&)            = default;
  btree_node& operator=(btree_node&&) = default;
  ~btree_node()                       = default;

  btree_node(const btree_node&)     = delete;
  void operator=(const btree_node&) = delete;

  // Getter/setter for whether this is a leaf node or not. This value doesn't
  // change after the node is created.
  bool leaf() const { return fields_.leaf; }

  // Getter for the position of this node in its parent.
  int  position() const { return fields_.position; }
  void set_position(int v) { fields_.position = v; }

  // Getter/setter for the number of values stored in this node.
  int  count() const { return fields_.count; }
  void set_count(int v) { fields_.count = v; }
  int  max_count() const { return fields_.max_count; }

  // Getter for the parent of this node.
  btree_node* parent() const { return fields_.parent; }
  // Getter for whether the node is the root of the tree. The parent of the
  // root of the tree is the leftmost node in the tree which is guaranteed to
  // be a leaf.
  bool is_root() const { return parent()->leaf(); }
  void make_root() {
    assert(parent()->is_root());
    fields_.parent = fields_.parent->parent();
  }

  // Getter for the rightmost root node field. Only valid on the root node.
  btree_node*  rightmost() const { return fields_.rightmost; }
  btree_node** mutable_rightmost() { return &fields_.rightmost; }

  // Getter for the size root node field. Only valid on the root node.
  size_type  size() const { return fields_.size; }
  size_type* mutable_size() { return &fields_.size; }

  // Getters for the key/value at position i in the node.
  const key_type& key(int i) const { return params_type::key(fields_.values[i]); }
  reference       value(int i) { return reinterpret_cast<reference>(fields_.values[i]); }
  const_reference value(int i) const {
    return reinterpret_cast<const_reference>(fields_.values[i]);
  }
  mutable_value_type* mutable_value(int i) { return &fields_.values[i]; }

  // Swap value i in this node with value j in node x.
  void value_swap(int i, btree_node* x, int j) {
    params_type::swap(mutable_value(i), x->mutable_value(j));
  }

  // Getters/setter for the child at position i in the node.
  btree_node*  child(int i) const { return fields_.children[i]; }
  btree_node** mutable_child(int i) { return &fields_.children[i]; }
  void         set_child(int i, btree_node* c) {
            *mutable_child(i)   = c;
            c->fields_.parent   = this;
            c->fields_.position = i;
  }

  // Returns the position of the first value whose key is not less than k.
  template <typename Compare>
  int lower_bound(const key_type& k, const Compare& comp) const {
    return search_type::lower_bound(k, *this, comp);
  }
  // Returns the position of the first value whose key is greater than k.
  template <typename Compare>
  int upper_bound(const key_type& k, const Compare& comp) const {
    return search_type::upper_bound(k, *this, comp);
  }

  // Returns the position of the first value whose key is not less than k using
  // linear search performed using plain compare.
  template <typename Compare>
  int linear_search_plain_compare(const key_type& k, int s, int e, const Compare& comp) const {
    while (s < e) {
      if (!btree_compare_keys(comp, key(s), k)) {
        break;
      }
      ++s;
    }
    return s;
  }

  // Returns the position of the first value whose key is not less than k using
  // linear search performed using compare-to.
  template <typename Compare>
  int linear_search_compare_to(const key_type& k, int s, int e, const Compare& comp) const {
    while (s < e) {
      int c = comp(key(s), k);
      if (c == 0) {
        return s | kExactMatch;
      } else if (c > 0) {
        break;
      }
      ++s;
    }
    return s;
  }

  // Returns the position of the first value whose key is not less than k using
  // binary search performed using plain compare.
  template <typename Compare>
  int binary_search_plain_compare(const key_type& k, int s, int e, const Compare& comp) const {
    while (s != e) {
      int mid = (s + e) / 2;
      if (btree_compare_keys(comp, key(mid), k)) {
        s = mid + 1;
      } else {
        e = mid;
      }
    }
    return s;
  }

  // Returns the position of the first value whose key is not less than k using
  // binary search performed using compare-to.
  template <typename CompareTo>
  int binary_search_compare_to(const key_type& k, int s, int e, const CompareTo& comp) const {
    while (s != e) {
      int mid = (s + e) / 2;
      int c   = comp(key(mid), k);
      if (c < 0) {
        s = mid + 1;
      } else if (c > 0) {
        e = mid;
      } else {
        // Need to return the first value whose key is not less than k, which
        // requires continuing the binary search. Note that we are guaranteed
        // that the result is an exact match because if "key(mid-1) < k" the
        // call to binary_search_compare_to() will return "mid".
        s = binary_search_compare_to(k, s, mid, comp);
        return s | kExactMatch;
      }
    }
    return s;
  }

  // Returns the pointer to the front of the values array.
  values_iterator begin_values() { return fields_.values.begin(); }

  // Returns the pointer to the back of the values array.
  values_iterator end_values() { return std::next(begin_values(), count()); }

  values_reverse_iterator rbegin_values() { return std::next(fields_.values.rbegin(), max_values_count() - values_count()); }
  values_reverse_iterator rend_values() { return fields_.values.rend(); }

  // Returns the pointer to the front of the children array.
  children_iterator begin_children() { return fields_.children.begin(); }

  // Returns the pointer to the back of the children array.
  children_iterator end_children() { return std::next(begin_children(), count() + 1); }

  children_reverse_iterator rbegin_children() { return std::next(fields_.children.rbegin(), max_children_count() - children_count()); }
  children_reverse_iterator rend_children() { return fields_.children.rend(); }

  int values_count() const { return count(); }
  int children_count() const { return count() + 1; }

  constexpr int max_values_count() const { return kNodeValues; }
  constexpr int max_children_count() const { return kNodeValues + 1; }

  // Rotate the values in the range [first, last) to the left.
  // As a result, the value pointed by middle will now be the first value and
  // the value pointed by middle - 1 will be the last value.
  // Example: let values be [0, 1, 2, 3, 4, 5], first = 1, middle = 3, last = 6,
  // after rotate_value_left(1, 3, 6) the values will be [0, 3, 4, 5, 1, 2].
  void rotate_values(int first, int middle, int last) {
    assert(0 <= first && first <= middle && middle <= last && last <= max_values_count());
    auto vbegin = begin_values();
    std::rotate(vbegin + first, vbegin + middle, vbegin + last);
  }

  // Shift the children in the range [first, last) to the right by shift.
  // CAUTION: This function  uninitializes [first, first + shift) in *this.
  void shift_children_right(int first, int last, int shift) {
    assert(0 <= first && first <= last && 0 <= shift && last + shift <= max_children_count());
    auto cbegin = begin_children();
    std::rotate(cbegin + first, cbegin + last, cbegin + last + shift);
    std::for_each(cbegin + first + shift, cbegin + last + shift, [shift](btree_node* np) {
      np->set_position(np->position() + shift);
    });
    std::fill_n(cbegin + first, shift, nullptr);
  }

  // Shift the children in the range [first, last) to the left by shift.
  // CAUTION: This function  uninitializes [last - shift, last) in *this.
  void shift_children_left(int first, int last, int shift) {
    assert(0 <= first && first <= last && last <= max_children_count());
    assert(0 <= shift && 0 <= first - shift);
    auto cbegin = begin_children();
    std::rotate(cbegin + first - shift, cbegin + first, cbegin + last);
    std::for_each(cbegin + first - shift, cbegin + last - shift, [shift](btree_node* np) {
      np->set_position(np->position() - shift);
    });
    std::fill_n(cbegin + last - shift, shift, nullptr);
  }

  // CAUTION: This function doesn't uninitialize [first, last) in *src.
  void receive_children(children_iterator dest, btree_node* src, children_iterator first, children_iterator last) {
    int dest_idx = dest - begin_children();
    int first_idx = first - src->begin_children();
    int n = last - first;
    for (int i = 0; i < n; ++i) {
      set_child(dest_idx + i, src->child(first_idx + i));
    }
  }

  // CAUTION: This function doesn't uninitialize [first, last) in *src.
  void receive_children_n(children_iterator dest, btree_node* src, children_iterator first, int n) {
    int dest_idx = dest - begin_children();
    int first_idx = first - src->begin_children();
    assert(0 <= n && dest_idx + n <= max_children_count() && first_idx + n <= src->max_children_count());
    for (int i = 0; i < n; ++i) {
      set_child(dest_idx + i, src->child(first_idx + i));
    }
  }

  // Rotate the values [values.begin() + i, values.end()) so that the last value is at position i.
  void rotate_back_to(int i);

  // Inserts the value x at position i, shifting all existing values and
  // children at positions >= i to the right by 1.
  void insert_value(int i, const value_type& x);

  // Emplaces the value at position i, shifting all existing values and children
  // at positions >= i to the right by 1.
  template <typename... Args>
  void emplace_value(int i, Args&&... args);

  // Removes the value at position i, shifting all existing values and children
  // at positions > i to the left by 1.
  void remove_value(int i);

  // Rebalances a node with its right sibling.
  void rebalance_right_to_left(btree_node* sibling, int to_move);
  void rebalance_left_to_right(btree_node* sibling, int to_move);

  // Splits a node, moving a portion of the node's values to its right sibling.
  void split(btree_node* sibling, int insert_position);

  // Merges a node with its right sibling, moving all of the values and the
  // delimiting key in the parent node onto itself.
  void merge(btree_node* sibling);

  // Swap the contents of "this" and "src".
  void swap(btree_node* src);

  // Node allocation/deletion routines.
  static btree_node* init_leaf(leaf_fields* f, btree_node* parent, int max_count) {
    btree_node* n = reinterpret_cast<btree_node*>(f);
    f->leaf       = true;
    f->position   = 0;
    f->max_count  = max_count;
    f->count      = 0;
    f->parent     = parent;
    if (!NDEBUG) {
      std::memset(f->values.data(), 0, max_count * sizeof(value_type));
    }
    return n;
  }
  static btree_node* init_internal(internal_fields* f, btree_node* parent) {
    btree_node* n = init_leaf(f, parent, kNodeValues);
    f->leaf       = false;
    if (!NDEBUG) {
      std::memset(f->children.data(), 0, sizeof(f->children));
    }
    return n;
  }
  static btree_node* init_root(root_fields* f, btree_node* parent) {
    btree_node* n = init_internal(f, parent);
    f->rightmost  = parent;
    f->size       = parent->count();
    return n;
  }
  void destroy() {
    for (int i = 0; i < count(); ++i) {
      value_destroy(i);
    }
  }

 private:
  template <typename... Args>
  void value_init(int i, Args&&... args) { new (&fields_.values[i]) mutable_value_type{std::forward<Args>(args)...}; }
  void value_init(int i, const value_type& x) { new (&fields_.values[i]) mutable_value_type(x); }
  void value_destroy(int i) { fields_.values[i].~mutable_value_type(); }

 private:
  root_fields fields_;
};

////
// btree_node methods
template <typename P>
inline void btree_node<P>::rotate_back_to(int i) {
  assert(i <= count());
  rotate_values(i, values_count(), values_count() + 1);
  set_count(count() + 1);

  if (!leaf()) {
    ++i;
    shift_children_right(i, children_count() - 1, 1);
  }
}

template <typename P>
inline void btree_node<P>::insert_value(int i, const value_type& x) {
  value_init(values_count(), x);
  rotate_back_to(i);
}

template <typename P>
template <typename... Args>
inline void btree_node<P>::emplace_value(int i, Args&&... args) {
  value_init(values_count(), std::forward<Args>(args)...);
  rotate_back_to(i);
}

template <typename P>
inline void btree_node<P>::remove_value(int i) {
  if (!leaf()) {
    assert(child(i + 1)->count() == 0);
    if (i + 2 != children_count()) {
      shift_children_left(i + 2, children_count(), 1);
    }
    *mutable_child(children_count() - 1) = nullptr;
  }

  auto old_values_count = values_count();
  set_count(count() - 1);
  if (i != count()) {
    rotate_values(i, i + 1, old_values_count);
  }
  value_destroy(old_values_count - 1);
}

template <typename P>
void btree_node<P>::rebalance_right_to_left(btree_node* src, int to_move) {
  assert(parent() == src->parent());
  assert(position() + 1 == src->position());
  assert(src->count() >= count());
  assert(to_move >= 1);
  assert(to_move <= src->count());

  // Make room in the left node for the new values.
  for (int i = 0; i < to_move; ++i) {
    value_init(i + values_count());
  }

  // Move the delimiting value to the left node and the new delimiting value
  // from the right node.
  value_swap(values_count(), parent(), position());
  parent()->value_swap(position(), src, to_move - 1);

  // Move the values from the right to the left node.
  std::swap_ranges(end_values() + 1, end_values() + to_move, src->begin_values());
  // Shift the values in the right node to their correct position.
  src->rotate_values(0, to_move, src->count());
  for (int i = 1; i <= to_move; ++i) {
    src->value_destroy(src->values_count() - i);
  }

  if (!leaf()) {
    // Move the child pointers from the right to the left node.
    receive_children_n(end_children(), src, src->begin_children(), to_move);
    assert(src->count() <= src->max_count());
    src->shift_children_left(to_move, src->children_count(), to_move);
  }

  // Fixup the counts on the src and dest nodes.
  set_count(count() + to_move);
  src->set_count(src->count() - to_move);
}

template <typename P>
void btree_node<P>::rebalance_left_to_right(btree_node* dest, int to_move) {
  assert(parent() == dest->parent());
  assert(position() + 1 == dest->position());
  assert(count() >= dest->count());
  assert(to_move >= 1);
  assert(to_move <= count());

  // Make room in the right node for the new values.
  for (int i = 0; i < to_move; ++i) {
    dest->value_init(i + dest->values_count());
  }
  dest->rotate_values(0, dest->values_count(), dest->values_count() + to_move);

  // Move the delimiting value to the right node and the new delimiting value
  // from the left node.
  dest->value_swap(to_move - 1, parent(), position());
  parent()->value_swap(position(), this, values_count() - to_move);
  value_destroy(values_count() - to_move);

  // Move the values from the left to the right node.
  std::swap_ranges(end_values() - to_move + 1, end_values(), dest->begin_values());
  for (int i = 1; i < to_move; ++i) {
    value_destroy(values_count() - to_move + i);
  }

  if (!leaf()) {
    // Move the child pointers from the left to the right node.
    dest->shift_children_right(0, dest->children_count(), to_move);
    dest->receive_children_n(dest->begin_children(), this, begin_children() + children_count() - to_move, to_move);
    std::fill_n(rbegin_children(), to_move, nullptr);
  }

  // Fixup the counts on the src and dest nodes.
  set_count(count() - to_move);
  dest->set_count(dest->count() + to_move);
}

template <typename P>
void btree_node<P>::split(btree_node* dest, int insert_position) {
  assert(dest->count() == 0);

  // We bias the split based on the position being inserted. If we're
  // inserting at the beginning of the left node then bias the split to put
  // more values on the right node. If we're inserting at the end of the
  // right node then bias the split to put more values on the left node.
  if (insert_position == 0) {
    dest->set_count(count() - 1);
  } else if (insert_position == max_count()) {
    dest->set_count(0);
  } else {
    dest->set_count(count() / 2);
  }
  set_count(count() - dest->count());
  assert(count() >= 1);

  // Move values from the left sibling to the right sibling.
  for (int i = 0; i < dest->count(); ++i) {
    dest->value_init(i);
  }
  std::move(end_values(), end_values() + dest->count(), dest->begin_values());
  for (int i = 0; i < dest->count(); ++i) {
    value_destroy(values_count() + i);
  }

  // The split key is the largest value in the left sibling.
  set_count(count() - 1);
  parent()->emplace_value(position());
  value_swap(values_count(), parent(), position());
  value_destroy(values_count());
  parent()->set_child(position() + 1, dest);

  if (!leaf()) {
    for (int i = 0; i <= dest->count(); ++i) {
      assert(child(children_count() + i) != nullptr);
    }
    dest->receive_children_n(dest->begin_children(), this, end_children(), dest->count() + 1);
    std::fill_n(end_children(), dest->count() + 1, nullptr);
  }
}

template <typename P>
void btree_node<P>::merge(btree_node* src) {
  assert(parent() == src->parent());
  assert(position() + 1 == src->position());

  // Move the delimiting value to the left node.
  value_init(values_count());
  value_swap(values_count(), parent(), position());

  // Move the values from the right to the left node.
  for (int i = 0; i < src->values_count(); ++i) {
    value_init(values_count() + 1 + i);
  }
  std::swap_ranges(src->begin_values(), src->end_values(), end_values() + 1);
  for (int i = 0; i < src->values_count(); ++i) {
    src->value_destroy(i);
  }

  if (!leaf()) {
    // Move the child pointers from the right to the left node.
    receive_children(end_children(), src, src->begin_children(), src->end_children());
    std::fill(src->begin_children(), src->end_children(), nullptr);
  }

  // Fixup the counts on the src and dest nodes.
  set_count(1 + count() + src->count());
  src->set_count(0);

  // Remove the value on the parent node.
  parent()->remove_value(position());
}

template <typename P>
void btree_node<P>::swap(btree_node* x) {
  assert(leaf() == x->leaf());

  // Swap the values.
  for (int i = count(); i < x->count(); ++i) {
    value_init(i);
  }
  for (int i = x->count(); i < count(); ++i) {
    x->value_init(i);
  }
  int n = std::max(count(), x->count());
  std::swap_ranges(begin_values(), begin_values() + n, x->begin_values());
  for (int i = count(); i < x->count(); ++i) {
    x->value_destroy(i);
  }
  for (int i = x->count(); i < count(); ++i) {
    value_destroy(i);
  }

  if (!leaf()) {
    // Swap the child pointers.
    std::swap_ranges(begin_children(), begin_children() + n + 1, x->begin_children());
    std::for_each_n(x->begin_children(), children_count(), [x](btree_node* np) {
      np->fields_.parent = x;
    });
    std::for_each_n(begin_children(), x->children_count(), [this](btree_node* np) {
      np->fields_.parent = this;
    });
  }

  // Swap the counts.
  btree_swap_helper(fields_.count, x->fields_.count);
}

} // namespace cppbtree

#endif // CPPBTREE_BTREE_NODE_H_