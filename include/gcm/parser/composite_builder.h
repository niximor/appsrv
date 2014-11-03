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

#include "composite.h"

namespace gcm {
namespace parser {

template<typename Rule1, typename Rule2, typename = std::enable_if_t<is_rule<Rule1>::value>, typename = std::enable_if_t<is_rule<Rule2>::value>>
inline auto and_rule(Rule1 &&r1, Rule2 &&r2) {
    return and_rule_t<std::decay_t<Rule1>, std::decay_t<Rule2>>(
        std::forward<Rule1>(r1),
        std::forward<Rule2>(r2)
    );
}

template<typename Rule1, typename Rule2, typename = std::enable_if_t<is_rule<Rule1>::value>, typename = std::enable_if_t<is_rule<Rule2>::value>>
inline auto or_rule(Rule1 &&r1, Rule2 &&r2) {
    return or_rule_t<std::decay_t<Rule1>, std::decay_t<Rule2>>(
        std::forward<Rule1>(r1),
        std::forward<Rule2>(r2)
    );
}

template<typename Rule1, typename Rule2, typename = std::enable_if_t<is_rule<Rule1>::value>, typename = std::enable_if_t<is_rule<Rule2>::value>>
inline auto exception_rule(Rule1 &&r1, Rule2 &&r2) {
    return exception_rule_t<std::decay_t<Rule1>, std::decay_t<Rule2>>(
        std::forward<Rule1>(r1),
        std::forward<Rule2>(r2)
    );
}

template<typename Rule, typename = std::enable_if_t<is_rule<Rule>::value>>
inline auto iteration_rule(Rule &&r, int min = 0, int max = -1) {
    return iteration_rule_t<std::decay_t<Rule>>(
        std::forward<Rule>(r),
        min,
        max
    );
}

} // namespace parser
} // namespace gcm
