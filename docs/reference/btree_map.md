# btree_map
In `platanus/btree_map.hpp`,

```cpp
namespace platanus {
template <
    typename Key,
    typename Value,
    typename Compare           = std::ranges::less,
    typename Alloc             = std::allocator<Key>,
    std::size_t MaxNumOfValues = 512>
class btree_map;
}
```


## Template parameters

| Parameter | Meaning |
| --- | --- |
| `Key` | Type of a key |
| `Value` | Type of a value |
| `Compare` | Type of a func obj comparing two key in a weak order. If Key doesn't implement three-way comparison operator, the default type does using `<` and `=`. |
| `Alloc` | Type of an allocator. The default is `std::allocator<Key>`. |
| `MaxNumOfValues` | The max number of values per node. The default is 64. |


## Member types

| Type | Meaning |
| --- | --- |
| key_type | Type of key, i.e. `Key` |
| value_type | Type of value, i.e. `std::pair<const Key, Value>` |
| mapped_type | Type of mapped value, i.e. `Value` |
| key_compare | Type of comparer of key, i.e. `Compare` |
| allocator_type | Type of allocator, i.e. `Alloc` |
| pointer | Type of pointer to value, i.e. `value_type*` |
| const_pointer | Type of pointer to const value, i.e. `const value_type*` |
| reference | Type of reference to value, i.e. `value_type&` |
| const_reference | Type of reference to const value, i.e. `const value_type&` |
| size_type | Unsigned integer type of size of `btree_map`, i.e. `std::size_t` |
| difference_type | Signed integer type of the difference of two iterators |
| iterator | Type of iterator |
| const_iterator | Type of const iterator |
| reverse_iterator | Type of reverse iterator |
| const_reverse_iterator | Type of const reverse iterator |


## Member function
### Constructor
```cpp
// (1)
btree_map();
// (2)
btree_map(const btree_map&);
// (3)
btree_map(btree_map&&);

// (4)
explicit btree_map(const key_compare& comp, const allocator_type& alloc = allocator_type());
// (5)
explicit btree_map(const allocator_type& alloc);

// (6)
template <class InputIterator>
btree_map(
    InputIterator         b,
    InputIterator         e,
    const key_compare&    comp  = key_compare(),
    const allocator_type& alloc = allocator_type()
);
// (7)
template <class InputIterator>
btree_map(InputIterator b, InputIterator e, const allocator_type& alloc);

// (8)
btree_map(const btree_map& x, const allocator_type& alloc);
// (9)
btree_map(btree_map&& x, const allocator_type& alloc);

// (10)
btree_map(
    std::initializer_list<value_type> init,
    const key_compare&                comp  = key_compare{},
    const allocator_type&             alloc = allocator_type{}
);
// (11)
btree_map(std::initializer_list<value_type> init, const allocator_type& alloc);

// (12)
btree_map& operator=(const btree_map& x);
// (13)
btree_map& operator=(btree_map&&) = default;
```

