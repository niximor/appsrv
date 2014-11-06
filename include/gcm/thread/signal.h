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

#include <functional>
#include <map>
#include <vector>

#include <cstring>

#include <signal.h>

namespace gcm {
namespace thread {

class Signal {
public:
    using Callback = std::function<void()>;

    static void at(int signum, Callback cb) {
        if (self.callbacks.find(signum) == self.callbacks.end()) {
            // Trap new signal.
            struct sigaction act;
            std::memset(&act, 0, sizeof(act));
            act.sa_handler = &Signal::handler;

            ::sigaction(signum, &act, NULL);
        }

        self.callbacks[signum].push_back(cb);
    }

    static Signal &get_instance() {
        return self;
    }

protected:
    static Signal self;

    Signal()
    {}

    static void handler(int signum) {
        for (auto &call: Signal::get_instance().callbacks[signum]) {
            call();
        }
    }

    std::map<int, std::vector<Callback>> callbacks;
};

} // namespace thread
} // namespace gcm
