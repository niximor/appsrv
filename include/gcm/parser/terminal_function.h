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

#include "base.h"
#include "terminal.h"
#include "composite.h"

#include <utility>
#include <string>

namespace gcm {
namespace parser {

template<typename Rule1>
or_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    char_rule
> operator|(Rule1 &&r1, const char ch) {
    return or_rule<Rule1, char_rule>(std::forward<Rule1>(r1), std::forward<char_rule>(char_rule(ch)));
}

template<typename Rule2>
or_rule<
    char_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator|(const char ch, Rule2 &&r2) {
    return or_rule<char_rule, Rule2>(std::forward<char_rule>(char_rule(ch)), std::forward<Rule2>(r2));
}

template<typename Rule1>
or_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    literal_rule
> operator|(Rule1 &&r1, std::string &&lit) {
    return or_rule<Rule1, literal_rule>(std::forward<Rule1>(r1), std::forward<literal_rule>(literal_rule(std::forward<std::string>(lit))));
}

template<typename Rule2>
or_rule<
    literal_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator|(std::string &&lit, Rule2 &&r2) {
    return or_rule<literal_rule, Rule2>(std::forward<literal_rule>(literal_rule(std::forward<std::string>(lit))), std::forward<Rule2>(r2));
}

template<typename Rule1>
and_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    char_rule
> operator&(Rule1 &&r1, char ch) {
    return and_rule<Rule1, char_rule>(std::forward<Rule1>(r1), std::forward<char_rule>(char_rule(ch)));
}

template<typename Rule2>
and_rule<
    char_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator&(char ch, Rule2 &&r2) {
    return and_rule<char_rule, Rule2>(std::forward<char_rule>(char_rule(ch)), std::forward<Rule2>(r2));
}

template<typename Rule1>
and_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    literal_rule
> operator&(Rule1 &&r1, std::string &&lit) {
    return and_rule<Rule1, literal_rule>(std::forward<Rule1>(r1), std::forward<literal_rule>(literal_rule(std::forward<std::string>(lit))));
}

template<typename Rule2>
and_rule<
    literal_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator&(std::string &&lit, Rule2 &&r2) {
    return and_rule<literal_rule, Rule2>(std::forward<literal_rule>(literal_rule(std::forward<std::string>(lit))), std::forward<Rule2>(r2));
}

template<typename Rule1>
exception_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    char_rule
> operator-(Rule1 &&r1, const char ch) {
    return exception_rule<Rule1, char_rule>(std::forward<Rule1>(r1), std::forward<char_rule>(char_rule(ch)));
}

template<typename Rule2>
exception_rule<
    char_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator-(const char ch, Rule2 &&r2) {
    return exception_rule<char_rule, Rule2>(std::forward<char_rule>(char_rule(ch)), std::forward<Rule2>(r2));
}

template<typename Rule1>
exception_rule<
    std::enable_if_t<is_rule<Rule1>::value, Rule1>,
    literal_rule
> operator-(Rule1 &&r1, std::string &&lit) {
    return exception_rule<Rule1, literal_rule>(std::forward<Rule1>(r1), std::forward<literal_rule>(literal_rule(std::forward<std::string>(lit))));
}

template<typename Rule2>
exception_rule<
    literal_rule,
    std::enable_if_t<is_rule<Rule2>::value, Rule2>
> operator-(std::string &&lit, Rule2 &&r2) {
    return exception_rule<literal_rule, Rule2>(std::forward<literal_rule>(literal_rule(std::forward<std::string>(lit))), std::forward<Rule2>(r2));
}

}
}