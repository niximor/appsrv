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

#include <functional>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "generic_socket.h"

namespace gcm {
namespace socket {

class Select {
public:
    using Callback = std::function<void()>;

    template<typename Address>
    void add(const Socket<Address> &socket, Callback rd, Callback wr, Callback ex) {
        calls.emplace_back(socket.fd, rd, wr, ex);
    }

    void select(long timeout = 0, long usec_timeout = 0) {
        fd_set rds;
        fd_set wrs;
        fd_set exs;

        FD_ZERO(&rds);
        FD_ZERO(&wrs);
        FD_ZERO(&exs);

        int highest = 0;
        for (auto call: calls) {
            if (highest < call.fd) {
                highest = call.fd;
            }

            FD_SET(call.fd, &rds);
            FD_SET(call.fd, &wrs);
            FD_SET(call.fd, &exs);
        }

        timeval tm;
        tm.tv_sec = timeout;
        tm.tv_usec = usec_timeout;

        int result;
        if (timeout == 0 && usec_timeout == 0) {
            result = ::select(highest + 1, &rds, &wrs, &exs, NULL);
        } else {
            result = ::select(highest + 1, &rds, &wrs, &exs, &tm);
        }

        if (result < 0) {
            switch (errno) {
                case EINTR: throw Interrupt(errno);
                default: throw SocketException(errno);
            }
        }

        for (auto call: calls) {
            if (FD_ISSET(call.fd, &rds)) {
                call.rd();
            }

            if (FD_ISSET(call.fd, &wrs)) {
                call.wr();
            }

            if (FD_ISSET(call.fd, &exs)) {
                call.ex();
            }
        }
    }

private:
    struct SocketCall {
        const int fd;
        Callback rd;
        Callback wr;
        Callback ex;

        SocketCall(const int fd, Callback rd, Callback wr, Callback ex): fd(fd), rd(rd), wr(wr), ex(ex) {}
    };

    std::vector<SocketCall> calls;
};

} // namespace socket
} // namespace gcm
