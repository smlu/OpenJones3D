// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/Window.h>

#include <babylon/log/Log.h>

namespace babylon::platform
{

    bool isValidWindowSize(Size size)
    {
        return size.width >= 1 && size.width <= kMaxWindowDimension
            && size.height >= 1 && size.height <= kMaxWindowDimension;
    }

    bool Window::resize(Size size)
    {
        // The one check every backend shares, so no backend has to repeat it
        // and no backend can be looser than another.
        if ( !isValidWindowSize(size) )
        {
            log::warning("platform: rejected window size {}x{}: both dimensions "
                "must be within 1 and {}", size.width, size.height, kMaxWindowDimension);
            return false;
        }
        return doResize(size);
    }

}
