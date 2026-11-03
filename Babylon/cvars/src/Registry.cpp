// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/cvars/Registry.h>

#include <babylon/core/Parse.h>
#include <babylon/log/Log.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace babylon::cvars
{
namespace
{

    // The longest accepted cvar name, in bytes (isValidName).
    constexpr std::size_t kMaxNameBytes = 64;

    // The longest value of a string cvar, in bytes (setFromString).
    constexpr std::size_t kMaxStringBytes = 1024;

    // 2^53: the largest magnitude an int64 has exactly as a double, so
    // only integers within reach can be compared against a double range.
    constexpr std::int64_t kExactDoubleLimit = 1LL << 53;

    bool isAsciiLetter(char c)
    {
        return c >= 'a' && c <= 'z';
    }

    bool isNameChar(char c)
    {
        return isAsciiLetter(c) || ( c >= '0' && c <= '9' ) || c == '_';
    }

    // The warning text naming a refusal.
    std::string_view setResultText(SetResult result)
    {
        switch ( result )
        {
        case SetResult::UnknownName: return "no such cvar";
        case SetResult::WrongType: return "wrong value type";
        case SetResult::InvalidText: return "invalid text";
        case SetResult::OutOfRange: return "value out of range";
        case SetResult::ReadOnly: return "read-only cvar set by the user";
        case SetResult::StartupOnly: return "startup-only cvar after startup";
        case SetResult::InCallback: return "set from inside a change callback";
        case SetResult::Ok:
        case SetResult::Unchanged: break;
        }
        return "refused";   // unreachable: only refusals are logged
    }

    // True when the alternative of `value` is the type fixed by
    // `defaultValue`.
    bool typeMatches(const Value& defaultValue, const Value& value)
    {
        return value.index() == defaultValue.index();
    }

    // True when `value` passes desc's range, which must only be set for
    // int and float cvars (add() checks) and whose bounds are finite
    // (add() checks). Every non-finite double fails here, range or not:
    // it would poison trace text and break the Unchanged comparison.
    // Ranges compare inclusively as double, and an integer beyond
    // -2^53..2^53, which a double would not hold exactly, fails a set
    // range too.
    bool inRange(const Desc& desc, const Value& value)
    {
        if ( const double* p = std::get_if<double>(&value);
            p != nullptr && !std::isfinite(*p) )
        {
            return false;
        }
        if ( !desc.min.has_value() && !desc.max.has_value() )
        {
            return true;
        }
        double number = 0.0;
        if ( const std::int64_t* p = std::get_if<std::int64_t>(&value) )
        {
            if ( *p < -kExactDoubleLimit || *p > kExactDoubleLimit )
            {
                return false;
            }
            number = static_cast<double>(*p);
        }
        else
        {
            number = std::get<double>(value);
        }
        // Negated comparisons so NaN fails.
        if ( desc.min.has_value() && !( number >= *desc.min ) )
        {
            return false;
        }
        if ( desc.max.has_value() && !( number <= *desc.max ) )
        {
            return false;
        }
        return true;
    }

    // The four accepted bool texts; anything else is nullopt.
    std::optional<bool> parseBoolText(std::string_view text)
    {
        if ( text == "true" || text == "1" )
        {
            return true;
        }
        if ( text == "false" || text == "0" )
        {
            return false;
        }
        return std::nullopt;
    }

    // A decimal with optional '-', consumed whole by std::from_chars;
    // signs it rejects, leftovers and int64 overflow are nullopt.
    std::optional<std::int64_t> parseIntText(std::string_view text)
    {
        std::int64_t value = 0;
        const std::from_chars_result result =
            std::from_chars(text.data(), text.data() + text.size(), value);
        if ( result.ec != std::errc{} || result.ptr != text.data() + text.size() )
        {
            return std::nullopt;
        }
        return value;
    }

    // A finite decimal consumed whole in babylon::core::parseDouble's
    // grammar; hex, the inf and nan forms, leftover characters and
    // overflow to infinity are nullopt.
    std::optional<double> parseFloatText(std::string_view text)
    {
        return core::parseDouble(text);
    }

    // Parses text for the type fixed by defaultValue; nullopt for text
    // that type does not accept.
    std::optional<Value> parseValue(const Value& defaultValue, std::string_view text)
    {
        if ( std::holds_alternative<bool>(defaultValue) )
        {
            const std::optional<bool> value = parseBoolText(text);
            if ( !value.has_value() )
            {
                return std::nullopt;
            }
            return Value{*value};
        }
        if ( std::holds_alternative<std::int64_t>(defaultValue) )
        {
            const std::optional<std::int64_t> value = parseIntText(text);
            if ( !value.has_value() )
            {
                return std::nullopt;
            }
            return Value{*value};
        }
        if ( std::holds_alternative<double>(defaultValue) )
        {
            const std::optional<double> value = parseFloatText(text);
            if ( !value.has_value() )
            {
                return std::nullopt;
            }
            return Value{*value};
        }
        if ( text.size() > kMaxStringBytes )
        {
            return std::nullopt;
        }
        return Value{std::string(text)};
    }

}

    bool isValidName(std::string_view name)
    {
        if ( name.empty() || name.size() > kMaxNameBytes )
        {
            return false;
        }
        // A dot must be followed by a letter, which rules out leading,
        // trailing and double dots in one rule.
        bool letterRequired = true;
        for ( const char c : name )
        {
            if ( c == '.' )
            {
                if ( letterRequired )
                {
                    return false;
                }
                letterRequired = true;
                continue;
            }
            if ( letterRequired )
            {
                if ( !isAsciiLetter(c) )
                {
                    return false;
                }
                letterRequired = false;
            }
            else if ( !isNameChar(c) )
            {
                return false;
            }
        }
        return !letterRequired;
    }

    std::string toString(const Value& value)
    {
        if ( const bool* p = std::get_if<bool>(&value) )
        {
            return *p ? "true" : "false";
        }
        if ( const std::int64_t* p = std::get_if<std::int64_t>(&value) )
        {
            return std::to_string(*p);
        }
        if ( const double* p = std::get_if<double>(&value) )
        {
            // The shortest decimal form that parses back to this double.
            // It needs far fewer bytes than the buffer holds: even the
            // smallest subnormal written out stays below 350.
            std::array<char, 512> buffer;
            const std::to_chars_result result =
                std::to_chars(buffer.data(), buffer.data() + buffer.size(), *p);
            if ( result.ec != std::errc{} )
            {
                return {};   // unreachable: the buffer is big enough
            }
            return std::string(buffer.data(), result.ptr);
        }
        return std::get<std::string>(value);
    }

    bool Registry::add(const Desc& desc)
    {
        if ( !isValidName(desc.name) )
        {
            log::warning("cvars: rejected invalid cvar name '{}'", desc.name);
            return false;
        }
        if ( m_entries.contains(desc.name) )
        {
            log::warning("cvars: cvar '{}' is already registered", desc.name);
            return false;
        }
        const bool numeric = std::holds_alternative<std::int64_t>(desc.defaultValue)
            || std::holds_alternative<double>(desc.defaultValue);
        if ( !numeric && ( desc.min.has_value() || desc.max.has_value() ) )
        {
            log::warning("cvars: '{}' has a range but is not an int or float cvar",
                desc.name);
            return false;
        }
        if ( desc.min.has_value() && desc.max.has_value() && *desc.min > *desc.max )
        {
            log::warning("cvars: '{}' has min {} above max {}", desc.name, *desc.min,
                *desc.max);
            return false;
        }
        if ( desc.min.has_value() && !std::isfinite(*desc.min) )
        {
            log::warning("cvars: '{}' has a non-finite minimum", desc.name);
            return false;
        }
        if ( desc.max.has_value() && !std::isfinite(*desc.max) )
        {
            log::warning("cvars: '{}' has a non-finite maximum", desc.name);
            return false;
        }
        if ( const double* p = std::get_if<double>(&desc.defaultValue);
            p != nullptr && !std::isfinite(*p) )
        {
            log::warning("cvars: '{}' has a non-finite default", desc.name);
            return false;
        }
        if ( !inRange(desc, desc.defaultValue) )
        {
            log::warning("cvars: '{}' has a default outside its range", desc.name);
            return false;
        }

        m_entries.emplace(desc.name, Entry{desc, desc.defaultValue, {}});
        return true;
    }

    bool Registry::contains(std::string_view name) const
    {
        return m_entries.contains(name);
    }

    std::optional<Value> Registry::get(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return std::nullopt;
        }
        return entry->value;
    }

    std::optional<bool> Registry::getBool(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return std::nullopt;
        }
        if ( const bool* p = std::get_if<bool>(&entry->value) )
        {
            return *p;
        }
        return std::nullopt;
    }

    std::optional<std::int64_t> Registry::getInt(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return std::nullopt;
        }
        if ( const std::int64_t* p = std::get_if<std::int64_t>(&entry->value) )
        {
            return *p;
        }
        return std::nullopt;
    }

    std::optional<double> Registry::getFloat(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return std::nullopt;
        }
        if ( const double* p = std::get_if<double>(&entry->value) )
        {
            return *p;
        }
        return std::nullopt;
    }

    std::optional<std::string> Registry::getString(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return std::nullopt;
        }
        if ( const std::string* p = std::get_if<std::string>(&entry->value) )
        {
            return *p;
        }
        return std::nullopt;
    }

    std::optional<Flag> Registry::flags(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return std::nullopt;
        }
        return entry->desc.flags;
    }

    const Desc* Registry::desc(std::string_view name) const
    {
        const Entry* entry = find(name);
        if ( entry == nullptr )
        {
            return nullptr;
        }
        return &entry->desc;
    }

    SetResult Registry::set(std::string_view name, const Value& value, Source source)
    {
        Entry* entry = find(name);
        if ( entry == nullptr )
        {
            log::warning("cvars: set of '{}': {}", name, setResultText(SetResult::UnknownName));
            return SetResult::UnknownName;
        }
        return applySet(name, *entry, value, source);
    }

    SetResult Registry::setFromString(std::string_view name, std::string_view text,
        Source source)
    {
        Entry* entry = find(name);
        if ( entry == nullptr )
        {
            log::warning("cvars: set of '{}': {}", name, setResultText(SetResult::UnknownName));
            return SetResult::UnknownName;
        }
        const std::optional<Value> value = parseValue(entry->desc.defaultValue, text);
        if ( !value.has_value() )
        {
            log::warning("cvars: rejected text '{}' for cvar '{}'", text, name);
            return SetResult::InvalidText;
        }
        return applySet(name, *entry, *value, source);
    }

    SetResult Registry::reset(std::string_view name, Source source)
    {
        Entry* entry = find(name);
        if ( entry == nullptr )
        {
            log::warning("cvars: reset of '{}': {}", name, setResultText(SetResult::UnknownName));
            return SetResult::UnknownName;
        }
        return applySet(name, *entry, entry->desc.defaultValue, source);
    }

    void Registry::finishStartup()
    {
        m_startupFinished = true;
    }

    Registry::CallbackId Registry::onChange(std::string_view name, Callback callback)
    {
        Entry* entry = find(name);
        if ( entry == nullptr )
        {
            log::warning("cvars: onChange for '{}': no such cvar", name);
            return 0;
        }
        const CallbackId id = m_nextCallbackId++;
        entry->callbacks.emplace_back(id, std::move(callback));
        return id;
    }

    void Registry::removeCallback(CallbackId id)
    {
        for (auto& node : m_entries)
        {
            std::erase_if(node.second.callbacks,
                [id](const std::pair<CallbackId, Callback>& callback)
                {
                    return callback.first == id;
                });
        }
    }

    std::vector<std::string> Registry::names() const
    {
        std::vector<std::string> result;
        result.reserve(m_entries.size());
        for ( const auto& node : m_entries )
        {
            result.push_back(node.first);
        }
        return result;
    }

    std::vector<std::pair<std::string, std::string>> Registry::simulationValues() const
    {
        std::vector<std::pair<std::string, std::string>> result;
        for ( const auto& node : m_entries )
        {
            if ( hasFlag(node.second.desc.flags, Flag::Simulation) )
            {
                result.emplace_back(node.first, toString(node.second.value));
            }
        }
        return result;
    }

    Registry::Entry* Registry::find(std::string_view name)
    {
        const auto found = m_entries.find(name);
        return found == m_entries.end() ? nullptr : &found->second;
    }

    const Registry::Entry* Registry::find(std::string_view name) const
    {
        const auto found = m_entries.find(name);
        return found == m_entries.end() ? nullptr : &found->second;
    }

    SetResult Registry::applySet(std::string_view name, Entry& entry, const Value& value,
        Source source)
    {
        const Desc& desc = entry.desc;
        SetResult refusal = SetResult::Ok;
        if ( !typeMatches(desc.defaultValue, value) )
        {
            refusal = SetResult::WrongType;
        }
        else if ( !inRange(desc, value) )
        {
            refusal = SetResult::OutOfRange;
        }
        else if ( hasFlag(desc.flags, Flag::ReadOnly) && source == Source::User )
        {
            refusal = SetResult::ReadOnly;
        }
        else if ( hasFlag(desc.flags, Flag::StartupOnly) && m_startupFinished )
        {
            refusal = SetResult::StartupOnly;
        }
        else if ( m_inCallback )
        {
            refusal = SetResult::InCallback;
        }
        else if ( entry.value == value )
        {
            return SetResult::Unchanged;
        }
        else
        {
            entry.value = value;
            runCallbacks(name, entry);
            return SetResult::Ok;
        }

        log::warning("cvars: rejected set of '{}': {}", name, setResultText(refusal));
        return refusal;
    }

    void Registry::runCallbacks(std::string_view name, Entry& entry)
    {
        if ( entry.callbacks.empty() )
        {
            return;
        }
        // Fix the call order and the set of callbacks up front: callbacks
        // added while these run start with the next change.
        std::vector<CallbackId> ids;
        ids.reserve(entry.callbacks.size());
        for ( const std::pair<CallbackId, Callback>& callback : entry.callbacks )
        {
            ids.push_back(callback.first);
        }

        m_inCallback = true;
        for ( const CallbackId id : ids )
        {
            const Callback* found = findCallback(id);
            if ( found == nullptr || !*found )
            {
                continue;   // removed while the earlier callbacks ran
            }
            // Copy so removing the callback from inside itself is safe.
            const Callback callback = *found;
            callback(name, entry.value);
        }
        m_inCallback = false;
    }

    const Registry::Callback* Registry::findCallback(CallbackId id) const
    {
        for ( const auto& node : m_entries )
        {
            for ( const std::pair<CallbackId, Callback>& callback : node.second.callbacks )
            {
                if ( callback.first == id )
                {
                    return &callback.second;
                }
            }
        }
        return nullptr;
    }

}
