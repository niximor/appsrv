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

#include <string.h>

#include <sys/socket.h>
#include <linux/un.h>

#include "types.h"

namespace gcm {
namespace socket {

/**
 * Unix socket
 */
class Unix: AddrFamily<AF_UNIX, sockaddr_un> {
public:
    Unix(): AddrFamily<AF_UNIX, sockaddr_un>() {
        addr.sun_family = AF_UNIX;
        ::memset(addr.sun_path, 0, UNIX_PATH_MAX);
    }

    Unix(const Unix &other): AddrFamily<AF_UNIX, sockaddr_un>(other) {
        ::memcpy(addr.sun_path, other.addr.sun_path, UNIX_PATH_MAX);
    }

    Unix(Unix &&other) = default;

    void set_path(const std::string &path) {
        size_t len = path.size() + 1;
        if (len > UNIX_PATH_MAX) {
            len = UNIX_PATH_MAX;
        }

        ::memcpy(addr.sun_path, path.c_str(), len);

        // Prevent buffer overflow
        if (len == UNIX_PATH_MAX) {
            addr.sun_path[UNIX_PATH_MAX - 1] = '\0';
        }
    }

    const std::string get_path() const {
        return addr.sun_path;
    }
};

} // namespace socket
} // namespace gcm
