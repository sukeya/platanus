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

cmake_minimum_required(VERSION 3.27.7)

project(cppbtree VERSION 1.1.0 LANGUAGES CXX)

option(build_tests "Build B-tree tests" OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(cppbtree INTERFACE)
add_library(cppbtree::cppbtree ALIAS cppbtree)

target_compile_features(cppbtree INTERFACE cxx_std_20)
target_include_directories(cppbtree INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/cppbtree>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(TARGETS cppbtree
  EXPORT cppbtreeTargets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(
  DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/cppbtree
  DESTINATION
    ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT cppbtreeTargets
  FILE cppbtreeTargets.cmake
  NAMESPACE cppbtree::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cppbtree
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/cppbtreeConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/cppbtree"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cppbtreeConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cppbtreeConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cppbtreeConfigVersion.cmake
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/cppbtree
)

export(EXPORT cppbtreeTargets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/cppbtreeTargets.cmake"
  NAMESPACE cppbtree::
)

if(build_tests)
  enable_testing()

  find_package(GTest REQUIRED)
  find_package(gflags REQUIRED)

  add_subdirectory(test)
  add_subdirectory(benchmark)
endif()
