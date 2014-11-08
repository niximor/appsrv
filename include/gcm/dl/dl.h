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

#include <exception>

#include <dlfcn.h>

namespace gcm {
namespace dl {

class DlError: public std::runtime_error {
public:
    DlError(const std::string &what): std::runtime_error(what)
    {}

    DlError(const char *what): std::runtime_error(what)
    {}

    DlError(DlError &&) = default;
    DlError(const DlError &) = default;
};

class Library {
public:
    Library(const std::string &path):
        libhandle(::dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL))
    {
        if (libhandle == nullptr) {
            throw DlError(::dlerror());
        }
    }

    Library(const Library &) = delete;
    Library(Library &&other): libhandle(other.libhandle)
    {
        other.libhandle = nullptr;
    }

    ~Library() {
        if (libhandle != nullptr) {
            ::dlclose(libhandle);
        }
    }

    template<typename Result, typename... Args>
    std::function<Result(Args...)> get(const std::string &symbol) {
        void *call = dlsym(libhandle, symbol.c_str());
        if (call == nullptr) {
            throw DlError(::dlerror());
        }

        return (Result (*)(Args...))call;
    }

    template<typename T>
    T &getval(const std::string &symbol) {
        void *sym = dlsym(libhandle, symbol.c_str());
        if (sym == nullptr) {
            throw DlError(::dlerror());
        }

        return (T &)(*sym);
    }

protected:
    void *libhandle;
};

} // namespace dl
} // namespace gcm