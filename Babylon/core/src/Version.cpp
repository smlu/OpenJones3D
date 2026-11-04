// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Version.h>

namespace babylon::core
{

    std::string_view versionString()
    {
        return BABYLON_VERSION_STRING;
    }

}
