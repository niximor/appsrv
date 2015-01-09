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
 * @date 2014-10-28
 *
 */

#pragma once

#include <mutex>
#include <condition_variable>

#include "../json.h"

namespace gcm {
namespace json {
namespace rpc {

namespace detail {
    class MethodProcessor;
}

class Promise {
public:
    friend class detail::MethodProcessor;

    Promise(): notify_done(nullptr), has_result(false)
    {}

    void wait() {
        while (!has_result) {
            std::unique_lock<std::mutex> lk(mutex);
            cb.wait(lk);
        }
    }

    bool try_wait() {
        return has_result;
    }

    JsonValue get() {
        wait();
        return result;
    }

public:
    std::condition_variable *notify_done;

protected:
    std::condition_variable cb;
    std::mutex mutex;

    bool has_result;
    JsonValue result;
};

} // namespace gcm
} // namespace json
} // namespace gcm
