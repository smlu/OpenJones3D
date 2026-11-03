// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/files/FileSystem.h>

#include <memory>

namespace babylon::files
{

    // FileSystem on the host file system through SDL3 (ADR-0017): files
    // over SDL_IOFromFile, the tree over SDL_GetPathInfo,
    // SDL_CreateDirectory, SDL_RemovePath and SDL_EnumerateDirectory, base
    // and user path over SDL_GetBasePath and SDL_GetPrefPath (ADR-0019).
    // '/'-separated paths are translated to the native form inside the
    // backend; relative paths resolve against the process working
    // directory, as they do on the host. Case is passed through
    // unchanged. Needs no SDL_Init. The returned file system is owned by
    // the caller, and its files must not outlive it.
    std::unique_ptr<FileSystem> createSdlFileSystem();

}
