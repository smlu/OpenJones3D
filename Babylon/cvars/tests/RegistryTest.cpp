// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/cvars/Registry.h>

#include <babylon/core/Parse.h>
#include <babylon/log/CaptureSink.h>
#include <babylon/log/Log.h>
#include <babylon/log/Sink.h>

#include <doctest.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using babylon::core::parseDouble;
    using babylon::cvars::Desc;
    using babylon::cvars::Flag;
    using babylon::cvars::hasFlag;
    using babylon::cvars::isValidName;
    using babylon::cvars::Registry;
    using babylon::cvars::SetResult;
    using babylon::cvars::Source;
    using babylon::cvars::toString;
    using babylon::cvars::Value;

    Desc makeDesc(std::string_view name, Value defaultValue, Flag flags = Flag::None)
    {
        Desc desc;
        desc.name = std::string(name);
        desc.defaultValue = std::move(defaultValue);
        desc.flags = flags;
        return desc;
    }

    // Checks that exactly one warning arrived since the last clear.
    void checkOneWarning(babylon::log::CaptureSink& sink)
    {
        REQUIRE(sink.records().size() == 1);
        CHECK(sink.records()[0].level == babylon::log::Level::Warning);
        sink.clear();
    }
}

TEST_CASE("isValidName accepts lowercase parts and rejects the broken forms")
{
    CHECK(isValidName("physics.fixed_step"));
    CHECK(isValidName("r_mode"));
    CHECK(isValidName("a"));
    CHECK(isValidName("a0.b_c9"));
    CHECK(isValidName(std::string(64, 'a')));

    CHECK_FALSE(isValidName(""));
    CHECK_FALSE(isValidName(std::string(65, 'a')));
    CHECK_FALSE(isValidName("R_mode"));
    CHECK_FALSE(isValidName("1st"));
    CHECK_FALSE(isValidName("_x"));
    CHECK_FALSE(isValidName("mode.1two"));
    CHECK_FALSE(isValidName(".a"));
    CHECK_FALSE(isValidName("a."));
    CHECK_FALSE(isValidName("a..b"));
    CHECK_FALSE(isValidName("mode-b"));
    CHECK_FALSE(isValidName("mode b"));
    CHECK_FALSE(isValidName("mode\n"));
}

TEST_CASE("Flag operators combine sets and query single bits")
{
    const Flag saved = Flag::Saved | Flag::ReadOnly;
    CHECK(hasFlag(saved, Flag::Saved));
    CHECK(hasFlag(saved, Flag::ReadOnly));
    CHECK_FALSE(hasFlag(saved, Flag::Simulation));
    CHECK_FALSE(hasFlag(saved, Flag::StartupOnly));
    CHECK((saved & Flag::Simulation) == Flag::None);
    CHECK((saved & Flag::Saved) == Flag::Saved);
    CHECK(hasFlag(Flag::None, Flag::Saved) == false);
}

TEST_CASE("toString is canonical and every form parses back exactly")
{
    CHECK(toString(Value{true}) == "true");
    CHECK(toString(Value{false}) == "false");
    CHECK(toString(Value{std::int64_t{-42}}) == "-42");
    CHECK(toString(Value{std::int64_t{0}}) == "0");
    CHECK(toString(Value{std::string("hello world")}) == "hello world");
    CHECK(toString(Value{0.1}) == "0.1");
    CHECK(toString(Value{1e300}) == "1e+300");
    CHECK(toString(Value{-0.0}) == "-0");

    // Each text re-reads as the identical value, -0.0 including its sign.
    const std::optional<double> tenth = parseDouble(toString(Value{0.1}));
    REQUIRE(tenth.has_value());
    CHECK(*tenth == 0.1);
    const std::optional<double> huge = parseDouble(toString(Value{1e300}));
    REQUIRE(huge.has_value());
    CHECK(*huge == 1e300);
    const std::optional<double> zero = parseDouble(toString(Value{-0.0}));
    REQUIRE(zero.has_value());
    CHECK(*zero == 0.0);
    CHECK(std::signbit(*zero));
}

