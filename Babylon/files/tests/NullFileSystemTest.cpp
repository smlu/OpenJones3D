// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/files/FileSystem.h>
#include <babylon/files/NullFileSystem.h>

#include <babylon/log/CaptureSink.h>
#include <babylon/log/Log.h>
#include <babylon/log/Sink.h>

#include <doctest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace
{
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
}

TEST_CASE("addFile seeds files that open() reads back exactly")
{
    babylon::files::NullFileSystem fs;
    babylon::log::CaptureSink sink;

    REQUIRE(fs.addFile("seed/data.bin", toBytes("seeded content")));
    CHECK(sink.records().empty());
    CHECK(fs.exists("seed/"));
    CHECK(fs.exists("seed"));

    const std::unique_ptr<babylon::files::File> file =
        fs.open("seed/data.bin", babylon::files::OpenMode::Read);
    REQUIRE(file != nullptr);
    CHECK(file->size() == 14);
    std::vector<std::byte> buffer(14);
    REQUIRE(file->read(buffer) == std::optional<std::size_t>(14));
    CHECK(fromBytes(buffer) == "seeded content");
    CHECK(sink.records().empty());

    // addFile replaces: the new bytes are the whole file.
    REQUIRE(fs.addFile("/seed/data.bin", toBytes("new")));
    const std::unique_ptr<babylon::files::File> again =
        fs.open("seed/data.bin", babylon::files::OpenMode::Read);
    REQUIRE(again != nullptr);
    CHECK(again->size() == 3);
    std::vector<std::byte> small(3);
    REQUIRE(again->read(small) == std::optional<std::size_t>(3));
    CHECK(fromBytes(small) == "new");
    CHECK(sink.records().empty());

    // An invalid path and a directory standing at the path each fail with
    // exactly one warning.
    CHECK_FALSE(fs.addFile("a//b", toBytes("x")));
    REQUIRE(sink.records().size() == 1);
    CHECK(sink.records()[0].level == babylon::log::Level::Warning);
    sink.clear();
    CHECK_FALSE(fs.addFile("seed", toBytes("x")));
    REQUIRE(sink.records().size() == 1);
    CHECK(sink.records()[0].level == babylon::log::Level::Warning);
    CHECK_FALSE(fs.exists("no-such-file.bin"));
}

TEST_CASE("The Null base path and user path match the documented layout")
{
    babylon::files::NullFileSystem fs;
    babylon::log::CaptureSink sink;

    CHECK(fs.basePath() == "/base/");
    CHECK(fs.exists("/base/"));
    CHECK(fs.currentPath() == "/");

    CHECK(fs.userPath("Aster Studio", "Field Trip") == "/user/Aster Studio/Field Trip/");
    CHECK(fs.userPath("Aster Studio", "Field Trip") == "/user/Aster Studio/Field Trip/");
    CHECK(fs.userPath("aster studio", "field trip") == "/user/aster studio/field trip/");
    CHECK(fs.exists("/user/Aster Studio/Field Trip/"));
    CHECK(fs.exists("/user/Aster Studio"));
    CHECK(fs.exists("/user"));
    CHECK(sink.records().empty());
}

TEST_CASE("userPath rejects names that are not one path segment")
{
    babylon::files::NullFileSystem fs;
    babylon::log::CaptureSink sink;

    CHECK(fs.userPath("", "Field Trip").empty());
    CHECK(fs.userPath("Aster", "").empty());
    CHECK(fs.userPath("As/ter", "Field Trip").empty());
    CHECK(fs.userPath("Aster", "Field/Trip").empty());
    CHECK(fs.userPath("As\\ter", "Field Trip").empty());
    CHECK(fs.userPath(".", "Field Trip").empty());
    CHECK(fs.userPath("Aster", "..").empty());
    REQUIRE(sink.records().size() == 7);
    for ( const babylon::log::CapturedRecord& record : sink.records() )
    {
        CHECK(record.level == babylon::log::Level::Warning);
    }
    CHECK_FALSE(fs.exists("/user/."));
    CHECK_FALSE(fs.exists("/user/Aster/.."));
}

TEST_CASE("Two Null file handles share one stored file")
{
    babylon::files::NullFileSystem fs;
    babylon::log::CaptureSink sink;

    // Two read handles of the same file work independently.
    REQUIRE(fs.addFile("shared.txt", toBytes("abcd")));
    const std::unique_ptr<babylon::files::File> first =
        fs.open("shared.txt", babylon::files::OpenMode::Read);
    const std::unique_ptr<babylon::files::File> second =
        fs.open("shared.txt", babylon::files::OpenMode::Read);
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    std::vector<std::byte> buffer(2);
    REQUIRE(first->read(buffer) == std::optional<std::size_t>(2));
    CHECK(fromBytes(buffer) == "ab");
    REQUIRE(second->read(buffer) == std::optional<std::size_t>(2));   // its own position
    CHECK(fromBytes(buffer) == "ab");

    // A write through one handle is visible to the other once the write
    // returns, without closing anything.
    REQUIRE(fs.addFile("live.txt", toBytes("old")));
    const std::unique_ptr<babylon::files::File> reader =
        fs.open("live.txt", babylon::files::OpenMode::Read);
    const std::unique_ptr<babylon::files::File> appender =
        fs.open("live.txt", babylon::files::OpenMode::Append);
    REQUIRE(reader != nullptr);
    REQUIRE(appender != nullptr);
    REQUIRE(appender->write(toBytes(" more")));   // "old" + " more" = 8 bytes
    std::vector<std::byte> all(9);
    REQUIRE(reader->read(all) == std::optional<std::size_t>(8));
    CHECK(fromBytes(std::span<std::byte>(all).first(8)) == "old more");
    CHECK(reader->size() == 8);
    CHECK(sink.records().empty());
}

TEST_CASE("NullFileSystem refuses to remove a file that is open")
{
    babylon::files::NullFileSystem fs;
    babylon::log::CaptureSink sink;

    REQUIRE(fs.addFile("locked.txt", toBytes("data")));
    std::unique_ptr<babylon::files::File> file =
        fs.open("locked.txt", babylon::files::OpenMode::Read);
    REQUIRE(file != nullptr);
    sink.clear();

    CHECK_FALSE(fs.remove("locked.txt"));
    REQUIRE(sink.records().size() == 1);
    CHECK(sink.records()[0].level == babylon::log::Level::Warning);
    CHECK(fs.exists("locked.txt"));
    sink.clear();

    file.reset();
    CHECK(sink.records().empty());   // closing a Null file logs nothing
    CHECK(fs.remove("locked.txt"));
    CHECK(sink.records().empty());
    CHECK_FALSE(fs.exists("locked.txt"));
}

TEST_CASE("The Null tree is separate from the host file system")
{
    babylon::files::NullFileSystem fs;
    babylon::log::CaptureSink sink;

    REQUIRE(fs.addFile("host-check.txt", toBytes("in memory only")));
    CHECK_FALSE(std::filesystem::exists(std::filesystem::path("host-check.txt")));
    CHECK(sink.records().empty());
    REQUIRE(fs.createDirectory("/nested/deep/dir/"));
    CHECK(fs.exists("/nested/deep/dir"));
    CHECK_FALSE(std::filesystem::exists(std::filesystem::path("nested")));
}
