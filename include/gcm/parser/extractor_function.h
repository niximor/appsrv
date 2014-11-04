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
#include "extractor.h"

#include <utility>

namespace gcm {
namespace parser {

template<typename Rule, typename Extractor>
std::enable_if_t<is_rule<Rule>::value, extractor_rule_t<std::decay_t<Rule>, std::decay_t<Extractor>>>
operator>>(Rule &&rule, Extractor &&extractor) {
    return extractor_rule(
        std::forward<Rule>(rule),
        std::forward<Extractor>(extractor)
    );
}

template<typename Rule, typename Error>
std::enable_if_t<is_rule<Rule>::value, error_rule_t<std::decay_t<Rule>, std::decay_t<Error>>>
operator/(Rule &&rule, Error &&error) {
    return error_rule(
        std::forward<Rule>(rule),
        std::forward<Error>(error)
    );
}

}
}