TEST_CASE("add registers valid cvars and refuses each broken one with one warning")
{
    Registry registry;
    babylon::log::CaptureSink sink;

    Desc mode;
    mode.name = "r_mode";
    mode.description = "render mode";
    mode.defaultValue = Value{std::int64_t{3}};
    mode.flags = Flag::Saved | Flag::Simulation;
    mode.min = 0.0;
    mode.max = 4.0;
    REQUIRE(registry.add(mode));
    CHECK(sink.records().empty());
    CHECK(registry.contains("r_mode"));
    const Desc* stored = registry.desc("r_mode");
    REQUIRE(stored != nullptr);
    CHECK(stored->name == "r_mode");
    CHECK(stored->description == "render mode");
    CHECK(stored->min == std::optional<double>{0.0});
    CHECK(stored->max == std::optional<double>{4.0});
    CHECK(registry.flags("r_mode") == std::optional<Flag>{Flag::Saved | Flag::Simulation});

    REQUIRE(registry.set("r_mode", Value{std::int64_t{1}}, Source::User) == SetResult::Ok);
    sink.clear();

    // A refused add never replaces the cvar already registered.
    CHECK_FALSE(registry.add(makeDesc("r_mode", Value{std::int64_t{2}})));
    checkOneWarning(sink);
    CHECK(registry.getInt("r_mode") == std::optional<std::int64_t>{1});

    CHECK_FALSE(registry.add(makeDesc("R_mode", Value{true})));
    checkOneWarning(sink);
    CHECK_FALSE(registry.add(makeDesc("mode.", Value{true})));
    checkOneWarning(sink);

    Desc boolRange = makeDesc("flag", Value{true});
    boolRange.min = 0.0;
    CHECK_FALSE(registry.add(boolRange));
    checkOneWarning(sink);
    Desc stringMax = makeDesc("who", Value{std::string("x")});
    stringMax.max = 10.0;
    CHECK_FALSE(registry.add(stringMax));
    checkOneWarning(sink);

    Desc flipped = makeDesc("bad.bounds", Value{std::int64_t{5}});
    flipped.min = 10.0;
    flipped.max = 1.0;
    CHECK_FALSE(registry.add(flipped));
    checkOneWarning(sink);

    Desc outsideDefault = makeDesc("bad.default", Value{std::int64_t{50}});
    outsideDefault.min = 0.0;
    outsideDefault.max = 10.0;
    CHECK_FALSE(registry.add(outsideDefault));
    checkOneWarning(sink);

    CHECK_FALSE(registry.add(makeDesc("nan.default", Value{std::nan("")})));
    checkOneWarning(sink);

    Desc nanBound = makeDesc("nan.min", Value{std::int64_t{0}});
    nanBound.min = std::nan("");
    CHECK_FALSE(registry.add(nanBound));
    checkOneWarning(sink);

    Desc infBound = makeDesc("inf.max", Value{0.0});
    infBound.max = std::numeric_limits<double>::infinity();
    CHECK_FALSE(registry.add(infBound));
    checkOneWarning(sink);

    // An int default beyond exact double range cannot be range-checked,
    // so with a range it is refused; without one it is accepted.
    Desc lostDefault = makeDesc("huge.range", Value{(std::int64_t{1} << 53) + 1});
    lostDefault.min = 0.0;
    CHECK_FALSE(registry.add(lostDefault));
    checkOneWarning(sink);
    REQUIRE(registry.add(makeDesc("huge.free", Value{(std::int64_t{1} << 53) + 1})));
    CHECK(sink.records().empty());

    // None of the refused adds left a cvar behind, and lookups log nothing.
    CHECK_FALSE(registry.contains("bad.default"));
    CHECK_FALSE(registry.contains("nan.min"));
    CHECK_FALSE(registry.contains("huge.range"));
    CHECK(registry.desc("bad.default") == nullptr);
    CHECK(sink.records().empty());
}

