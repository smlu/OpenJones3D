// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Version.h>

#include <doctest.h>

TEST_CASE("The version constants are the configured Babylon project version")
{
    CHECK(babylon::core::kVersionMajor == 0);
    CHECK(babylon::core::kVersionMinor == 1);
    CHECK(babylon::core::kVersionPatch == 0);
}

TEST_CASE("versionString() returns the dotted version")
{
    CHECK(babylon::core::versionString() == "0.1.0");
}
