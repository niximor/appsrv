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

#include <gcm/logging/logging.h>

#include "types.h"
#include "exception.h"
#include "../validator.h"

namespace gcm {
namespace json {
namespace rpc {
namespace detail {

template<typename T>
class SafeCallback_t {
public:
    SafeCallback_t(gcm::logging::Logger &, std::function<Method> method, const std::string &, T params):
        method(method),
        params(params)
    {}

    JsonValue operator()(Array &p) {
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
    SafeCallbackR_t(gcm::logging::Logger &log, std::function<Method> method, const std::string &method_name, T params, R result):
        log(log),
        method(method),
        method_name(method_name),
        params(params),
        result(result)
    {}

    JsonValue operator()(Array &p) {
        try {
            params.validate(p);
            auto out = method(p);

            // Validate result of function
            validator::Diagnostics diag;
            auto res = result(diag, out);
            if (!res) {
                ERROR(log) << "Method " << method_name << " returned invalid result:";
                for (auto &problem: diag.get_problems()) {
                    ERROR(log) << problem.get_item() << ": " << problem.get_description();
                }

                throw RpcException(ErrorCode::InternalError, "Function returned unexpected result.");
                //throw diag;
            }

            return out;
        } catch (validator::Diagnostics &diag) {
            json::Array array;

            for (auto &problem: diag.get_problems()) {
                array.push_back(problem.to_json());
            }

            throw RpcException(
                ErrorCode::InvalidParams,
                diag.what(),
                std::make_shared<Array>(std::move(array))
            );
        }
    }

protected:
    gcm::logging::Logger &log;
    std::function<Method> method;
    std::string method_name;
    T params;
    R result;
};

template<typename T>
auto SafeCallback(gcm::logging::Logger &log, std::function<Method> method, const std::string &method_name, T params) {
    return SafeCallback_t<T>(log, method, method_name, params);
}

template<typename T, typename R>
auto SafeCallback(gcm::logging::Logger &log, std::function<Method> method, const std::string &method_name, T params, R result) {
    return SafeCallbackR_t<T, R>(log, method, method_name, params, result);
}

} // namespace detail
} // namespace rpc
} // namespace json
} // namespace gcm
