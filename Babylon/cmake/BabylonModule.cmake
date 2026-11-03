# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Helpers that declare Babylon modules as CMake targets (ADR-0015).
#
#   babylon_add_module(<name>
#       SOURCES <files...>
#       [DEPS <babylon modules...>]          # private dependencies
#       [PUBLIC_DEPS <babylon modules...>])  # dependencies in this module's public headers
#
#   babylon_add_module_tests(<name>
#       SOURCES <files...>)
#
# A dependency must name a module already declared in an add_subdirectory added
# before the dependent module, which keeps the target graph acyclic.

# Warnings and sanitizer flags for Babylon's own targets. Vendored code never
# goes through this function, so it gets neither Babylon's warnings nor -Werror.
function(_babylon_target_settings target)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    if (BABYLON_SANITIZE)
        # BABYLON_SANITIZE is rejected on MSVC in Babylon/CMakeLists.txt.
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

# Links babylon_<dep> into babylon_<name> and checks the dependency is a known
# module. Called for every name in DEPS and PUBLIC_DEPS.
function(_babylon_link_dependency name dep visibility)
    if (dep STREQUAL name)
        message(FATAL_ERROR "babylon module ${name}: a module may not depend on itself")
    endif()
    if (NOT TARGET babylon_${dep})
        message(FATAL_ERROR "babylon module ${name}: unknown dependency ${dep}")
    endif()
    target_link_libraries(babylon_${name} ${visibility} babylon_${dep})
endfunction()

function(babylon_add_module name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;DEPS;PUBLIC_DEPS")
    if (arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "babylon_add_module(${name}): unknown arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if (NOT arg_SOURCES)
        message(FATAL_ERROR "babylon module ${name}: SOURCES must not be empty")
    endif()

    add_library(babylon_${name} STATIC ${arg_SOURCES})
    add_library(babylon::${name} ALIAS babylon_${name})

    target_include_directories(babylon_${name}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    foreach (dep IN LISTS arg_DEPS)
        _babylon_link_dependency(${name} ${dep} PRIVATE)
    endforeach()
    foreach (dep IN LISTS arg_PUBLIC_DEPS)
        _babylon_link_dependency(${name} ${dep} PUBLIC)
    endforeach()

    # Per-target, so the C++20 requirement never changes a parent project's globals.
    target_compile_features(babylon_${name} PUBLIC cxx_std_20)
    set_target_properties(babylon_${name} PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER Babylon)

    _babylon_target_settings(babylon_${name})
endfunction()

function(babylon_add_module_tests name)
    if (NOT BABYLON_BUILD_TESTS)
        return()
    endif()

    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES")
    if (arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "babylon_add_module_tests(${name}): unknown arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if (NOT arg_SOURCES)
        message(FATAL_ERROR "babylon module ${name}: test SOURCES must not be empty")
    endif()
    if (NOT TARGET babylon_${name})
        message(FATAL_ERROR "babylon_add_module_tests(${name}): no such module")
    endif()

    add_executable(babylon_${name}_tests ${arg_SOURCES})
    target_link_libraries(babylon_${name}_tests PRIVATE babylon_${name} babylon_doctest_main)
    set_target_properties(babylon_${name}_tests PROPERTIES
        CXX_EXTENSIONS OFF
        FOLDER Babylon)

    _babylon_target_settings(babylon_${name}_tests)

    add_test(NAME babylon.${name} COMMAND babylon_${name}_tests)
    set_property(TEST babylon.${name} PROPERTY LABELS "babylon;unit")
endfunction()
