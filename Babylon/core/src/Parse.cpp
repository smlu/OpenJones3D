// SPDX-License-Identifier: MPL-2.0
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <babylon/core/Parse.h>

#include <cmath>
#include <cstddef>
#include <locale>
#include <sstream>
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
    // stream extraction would also accept leading whitespace, hex-like
    // prefixes it stops at and signs or separators the grammar rules out.
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
        // The hand check above rules out everything the stream conversion
        // reads differently; what is left is a plain decimal whose rounding
        // is the library's, so the result is correctly rounded. The classic
        // locale pins the decimal point and the accepted spellings.
        std::istringstream in{std::string(text)};
        in.imbue(std::locale::classic());
        double value = 0.0;
        if ( !(in >> value) || in.peek() != std::istringstream::traits_type::eof() )
        {
            return std::nullopt;   // overflow, e.g. "1e400"
        }
        if ( !std::isfinite(value) )
        {
            return std::nullopt;
        }
        return value;
    }

}
