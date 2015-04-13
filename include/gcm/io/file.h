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
 * @date 2015-03-13
 *
 */

#pragma once

#include <stdio.h>

#include "exception.h"

namespace gcm {
namespace io {

/**
 * Class that wraps standard FILE * object and manages it automatically.
 */
class File {
public:
    File(const std::string &file_name, const char *mode): f(::fopen(file_name.c_str(), mode)) {
        if (!f) {
            throw gcm::io::IOException(errno);
        }
    }

    File(const File &other) = delete;

    File(File &&other): f(other.f) {
        other.f = nullptr;
    }

    operator FILE *() {
        return f;
    }

    ~File() {
        if (f) {
            ::fclose(f);
        }
    }

    std::string get_contents() {
        std::string str;

        // Backup
        long pos = ftell(f);

        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        
        str.reserve(size);

        // Seek to the start
        fseek(f, 0, SEEK_SET);

        static const std::size_t buffer_size = 65535;
        char buffer[buffer_size];

        while (!feof(f)) {
            std::size_t readed = fread(buffer, 1, buffer_size, f);
            str.append(buffer, readed);
        }

        // Restore
        fseek(f, pos, SEEK_SET);

        return str;
    }

protected:
    FILE *f;
};

} // namespace io
} // namespace gcm
