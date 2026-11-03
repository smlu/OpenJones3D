// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/files/FileSystem.h>
#include <babylon/files/NullFileSystem.h>
#include <babylon/files/Path.h>
#include <babylon/files/SdlFileSystem.h>

#include <babylon/log/CaptureSink.h>
#include <babylon/log/Log.h>
#include <babylon/log/Sink.h>

#include <doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    using babylon::files::DirectoryEntry;
    using babylon::files::File;
    using babylon::files::FileSystem;
    using babylon::files::OpenMode;
    using babylon::files::SeekOrigin;
    using babylon::log::CaptureSink;
    using babylon::log::Level;

    std::vector<std::byte> toBytes(const char* text)
    {
        std::vector<std::byte> bytes;
        for ( const char* p = text; *p != '\0'; ++p )
        {
            bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(*p)));
        }
        return bytes;
    }

    std::string fromBytes(std::span<const std::byte> bytes)
    {
        std::string text;
        text.reserve(bytes.size());
        for ( const std::byte one : bytes )
        {
            text.push_back(static_cast<char>(static_cast<unsigned char>(one)));
        }
        return text;
    }

    // The FileSystem contract, run through the interface against one
    // backend. Every failure promised by the headers is exercised: exactly
    // one babylon::log::warning line per failed call, and nothing logged on
    // the success paths. `root` is the '/'-separated path of a directory
    // that does not exist yet; the helper creates it with createDirectory.
    // It must not call userPath() on a backend that would touch a real
    // user profile.
    void checkFileSystemContract(FileSystem& fs, std::string root)
    {
        CaptureSink sink;
        const auto noWarning = [&sink]()
        {
            CHECK(sink.records().empty());
            sink.clear();
        };
        const auto oneWarning = [&sink]()
        {
            REQUIRE(sink.records().size() == 1);
            CHECK(sink.records()[0].level == Level::Warning);
            sink.clear();
        };

        // createDirectory makes the root; asking again is true and quiet.
        CHECK(fs.createDirectory(root));
        noWarning();
        CHECK(fs.createDirectory(root));
        noWarning();

        // Write, then read back exact bytes; size() and tell() agree; a
        // read at the end returns 0 and is not an error.
        const std::string dataPath = babylon::files::joinPath(root, "data.bin");
        const std::vector<std::byte> payload = toBytes("babylon files contract payload");
        {
            const std::unique_ptr<File> file = fs.open(dataPath, OpenMode::Write);
            REQUIRE(file != nullptr);
            REQUIRE(file->write(payload));
        }
        noWarning();
        {
            const std::unique_ptr<File> file = fs.open(dataPath, OpenMode::Read);
            REQUIRE(file != nullptr);
            CHECK(file->size() == static_cast<std::int64_t>(payload.size()));
            CHECK(file->tell() == 0);

            std::vector<std::byte> buffer(payload.size());
            const std::optional<std::size_t> num = file->read(buffer);
            REQUIRE(num.has_value());
            CHECK(*num == payload.size());
            CHECK(fromBytes(buffer) == fromBytes(payload));
            CHECK(file->tell() == file->size());

            const std::optional<std::size_t> atEnd = file->read(buffer);
            REQUIRE(atEnd.has_value());
            CHECK(*atEnd == 0);
            noWarning();

            // Seek from Begin, Current and End.
            CHECK(file->seek(3, SeekOrigin::Begin));
            CHECK(file->tell() == 3);
            CHECK(file->seek(2, SeekOrigin::Current));
            CHECK(file->tell() == 5);
            CHECK(file->seek(-4, SeekOrigin::End));
            CHECK(file->tell() == static_cast<std::int64_t>(payload.size()) - 4);
            noWarning();

            // A negative result is refused, with the position unchanged.
            CHECK_FALSE(file->seek(-100000, SeekOrigin::Current));
            oneWarning();
            CHECK(file->tell() == static_cast<std::int64_t>(payload.size()) - 4);

            // A file opened for Read refuses a seek past its end.
            CHECK_FALSE(file->seek(static_cast<std::int64_t>(payload.size()) + 1,
                SeekOrigin::Begin));
            oneWarning();
            CHECK(file->tell() == static_cast<std::int64_t>(payload.size()) - 4);

            // The position the last accepted seek set is the position the
            // next read uses.
            CHECK(file->seek(0, SeekOrigin::Begin));
            std::vector<std::byte> first3(3);
            CHECK(file->read(first3) == std::optional<std::size_t>(3));
            CHECK(fromBytes(first3) == "bab");
            noWarning();
        }

        // Append mode appends.
        const std::string logPath = babylon::files::joinPath(root, "log.txt");
        {
            const std::unique_ptr<File> file = fs.open(logPath, OpenMode::Write);
            REQUIRE(file != nullptr);
            REQUIRE(file->write(toBytes("aaa")));
        }
        {
            const std::unique_ptr<File> file = fs.open(logPath, OpenMode::Append);
            REQUIRE(file != nullptr);
            REQUIRE(file->write(toBytes("bbb")));
        }
        {
            const std::unique_ptr<File> file = fs.open(logPath, OpenMode::Read);
            REQUIRE(file != nullptr);
            CHECK(file->size() == 6);
            std::vector<std::byte> buffer(6);
            REQUIRE(file->read(buffer) == std::optional<std::size_t>(6));
            CHECK(fromBytes(buffer) == "aaabbb");
        }
        noWarning();

        // Write mode truncates an existing file.
        {
            const std::unique_ptr<File> file = fs.open(logPath, OpenMode::Write);
            REQUIRE(file != nullptr);
            REQUIRE(file->write(toBytes("x")));
        }
        {
            const std::unique_ptr<File> file = fs.open(logPath, OpenMode::Read);
            REQUIRE(file != nullptr);
            CHECK(file->size() == 1);
            std::vector<std::byte> buffer(2);
            REQUIRE(file->read(buffer) == std::optional<std::size_t>(1));
            CHECK(fromBytes(std::span<std::byte>(buffer).first(1)) == "x");
        }
        noWarning();

        // exists() for a file, a directory, a missing path; invalid paths
        // are false without logging.
        CHECK(fs.exists(dataPath));
        CHECK(fs.exists(root));
        CHECK_FALSE(fs.exists(babylon::files::joinPath(root, "missing.txt")));
        CHECK_FALSE(fs.exists("a//b"));
        CHECK_FALSE(fs.exists(""));
        noWarning();

        // remove() of a file, of an empty directory and of a missing path.
        CHECK(fs.remove(logPath));
        CHECK_FALSE(fs.exists(logPath));
        const std::string subPath = babylon::files::joinPath(root, "sub");
        CHECK(fs.createDirectory(subPath));
        CHECK(fs.exists(subPath));
        CHECK(fs.remove(subPath));
        CHECK_FALSE(fs.exists(subPath));
        CHECK(fs.remove(babylon::files::joinPath(root, "never-existed.txt")));
        noWarning();

        // A non-empty directory is not removed: the path stays, false and
        // one warning.
        CHECK(fs.createDirectory(subPath));
        {
            const std::unique_ptr<File> file =
                fs.open(babylon::files::joinPath(subPath, "inside.txt"), OpenMode::Write);
            REQUIRE(file != nullptr);
        }
        noWarning();
        CHECK_FALSE(fs.remove(subPath));
        oneWarning();

        // listDirectory() returns the entries sorted by name in byte order
        // ('B' before 'a'), with the right isDirectory, and without "."
        // and "..".
        const std::string listPath = babylon::files::joinPath(root, "listing");
        CHECK(fs.createDirectory(listPath));
        CHECK(fs.createDirectory(babylon::files::joinPath(listPath, "zeta")));
        for ( const char* name : {"alpha.txt", "Beta.txt"} )
        {
            const std::unique_ptr<File> file =
                fs.open(babylon::files::joinPath(listPath, name), OpenMode::Write);
            REQUIRE(file != nullptr);
        }
        const std::optional<std::vector<DirectoryEntry>> listing = fs.listDirectory(listPath);
        REQUIRE(listing.has_value());
        noWarning();
        const std::vector<DirectoryEntry> expected = {
            DirectoryEntry{"Beta.txt", false},
            DirectoryEntry{"alpha.txt", false},
            DirectoryEntry{"zeta", true},
        };
        REQUIRE(listing->size() == expected.size());
        for ( std::size_t i = 0; i < expected.size(); ++i )
        {
            CHECK((*listing)[i].name == expected[i].name);
            CHECK((*listing)[i].isDirectory == expected[i].isDirectory);
        }

        // listDirectory() of a missing directory and of a file: nullopt
        // and one warning each.
        CHECK_FALSE(fs.listDirectory(babylon::files::joinPath(root, "no-such-dir")).has_value());
        oneWarning();
        CHECK_FALSE(fs.listDirectory(dataPath).has_value());
        oneWarning();

        // open() of a missing file for Read, of an invalid path, and of a
        // file whose parent directory is missing: nullptr and one warning.
        CHECK(fs.open(babylon::files::joinPath(root, "missing.txt"), OpenMode::Read) == nullptr);
        oneWarning();
        CHECK(fs.open("a//b", OpenMode::Read) == nullptr);
        oneWarning();
        CHECK(fs.open("../x", OpenMode::Write) == nullptr);
        oneWarning();
        CHECK(fs.open(babylon::files::joinPath(root, "missing-dir/deep.txt"),
            OpenMode::Write) == nullptr);
        oneWarning();

        // A read on a file opened for writing and a write on a file opened
        // for reading fail, each with one warning, and change nothing.
        const std::string modePath = babylon::files::joinPath(root, "mode.bin");
        {
            const std::unique_ptr<File> writer = fs.open(modePath, OpenMode::Write);
            REQUIRE(writer != nullptr);
            std::vector<std::byte> buffer(4);
            CHECK_FALSE(writer->read(buffer).has_value());
            oneWarning();
            REQUIRE(writer->write(toBytes("kept")));
            noWarning();
        }
        {
            const std::unique_ptr<File> reader = fs.open(modePath, OpenMode::Read);
            REQUIRE(reader != nullptr);
            CHECK_FALSE(reader->write(toBytes("nope")));
            oneWarning();
            std::vector<std::byte> buffer(4);
            REQUIRE(reader->read(buffer) == std::optional<std::size_t>(4));
            CHECK(fromBytes(buffer) == "kept");
            noWarning();
        }

        // The invalid paths above must not have touched the host tree.
        CHECK_FALSE(fs.exists("a//b"));
        CHECK_FALSE(fs.exists("../x"));
        noWarning();
        CHECK(fs.remove(modePath));
        noWarning();

        // basePath() is where the program was started: not empty and
        // ending with '/'.
        noWarning();
        const std::string base = fs.basePath();
        CHECK_FALSE(base.empty());
        CHECK(base.back() == '/');
        noWarning();
    }
}

