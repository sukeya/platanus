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
# Copyright 2024-2025 Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

cmake_minimum_required(VERSION 3.27.7)

project(platanus VERSION 1.1.0 LANGUAGES CXX)

option(PLATANUS_BUILD_TESTS "Build B-tree tests" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(platanus INTERFACE)

target_compile_features(platanus INTERFACE cxx_std_20)
target_include_directories(platanus INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/platanus>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

install(TARGETS platanus
  EXPORT platanusTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/platanus
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT platanusTargets
  FILE platanusTargets.cmake
  NAMESPACE platanus::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/platanus
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/platanusConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/platanus"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/platanusConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/platanusConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/platanusConfigVersion.cmake
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/platanus
)

export(EXPORT platanusTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/platanusTargets.cmake"
  NAMESPACE platanus::
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(gcc "$<COMPILE_LANG_AND_ID:CXX,GNU>")
set(clang "$<COMPILE_LANG_AND_ID:CXX,Clang>")
set(msvc "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

set(gcc_like "$<OR:${gcc},${clang}>")

if(PLATANUS_BUILD_TESTS)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
    OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(util)
  add_subdirectory(test)
endif()

if(PLATANUS_BUILD_BENCHMARK)
  include(FetchContent)
  set(BENCHMARK_ENABLE_TESTING OFF)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        eddb0241389718a23a42db6af5f0164b6e0139af # v1.9.4
    OVERRIDE_FIND_PACKAGE
  )
  FetchContent_MakeAvailable(benchmark)

  add_subdirectory(util)
  add_subdirectory(benchmark)
endif()
