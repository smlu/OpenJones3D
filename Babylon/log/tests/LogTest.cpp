// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/log/Log.h>
#include <babylon/log/StderrSink.h>

#include <doctest.h>

#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace
{

    // Sets the minimum level for one test case and restores the previous
    // level on scope exit, so every case leaves the default Level::Info and
    // each test passes alone and in any order.
    class ScopedLevel
    {
    public:
        explicit ScopedLevel(babylon::log::Level level = babylon::log::Level::Info) :
            m_previous(babylon::log::minimumLevel())
        {
            babylon::log::setMinimumLevel(level);
        }

        ~ScopedLevel()
        {
            babylon::log::setMinimumLevel(m_previous);
        }

        ScopedLevel(const ScopedLevel&) = delete;
        ScopedLevel& operator=(const ScopedLevel&) = delete;

    private:
        babylon::log::Level m_previous;
    };

    // Attaches a sink for the scope and detaches it on scope exit, so even a
    // failing REQUIRE cannot leave the sink attached or dangling. Declare it
    // after the sink it attaches.
    class ScopedSink
    {
    public:
        explicit ScopedSink(babylon::log::Sink& sink) :
            m_sink(sink)
        {
            babylon::log::attachSink(sink);
        }

        ~ScopedSink()
        {
            babylon::log::detachSink(m_sink);
        }

        ScopedSink(const ScopedSink&) = delete;
        ScopedSink& operator=(const ScopedSink&) = delete;

    private:
        babylon::log::Sink& m_sink;
    };

    // Bumped whenever Counted is turned into a string, so a test can show
    // that a dropped record never reaches std::format.
    int formattedCount = 0;

    // Test argument type with a counting formatter below.
    struct Counted
    {
    };

    // Sink that appends "<tag> <level> <message>" to a shared vector, so the
    // tests observe arrival, content and attach order. Does not attach
    // itself: the test controls the attach order.
    class RecordingSink final : public babylon::log::Sink
    {
    public:
        RecordingSink(std::string_view tag, std::vector<std::string>& out) :
            m_tag(tag), m_out(out)
        {
        }

        void write(const babylon::log::Record& record) override
        {
            m_out.push_back(std::format("{} {} {}",
                m_tag, babylon::log::levelName(record.level), record.message));
        }

    private:
        std::string_view m_tag;
        std::vector<std::string>& m_out;
    };

}

// std::formatter customization for Counted, in std as usual.
template <>
struct std::formatter<Counted>
{
    constexpr auto parse(std::format_parse_context& context)
    {
        return context.begin();
    }

    auto format(const Counted&, std::format_context& context) const
    {
        ++formattedCount;
        return std::format_to(context.out(), "counted");
    }
};

TEST_CASE("levelName names every level in lowercase")
{
    CHECK(babylon::log::levelName(babylon::log::Level::Trace) == "trace");
    CHECK(babylon::log::levelName(babylon::log::Level::Debug) == "debug");
    CHECK(babylon::log::levelName(babylon::log::Level::Info) == "info");
    CHECK(babylon::log::levelName(babylon::log::Level::Warning) == "warning");
    CHECK(babylon::log::levelName(babylon::log::Level::Error) == "error");
}

TEST_CASE("The default minimum level is Info")
{
    // Holds alone and in any order because every test case that changes the
    // level does it through ScopedLevel, which restores the default.
    CHECK(babylon::log::minimumLevel() == babylon::log::Level::Info);
}

TEST_CASE("With no sink attached nothing is enabled and writing is safe")
{
    ScopedLevel level;
    CHECK_FALSE(babylon::log::isEnabled(babylon::log::Level::Error));
    CHECK_FALSE(babylon::log::isEnabled(babylon::log::Level::Trace));
    CHECK_NOTHROW(babylon::log::error("dropped {}", 1));
    CHECK_NOTHROW(babylon::log::write(babylon::log::Level::Error, "dropped directly"));
}

TEST_CASE("A record reaches an attached sink with its level and text")
{
    ScopedLevel level;
    std::vector<std::string> out;
    RecordingSink sink("sink", out);
    ScopedSink attach(sink);

    CHECK(babylon::log::isEnabled(babylon::log::Level::Info));
    babylon::log::write(babylon::log::Level::Warning, "direct record");
    babylon::log::info("{} of {}", 3, 7);

    REQUIRE(out.size() == 2);
    CHECK(out[0] == "sink warning direct record");
    CHECK(out[1] == "sink info 3 of 7");
}

TEST_CASE("A record below the minimum is dropped and its arguments are not formatted")
{
    ScopedLevel level;
    std::vector<std::string> out;
    RecordingSink sink("sink", out);
    ScopedSink attach(sink);

    formattedCount = 0;
    babylon::log::debug("{}", Counted());
    CHECK_FALSE(babylon::log::isEnabled(babylon::log::Level::Debug));
    CHECK(formattedCount == 0);
    CHECK(out.empty());

    // The same call at the minimum does format: the counter is not inert.
    babylon::log::info("{}", Counted());
    CHECK(formattedCount == 1);
    REQUIRE(out.size() == 1);
    CHECK(out[0] == "sink info counted");
}

TEST_CASE("Two sinks receive the record in attach order")
{
    ScopedLevel level;
    std::vector<std::string> out;
    RecordingSink first("first", out);
    RecordingSink second("second", out);
    ScopedSink attachFirst(first);
    ScopedSink attachSecond(second);

    babylon::log::error("boom");

    REQUIRE(out.size() == 2);
    CHECK(out[0] == "first error boom");
    CHECK(out[1] == "second error boom");
}

TEST_CASE("Attaching the same sink twice delivers once and detaching is forgiving")
{
    ScopedLevel level;
    std::vector<std::string> out;
    RecordingSink sink("sink", out);
    RecordingSink stranger("stranger", out);
    ScopedSink attach(sink);

    babylon::log::attachSink(sink);   // already attached: does nothing
    babylon::log::error("once");
    REQUIRE(out.size() == 1);

    babylon::log::detachSink(sink);
    babylon::log::detachSink(sink);   // not attached any more: does nothing
    babylon::log::detachSink(stranger); // never attached: does nothing
    babylon::log::error("gone");
    CHECK(out.size() == 1);
    CHECK_FALSE(babylon::log::isEnabled(babylon::log::Level::Error));
}

TEST_CASE("The minimum level lets and blocks levels")
{
    std::vector<std::string> out;
    RecordingSink sink("sink", out);
    ScopedSink attach(sink);

    {
        ScopedLevel trace(babylon::log::Level::Trace);
        CHECK(babylon::log::isEnabled(babylon::log::Level::Trace));
        babylon::log::trace("visible");
        REQUIRE(out.size() == 1);
        CHECK(out[0] == "sink trace visible");
    }

    out.clear();
    {
        ScopedLevel errorsOnly(babylon::log::Level::Error);
        CHECK_FALSE(babylon::log::isEnabled(babylon::log::Level::Warning));
        babylon::log::warning("blocked");
        CHECK(out.empty());
        babylon::log::error("passes");
        REQUIRE(out.size() == 1);
        CHECK(out[0] == "sink error passes");
    }
}

TEST_CASE("StderrSink can be attached, writes a record and detaches (smoke)")
{
    // The line "[warning] StderrSink smoke record" lands in the test log;
    // checking stderr content is not part of this suite.
    ScopedLevel level;
    babylon::log::StderrSink sink;
    ScopedSink attach(sink);
    babylon::log::warning("StderrSink smoke record");
}
