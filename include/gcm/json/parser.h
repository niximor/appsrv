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
#include <gcm/logging/logging.h>

#include "json.h"

// Define JSON_PARSER_DEBUG to see debug output from the parser.
//#define JSON_PARSER_DEBUG

namespace gcm {
namespace json {

class ParseException: public Exception {
public:
    ParseException(std::string message): Exception(message)
    {}
};

struct parser {
    struct Item {
        JsonValue *value;
        std::shared_ptr<Item> parent;
    };

    parser():
        current(std::make_shared<Item>()),
        stop(false),
        log(gcm::logging::getLogger("GCM.json.parser"))
    {
        current->value = &value;
    }

    void level_up() {
        if (current->parent) {
            current = current->parent;
        } else {
            stop = true;
        }
    }

    void test_stop() {
        if (stop) {
            throw ParseException("Unexpected input after JSON value.");
        }
    }

    template<typename I>
    void null_value(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "null value";
#endif
        *(current->value) = make_null();
        level_up();
    }

    template<typename I>
    void true_value(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "true value";
#endif
        *(current->value) = make_bool(true);
        level_up();
    }

    template<typename I>
    void false_value(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "false value";
#endif
        *(current->value) = make_bool(false);
        level_up();
    }

    template<typename I>
    void int_value(I begin, I end) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "int value " << std::string(begin, end);
#endif
        *(current->value) = make_int(std::stoi(std::string(begin, end)));
        level_up();
    }

    template<typename I>
    void double_value(I begin, I end) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "double value " << std::string(begin, end);
#endif
        *(current->value) = make_double(std::stod(std::string(begin, end)));
        level_up();
    }

    template<typename I>
    void string_value(I begin, I end) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "string value " << std::string(begin, end);
#endif
        *(current->value) = make_string(std::string(begin, end));
        level_up();
    }

    template<typename I>
    void array_begin(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "array begin";
#endif
        *(current->value) = make_array();
    }

    template<typename I>
    void new_array_item(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "array item";
#endif
        auto &arr = to<Array>(*(current->value));
        arr.push_back(make_null());

        auto item = std::make_shared<Item>();
        item->parent = current;
        item->value = &arr.back();
        current = item;
    }

    template<typename I>
    void array_end(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "array end";
#endif
        level_up();
    }

    template<typename I>
    void obj_begin(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "obj begin";
#endif
        *(current->value) = make_object();
    }

    template<typename I>
    void new_obj_item(I begin, I end) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "new obj item: " << std::string(begin, end);
#endif
        auto &obj = to<Object>(*(current->value));
        
        auto item = std::make_shared<Item>();
        item->value = &(obj[std::string(begin, end)] = make_null());
        item->parent = current;

        current = item;
    }

    template<typename I>
    void obj_end(I, I) {
        test_stop();
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << "obj end";
#endif
        level_up();
    }

    template<typename I>
    void debug(const std::string &msg, I begin, I end) {
#ifdef JSON_PARSER_DEBUG
        DEBUG(log) << msg << " " << std::string(begin, end);
#endif
    }

    JsonValue value;
    std::shared_ptr<Item> current;
    bool stop;
    gcm::logging::Logger &log;
};

template<typename I>
JsonValue parse(I &begin, I &end) {
    using namespace gcm::parser;
    using namespace std::placeholders;
    
    parser p;

    rule<I> value;

    auto _ws = *space();

    auto _char = *((any_rule() - '"'_r - '\\'_r ) | ('\\'_r & ('"'_r | '\\'_r | '/'_r | 'b'_r | 'f'_r | 'n'_r | 'r'_r | 't'_r | ('u' & iteration_rule(xdigit(), 4, 4)))));

    auto v_null = "null"_r >> std::bind(&parser::null_value<I>, &p, _1, _2);
    auto v_bool = "true"_r >> std::bind(&parser::true_value<I>, &p, _1, _2) | "false"_r >> std::bind(&parser::false_value<I>, &p, _1, _2);

    auto int_part = (-('+'_r | '-'_r) & +digit());
    auto v_int = int_part >> std::bind(&parser::int_value<I>, &p, _1, _2);

    auto frac = '.'_r & *digit();
    auto _e = "e+"_r | "e-"_r | 'e'_r | "E+"_r | "E-"_r | "E"_r;
    auto _exp = 'e'_r & *digit();
    auto v_double = (int_part & (frac | _exp | (frac & _exp))) >> std::bind(&parser::double_value<I>, &p, _1, _2);

    auto v_string = '"'_r & _char >> std::bind(&parser::string_value<I>, &p, _1, _2) & '"'_r;

    rule<I> value_in_array;
    value_in_array = !']'_r >> std::bind(&parser::new_array_item<I>, &p, _1, _2)
        & value
        & _ws
        & -(','_r & _ws & -value_in_array);

    auto v_array =
        '['_r >> std::bind(&parser::array_begin<I>, &p, _1, _2)
        & _ws
        & -value_in_array
        & _ws
        & ']'_r >> std::bind(&parser::array_end<I>, &p, _1, _2);

    auto identifier = '"'_r & _char >> std::bind(&parser::new_obj_item<I>, &p, _1, _2) & '"'_r;

    rule<I> value_in_obj;
    value_in_obj = identifier & _ws & ':'_r & _ws & value & -(',' & _ws & -value_in_obj);

    auto v_object =
        '{'_r >> std::bind(&parser::obj_begin<I>, &p, _1, _2)
        & _ws
        & -value_in_obj
        & _ws
        & '}'_r >> std::bind(&parser::obj_end<I>, &p, _1, _2);

    value = v_null | v_bool | v_double | v_int | v_string | v_array | v_object;

    auto document = _ws & value & _ws;

    if (document(begin, end)) {
        return p.value;
    } else {
        return nullptr;
    }
}

}
}