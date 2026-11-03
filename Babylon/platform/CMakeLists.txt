# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

babylon_add_module(platform
    SOURCES src/Window.cpp src/NullPlatform.cpp backends/sdl/SdlPlatform.cpp
    DEPS log
    LIBS ${BABYLON_SDL3_TARGET})
babylon_add_module_tests(platform
    DEPS log
    SOURCES tests/PlatformContractTest.cpp tests/NullPlatformTest.cpp)
if (TARGET babylon_platform_tests)
    # SDL's offscreen video driver needs no display, so the tests run in CI and over SSH.
    set_tests_properties(babylon.platform PROPERTIES ENVIRONMENT "SDL_VIDEO_DRIVER=offscreen")
endif()
