# platanus

platanus is an fork of [cpp-btree](https://code.google.com/archive/p/cpp-btree/) which is an implementation of B-tree having the same interface as the standard C++ containers.
So, it is relatively simple to replace `map` with `btree_map`, `set` with `btree_set`, `multimap` with `btree_multimap` and `multiset` with `btree_multiset`.

## Installation
### CMake (recommended)
You only have to add this project directory to your project and the following codes to your `CMakeLists.txt`.

```cmake
add_subdirectory(platanus)

# replace 'foo' and 'main.cpp' with your programs.
add_executable(foo main.cpp)
target_link_libraries(foo platanus)
```

### Source
We recommend using CMake, but you can also use by downloading source codes.
In this case, copy the `platanus` directory in the uncompressed directory to your source directory.

## Documentation
Below the `docs` directory, there are the reference of all classes this library provides.
You might also want to see [Google's usage instructions](http://code.google.com/p/cpp-btree/wiki/UsageInstructions).

## Test
If you want to test, download and install the following libraries.

- [googletest](https://github.com/google/googletest)
- [gflags](https://github.com/google/googletest)

Then, run the following commands in the top directory of `platanus`.
```
cmake -S . -B build/debug -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cd build/debug/test
./btree_test
```

#### Performance test
If you want to check how much `platanus` is faster than STL, run the following commands.

```
cmake -S . -B build/release -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release
cd build/release/benchmark
./btree_bench
```

The output has the following form.

```
BM_("stl" or "btree_(the max number of values per node)")_(container type)_(value type)_(test case) (average time[ns] per iteration) (the total of iterations)
```

The test cases are:

| test case | meaning |
| --- | --- |
| Insert | Benchmark insertion of values into a container. |
| Lookup | Benchmark lookup of values in a container. |
| FullLookup | Benchmark lookup of values in a full container, meaning that values are inserted in-order to take advantage of biased insertion, which yields a full tree. |
| Delete | Benchmark deletion of values from a container. |
| QueueAddRem | Benchmark steady-state insert (into first half of range) and remove (from second second half of range), treating the container approximately like a queue with log-time access for all elements. This benchmark does not test the case where insertion and removal happen in the same region of the tree.  This benchmark counts two value constructors. |
| MixedAddRem | Benchmark mixed insertion and deletion in the same range using pre-constructed values. |
| Fifo | Insertion at end, removal from the beginning. This benchmark counts one value constructors. |
| FwdIter | Iteration (forward) through the tree. |

If you want to know a good size of values per node, run the following comamnd.

```
cmake -S . -B build/release -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DVALUES_SIZE_TEST
cd build/release/benchmark
./btree_bench
```

## License
platanus is licensed under [Apache License, Version 2.0](COPYING).
