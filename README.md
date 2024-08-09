# platanus

platanus is an fork of [cpp-btree](https://code.google.com/archive/p/cpp-btree/) which is an implementation of B-tree having the same interface as the standard C++ containers.
So, it is relatively simple to replace `map` with `btree_map`, `set` with `btree_set`, `multimap` with `btree_multimap` and `multiset` with `btree_multiset`.

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
./safe_btree_test
```

## License
platanus is licensed under [Apache License, Version 2.0](COPYING).
