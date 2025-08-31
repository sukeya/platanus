#include "gtest/gtest.h"
#include "platanus/btree_map.hpp"
#include "platanus/btree_set.hpp"
#include "btree_test.hpp"

namespace platanus {
template <typename K, int N>
void SetTest() {
  BtreeTest<pmr::btree_set<K, std::ranges::less, N>, std::set<K>>();
}

template <typename K, int N>
void MapTest() {
  BtreeTest<pmr::btree_map<K, K, std::ranges::less, N>, std::map<K, K>>();
  BtreeMapTest<pmr::btree_map<K, K, std::ranges::less, N>>();
}

template <typename K, int N>
void MultiSetTest() {
  BtreeMultiTest<pmr::btree_multiset<K, std::ranges::less, N>, std::multiset<K>>();
}

template <typename K, int N>
void MultiMapTest() {
  BtreeMultiTest<pmr::btree_multimap<K, K, std::ranges::less, N>, std::multimap<K, K>>();
  BtreeMultiMapTest<pmr::btree_multimap<K, K, std::ranges::less, N>>();
}

BTREE_TEST(SetTest, Set)
BTREE_TEST(MapTest, Map)
BTREE_TEST(MultiSetTest, MultiSet)
BTREE_TEST(MultiMapTest, MultiMap)

TEST(node_factory, leaf_root_node) {
  auto node_factory = internal::btree_node_factory<
      internal::
          btree_set_params<int, std::ranges::less, platanus::pmr::polymorphic_allocator<>, 3>>();

  auto root = node_factory.make_root_node(true);
}

TEST(node_factory, internal_root_node) {
  auto node_factory = internal::btree_node_factory<
      internal::
          btree_set_params<int, std::ranges::less, platanus::pmr::polymorphic_allocator<>, 3>>();

  auto root = node_factory.make_root_node(false);
}

TEST(node_factory, leaf_node) {
  auto node_factory = internal::btree_node_factory<
      internal::
          btree_set_params<int, std::ranges::less, platanus::pmr::polymorphic_allocator<>, 3>>();

  auto root = node_factory.make_root_node(false);
  internal::set_child(root.get(), 0, node_factory.make_node(true, root.get()));
}

TEST(node_factory, internal_node) {
  auto node_factory = internal::btree_node_factory<
      internal::
          btree_set_params<int, std::ranges::less, platanus::pmr::polymorphic_allocator<>, 3>>();

  auto root = node_factory.make_root_node(false);
  internal::set_child(root.get(), 0, node_factory.make_node(false, root.get()));
}
}  // namespace platanus
