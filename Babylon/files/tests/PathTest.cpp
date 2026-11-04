// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/files/Path.h>

#include <doctest.h>

#include <cstddef>
#include <string>
#include <string_view>

TEST_CASE("isValidPath accepts ordinary paths and rejects the empty path")
{
    CHECK(babylon::files::isValidPath("data/level1.gob"));
    CHECK(babylon::files::isValidPath("cog"));
    CHECK_FALSE(babylon::files::isValidPath(""));
}

TEST_CASE("isValidPath accepts kMaxPathLength bytes and rejects one more")
{
    CHECK(babylon::files::isValidPath(std::string(babylon::files::kMaxPathLength, 'a')));
    CHECK_FALSE(babylon::files::isValidPath(std::string(babylon::files::kMaxPathLength + 1, 'a')));
}

TEST_CASE("isValidPath rejects NUL and backslash bytes")
{
    CHECK(babylon::files::isValidPath("save/game.nds"));
    CHECK_FALSE(babylon::files::isValidPath(std::string_view("mo\0del", 6)));
    CHECK_FALSE(babylon::files::isValidPath("game\\data"));
}

TEST_CASE("isValidPath allows only a leading and a trailing '/' as empty segment")
{
    CHECK(babylon::files::isValidPath("/base/data"));
    CHECK(babylon::files::isValidPath("data/"));
    CHECK(babylon::files::isValidPath("/"));
    CHECK_FALSE(babylon::files::isValidPath("a//b"));
    CHECK_FALSE(babylon::files::isValidPath("//a"));
    CHECK_FALSE(babylon::files::isValidPath("a//"));
}

TEST_CASE("isValidPath rejects '.' and '..' segments but not dotted names")
{
    CHECK(babylon::files::isValidPath("cog/main.cog"));
    CHECK(babylon::files::isValidPath("..a/b"));
    CHECK(babylon::files::isValidPath("a/..."));
    CHECK_FALSE(babylon::files::isValidPath("../outside"));
    CHECK_FALSE(babylon::files::isValidPath("a/../b"));
    CHECK_FALSE(babylon::files::isValidPath("a/."));
    CHECK_FALSE(babylon::files::isValidPath("."));
    CHECK_FALSE(babylon::files::isValidPath(".."));
    CHECK_FALSE(babylon::files::isValidPath("/.."));
}

TEST_CASE("joinPath puts exactly one separator between the parts")
{
    CHECK(babylon::files::joinPath("data", "save.nds") == "data/save.nds");
    CHECK(babylon::files::joinPath("data/", "save.nds") == "data/save.nds");
    CHECK(babylon::files::joinPath("/base/", "cog/script.cog") == "/base/cog/script.cog");
    CHECK(babylon::files::joinPath("/", "etc") == "/etc");
    CHECK(babylon::files::joinPath("/base/", "") == "");   // an empty part is invalid
}

TEST_CASE("joinPath returns an empty string for an invalid part")
{
    CHECK(babylon::files::joinPath("data", "save.nds") == "data/save.nds");   // control
    CHECK(babylon::files::joinPath("", "a").empty());
    CHECK(babylon::files::joinPath("a//b", "c").empty());
    CHECK(babylon::files::joinPath("data", "..").empty());
    CHECK(babylon::files::joinPath("data", "x\\y").empty());
    CHECK(babylon::files::joinPath("data", "").empty());
}

TEST_CASE("joinPath refuses an absolute second part")
{
    CHECK(!babylon::files::joinPath("data", "relative/x").empty());   // control
    CHECK(babylon::files::joinPath("data", "/etc/passwd").empty());
    CHECK(babylon::files::joinPath("data", "/x").empty());
}

TEST_CASE("joinPath refuses a result longer than kMaxPathLength")
{
    const std::string base = std::string(babylon::files::kMaxPathLength - 2, 'a');
    CHECK(babylon::files::joinPath(base, "z") == base + "/z");   // exactly the limit
    const std::string tooLong = std::string(babylon::files::kMaxPathLength - 1, 'a');
    CHECK(babylon::files::joinPath(tooLong, "z").empty());
}
