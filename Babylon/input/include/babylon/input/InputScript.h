// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <babylon/platform/Event.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace babylon::input
{

    // One problem found in a script: the 1-based line it sits on and a
    // message naming the problem.
    struct ScriptError
    {
        std::size_t line;
        std::string message;
    };

    // A recorded stream of per-frame input events, so replays and headless
    // runs feed an InputState exactly the events a device would
    // (ADR-0018). Input has no backend of its own: the events come from
    // here or from the platform queue, never from a device.
    //
    // The text format is one event per line, UTF-8, tokens separated by
    // spaces or tabs; '#' starts a comment that runs to the end of the
    // line, and blank lines are ignored:
    //
    //     frame <n>: key <KeyName> down|up
    //     frame <n>: mouse move <dx> <dy>
    //     frame <n>: mouse button <MouseButtonName> down|up
    //     frame <n>: mouse wheel <dx> <dy>
    //     frame <n>: gamepad <index> connect|disconnect
    //     frame <n>: gamepad <index> button <GamepadButtonName> down|up
    //     frame <n>: gamepad <index> axis <GamepadAxisName> <value>
    //
    // Names are those of keyName(), mouseButtonName(), gamepadButtonName()
    // and gamepadAxisName(). <n> is a decimal frame number 0..10,000,000
    // that never decreases from line to line; several lines may share a
    // frame and keep their order. <index> is 0..15 and becomes a
    // GamepadId of index + 1. Numbers are finite decimals parsed without a
    // locale; |dx| and |dy| are at most 100000; axis values must fit -1..1
    // for sticks and 0..1 for triggers. A line is at most 256 bytes and
    // the whole text at most 4 MiB.
    //
    // Immutable once parsed. Not thread-safe to copy; sharing a const
    // instance between threads is fine.
    class InputScript
    {
    public:
        // Parses a whole script. On the first error returns the error and
        // no script. Key and mouse events carry the given window id, so a
        // replay can feed a queue that a window-aware consumer drains;
        // MouseMoved has x = y = 0, only the motion counts.
        static std::variant<InputScript, ScriptError> parse(std::string_view text,
            platform::WindowId window);

        // The events recorded for this frame, in file order; empty for
        // frames without events. The span lives as long as this script.
        // O(log n) in the number of frames, not a scan of the events.
        std::span<const platform::Event> eventsForFrame(std::uint64_t frame) const;

        // The highest frame with events, or nullopt for an empty script.
        std::optional<std::uint64_t> lastFrame() const;

        // Total number of events in the script.
        std::size_t eventCount() const;

    private:
        // Only parse() builds a script.
        InputScript() = default;

        // Where one frame's events start in m_events. Entries are strictly
        // ascending in frame, one per frame that has events.
        struct FrameEntry
        {
            std::uint64_t frame;
            std::size_t begin;
        };

        std::vector<platform::Event> m_events;
        std::vector<FrameEntry> m_frameIndex;
    };

    // "<sourceName>:<line>: <message>", for logs and command-line errors.
    std::string formatScriptError(std::string_view sourceName, const ScriptError& error);

}
