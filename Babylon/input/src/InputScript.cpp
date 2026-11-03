// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/input/InputScript.h>

#include <babylon/core/Parse.h>
#include <babylon/platform/Keys.h>

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace babylon::input
{
namespace
{

    // The whole text one script may have, in bytes.
    constexpr std::size_t kMaxTextBytes = 4 * 1024 * 1024;

    // The longest single line, in bytes.
    constexpr std::size_t kMaxLineBytes = 256;

    // The largest frame number a line may name.
    constexpr std::uint64_t kMaxFrameNumber = 10'000'000;

    // The largest magnitude of one mouse motion or wheel value.
    constexpr float kMaxMouseMotion = 100'000.0f;

    // The largest gamepad index; the GamepadId is one above it.
    constexpr std::uint64_t kMaxGamepadIndex = 15;

    // Splits at runs of spaces and tabs; there are then no empty tokens, so
    // a missing token shows up as a short token list.
    std::vector<std::string_view> tokenize(std::string_view line)
    {
        std::vector<std::string_view> tokens;
        std::size_t pos = 0;
        while ( pos < line.size() )
        {
            if ( line[pos] == ' ' || line[pos] == '\t' )
            {
                ++pos;
                continue;
            }
            const std::size_t start = pos;
            while ( pos < line.size() && line[pos] != ' ' && line[pos] != '\t' )
            {
                ++pos;
            }
            tokens.push_back(line.substr(start, pos - start));
        }
        return tokens;
    }

    // Removes leading and trailing spaces and tabs.
    std::string_view trimSpaces(std::string_view text)
    {
        while ( !text.empty() && ( text.front() == ' ' || text.front() == '\t' ) )
        {
            text.remove_prefix(1);
        }
        while ( !text.empty() && ( text.back() == ' ' || text.back() == '\t' ) )
        {
            text.remove_suffix(1);
        }
        return text;
    }

    // Formats "<what> '<token>'" for the error messages that name the value
    // they rejected.
    std::string quoted(std::string_view what, std::string_view token)
    {
        return std::string(what) + " '" + std::string(token) + "'";
    }

    // Parses a plain decimal without sign handling beyond rejecting '-':
    // the whole token must be digits and the value must not exceed `maximum`.
    // `what` names the number in the message.
    std::optional<std::uint64_t> parseCount(std::string_view token, std::string_view what,
        std::uint64_t maximum, std::string& error)
    {
        if ( !token.empty() && token.front() == '-' )
        {
            error = std::string(what) + " must not be negative";
            return std::nullopt;
        }
        std::uint64_t value = 0;
        const std::from_chars_result result =
            std::from_chars(token.data(), token.data() + token.size(), value);
        if ( result.ec == std::errc::result_out_of_range )
        {
            error = std::string(what) + " " + std::string(token) + " is above "
                + std::to_string(maximum);
            return std::nullopt;
        }
        if ( result.ec != std::errc{} || result.ptr != token.data() + token.size() )
        {
            error = "invalid " + quoted(what, token);
            return std::nullopt;
        }
        if ( value > maximum )
        {
            error = std::string(what) + " " + std::string(token) + " is above "
                + std::to_string(maximum);
            return std::nullopt;
        }
        return value;    }

    // Parses a finite decimal in babylon::core::parseDouble's grammar,
    // no locale; anything that is not one number, including the inf and
    // nan forms, is an error, and so is a value outside the float range.
    // `what` names the number in the message.
    std::optional<float> parseNumber(std::string_view token, std::string_view what,
        std::string& error)
    {
        const std::optional<double> parsed = core::parseDouble(token);
        if ( !parsed.has_value()
            || *parsed > std::numeric_limits<float>::max()
            || *parsed < std::numeric_limits<float>::lowest() )
        {
            error = "invalid " + quoted(what, token);
            return std::nullopt;
        }
        return static_cast<float>(*parsed);
    }

    // True for "down", false for "up", nullopt for anything else; the
    // caller turns that, and a missing token, into the message.
    std::optional<bool> parseDownUp(std::string_view token)
    {
        if ( token == "down" )
        {
            return true;
        }
        if ( token == "up" )
        {
            return false;
        }
        return std::nullopt;
    }

    // The error when a token that must start at `index` is not there.
    std::string missingToken(std::string_view what)
    {
        return "missing " + std::string(what);
    }

    // The error for a token after the one the form ends with.
    std::string extraToken(std::string_view token)
    {
        return "unexpected extra token '" + std::string(token) + "'";
    }

    // Parses the "<dx> <dy>" pair of a mouse move or wheel line.
    std::optional<std::pair<float, float>> parseMotion(const std::vector<std::string_view>& tokens,
        std::size_t first, std::string_view what, std::string& error)
    {
        if ( tokens.size() <= first )
        {
            error = missingToken(std::string(what) + " in x");
            return std::nullopt;
        }
        const std::optional<float> x = parseNumber(tokens[first], what, error);
        if ( !x.has_value() )
        {
            return std::nullopt;
        }
        if ( tokens.size() <= first + 1 )
        {
            error = missingToken(std::string(what) + " in y");
            return std::nullopt;
        }
        const std::optional<float> y = parseNumber(tokens[first + 1], what, error);
        if ( !y.has_value() )
        {
            return std::nullopt;
        }
        for ( const std::size_t current : {first, first + 1} )
        {
            const float value = current == first ? *x : *y;
            if ( value < -kMaxMouseMotion || value > kMaxMouseMotion )
            {
                error = std::string(what) + " " + std::string(tokens[current])
                    + " is outside -100000..100000";
                return std::nullopt;
            }
        }
        if ( tokens.size() > first + 2 )
        {
            error = extraToken(tokens[first + 2]);
            return std::nullopt;
        }
        return std::pair{*x, *y};
    }

    // Parses one event line body, the tokens after the "frame <n>:" header.
    // Returns the event, or nullopt with `error` naming the first problem.
    std::optional<platform::Event> parseEvent(const std::vector<std::string_view>& tokens,
        platform::WindowId window, std::string& error)
    {
        const std::string_view verb = tokens[0];

        if ( verb == "key" )
        {
            if ( tokens.size() == 1 )
            {
                error = missingToken("key name");
                return std::nullopt;
            }
            const std::optional<platform::Key> key = platform::keyFromName(tokens[1]);
            if ( !key.has_value() )
            {
                error = "unknown " + quoted("key name", tokens[1]);
                return std::nullopt;
            }
            if ( tokens.size() == 2 )
            {
                error = missingToken("'down' or 'up'");
                return std::nullopt;
            }
            const std::optional<bool> state = parseDownUp(tokens[2]);
            if ( !state.has_value() )
            {
                error = "expected 'down' or 'up', found '" + std::string(tokens[2]) + "'";
                return std::nullopt;
            }
            if ( tokens.size() > 3 )
            {
                error = extraToken(tokens[3]);
                return std::nullopt;
            }
            return platform::KeyChanged{window, *key, *state, false};
        }

        if ( verb == "mouse" )
        {
            if ( tokens.size() == 1 )
            {
                error = missingToken("'move', 'button' or 'wheel'");
                return std::nullopt;
            }
            const std::string_view kind = tokens[1];
            if ( kind == "move" || kind == "wheel" )
            {
                const std::string_view what = kind == "move" ? "mouse motion" : "wheel motion";
                const std::optional<std::pair<float, float>> motion =
                    parseMotion(tokens, 2, what, error);
                if ( !motion.has_value() )
                {
                    return std::nullopt;
                }
                if ( kind == "move" )
                {
                    // A script records motion, not a position; the game
                    // keeps its own cursor position across frames.
                    return platform::MouseMoved{window, 0.0f, 0.0f, motion->first, motion->second};
                }
                return platform::MouseWheelMoved{window, motion->first, motion->second};
            }
            if ( kind == "button" )
            {
                if ( tokens.size() == 2 )
                {
                    error = missingToken("mouse button name");
                    return std::nullopt;
                }
                const std::optional<platform::MouseButton> button =
                    platform::mouseButtonFromName(tokens[2]);
                if ( !button.has_value() )
                {
                    error = "unknown " + quoted("mouse button name", tokens[2]);
                    return std::nullopt;
                }
                if ( tokens.size() == 3 )
                {
                    error = missingToken("'down' or 'up'");
                    return std::nullopt;
                }
                const std::optional<bool> state = parseDownUp(tokens[3]);
                if ( !state.has_value() )
                {
                    error = "expected 'down' or 'up', found '" + std::string(tokens[3]) + "'";
                    return std::nullopt;
                }
                if ( tokens.size() > 4 )
                {
                    error = extraToken(tokens[4]);
                    return std::nullopt;
                }
                return platform::MouseButtonChanged{window, *button, *state};
            }
            error = "unknown mouse event '" + std::string(kind) + "'";
            return std::nullopt;
        }

        if ( verb == "gamepad" )
        {
            if ( tokens.size() == 1 )
            {
                error = missingToken("gamepad index");
                return std::nullopt;
            }
            const std::optional<std::uint64_t> index =
                parseCount(tokens[1], "gamepad index", kMaxGamepadIndex, error);
            if ( !index.has_value() )
            {
                return std::nullopt;
            }
            const platform::GamepadId gamepad =
                static_cast<platform::GamepadId>(*index + 1);
            if ( tokens.size() == 2 )
            {
                error = missingToken("'connect', 'disconnect', 'button' or 'axis'");
                return std::nullopt;
            }
            const std::string_view action = tokens[2];
            if ( action == "connect" || action == "disconnect" )
            {
                if ( tokens.size() > 3 )
                {
                    error = extraToken(tokens[3]);
                    return std::nullopt;
                }
                if ( action == "connect" )
                {
                    return platform::GamepadConnected{gamepad};
                }
                return platform::GamepadDisconnected{gamepad};
            }
            if ( action == "button" )
            {
                if ( tokens.size() == 3 )
                {
                    error = missingToken("gamepad button name");
                    return std::nullopt;
                }
                const std::optional<platform::GamepadButton> button =
                    platform::gamepadButtonFromName(tokens[3]);
                if ( !button.has_value() )
                {
                    error = "unknown " + quoted("gamepad button name", tokens[3]);
                    return std::nullopt;
                }
                if ( tokens.size() == 4 )
                {
                    error = missingToken("'down' or 'up'");
                    return std::nullopt;
                }
                const std::optional<bool> state = parseDownUp(tokens[4]);
                if ( !state.has_value() )
                {
                    error = "expected 'down' or 'up', found '" + std::string(tokens[4]) + "'";
                    return std::nullopt;
                }
                if ( tokens.size() > 5 )
                {
                    error = extraToken(tokens[5]);
                    return std::nullopt;
                }
                return platform::GamepadButtonChanged{gamepad, *button, *state};
            }
            if ( action == "axis" )
            {
                if ( tokens.size() == 3 )
                {
                    error = missingToken("gamepad axis name");
                    return std::nullopt;
                }
                const std::optional<platform::GamepadAxis> axis =
                    platform::gamepadAxisFromName(tokens[3]);
                if ( !axis.has_value() )
                {
                    error = "unknown " + quoted("gamepad axis name", tokens[3]);
                    return std::nullopt;
                }
                if ( tokens.size() == 4 )
                {
                    error = missingToken("axis value");
                    return std::nullopt;
                }
                const std::optional<float> value = parseNumber(tokens[4], "axis value", error);
                if ( !value.has_value() )
                {
                    return std::nullopt;
                }
                const bool trigger = *axis == platform::GamepadAxis::LeftTrigger
                    || *axis == platform::GamepadAxis::RightTrigger;
                if ( *value < ( trigger ? 0.0f : -1.0f ) || *value > 1.0f )
                {
                    error = "axis value " + std::string(tokens[4]) + " is outside "
                        + ( trigger ? "0..1" : "-1..1" );
                    return std::nullopt;
                }
                if ( tokens.size() > 5 )
                {
                    error = extraToken(tokens[5]);
                    return std::nullopt;
                }
                return platform::GamepadAxisMoved{gamepad, *axis, *value};
            }
            error = "unknown gamepad event '" + std::string(action) + "'";
            return std::nullopt;
        }

        error = "unknown verb '" + std::string(verb) + "'";
        return std::nullopt;
    }

}

    std::variant<InputScript, ScriptError> InputScript::parse(std::string_view text,
        platform::WindowId window)
    {
        if ( text.size() > kMaxTextBytes )
        {
            return ScriptError{1, "input script is larger than "
                + std::to_string(kMaxTextBytes) + " bytes"};
        }

        std::vector<platform::Event> events;
        std::vector<FrameEntry> frameIndex;
        std::uint64_t previousFrame = 0;
        bool havePrevious = false;

        std::size_t position = 0;
        std::size_t lineNumber = 0;
        while ( position < text.size() )
        {
            ++lineNumber;
            const std::size_t newline = text.find('\n', position);
            const std::size_t end = newline == std::string_view::npos ? text.size() : newline;
            std::string_view line = text.substr(position, end - position);
            position = end == text.size() ? text.size() : end + 1;

            // A CRLF file parses like an LF one; the length check counts the
            // line without its terminator.
            if ( !line.empty() && line.back() == '\r' )
            {
                line.remove_suffix(1);
            }
            if ( line.size() > kMaxLineBytes )
            {
                return ScriptError{lineNumber,
                    "line is longer than " + std::to_string(kMaxLineBytes) + " bytes"};
            }

            // '#' starts a comment that runs to the end of the line.
            const std::size_t hash = line.find('#');
            if ( hash != std::string_view::npos )
            {
                line = line.substr(0, hash);
            }
            line = trimSpaces(line);
            if ( line.empty() )
            {
                continue;   // blank or comment-only line
            }

            const std::size_t colon = line.find(':');
            if ( colon == std::string_view::npos )
            {
                return ScriptError{lineNumber, "missing ':' after the frame number"};
            }

            const std::vector<std::string_view> head = tokenize(line.substr(0, colon));
            if ( head.size() != 2 || head[0] != "frame" )
            {
                return ScriptError{lineNumber, "expected 'frame <n>:'"};
            }

            std::string error;
            const std::optional<std::uint64_t> frame =
                parseCount(head[1], "frame number", kMaxFrameNumber, error);
            if ( !frame.has_value() )
            {
                return ScriptError{lineNumber, error};
            }
            if ( havePrevious && *frame < previousFrame )
            {
                return ScriptError{lineNumber, "frame number " + std::to_string(*frame)
                    + " is below previous frame " + std::to_string(previousFrame)};
            }

            const std::vector<std::string_view> body = tokenize(line.substr(colon + 1));
            if ( body.empty() )
            {
                return ScriptError{lineNumber, "missing event after ':'"};
            }
            const std::optional<platform::Event> event = parseEvent(body, window, error);
            if ( !event.has_value() )
            {
                return ScriptError{lineNumber, error};
            }

            if ( frameIndex.empty() || frameIndex.back().frame != *frame )
            {
                frameIndex.push_back(FrameEntry{*frame, events.size()});
            }
            events.push_back(*event);
            previousFrame = *frame;
            havePrevious = true;
        }

        InputScript script;
        script.m_events = std::move(events);
        script.m_frameIndex = std::move(frameIndex);
        return script;
    }

    std::span<const platform::Event> InputScript::eventsForFrame(std::uint64_t frame) const
    {
        // m_frameIndex is strictly ascending, so this is a binary search and
        // never a scan of the events.
        const auto found = std::lower_bound(m_frameIndex.begin(), m_frameIndex.end(), frame,
            [](const FrameEntry& entry, std::uint64_t value) { return entry.frame < value; });
        if ( found == m_frameIndex.end() || found->frame != frame )
        {
            return {};
        }
        const std::size_t first = found->begin;
        const std::size_t last =
            found + 1 != m_frameIndex.end() ? (found + 1)->begin : m_events.size();
        return std::span<const platform::Event>(m_events.data() + first, last - first);
    }

    std::optional<std::uint64_t> InputScript::lastFrame() const
    {
        if ( m_frameIndex.empty() )
        {
            return std::nullopt;
        }
        return m_frameIndex.back().frame;
    }

    std::size_t InputScript::eventCount() const
    {
        return m_events.size();
    }

    std::string formatScriptError(std::string_view sourceName, const ScriptError& error)
    {
        return std::string(sourceName) + ":" + std::to_string(error.line) + ": " + error.message;
    }

}
