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

#include <memory>
#include <string>
#include <chrono>
#include <iomanip>

#include "../json.h"
#include "types.h"
#include "promise.h"

namespace gcm {
namespace json {
namespace rpc {
namespace detail {

class MethodProcessor {
public:
    MethodProcessor(gcm::logging::Logger &log, MethodRegistry &registry, std::shared_ptr<Promise> promise, JsonValue request_id, std::string &&method, Array &&params):
        log(log),
        registry(registry),
        promise(promise),
        request_id(request_id),
        method(std::forward<std::string>(method)),
        params(std::forward<Array>(params))
    {}

    void operator()() {
        std::chrono::high_resolution_clock::time_point tm_start = std::chrono::high_resolution_clock::now();

        auto response = Object();
        response["jsonrpc"] = make_string("2.0");
        response["id"] = request_id;

        bool is_success = true;

        std::string str_params = params.to_string();
        INFO(log) << "Calling method " << method << "(" << str_params.substr(1, str_params.size() - 2) << ").";

        try {
            auto it = registry.find(method);
            if (it == registry.end()) {
                throw MethodNotFound(std::forward<JsonValue>(request_id), method);
            }

            auto &result = response["result"];
            result = it->second(params);
        } catch (RpcException &e) {
            is_success = false;
            response["error"] = e.to_json(false);
        } catch (std::exception &e) {
            is_success = false;
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(ErrorCode::InternalError);
            error["message"] = make_string(e.what());
        } catch (...) {
            is_success = false;
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(ErrorCode::InternalError);
            error["message"] = make_string("Server error.");
        }

        std::chrono::microseconds method_duration =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - tm_start);

        std::string str_status;
        if (is_success) {
            str_status = "OK";
        } else {
            int code = to<Int>(to<Object>(response["error"])["code"]);
            switch (code) {
                case static_cast<int>(ErrorCode::ParseError): str_status = "ParseError"; break;
                case static_cast<int>(ErrorCode::InvalidRequest): str_status = "InvalidRequest"; break;
                case static_cast<int>(ErrorCode::MethodNotFound): str_status = "MethodNotFound"; break;
                case static_cast<int>(ErrorCode::InvalidParams): str_status = "InvalidParams"; break;
                case static_cast<int>(ErrorCode::InternalError): str_status = "InternalError"; break;
                case static_cast<int>(ErrorCode::ServerError): str_status = "ServerError"; break;
                default: str_status = std::to_string(code); break;
            }
        }

        INFO(log) << method << "(" << str_params.substr(1, str_params.size() - 2) << ") "
            << "status=" << str_status << "; "
            << "time=" << std::setprecision(3) << (method_duration.count() / 1000.0) << "ms";

        // Notify of job done.
        promise->result = std::make_shared<Object>(response);
        promise->has_result = true;

        if (promise->notify_done != nullptr) {
            promise->notify_done->notify_one();
        }

        promise->cb.notify_one();
    }

protected:
    gcm::logging::Logger &log;
    MethodRegistry &registry;
    std::shared_ptr<Promise> promise;
    JsonValue request_id;
    std::string method;
    Array params;
};

} // namespace detail
} // namespace rpc
} // namespace json
} // namespace gcm
