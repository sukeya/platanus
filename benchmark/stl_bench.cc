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

#include <stdint.h>
#include <stdlib.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <random>
#include <set>
#include <string>
#include <sys/time.h>
#include <type_traits>
#include <vector>

#include <array>
#include <algorithm>
#include <deque>
#include <list>
#include <chrono>
#include <random>
#include <iostream>
#include <utility>

#include "gflags/gflags.h"
#include "../test/btree_test.h"

DEFINE_int32(test_random_seed, 123456789, "Seed for srand()");
DEFINE_int32(benchmark_max_iters, 1'000'000, "Maximum test iterations");
DEFINE_int32(benchmark_min_iters, 100, "Minimum test iterations");
DEFINE_int32(benchmark_target_seconds, 1, "Attempt to benchmark for this many seconds");

using std::allocator;
using std::max;
using std::min;
using std::string;
using std::vector;
using std::deque;
using std::list;

namespace platanus {
struct node {
  static constexpr int N = 10;
  float a[N];
};

template <>
struct Generator<node> {
  int maxval;
  std::random_device seed_gen;
  std::mt19937 engine;
  std::uniform_real_distribution<float> dist;
  Generator(int m) : maxval(m) {
    engine = std::mt19937(seed_gen());
    dist = std::uniform_real_distribution<float>(0, maxval);
  }
  node operator()(int) {
    node n;
    for (int i = 0; i < node::N; ++i) {
      n.a[i] = dist(engine);
    }
    return n;
  }
};

namespace {
struct StringComp {
bool operator()(const std::string& lhd, const std::string& rhd) const noexcept {
  auto result = lhd.compare(rhd);
  if (result < 0) {
    return true;
  } else {
    return false;
  }
}
};

struct NodeComp {
float sum(const node& n) const noexcept {
  float result = 0;
  for (int i = 0; i < node::N; ++i) {
    result += n.a[i];
  }
  return result;
}
bool operator()(const node& lhd, const node& rhd) const noexcept {
  if (sum(lhd) < sum(rhd)) {
    return true;
  } else {
    return false;
  }
}
};

struct BenchmarkRun {
  BenchmarkRun(const char* name, void (*func)(int));
  void Run();
  void Stop();
  void Start();
  void Reset();

