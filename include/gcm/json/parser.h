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

class ParseException: public Exception {
public:
    ParseException(std::string message): Exception(message)
    {}
};

struct parser {
    struct Item {
        std::shared_ptr<Value> *value;
        std::shared_ptr<Item> parent;
    };

    parser():
        current(std::make_shared<Item>()), stop(false)
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
        //std::cout << "null value" << std::endl;
        *(current->value) = make_null();
        level_up();
    }

    template<typename I>
    void true_value(I, I) {
        test_stop();
        //std::cout << "true value" << std::endl;
        *(current->value) = make_bool(true);
        level_up();
    }

    template<typename I>
    void false_value(I, I) {
        test_stop();
        //std::cout << "false value" << std::endl;
        *(current->value) = make_bool(false);
        level_up();
    }

    template<typename I>
    void int_value(I begin, I end) {
        test_stop();
        //std::cout << "int value" << std::endl;
        *(current->value) = make_int(std::stoi(std::string(begin, end)));
        level_up();
    }

    template<typename I>
    void double_value(I begin, I end) {
        test_stop();
        //std::cout << "double value" << std::endl;
        *(current->value) = make_double(std::stod(std::string(begin, end)));
        level_up();
    }

    template<typename I>
    void string_value(I begin, I end) {
        test_stop();
        //std::cout << "string value" << std::endl;
        *(current->value) = make_string(std::string(begin, end));
        level_up();
    }

    template<typename I>
    void array_begin(I, I) {
        test_stop();
        //std::cout << "array begin" << std::endl;
        *(current->value) = make_array();
    }

    template<typename I>
    void new_array_item(I, I) {
        test_stop();
        //std::cout << "array item" << std::endl;
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
        //std::cout << "array end" << std::endl;
        level_up();
    }

    template<typename I>
    void obj_begin(I, I) {
        test_stop();
        //std::cout << "obj begin" << std::endl;
        *(current->value) = make_object();
    }

    template<typename I>
    void new_obj_item(I begin, I end) {
        test_stop();
        //std::cout << "new obj item: " << std::string(begin, end) << std::endl;
        auto &obj = to<Object>(*(current->value));
        
        auto item = std::make_shared<Item>();
        item->value = &(obj[std::string(begin, end)] = make_null());
        item->parent = current;

        current = item;
    }

    template<typename I>
    void obj_end(I, I) {
        test_stop();
        //std::cout << "obj end" << std::endl;
        level_up();
    }

    std::shared_ptr<Value> value;
    std::shared_ptr<Item> current;
    bool stop;
};

template<typename I>
std::shared_ptr<Value> parse(I &begin, I &end) {
    using namespace gcm::parser;
    using namespace std::placeholders;
    
    parser p;

    rule<I> value;

    auto _ws = *space();

    auto _char = *((any_rule() - '"'_r - '\\'_r ) | ('\\'_r & ('"'_r | '\\'_r | '/'_r | 'b'_r | 'f'_r | 'n'_r | 'r'_r | 't'_r | ('u' & iteration_rule(xdigit(), 4, 4)))));

    auto v_null = "null"_r >> std::bind(&parser::null_value<I>, &p, _1, _2);
    auto v_bool = "true"_r >> std::bind(&parser::true_value<I>, &p, _1, _2) | "false"_r >> std::bind(&parser::false_value<I>, &p, _1, _2);
    auto v_int = (-('+'_r | '-'_r) & +digit()) >> std::bind(&parser::int_value<I>, &p, _1, _2);

    auto frac = '.'_r & *digit();
    auto _e = "e+"_r | "e-"_r | 'e'_r | "E+"_r | "E-"_r | "E"_r;
    auto _exp = 'e'_r & *digit();
    auto v_double = (v_int & (frac | _exp | (frac & _exp))) >> std::bind(&parser::double_value<I>, &p, _1, _2);

    auto v_string = '"'_r & _char >> std::bind(&parser::string_value<I>, &p, _1, _2) & '"'_r;

    auto value_in_array = !']'_r >> std::bind(&parser::new_array_item<I>, &p, _1, _2) & value;
    auto v_array =
        '['_r >> std::bind(&parser::array_begin<I>, &p, _1, _2)
        & *(_ws & value_in_array & _ws & ','_r)
        & _ws & -value_in_array
        & _ws & ']'_r >> std::bind(&parser::array_end<I>, &p, _1, _2);

    auto identifier = '"'_r & _char >> std::bind(&parser::new_obj_item<I>, &p, _1, _2) & '"'_r;
    auto value_in_obj = identifier & _ws & ':'_r & _ws & value;
    auto v_object =
        '{'_r >> std::bind(&parser::obj_begin<I>, &p, _1, _2)
        & *(_ws & value_in_obj & _ws & ','_r)
        & _ws & -value_in_obj
        & _ws & '}'_r >> std::bind(&parser::obj_end<I>, &p, _1, _2);

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