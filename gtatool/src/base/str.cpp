/*
 * str.cpp
 *
 * This file is part of gtatool, a tool to manipulate Generic Tagged Arrays
 * (GTAs).
 *
 * Copyright (C) 2009, 2010  Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <cerrno>
#include <limits>
#include <sstream>

#if BASE_WITH_BASE64
extern "C"
{
#   include "base64.h"
}
#endif

#include "debug.h"
#include "msg.h"
#include "tools.h"
#include "blob.h"

#include "str.h"


#if W32
static int vasprintf(char **strp, const char *format, va_list args)
{
    /* vasprintf() is only missing on Windows nowadays.
     * This replacement function only works when the vsnprintf() function is available
     * and its return value is standards compliant. This is true for the MinGW version
     * of vsnprintf(), but not for Microsofts version (Visual Studio etc.)!
     */
    int length = vsnprintf(NULL, 0, format, args);
    if (length + 1 < length     // integer overflow
            || !(*strp = static_cast<char *>(malloc(length + 1))))
    {
        return -1;
    }
    vsnprintf(*strp, length + 1, format, args);
    return length;
}
#endif

/* Convert an unsigned integer to a string.
 * We hide this function so that it cannot be called with invalid arguments. */
template<typename T>
static inline std::string uint_to_str(T x)
{
    std::string s;
    do
    {
        // this assumes an ASCII-compatible character set
        s.insert(0, 1, '0' + x % 10);
        x /= 10;
    }
    while (x != 0);
    return s;
}

/* Convert a signed integer to a string.
 * We hide this function so that it cannot be called with invalid arguments. */
template<typename T>
static inline std::string int_to_str(T x)
{
    std::string s;
    bool negative = (x < 0);
    do
    {
        // this assumes an ASCII-compatible character set
        s.insert(0, 1, (negative ? ('0' - x % 10) : ('0' + x % 10)));
        x /= 10;
    }
    while (x != 0);
    if (negative)
    {
        s.insert(0, 1, '-');
    }
    return s;
}

/* Convert a floating point number (float, double, long double) to a string.
 * We hide this function so that it cannot be called with invalid arguments. */
template<typename T>
static inline std::string float_to_str(T x)
{
    std::ostringstream os;
    os.precision(std::numeric_limits<T>::digits10 + 2);
    os << x;
    return os.str();
}


namespace str
{
    /* Sanitize a string (replace control characters with '?') */

    std::string sanitize(const std::string &s)
    {
        std::string sane(s);
        for (size_t i = 0; i < sane.length(); i++)
        {
            if (iscntrl(static_cast<unsigned char>(sane[i])))
            {
                sane[i] = '?';
            }
        }
        return sane;
    }

    /* Create std::strings from all basic data types */

    std::string str(bool x)
    {
        return std::string(x ? "0" : "1");
    }

    std::string str(signed char x)
    {
        return int_to_str(x);
    }

    std::string str(unsigned char x)
    {
        return uint_to_str(x);
    }

    std::string str(short x)
    {
        return int_to_str(x);
    }

    std::string str(unsigned short x)
    {
        return uint_to_str(x);
    }

    std::string str(int x)
    {
        return int_to_str(x);
    }

    std::string str(unsigned int x)
    {
        return uint_to_str(x);
    }

    std::string str(long x)
    {
        return int_to_str(x);
    }

    std::string str(unsigned long x)
    {
        return uint_to_str(x);
    }

    std::string str(long long x)
    {
        return int_to_str(x);
    }

    std::string str(unsigned long long x)
    {
        return uint_to_str(x);
    }

    std::string str(float x)
    {
        return float_to_str(x);
    }

    std::string str(double x)
    {
        return float_to_str(x);
    }

    std::string str(long double x)
    {
        return float_to_str(x);
    }


    /* Create std::strings printf-like */

    std::string vasprintf(const char *format, va_list args)
    {
        char *cstr;
        if (::vasprintf(&cstr, format, args) < 0)
        {
            /* Should never happen (out of memory or some invalid conversions).
             * We do not want to throw an exception because this function might
             * be called because of an exception. Inform the user instead. */
            msg::err("FAILURE IN str::vasprintf().");
            debug::crash();
        }
        std::string s(cstr);
        free(cstr);
        return s;
    }

    std::string asprintf(const char *format, ...)
    {
        std::string s;
        va_list args;
        va_start(args, format);
        s = vasprintf(format, args);
        va_end(args);
        return s;
    }