TEST_CASE("Lookups return the right type and nothing else, without logging")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("b", Value{true}, Flag::Saved)));
    REQUIRE(registry.add(makeDesc("i", Value{std::int64_t{-42}})));
    REQUIRE(registry.add(makeDesc("f", Value{0.5})));
    REQUIRE(registry.add(makeDesc("s", Value{std::string("text")})));

    CHECK(registry.getBool("b") == std::optional<bool>{true});
    CHECK(registry.getInt("i") == std::optional<std::int64_t>{-42});
    CHECK(registry.getFloat("f") == std::optional<double>{0.5});
    CHECK(registry.getString("s") == std::optional<std::string>{"text"});
    CHECK(registry.get("i") == std::optional<Value>{Value{std::int64_t{-42}}});
    CHECK(registry.flags("b") == std::optional<Flag>{Flag::Saved});

    CHECK_FALSE(registry.getInt("b").has_value());
    CHECK_FALSE(registry.getBool("i").has_value());
    CHECK_FALSE(registry.getFloat("i").has_value());
    CHECK_FALSE(registry.getString("f").has_value());
    CHECK_FALSE(registry.contains("nope"));
    CHECK_FALSE(registry.get("nope").has_value());
    CHECK_FALSE(registry.flags("nope").has_value());
    CHECK(registry.desc("nope") == nullptr);
    CHECK(sink.records().empty());
}

TEST_CASE("set stores exact types, checks ranges inclusively and refuses the rest")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("gain", Value{0.5})));
    Desc bounded = makeDesc("count", Value{std::int64_t{1}});
    bounded.min = 1.0;
    bounded.max = 10.0;
    REQUIRE(registry.add(bounded));
    Desc ratio = makeDesc("ratio", Value{0.0});
    ratio.min = 0.0;
    ratio.max = 1.0;
    REQUIRE(registry.add(ratio));
    REQUIRE(registry.add(makeDesc("big", Value{std::int64_t{0}})));

    CHECK(registry.set("gain", Value{1.25}, Source::Program) == SetResult::Ok);
    CHECK(registry.getFloat("gain") == std::optional<double>{1.25});
    CHECK(sink.records().empty());

    // Non-finite doubles are refused on a float cvar even without a
    // range, each with exactly one warning and the value untouched.
    CHECK(registry.set("gain", Value{std::nan("")}, Source::Program) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.set("gain", Value{std::numeric_limits<double>::infinity()},
        Source::Program) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.set("gain", Value{-std::numeric_limits<double>::infinity()},
        Source::Program) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.getFloat("gain") == std::optional<double>{1.25});
    CHECK(sink.records().empty());

    // The alternative must match the cvar's type exactly, no conversions.
    CHECK(registry.set("gain", Value{std::int64_t{1}}, Source::Program) == SetResult::WrongType);
    checkOneWarning(sink);
    CHECK(registry.set("count", Value{1.0}, Source::Program) == SetResult::WrongType);
    checkOneWarning(sink);
    CHECK(registry.set("count", Value{true}, Source::Program) == SetResult::WrongType);
    checkOneWarning(sink);
    CHECK(registry.getInt("count") == std::optional<std::int64_t>{1});
    CHECK(registry.set("nope", Value{std::int64_t{1}}, Source::Program) == SetResult::UnknownName);
    checkOneWarning(sink);

    // Bounds are inclusive; refusals are refused, not clamped.
    CHECK(registry.set("count", Value{std::int64_t{10}}, Source::User) == SetResult::Ok);
    CHECK(registry.set("count", Value{std::int64_t{11}}, Source::User) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.getInt("count") == std::optional<std::int64_t>{10});
    CHECK(registry.set("count", Value{std::int64_t{0}}, Source::User) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.getInt("count") == std::optional<std::int64_t>{10});

    // A non-finite double never passes a set range.
    CHECK(registry.set("ratio", Value{std::nan("")}, Source::Program) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.set("ratio", Value{std::numeric_limits<double>::infinity()},
        Source::Program) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.getFloat("ratio") == std::optional<double>{0.0});

    // An integer outside exact double range cannot be range-checked, so it
    // is out of range; without a range it is stored exactly.
    CHECK(registry.set("count", Value{(std::int64_t{1} << 53) + 1}, Source::User)
        == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.set("big", Value{std::int64_t{1} << 62}, Source::User) == SetResult::Ok);
    CHECK(registry.getInt("big") == std::optional<std::int64_t>{std::int64_t{1} << 62});
    CHECK(sink.records().empty());
}

