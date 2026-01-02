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
//
// Copyright 2024-2025 Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "benchmark/benchmark.h"

#include "platanus/btree_map.hpp"
#include "platanus/btree_set.hpp"
#include "util.hpp"

static_assert(sizeof(std::size_t) == 8, "must fix the bits of mersenne twister engine.");

// The size of values used in each benchmark.
static constexpr std::int64_t values_size = 1'000'000;

struct StringComp {
  std::weak_ordering operator()(const std::string& lhd, const std::string& rhd) const noexcept {
    auto result = lhd.compare(rhd);
    if (result < 0) {
      return std::weak_ordering::less;
    } else if (result > 0) {
      return std::weak_ordering::greater;
    } else {
      return std::weak_ordering::equivalent;
    }
  }
};

// Benchmark insertion of values into a container.
template <typename T>
static void BM_Insert(benchmark::State& state) {
  using V = std::remove_const_t<typename T::value_type>;

  std::vector<V> values = platanus::GenerateValues<V>(values_size);

  T container;
  for (auto _ : state) {
    if (values.empty()) {
      state.PauseTiming();
      values = platanus::GenerateValues<V>(values_size);
      container.clear();
      state.ResumeTiming();
    }
    container.insert(std::move(values.back()));
    values.pop_back();
  }
}

// Benchmark lookup of values in a container.
template <typename T>
static void BM_Lookup(benchmark::State& state) {
  using V          = std::remove_const_t<typename T::value_type>;
  using KeyOfValue = platanus::KeyOfValue<typename T::key_type, V>::type;

  std::vector<V> values = platanus::GenerateValues<V>(values_size);
  T              container{values.begin(), values.end()};

  std::random_device                         seed_gen;
  std::mt19937_64                            engine(seed_gen());
  std::uniform_int_distribution<std::size_t> dist{0, values_size - 1};

  for (auto _ : state) {
    auto r = KeyOfValue::Get(*container.find(KeyOfValue::Get(values[dist(engine)])));
    benchmark::DoNotOptimize(r);
  }
}

// Benchmark deletion of values from a container.
template <typename T>
static void BM_Delete(benchmark::State& state) {
  using V          = std::remove_const_t<typename T::value_type>;
  using KeyOfValue = platanus::KeyOfValue<typename T::key_type, V>::type;

  std::vector<V> values = platanus::GenerateValues<V>(values_size);
  T              container{values.begin(), values.end()};

  for (auto _ : state) {
    if (values.empty()) {
      state.PauseTiming();
      values    = platanus::GenerateValues<V>(values_size);
      container = T{values.begin(), values.end()};
      state.ResumeTiming();
    }
    container.erase(KeyOfValue::Get(values.back()));
    values.pop_back();
  }
}

// Iteration (forward) through the tree
template <typename T>
static void BM_FwdIter(benchmark::State& state) {
  using V          = std::remove_const_t<typename T::value_type>;
  using KeyOfValue = platanus::KeyOfValue<typename T::key_type, V>::type;

  std::vector<V> values = platanus::GenerateValues<V>(values_size);
  T              container{values.begin(), values.end()};

  auto iter = container.begin();

  for (auto _ : state) {
    if (iter == container.end()) {
      iter = container.begin();
    }

    auto r = KeyOfValue::Get(*iter);
    ++iter;

    benchmark::DoNotOptimize(r);
  }
}

template <typename T>
static void initialize_tree(T& t) {
  using V = std::remove_const_t<typename T::value_type>;

  t.clear();
  std::vector<V> values =
      platanus::GenerateValues<V>(static_cast<std::size_t>(std::sqrt(values_size)));
  t = T{values.begin(), values.end()};
}

// Merge two b-tree.
template <typename T>
static void BM_Merge(benchmark::State& state) {
  for (auto _ : state) {
    T trunk, branch;
    state.PauseTiming();
    initialize_tree(trunk);
    initialize_tree(branch);
    state.ResumeTiming();

    trunk.merge(branch);
    benchmark::DoNotOptimize(trunk);
  }
}

template <
    template <class, class, class, std::int_least16_t> class BTreeContainer,
    class T,
    std::int_least16_t N>
struct SetCompAndAllocToSet {
  using type = BTreeContainer<T, std::ranges::less, std::allocator<T>, N>;
};

template <
    template <class, class, class, std::int_least16_t> class BTreeContainer,
    std::int_least16_t N>
struct SetCompAndAllocToSet<BTreeContainer, std::string, N> {
  using type = BTreeContainer<std::string, StringComp, std::allocator<std::string>, N>;
};

template <
    template <class, class, class, class, std::int_least16_t> class BTreeContainer,
    class T,
    std::int_least16_t N>
struct SetCompAndAllocToMap {
  using type = BTreeContainer<T, T, std::ranges::less, std::allocator<T>, N>;
};

