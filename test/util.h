#ifndef PLATANUS_GENERATOR_H__
#define PLATANUS_GENERATOR_H__

#include <string>
#include <utility>
#include <vector>
#include <set>
#include <cstdlib>
#include <sstream>
#include <type_traits>

#include "gtest/gtest.h"

#include "btree_test_flags.h"

namespace std {
// Provide operator<< support for std::pair<T, U>.
template <typename T, typename U>
ostream& operator<<(ostream& os, const std::pair<T, U>& p) {
  os << "(" << p.first << "," << p.second << ")";
  return os;
}

// Partial specialization of remove_const that propagates the removal through
// std::pair.
template <typename T, typename U>
struct remove_const<pair<T, U>> {
  using type = pair<typename remove_const<T>::type, typename remove_const<U>::type>;
};
}  // namespace std

namespace platanus {
char* GenerateDigits(char buf[16], int val, int maxval);

template <typename K>
struct Generator {
  int maxval;
  Generator(int m) : maxval(m) {}
  K operator()(int i) const {
    EXPECT_LE(i, maxval);
    return i;
  }
};

template <>
struct Generator<std::string> {
  int maxval;
  Generator(int m) : maxval(m) {}
  std::string operator()(int i) const {
    char buf[16];
    return GenerateDigits(buf, i, maxval);
  }
};

template <typename T, typename U>
struct Generator<std::pair<T, U>> {
  Generator<typename std::remove_const<T>::type> tgen;
  Generator<typename std::remove_const<U>::type> ugen;

  Generator(int m) : tgen(m), ugen(m) {}
  std::pair<T, U> operator()(int i) const { return std::make_pair(tgen(i), ugen(i)); }
};

// Generate values for our tests and benchmarks. Value range is [0, maxval].
const std::vector<int>& GenerateNumbers(int n, int maxval);

// Generates values in the range
// [0, 4 * min(FLAGS_benchmark_values, FLAGS_test_values)]
template <typename V>
std::vector<V> GenerateValues(int n) {
  int two_times_max  = 2 * std::max(FLAGS_benchmark_values, FLAGS_test_values);
  int four_times_max = 2 * two_times_max;
  EXPECT_LE(n, two_times_max);
  const std::vector<int>& nums = GenerateNumbers(n, four_times_max);
  Generator<V>            gen(four_times_max);
  std::vector<V>          vec;

  for (int i = 0; i < n; i++) {
    vec.push_back(gen(nums[i]));
  }

  return vec;
}

// Select the first member of a pair.
template <class Pair>
struct select1st {
  const typename Pair::first_type& operator()(const Pair& x) const { return x.first; }
};

// Utility class to provide an accessor for a key given a value. The default
// behavior is to treat the value as a pair and return the first element.
template <typename K, typename V>
struct KeyOfValue {
  using type = select1st<V>;
};

template <typename T>
struct identity {
  const T& operator()(const T& t) const { return t; }
};

// Partial specialization of KeyOfValue class for when the key and value are
// the same type such as in set<> and btree_set<>.
template <typename K>
struct KeyOfValue<K, K> {
  using type = identity<K>;
};

}  // namespace platanus

#endif
