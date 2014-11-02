#pragma once

#include <stdexcept>
#include <iostream>
#include <fstream>

#include <gcm/parser/parser.h>
#include <gcm/io/glob.h>

namespace gcm {
namespace config {

class parse_error: public std::runtime_error {
public:
    parse_error(const std::string &what): std::runtime_error(what)
    {}
};

class Parser {
public:
    Parser(Value &val): current(&val) {}

    template<typename I>
    void parse(I begin, I end) {
        using namespace std::placeholders;
        using namespace gcm::parser;

        rule<I> value;

        auto v_string = ('"' & many(any_rule() - '"') & '"');
        auto v_int = (-(char_rule('+') | '-') & +digit());
        auto v_double = (-(char_rule('+') | '-') & ((+digit() & -('.' & many(digit()))) | (many(digit()) & '.' & +digit())));
        auto v_bool = literal_rule("true") >> std::bind(&Parser::true_value<I>, this, _1, _2) | literal_rule("false") >> std::bind(&Parser::false_value<I>, this, _1, _2);
        auto v_null = literal_rule("null") >> std::bind(&Parser::null_value<I>, this, _1, _2);

        auto line_comment = (char_rule('#') | "//") & many(any_rule() - '\n');
        auto block_comment = "/*" & many(any_rule() - "*/") & "*/";
        auto comment = line_comment | block_comment;

        auto include_cmd = "include" & many(gcm::parser::space()) & (v_string >> std::bind(&Parser::include_file<I>, this, _1, _2)) & many(gcm::parser::space() - '\n') & '\n';
        auto preprocessor = char_rule('%') & (include_cmd);
        auto space = *(preprocessor | comment | gcm::parser::space());

        auto identifier = (alpha() & many(alnum())) >> std::bind(&Parser::identifier<I>, this, _1, _2);

        auto item = identifier & space & '=' & space & value & space & ';';

        auto v_array = char_rule('[') >> std::bind(&Parser::array_begin<I>, this, _1, _2) & space & many(value & ',' & space) & -(value) & space & char_rule(']') >> std::bind(&Parser::array_end<I>, this, _1, _2);
        auto v_struct = char_rule('{') >> std::bind(&Parser::struct_begin<I>, this, _1, _2) & space & many(item & space) & space & char_rule('}') >> std::bind(&Parser::struct_end<I>, this, _1, _2);

        value =
              v_string >> std::bind(&Parser::value_string<I>, this, _1, _2)
            | v_double >> std::bind(&Parser::value_double<I>, this, _1, _2)
            | v_int >> std::bind(&Parser::value_int<I>, this, _1, _2)
            | v_array 
            | v_struct 
            | v_bool 
            | v_null;

        auto parser = space & many(item & space);

        if (parser(begin, end) && begin == end) {
            std::cout << "Success" << std::endl;
        } else {
            throw parse_error("Parse error while reading config file.");
        }
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