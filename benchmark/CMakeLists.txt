cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH PARENT_DIR)

add_executable(btree_bench btree_bench.cc ${PARENT_DIR}/test/btree_test_flags.cc)
target_link_libraries(btree_bench gflags GTest::gtest_main cppbtree)
if (NODESIZE_TEST)
  target_compile_definitions(btree_bench PRIVATE NODESIZE_TESTING)
endif()