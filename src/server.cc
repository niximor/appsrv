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

#include <vector>
#include <thread>

#include <gcm/socket/server.h>
#include <gcm/logging/logging.h>
#include <gcm/logging/util.h>
#include <gcm/config/config.h>
#include <gcm/io/io.h>
#include <gcm/thread/pool.h>
#include <gcm/thread/signal.h>

#include "interface.h"

namespace s = gcm::socket;
namespace l = gcm::logging;
namespace c = gcm::config;

using namespace gcm::appsrv;

class ClientProcessor {
public:
    ClientProcessor(s::ConnectedSocket<s::AnyIpAddress> &&client):
        client(std::forward<s::ConnectedSocket<s::AnyIpAddress>>(client))
    {}
    ClientProcessor(ClientProcessor &&) = default;

    void operator()() {
        auto &log = l::getLogger("client");
        auto &addr = client.get_client_address();

        DEBUG(log) << "Handle client " << addr.get_ip() << ":" << addr.get_port() << ".";

        client << std::string("Hello!\r\n");
        sleep(5);
        client << std::string("Bye!\r\n");

        DEBUG(log) << "Client " << addr.get_ip() << ":" << addr.get_port() << " handled.";
    }

protected:
    s::ConnectedSocket<s::AnyIpAddress> client;
};

class Handler {
public:
    Handler(): pool(gcm::thread::make_pool<ClientProcessor>(5, 5))
    {}

    ~Handler() {
        pool->stop();
    }

    void operator()(s::ConnectedSocket<s::AnyIpAddress> &&client) {
        auto &log = l::getLogger("client");
        auto &addr = client.get_client_address();

        DEBUG(log) << "New connection from " << addr.get_ip() << ":" << addr.get_port() << ".";
        pool->add_work(ClientProcessor(std::forward<s::ConnectedSocket<s::AnyIpAddress>>(client)));
    }

protected:
    std::shared_ptr<gcm::thread::Pool<ClientProcessor>> pool;
};

int main(int, char *argv[]) {
	std::string appname{gcm::io::basename(argv[0])};
	l::util::setup_logging(appname, {
        l::MessageType::Critical,
        l::MessageType::Error,
        l::MessageType::Warning,
        l::MessageType::Info,
        l::MessageType::Debug
    });

	c::Config cfg("../conf/appsrv.conf");
	l::util::setup_logging(appname, cfg);

    std::vector<Interface> interfaces;

    auto &cfg_interfaces = cfg.getAll("interface");
    for (auto &if: cfg_interfaces) {
        interfaces.emplace_back(if);
        interfaces.back()._start();
    }

    INFO(l::getLogger("")) << "Server quit.";
}
