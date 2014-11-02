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

#include <cstring>
#include <glob.h>

namespace gcm {
namespace io {

class Glob {
public:
    class iterator {
    public:
        friend class Glob;

        iterator &operator++() {
            if (pos < gl.buf.gl_pathc) ++pos;
            return *this;
        }

        iterator &operator++(int) {
            if (pos < gl.buf.gl_pathc) ++pos;
            return *this;
        }

        iterator &operator--() {
            if (pos > 0) --pos;
            return *this;
        }

        iterator &operator--(int) {
            if (pos > 0) --pos;
            return *this;
        }

        bool operator==(const iterator &other) {
            return &other.gl == &gl && other.pos == pos;
        }

        bool operator!=(const iterator &other) {
            return &other.gl != &gl || other.pos != pos;
        }

        const std::string operator*() {
            if (pos < gl.buf.gl_pathc) {
                return gl.buf.gl_pathv[pos];
            } else {
                return std::string();
            }
        }

    protected:
        Glob &gl;
        size_t pos;

        iterator(Glob &gl, int pos): gl(gl), pos(pos) {}
    };

    Glob(const std::string &path) {
        ::glob(path.c_str(), 0, &Glob::err, &buf);
    }

    ~Glob() {
        ::globfree(&buf);
    }

    iterator begin() {
        return iterator(*this, 0);
    }

    iterator end() {
        return iterator(*this, buf.gl_pathc);
    }

protected:
    glob_t buf;

    static int err(const char *path, int err) {
        throw std::runtime_error(std::string(path) + ": " + std::strerror(err));
    }
};

}
}