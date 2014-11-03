#pragma once

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>

#include <gcm/parser/parser.h>
#include <gcm/io/glob.h>

namespace gcm {
namespace config {

class parse_error: public std::runtime_error {
public:
    parse_error(const std::string &what): std::runtime_error(what)
    {}
};

template<typename T>
void print_type() {
    std::cout << "Func = " << __PRETTY_FUNCTION__ << std::endl;
}

class Parser {
public:
    Parser(Value &val): current(&val) {}

    template<typename I>
    void parse(I begin, I end) {
        using namespace std::placeholders;
        using namespace gcm::parser;

        auto test = iteration_rule(alpha(), 0, 1);
        print_type<decltype(test & alpha())>();
        //auto test2 = alpha() & test;

//        rule<I> value;
//
//        auto v_string = '"'_r & *(any_rule() - '"'_r) & '"'_r;
//        auto v_int = -('+'_r | '-'_r) & +digit();
//        auto v_double = -('+'_r | '-'_r) & ((+digit() & -('.'_r & *digit())) | (*digit() & '.'_r & +digit()));
//        auto v_bool = "true"_r >> std::bind(&Parser::true_value<I>, this, _1, _2) | "false"_r >> std::bind(&Parser::false_value<I>, this, _1, _2);
//        auto v_null = "null"_r >> std::bind(&Parser::null_value<I>, this, _1, _2);
//
//        auto line_comment = ('#'_r | "//"_r) & *(any_rule() - '\n'_r);
//        auto block_comment = "/*"_r & *(any_rule() - "*/"_r) & "*/"_r;
//        auto comment = line_comment | block_comment;
//
//        auto include_cmd = "include"_r & *gcm::parser::space() & (v_string >> std::bind(&Parser::include_file<I>, this, _1, _2)) & *(gcm::parser::space() - '\n'_r) & '\n'_r;
//        auto preprocessor = '%'_r & include_cmd;
//        //auto space = *(preprocessor | comment | gcm::parser::space());
//        auto identifier = (alpha() & *alnum()) >> std::bind(&Parser::identifier<I>, this, _1, _2);
//
//        auto item = identifier & space & '='_r & space & value & space & ';'_r;
//
//        auto v_array = '['_r >> std::bind(&Parser::array_begin<I>, this, _1, _2)
//            & space
//            & *(value & ','_r & space)
//            & -(value)
//            & space
//            & ']'_r >> std::bind(&Parser::array_end<I>, this, _1, _2);
//
//        auto v_struct = '{'_r >> std::bind(&Parser::struct_begin<I>, this, _1, _2)
//            & space
//            & *(item & space)
//            & space
//            & '}'_r >> std::bind(&Parser::struct_end<I>, this, _1, _2);
//
//        value =
//              v_string >> std::bind(&Parser::value_string<I>, this, _1, _2)
//            | v_double >> std::bind(&Parser::value_double<I>, this, _1, _2)
//            | v_int >> std::bind(&Parser::value_int<I>, this, _1, _2)
//            | v_array 
//            | v_struct 
//            | v_bool 
//            | v_null;
//
//        auto parser = space & *(item & space);
//
//        if (parser(begin, end) && begin == end) {
//            std::cout << "Success" << std::endl;
//        } else {
//            throw parse_error("Parse error while reading config file.");
//        }
    }

protected:
    Value *current;

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
        auto p = s.emplace(std::make_pair(identifier, std::move(v)));
        current = &(p.first->second);
    }

    template<typename T>
    void set_value(T value) {
        if (current->getType() == Value::Type::Array) {
            current->asArray().emplace_back(value);
        } else {
            (*current) = value;
            current = current->parent;
        }
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
        (*current) = Value::ArrayType();
    }

    template<typename I>
    void struct_begin(I, I) {
        (*current) = Value::StructType();
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

        std::cout << "Including " << file << std::endl;

        gcm::io::Glob glob(file);
        for (auto file: glob) {
            std::cout << "Matched " << file << std::endl;

            std::ifstream f(file, std::ios_base::in);
            std::string contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

            Parser p(*current);
            p.parse(contents.begin(), contents.end());
        }
    }
};

}
}