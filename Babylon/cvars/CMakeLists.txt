# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

babylon_add_module(cvars
    SOURCES src/Registry.cpp
    DEPS log core)
babylon_add_module_tests(cvars
    DEPS log core
    SOURCES tests/RegistryTest.cpp)
