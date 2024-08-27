#ifndef PLATANUS_GENERATOR_H__
#define PLATANUS_GENERATOR_H__

#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include <set>
#include <sstream>
#include <type_traits>

#include "gtest/gtest.h"

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
template <typename K>
struct Generator {
  static constexpr K Generate(std::size_t i) { return static_cast<K>(i); }
};

template <>
struct Generator<std::string> {
  static std::string Generate(std::size_t i) { return std::to_string(i); }
};

template <typename T_, typename U_>
struct Generator<std::pair<T_, U_>> {
  using T = std::remove_const_t<T_>;
  using U = std::remove_const_t<U_>;

  static std::pair<T, U> Generate(std::size_t i) {
    return std::make_pair(Generator<T>::Generate(i), Generator<U>::Generate(i));
  }
};

// Generate random values for our tests and benchmarks.
const std::vector<std::size_t>& GenerateNumbers(std::size_t n);

// Generates values in the range
template <typename V>
std::vector<V> GenerateValues(std::size_t n) {
  std::vector<V> vec;

  for (auto i : GenerateNumbers(n)) {
    vec.push_back(Generator<V>::Generate(i));
  }

  return vec;
}

// Select the first member of a pair.
template <class Pair>
struct Select1st {
  static const typename Pair::first_type& Get(const Pair& x) { return x.first; }
};

// Utility class to provide an accessor for a key given a value. The default
// behavior is to treat the value as a pair and return the first element.
template <typename K, typename V>
struct KeyOfValue {
  using type = Select1st<V>;
};

template <typename T>
struct Identity {
  static const T& Get(const T& t) { return t; }
};

// Partial specialization of KeyOfValue class for when the key and value are
// the same type such as in set<> and btree_set<>.
template <typename K>
struct KeyOfValue<K, K> {
  using type = Identity<K>;
};

}  // namespace platanus

#endif
