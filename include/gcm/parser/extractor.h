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

#include <utility>

#include "base.h"

namespace gcm {
namespace parser {

template<typename Rule, typename Extractor>
class extractor_rule_t: public rule_base {
public:
    template<typename R, typename E>
    extractor_rule_t(R &&rule, E &&extractor):
        rule_base(),
        rule(std::forward<R>(rule)),
        extractor(std::forward<E>(extractor))
    {}

    template<typename I>
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        I begin1 = begin;
        if (rule(begin1, end)) {
            extractor(begin, begin1);
            begin = begin1;
            return true;
        }

        return false;
    }

    operator std::string() const {
        return "(" + (std::string)rule + ") >> extract";
    }

protected:
    Rule rule;
    Extractor extractor;
};

template<typename Rule, typename Extractor, typename = std::enable_if_t<is_rule<Rule>::value>>
inline auto extractor_rule(Rule &&rule, Extractor &&extractor) {
    return extractor_rule_t<std::decay_t<Rule>, std::decay_t<Extractor>>(
        std::forward<Rule>(rule),
        std::forward<Extractor>(extractor)
    );
}

} // namespace parser
} // namespace gcm
