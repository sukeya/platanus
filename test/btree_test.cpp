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

#include "gtest/gtest.h"
#include "platanus/btree_map.hpp"
#include "platanus/btree_set.hpp"
#include "btree_test.hpp"

namespace platanus {

TEST(Nullptr, access) { int i = *std::nullptr; }

template <typename K, int N>
void SetTest() {
  using TestAlloc = TestAllocator<K>;
  BtreeTest<btree_set<K, std::ranges::less, std::allocator<K>, N>, std::set<K>>();
  BtreeAllocatorTest<btree_set<K, std::ranges::less, TestAlloc, N>>();
}

template <typename K, int N>
void MapTest() {
  using TestAlloc = TestAllocator<K>;
  BtreeTest<btree_map<K, K, std::ranges::less, std::allocator<K>, N>, std::map<K, K>>();
  BtreeAllocatorTest<btree_map<K, K, std::ranges::less, TestAlloc, N>>();
  BtreeMapTest<btree_map<K, K, std::ranges::less, std::allocator<K>, N>>();
}

template <typename K, int N>
void MultiSetTest() {
  using TestAlloc = TestAllocator<K>;
  BtreeMultiTest<btree_multiset<K, std::ranges::less, std::allocator<K>, N>, std::multiset<K>>();
  BtreeAllocatorTest<btree_multiset<K, std::ranges::less, TestAlloc, N>>();
}

template <typename K, int N>
void MultiMapTest() {
  using TestAlloc = TestAllocator<K>;
  BtreeMultiTest<
      btree_multimap<K, K, std::ranges::less, std::allocator<K>, N>,
      std::multimap<K, K>>();
  BtreeMultiMapTest<btree_multimap<K, K, std::ranges::less, std::allocator<K>, N>>();
  BtreeAllocatorTest<btree_multimap<K, K, std::ranges::less, TestAlloc, N>>();
}

BTREE_TEST(SetTest, Set)
BTREE_TEST(MapTest, Map)
BTREE_TEST(MultiSetTest, MultiSet)
BTREE_TEST(MultiMapTest, MultiMap)

TEST(Btree, SwapKeyCompare) {
  using SubstringSet = btree_set<std::string, SubstringLess>;
  SubstringSet s1(SubstringLess(1), SubstringSet::allocator_type());
  SubstringSet s2(SubstringLess(2), SubstringSet::allocator_type());

  ASSERT_TRUE(s1.insert("a").second);
  ASSERT_FALSE(s1.insert("aa").second);

  ASSERT_TRUE(s2.insert("a").second);
  ASSERT_TRUE(s2.insert("aa").second);
  ASSERT_FALSE(s2.insert("aaa").second);

  swap(s1, s2);

  ASSERT_TRUE(s1.insert("b").second);
  ASSERT_TRUE(s1.insert("bb").second);
  ASSERT_FALSE(s1.insert("bbb").second);

  ASSERT_TRUE(s2.insert("b").second);
  ASSERT_FALSE(s2.insert("bb").second);
}

TEST(Btree, UpperBoundRegression) {
  // Regress a bug where upper_bound would default-construct a new key_compare
  // instead of copying the existing one.
  using SubstringSet = btree_set<std::string, SubstringLess>;
  SubstringSet my_set(SubstringLess(3));
  my_set.insert("aab");
  my_set.insert("abb");
  // We call upper_bound("aaa").  If this correctly uses the length 3
  // comparator, aaa < aab < abb, so we should get aab as the result.
  // If it instead uses the default-constructed length 2 comparator,
  // aa == aa < ab, so we'll get abb as our result.
  SubstringSet::iterator it = my_set.upper_bound("aaa");
  ASSERT_TRUE(it != my_set.end());
  EXPECT_EQ("aab", *it);
}

TEST(Btree, Comparison) {
  const int          kSetSize = 1201;
  btree_set<int64_t> my_set;
  for (int i = 0; i < kSetSize; ++i) {
    my_set.insert(i);
  }
  btree_set<int64_t> my_set_copy(my_set);
  EXPECT_TRUE(my_set_copy == my_set);
  EXPECT_TRUE(my_set == my_set_copy);
  EXPECT_FALSE(my_set_copy != my_set);
  EXPECT_FALSE(my_set != my_set_copy);

  my_set.insert(kSetSize);
  EXPECT_FALSE(my_set_copy == my_set);
  EXPECT_FALSE(my_set == my_set_copy);
  EXPECT_TRUE(my_set_copy != my_set);
  EXPECT_TRUE(my_set != my_set_copy);

  my_set.erase(kSetSize - 1);
  EXPECT_FALSE(my_set_copy == my_set);
  EXPECT_FALSE(my_set == my_set_copy);
  EXPECT_TRUE(my_set_copy != my_set);
  EXPECT_TRUE(my_set != my_set_copy);

  btree_map<std::string, int64_t> my_map;
  for (int i = 0; i < kSetSize; ++i) {
    my_map[std::string(i, 'a')] = i;
  }
  btree_map<std::string, int64_t> my_map_copy(my_map);
  EXPECT_TRUE(my_map_copy == my_map);
  EXPECT_TRUE(my_map == my_map_copy);
  EXPECT_FALSE(my_map_copy != my_map);
  EXPECT_FALSE(my_map != my_map_copy);

  ++my_map_copy[std::string(7, 'a')];
  EXPECT_FALSE(my_map_copy == my_map);
  EXPECT_FALSE(my_map == my_map_copy);
  EXPECT_TRUE(my_map_copy != my_map);
  EXPECT_TRUE(my_map != my_map_copy);

  my_map_copy     = my_map;
  my_map["hello"] = kSetSize;
  EXPECT_FALSE(my_map_copy == my_map);
  EXPECT_FALSE(my_map == my_map_copy);
  EXPECT_TRUE(my_map_copy != my_map);
  EXPECT_TRUE(my_map != my_map_copy);

  my_map.erase(std::string(kSetSize - 1, 'a'));
  EXPECT_FALSE(my_map_copy == my_map);
  EXPECT_FALSE(my_map == my_map_copy);
  EXPECT_TRUE(my_map_copy != my_map);
  EXPECT_TRUE(my_map != my_map_copy);
}

TEST(Btree, RangeCtorSanity) {
  using test_set  = btree_set<int, std::ranges::less, std::allocator<int>>;
  using test_map  = btree_map<int, int, std::ranges::less, std::allocator<int>>;
  using test_mset = btree_multiset<int, std::ranges::less, std::allocator<int>>;
  using test_mmap = btree_multimap<int, int, std::ranges::less, std::allocator<int>>;
  std::vector<int> ivec;
  ivec.push_back(1);
  std::map<int, int> imap;
  imap.insert(std::make_pair(1, 2));
  test_mset tmset(ivec.begin(), ivec.end());
  test_mmap tmmap(imap.begin(), imap.end());
  test_set  tset(ivec.begin(), ivec.end());
  test_map  tmap(imap.begin(), imap.end());
  EXPECT_EQ(1, tmset.size());
  EXPECT_EQ(1, tmmap.size());
  EXPECT_EQ(1, tset.size());
  EXPECT_EQ(1, tmap.size());
}

// Test custom comparer.
TEST(Btree, set_vec2i_64) {
  static constexpr int N = 64;

  using K = Vec2i;

  BtreeTest<btree_set<K, Vec2iComp, std::allocator<K>, N>, std::set<K, Vec2iComp>>();
}

TEST(Btree, map_vec2i_64) {
  static constexpr int N = 64;

  using K = Vec2i;

  BtreeTest<btree_map<K, K, Vec2iComp, std::allocator<K>, N>, std::map<K, K, Vec2iComp>>();
  BtreeMapTest<btree_map<K, K, Vec2iComp, std::allocator<K>, N>>();
}

TEST(Btree, multiset_vec2i_64) {
  static constexpr int N = 64;

  using K = Vec2i;

  BtreeMultiTest<btree_multiset<K, Vec2iComp, std::allocator<K>, N>, std::multiset<K, Vec2iComp>>();
}

TEST(Btree, multimap_vec2i_64) {
  static constexpr int N = 64;

  using K = Vec2i;

  BtreeMultiTest<
      btree_multimap<K, K, Vec2iComp, std::allocator<K>, N>,
      std::multimap<K, K, Vec2iComp>>();
  BtreeMultiMapTest<btree_multimap<K, K, Vec2iComp, std::allocator<K>, N>>();
}

}  // namespace platanus
