// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace babylon::files
{

    // How a file is opened. Read: an existing file, reading only. Write:
    // creates a new file or truncates an existing one, writing only.
    // Append: creates a new file or writes at the end of an existing one,
    // writing only.
    enum class OpenMode
    {
        Read,
        Write,
        Append
    };

    // The point a seek is relative to.
    enum class SeekOrigin
    {
        Begin,
        Current,
        End
    };

    // One entry of a directory listing. The name has no directory part.
    struct DirectoryEntry
    {
        std::string name;
        bool isDirectory;
        // Last modification, in seconds since the Unix epoch; 0 when the
        // backend keeps no times.
        std::int64_t modifiedSeconds = 0;
    };

    // An open file, created by FileSystem::open and closed when destroyed.
    // Not thread-safe. Every failure logs one babylon::log::warning line
    // and returns an empty result; nothing throws.
    class File
    {
    public:
        // The interface is abstract, so only backends reach this. Copying a
        // handle is refused: it would duplicate the ownership of the one
        // open file.
        File() = default;
        virtual ~File() = default;

        File(const File&) = delete;
        File& operator=(const File&) = delete;

        // Reads up to buffer.size() bytes into buffer. Returns the number
        // read (0 at the end of the file), or nullopt on error. Reading a
        // file opened for Write or Append fails as an error.
        [[nodiscard]] virtual std::optional<std::size_t> read(std::span<std::byte> buffer) = 0;

        // Writes all bytes of data at the current position, or at the end
        // of the file for Append. Returns false and logs a warning on error
        // or on a file opened for Read; a failed write leaves the file
        // unchanged.
        [[nodiscard]] virtual bool write(std::span<const std::byte> data) = 0;

        // Moves the read or write position. Returns false and logs a
        // warning, position unchanged, if the result would be negative or,
        // on a file opened for Read, past the end of the file. Seeking past
        // the end is allowed for Write and Append files.
        [[nodiscard]] virtual bool seek(std::int64_t offset, SeekOrigin origin) = 0;

        // The current position in bytes from the start of the file, or
        // nullopt and a warning if the backend cannot report it.
        virtual std::optional<std::int64_t> tell() const = 0;

        // The length in bytes, or nullopt and a warning if the backend
        // cannot report it.
        virtual std::optional<std::int64_t> size() const = 0;
    };

    // Access to one file system: files, directory listing, the program's
    // base directory and a per-user writable directory (ADR-0019). Paths
    // are UTF-8 with '/' separators and must satisfy isValidPath(); backends
    // translate them to the host form. Failures return an empty result and
    // log one babylon::log::warning line; nothing throws. Not thread-safe
    // unless a backend says so.
    class FileSystem
    {
    public:
        virtual ~FileSystem() = default;

        // Opens a file. Returns nullptr and logs a warning if the path is
        // invalid, names a directory, or the file cannot be opened (missing
        // for Read, or a missing parent directory for Write and Append).
        // The caller owns the file; it must not outlive the FileSystem.
        [[nodiscard]] virtual std::unique_ptr<File> open(std::string_view path, OpenMode mode) = 0;

        // True when a file exists at the path, or a directory when the path
        // ends with '/'. False for invalid paths and for missing paths,
        // without logging.
        virtual bool exists(std::string_view path) = 0;

        // Creates a directory and the missing parent directories. True if
        // it exists afterwards, also when it already existed; false and one
        // warning if the path is invalid or a file stands at or above it.
        [[nodiscard]] virtual bool createDirectory(std::string_view path) = 0;

        // Deletes a file or an empty directory. True if the path is gone
        // afterwards, also if it never existed; false and one warning if
        // the path is invalid or the path stays, such as a non-empty
        // directory or, on backends that refuse it, a file still open.
        [[nodiscard]] virtual bool remove(std::string_view path) = 0;

        // The entries of a directory, sorted by name (byte order), without
        // "." and "..". nullopt and a warning if the path is invalid or
        // isn't a readable directory.
        virtual std::optional<std::vector<DirectoryEntry>> listDirectory(std::string_view path) = 0;

        // The directory the program was started from (where the executable
        // is), ending with '/'. Empty string and a warning if the backend
        // cannot determine it.
        virtual std::string basePath() = 0;

        // The directory relative paths resolve against (the process working
        // directory for host backends), absolute, ending with '/'. Empty
        // string and a warning if the backend cannot determine it or it is
        // not a valid path.
        virtual std::string currentPath() = 0;

        // A per-user writable directory for this organization and
        // application, created if missing, ending with '/'. Empty string
        // and a warning on failure, or if either name is empty, contains
        // '/' or '\\', is "." or "..", or holds a NUL byte: each name must
        // be one usable path segment, the same way in every backend.
        virtual std::string userPath(std::string_view organization, std::string_view application) = 0;
    };

}
