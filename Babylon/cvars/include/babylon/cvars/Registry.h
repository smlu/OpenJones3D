// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace babylon::cvars
{

    // The value of a cvar. The active alternative fixes the cvar's type:
    // bool, int64_t for an integer cvar, double for a float cvar and
    // std::string for a string cvar. set() requires the exact alternative
    // and never converts between types.
    using Value = std::variant<bool, std::int64_t, double, std::string>;

    // What a cvar is used for, fixed when it is registered (ADR-0020).
    enum class Flag : std::uint32_t
    {
        None        = 0,
        Saved       = 1u << 0,   // written to the configuration file
        ReadOnly    = 1u << 1,   // only Source::Program may set it
        StartupOnly = 1u << 2,   // can't change after finishStartup()
        Simulation  = 1u << 3    // affects the simulation: written into
                                 // traces and replays
    };

    // The union and the intersection of flag sets.
    constexpr Flag operator|(Flag left, Flag right) noexcept
    {
        return static_cast<Flag>(static_cast<std::uint32_t>(left)
            | static_cast<std::uint32_t>(right));
    }

    constexpr Flag operator&(Flag left, Flag right) noexcept
    {
        return static_cast<Flag>(static_cast<std::uint32_t>(left)
            & static_cast<std::uint32_t>(right));
    }

    // True when `flags` carries the single flag bit `flag`; false for
    // flag = Flag::None.
    constexpr bool hasFlag(Flag flags, Flag flag) noexcept
    {
        return (flags & flag) != Flag::None;
    }

    // Who asks for a change. Program: game and engine code. User: the
    // console, the configuration file and the command line (ADR-0020).
    enum class Source
    {
        Program,
        User
    };

    // The declaration of one cvar, given to Registry::add.
    struct Desc
    {
        std::string name;            // see isValidName
        std::string description;     // one line, may be empty
        Value defaultValue;          // also fixes the type
        Flag flags = Flag::None;
        std::optional<double> min;   // only for int and float cvars; inclusive
        std::optional<double> max;
    };

    // The outcome of Registry::set, setFromString and reset. Ok means the
    // value changed; Unchanged means it already held this value. The rest
    // are refusals that change nothing and log one warning each.
    enum class SetResult
    {
        Ok,
        Unchanged,
        UnknownName,   // no cvar of that name
        WrongType,     // the value's alternative is not the cvar's type
        InvalidText,   // setFromString only: not parseable for the type
        OutOfRange,    // refused, not clamped
        ReadOnly,      // a ReadOnly cvar set with Source::User
        StartupOnly,   // any change after finishStartup()
        InCallback     // a set from inside a change callback
    };

    // True for a name Registry::add accepts: 1..64 bytes of dot-separated
    // parts, each part lowercase ASCII letters, digits and '_', starting
    // with a letter: "physics.fixed_step", "r_mode". A part must not be
    // empty, so leading, trailing and double dots are invalid.
    bool isValidName(std::string_view name);

    // Canonical text of a value, as written to configuration files and
    // traces: bools as "true"/"false", integers as decimal, doubles in the
    // shortest form that parses back to the same value (std::to_chars) and
    // strings as they are.
    std::string toString(const Value& value);

    // Named, typed settings with defaults, ranges, flags and change
    // callbacks (ADR-0013). An ordinary object owned by the composition
    // root; there is no global registry and the cvars module holds no
    // state of its own. Not thread-safe.
    //
    // A refused operation changes nothing and logs exactly one warning;
    // a successful or unchanged one logs nothing. set() and reset() check
    // in this order and return the first refusal they hit: unknown name,
    // wrong type, out of range, read-only for Source::User, startup-only
    // after finishStartup(), inside a callback; only then does the value
    // comparison return Unchanged.
    class Registry
    {
    public:
        // Registers a cvar with its default value. Returns false and logs
        // one warning for an invalid name (isValidName), a name already
        // registered, min or max on a bool or string cvar, min > max, a
        // non-finite double default or range bound, or a default outside
        // the range. The stored copy of the Desc lives as long as the
        // registry.
        [[nodiscard]] bool add(const Desc& desc);

        // Lookups. The typed getters return std::nullopt for an unknown
        // name and for a cvar of another type; they never log. desc()
        // returns nullptr for an unknown name; the pointer to the stored
        // Desc is valid until the registry dies.
        bool contains(std::string_view name) const;
        std::optional<Value> get(std::string_view name) const;
        std::optional<bool> getBool(std::string_view name) const;
        std::optional<std::int64_t> getInt(std::string_view name) const;
        std::optional<double> getFloat(std::string_view name) const;
        std::optional<std::string> getString(std::string_view name) const;
        std::optional<Flag> flags(std::string_view name) const;
        const Desc* desc(std::string_view name) const;

        // Changes a cvar. The value's alternative must match the cvar's
        // type exactly; no conversions. A non-finite double value (NaN,
        // +-inf) is always OutOfRange, whether or not the cvar has a
        // range: it would poison trace text and break the Unchanged
        // comparison (NaN != NaN). Ranges apply to int and float cvars
        // only and compare inclusively as double; bounds are finite
        // (add() checks), and an integer beyond -2^53..2^53, which a
        // double would not hold exactly, is out of range whenever it is
        // range-checked. Out of range is refused, not clamped. ReadOnly
        // cvars refuse Source::User. StartupOnly cvars refuse every
        // source after finishStartup(). A callback runs for Ok only.
        SetResult set(std::string_view name, const Value& value, Source source);

        // Parses text for the cvar's type, then behaves like set(). An
        // unknown name returns UnknownName before the text is looked at.
        // Bool: "true", "false", "1", "0". Int: a decimal with optional
        // '-', std::from_chars over the whole text; overflow is
        // InvalidText. Float: babylon::core::parseDouble over the whole
        // text; no hex, no "inf" or "nan", no overflow to infinity.
        // String: as it is, at most 1024 bytes. Anything else is
        // InvalidText.
        SetResult setFromString(std::string_view name, std::string_view text, Source source);

        // Resets to the default, with the same rules as set(); the default
        // always has the right type and is in range, so only UnknownName,
        // ReadOnly, StartupOnly, InCallback, Unchanged or Ok can come
        // back.
        SetResult reset(std::string_view name, Source source);

        // After this, StartupOnly cvars are fixed for the life of the
        // registry. Calling it again does nothing.
        void finishStartup();

        // Called after every change of this cvar (not on Unchanged), in
        // the order callbacks were added. The name is the requested cvar
        // name and the value the new stored value; both are valid only
        // during the call. A callback must not set cvars: set() from
        // inside a callback returns InCallback. A callback removed while
        // the current change dispatches is not called by it; callbacks
        // added during it start with the next change.
        using Callback = std::function<void(std::string_view name, const Value& value)>;
        using CallbackId = std::uint64_t;   // never 0

        // Returns 0 and logs one warning for an unknown name.
        CallbackId onChange(std::string_view name, Callback callback);

        // Removes that callback; unknown ids are ignored.
        void removeCallback(CallbackId id);

        // The registered names, sorted in byte order.
        std::vector<std::string> names() const;

        // Every Simulation cvar as (name, toString(value)), sorted by
        // name: what a trace records.
        std::vector<std::pair<std::string, std::string>> simulationValues() const;

    private:
        struct Entry
        {
            Desc desc;
            Value value;   // the current value, the default until changed
            std::vector<std::pair<CallbackId, Callback>> callbacks;
        };

        // Sorted so lookups by string_view allocate nothing and
        // enumeration is deterministic; the node-based storage keeps the
        // Desc pointers stable.
        using Entries = std::map<std::string, Entry, std::less<>>;

        Entries m_entries;
        std::uint64_t m_nextCallbackId = 1;
        bool m_startupFinished = false;
        bool m_inCallback = false;

        // The looked-up entry or nullptr; find(name) is the caller's
        // requested name, used for warnings and callbacks.
        Entry* find(std::string_view name);
        const Entry* find(std::string_view name) const;

        // The shared tail of set(), setFromString() and reset(); the entry
        // must exist. Logs the one warning every refusal needs.
        SetResult applySet(std::string_view name, Entry& entry, const Value& value,
            Source source);

        // Dispatches this cvar's callbacks over a snapshot of their ids.
        void runCallbacks(std::string_view name, Entry& entry);

        // The callback registered under this id, or nullptr; ids are
        // unique across the registry.
        const Callback* findCallback(CallbackId id) const;
    };

}