  BenchmarkRun* next_benchmark;
  const char*   benchmark_name;
  void (*benchmark_func)(int);
  int64_t accum_micros;
  int64_t last_started;
};

BenchmarkRun* first_benchmark;
BenchmarkRun* current_benchmark;

int64_t get_micros() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

BenchmarkRun::BenchmarkRun(const char* name, void (*func)(int))
    : next_benchmark(first_benchmark),
      benchmark_name(name),
      benchmark_func(func),
      accum_micros(0),
      last_started(0) {
  first_benchmark = this;
}

#define BTREE_BENCHMARK(name)                 BTREE_BENCHMARK2(#name, name, __COUNTER__)
#define BTREE_BENCHMARK2(name, func, counter) BTREE_BENCHMARK3(name, func, counter)
#define BTREE_BENCHMARK3(name, func, counter) BenchmarkRun bench##counter(name, func)

void StopBenchmarkTiming() { current_benchmark->Stop(); }

void StartBenchmarkTiming() { current_benchmark->Start(); }

void RunBenchmarks() {
  for (BenchmarkRun* bench = first_benchmark; bench; bench = bench->next_benchmark) {
    bench->Run();
  }
}

void BenchmarkRun::Start() {
  assert(!last_started);
  last_started = get_micros();
}

void BenchmarkRun::Stop() {
  if (last_started == 0) {
    return;
  }
  accum_micros += get_micros() - last_started;
  last_started = 0;
}

void BenchmarkRun::Reset() {
  last_started = 0;
  accum_micros = 0;
}

void BenchmarkRun::Run() {
  assert(current_benchmark == nullptr);
  current_benchmark = this;
  int iters         = FLAGS_benchmark_min_iters;
  for (;;) {
    Reset();
    Start();
    benchmark_func(iters);
    Stop();
    if (accum_micros > FLAGS_benchmark_target_seconds * 1000000
        || iters >= FLAGS_benchmark_max_iters) {
      break;
    } else if (accum_micros == 0) {
      iters *= 100;
    } else {
      int64_t target_micros = FLAGS_benchmark_target_seconds * 1000000;
      iters                 = target_micros * iters / accum_micros;
    }
    iters = min(iters, FLAGS_benchmark_max_iters);
  }
  fprintf(stdout, "%s\t%ld\t%d\n", benchmark_name, accum_micros * 1000 / iters, iters);
  current_benchmark = nullptr;
}

// Used to avoid compiler optimizations for these benchmarks.
template <typename T>
void sink(const T& t0) {
  volatile T t = t0;
}

// Benchmark insertion of values into a container.
template <typename T>
void BM_Insert(int n) {
  using V = typename std::remove_const<typename T::value_type>::type;

  // Disable timing while we perform some initialization.
  StopBenchmarkTiming();

  T         container;
  vector<V> values = GenerateValues<V>(FLAGS_benchmark_values);
  for (int i = 0; i < values.size(); i++) {
    container.push_back(values[i]);
  }

  for (int i = 0; i < n;) {
    // Remove and re-insert 10% of the keys
    int m = min(n - i, FLAGS_benchmark_values / 10);

    for (int j = i; j < i + m; j++) {
      int x = j % container.size();
      container.erase(std::next(container.begin(), x));
    }

    StartBenchmarkTiming();

    for (int j = i; j < i + m; j++) {
      int x = j % container.size();
      container.insert(std::next(container.begin(), x), values[x]);
    }

    StopBenchmarkTiming();

    i += m;
  }
}

// Benchmark lookup of values in a container.
template <typename T>
void BM_Lookup(int n) {
  using V = typename std::remove_const<typename T::value_type>::type;

  // Disable timing while we perform some initialization.
  StopBenchmarkTiming();

  T         container;
  vector<V> values = GenerateValues<V>(FLAGS_benchmark_values);
  vector<V> sorted(values);

  if constexpr (std::is_same_v<V, std::string>) {
    std::sort(sorted.begin(), sorted.end(), StringComp{});
  } else if constexpr (std::is_same_v<V, node>) {
    std::sort(sorted.begin(), sorted.end(), NodeComp{});
  } else {
    std::sort(sorted.begin(), sorted.end());
  }

  for (int i = 0; i < sorted.size(); i++) {
    container.push_back(sorted[i]);
  }

  bool r = false;

  StartBenchmarkTiming();

  for (int i = 0; i < n; i++) {
    int m = i % values.size();

    if constexpr (std::is_same_v<V, std::string>) {
      r = std::binary_search(values.begin(), values.end(), values[m], StringComp{});
    } else if constexpr (std::is_same_v<V, node>) {
      std::sort(sorted.begin(), sorted.end(), NodeComp{});
    } else {
      r = std::binary_search(values.begin(), values.end(), values[m]);
    }
  }

  StopBenchmarkTiming();

  sink(r);  // Keep compiler from optimizing away r.
}

// Benchmark deletion of values from a container.
template <typename T>
void BM_Delete(int n) {
  using V = typename std::remove_const<typename T::value_type>::type;

  // Disable timing while we perform some initialization.
  StopBenchmarkTiming();

  T         container;
  vector<V> values = GenerateValues<V>(FLAGS_benchmark_values);
  for (int i = 0; i < values.size(); i++) {
    container.push_back(values[i]);
  }

  for (int i = 0; i < n;) {
    // Remove and re-insert 10% of the keys
    int m = min(n - i, FLAGS_benchmark_values / 10);

    StartBenchmarkTiming();

    for (int j = i; j < i + m; j++) {
      int x = j % container.size();
      container.erase(std::next(container.begin(), x));
    }

    StopBenchmarkTiming();

    for (int j = i; j < i + m; j++) {
      int x = j % container.size();
      container.insert(std::next(container.begin(), x), values[x]);
    }

    i += m;
  }
}

// Insertion at end, removal from the beginning.  This benchmark
// counts two value constructors.
template <typename T>
void BM_Fifo(int n) {
  using V = typename std::remove_const<typename T::value_type>::type;

  // Disable timing while we perform some initialization.
  StopBenchmarkTiming();

  T            container;
  Generator<V> g(FLAGS_benchmark_values + FLAGS_benchmark_max_iters);

  for (int i = 0; i < FLAGS_benchmark_values; i++) {
    container.push_back(g(i));
  }

  StartBenchmarkTiming();

  for (int i = 0; i < n; i++) {
    container.erase(container.begin());
    container.push_back(g(i + FLAGS_benchmark_values));
  }

  StopBenchmarkTiming();
}

// Iteration (forward) through the tree
template <typename T>
void BM_FwdIter(int n) {
  using V = typename std::remove_const<typename T::value_type>::type;

  // Disable timing while we perform some initialization.
  StopBenchmarkTiming();

  T         container;
  vector<V> values = GenerateValues<V>(FLAGS_benchmark_values);

  for (int i = 0; i < FLAGS_benchmark_values; i++) {
    container.push_back(values[i]);
  }

  typename T::iterator iter;

  V r = V();

  StartBenchmarkTiming();

  for (int i = 0; i < n; i++) {
    int idx = i % FLAGS_benchmark_values;

    if (idx == 0) {
      iter = container.begin();
    }
    r = *iter;
    ++iter;
  }

  StopBenchmarkTiming();

  sink(r);  // Keep compiler from optimizing away r.
}

using vector_int32 = vector<int32_t>;
using vector_int64 = vector<int64_t>;
using vector_string = vector<string>;
using vector_node = vector<node>;

using deque_int32 = deque<int32_t>;
using deque_int64 = deque<int64_t>;
using deque_string = deque<string>;
using deque_node = deque<node>;

using list_int32 = list<int32_t>;
using list_int64 = list<int64_t>;
using list_string = list<string>;
using list_node = list<node>;

#define MY_BENCHMARK2(type, name, func)                  \
  void BM_##type##_##name(int n) { BM_##func<type>(n); } \
  BTREE_BENCHMARK(BM_##type##_##name)

#define MY_BENCHMARK(type)                       \
  MY_BENCHMARK2(type, insert, Insert);           \
  MY_BENCHMARK2(type, lookup, Lookup);           \
  MY_BENCHMARK2(type, delete, Delete);           \
  MY_BENCHMARK2(type, fifo, Fifo);               \
  MY_BENCHMARK2(type, fwditer, FwdIter)

MY_BENCHMARK(vector_int64);
MY_BENCHMARK(vector_string);
MY_BENCHMARK(vector_node);

MY_BENCHMARK(deque_int64);
MY_BENCHMARK(deque_string);
MY_BENCHMARK(deque_node);

MY_BENCHMARK(list_int64);
MY_BENCHMARK(list_string);
MY_BENCHMARK(list_node);
}  // namespace
}  // namespace platanus

int main(int argc, char** argv) {
  platanus::RunBenchmarks();
  return 0;
}
