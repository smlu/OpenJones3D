# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

babylon_add_module(time
    SOURCES src/NullClock.cpp backends/sdl/SdlClock.cpp
    LIBS ${BABYLON_SDL3_TARGET})
babylon_add_module_tests(time SOURCES tests/ClockContractTest.cpp tests/NullClockTest.cpp)
