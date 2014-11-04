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
    parse_error(const std::string &what):
        std::runtime_error(what)
    {}

    parse_error(const gcm::parser::ParserPosition &pos, const std::string &what):
        std::runtime_error([](const gcm::parser::ParserPosition &pos, const std::string &what) {
            std::stringstream ss;
            ss << "on line " << pos.first << ", column " << pos.second << ": " << what;
            return ss.str();
        }(pos, what))
    {}

    parse_error(const gcm::parser::ParserPosition &pos, const char *what):
        std::runtime_error([](const gcm::parser::ParserPosition &pos, const char *what) {
            std::stringstream ss;
            ss << "on line " << pos.first << ", column " << pos.second << ": " << what;
            return ss.str();
        }(pos, what))
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

        auto v_string = '"' & *(any_rule() - '"') & '"';
        auto v_int = -('+'_r | '-'_r) & +digit();
        auto v_double = -('+'_r | '-'_r) & ((+digit() & -('.' & *digit())) | (*digit() & '.' & +digit()));
        auto v_bool = "true"_r >> std::bind(&Parser::true_value<I>, this, _1, _2) | "false"_r >> std::bind(&Parser::false_value<I>, this, _1, _2);
        auto v_null = "null"_r >> std::bind(&Parser::null_value<I>, this, _1, _2);

        auto line_comment = ('#'_r | "//"_r) & *(any_rule() - '\n');
        auto block_comment = "/*" & *(any_rule() - "*/") & "*/";
        auto comment = line_comment | block_comment;

        auto include_cmd = "include" & *gcm::parser::space() & (v_string >> std::bind(&Parser::include_file<I>, this, _1, _2)) & *(gcm::parser::space() - '\n') & '\n';
        auto preprocessor = '%' & include_cmd;
        auto space = *(preprocessor | comment | gcm::parser::space());
        auto identifier = (alpha() & *alnum()) >> std::bind(&Parser::identifier<I>, this, _1, _2);

        auto item =
            (identifier / std::bind(&Parser::error<I>, this, "Required identifier.", begin, _1))
            & space & '=' & space
            & (value / std::bind(&Parser::error<I>, this, "Required value.", begin, _1))
            & space & (';'_r / std::bind(&Parser::error<I>, this, "Required ;.", begin, _1));

        auto v_array = '['_r >> std::bind(&Parser::array_begin<I>, this, _1, _2)
            & space
            & *(value & ','_r & space)
            & -(value)
            & space
            & ']'_r >> std::bind(&Parser::array_end<I>, this, _1, _2);

        auto v_struct = '{'_r >> std::bind(&Parser::struct_begin<I>, this, _1, _2)
            & space
            & *(item & space)
            & space
            & '}'_r >> std::bind(&Parser::struct_end<I>, this, _1, _2);

        // TODO: Rule is copied, so this is discarded.
        value =
              (v_string >> std::bind(&Parser::value_string<I>, this, _1, _2)
            | v_double >> std::bind(&Parser::value_double<I>, this, _1, _2)
            | v_int >> std::bind(&Parser::value_int<I>, this, _1, _2)
            | v_array 
            | v_struct 
            | v_bool 
            | v_null);

        auto parser = space & *(item & space);

        I orig_begin = begin;
        if (parser(begin, end) && begin == end) {
            std::cout << "Success" << std::endl;
        } else {
            auto pos = calc_line_column(orig_begin, begin);
            throw parse_error(pos, "Parse error while reading config file.");
        }
    }

protected:
    Value *current;

    template<typename I>
    void error(const std::string &desc, I begin, I end) {
        throw parse_error(gcm::parser::calc_line_column(begin, end), desc);
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