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

#include "detail/flags.h"

namespace gcm {
namespace json {
namespace validator {

template<std::size_t N>
constexpr std::size_t cstrlen(const char (&)[N]) {
    return N;
}

inline std::string genhelp(std::string method_name, std::string desc) {
    std::stringstream ss;

    ss << desc << "\n\n";

    // Generate method signature
    ss << "\t" << "mixed " << method_name << "(...)\n\n";

    return ss.str();
}

template<typename T>
inline std::string genhelp(std::string method_name, std::string desc, T params) {
    std::stringstream ss;

    ss << desc << "\n\n";
    ss << "\t" << "mixed " << method_name << "(";

    // Generate method signature
    bool first = true;
    size_t longest = 0;
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

            default: break;
        }

        if (longest < current_size) {
            longest = current_size;
        }

        ss << param.get_item();
    });

    ss << ")\n\n";

    ss << "Params:\n\n";

    // Generate params help
    params.map([&](auto &param){
        ss << "    ";

        switch (param.get_type()) {
            case ValueType::Object: ss << "object "; break;
            case ValueType::Array: ss << "array "; break;
            case ValueType::Int: ss << "int "; break;
            case ValueType::Double: ss << "double "; break;
            case ValueType::String: ss << "string "; break;
            case ValueType::Bool: ss << "bool "; break;
            default: break;
        }

        ss << param.get_item();

        if (param.get_type() == ValueType::Array) {
            ss << " [";
        } else if (param.get_type() == ValueType::Object) {
            ss << " {";
        }

        ss << "    " << param.get_help() << "\n";
    });

    return ss.str();
}

template<typename T, typename R>
inline std::string genhelp(std::string method_name, std::string desc, T params, R) {
    std::stringstream ss;
    ss << genhelp(method_name, desc, params);

    ss << "\nResult:\n";

    return ss.str();
}

} // namespace validator
} // namespace json
} // namespace gcm
