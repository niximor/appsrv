/**
 * Copyright 2014 Michal Kuchta <niximor@gmail.com>
 *
 * This file is part of GCM::AppSrv.
 *
 * GCM::AppSrv is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * GCM::AppSrv is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with GCM::AppSrv. If not, see http://www.gnu.org/licenses/.
 *
 * @author Michal Kuchta <niximor@gmail.com>
 * @date 2014-11-02
 *
 */

#pragma once

#include <string>
#include <cctype>

#include "base.h"

namespace gcm {
namespace parser {

class any_rule: public rule_base {
public:
    template<typename I>
    bool operator()(I &begin, I&end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (begin == end) {
            return false;
        }

        ++begin;
        return true;
    }

    operator std::string() const {
        return "any";
    }
};

class char_rule: public rule_base {
public:
    char_rule(const char ch): rule_base(), ch(ch)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (begin == end) {
            return false;
        }

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Got input '" << *begin << "', matching against " << ch;
#endif

        if (*begin == ch) {
            ++begin;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "OK";
#endif

            return true;
        }
        return false;
    }

    operator std::string() const {
        return std::string("'") + ch + "'";
    }

protected:
    const char ch;
};

using call_type = int(int);

template<call_type call>
class ctype_rule: public rule_base {
public:
    template<typename I>
    bool operator()(I &begin, I&end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (begin == end) return false;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Got input '" << *begin << "', matching against ctype.";
#endif

        if (call(*begin) > 0) {
            ++begin;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "OK";
#endif

            return true;
        } else {
            return false;
        }
    }

    operator std::string() const {
        return std::string("ctype(") + typeid(call).name() + ")";
    }
};

class literal_rule: public rule_base {
public:
    literal_rule(const std::string &lit): rule_base(), lit(lit)
    {}

    literal_rule(std::string &&lit): rule_base(), lit(std::move(lit))
    {}

    template<typename I>
    bool operator()(I &begin, I&end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif
        I begin1 = begin;

        for (auto it = lit.begin(); it != lit.end(); ++it) {
            if (begin1 == end) {
                return false;
            }

            if (*begin1 != *it) {
                return false;
            }
            ++begin1;
        }

        begin = begin1;

#ifdef PARSER_DEBUG
        DEBUG(this->log) << "OK";
#endif

        return true;
    }

    operator std::string() const {
        return "\"" + lit + "\"";
    }

protected:
    const std::string lit;
};

class space: public ctype_rule<std::isspace> {
public:
    operator std::string() const { return "space"; };
};

class alpha: public ctype_rule<std::isalpha> {
public:
    operator std::string() const { return "alpha"; };
};

class alnum: public ctype_rule<std::isalnum> {
public:
    operator std::string() const { return "alnum"; };
};

class digit: public ctype_rule<std::isdigit> {
public:
    operator std::string() const { return "digit"; };
};

class xdigit: public ctype_rule<std::isxdigit> {
public:
    operator std::string() const { return "xdigit"; };
};

class print: public ctype_rule<std::isprint> {
public:
    operator std::string() const { return "print"; };
};

} // namespace parser
} // namespace gcm
