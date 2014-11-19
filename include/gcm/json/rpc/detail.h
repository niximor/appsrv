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

#include "types.h"
#include "../validator.h"

namespace gcm {
namespace json {
namespace rpc {
namespace detail {

template<typename T>
class SafeCallback_t {
public:
    SafeCallback_t(std::function<Method> method, T params):
        method(method),
        params(params)
    {}

    std::shared_ptr<Value> operator()(Array &p) {
        params.validate(p);
        return method(p);
    }

protected:
    std::function<Method> method;
    T params;
};

template<typename T, typename R>
class SafeCallbackR_t {
public:
    SafeCallbackR_t(std::function<Method> method, T params, R result):
        method(method),
        params(params),
        result(result)
    {}

    std::shared_ptr<Value> operator()(Array &p) {
        params.validate(p);
        auto out = method(p);

        // Validate result of function
        validator::Diagnostics diag;
        auto res = result(diag, out);
        if (!res) {
            throw diag;
        }

        return out;
    }

protected:
    std::function<Method> method;
    T params;
    R result;
};

template<typename T>
auto SafeCallback(std::function<Method> method, T params) {
    return SafeCallback_t<T>(method, params);
}

template<typename T, typename R>
auto SafeCallback(std::function<Method> method, T params, R result) {
    return SafeCallbackR_t<T, R>(method, params, result);
}

} // namespace detail
} // namespace rpc
} // namespace json
} // namespace gcm
