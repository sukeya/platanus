add_library(btree_test_flags btree_test_flags.cc)
target_link_libraries(btree_test_flags gflags)

add_library(util util.cc)
target_link_libraries(util GTest::gtest_main gflags btree_test_flags)

set(tests 
  btree_test
  safe_btree_test
)

foreach(test ${tests})
    add_executable(${test} ${test}.cc)
    target_link_libraries(${test} gflags platanus util)
endforeach()
