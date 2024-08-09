add_executable(btree_bench btree_bench.cc)
target_link_libraries(btree_bench gflags GTest::gtest_main platanus btree_test_flags util)
if (VALUES_SIZE_TEST)
  target_compile_definitions(btree_bench PRIVATE VALUES_SIZE_TEST)
endif()

add_executable(stl_bench stl_bench.cc stl_test_flags.cc)
target_link_libraries(stl_bench gflags GTest::gtest_main platanus util)