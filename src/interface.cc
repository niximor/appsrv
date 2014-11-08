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

#include "interface.h"

using namespace gcm::appsrv;
using gcm::thread::Signal;

Interface::Interface(gcm::config::Value &interface):
    library(interface["handler"].asString()),
    config(interface)
{}

Interface::~Interface() {
    // Collect result and correctly join server thread.
    server_task.get();
}

void Interface::handle(gcm::socket::ConnectedSocket<gcm::socket::AnyIpAddress> client) {
    library.get("handle")(&client);
}

void Interface::_start() {
    server_task = std::async(&Interface::start, &(interfaces.back()), std::launch::async);
}

bool Interface::start() {
    auto server = s::TcpServer{};

    for (auto &listen: config.getAll("listen")) {
        auto &s = listen.asString();
        if (is_ipv6(s)) {
            // V6
        } else {
            // V4
        }


        server.listen(s::Inet6{"::", 12345});
    }

    server.listen(s::Inet{"0.0.0.0", 12346});

    Signal::at(SIGINT, std::bind(&s::TcpServer::stop, &server));

    Handler handler;
    server.serve_forever(handler);

    return true;
}