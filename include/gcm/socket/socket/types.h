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

namespace gcm {
namespace socket {

/**
 * Socket type
 */
enum class Type {
    Stream = SOCK_STREAM, // TCP in case of Inet family
    Datagram = SOCK_DGRAM, // UDP in case of Inet family
    Raw = SOCK_RAW // RAW socket. Usually requires root privileges to open this type of socket.
};

/**
 * Generic class holding address family specific members.
 * It is used to specify socket type.
 */
template <int Family, typename Addr>
class AddrFamily {
public:
    static constexpr int family = Family;

    const Addr &get_addr() const {
        return addr;
    }

    Addr &get_addr() {
        return addr;
    }

protected:
    Addr addr;
};

} // namespace socket
} // namespace gcm
