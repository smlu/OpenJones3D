// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/files/FileSystem.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace babylon::files
{

    // FileSystem that keeps the whole tree in memory and touches nothing on
    // the host, for tests and headless runs. Files and directories live
    // under the root '/'; relative paths resolve against '/'. Paths behave
    // as the interface describes, including validation, the byte-order sort
    // of listings and the one warning per failure. basePath() is "/base/"
    // and userPath(organization, application) is
    // "/user/<organization>/<application>/"; both exist as directories from
    // the moment they are asked for. Case is passed through unchanged, so
    // two spellings are two directories.
    //
    // Handles opened for the same path share the stored contents: a write
    // through one is visible to the others once it returns, and the handles
    // keep working while the file stays open. Removing a file that is open
    // is refused, as on a host a removal under an open handle would take
    // effect only after the last close.
    class NullFileSystem final : public FileSystem
    {
    public:
        NullFileSystem();

        std::unique_ptr<File> open(std::string_view path, OpenMode mode) override;
        bool exists(std::string_view path) override;
        bool createDirectory(std::string_view path) override;
        bool remove(std::string_view path) override;
        std::optional<std::vector<DirectoryEntry>> listDirectory(std::string_view path) override;
        std::string basePath() override;
        std::string userPath(std::string_view organization, std::string_view application) override;

        // Test and headless helper: creates or replaces a file with these
        // bytes, making missing parent directories. Returns false and logs
        // one warning for an invalid path or when a directory stands at or
        // above the path. Replacing is in place: handles that have the file
        // open see the new contents.
        [[nodiscard]] bool addFile(std::string_view path, std::span<const std::byte> contents);

    private:
        // Keys are absolute paths: "/a/b" for a file, and for a directory
        // the same path with a trailing '/', the root being "/".
        std::map<std::string, std::shared_ptr<std::vector<std::byte>>> m_files;
        std::set<std::string> m_directories;
    };

}
