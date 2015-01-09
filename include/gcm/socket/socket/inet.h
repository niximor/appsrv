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
#include <netinet/in.h>
#include <arpa/inet.h>

#include "exception.h"
#include "types.h"

namespace gcm {
namespace socket {

/**
 * Inet class means IPv4.
 */
class Inet: public AddrFamily<AF_INET, sockaddr_in> {
public:
    Inet(): AddrFamily<AF_INET, sockaddr_in>() {
        addr.sin_family = Inet::family;
        addr.sin_port = htons(0);
        addr.sin_addr.s_addr = INADDR_ANY;
    }

    Inet(const Inet &other) = default;
    Inet(Inet &&other) = default;

    Inet(const std::string &ip, in_port_t port): AddrFamily<AF_INET, sockaddr_in>() {
        addr.sin_family = Inet::family;
        set_ip(ip);
        set_port(port);
    }

    Inet(const in_addr &ip, in_port_t port): AddrFamily<AF_INET, sockaddr_in>() {
        addr.sin_family = Inet::family;
        set_ip(ip);
        set_port(port);
    }

    Inet &operator=(const Inet &other) = default;

    void set_port(int port) {
        addr.sin_port = htons(port);
    }

    int get_port() const {
        return ntohs(addr.sin_port);
    }

    void set_ip(const in_addr &ip) {
        addr.sin_addr = ip;
    }

    void set_ip(const std::string &host) {
        int result = ::inet_pton(Inet::family, host.c_str(), &addr.sin_addr.s_addr);
        if (result == 0) {
            throw SocketException("Invalid address supplied.");
        } else if (result < 0) {
            throw SocketException(errno);
        }
    }

    const std::string get_ip() const {
        char dst[INET_ADDRSTRLEN];
        if (::inet_ntop(Inet::family, &(addr.sin_addr), dst, INET_ADDRSTRLEN) == NULL) {
            throw SocketException(errno);
        }

        return dst;
    }
};

} // namespace socket
} // namespace gcm
