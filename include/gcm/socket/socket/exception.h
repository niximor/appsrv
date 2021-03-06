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

#include <exception>
#include <sstream>

#include <string.h>

namespace gcm {
namespace socket {

class SocketException: public std::exception {
public:
    SocketException(int &num) {
        std::stringstream ss;
        ss << ::strerror(num) << " (errno: " << num << ")";
        msg = ss.str();
    }
    SocketException(const std::string &msg): msg(msg) {}
    SocketException(const SocketException &other) = default;
    SocketException(SocketException &&other) = default;

    const char *what() const noexcept {
        return msg.c_str();
    }

private:
    std::string msg;
};

class Interrupt: public SocketException {
public:
    Interrupt(int &num): SocketException(num) {}
    Interrupt(const std::string &msg): SocketException(msg) {}
    Interrupt(const Interrupt &other) = default;
    Interrupt(Interrupt &&other) = default;
};

} // namespace socket
} // namespace gcm