1. Constructs an empty `btree_map`.
1. Copy constructor. The allocator is copied by `std::allocator_traits::select_on_container_copy_construction`.
1. Move constructor. The allocator is copied by `operator=`.
1. Constructs an empty `btree_map` with `comp` and `alloc`.
1. Constructs an empty `btree_map` with `alloc`.
1. Constructs a `btree_map` with `comp` and `alloc` by inserting values from `b` to `e` (`e` isn't included).
1. Constructs a `btree_map` with `alloc` by inserting values from `b` to `e` (`e` isn't included).
1. Constructs a copy of `x` with `alloc`.
1. Constructs a `btree_map` to which `x` is moved with `alloc`.
1. Constructs a `btree_map` with `comp` and `alloc` from an initializer list.
1. Constructs a `btree_map` with `alloc` from an initializer list.
1. Assigns a copy of `x` and return `*this`. If `x` is `*this`, does nothing.
1. Default move assignment operator.


### Iterator
```cpp
// (1)
iterator               begin();
// (2)
const_iterator         begin() const;
// (3)
const_iterator         cbegin() const;

// (4)
iterator               end();
// (5)
const_iterator         end() const;
// (6)
const_iterator         cend() const;

// (7)
reverse_iterator       rbegin();
// (8)
const_reverse_iterator rbegin() const;
// (9)
const_reverse_iterator crbegin() const;

// (10)
reverse_iterator       rend();
// (11)
const_reverse_iterator rend() const;
// (12)
const_reverse_iterator crend() const;
```

1. Return the iterator pointing to the first value of `*this`.
1. The `const` version of 1.
1. Return the const iterator pointing to the first value of `*this`.
1. Return the iterator pointing to the next address of the last value of `*this`.
1. The `const` version of 4.
1. Return the const iterator pointing to the next address of the last value of `*this`.
1. Return the reverse iterator pointing to the last value of `*this`.
1. The `const` version of 7.
1. Return the const reverse iterator pointing to the last value of `*this`.
1. Return the reverse iterator pointing to the previous address of the first value of `*this`.
1. The `const` version of 10.
1. Return the const reverse iterator pointing to the previous address of the first value of `*this`.


### Modifiers
```cpp
// (1)
void clear();
// (2)
void swap(btree_map& x);
// (3)
void merge(btree_map& x);
// (4)
void merge(btree_map&& x);
// (5)
mapped_type& operator[](const key_type& key);
// (6)
mapped_type& operator[](key_type&& key);
// (7)
mapped_type& at(const key_type& key);
// (8)
const mapped_type& at(const key_type& key) const;
```

1. Clear `*this`, i.e., delete all values in `*this`.
1. Swap `*this` for `x`.
1. Merge another `btree_map`. The duplicated values will not be merged to `*this`.
1. Same as 3. This function is provided to receive an rvalue `btree_map`, so no rvalue-specific optimization is done.
1. Return the reference of the value mapped to `key`. If `key` doesn't exist, insert `key` and default constructed `mapped_value`.
1. The rvalue version of 5.
1. Return the reference of the value mapped to `key`. If `key` doesn't exist, throw `std::out_of_range`.
1. The const version of 7.


#### `insert`
```cpp
// (1)
std::pair<iterator, bool> insert(const value_type& x);
// (2)
std::pair<iterator, bool> insert(value_type&& x);

// (3)
iterator insert(iterator hint, const value_type& x);
// (4)
iterator insert(iterator hint, value_type&& x);

// (5)
template <typename InputIterator>
void insert(InputIterator b, InputIterator e);

// (6)
void insert(std::initializer_list<value_type> list);
```

1. If there is `x` in `*this`, returns an iterator pointing to `x` in `*this` and `false`. Otherwise, inserts `x` and returns an iterator pointing to `x` in `*this` and `true`.
1. The rvalue version of 1.
1. Does 1 with hint. If the value is placed immediately before `hint` in the tree, the insertion will take amortized constant time. If not, the insertion will take amortized logarithmic time as if a call to `insert(x)` were made.
1. The rvalue version of 3.
1. Does 1 for each value from `b` to `e` (`e` isn't included).
1. Does 1 for each value of `list`.


#### `erase`
```cpp
size_type erase(const key_type& key);
iterator  erase(const iterator& iter);
void      erase(const iterator& b, const iterator& e);
```

1. Erases the specified key from `*this` and returns
1. Erases the specified iterator from `*this`. The iterator must be valid (i.e. not equal to end()). Return an iterator pointing to the node after the one that was erased (or end() if none exists).
1. Erases a range from `b` to `e` (`e` isn't included) and return the number of erased keys.


### Lookup
```cpp
// (1)
iterator       lower_bound(const key_type& key);
// (2)
const_iterator lower_bound(const key_type& key) const;

// (3)
iterator       upper_bound(const key_type& key);
// (4)
const_iterator upper_bound(const key_type& key) const;

// (5)
std::pair<iterator, iterator> equal_range(const key_type& key);
// (6)
std::pair<const_iterator, const_iterator> equal_range(const key_type& key) const;

// (7)
iterator       find(const key_type& key);
// (8)
const_iterator find(const key_type& key) const;

// (9)
size_type      count(const key_type& key) const;

// (10)
bool           contains(const key_type& key) const;
```

1. If `*this` is empty or `key` isn't found, returns `end()`. Otherwise, returns an iterator which points to the minimum value in all values which are not less than `key`.
1. The `const` version of 1.
1. If `*this` is empty or `key` isn't found, returns `end()`. Otherwise, returns an iterator which points to the minimum value in all values which are greater than `key`.
1. The `const` version of 3.
1. If `*this` is empty or `key` isn't found, returns `std::make_pair(end(), end())`. Otherwise, returns a pair of the iterators which point to the fist value in all values which are equal to `key` and the value next to the last value in them in order.
1. The `const` version of 5.
1. If `*this` is empty or `key` isn't found, returns `end()`. Otherwise, returns an iterator which points to the fist value in all values which are equal to `key`.
1. The `const` version of 7.
1. If `*this` is empty or `key` isn't found, returns `0`. Otherwise, returns `1`.
1. If `*this` is empty or `key` isn't found, returns `false`. Otherwise, returns `true`.


### Capacity
```cpp
// (1)
size_type size() const;
// (2)
size_type max_size() const;
// (3)
bool      empty() const;
```
1. Returns the number of values.
1. Returns the max number of values.
1. Returns a bool whether `*this` is empty or not.


### Observers
```cpp
key_compare key_comp() const noexcept;
```
Returns the `key_compare` object used by `*this`.


### non-STL
<span style="color: red">**[WARNING] The following functions may be suddenly deleted.**</span>

```cpp
// (1)
size_type height() const;
// (2)
size_type internal_nodes() const;
// (3)
size_type leaf_nodes() const;
// (4)
size_type nodes() const;
// (5)
size_type bytes_used() const;
// (6)
double    average_bytes_per_value() const;
// (7)
double    fullness() const;
// (8)
double    overhead() const;
// (9)
void      dump(std::ostream& os) const;
// (10)
void      verify() const;
// (11)
static constexpr std::size_t sizeof_leaf_node();
// (12)
static constexpr std::size_t sizeof_internal_node();
```

1. Returns the hight of `*this`.
1. Returns the number of internal nodes.
1. Returns the number of leaf nodes.
1. Returns the total of internal nodes and leaf nodes.
1. Returns the total of bytes used by `*this`.
1. Returns the average bytes per value.
1. Returns the fullness of `*this`, which is computed as the number of elements in `*this` divided by the maximum number of elements a tree with the current number of nodes could hold.
A value of 1 indicates perfect space utilization.
Smaller values indicate space wastage.
1. Returns the overhead of the btree structure in bytes per node, which is computed as the total number of bytes used by `*this` minus the number of bytes used for storing elements divided by the number of values.
1. Dumps the btree to the specified ostream. Requires that `operator<<` is defined for value.
1. Verifies the structure of `*this`.
1. Returns the size of a leaf node.
1. Returns the size of a internal node.


## Non-member function
### Swap
```cpp
template <typename K, typename C, typename A, std::size_t N>
void swap(btree_map<K, C, A, N>& x, btree_map<K, C, A, N>& y);
```

Swap `x` for `y` using `std::swap` to swap each member variable of `x` for that of `y`.
