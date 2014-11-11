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

#include <gcm/config/config.h>
#include <gcm/dl/dl.h>
#include <gcm/socket/socket.h>

#include <future>

namespace gcm {
namespace appsrv {

class IntInterface {
public:
    IntInterface(gcm::config::Config &cfg, gcm::config::Value &interface);
    IntInterface(IntInterface &&) = default;
    ~IntInterface();

    void handle(gcm::socket::ConnectedSocket<gcm::socket::AnyIpAddress> client);
    void _start();
    bool start();

protected:
    gcm::dl::Library library;
    gcm::config::Value &config;
    std::future<bool> server_task;
    std::string interface_name;
    gcm::config::Config &cfgfile;
};

} // namespace appsrv
} // namespace gcm
