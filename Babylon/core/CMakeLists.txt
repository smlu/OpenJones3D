# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# The version header is generated from the project() version so that Babylon's
# own numbers are used even when it builds inside the repository.
configure_file(include/babylon/core/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/babylon/core/Version.h @ONLY)

babylon_add_module(core SOURCES src/Assert.cpp src/Version.cpp)
target_include_directories(babylon_core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)

babylon_add_module_tests(core SOURCES tests/AssertTest.cpp tests/VersionTest.cpp)