TEST_CASE("ReadOnly refuses the user and takes the program")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("engine.build", Value{std::int64_t{5}}, Flag::ReadOnly)));

    CHECK(registry.set("engine.build", Value{std::int64_t{6}}, Source::User)
        == SetResult::ReadOnly);
    checkOneWarning(sink);
    CHECK(registry.getInt("engine.build") == std::optional<std::int64_t>{5});
    CHECK(registry.set("engine.build", Value{std::int64_t{6}}, Source::Program) == SetResult::Ok);
    CHECK(registry.getInt("engine.build") == std::optional<std::int64_t>{6});

    // setFromString parses first; a valid text still hits the flag.
    CHECK(registry.setFromString("engine.build", "7", Source::User) == SetResult::ReadOnly);
    checkOneWarning(sink);
    CHECK(registry.getInt("engine.build") == std::optional<std::int64_t>{6});

    // A refused source is refused even when the value would not change.
    CHECK(registry.set("engine.build", Value{std::int64_t{6}}, Source::User)
        == SetResult::ReadOnly);
    checkOneWarning(sink);

    CHECK(registry.reset("engine.build", Source::User) == SetResult::ReadOnly);
    checkOneWarning(sink);
    CHECK(registry.reset("engine.build", Source::Program) == SetResult::Ok);
    CHECK(registry.getInt("engine.build") == std::optional<std::int64_t>{5});
    CHECK(sink.records().empty());
}

TEST_CASE("StartupOnly is open before finishStartup and fixed after")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("startup.only", Value{true}, Flag::StartupOnly)));
    REQUIRE(registry.add(makeDesc("runtime", Value{std::int64_t{1}})));

    // During startup both sources may set it.
    CHECK(registry.set("startup.only", Value{false}, Source::User) == SetResult::Ok);
    CHECK(registry.set("startup.only", Value{true}, Source::Program) == SetResult::Ok);
    CHECK(registry.setFromString("startup.only", "false", Source::User) == SetResult::Ok);
    CHECK(registry.getBool("startup.only") == std::optional<bool>{false});
    CHECK(sink.records().empty());

    registry.finishStartup();

    CHECK(registry.set("startup.only", Value{true}, Source::Program) == SetResult::StartupOnly);
    checkOneWarning(sink);
    CHECK(registry.set("startup.only", Value{true}, Source::User) == SetResult::StartupOnly);
    checkOneWarning(sink);
    CHECK(registry.setFromString("startup.only", "true", Source::Program)
        == SetResult::StartupOnly);
    checkOneWarning(sink);
    CHECK(registry.reset("startup.only", Source::Program) == SetResult::StartupOnly);
    checkOneWarning(sink);
    CHECK(registry.getBool("startup.only") == std::optional<bool>{false});

    // The flag belongs to the cvar only; other cvars keep working.
    CHECK(registry.set("runtime", Value{std::int64_t{2}}, Source::User) == SetResult::Ok);
    registry.finishStartup();   // idempotent
    CHECK(registry.set("runtime", Value{std::int64_t{3}}, Source::User) == SetResult::Ok);
    CHECK(sink.records().empty());
}

