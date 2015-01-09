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

#include <utility>

#include <sys/types.h>
#include <sys/socket.h>

#include "generic_socket.h"
#include "exception.h"

namespace gcm {
namespace socket {

template<typename Address>
class ConnectedSocket;

/**
 * Server that can accept connections.
 */
template<typename Address>
class ServerSocket: public Socket<Address> {
public:
    static constexpr int DefaultListenBacklog = 50;

    ServerSocket(Type type): Socket<Address>(type) {}
    ServerSocket(const ServerSocket &other) = default;
    ServerSocket(ServerSocket &&other) = default;

    void listen(int backlog = DefaultListenBacklog) {
        if (::listen(this->fd, backlog) < 0) {
            throw SocketException(errno);
        }
    }

    template<typename T>
    ConnectedSocket<T> accept() {
        Address client;
        socklen_t addrlen = sizeof(decltype(client.get_addr()));

        int newfd = ::accept(this->fd, reinterpret_cast<sockaddr *>(&(client.get_addr())), &addrlen);
        if (newfd <= 0) {
            throw SocketException(errno);
        }

        return ConnectedSocket<T>(*this, newfd, std::move(client));
    }
};

/**
 * Client's connection to server.
 */
template<typename Address>
class ConnectedSocket: public WritableSocket<Address> {
public:
    template<typename T>
    friend class ServerSocket;

    const Address &get_client_address() const {
        return client_address;
    }

protected:
    template<typename ServerAddress>
    ConnectedSocket(ServerSocket<ServerAddress> &server, int fd, Address &&addr):
        WritableSocket<Address>(fd, std::move(Address(server.get_bind_address()))),
        client_address(addr)
    {}

protected:
    Address client_address;
};

} // namespace socket
} // namespace gcm
