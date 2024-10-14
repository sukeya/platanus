# Copyright 2013 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Copyright 2024 Yuya Asano <my_favorite_theory@yahoo.co.jp>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

add_library(util INTERFACE)
target_include_directories(util INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

function(add_btree_test target)
  add_executable(${target} ${target}.cc)
  target_compile_options(
    ${target}
    PUBLIC
      "$<${gcc_like_cxx}:-Wall;-Wextra>"
      "$<${msvc_cxx}:/Wall>"
  )
  target_link_libraries(${target} GTest::gtest_main platanus util)
endfunction()

add_btree_test(btree_test)
add_btree_test(btree_pmr_test)
