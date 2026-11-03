// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace babylon::files
{

    // Paths in the files interface are UTF-8 and use '/' as their separator
    // on every platform; the backend translates. Relative paths resolve
    // against the process working directory (SDL backend) or the root '/'
    // (Null backend). Case is passed through unchanged: whether "A.TXT"
    // finds "a.txt" is up to the host file system.

    // Longest path, in bytes, the files interface accepts. Both backends
    // check every path against it before touching a device.
    inline constexpr std::size_t kMaxPathLength = 4096;

    // True when the path is usable: not empty, at most kMaxPathLength bytes,
    // no NUL byte, no backslash (callers use '/'), no empty segment except a
    // leading '/' (so "a//b" is rejected), no "." or ".." segment. A
    // trailing '/' is allowed; directory paths, such as the ones
    // FileSystem::basePath() returns, end with one.
    bool isValidPath(std::string_view path);

    // Joins two valid paths with exactly one '/' between them. Returns an
    // empty string if either is invalid, if the result is longer than
    // kMaxPathLength or if `relative` starts with '/'.
    std::string joinPath(std::string_view base, std::string_view relative);

}
