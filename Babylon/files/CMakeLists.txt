# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

babylon_add_module(files
    SOURCES src/Path.cpp src/NullFileSystem.cpp backends/sdl/SdlFileSystem.cpp
    DEPS log
    LIBS ${BABYLON_SDL3_TARGET})
babylon_add_module_tests(files
    DEPS log
    SOURCES tests/FileSystemContractTest.cpp tests/NullFileSystemTest.cpp tests/PathTest.cpp)
