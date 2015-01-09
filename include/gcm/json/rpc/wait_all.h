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

#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

#include <gcm/logging/logging.h>

#include "promise.h"

namespace gcm {
namespace json {
namespace rpc {

template<typename PromiseDone>
inline void wait_all(std::vector<std::shared_ptr<Promise>> promises, PromiseDone done) {
    std::mutex m;
    std::condition_variable cv;

    for (auto &p: promises) {
        p->notify_done = &cv;
    }

    auto &log = gcm::logging::getLogger("debug");

    bool changed = false;
    while (!promises.empty()) {
        changed = false;

        for (auto it = promises.begin(); it != promises.end(); ++it) {
            if ((*it)->try_wait()) {
                // Promise has work done.
                done(**it);

                promises.erase(it);
                changed = true;
                break;
            }
        }

        if (!changed) {
            std::unique_lock<std::mutex> lock(m);
            cv.wait(lock);
        }
    }
}

} // namespace rpc
} // namespace json
} // namespace gcm
