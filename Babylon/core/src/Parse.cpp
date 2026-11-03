// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Parse.h>

#include <clocale>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <string_view>

namespace babylon::core
{
namespace
{

    bool isDigit(char c)
    {
        return c >= '0' && c <= '9';
    }

    // True when the text is exactly the grammar parseDouble documents, and
    // only a conversion of it remains to be done. Checked by hand because
    // the conversion below would also accept leading whitespace, hex
    // prefixes, the inf and nan forms and signs the grammar rules out.
    bool matchesGrammar(std::string_view text)
    {
        std::size_t pos = 0;
        if ( pos < text.size() && text[pos] == '-' )
        {
            ++pos;
        }
        std::size_t mantissaDigits = 0;
        while ( pos < text.size() && isDigit(text[pos]) )
        {
            ++pos;
            ++mantissaDigits;
        }
        if ( pos < text.size() && text[pos] == '.' )
        {
            ++pos;
            while ( pos < text.size() && isDigit(text[pos]) )
            {
                ++pos;
                ++mantissaDigits;
            }
        }
        if ( mantissaDigits == 0 )
        {
            return false;
        }
        if ( pos < text.size() && ( text[pos] == 'e' || text[pos] == 'E' ) )
        {
            ++pos;
            if ( pos < text.size() && ( text[pos] == '+' || text[pos] == '-' ) )
            {
                ++pos;
            }
            std::size_t exponentDigits = 0;
            while ( pos < text.size() && isDigit(text[pos]) )
            {
                ++pos;
                ++exponentDigits;
            }
            if ( exponentDigits == 0 )
            {
                return false;
            }
        }
        return pos == text.size();
    }

}

    std::optional<double> parseDouble(std::string_view text)
    {
        if ( !matchesGrammar(text) )
        {
            return std::nullopt;
        }
        // std::strtod reads the current C locale's LC_NUMERIC category, so
        // the grammar's '.' is exchanged for that locale's decimal point
        // before the conversion: the result is the same in every locale
        // without being locale-aware. In the classic C locale it already is
        // '.', and the text converts untouched. The substitution reads the
        // locale once; calling setlocale from another thread while this
        // runs is not supported.
        std::string converted(text);
        const char* point = std::localeconv()->decimal_point;
        if ( point[0] != '.' )
        {
            const std::size_t dot = converted.find('.');
            if ( dot != std::string::npos )
            {
                converted[dot] = point[0];   // the grammar has at most one dot
            }
        }
        // The hand check above rules out everything strtod would read
        // differently (hex, "inf", "nan", whitespace, leading '+', trailing
        // characters), so only the conversion and its rounding remain. A
        // finite result is accepted even where underflow to zero or a
        // denormal sets errno ERANGE; overflow lands on HUGE_VAL and is not
        // finite. The rounding is the library's, so the value is correct.
        const char* begin = converted.c_str();
        char* end = nullptr;
        const double value = std::strtod(begin, &end);
        if ( end != begin + converted.size() || !std::isfinite(value) )
        {
            return std::nullopt;   // overflow, e.g. "1e400"
        }
        return value;
    }

}
