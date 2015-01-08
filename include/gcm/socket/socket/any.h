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

#include <string>

#include <netinet/in.h>

#include "types.h"
#include "inet.h"
#include "inet6.h"

namespace gcm {
namespace socket {

struct AnyIpStorage {
    std::string address;
    in_port_t port;
};

class AnyIpAddress: public AddrFamily<0, AnyIpStorage> {
public:
    enum class Type {
        IPv4, IPv6
    };

    AnyIpAddress(Inet &&other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv4} {
        addr.address = other.get_ip();
        addr.port = other.get_port();
    }

    AnyIpAddress(const Inet &other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv4} {
        addr.address = other.get_ip();
        addr.port = other.get_port();
    }

    AnyIpAddress(Inet6 &&other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv6} {
        addr.address = other.get_ip();
        addr.port = other.get_port();
    }

    AnyIpAddress(const Inet6 &other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv6} {
        addr.address = other.get_ip();
        addr.port = other.get_port();
    }

    AnyIpAddress(const AnyIpAddress &other) = default;
    AnyIpAddress(AnyIpAddress &&other) = default;

    operator Inet() {
        if (current_type == Type::IPv4) {
            return std::move(Inet(addr.address, addr.port));
        } else {
            throw SocketException("Trying to convert IPv6 to IPv4.");
        }
    }

    operator Inet6() {
        if (current_type == Type::IPv6) {
            return std::move(Inet6(addr.address, addr.port));
        } else {
            throw SocketException("Trying to convert IPv4 to IPv6.");
        }
    }

    Type get_type() const { return current_type; }
    const std::string &get_ip() const { return addr.address; }
    in_port_t get_port() const { return addr.port; }
    int get_family() const {
        switch (current_type) {
            case Type::IPv4: return AF_INET;
            case Type::IPv6: return AF_INET6;
        }
    }

protected:
    Type current_type;
};

} // namespace socket
} // namespace gcm