TEST_CASE("Unchanged sets return Unchanged, run no callback and log nothing")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("mode", Value{std::int64_t{2}}, Flag::Simulation)));

    int calls = 0;
    registry.onChange("mode", [&calls](std::string_view, const Value&) { ++calls; });

    CHECK(registry.set("mode", Value{std::int64_t{2}}, Source::User) == SetResult::Unchanged);
    CHECK(registry.setFromString("mode", "2", Source::User) == SetResult::Unchanged);
    CHECK(registry.reset("mode", Source::User) == SetResult::Unchanged);
    CHECK(calls == 0);
    CHECK(sink.records().empty());

    CHECK(registry.set("mode", Value{std::int64_t{3}}, Source::User) == SetResult::Ok);
    CHECK(calls == 1);
    CHECK(sink.records().empty());
}

TEST_CASE("Callbacks run in add order and removals drop them")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("a", Value{std::int64_t{0}}, Flag::Simulation)));

    std::vector<std::string> calls;
    const Registry::CallbackId id1 = registry.onChange("a",
        [&calls](std::string_view name, const Value& value)
        {
            calls.push_back(std::string(name) + "=" + toString(value));
        });
    const Registry::CallbackId id2 = registry.onChange("a",
        [&calls](std::string_view, const Value&) { calls.push_back("second"); });
    const Registry::CallbackId id3 = registry.onChange("a",
        [&calls](std::string_view, const Value&) { calls.push_back("third"); });
    CHECK(id1 != 0);
    CHECK(id2 != 0);
    CHECK(id1 != id2);
    CHECK(id2 != id3);
    CHECK(sink.records().empty());

    CHECK(registry.set("a", Value{std::int64_t{1}}, Source::Program) == SetResult::Ok);
    CHECK(calls == std::vector<std::string>{"a=1", "second", "third"});
    CHECK(sink.records().empty());

    registry.removeCallback(id2);
    calls.clear();
    CHECK(registry.set("a", Value{std::int64_t{2}}, Source::Program) == SetResult::Ok);
    CHECK(calls == std::vector<std::string>{"a=2", "third"});

    // Unknown ids, including 0, are ignored and log nothing.
    registry.removeCallback(0);
    registry.removeCallback(123456789);
    registry.removeCallback(id2);   // already removed
    CHECK(sink.records().empty());
}

TEST_CASE("A callback removed while the change dispatches is not called by it")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("a", Value{std::int64_t{0}})));

    std::vector<std::string> calls;
    Registry::CallbackId second = 0;
    registry.onChange("a", [&calls, &registry, &second](std::string_view, const Value&)
    {
        calls.push_back("first");
        registry.removeCallback(second);
    });
    second = registry.onChange("a",
        [&calls](std::string_view, const Value&) { calls.push_back("second"); });

    CHECK(registry.set("a", Value{std::int64_t{1}}, Source::Program) == SetResult::Ok);
    CHECK(calls == std::vector<std::string>{"first"});
    CHECK(sink.records().empty());
}

TEST_CASE("A callback added during a callback starts with the next change")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("a", Value{std::int64_t{0}})));

    std::vector<std::string> calls;
    bool added = false;
    registry.onChange("a", [&registry, &calls, &added](std::string_view, const Value&)
    {
        calls.push_back("first");
        if ( !added )
        {
            added = true;
            registry.onChange("a",
                [&calls](std::string_view, const Value&) { calls.push_back("late"); });
        }
    });

    CHECK(registry.set("a", Value{std::int64_t{1}}, Source::Program) == SetResult::Ok);
    CHECK(calls == std::vector<std::string>{"first"});
    calls.clear();
    CHECK(registry.set("a", Value{std::int64_t{2}}, Source::Program) == SetResult::Ok);
    CHECK(calls == std::vector<std::string>{"first", "late"});
    CHECK(sink.records().empty());
}

