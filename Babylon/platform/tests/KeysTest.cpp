// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/platform/Keys.h>

#include <doctest.h>

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    using babylon::platform::GamepadAxis;
    using babylon::platform::GamepadButton;
    using babylon::platform::Key;
    using babylon::platform::MouseButton;

    // Every enumerator of a contiguous enum in value order: first is the
    // lowest value to list and last the highest.
    template <typename E>
    std::vector<E> enumerators(int first, int last)
    {
        std::vector<E> values;
        for ( int value = first; value <= last; ++value )
        {
            values.push_back(static_cast<E>(value));
        }
        return values;
    }

    // The round trip every name table promises: name -> enumerator -> name,
    // with no two enumerators sharing a name.
    template <typename E>
    void checkNamesRoundTrip(const std::vector<E>& values,
        std::string_view (*name)(E), std::optional<E> (*fromName)(std::string_view))
    {
        std::set<std::string> seen;
        for ( const E value : values )
        {
            const std::string_view text = name(value);
            INFO("enumerator value " << static_cast<int>(value)
                << " name " << text);
            CHECK(!text.empty());
            CHECK(seen.insert(std::string(text)).second);   // every name is unique
            CHECK(fromName(text) == value);                 // the inverse is exact
        }
    }
}

TEST_CASE("Every key name round-trips through keyFromName and is unique")
{
    // Unknown is not a name; every enumerator up to the last real key is.
    checkNamesRoundTrip(enumerators<Key>(1, static_cast<int>(Key::Count) - 1),
        babylon::platform::keyName, babylon::platform::keyFromName);
}

TEST_CASE("keyName gives Unknown outside the table")
{
    CHECK(babylon::platform::keyName(Key::Unknown) == "Unknown");
    CHECK(babylon::platform::keyName(static_cast<Key>(Key::Count)) == "Unknown");
    CHECK(babylon::platform::keyName(static_cast<Key>(60000)) == "Unknown");
}

TEST_CASE("keyFromName refuses names that are not keys")
{
    // Case-sensitive: the table spells the letters uppercase.
    CHECK(babylon::platform::keyFromName("a") == std::nullopt);
    // The two enumerators that name no key.
    CHECK(babylon::platform::keyFromName("Unknown") == std::nullopt);
    CHECK(babylon::platform::keyFromName("Count") == std::nullopt);
    CHECK(babylon::platform::keyFromName("") == std::nullopt);
    CHECK(babylon::platform::keyFromName("Keypad") == std::nullopt);
}

TEST_CASE("Every mouse button name round-trips and is unique")
{
    checkNamesRoundTrip(enumerators<MouseButton>(0, static_cast<int>(MouseButton::X2)),
        babylon::platform::mouseButtonName, babylon::platform::mouseButtonFromName);
    CHECK(babylon::platform::mouseButtonFromName("left") == std::nullopt);
    CHECK(babylon::platform::mouseButtonFromName("") == std::nullopt);
    CHECK(babylon::platform::mouseButtonName(static_cast<MouseButton>(7)).empty());
}

TEST_CASE("Every gamepad button name round-trips and is unique")
{
    checkNamesRoundTrip(
        enumerators<GamepadButton>(0, static_cast<int>(GamepadButton::DpadRight)),
        babylon::platform::gamepadButtonName, babylon::platform::gamepadButtonFromName);
    CHECK(babylon::platform::gamepadButtonFromName("North ") == std::nullopt);
    CHECK(babylon::platform::gamepadButtonFromName("South") == GamepadButton::South);
}

TEST_CASE("Every gamepad axis name round-trips and is unique")
{
    checkNamesRoundTrip(enumerators<GamepadAxis>(0, static_cast<int>(GamepadAxis::RightTrigger)),
        babylon::platform::gamepadAxisName, babylon::platform::gamepadAxisFromName);
    CHECK(babylon::platform::gamepadAxisFromName("Left") == std::nullopt);
    CHECK(babylon::platform::gamepadAxisFromName("LeftX") == GamepadAxis::LeftX);
}
