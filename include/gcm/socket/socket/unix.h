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
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>

#include "types.h"
#include "generic_socket.h"
#include "exception.h"

namespace gcm {
namespace socket {

class UnixSocket;

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

    static std::pair<UnixSocket, UnixSocket> make_pair();
};

class UnixSocket: public WritableSocket<Unix> {
friend class Unix;
public:
    UnixSocket(UnixSocket &&other) = default;
    UnixSocket(): WritableSocket<Unix>(0)
    {}

    // Send other socket over this socket.
    // This socket must be of type Unix.
    template<typename OtherSocketType>
    void send_socket(OtherSocketType &socket) {
        msghdr hdr;
        iovec data;

        char dummy = '*';
        data.iov_base = &dummy;
        data.iov_len = sizeof(dummy);

        memset(&hdr, 0, sizeof(hdr));
        hdr.msg_name = NULL;
        hdr.msg_namelen = 0;
        hdr.msg_iov = &data;
        hdr.msg_iovlen = 1;
        hdr.msg_flags = 0;

        char cmsgbuf[CMSG_SPACE(sizeof(socket.fd))];
        hdr.msg_control = cmsgbuf;
        hdr.msg_controllen = CMSG_LEN(sizeof(int));

        cmsghdr *cmsg = CMSG_FIRSTHDR(&hdr);
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        *(int *)CMSG_DATA(cmsg) = socket.fd;

        int res = sendmsg(fd, &hdr, 0);
        if (res < 0) {
            throw SocketException(errno);
        }

        return *this;
    }

    // Receive other socket from 
    template<typename OtherSocketType>
    OtherSocketType receive_socket() {
        char buf[1];
        iovec iov;
        iov.iov_base = buf;
        iov.iov_len = 1;

        msghdr msg;
        memset(&msg, 0, sizeof(msg));

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        char cms[CMSG_SPACE(sizeof(int))];
        msg.msg_control = (caddr_t)cms;

        int rec = recvmsg(fd, &msg, 0);
        if (rec < 0) {
            throw SocketException(errno);
        } else if (rec == 0) {
            throw SocketException("Unexpected end of stream.");
        }

        cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

        int other_fd;
        memmove(other_fd, CMSG_DATA(cmsg), sizeof(int));

        return OtherSocketType(other_fd);
    }

protected:
    UnixSocket(int fd): WritableSocket<Unix>(fd)
    {}
};

inline std::pair<UnixSocket, UnixSocket> Unix::make_pair() {
    int fds[2];
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fds) < 0) {
        throw SocketException(errno);
    }

    return std::make_pair(UnixSocket(fds[0]), UnixSocket(fds[1]));
}

} // namespace socket
} // namespace gcm
