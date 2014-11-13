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
#include <functional>

#include <gcm/thread/pool.h>

#include "json.h"

namespace gcm {
namespace json {
namespace rpc {

using Method = std::shared_ptr<Value>(Array &params);
using MethodRegistry = std::map<std::string, std::function<Method>>;

class RpcException: public Exception {
public:
    RpcException(int code, std::string message): Exception(message), code(code)
    {}

    int get_code() const {
        return code;
    }

protected:
    int code;
};

class MethodNotFound: public RpcException {
public:
    MethodNotFound(std::string name): RpcException(-32601, "Method " + name + " not found.")
    {}
};

class Promise {
public:
    friend class MethodProcessor;

    void wait() {
        while (!has_result) {
            std::unique_lock<std::mutex> lk(mutex);
            cb.wait(lk);
        }
    }

    bool try_wait() {
        return has_result;
    }

    std::shared_ptr<Value> get() {
        wait();
        return result;
    }

protected:
    std::condition_variable cb;
    std::mutex mutex;

    bool has_result;
    std::shared_ptr<Value> result;
};

class MethodProcessor {
public:
    MethodProcessor(MethodRegistry &registry, std::shared_ptr<Promise> promise, int request_id, std::string &&method, Array &&params):
        registry(registry),
        promise(promise),
        request_id(request_id),
        method(std::forward<std::string>(method)),
        params(std::forward<Array>(params))
    {}

    void operator()() {
        auto response = Object();
        response["jsonrpc"] = make_string("2.0");

        try {
            // For now, no method exists.
            auto it = registry.find(method);
            if (it == registry.end()) {
                throw MethodNotFound(method);
            }

            auto &result = response["result"];
            result = it->second(params);
        } catch (RpcException &e) {
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(e.get_code());
            error["message"] = make_string(e.what());
        } catch (std::exception &e) {
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(-32000);
            error["message"] = make_string(e.what());
        } catch (...) {
            auto &error = to<Object>(response["error"] = make_object());
            error["code"] = make_int(-32000);
            error["message"] = make_string("Server error.");
        }
        
        // Notify of job done.
        promise->result = std::make_shared<Object>(response);
        promise->has_result = true;
        promise->cb.notify_one();
    }

protected:
    MethodRegistry &registry;
    std::shared_ptr<Promise> promise;
    int request_id;
    std::string method;
    Array params;
};

class Rpc {
public:
    Rpc() {
        using namespace std::placeholders;
        register_method("system.listMethods", std::bind(&Rpc::list_methods, this, _1));
    }

    std::shared_ptr<Promise> add_work(int request_id, std::string &&method, Array &&params) {
        auto p = std::make_shared<Promise>();

        pool.add_work(MethodProcessor(
            methods,
            p,
            request_id,
            std::forward<std::string>(method),
            std::forward<Array>(params)
        ));

        return p;
    }

    void register_method(std::string name, std::function<Method> callback) {
        methods[name] = callback;
    }

protected:
    std::shared_ptr<Value> list_methods(Array &) {
        Array result;
        for (auto &m: methods) {
            result.push_back(make_string(m.first));
        }
        return std::make_shared<Array>(result);
    }

protected:
    std::map<std::string, std::function<Method>> methods;
    gcm::thread::Pool<MethodProcessor> pool;
};

} // namespace rpc
} // namespace json
} // namespace gcm