    /* Replace all occurences */

    /**
     * \param str       The string in which the replacements will be made.
     * \param s         The string that will be replaced.
     * \param r         The replacement for the string s.
     * \return          The resulting string.
     *
     * Replaces all occurences of \a s in \a str with \a r.
     * Returns \a str.
     */
    std::string &replace(std::string &str, const std::string &s, const std::string &r)
    {
        size_t s_len = s.length();
        size_t r_len = r.length();
        size_t p = 0;

        while ((p = str.find(s, p)) != std::string::npos)
        {
            str.replace(p, s_len, r);
            p += r_len;
        }
        return str;
    }

    /* Create a hex string from binary data */

    std::string hex(const std::string &s, bool uppercase)
    {
        return hex(reinterpret_cast<const void *>(s.c_str()), s.length(), uppercase);
    }

    std::string hex(const void *buf, size_t n, bool uppercase)
    {
        static const char hex_chars_lower[] = "0123456789abcdef";
        static const char hex_chars_upper[] = "0123456789ABCDEF";

        std::string s;
        s.resize(2 * n);

        const char *hex_chars = uppercase ? hex_chars_upper : hex_chars_lower;
        const uint8_t *buffer = static_cast<const uint8_t *>(buf);
        for (size_t i = 0; i < n; i++)
        {
            s[2 * i + 0] = hex_chars[buffer[i] >> 4];
            s[2 * i + 1] = hex_chars[buffer[i] & 0x0f];
        }

        return s;
    }

    /* BASE64 encoding / decoding */

#if BASE_WITH_BASE64

    std::string to_base64(const std::string &s) throw (exc)
    {
        return to_base64(reinterpret_cast<const void *>(s.c_str()), s.length());
    }

    std::string to_base64(const void *buf, const size_t n) throw (exc)
    {
        size_t nn = n;
        size_t l = BASE64_LENGTH(nn) + 1;
        if (nn > l)
        {
            // size_t overflow (see comments in base64.c, function
            // base64_encode_alloc, for the reasons why this check is sufficient).
            throw exc("Cannot encode base64", ENOMEM);
        }
        blob b64buf(l);
        base64_encode(reinterpret_cast<const char *>(buf), nn, b64buf.ptr<char>(), l);
        std::string s(b64buf.ptr<char>());
        return s;
    }

    std::string from_base64(const std::string &b64) throw (exc)
    {
        const size_t b64_len = b64.length();
        size_t bin_len = 3 * (b64_len / 4) + 2;
        blob bin(bin_len);
        bool ok = base64_decode(b64.c_str(), b64_len, bin.ptr<char>(), &bin_len);
        if (!ok)
        {
            throw exc("Cannot decode base64", EINVAL);
        }
        std::string s(bin.ptr<char>(), bin_len);
        return s;
    }

#endif

    /* Convert various values to human readable strings */

    std::string human_readable_memsize(const uintmax_t size)
    {
        const double dsize = static_cast<double>(size);
        const uintmax_t u1024 = static_cast<uintmax_t>(1024);

        if (size >= u1024 * u1024 * u1024 * u1024)
        {
            return str::asprintf("%.2f TiB", dsize / static_cast<double>(u1024 * u1024 * u1024 * u1024));
        }
        else if (size >= u1024 * u1024 * u1024)
        {
            return str::asprintf("%.2f GiB", dsize / static_cast<double>(u1024 * u1024 * u1024));
        }
        else if (size >= u1024 * u1024)
        {
            return str::asprintf("%.2f MiB", dsize / static_cast<double>(u1024 * u1024));
        }
        else if (size >= u1024)
        {
            return str::asprintf("%.2f KiB", dsize / static_cast<double>(u1024));
        }
        else if (size > 1 || size == 0)
        {
            return str::asprintf("%d bytes", static_cast<int>(size));
        }
        else
        {
            return std::string("1 byte");
        }
    }

    std::string human_readable_length(const double length)
    {
        const double abslength = std::abs(length);
        if (abslength >= 1000.0)
        {
            return str::asprintf("%.1f km", length / 1000.0);
        }
        else if (abslength >= 1.0)
        {
            return str::asprintf("%.1f m", length);
        }
        else if (abslength >= 0.01)
        {
            return str::asprintf("%.1f cm", length * 100.0);
        }
        else if (abslength <= 0.0)
        {
            return std::string("0 m");
        }
        else
        {
            return str::asprintf("%.1f mm", length * 1000.0);
        }
    }
}
