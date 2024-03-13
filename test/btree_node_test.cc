#include "gtest/gtest.h"

#include <functional>
#include <memory>
#include <string>

#include "cppbtree/btree_node.h"
#include "cppbtree/btree_param.h"

namespace cppbtree {

template <typename K, int N>
void SetTest() {
  using node_type = btree_node<btree_set_params<K, std::less<K>, std::allocator<K>, N>>;

  auto alloc = typename node_type::allocator_type{};
  auto node = node_type{false, nullptr, alloc};
  for (int i = 0; i < node.max_children_count(); i++) {
    EXPECT_EQ(node.borrow_readonly_child(i), nullptr);
  }
}

template <typename K, int N>
void MapTest() {
  using node_type = btree_node<btree_map_params<K, K, std::less<K>, std::allocator<K>, N>>;

  auto alloc = typename node_type::allocator_type{};
  auto node = node_type{false, nullptr, alloc};
  for (int i = 0; i < node.max_children_count(); i++) {
    EXPECT_EQ(node.borrow_readonly_child(i), nullptr);
  }
}

TEST(Btree_node, set_int32_32) { SetTest<int32_t, 32>(); }
/*
TEST(Btree_node, set_int32_64) { SetTest<int32_t, 64>(); }
TEST(Btree_node, set_int32_128) { SetTest<int32_t, 128>(); }
TEST(Btree_node, set_int32_256) { SetTest<int32_t, 256>(); }
TEST(Btree_node, set_int64_256) { SetTest<int64_t, 256>(); }
TEST(Btree_node, set_string_256) { SetTest<std::string, 256>(); }
TEST(Btree_node, set_pair_256) { SetTest<std::pair<int, int>, 256>(); }
TEST(Btree_node, map_int32_256) { MapTest<int32_t, 256>(); }
TEST(Btree_node, map_int64_256) { MapTest<int64_t, 256>(); }
TEST(Btree_node, map_string_256) { MapTest<std::string, 256>(); }
TEST(Btree_node, map_pair_256) { MapTest<std::pair<int, int>, 256>(); }
*/

// Large-node tests
/*
TEST(Btree_node, map_int32_1024) { MapTest<int32_t, 1024>(); }
TEST(Btree_node, map_int32_1032) { MapTest<int32_t, 1032>(); }
TEST(Btree_node, map_int32_1040) { MapTest<int32_t, 1040>(); }
TEST(Btree_node, map_int32_1048) { MapTest<int32_t, 1048>(); }
TEST(Btree_node, map_int32_1056) { MapTest<int32_t, 1056>(); }

TEST(Btree_node, map_int32_2048) { MapTest<int32_t, 2048>(); }
TEST(Btree_node, map_int32_4096) { MapTest<int32_t, 4096>(); }
TEST(Btree_node, set_int32_1024) { SetTest<int32_t, 1024>(); }
TEST(Btree_node, set_int32_2048) { SetTest<int32_t, 2048>(); }
TEST(Btree_node, set_int32_4096) { SetTest<int32_t, 4096>(); }
TEST(Btree_node, map_string_1024) { MapTest<std::string, 1024>(); }
TEST(Btree_node, map_string_2048) { MapTest<std::string, 2048>(); }
TEST(Btree_node, map_string_4096) { MapTest<std::string, 4096>(); }
TEST(Btree_node, set_string_1024) { SetTest<std::string, 1024>(); }
TEST(Btree_node, set_string_2048) { SetTest<std::string, 2048>(); }
TEST(Btree_node, set_string_4096) { SetTest<std::string, 4096>(); }
*/

}