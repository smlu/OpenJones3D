// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/files/Path.h>

namespace babylon::files
{

    bool isValidPath(std::string_view path)
    {
        if ( path.empty() || path.size() > kMaxPathLength )
        {
            return false;
        }
        if ( path.find('\0') != std::string_view::npos ||
            path.find('\\') != std::string_view::npos )
        {
            return false;
        }

        // Walk the '/'-separated segments, including the one that starts
        // after the final '/' so a trailing separator is seen. An empty
        // segment is allowed only as the leading one (an absolute path) or
        // as the trailing one (a directory path); any other empty segment
        // comes from a double separator. "." and ".." are refused: paths
        // reach the backend normalized, and a backend must not reinterpret
        // them.
        std::size_t segmentStart = 0;
        for ( std::size_t i = 0; i <= path.size(); ++i )
        {
            if ( i != path.size() && path[i] != '/' )
            {
                continue;
            }
            const std::string_view segment = path.substr(segmentStart, i - segmentStart);
            if ( segment.empty() )
            {
                if ( i != 0 && i != path.size() )
                {
                    return false;
                }
            }
            else if ( segment == "." || segment == ".." )
            {
                return false;
            }
            segmentStart = i + 1;
        }
        return true;
    }

    std::string joinPath(std::string_view base, std::string_view relative)
    {
        if ( !isValidPath(base) || !isValidPath(relative) )
        {
            return {};
        }
        // An absolute second part replaces the base; that is not a join.
        // Both parts are valid, hence not empty.
        if ( relative.front() == '/' )
        {
            return {};
        }
        std::string result(base);
        if ( result.back() != '/' )
        {
            result.push_back('/');
        }
        result.append(relative);
        if ( result.size() > kMaxPathLength )
        {
            return {};
        }
        return result;
    }

}
