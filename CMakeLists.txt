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
# Copyright 2024- Yuya Asano <my_favorite_theory@yahoo.co.jp>
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

option(build_tests "Build B-tree tests" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(platanus INTERFACE)

target_compile_features(platanus INTERFACE cxx_std_20)
target_include_directories(platanus INTERFACE
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/platanus>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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

if(BUILD_TESTS)
  enable_testing()

  find_package(GTest REQUIRED)
  find_package(benchmark REQUIRED)

  set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
  set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

  add_subdirectory(test)
  add_subdirectory(benchmark)
endif()
