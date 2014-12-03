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

#include "diagnostics.h"
#include "detail/validators.h"

#include "../json.h"

namespace gcm {
namespace json {
namespace validator {

class Int: public detail::ParamDefinition {
public:
    Int(const char *item, const char *help):
        detail::ParamDefinition(ValueType::Int, item, help)
    {}
};

class Double: public detail::ParamDefinition {
public:
    Double(const char *item, const char *help):
        detail::ParamDefinition(ValueType::Double, item, help)
    {}
};

class Bool: public detail::ParamDefinition {
public:
    Bool(const char *item, const char *help):
        detail::ParamDefinition(ValueType::Bool, item, help)
    {}
};

class String: public detail::ParamDefinition {
public:
    String(const char *item, const char *help):
        detail::ParamDefinition(ValueType::String, item, help)
    {}
};

template<typename T>
auto Array(const char *item, const char *help, T validator) {
    return detail::Array_t<T>(item, help, validator);
}

template<typename... Params>
auto Object(const char *item, const char *help, Params... params) {
    return detail::Object_t<Params...>(item, help, params...);
}

class AnyType: public detail::ParamDefinition {
public:
    AnyType(const char *item, const char *help):
        detail::ParamDefinition(ValueType::Null, item, help)
    {}

    bool operator()(Diagnostics &diag, JsonValue &value) const {
        if (!value) {
            diag.add_problem(
                get_item(),
                ProblemCode::MustBePresent,
                "Item " + get_item() + " must be present."
            );
            return false;
        } else {
            return true;
        }
    }
};

}
}
}