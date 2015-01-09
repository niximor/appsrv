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

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>

#include <gcm/parser/parser.h>
#include <gcm/logging/logging.h>
#include <gcm/io/glob.h>
#include <gcm/io/exception.h>

#include "value.h"

namespace gcm {
namespace config {

class parse_error: public std::runtime_error {
public:
    parse_error(const std::string &what):
        std::runtime_error(what)
    {}

    parse_error(const std::string &file, const gcm::parser::ParserPosition &pos, const std::string &what):
        std::runtime_error([&]() {
            std::stringstream ss;
            ss << file << ":" << pos.first << ":" << pos.second << ": " << what;
            return ss.str();
        }())
    {}

    parse_error(const std::string &file, const gcm::parser::ParserPosition &pos, const char *what):
        std::runtime_error([&]() {
            std::stringstream ss;
            ss << file << ":" << pos.first << ":" << pos.second << ": " << what;
            return ss.str();
        }())
    {}
};

class Parser {
public:
    static constexpr const char *DefaultFileName = "unknown";

    Parser(Value &val):
        root_value(val),
        current(&val),
        file_name(DefaultFileName),
        log(gcm::logging::getLogger("GCM.ConfigParser"))
    {}

    Parser(Value &val, const std::string &file):
        root_value(val),
        current(&val),
        file_name(file),
        log(gcm::logging::getLogger("GCM.ConfigParser"))
    {
        DEBUG(log) << "Reading config file " << file << ".";
    }

    template<typename I>
    void parse(I begin, I end) {
        using namespace std::placeholders;
        using namespace gcm::parser;

        rule<I> value;
        rule<I> value_in_array;

        auto v_string = '"' & *(any_rule() - '"') & '"';
        auto v_int = -('+'_r | '-'_r) & +digit();
        auto v_double = -('+'_r | '-'_r) & ((+digit() & -('.' & *digit())) | (*digit() & '.' & +digit()));
        auto v_bool = "true"_r >> std::bind(&Parser::true_value<I>, this, _1, _2) | "false"_r >> std::bind(&Parser::false_value<I>, this, _1, _2);
        auto v_null = "null"_r >> std::bind(&Parser::null_value<I>, this, _1, _2);

        auto line_comment = ('#'_r | "//"_r) & *(any_rule() - '\n');
        auto block_comment = "/*" & *(any_rule() - "*/") & "*/";
        auto comment = line_comment | block_comment;

        auto space = *(comment | gcm::parser::space());

        auto include_cmd = "include" & *gcm::parser::space() & (v_string >> std::bind(&Parser::include_file<I>, this, _1, _2)) & *(gcm::parser::space() - '\n') & '\n';
        auto preprocessor = '%'_r & include_cmd;
        auto identifier = (alpha() & *(alnum() | '_' | '-')) >> std::bind(&Parser::identifier<I>, this, _1, _2);

        auto assignment = '='_r / std::bind(&Parser::error<I>, this, "Required =.", begin, _1);

        auto item = 
            identifier
            & space & assignment & space
            & (value / std::bind(&Parser::error<I>, this, "Required value.", begin, _1))
            & space & (';'_r / std::bind(&Parser::error<I>, this, "Required ;.", begin, _1))
            & space;

        auto item_in_struct =
            (
                (
                    identifier & space & assignment & space
                    & (value / std::bind(&Parser::error<I>, this, "Required value.", begin, _1))
                    & space & (';'_r / std::bind(&Parser::error<I>, this, "Required ;.", begin, _1))
                    & space
                ) | '}'_r >> std::bind(&Parser::struct_end<I>, this, _1, _2)
            );

        auto v_array = '['_r >> std::bind(&Parser::array_begin<I>, this, _1, _2)
            & space
            & *(value_in_array & space & ','_r & space)
            & -(value_in_array)
            & space
            & ']'_r >> std::bind(&Parser::array_end<I>, this, _1, _2);

        auto v_struct = '{'_r >> std::bind(&Parser::struct_begin<I>, this, _1, _2)
            & space
            & (+item_in_struct | '}'_r >> std::bind(&Parser::struct_end<I>, this, _1, _2))
            / std::bind(&Parser::error<I>, this, "Required identifier or }.", begin, _1);

        value =
              (v_string >> std::bind(&Parser::value_string<I>, this, _1, _2)
            | v_double >> std::bind(&Parser::value_double<I>, this, _1, _2)
            | v_int >> std::bind(&Parser::value_int<I>, this, _1, _2)
            | v_array 
            | v_struct 
            | v_bool 
            | v_null);

        value_in_array = !(']'_r) >> std::bind(&Parser::array_item<I>, this, _1, _2) & value;

        auto parser = space &
            (+(preprocessor | item) | end_rule()) / std::bind(&Parser::error<I>, this, "Required identifier.", begin, _1)
            & space;

        I orig_begin = begin;
        if (parser(begin, end) && begin == end && &root_value == current) {
            //std::cout << "Success" << std::endl;
        } else {
            auto pos = calc_line_column(orig_begin, begin);

            if (begin == end) {
                throw parse_error(file_name, pos, "Premature end of input.");
            } else {
                throw parse_error(file_name, pos, "Parse error while reading config file.");
            }
        }
    }

protected:
    Value &root_value;
    Value *current;
    const std::string file_name;
    gcm::logging::Logger &log;

