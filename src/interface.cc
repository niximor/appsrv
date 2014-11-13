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

#include <future>

#include <gcm/config/value.h>
#include <gcm/socket/server.h>
#include <gcm/socket/socket.h>
#include <gcm/socket/util.h>
#include <gcm/thread/pool.h>
#include <gcm/thread/signal.h>
#include <gcm/appsrv/interface.h>
#include <gcm/logging/logging.h>
#include <gcm/io/util.h>

#include "interface.h"

using namespace gcm::appsrv;
using gcm::thread::Signal;
namespace s = gcm::socket;
namespace l = gcm::logging;

Handler::~Handler() {
    
}

std::string find_handler(gcm::config::Config &cfg, std::string name, bool test_default_dir = true) {
    if (gcm::io::exists(name)) {
        return name;
    } else if (gcm::io::exists(name + ".so")) {
        return name + ".so";
    } else if (test_default_dir) {
        return find_handler(cfg, cfg.get("handler_dir", "./") + "/" + name, false);
    } else {
        return name;
    }
}

IntInterface::IntInterface(gcm::config::Config &cfg, gcm::config::Value &interface):
    library(find_handler(cfg, interface["handler"].asString())),
    config(interface),
    interface_name(interface["name"].asString()),
    cfgfile(cfg)
{}

IntInterface::~IntInterface() {
    // Collect result and correctly join server thread.
    server_task.get();
}

void IntInterface::_start() {
    server_task = std::async(std::launch::async, &IntInterface::start, this);
}

class IntHandler;

class ClientProcessor {
public:
    ClientProcessor(s::ConnectedSocket<s::AnyIpAddress> &&client, Handler *handler, IntHandler &int_handler):
        client(std::forward<s::ConnectedSocket<s::AnyIpAddress>>(client)),
        handler(handler),
        int_handler(int_handler)
    {}
    ClientProcessor(ClientProcessor &&) = default;

    void operator()();

protected:
    s::ConnectedSocket<s::AnyIpAddress> client;
    Handler *handler;
    IntHandler &int_handler;
};

class IntHandler {
public:
    friend class ClientProcessor;

    IntHandler(gcm::dl::Library &lib, gcm::config::Value &cfg, ServerApi &api):
        library(lib),
        handler((Handler *)(library.get<void *, void *>("init")(&api))),
        pool(gcm::thread::make_pool<ClientProcessor>(cfg.get("MinThreads", 5), cfg.get("MaxThreads", 100))),
        name(cfg["name"].asString()),
        handler_stats(api.handler_stats)
    {
        auto &log = l::getLogger(name);
        INFO(log) << "Handler " << name << " initialized successfully.";
    }

    ~IntHandler() {
        pool->stop();

        // Cleanup the module.
        try {
            library.get<void, void *>("stop")(handler);
        } catch (std::exception &e) {
            auto &log = l::getLogger(name);
            ERROR(log) << "Caught exception while executing handler stop function: " << e.what();
        } catch (...) {
            auto &log = l::getLogger(name);
            ERROR(log) << "Caught unknown exception while executing handler stop function.";
        }
    }

    void operator()(s::ConnectedSocket<s::AnyIpAddress> &&client) {
        ++handler_stats.req_received;
        pool->add_work(ClientProcessor(
            std::forward<s::ConnectedSocket<s::AnyIpAddress>>(client),
            handler,
            *this
        ));
    }

protected:
    gcm::dl::Library &library;
    Handler *handler;
    std::shared_ptr<gcm::thread::Pool<ClientProcessor>> pool;
    std::string name;
    Stats &handler_stats;
};

bool IntInterface::start() {
    auto &log = gcm::logging::getLogger("");

    auto server = s::TcpServer{};

    bool listens = false;

    // Listen on all given interfaces.
    for (auto &listen: config.getAll("listen")) {
        auto s = listen->asString();

        auto port = s::util::get_port(s.begin(), s.end());
        auto ipv4 = s::util::get_ipv4(s.begin(), s.end());
        auto ipv6 = s::util::get_ipv6(s.begin(), s.end());

        if (port > 0) {
            if (ipv4.first != s.end()) {
                server.listen(s::Inet{std::string(ipv4.first, ipv4.second), port});
                listens = true;
            } else if (ipv6.first != s.end()) {
                server.listen(s::Inet6{std::string(ipv6.first, ipv6.second), port});
                listens = true;
            }
        }
    }

    if (!listens) {
        ERROR(log) << "You must define at least one listen directive for interface " << interface_name;
        return false;
    }

    Stats handler_stats;

    // Stop server at sigint.
    Signal::at(SIGINT, std::bind(&s::TcpServer::stop, &server));

    ServerApi api{cfgfile, handler_stats, config["name"].asString()};

    IntHandler handler(library, config, api);
    server.serve_forever(handler);

    return true;
}

void ClientProcessor::operator()() {
    try {
        handler->handle(std::move(client));
        ++int_handler.handler_stats.req_handled;
    } catch (std::exception &e) {
        auto &log = l::getLogger(int_handler.name);
        ERROR(log) << "Caught exception while processing connection: " << e.what();
        ++int_handler.handler_stats.req_error;
    } catch (...) {
        auto &log = l::getLogger(int_handler.name);
        ERROR(log) << "Caught unknown exception while processing connection.";
        ++int_handler.handler_stats.req_error;
    }
}