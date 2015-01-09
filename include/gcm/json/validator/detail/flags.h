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

#include "../diagnostics.h"

namespace gcm {
namespace json {
namespace validator {
namespace detail {

template<typename T>
class Nullable_t {
public:
    Nullable_t(T param_def): param_def(param_def)
    {}

    bool operator()(Diagnostics &diag, JsonValue &value) const {
        if (value && value->get_type() == ValueType::Null) {
            return true;
        } else {
            return param_def(diag, value);
        }
    }

    auto get_item() {
        return param_def.get_item();
    }

    auto get_help() {
        return param_def.get_help();
    }

protected:
    T param_def;
};

template<typename T>
class Optional_t {
public:
    Optional_t(T param_def): param_def(param_def) {

    }

    bool operator()(Diagnostics &diag, JsonValue &value) const {
        if (!value) {
            // Optional and not present.
            return true;
        } else {
            return param_def(diag, value);
        }
    }

    auto get_item() {
        return param_def.get_item();
    }

    auto get_type() {
        return param_def.get_type();
    }

    auto get_help() {
        return param_def.get_help();
    }

protected:
    T param_def;
};

template<typename T, typename V>
class OptionalWithDefault_t {
public:
    OptionalWithDefault_t(T param_def, V default_value):
        param_def(param_def),
        default_value(default_value)
    {
    }

    bool operator()(Diagnostics &diag, JsonValue &value) const {
        if (!value) {
            // Optional and not present.
            value = default_value;
            return param_def(diag, value);
        } else {
            return param_def(diag, value);
        }
    }

    auto get_item() {
        return param_def.get_item();
    }

    auto get_type() {
        return param_def.get_type();
    }

    auto get_help() {
        return param_def.get_help();
    }

protected:
    T param_def;
    V default_value;
};

} // namespace detail
} // namespace validator
} // namespace json
} // namespace gcm