template <
    template <class, class, class, class, std::int_least16_t> class BTreeContainer,
    std::int_least16_t N>
struct SetCompAndAllocToMap<BTreeContainer, std::string, N> {
  using type = BTreeContainer<std::string, std::string, StringComp, std::allocator<std::string>, N>;
};

template <
    template <class, class, std::int_least16_t> class BTreeContainer,
    class T,
    std::int_least16_t N>
struct SetCompToPmrSet {
  using type = BTreeContainer<T, std::ranges::less, N>;
};

template <template <class, class, std::int_least16_t> class BTreeContainer, std::int_least16_t N>
struct SetCompToPmrSet<BTreeContainer, std::string, N> {
  using type = BTreeContainer<std::string, StringComp, N>;
};

template <
    template <class, class, class, std::int_least16_t> class BTreeContainer,
    class T,
    std::int_least16_t N>
struct SetCompToPmrMap {
  using type = BTreeContainer<T, T, std::ranges::less, N>;
};

template <
    template <class, class, class, std::int_least16_t> class BTreeContainer,
    std::int_least16_t N>
struct SetCompToPmrMap<BTreeContainer, std::string, N> {
  using type = BTreeContainer<std::string, std::string, StringComp, N>;
};

template <class T, std::int_least16_t N>
using BTreeSet = typename SetCompAndAllocToSet<platanus::btree_set, T, N>::type;

template <class T, std::int_least16_t N>
using BTreeMultiSet = typename SetCompAndAllocToSet<platanus::btree_multiset, T, N>::type;

template <class T, std::int_least16_t N>
using BTreeMap = typename SetCompAndAllocToMap<platanus::btree_map, T, N>::type;

template <class T, std::int_least16_t N>
using BTreeMultiMap = typename SetCompAndAllocToMap<platanus::btree_multimap, T, N>::type;

template <class T, std::int_least16_t N>
using BTreePmrSet = typename SetCompToPmrSet<platanus::pmr::btree_set, T, N>::type;

template <class T, std::int_least16_t N>
using BTreePmrMultiSet = typename SetCompToPmrSet<platanus::pmr::btree_multiset, T, N>::type;

template <class T, std::int_least16_t N>
using BTreePmrMap = typename SetCompToPmrMap<platanus::pmr::btree_map, T, N>::type;

template <class T, std::int_least16_t N>
using BTreePmrMultiMap = typename SetCompToPmrMap<platanus::pmr::btree_multimap, T, N>::type;

template <class T>
using STLSet = std::set<T>;

template <class T>
using STLMultiSet = std::multiset<T>;

template <class T>
using STLMap = std::map<T, T>;

template <class T>
using STLMultiMap = std::multimap<T, T>;

#ifdef PLATANUS_VALUES_SIZE_TEST
#define BTREE_BENCHMARK(tree, type, func) \
  BENCHMARK(BM_##func<tree<type, 3>>);    \
  BENCHMARK(BM_##func<tree<type, 8>>);    \
  BENCHMARK(BM_##func<tree<type, 16>>);   \
  BENCHMARK(BM_##func<tree<type, 32>>);   \
  BENCHMARK(BM_##func<tree<type, 64>>);   \
  BENCHMARK(BM_##func<tree<type, 128>>);  \
  BENCHMARK(BM_##func<tree<type, 256>>);
#else
#define BTREE_BENCHMARK(tree, type, func) \
  BENCHMARK(BM_##func<tree<type, 64>>);   \
  BENCHMARK(BM_##func<tree<type, 128>>);
#endif

#define STL_AND_BTREE_BENCHMARK(container, type, func) \
  BENCHMARK(BM_##func<STL##container<type>>);          \
  BTREE_BENCHMARK(BTree##container, type, func);       \
  BTREE_BENCHMARK(BTreePmr##container, type, func);

#define REGISTER_BENCHMARK_FUNCTIONS(container, type) \
  STL_AND_BTREE_BENCHMARK(container, type, Insert);   \
  STL_AND_BTREE_BENCHMARK(container, type, Lookup);   \
  STL_AND_BTREE_BENCHMARK(container, type, Delete);   \
  STL_AND_BTREE_BENCHMARK(container, type, FwdIter);  \
  STL_AND_BTREE_BENCHMARK(container, type, Merge);

#define REGISTER_BENCHMARK_TYPES(container)              \
  REGISTER_BENCHMARK_FUNCTIONS(container, std::int32_t); \
  REGISTER_BENCHMARK_FUNCTIONS(container, std::int64_t); \
  REGISTER_BENCHMARK_FUNCTIONS(container, std::string);

REGISTER_BENCHMARK_TYPES(Set);
REGISTER_BENCHMARK_TYPES(MultiSet);
REGISTER_BENCHMARK_TYPES(Map);
REGISTER_BENCHMARK_TYPES(MultiMap);

BENCHMARK_MAIN();
