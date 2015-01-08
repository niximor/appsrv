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

namespace gcm {
namespace socket {

/**
 * Inet6 class means IPv6.
 */
class Inet6: public AddrFamily<AF_INET6, sockaddr_in6> {
public:
    static constexpr int DefaultFlowInfo = 0;
    static constexpr int DefaultScopeId = 0;

    Inet6(int flowInfo = DefaultFlowInfo, int scopeId = DefaultScopeId): AddrFamily<AF_INET6, sockaddr_in6>() {
        addr.sin6_family = Inet6::family;
        addr.sin6_addr = IN6ADDR_ANY_INIT;
        addr.sin6_flowinfo = flowInfo;
        addr.sin6_scope_id = scopeId;
    }

    Inet6(const Inet6 &other): AddrFamily<AF_INET6, sockaddr_in6>(other) {
        // TODO: Do we need to copy addr.sin6_addr.s6_addr here?
        // Where is it stored?
    }

    Inet6(Inet6 &&other): AddrFamily<AF_INET6, sockaddr_in6>(other) {
        other.addr.sin6_addr = IN6ADDR_ANY_INIT;
    }

    Inet6(const std::string &ip, in_port_t port): AddrFamily<AF_INET6, sockaddr_in6>() {
        addr.sin6_family = Inet6::family;
        addr.sin6_flowinfo = DefaultFlowInfo;
        addr.sin6_scope_id = DefaultScopeId;

        set_ip(ip);
        set_port(port);
    }

    Inet6(const in6_addr &ip, in_port_t port): AddrFamily<AF_INET6, sockaddr_in6>() {
        addr.sin6_family = Inet6::family;
        addr.sin6_flowinfo = DefaultFlowInfo;
        addr.sin6_scope_id = DefaultScopeId;

        set_ip(ip);
        set_port(port);
    }

    Inet6 &operator=(const Inet6 &other) = default;

    void set_port(in_port_t port) {
        addr.sin6_port = htons(port);
    }

    in_port_t get_port() const {
        return ntohs(addr.sin6_port);
    }

    void set_ip(const in6_addr &ip) {
        addr.sin6_addr = ip;
    }

    void set_ip(const std::string &host) {
        switch (::inet_pton(Inet6::family, host.c_str(), &addr.sin6_addr.s6_addr)) {
            case 0: throw SocketException("Invalid address supplied.");
            case -1: throw SocketException(errno);
        }
    }

    const std::string get_ip() const {
        char dst[INET6_ADDRSTRLEN];
        if (::inet_ntop(Inet6::family, &(addr.sin6_addr), dst, INET6_ADDRSTRLEN) == NULL) {
            throw SocketException(errno);
        }

        return dst;
    }
};

} // namespace socket
} // namespace gcm
