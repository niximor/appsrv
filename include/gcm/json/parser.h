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
 * @date 2014-10-28
 *
 */

#pragma once

#include <functional>

#include <gcm/parser/parser.h>

#include "json.h"

namespace gcm {
namespace json {

template<typename I>
std::shared_ptr<Value> parse(I begin, I end) {
    using namespace gcm::parser;
    using namespace std::placeholders;

    struct parser {
        using ParentPair = std::pair<ParentPair *, std::shared_ptr<Value>>;

        parser(): value(std::make_shared<Value>()), current(std::make_pair(nullptr, value.get()))
        {}

        template<typename I>
        void null_value(I, I) {
            current.second = Value();
            current = *current.first;
        }

        template<typename I>
        void true_value(I, I) {
            current.second = Bool(true);
            current = *current.first;
        }

        template<typename I>
        void false_value(I, I) {
            current.second = Bool(false);
            current = *current.first;
        }

        template<typename I>
        void int_value(I begin, I end) {
            current.second = Int(std::stoi(std::string(begin, end)));
            current = *current.first;
        }

        template<typename I>
        void array_begin(I, I) {
            current.second = Array();
            // TODO: This won't work, need to think of a better way... probably some wrapper object
            // that will hold current item's parent.
            current = std::make_pair(current, current.second);
        }

        std::shared_ptr<Value> value;
        ParentPair current;
    };
    parser p;

    rule<I> value;

    auto v_null = "null"_r >> std::bind(&parser::null_value, p, _1, _2);
    auto v_bool = "true"_r >> std::bind(&parser::true_value, p, _1, _2) | "false"_r >> std::bind(&parser::false_value, p, _1, _2);;
    auto v_int = (-('+'_r | '-'_r) & +digit()) >> std::bind(&parser::int_value, p, _1, _2);
    auto value_in_array = !']'_r >> std::bind(&parser::new_array_item, p, _1, _2) & value;
    auto v_array =
        '['_r >> std::bind(&parser::array_begin, p, _1, _2)
        & *(value_in_array & ','_r)
        & -value_in_array
        & ']'_r >> std::bind(&parser::array_end, p, _1, _2);

    value = v_null | v_bool | v_double | v_int | v_string | v_array | v_object;

    if (value(begin, end)) {
        return p.value;
    }
}

}
}