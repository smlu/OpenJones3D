# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Vendored test framework: doctest (MIT), used by the Babylon module tests.
# The include directory is added as SYSTEM so the vendored header does not get
# Babylon's own warning flags or -Werror.

add_library(babylon_doctest INTERFACE)
target_include_directories(babylon_doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# One shared doctest main for every babylon_<module>_tests executable.
# Warnings and -Werror are not applied to this target.
add_library(babylon_doctest_main STATIC DoctestMain.cpp)
target_compile_features(babylon_doctest_main PRIVATE cxx_std_20)
set_target_properties(babylon_doctest_main PROPERTIES CXX_EXTENSIONS OFF FOLDER Babylon)
target_link_libraries(babylon_doctest_main PUBLIC babylon_doctest)
