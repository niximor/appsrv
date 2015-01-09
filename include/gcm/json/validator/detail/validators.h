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

#include "../../json.h"
#include "types.h"
#include "../diagnostics.h"

namespace gcm {
namespace json {
namespace validator {
namespace detail {

class ParamDefinition {
public:
    ParamDefinition(ValueType type, std::string &&item, std::string &&help):
        type(type), item(std::forward<std::string>(item)), help(std::forward<std::string>(help))
    {}

    ValueType get_type() const {
        return type;
    }

    std::string get_item() const {
        return item;
    }

    std::string get_help() const {
        return help;
    }

    bool operator()(Diagnostics &diag, JsonValue &value) const {
        if (!value) {
            diag.add_problem(
                get_item(),
                ProblemCode::MustBePresent,
                "Item " + get_item() + " must be present."
            );
            return false;
        } else if (value->get_type() != type) {
            if (value->get_type() == ValueType::Null) {
                diag.add_problem(
                    get_item(),
                    ProblemCode::CannotBeNull,
                    "Item " + get_item() + " cannot be null."
                );
            } else {
                diag.add_problem(
                    get_item(),
                    ProblemCode::WrongType,
                    "Wrong type. Expected " + str_type(type) + ", instead got " + str_type(value->get_type())
                );
            }
            return false;
        } else {
            return true;
        }
    }

protected:
    ValueType type;
    std::string item;
    std::string help;
};

template<typename T>
class Array_t: public detail::ParamDefinition, public ArrayBase {
public:
    Array_t(const char *item, const char *help, T validator):
        detail::ParamDefinition(ValueType::Array, item, help),
        validator(validator)
    {}

    T &get_contents() {
        return validator;
    }

    bool operator()(Diagnostics &diag, JsonValue &value) {
        bool result = ParamDefinition::operator()(diag, value);
        if (result) {
            auto &arr = to<json::Array>(value);
            for (auto &i: arr) {
                result &= validator(diag, i);
            }
        }
        return result;
    }

protected:
    T validator;
};

template<typename... T>
class Object_t: public ParamDefinition, public Mappable {
public:
    Object_t(const char *item, const char *help, T... params):
        ParamDefinition(ValueType::Object, item, help),
        params(std::forward_as_tuple(params...))
    {}

    bool operator()(Diagnostics &diag, JsonValue &value) const {
        auto res = ParamDefinition::operator()(diag, value);

        if (res) {
            auto &obj = to<json::Object>(value);

            // Test if there are all required options in the struct
            res &= call_func(diag, obj, std::index_sequence_for<T...>{});

            // TODO: Test if there is no unexpected argument.
            for (auto &pair: obj) {
                if (!find_item(pair.first, std::index_sequence_for<T...>{})) {
                    diag.add_problem(pair.first, ProblemCode::UnexpectedParam, "Object does not have item " + pair.first);
                    res &= false;
                }
            }
        }

        return res;
    }

    template<typename C>
    void map(C call) {
        int_map(call, std::index_sequence_for<T...>{});
    }

protected:
    std::tuple<T...> params;

    template<typename Head, typename... Tail>
    bool int_validator(Diagnostics &diag, json::Object &obj, Head head, Tail... tail) const {
        auto res = head(diag, obj[head.get_item()]);
        res &= int_validator(diag, obj, tail...);
        return res;
    }

    bool int_validator(Diagnostics &, json::Object &) const {
        return true;
    }

    template<std::size_t ...I>
    bool call_func(Diagnostics &diag, json::Object &obj, std::index_sequence<I...>) const {
        return int_validator(diag, obj, std::get<I>(params)...);
    }

    template<typename Head, typename... Tail>
    bool int_find_item(const std::string &item, Head head, Tail... tail) const {
        return head.get_item() == item || int_find_item(item, tail...);
    }

    bool int_find_item(const std::string &) const {
        return false;
    }

    template<std::size_t ...I>
    bool find_item(const std::string &item, std::index_sequence<I...>) const {
        return int_find_item(item, std::get<I>(params)...);
    }

    template<typename C, std::size_t... I>
    void int_map(C call, std::index_sequence<I...>) {
        int_map2(call, std::get<I>(params)...);
    }

    template<typename C, typename Head, typename... Tail>
    void int_map2(C call, Head head, Tail... tail) {
        call(head);
        int_map2(call, tail...);
    }

    template<typename C>
    void int_map2(C) {
        // Trailer for list
    }
};

} // namespace detail
} // namespace validator
} // namespace json
} // namespace gcm
