# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.24)

project(Babylon VERSION 0.1.0 LANGUAGES C CXX)

# Babylon builds alone (cmake -S Babylon) and as a subdirectory of a parent
# repository. C++20, warnings and sanitizers are set per target in
# cmake/BabylonModule.cmake; no variables of a parent project are touched.
# The C language is enabled because the fetched SDL3 builds C code.
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
include(cmake/BabylonSdl3.cmake)

if (BABYLON_BUILD_TESTS)
    add_subdirectory(third_party/doctest)
endif()

add_subdirectory(core)
add_subdirectory(log)
add_subdirectory(time)
add_subdirectory(files)
add_subdirectory(platform)
add_subdirectory(input)
add_subdirectory(cvars)

# Machine checks for the Babylon boundary and the module graph (ADR-0006,
# ADR-0008, ADR-0015). They inspect the source tree, so they carry the label
# check, not unit, and the graph check runs only in CI (it needs a configure
# with --graphviz).
if (BABYLON_BUILD_TESTS)
    find_package(Python3 COMPONENTS Interpreter)
    if (Python3_FOUND)
        add_test(NAME babylon.check.boundary
            COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_boundary.py ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME babylon.check.selftest
            COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_checks.py)
        set_tests_properties(babylon.check.boundary babylon.check.selftest
            PROPERTIES LABELS "babylon;check")
    else()
        message(STATUS "Python3 not found: skipping the Babylon boundary checks")
    endif()
endif()
