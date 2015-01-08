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

#include <type_traits>
#include <string>
#include <vector>

#include <string.h>

#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "exception.h"

#include <gcm/logging/logger.h>

namespace gcm {
namespace socket {

template<typename T, std::size_t size>
constexpr std::size_t ArraySize(T (&)[size]) {
    return size;
}

class binary_flag {
public:
    constexpr binary_flag(bool binary): binary(binary)
    {}
    constexpr bool is_binary() { return binary; }

protected:
    bool binary;
};

constexpr const binary_flag binary(true);
constexpr const binary_flag ascii(false);

template<typename T>
struct is_char: std::integral_constant<bool,
    std::is_same<std::remove_cv_t<std::decay_t<T>>, char>::value ||
    std::is_same<std::remove_cv_t<std::decay_t<T>>, unsigned char>::value ||
    std::is_same<std::remove_cv_t<std::decay_t<T>>, signed char>::value ||
    std::is_same<std::remove_cv_t<std::decay_t<T>>, wchar_t>::value>
{};

/**
 * Generic socket class
 */
template<typename Address>
class Socket {
public:
    friend class Select;

    static constexpr int DefaultListenBacklog = 50;
    typedef Address Family;

    Socket(Type type): fd(::socket(Address::family, static_cast<int>(type), 0)), bind_address() {
        if (fd <= 0) {
            throw SocketException(errno);
        }
    }

    Socket(Socket<Address> &&other): fd(other.fd), bind_address(std::move(other.bind_address)) {
        other.fd = 0;
    }

    ~Socket() {
        if (fd > 0) {
            close(fd);
        }
    }

    const Address &get_bind_address() {
        return bind_address;
    }

    void bind(Address bind_addr) {
        if (::bind(fd, reinterpret_cast<const sockaddr *>(&bind_addr.addr), sizeof(bind_addr.addr)) < 0) {
            throw SocketException(errno);
        }
        bind_address = bind_addr;
    }

    /**
     * Bind to device (eth0 for example).
     */
    void bind(std::string device) {
        setopt(SO_BINDTODEVICE, device);
    }

    template<typename T>
    void setopt(int optname, T val, int level = SOL_SOCKET) {
        if (::setsockopt(fd, level, optname, &val, sizeof(T)) < 0) {
            throw SocketException(errno);
        }
    }

    void setopt(int optname, std::string val, int level = SOL_SOCKET) {
        if (::setsockopt(fd, level, optname, val.c_str(), val.size()) < 0) {
            throw SocketException(errno);
        }
    }

    template<typename T>
    T getopt(int optname, int level = SOL_SOCKET) {
        T out;
        ::getsockopt(fd, level, &out, sizeof(T));

        return out;
    }

protected:
    int fd;
    Address bind_address;

    Socket(int fd, Address &&addr): fd(fd), bind_address(addr)
    {}

    Socket(int fd, const Address &addr): fd(fd), bind_address(addr)
    {}

    Socket(int fd): fd(fd)
    {}
};

template<typename Address>
class WritableSocket: public Socket<Address> {
public:
    WritableSocket(WritableSocket &&other) = default;

    bool eof() { return eof_flag; }

    ssize_t write(void *val, size_t size) {
        sigset_t sigpipe_mask;
        sigemptyset(&sigpipe_mask);
        sigaddset(&sigpipe_mask, SIGPIPE);
        sigset_t saved_mask;

        // TODO: Some error checking would be fine.
        pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask);

        ssize_t written = ::write(this->fd, val, size);
        int err = 0;
        if (written < 0) {
            err = errno;
        }
        
        struct timespec zerotime = {0, 0};
        sigtimedwait(&sigpipe_mask, 0, &zerotime);

        pthread_sigmask(SIG_SETMASK, &saved_mask, 0);

        if (written < 0) {
            throw SocketException(err);
        }

        auto &log = logging::getLogger("GCM.Socket");
        DEBUG(log) << "<< " << std::string((char *)val, size);

        return written;
    }

    ssize_t read(void *val, size_t size) {
        return ::read(this->fd, val, size);
    }

    // Set binary flag.
    WritableSocket &operator<<(binary_flag binary) {
        binary_flag = binary.is_binary();
        return *this;
    };

    template<typename T>
    typename std::enable_if<is_char<T>::value, WritableSocket &>::type
    operator <<(const T &val) {
        write((void *)&val, sizeof(val));
        return *this;
    }

    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value && !is_char<T>::value, WritableSocket &>::type
    operator <<(const T &val) {
        if (binary_flag) {
            // Write as binary
            write((void *)&val, sizeof(val));
        } else {
            // Write as ASCII
            auto s = std::to_string(val);
            write((void *)s.c_str(), s.size());
        }

        return *this;
    }

    template<typename T>
    WritableSocket &operator <<(const std::basic_string<T> &val) {
        using s = std::basic_string<T>;
        write((void *)val.c_str(), val.size() * sizeof(typename s::value_type));

        return *this;
    }

    WritableSocket &operator<<(const char *str) {
        write((void *)str, ::strlen(str));

        return *this;
    }

    // Read from socket, for arithmetic types
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, WritableSocket &>::type
    operator >>(T &val) {
        ssize_t readed = read((void *)&val, sizeof(val));
        if (readed < 0) {
            throw SocketException(errno);
        }

        if (readed == 0) {
            eof_flag = true;
        }

        return *this;
    }

    // Read from socket for strings.
    template<typename T>
    WritableSocket &operator >>(std::basic_string<T> &val) {
        using s = std::basic_string<T>;
        auto capacity = val.capacity();
        if (capacity <= 0) {
            capacity = 1;
        }

        std::vector<typename s::value_type> dt;
        dt.reserve(capacity);

        // Need to convert capacity to bytes and read specified number of bytes.
        ssize_t received_size = read((void *)&dt[0], capacity * sizeof(typename s::value_type));
        if (received_size < 0) {
            throw SocketException(errno);
        }

        if (received_size == 0) {
            eof_flag = true;
        }

        val.assign(&dt[0], received_size / sizeof(typename s::value_type));

        return *this;
    }

protected:
    WritableSocket(int fd, Address &&addr): Socket<Address>(fd, addr), eof_flag(false), binary_flag(true) {}
    WritableSocket(int fd, const Address &addr): Socket<Address>(fd, addr), eof_flag(false), binary_flag(true) {}

private:
    bool eof_flag;
    bool binary_flag;
};

} // namespace socket
} // namespace gcm