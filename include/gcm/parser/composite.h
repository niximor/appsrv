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

#include "base.h"

namespace gcm {
namespace parser {

template<typename Rule1, typename Rule2>
class and_rule_t: public rule_base {
public:
    template<typename R1, typename R2>
    and_rule_t(R1 &&r1, R2 &&r2): rule_base(), r1(std::forward<R1>(r1)), r2(std::forward<R2>(r2))
    {}

    template<typename I>
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif
        return r1(begin, end) && r2(begin, end);
    }

    operator std::string() const {
        return "a(" + (std::string)r1 + " & " + (std::string)r2 + ")";
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule1, typename Rule2>
class or_rule_t: public rule_base {
public:
    template<typename R1, typename R2>
    or_rule_t(R1 &&r1, R2 &&r2): rule_base(), r1(std::forward<R1>(r1)), r2(std::forward<R2>(r2))
    {}

    template<typename I>
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        return r1(begin, end) || r2(begin, end);
    }

    operator std::string() const {
        return "o(" + (std::string)r1 + " | " + (std::string)r2 + ")";
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule1, typename Rule2>
class exception_rule_t: public rule_base {
public:
    template<typename R1, typename R2>
    exception_rule_t(R1 &&r1, R2 &&r2): rule_base(), r1(std::forward<R1>(r1)), r2(std::forward<R2>(r2))
    {}

    template<typename I>
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        I begin1 = begin;
        I begin2 = begin;

        if (r1(begin1, end) && !r2(begin2, end)) {
            begin = begin1;
            return true;
        } else {
            return false;
        }
    }

    operator std::string() const {
        return "e(" + (std::string)r1 + " - " + (std::string)r2 + ")";
    }

protected:
    Rule1 r1;
    Rule2 r2;
};

template<typename Rule>
class iteration_rule_t: public rule_base {
public:
    template<typename R>
    iteration_rule_t(R &&rule, int min = 0, int max = -1): rule_base(), rule(std::forward<R>(rule)), min(min), max(max)
    {}

    template<typename I>
    bool operator()(I &begin, I &end) const {
#ifdef PARSER_DEBUG
        DEBUG(this->log) << "Calling " << (std::string)*this << " on " << std::string(begin, end);
#endif

        int count = 0;

        I begin1 = begin;

        if (max >= 0) {
            while (count < max && rule(begin1, end)) {
                ++count;
            }
        } else {
            while (rule(begin1, end)) {
                ++count;
            }
        }

        if (count >= min) {
            begin = begin1;
            return true;
        }

        return false;
    }

    operator std::string() const {
        return "*(" + (std::string)rule + ")";
    }

protected:
    Rule rule;
    int min;
    int max;
};

} // namespace parser
} // namespace gcm
