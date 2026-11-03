# SPDX-License-Identifier: MPL-2.0
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

babylon_add_module(log SOURCES src/Log.cpp src/StderrSink.cpp src/CaptureSink.cpp)
babylon_add_module_tests(log SOURCES tests/LogTest.cpp tests/CaptureSinkTest.cpp)