TEST_CASE("Setting a cvar from inside a callback returns InCallback and changes nothing")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("a", Value{std::int64_t{0}})));
    REQUIRE(registry.add(makeDesc("b", Value{std::int64_t{10}})));

    SetResult inner = SetResult::Ok;
    registry.onChange("a", [&registry, &inner](std::string_view, const Value&)
    {
        inner = registry.set("b", Value{std::int64_t{20}}, Source::Program);
    });
    int bCalls = 0;
    registry.onChange("b", [&bCalls](std::string_view, const Value&) { ++bCalls; });

    CHECK(registry.set("a", Value{std::int64_t{1}}, Source::Program) == SetResult::Ok);
    CHECK(inner == SetResult::InCallback);
    CHECK(registry.getInt("b") == std::optional<std::int64_t>{10});
    CHECK(bCalls == 0);
    checkOneWarning(sink);   // the refused inner set, and only it

    // Outside a callback the same set succeeds.
    CHECK(registry.set("b", Value{std::int64_t{20}}, Source::Program) == SetResult::Ok);
    CHECK(registry.getInt("b") == std::optional<std::int64_t>{20});
    CHECK(bCalls == 1);
    CHECK(sink.records().empty());
}

TEST_CASE("onChange for an unknown name returns 0 with one warning")
{
    Registry registry;
    babylon::log::CaptureSink sink;

    const Registry::CallbackId id = registry.onChange("nope",
        [](std::string_view, const Value&) {});
    CHECK(id == 0);
    checkOneWarning(sink);
    CHECK(sink.records().empty());
}

