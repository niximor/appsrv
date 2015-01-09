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
 * @date 2015-01-08
 *
 */

#pragma once

#include <sys/socket.h>

#include "types.h"
#include "generic_socket.h"

namespace gcm {
namespace socket {

/**
 * Connection to server.
 */
template<typename Address, Type type = Type::Stream>
class ClientSocket: public WritableSocket<Address> {
public:
    ClientSocket(const Address &addr): WritableSocket<Address>(type)
    {
        connect(addr);
    }

    ClientSocket(ClientSocket &&other) = default;

    const Address &get_server_address() const {
        return server_address;
    }

    void connect(const Address &address) {
        server_address = address;

        if (::connect(this->fd, &server_address.addr, sizeof(server_address.addr)) < 0) {
            throw SocketException(errno);
        }
    }

protected:
    Address server_address;
};

} // namespace socket
} // namespace gcm
