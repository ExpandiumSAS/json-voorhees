/** \file
 *  
 *  Copyright (c) 2014 by Travis Gockel. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify it under the terms of the Apache License
 *  as published by the Apache Software Foundation, either version 2 of the License, or (at your option) any later
 *  version.
 *
 *  \author Travis Gockel (travis@gockelhut.com)
**/
#include <jsonv/detail/token_patterns.hpp>

#include <algorithm>
#include <iterator>

#include JSONV_REGEX_INCLUDE

namespace jsonv
{
namespace detail
{

namespace regex_ns = JSONV_REGEX_NAMESPACE;

const regex_ns::regex_constants::syntax_option_type syntax_options = regex_ns::regex_constants::ECMAScript
                                                                   | regex_ns::regex_constants::optimize;

const regex_ns::regex re_number(      R"(^-?[0-9]+(\.[0-9]+)?([eE]-?[0-9]+(\.[0-9]+)?)?)", syntax_options);
const regex_ns::regex re_string(      R"(^\"([^\]|\["\/bfnrt]|\\u[0-9a-fA-F]{4})*\")", syntax_options);
const regex_ns::regex re_whitespace(  R"(^[ \t\r\n]+)", syntax_options);
const regex_ns::regex re_comment(     R"(^/\*([^\*]*|\*[^/])*\*/)", syntax_options);

template <std::ptrdiff_t N>
static match_result match_literal(const char* begin, const char* end, const char (& literal)[N], std::size_t& length)
{
    for (length = 0; length < (N-1); ++length)
    {
        if (begin + length == end)
            return match_result::incomplete_eof;
        else if (begin[length] != literal[length])
            return match_result::unmatched;
    }
    return match_result::complete;
}

static match_result match_true(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::boolean;
    return match_literal(begin, end, "true", length);
}

static match_result match_false(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::boolean;
    return match_literal(begin, end, "false", length);
}

static match_result match_null(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::null;
    return match_literal(begin, end, "null", length);
}

static match_result match_pattern(const char* begin,
                                  const char* end,
                                  const regex_ns::regex& pattern,
                                  std::size_t& length
                                 )
{
    regex_ns::cmatch match;
    if (regex_ns::regex_search(begin, end, match, pattern))
    {
        length = match.length(0);
        return begin + length == end ? match_result::complete_eof : match_result::complete;
    }
    else
    {
        length = 1;
        return match_result::unmatched;
    }
}

static match_result match_number(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::number;
    return match_pattern(begin, end, re_number, length);
}

static match_result match_string(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::string;
    return match_pattern(begin, end, re_string, length);
}

static match_result match_whitespace(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::whitespace;
    return match_pattern(begin, end, re_whitespace, length);
}

static match_result match_comment(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    kind = token_kind::comment;
    return match_pattern(begin, end, re_comment, length);
}

match_result attempt_match(const char* begin, const char* end, token_kind& kind, std::size_t& length)
{
    auto result = [&] (match_result r, token_kind kind_, std::size_t length_)
                  {
                      kind = kind_;
                      length = length_;
                      return r;
                  };
    
    if (begin == end)
    {
        return result(match_result::incomplete_eof, token_kind::unknown, 0);
    }
    
    switch (*begin)
    {
    case '[': return result(match_result::complete, token_kind::array_begin,          1);
    case ']': return result(match_result::complete, token_kind::array_end,            1);
    case '{': return result(match_result::complete, token_kind::object_begin,         1);
    case '}': return result(match_result::complete, token_kind::object_end,           1);
    case ':': return result(match_result::complete, token_kind::object_key_delimiter, 1);
    case ',': return result(match_result::complete, token_kind::separator,            1);
    case 't': return match_true( begin, end, kind, length);
    case 'f': return match_false(begin, end, kind, length);
    case 'n': return match_null( begin, end, kind, length);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return match_number(begin, end, kind, length);
    case '\"':
        return match_string(begin, end, kind, length);
    case ' ':
    case '\t':
    case '\n':
    case '\r':
        return match_whitespace(begin, end, kind, length);
    case '/':
        return match_comment(begin, end, kind, length);
    default:
        return result(match_result::unmatched, token_kind::unknown, 1);
    }
}

}
}
