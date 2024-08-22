add_library(btree_test_flags btree_test_flags.cc)
target_link_libraries(btree_test_flags gflags)

add_library(util util.cc)
target_link_libraries(util GTest::gtest_main gflags btree_test_flags)

add_executable(btree_test btree_test.cc)
target_link_libraries(btree_test gflags platanus util)
