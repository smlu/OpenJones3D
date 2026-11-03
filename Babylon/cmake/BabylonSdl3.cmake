# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# SDL3 for Babylon (ADR-0017): fetched through FetchContent, pinned to an exact
# release tag, never a branch. FIND_PACKAGE_ARGS makes CMake use an installed
# SDL3 of that version or newer instead of fetching, when one is found. SDL3
# builds C code, so the project enables the C language.
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.18
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS 3.4.18 CONFIG)
FetchContent_MakeAvailable(SDL3)

# The one name Babylon links against: a static FetchContent build provides the
# namespaced alias SDL3::SDL3-static, an installed SDL3 at least SDL3::SDL3.
# Modules list ${BABYLON_SDL3_TARGET} in LIBS and allowed-deps.txt gives it
# exactly as the graphviz output resolves it.
if (TARGET SDL3::SDL3-static)
    set(BABYLON_SDL3_TARGET SDL3::SDL3-static)
elseif (TARGET SDL3::SDL3)
    set(BABYLON_SDL3_TARGET SDL3::SDL3)
else()
    message(FATAL_ERROR "SDL3 was made available but names no link target (expected SDL3::SDL3-static or SDL3::SDL3)")
endif()
