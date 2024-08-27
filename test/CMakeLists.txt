add_library(util util.cc)
target_link_libraries(util GTest::gtest_main gflags)

add_executable(btree_test btree_test.cc)
target_link_libraries(btree_test gflags platanus util)
