# platanus
`platanus` is a modern fork of the B-tree library [`cpp-btree`](https://code.google.com/archive/p/cpp-btree/).


## Documentation
See [here](https://sukeya.github.io/platanus/).

## Benchmark
You can build the benchmark with:

```bash
cmake -S . -B build/release -DPLATANUS_BUILD_BENCHMARK=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --target btree_bench
```

When `PLATANUS_BENCHMARK_WITH_ABSL=ON` (default), the benchmark runs in comparison mode and registers:

- `STL`
- `absl`
- `platanus(64)`

In that mode, `platanus(128)` and `platanus::pmr` benchmark variants are intentionally excluded.
See the documentation site for more benchmark details and plotting scripts.


## License
`platanus` is licensed under [Apache License, Version 2.0](LICENSE).
