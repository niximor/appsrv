#pragma once

#include <gcm/json/rpc.h>
#include <gcm/logging/logging.h>

#include "interface.h"

namespace gcm {
namespace json {
namespace rpc {

class RpcApi {
public:
    RpcApi(gcm::json::rpc::Rpc &rpc, gcm::appsrv::ServerApi &server_api, gcm::logging::Logger &logger):
        rpc(rpc),
        server_api(server_api),
        logger(logger)
    {}

    void register_method(const std::string &name, std::function<Method> callback) {
        rpc.register_method(name, callback);
    }

    void register_method(const std::string &name, std::function<Method> callback, const std::string &desc) {
        rpc.register_method(name, callback, desc);
    }

    template<typename T>
    void register_method(const std::string &name, std::function<Method> callback, T params) {
        rpc.register_method(name, callback, params);
    }

    template<typename T>
    void register_method(const std::string &name, std::function<Method> callback, const std::string &desc, T params) {
        rpc.register_method(name, callback, desc, params);
    }

    template<typename T, typename R>
    void register_method(const std::string &name, std::function<Method> callback, T params, R result) {
        rpc.register_method(name, callback, params, result);
    }

    template<typename T, typename R>
    void register_method(const std::string &name, std::function<Method> callback, const std::string &desc, T params, R result) {
        rpc.register_method(name, callback, desc, params, result);
    }

    gcm::appsrv::Stats &get_stats() {
        return server_api.handler_stats;
    }

    gcm::config::Config &get_config() {
        return server_api.config;
    }

    gcm::config::Value &get_interface_config() {
        return server_api.interface_config;
    }

    const std::string &get_handler_name() {
        return server_api.handler_name;
    }

    gcm::logging::Logger &get_logger() {
        return logger;
    }

protected:
    gcm::json::rpc::Rpc &rpc;
    gcm::appsrv::ServerApi &server_api;
    gcm::logging::Logger &logger;
};

}
}
}