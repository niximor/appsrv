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

inline auto operator "" _r(const char ch) {
    return char_rule(ch);
}

inline auto operator "" _r(const char *lit) {
    return literal_rule(std::forward<std::string>(lit));
}

inline auto operator "" _r(const char lit[], size_t len) {
    return literal_rule(std::forward<std::string>({lit, len}));
}


template<typename Rule1>
inline auto operator|(Rule1 &&r1, const char ch) {
    return or_rule(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
inline auto operator|(const char ch, Rule2 &&r2) {
    return or_rule(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule1>
inline auto operator|(Rule1 &&r1, std::string &&lit) {
    return or_rule(std::forward<Rule1>(r1), literal_rule(std::forward<std::string>(lit)));
}

template<typename Rule1>
inline auto operator|(Rule1 &&r1, const char *lit) {
    return or_rule(std::forward<Rule1>(r1), literal_rule(lit));
}

template<typename Rule2>
inline auto operator|(std::string &&lit, Rule2 &&r2) {
    return or_rule(literal_rule(std::forward<std::string>(lit)), std::forward<Rule2>(r2));
}

template<typename Rule2>
inline auto operator|(const char *lit, Rule2 &&r2) {
    return or_rule(literal_rule(lit), std::forward<Rule2>(r2));
}

template<typename Rule1>
inline auto operator&(Rule1 &&r1, char ch) {
    return and_rule(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
inline auto operator&(char ch, Rule2 &&r2) {
    return and_rule(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule1>
inline auto operator&(Rule1 &&r1, std::string &&lit) {
    return and_rule(std::forward<Rule1>(r1), literal_rule(std::forward<std::string>(lit)));
}

template<typename Rule1>
inline auto operator&(Rule1 &&r1, const char *lit) {
    return and_rule(std::forward<Rule1>(r1), literal_rule(lit));
}

template<typename Rule2>
inline auto operator&(std::string &&lit, Rule2 &&r2) {
    return and_rule(literal_rule(std::forward<std::string>(lit)), std::forward<Rule2>(r2));
}

template<typename Rule2>
inline auto operator&(const char *lit, Rule2 &&r2) {
    return and_rule(literal_rule(lit), std::forward<Rule2>(r2));
}

template<typename Rule1>
inline auto operator-(Rule1 &&r1, const char ch) {
    return exception_rule(std::forward<Rule1>(r1), char_rule(ch));
}

template<typename Rule2>
inline auto operator-(const char ch, Rule2 &&r2) {
    return exception_rule(char_rule(ch), std::forward<Rule2>(r2));
}

template<typename Rule1>
inline auto operator-(Rule1 &&r1, std::string &&lit) {
    return exception_rule(std::forward<Rule1>(r1), literal_rule(std::forward<std::string>(lit)));
}

template<typename Rule1>
inline auto operator-(Rule1 &&r1, const char *lit) {
    return exception_rule(std::forward<Rule1>(r1), literal_rule(lit));
}

template<typename Rule2>
inline auto operator-(std::string &&lit, Rule2 &&r2) {
    return exception_rule(literal_rule(std::forward<std::string>(lit)), std::forward<Rule2>(r2));
}

template<typename Rule2>
inline auto operator-(const char *lit, Rule2 &&r2) {
    return exception_rule(lit, std::forward<Rule2>(r2));
}

} // namespace parser
} // namespace gcm
