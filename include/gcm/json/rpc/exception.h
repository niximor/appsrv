#pragma once

#include "../json.h"

namespace gcm {
namespace json {
namespace rpc {

class RpcException: public Exception {
public:
    RpcException(std::string message):
        Exception(message), code(-32603), request_id(make_null())
    {}

    RpcException(int code, std::string message):
        Exception(message), code(code), request_id(make_null())
    {}

    RpcException(std::shared_ptr<Value> request_id, int code, std::string message):
        Exception(message), code(code), request_id(request_id)
    {}

    RpcException(std::shared_ptr<Value> request_id, std::string message):
        Exception(message), code(-32603), request_id(request_id)
    {}

    int get_code() const {
        return code;
    }

    std::shared_ptr<Value> to_json(bool full_resp = true) {
        auto obj = Object();
        obj["jsonrpc"] = make_string("2.0");
        obj["id"] = request_id;

        auto &err = to<Object>(obj["error"] = make_object());
        err["code"] = make_int(code);
        err["message"] = make_string(this->what());

        if (full_resp) {
            return std::make_shared<Object>(obj);
        } else {
            return std::make_shared<Object>(err);
        }
    }

protected:
    int code;
    std::shared_ptr<Value> request_id;
};

class MethodNotFound: public RpcException {
public:
    MethodNotFound(std::string name): RpcException(-32601, "Method " + name + " not found.")
    {}

    MethodNotFound(std::shared_ptr<Value> request_id, std::string name):
        RpcException(request_id, -32601, "Method " + name + " not found.")
    {}
};

} // namespace rpc
} // namespace json
} // namespace gcm
