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

#include <iostream>
#include <fstream>
#include <vector>

#include "value.h"
#include "parser.h"

#include <gcm/logging/logging.h>
#include <gcm/io/exception.h>

namespace gcm {
namespace config {

class Config {
public:
    Config(const std::string &file): val(Value::StructType()), log(gcm::logging::getLogger("GCM.ConfigParser")) {
        std::ifstream f(file, std::ios_base::in);
        std::string contents((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

        Parser p(val, file);
        try {
            p.parse(contents.begin(), contents.end());
        } catch (parse_error &e) {
            ERROR(log) << e.what();
        }
    }

    Config(Config &&) = default;
    Config(const Config &) = default;

    Value &operator[](const std::string &index) {
        return val[index];
    }

    bool hasItem(const std::string &index) {
        return val.hasItem(index);
    }

    template<typename T>
    auto get(const std::string &index, T &&def) {
        return val.get(index, def);
    }

    auto get(const std::string &index, const char *def) {
        return val.get(index, std::string(def));
    }

    auto getAll(const std::string &index) {
        return val.getAll(index);
    }

private:
    Value val;
    gcm::logging::Logger &log;
};

} // namespace config
} // namespace gcm
