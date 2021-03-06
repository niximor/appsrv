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
#include <functional>
#include <memory>

#include "base.h"

namespace gcm {
namespace parser {

template<typename I>
class rule: public rule_base {
public:
    using Func = std::function<bool(I&,I&)>;

    // Default constructor, no action in this rule.
    rule(): rule_base(), func(std::make_shared<Func>(nullptr))
    {}

    template<typename Rule>
    rule(std::enable_if_t<is_rule<Rule>::value, Rule &&> r):
        rule_base(),
        func(std::make_shared<Func>(std::forward<Rule>(r)))
    {}

    // Default constructors
    rule(rule &&) = default;
    rule(const rule &) = default;

    // Assignment operator
    rule<I> &operator=(const rule<I> &other) = default;
    rule<I> &operator=(rule<I> &&other) = default;

    template<typename Rule>
    std::enable_if_t<is_rule<Rule>::value, rule<I> &>
    operator=(Rule && r) {
        *func = std::forward<Rule>(r);
        return *this;
    }

    template<typename Rule>
    std::enable_if_t<is_rule<Rule>::value, rule<I> &>
    operator=(const Rule & r) {
        *func = r;
        return *this;
    }

    // When calling rule, evaluate the function that actualy does the matching.
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        if (*func != nullptr) {
            return (*func)(begin, end);
        } else {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Empty " << (std::string)*this;
#endif
            return true;
        }
    }

    operator std::string() const {
        return "generic_rule";
    }

protected:
    std::shared_ptr<Func> func;
};

}
}