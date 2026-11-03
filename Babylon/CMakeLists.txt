# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.24)

project(Babylon VERSION 0.1.0 LANGUAGES CXX)

# Babylon builds alone (cmake -S Babylon) and as a subdirectory of the Jones3D EX
# repository. C++20, warnings and sanitizers are set per target in
# cmake/BabylonModule.cmake; no variables of a parent project are touched.
if (PROJECT_IS_TOP_LEVEL)
    set(BABYLON_IS_TOP_LEVEL ON)
else()
    set(BABYLON_IS_TOP_LEVEL OFF)
endif()

option(BABYLON_BUILD_TESTS "Build the Babylon doctest unit tests" ON)
option(BABYLON_SANITIZE "Add address and undefined behavior sanitizer flags to Babylon targets" OFF)

# The sanitizer flags below are GCC and Clang options with no MSVC equivalent here,
# so MSVC builds reject the option instead of silently ignoring it.
if (BABYLON_SANITIZE AND MSVC)
    message(FATAL_ERROR "BABYLON_SANITIZE requires GCC or Clang")
endif()

# A parent project that adds Babylon as a subdirectory calls enable_testing() itself.
if (BABYLON_IS_TOP_LEVEL AND BABYLON_BUILD_TESTS)
    enable_testing()
endif()

include(cmake/BabylonModule.cmake)

if (BABYLON_BUILD_TESTS)
    add_subdirectory(third_party/doctest)
endif()

add_subdirectory(core)
