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
#include <mutex>

#include <cstring>

#include <signal.h>

#include <gcm/logging/logging.h>

namespace gcm {
namespace thread {

using Callback = std::function<void()>;

class Signal;

class SignalBind {
public:
    friend class Signal;

    SignalBind() = delete;
    SignalBind(const SignalBind &other) = delete;
    SignalBind(SignalBind &&) = default;

    SignalBind &operator=(const SignalBind &other) = delete;

    ~SignalBind();

private:
    SignalBind(Signal &signal, int signum, std::size_t pos): signal(signal), signum(signum), pos(pos) {
    }

protected:
    Signal &signal;
    int signum;
    std::size_t pos;
};

class Signal {
public:
    friend class SignalBind;

    static SignalBind at(int signum, Callback cb) {
        std::lock_guard<std::mutex> lock(self.mutex);

        if (self.callbacks.find(signum) == self.callbacks.end()) {
            // Trap new signal.
            struct sigaction act;
            std::memset(&act, 0, sizeof(act));
            act.sa_handler = &Signal::handler;

            ::sigaction(signum, &act, NULL);
        }

        auto &vect = self.callbacks[signum];

        std::size_t index = 0;
        for (auto it = vect.begin(); it != vect.end(); ++it, ++index) {
            if (*it == nullptr) {
                *it = cb;
                return SignalBind(self, signum, index);
            }
        }

        vect.push_back(cb);
        return SignalBind(self, signum, vect.size() - 1);
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
            if (call) {
                call();
            }
        }
    }

    void unbind(SignalBind &bind) {
        auto siglist = callbacks.find(bind.signum);
        if (siglist != callbacks.end()) {
            if (bind.pos < siglist->second.size()) {
                siglist->second[bind.pos] = nullptr;
            }
        }
    }

    std::map<int, std::vector<Callback>> callbacks;
    std::mutex mutex;
};

inline SignalBind::~SignalBind() {
    signal.unbind(*this);
}

} // namespace thread
} // namespace gcm