TEST_CASE("The Null file system honors the FileSystem contract")
{
    babylon::files::NullFileSystem fs;
    checkFileSystemContract(fs, "/contract/");
}

TEST_CASE("The SDL file system honors the FileSystem contract")
{
    namespace stdfs = std::filesystem;

    // A fresh directory under the system temp directory, checked to not
    // exist yet so runs never share it, and removed again below.
    stdfs::path root;
    for ( int attempt = 0; attempt < 64 && root.empty(); ++attempt )
    {
        const stdfs::path candidate = stdfs::temp_directory_path()
            / ("babylon-files-contract-" + std::to_string(attempt));
        if ( !stdfs::exists(candidate) )
        {
            root = candidate;
        }
    }
    REQUIRE(!root.empty());

    // The interface speaks '/', so the root is handed over that way even
    // on hosts with another native separator.
    std::string rootString = root.string();
    std::replace(rootString.begin(), rootString.end(), '\\', '/');

    {
        const std::unique_ptr<FileSystem> fs = babylon::files::createSdlFileSystem();
        REQUIRE(fs != nullptr);
        checkFileSystemContract(*fs, rootString);
        // The contract must not leave files open beyond the helper.
    }

    std::error_code error;
    stdfs::remove_all(root, error);
    INFO("remove_all: " << error.message());
    CHECK(!stdfs::exists(root));
}
