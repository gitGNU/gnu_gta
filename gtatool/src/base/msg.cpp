/*
 * msg.cpp
 *
 * This file is part of gtatool, a tool to manipulate Generic Tagged Arrays
 * (GTAs).
 *
 * Copyright (C) 2009, 2010
 * Martin Lambers <marlam@marlam.de>
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

#include <cstdlib>

#include "str.h"
#include "msg.h"


namespace msg
{
    static FILE *_file = stderr;
    static level_t _level = WRN;
    static int _columns = 80;
    static std::string _program_name("");
    static std::string _category_name("");

    /* Get / Set configuration */

    FILE *file()
    {
        return _file;
    }

    void set_file(FILE *f)
    {
        _file = f;
        ::setvbuf(f, NULL, _IOLBF, 0);
    }

    level_t level()
    {
        return _level;
    }

    void set_level(level_t l)
    {
        _level = l;
    }

    int columns()
    {
        return _columns;
    }

    void set_columns(int l)
    {
        _columns = l;
    }

    void set_columns_from_env()
    {
        char *s;
        if ((s = ::getenv("COLUMNS")))
        {
            int c = ::atoi(s);
            if (c > 0)
            {
                set_columns(c);
            }
        }
    }

    std::string program_name()
    {
        return _program_name;
    }

    void set_program_name(const std::string &n)
    {
        _program_name = n;
    }

    std::string category_name()
    {
        return _category_name;
    }

    void set_category_name(const std::string &n)
    {
        _category_name = n;
    }

    /* Print messages */

    static std::string prefix(level_t level)
    {
        static const char *_level_prefixes[] = { "[DBG] ", "[INF] ", "[WRN] ", "[ERR] ", "" };

        if (!_program_name.empty() && !_category_name.empty())
        {
            return _program_name + ": " + _level_prefixes[level] + _category_name + ": ";
        }
        else if (!_program_name.empty() && _category_name.empty())
        {
            return _program_name + ": " + _level_prefixes[level];
        }
        else if (_program_name.empty() && !_category_name.empty())
        {
            return _level_prefixes[level] + _category_name + ": ";
        }
        else
        {
            return _level_prefixes[level];
        }
    }

    void msg(level_t level, const std::string &s)
    {
        if (level < _level)
        {
            return;
        }

        std::string pfx = prefix(level);
        ::fputs(pfx.c_str(), _file);
        ::fputs(s.c_str(), _file);
        ::fputc('\n', _file);
    }

    void msg(level_t level, const char *format, ...)
    {
        if (level < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg(level, s);
    }

    void msg_txt(level_t level, const std::string &s)
    {
        if (level < _level)
        {
            return;
        }

        std::string pfx = prefix(level);
        int pfx_len = pfx.length();
        std::string text(s);

        int line_len = 0;
        int first_unprinted = 0;
        int last_blank = -1;
        bool end_of_text = false;
        for (int text_index = 0; !end_of_text; text_index++)
        {
            if (text[text_index] == '\0')
            {
                text[text_index] = '\n';
                end_of_text = true;
            }
            if (text[text_index] == '\n')
            {
                // output from first_unprinted to text_index
                ::fputs(pfx.c_str(), _file);
                ::fwrite(text.c_str() + first_unprinted, sizeof(char), text_index - first_unprinted + 1, _file);
                first_unprinted = text_index + 1;
                last_blank = -1;
                line_len = 0;
            }
            else
            {
                if (text[text_index] == ' ' || text[text_index] == '\t')
                {
                    last_blank = text_index;
                }
                if (line_len >= _columns - pfx_len)
                {
                    // output from first_unprinted to last_blank (which is replaced
                    // by '\n'), then update first_unprinted.
                    if (last_blank == -1)
                    {
                        // word is too long for line; search next blank and use that
                        do
                        {
                            text_index++;
                        }
                        while (text[text_index] != ' '
                                && text[text_index] != '\t'
                                && text[text_index] != '\n'
                                && text[text_index] != '\0');
                        if (text[text_index] == '\0')
                        {
                            end_of_text = true;
                        }
                        last_blank = text_index;
                    }
                    text[last_blank] = '\n';
                    ::fputs(pfx.c_str(), _file);
                    ::fwrite(text.c_str() + first_unprinted, sizeof(char), last_blank - first_unprinted + 1, _file);
                    first_unprinted = last_blank + 1;
                    last_blank = -1;
                    line_len = text_index - first_unprinted + 1;
                }
                else
                {
                    line_len++;
                }
            }
        }
    }

    void msg_txt(level_t level, const char *format, ...)
    {
        if (level < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg_txt(level, s);
    }

    void dbg(const std::string &s)
    {
        if (DBG < _level)
        {
            return;
        }

        msg(DBG, s);
    }

    void dbg(const char *format, ...)
    {
        if (DBG < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg(DBG, s);
    }

    void dbg_txt(const std::string &s)
    {
        if (DBG < _level)
        {
            return;
        }

        msg_txt(DBG, s);
    }

    void dbg_txt(const char *format, ...)
    {
        if (DBG < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg_txt(DBG, s);
    }

    void inf(const std::string &s)
    {
        if (INF < _level)
        {
            return;
        }

        msg(INF, s);
    }

    void inf(const char *format, ...)
    {
        if (INF < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg(INF, s);
    }

    void inf_txt(const std::string &s)
    {
        if (INF < _level)
        {
            return;
        }

        msg_txt(INF, s);
    }

    void inf_txt(const char *format, ...)
    {
        if (INF < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg_txt(INF, s);
    }

    void wrn(const std::string &s)
    {
        if (WRN < _level)
        {
            return;
        }

        msg(WRN, s);
    }

    void wrn(const char *format, ...)
    {
        if (WRN < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg(WRN, s);
    }

    void wrn_txt(const std::string &s)
    {
        if (WRN < _level)
        {
            return;
        }

        msg_txt(WRN, s);
    }

    void wrn_txt(const char *format, ...)
    {
        if (WRN < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg_txt(WRN, s);
    }

    void err(const std::string &s)
    {
        if (ERR < _level)
        {
            return;
        }

        msg(ERR, s);
    }

    void err(const char *format, ...)
    {
        if (ERR < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg(ERR, s);
    }

    void err_txt(const std::string &s)
    {
        if (ERR < _level)
        {
            return;
        }

        msg_txt(ERR, s);
    }

    void err_txt(const char *format, ...)
    {
        if (ERR < _level)
        {
            return;
        }

        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg_txt(ERR, s);
    }

    void req(const std::string &s)
    {
        msg(REQ, s);
    }

    void req(const char *format, ...)
    {
        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg(REQ, s);
    }

    void req_txt(const std::string &s)
    {
        msg_txt(REQ, s);
    }

    void req_txt(const char *format, ...)
    {
        std::string s;
        va_list args;
        va_start(args, format);
        s = str::vasprintf(format, args);
        va_end(args);
        msg_txt(REQ, s);
    }
}
