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

#include <gcm/socket/socket.h>
#include <gcm/config/config.h>

namespace gcm {
namespace appsrv {

class Stats {
public:
    Stats(): req_received(0), req_handled(0), req_error(0)
    {}

    Stats(Stats &&) = default;
    Stats(const Stats &) = delete;

    uint64_t req_received;
    uint64_t req_handled;
    uint64_t req_error;
};

class ServerApi {
public:
    ServerApi(config::Config &config, Stats &handler_stats, const std::string &handler_name):
        config(config),
        handler_stats(handler_stats),
        handler_name(handler_name)
    {}
    ServerApi(ServerApi &&) = default;
    ServerApi(const ServerApi &) = default;

    config::Config &config;
    Stats &handler_stats;
    const std::string &handler_name;
};

/**
 * Signature of init function of each handler module.
 * @param api Published server API for the module.
 * @return Instance of Handler class.
 */
using InitSig = void *(*)(ServerApi *);

/**
 * Signature of stop function of each handler module.
 * @param handler Instance of Handler class. Module should free memory of the handler.
 */
using StopSig = void (*)(void *);

/**
 * Handler class returned by call to handler's init() method.
 */
class Handler {
public:
    virtual void handle(gcm::socket::ConnectedSocket<gcm::socket::AnyIpAddress> &&client) = 0;
    virtual ~Handler();
};

}
}