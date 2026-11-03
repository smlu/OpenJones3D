// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>
#include <string_view>

namespace babylon::core
{

    // Parses the whole text as a finite decimal floating-point number,
    // independent of the C and C++ locale. Accepted: an optional '-', then
    // digits with an optional '.' and fraction (at least one digit in
    // total), then an optional exponent 'e' or 'E' with an optional sign
    // and at least one digit. Nothing else: no leading '+', no whitespace,
    // no hex, no "inf" or "nan", no trailing characters. Results that
    // overflow to infinity are nullopt; underflow to zero or a denormal is
    // accepted. The value is correctly rounded.
    std::optional<double> parseDouble(std::string_view text);

}
