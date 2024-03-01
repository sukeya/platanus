add_executable(btree_test btree_test.cc btree_test_flags.cc)
add_executable(safe_btree_test safe_btree_test.cc btree_test_flags.cc)
target_link_libraries(btree_test GTest::gtest_main gflags cppbtree)
target_link_libraries(safe_btree_test GTest::gtest_main gflags cppbtree)