    template<typename I>
    void error(const std::string &desc, I begin, I end) {
        throw parse_error(file_name, gcm::parser::calc_line_column(begin, end), desc);
    }

    template<typename I>
    void identifier(I begin, I end) {
        auto identifier = std::string(begin, end);

        Value v;
        v.parent = current;

        if (current->identifier.empty()) {
            v.identifier = identifier;
        } else {
            v.identifier = current->identifier + "." + identifier;
        }

        auto &s = current->asStruct();

        s.emplace_back(identifier, v);
        auto &p = s.back();
        current = &(p.second);
    }

    template<typename I>
    void array_item(I, I) {
        Value v;
        v.parent = current;

        std::stringstream ss;
        ss << current->asArray().size();

        if (current->identifier.empty()) {
            v.identifier = ss.str();
        } else {
            v.identifier = current->identifier + "." + ss.str();
        }

        auto &s = current->asArray();

        s.emplace_back(v);
        current = &(s.back());
    }

    template<typename T>
    void set_value(T value) {
        DEBUG(gcm::logging::getLogger("GCM.ConfigParser"))
            << std::boolalpha << "Setting value of " << current->identifier << " to '" << value << "'";

        (*current) = value;
        current = current->parent;
    }

    void set_value(nullptr_t value) {
        DEBUG(gcm::logging::getLogger("GCM.ConfigParser"))
            << std::boolalpha << "Setting value of " << current->identifier << "to NULL";

        (*current) = value;
        current = current->parent;
    }

    template<typename I>
    void value_string(I begin, I end) {
        std::string val{begin + 1, end - 1};
        set_value(val);
    }

    template<typename I>
    void value_int(I begin, I end) {
        int64_t val = ::atoll(std::string(begin, end).c_str());
        set_value(val);
    }

    template<typename I>
    void value_double(I begin, I end) {
        double val = ::atof(std::string(begin, end).c_str());
        set_value(val);
    }

    template<typename I>
    void true_value(I, I) {
        set_value(true);
    }

    template<typename I>
    void false_value(I, I) {
        set_value(false);
    }

    template<typename I>
    void null_value(I, I) {
        set_value(nullptr);
    }

    template<typename I>
    void array_begin(I, I) {
        (*current) = ArrayType();
    }

    template<typename I>
    void struct_begin(I, I) {
        (*current) = StructType();
    }

    template<typename I>
    void array_end(I, I) {
        current = current->parent;
    }

    template<typename I>
    void struct_end(I, I) {
        current = current->parent;
    }

    template<typename I>
    void include_file(I begin, I end) {
        auto file = std::string(begin + 1, end - 1);

        try {
            gcm::io::Glob glob(file);
            for (auto file: glob) {
                std::ifstream f(file, std::ios_base::in);
                std::string contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

                Parser p(root_value, file);
                p.parse(contents.begin(), contents.end());
            }
        } catch (gcm::io::IOException &e) {
            ERROR(gcm::logging::getLogger("GCM.ConfigParser")) << e.what();
        }
    }
};

} // namespace config
} // namespace gcm
