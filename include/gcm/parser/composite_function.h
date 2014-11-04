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
#include "composite.h"
#include "composite_builder.h"

#include <utility>

namespace gcm {
namespace parser {

template<typename Rule1, typename Rule2>
std::enable_if_t<is_rule<Rule1>::value && is_rule<Rule2>::value, and_rule_t<std::decay_t<Rule1>, std::decay_t<Rule2>>>
operator&(Rule1 &&r1, Rule2 &&r2) {
    return and_rule(
        std::forward<Rule1>(r1),
        std::forward<Rule2>(r2)
    );
}

template<typename Rule1, typename Rule2>
std::enable_if_t<is_rule<Rule1>::value && is_rule<Rule2>::value, or_rule_t<std::decay_t<Rule1>, std::decay_t<Rule2>>>
operator|(Rule1 &&r1, Rule2 &&r2) {
    return or_rule(
        std::forward<Rule1>(r1),
        std::forward<Rule2>(r2)
    );
}

template<typename Rule1, typename Rule2>
std::enable_if_t<is_rule<Rule1>::value && is_rule<Rule2>::value, exception_rule_t<std::decay_t<Rule1>, std::decay_t<Rule2>>>
operator-(Rule1 &&r1, Rule2 &&r2) {
    return exception_rule(
        std::forward<Rule1>(r1),
        std::forward<Rule2>(r2)
    );
}

template<typename Rule>
std::enable_if_t<is_rule<Rule>::value, iteration_rule_t<std::decay_t<Rule>>>
operator*(Rule &&rule) {
    return iteration_rule(
        std::forward<Rule>(rule),
        0,
        -1
    );
}

template<typename Rule>
std::enable_if_t<is_rule<Rule>::value, iteration_rule_t<std::decay_t<Rule>>>
operator+(Rule &&rule) {
    return iteration_rule(
        std::forward<Rule>(rule),
        1,
        -1
    );
}

template<typename Rule>
iteration_rule_t<std::decay_t<Rule>> 
operator-(Rule &&rule) {
    return iteration_rule(
        std::forward<Rule>(rule),
        0,
        1
    );
}

template<typename Rule>
not_rule_t<std::decay_t<Rule>>
operator!(Rule &&rule) {
    return not_rule(std::forward<Rule>(rule));
}

} // namespace parser
} // namespace gcm
