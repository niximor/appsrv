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

#include "../json.h"
#include "types.h"
#include "promise.h"

namespace gcm {
namespace json {
namespace rpc {
namespace detail {

class MethodProcessor {
public:
    MethodProcessor(MethodRegistry &registry, std::shared_ptr<Promise> promise, std::shared_ptr<Value> request_id, std::string &&method, Array &&params):
        registry(registry),
        promise(promise),
        request_id(request_id),
        method(std::forward<std::string>(method)),
        params(std::forward<Array>(params))
    {}

    void operator()() {
        auto &log = gcm::logging::getLogger("json-rpc");

        auto response = Object();
        response["jsonrpc"] = make_string("2.0");
        response["id"] = request_id;

        try {
            auto it = registry.find(method);
            if (it == registry.end()) {
                throw MethodNotFound(request_id, method);
            }

            DEBUG(log) << "Calling method " << method << ".";

            auto &result = response["result"];
            result = it->second(params);

            DEBUG(log) << "Method " << method << " succeeded.";
        } catch (RpcException &e) {
            response["error"] = e.to_json(false);
        } catch (std::exception &e) {
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(-32603);
            error["message"] = make_string(e.what());
        } catch (...) {
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(-32603);
            error["message"] = make_string("Server error.");
        }
        
        // Notify of job done.
        promise->result = std::make_shared<Object>(response);
        promise->has_result = true;

        if (promise->notify_done != nullptr) {
            promise->notify_done->notify_one();
        }

        promise->cb.notify_one();
    }

protected:
    MethodRegistry &registry;
    std::shared_ptr<Promise> promise;
    std::shared_ptr<Value> request_id;
    std::string method;
    Array params;
};

} // namespace detail
} // namespace rpc
} // namespace json
} // namespace gcm
