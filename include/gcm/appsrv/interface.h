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

namespace gcm {
namespace appsrv {

class ServerApi {

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