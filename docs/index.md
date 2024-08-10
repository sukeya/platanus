# platanus
`platanus` is an fork of [cpp-btree](https://code.google.com/archive/p/cpp-btree/) which is an implementation of B-tree.


## Comparing to STL set/map
A btree is both smaller and faster than STL set/map.
The red-black tree implementation of STL set/map has an overhead of 3 pointers (left, right and parent) plus the node color information for each stored value.
So a `std::set<std::int32>` consumes 20 bytes for each value stored.
This btree implementation stores multiple values on fixed size nodes (usually 512 bytes) and doesn't store child
pointers for leaf nodes.
The result is that a `platanus::btree_set<std::int32>` may use much less memory per stored value.
For the random insertion benchmark in btree_test.cc, a `platanus::btree_set<std::int32>` with node-size of 512 uses 5.19 bytes per stored value.

The packing of multiple values on to each node of a btree has another effect besides better space utilization: better cache locality due to fewer cache lines being accessed.
Better cache locality translates into faster operations.


## Feature
* satisfying alignment requirement,
* supporting stateful comparer,
* the standard C++ container-like interface,
* supporting CMake,
* easier to read than the original,
* supporting three-way comparison operator.


## Performance
Generally speaking, `platunus` is slower than `cpp-btree` by approximately 13%, but is faster than `std::(multi)set` and `std::(multi)map` by approximately 59% (the values are median and the node size of B-tree is 512 byte (default)).
However, forwarding an iterator of `platanus` is extremely faster than doing that of STL, while FIFO of `platanus` is slower than that of STL by approximately 19%.
So, you should check how much the performance is improved.


## Installation
platanus is an header only library, so you don't have to install it.
When using CMake, you only have to link your program to `platanus`.
For example, `target_link_libraries(your_program PUBLIC platanus)`.


## Limitation and caveats
### Supporting order
We need at least 3 values per a node in order to perform splitting (1 value for the two nodes involved in the split and 1 value propagated to the parent as the delimiter for the split). That is, We don't support the 3 order B-trees.

### Invalidating iterators after insertions and deletions
Insertions and deletions on a btree can cause splitting, merging or rebalancing of btree nodes.
And even without these operations, insertions and deletions on a btree will move values around within a node.
In both cases, the result is that insertions and deletions can invalidate iterators pointing to values other than the one being inserted/deleted.
This is notably different from STL set/map which takes care to not invalidate iterators on `insert`/`erase` except, of course, for iterators pointing to the value being erased.
A partial workaround when erasing is available: `erase()` returns an iterator pointing to the item just after the one that was erased (or `end()` if none exists).
See also [`safe_btree`](./reference/safe_btree.md).


## License
platanus is licensed under [Apache License, Version 2.0](COPYING).