TEST_CASE("setFromString parses each type and refuses invalid text")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("flag", Value{false})));
    REQUIRE(registry.add(makeDesc("count", Value{std::int64_t{0}})));
    REQUIRE(registry.add(makeDesc("gain", Value{0.0})));
    Desc bounded = makeDesc("gain.range", Value{0.0});
    bounded.min = 0.0;
    bounded.max = 1.0;
    REQUIRE(registry.add(bounded));
    REQUIRE(registry.add(makeDesc("message", Value{std::string("hi")})));

    // Bool: the four accepted texts and nothing else.
    CHECK(registry.setFromString("flag", "true", Source::User) == SetResult::Ok);
    CHECK(registry.getBool("flag") == std::optional<bool>{true});
    CHECK(registry.setFromString("flag", "1", Source::User) == SetResult::Unchanged);
    CHECK(registry.setFromString("flag", "0", Source::User) == SetResult::Ok);
    CHECK(registry.getBool("flag") == std::optional<bool>{false});
    CHECK(registry.setFromString("flag", "false", Source::User) == SetResult::Unchanged);
    CHECK(registry.setFromString("flag", "yes", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("flag", "True", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.getBool("flag") == std::optional<bool>{false});

    // Int: a decimal with optional '-', consumed whole.
    CHECK(registry.setFromString("count", "-7", Source::User) == SetResult::Ok);
    CHECK(registry.getInt("count") == std::optional<std::int64_t>{-7});
    CHECK(registry.setFromString("count", "42", Source::User) == SetResult::Ok);
    CHECK(registry.setFromString("count", "1.5", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("count", "12abc", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("count", "+3", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("count", "", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("count", "99999999999999999999", Source::User)
        == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.getInt("count") == std::optional<std::int64_t>{42});

    // Float: babylon::core::parseDouble over the whole text; the inf and
    // nan forms and overflow are InvalidText.
    CHECK(registry.setFromString("gain", "1e3", Source::User) == SetResult::Ok);
    CHECK(registry.getFloat("gain") == std::optional<double>{1000.0});
    CHECK(registry.setFromString("gain", "-0.5", Source::User) == SetResult::Ok);
    CHECK(registry.setFromString("gain", "nan", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("gain", "inf", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("gain", "1e400", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.setFromString("gain", "abc", Source::User) == SetResult::InvalidText);
    checkOneWarning(sink);
    CHECK(registry.getFloat("gain") == std::optional<double>{-0.5});

    // A valid parse still goes through the normal set checks.
    CHECK(registry.setFromString("gain.range", "1", Source::User) == SetResult::Ok);
    CHECK(registry.setFromString("gain.range", "2", Source::User) == SetResult::OutOfRange);
    checkOneWarning(sink);
    CHECK(registry.getFloat("gain.range") == std::optional<double>{1.0});

    // String: kept as it is, up to 1024 bytes.
    CHECK(registry.setFromString("message", "hello world", Source::User) == SetResult::Ok);
    CHECK(registry.getString("message") == std::optional<std::string>{"hello world"});
    CHECK(registry.setFromString("message", std::string(1024, 'x'), Source::User)
        == SetResult::Ok);
    CHECK(registry.getString("message")->size() == 1024);
    CHECK(registry.setFromString("message", std::string(1025, 'x'), Source::User)
        == SetResult::InvalidText);
    checkOneWarning(sink);

    // An unknown name is UnknownName before the text is looked at.
    CHECK(registry.setFromString("nope", "?", Source::User) == SetResult::UnknownName);
    checkOneWarning(sink);
    CHECK(sink.records().empty());
}

TEST_CASE("reset restores the default with the same rules as set")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("vol", Value{std::int64_t{5}}, Flag::Simulation)));

    std::vector<std::string> seen;
    registry.onChange("vol", [&seen](std::string_view name, const Value& value)
    {
        seen.push_back(std::string(name) + "=" + toString(value));
    });

    // Already at the default.
    CHECK(registry.reset("vol", Source::User) == SetResult::Unchanged);
    CHECK(seen.empty());

    REQUIRE(registry.set("vol", Value{std::int64_t{2}}, Source::User) == SetResult::Ok);
    CHECK(seen == std::vector<std::string>{"vol=2"});
    CHECK(registry.reset("vol", Source::User) == SetResult::Ok);
    CHECK(registry.getInt("vol") == std::optional<std::int64_t>{5});
    CHECK(seen == std::vector<std::string>{"vol=2", "vol=5"});
    CHECK(sink.records().empty());

    CHECK(registry.reset("nope", Source::Program) == SetResult::UnknownName);
    checkOneWarning(sink);
}

TEST_CASE("names lists every cvar in sorted order")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("zz", Value{true})));
    REQUIRE(registry.add(makeDesc("m_mode", Value{std::int64_t{0}})));
    REQUIRE(registry.add(makeDesc("a", Value{std::int64_t{0}})));
    REQUIRE(registry.add(makeDesc("a.b", Value{std::int64_t{0}})));
    CHECK(sink.records().empty());

    CHECK(registry.names() == std::vector<std::string>({"a", "a.b", "m_mode", "zz"}));

    Registry empty;
    CHECK(empty.names().empty());
}

TEST_CASE("simulationValues lists only Simulation cvars, sorted, in canonical text")
{
    Registry registry;
    babylon::log::CaptureSink sink;
    REQUIRE(registry.add(makeDesc("zz_plain", Value{std::int64_t{99}}, Flag::Saved)));
    REQUIRE(registry.add(makeDesc("sim_b", Value{std::int64_t{2}}, Flag::Simulation)));
    REQUIRE(registry.add(makeDesc("sim_a", Value{true}, Flag::Simulation | Flag::Saved)));
    REQUIRE(registry.add(makeDesc("sim_f", Value{0.1}, Flag::Simulation)));
    CHECK(sink.records().empty());

    CHECK(registry.simulationValues() ==
        std::vector<std::pair<std::string, std::string>>(
            {{"sim_a", "true"}, {"sim_b", "2"}, {"sim_f", "0.1"}}));

    REQUIRE(registry.set("sim_b", Value{std::int64_t{3}}, Source::Program) == SetResult::Ok);
    CHECK(registry.simulationValues() ==
        std::vector<std::pair<std::string, std::string>>(
            {{"sim_a", "true"}, {"sim_b", "3"}, {"sim_f", "0.1"}}));

    Registry empty;
    CHECK(empty.simulationValues().empty());
}
