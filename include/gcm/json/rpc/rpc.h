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
#include <memory>
#include <string>
#include <map>

#include <gcm/thread/pool.h>
#include <gcm/thread/signal.h>

#include "detail.h"
#include "method_processor.h"
#include "../validator.h"

namespace gcm {
namespace json {
namespace rpc {

class Rpc {
public:
    Rpc():
        on_sigint(gcm::thread::Signal::at(SIGINT, std::bind(&Rpc::stop, this)))
    {
        using namespace std::placeholders;
        namespace v = validator;

        register_method("system.listMethods", std::bind(&Rpc::list_methods, this, _1),
            "List available methods.",
            v::ParamDefinitions(),
            v::Array("methods", "List of methods",
                v::String("method_name", "Name of each method registered on this server.")
            )
        );

        register_method("system.methodHelp", std::bind(&Rpc::method_help, this, _1),
            "Get help for method.",
            v::ParamDefinitions(
                v::String("method_name", "Name of method to get help for.")
            ),
            v::String("method_help", "Help for given method.")
        );
    }

    void stop() {
        pool.stop();
    }

    std::shared_ptr<Promise> add_work(std::shared_ptr<Value> request_id, std::string &&method, Array &&params) {
        auto p = std::make_shared<Promise>();

        pool.add_work(detail::MethodProcessor(
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
        help[name] = validator::genhelp(name, "");
    }

    void register_method(std::string name, std::function<Method> callback, std::string desc) {
        methods[name] = callback;
        help[name] = validator::genhelp(name, desc);
    }

    template<typename T>
    void register_method(std::string name, std::function<Method> callback, T params) {
        methods[name] = detail::SafeCallback(callback, params);
        help[name] = validator::genhelp(name, "", params);
    }

    template<typename T>
    void register_method(std::string name, std::function<Method> callback, std::string desc, T params) {
        methods[name] = detail::SafeCallback(callback, params);
        help[name] = validator::genhelp(name, desc, params);
    }

    template<typename T, typename R>
    void register_method(std::string name, std::function<Method> callback, T params, R result) {
        methods[name] = detail::SafeCallback(callback, params, result);
        help[name] = validator::genhelp(name, "", params, result);
    }

    template<typename T, typename R>
    void register_method(std::string name, std::function<Method> callback, std::string desc, T params, R result) {
        methods[name] = detail::SafeCallback(callback, params, result);
        help[name] = validator::genhelp(name, desc, params, result);
    }

protected:
    std::shared_ptr<Value> list_methods(Array &) {
        Array result;
        for (auto &m: methods) {
            result.push_back(make_string(m.first));
        }
        return std::make_shared<Array>(result);
    }

    std::shared_ptr<Value> method_help(Array &params) {
        std::string methodName = to<String>(params[0]);

        auto it = methods.find(methodName);
        if (it != methods.end()) {
            const std::string method_help = help[methodName];
            String result(method_help);
            return std::make_shared<String>(result);
        } else {
            throw MethodNotFound(methodName);
        }
    }

protected:
    std::map<std::string, std::function<Method>> methods;
    std::map<std::string, std::string> help;

    gcm::thread::Pool<detail::MethodProcessor> pool;
    gcm::thread::SignalBind on_sigint;
};

} // namespace gcm
} // namespace json
} // namespace gcm
