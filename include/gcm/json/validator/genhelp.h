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

#include <sstream>
#include <type_traits>
#include <iostream>

#include "detail/flags.h"
#include "detail/types.h"

namespace gcm {
namespace json {
namespace validator {

namespace detail {

constexpr const char *IndentStr = "    ";

/**
 * Return length of string, in compile time.
 */
template<std::size_t N>
constexpr std::size_t cstrlen(const char (&)[N]) {
    return N;
}

/**
 * Repeat \p fill \p num times to \p ss stream.
 */
template<typename T>
void fill(std::stringstream &ss, T fill, int num) {
    for (; num > 0; --num) {
        ss << fill;
    }
}

/**
 * Generate short method params to method signature.
 */
template<typename T>
std::size_t gen_method_params(std::stringstream &ss, T &params) {
    ss << "(";

    std::size_t longest{0};

    bool first = true;
    params.map([&](auto &param){
        if (first) {
            first = false;
        } else {
            ss << ", ";
        }

        std::size_t current_size = param.get_item().size();

        switch (param.get_type()) {
            case ValueType::Object:
                ss << "object ";
                current_size += cstrlen("object{");
                break;

            case ValueType::Array:
                ss << "array ";
                current_size += cstrlen("array[");
                break;

            case ValueType::Int:
                ss << "int ";
                current_size += cstrlen("int");
                break;

            case ValueType::Double:
                ss << "double ";
                current_size += cstrlen("double");
                break;

            case ValueType::String:
                ss << "string ";
                current_size += cstrlen("sitrng");
                break;

            case ValueType::Bool:
                ss << "bool ";
                current_size += cstrlen("bool");
                break;

            case ValueType::Null:
                ss << "void ";
                current_size += cstrlen("void");
        }

        if (longest < current_size) {
            longest = current_size;
        }

        ss << param.get_item();
    });

    ss << ")";

    return longest;
}

/**
 * Generate method signature.
 * Method without specified params or return type.
 */
inline std::size_t gen_method_signature(const std::string &method_name, std::stringstream &ss) {
    ss << "mixed " << method_name << "(...)";
    return 0;
}

/**
 * Generate method signature.
 * Method without specified return type.
 */
template<typename T>
std::size_t gen_method_signature(const std::string &method_name, std::stringstream &ss, T &params) {
    ss << "mixed " << method_name;
    return gen_method_params(ss, params);
}

/**
 * Generated method signature.
 * Method with specified return type
 */
template<typename T, typename R>
std::size_t gen_method_signature(const std::string &method_name, std::stringstream &ss, T &params, R &result) {
    switch (result.get_type()) {
        case ValueType::Int: ss << "int"; break;
        case ValueType::Double: ss << "double"; break;
        case ValueType::Bool: ss << "bool"; break;
        case ValueType::String: ss << "string"; break;
        case ValueType::Object: ss << "object"; break;
        case ValueType::Array: ss << "array"; break;
        case ValueType::Null: ss << "void"; break;
        default: ss << "mixed";
    }

    ss << " " << method_name;
    return gen_method_params(ss, params);
}

template<typename T>
std::enable_if_t<std::is_base_of<Mappable, T>::value, std::size_t>
calc_longest(T &param_pack) {
    std::size_t longest = 0;

    param_pack.map([&](auto &param){
        std::size_t current_size = param.get_item().size();

        switch (param.get_type()) {
            case ValueType::Object:
                current_size += cstrlen("object{");
                break;

            case ValueType::Array:
                current_size += cstrlen("array[");
                break;

            case ValueType::Int:
                current_size += cstrlen("int");
                break;

            case ValueType::Double:
                current_size += cstrlen("double");
                break;

            case ValueType::String:
                current_size += cstrlen("sitrng");
                break;

            case ValueType::Bool:
                current_size += cstrlen("bool");
                break;

            case ValueType::Null:
                current_size += cstrlen("void");
        }

        if (longest < current_size) {
            longest = current_size;
        }
    });

    return longest;
}

/**
 * For other types, there are no inner objects. So length of inner items is 0.
 */
template<typename T>
std::enable_if_t<!std::is_base_of<Mappable, T>::value, std::size_t>
calc_longest(T &) {
    return 0;
}

/**
 * Generate inner content of object.
 */
template<typename T>
std::enable_if_t<std::is_base_of<Mappable, T>::value, void>
gen_contents(std::stringstream &ss, std::size_t indent, T &param) {
    std::size_t longest = calc_longest(param);
    param.map([&](auto &obj_param){
        gen_param(ss, indent + 1, longest, obj_param);
    });
}

/**
 * For array type, there is only one inner object.
 */
template<typename T>
std::enable_if_t<std::is_base_of<ArrayBase, T>::value, void>
gen_contents(std::stringstream &ss, std::size_t indent, T &param) {
    gen_param(ss, indent + 1, 0, param.get_contents());
}

/**
 * For non-mappable types, no content, so no generation here.
 */
template<typename T>
std::enable_if_t<!std::is_base_of<ArrayBase, T>::value && !std::is_base_of<Mappable, T>::value, void>
gen_contents(std::stringstream &, std::size_t, T &) {
}

/**
 * Generate one param description.
 */
template<typename T>
void gen_param(std::stringstream &ss, std::size_t indent, std::size_t alignment, T &param) {
    fill(ss, IndentStr, indent);

    // Length of current type including it's name.
    std::size_t current = param.get_item().size();

    // Generate type
    switch (param.get_type()) {
        case ValueType::Object:
            ss << "object ";
            current += cstrlen("object{");
            break;

        case ValueType::Array:
            ss << "array ";
            current += cstrlen("array[");
            break;

        case ValueType::Int:
            ss << "int ";
            current += cstrlen("int");
            break;

        case ValueType::Double:
            ss << "double ";
            current += cstrlen("double");
            break;

        case ValueType::String:
            ss << "string ";
            current += cstrlen("string");
            break;

        case ValueType::Bool:
            ss << "bool ";
            current += cstrlen("bool");
            break;

        case ValueType::Null:
            ss << "void ";
            current += cstrlen("void");
            break;
    }

    ss << param.get_item();

    // Generate opening brackets for object or array.
    if (param.get_type() == ValueType::Array) {
        ss << " [";
    } else if (param.get_type() == ValueType::Object) {
        ss << " {";
    }

    int fillcnt = alignment - current;
    fill(ss, ' ', fillcnt);

    ss << IndentStr;
    ss << param.get_help();
    ss << '\n';

    if (param.get_type() == ValueType::Array) {
        gen_contents(ss, indent, param);
        fill(ss, IndentStr, indent);
        ss << "]\n";
    } else if (param.get_type() == ValueType::Object) {
        gen_contents(ss, indent, param);
        fill(ss, IndentStr, indent);
        ss << "}\n";
    }
}

} // namespace detail

inline std::string genhelp(const std::string &method_name, std::string desc) {
    std::stringstream ss;
    ss << desc << "\n\n";

    // Generate method signature
    ss << '\t';
    detail::gen_method_signature(method_name, ss);
    ss << "\n\n";

    return ss.str();
}

template<typename T>
inline std::string genhelp(const std::string &method_name, const std::string &desc, T &params) {
    std::stringstream ss;
    ss << desc << "\n\n";

    // Generate method signature
    ss << detail::IndentStr;
    std::size_t longest = detail::gen_method_signature(method_name, ss, params);
    ss << "\n\n";

    ss << "Params:\n\n";

    // Generate params help
    params.map([&](auto &param){
        detail::gen_param(ss, 1, longest, param);
    });

    return ss.str();
}

template<typename T, typename R>
inline std::string genhelp(const std::string &method_name, const std::string &desc, T &params, R &result) {
    std::stringstream ss;
    ss << desc << "\n\n";

    // Generate method signature
    ss << detail::IndentStr;
    std::size_t longest = detail::gen_method_signature(method_name, ss, params, result);
    ss << "\n\n";

    std::size_t res_longest = detail::calc_longest(result);
    if (res_longest > longest) {
        longest = res_longest;
    }

    ss << "Params:\n\n";

    // Generate params help
    params.map([&](auto &param){
        detail::gen_param(ss, 1, longest, param);
    });

    ss << "\nResult:\n\n";
    
    detail::gen_param(ss, 1, longest, result);

    return ss.str();
}

} // namespace validator
} // namespace json
} // namespace gcm
