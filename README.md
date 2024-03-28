# cpp-btree

cpp-btree is an implementation of B-tree which have the same interface as the standard C++ containers, so it is relatively simple to replace `map` with `btree_map`, `set` with `btree_set`, `multimap` with `btree_multimap` and `multiset` with `btree_multiset`.

## Installation
### CMake (recommended)
You only have to add the following codes to your `CMakeLists.txt`.

```cmake
find_package(platanus CONFIG)

# replace 'foo' and 'main.cpp' with your programs.
add_executable(foo main.cpp)
target_link_libraries(foo platanus::platanus)
```

### Source
We recommend using CMake, but you can also use by downloading source codes.
In this case, copy the `platanus` directory in the uncompressed directory to your source directory.

## How to use
See [Google's usage instructions](http://code.google.com/p/cpp-btree/wiki/UsageInstructions).

## Test
If you want to test, Download and install the following libraries.

- [googletest](https://github.com/google/googletest)
- [gflags](https://github.com/google/googletest)

Then, run `cmake -S . -B build -Dbuild_tests=ON`.

## License
cpp-btree is licensed under [Apache License, Version 2.0](COPYING).
