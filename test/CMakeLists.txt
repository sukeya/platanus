set(tests 
  btree_test
  safe_btree_test
)

foreach(test ${tests})
    add_executable(${test} ${test}.cc btree_test_flags.cc)
    target_link_libraries(${test} GTest::gtest_main gflags platanus)
endforeach()
