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

#include "../json.h"
#include "types.h"

namespace gcm {
namespace json {
namespace rpc {

class RpcException: public Exception {
public:
    RpcException(std::string &&message):
        RpcException(
            make_null(),
            ErrorCode::InternalError,
            std::forward<std::string>(message),
            make_null()
        )
    {}

    RpcException(std::string &&message, JsonValue &&data):
        RpcException(
            make_null(),
            ErrorCode::InternalError,
            std::forward<std::string>(message),
            std::forward<JsonValue>(data)
        )
    {}

    RpcException(int code, std::string &&message):
        RpcException(
            make_null(),
            code,
            std::forward<std::string>(message),
            make_null()
        )
    {}

    RpcException(int code, std::string &&message, JsonValue &&data):
        RpcException(
            make_null(),
            code,
            std::forward<std::string>(message),
            std::forward<JsonValue>(data)
        )
    {}

    RpcException(ErrorCode code, std::string &&message):
        RpcException(
            make_null(),
            code,
            std::forward<std::string>(message),
            make_null()
        )
    {}

    RpcException(ErrorCode code, std::string &&message, JsonValue &&data):
        RpcException(
            make_null(),
            code,
            std::forward<std::string>(message),
            std::forward<JsonValue>(data)
        )
    {}

    RpcException(JsonValue &&request_id, std::string &&message):
        RpcException(
            std::forward<JsonValue>(request_id),
            ErrorCode::InternalError,
            std::forward<std::string>(message),
            make_null()
        )
    {}

    RpcException(JsonValue &&request_id, std::string &&message, JsonValue &&data):
        RpcException(
            std::forward<JsonValue>(request_id),
            ErrorCode::InternalError,
            std::forward<std::string>(message),
            std::forward<JsonValue>(data)
        )
    {}

    RpcException(JsonValue &&request_id, ErrorCode code, std::string &&message):
        RpcException(
            std::forward<JsonValue>(request_id),
            code,
            std::forward<std::string>(message),
            make_null()
        )
    {}

    RpcException(JsonValue &&request_id, ErrorCode code, std::string &&message, JsonValue &&data):
        RpcException(
            std::forward<JsonValue>(request_id),
            static_cast<int>(code),
            std::forward<std::string>(message),
            std::forward<JsonValue>(data)
        )
    {}

    RpcException(JsonValue &&request_id, int code, std::string &&message):
        RpcException(
            std::forward<JsonValue>(request_id),
            code,
            std::forward<std::string>(message),
            make_null()
        )
    {}

    RpcException(JsonValue &&request_id, int code, std::string &&message, JsonValue &&data):
        Exception(std::forward<std::string>(message)),
        code(code),
        request_id(std::forward<JsonValue>(request_id)),
        data(std::forward<JsonValue>(data))
    {}

    int get_code() const {
        return code;
    }

    JsonValue to_json(bool full_resp = true) {
        auto obj = Object();
        obj["jsonrpc"] = make_string("2.0");
        obj["id"] = request_id;

        auto &err = to<Object>(obj["error"] = make_object());
        err["code"] = make_int(code);
        err["message"] = make_string(this->what());

        if (!data->is_null()) {
            err["data"] = data;
        }

        if (full_resp) {
            return std::make_shared<Object>(std::move(obj));
        } else {
            return std::make_shared<Object>(std::move(err));
        }
    }

protected:
    int code;
    JsonValue request_id;
    JsonValue data;
};

class MethodNotFound: public RpcException {
public:
    MethodNotFound(std::string name): RpcException(ErrorCode::MethodNotFound, "Method " + name + " not found.")
    {}

    MethodNotFound(JsonValue &&request_id, std::string name):
        RpcException(std::forward<JsonValue>(request_id), ErrorCode::MethodNotFound, "Method " + name + " not found.")
    {}
};

} // namespace rpc
} // namespace json
} // namespace gcm
