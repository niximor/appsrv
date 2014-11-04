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
 * @date 2014-11-02
 *
 */

#pragma once

#include <string>
#include <iostream>

namespace gcm {
namespace io {

constexpr const char PathSeparator = '/';

std::string basename(const std::string &s, const std::string &ext = "") {
    auto slash_pos = s.rfind(PathSeparator);

    if (slash_pos == std::string::npos) {
        slash_pos = 0;
    } else {
        ++slash_pos;
    }

    return s.substr(slash_pos, s.size() - slash_pos - ext.size());
}

std::string dirname(const std::string &s) {
    auto slash_pos = s.rfind(PathSeparator);
    if (slash_pos == std::string::npos) {
        return ".";
    } else {
        return s.substr(0, slash_pos - 1);
    }
}

} // namespace io
} // namespace gcm
