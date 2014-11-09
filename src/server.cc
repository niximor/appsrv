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

#include <vector>
#include <chrono>

#include <unistd.h>

#include <gcm/logging/logging.h>
#include <gcm/logging/util.h>
#include <gcm/config/config.h>
#include <gcm/io/io.h>
#include <gcm/thread/signal.h>
#include "interface.h"

#include "interface.h"

namespace s = gcm::socket;
namespace l = gcm::logging;
namespace c = gcm::config;

using namespace gcm::appsrv;

class InfiniteLoop {
public:
    InfiniteLoop(): quit(false)
    {}

    void stop() {
        quit = true;
    }

    void operator()() {
        gcm::thread::Signal::at(SIGINT, std::bind(&InfiniteLoop::stop, this));

        while (!quit) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

protected:
    bool quit;
};

int main(int, char *argv[]) {
	std::string appname{gcm::io::basename(argv[0])};
	l::util::setup_logging(appname, {
        l::MessageType::Critical,
        l::MessageType::Error,
        l::MessageType::Warning,
        l::MessageType::Info,
        l::MessageType::Debug
    });

	c::Config cfg("../conf/appsrv.conf");
	l::util::setup_logging(appname, cfg);

    std::vector<IntInterface> interfaces;

	auto cfg_interfaces = cfg.getAll("interface");
    for (auto &interface: cfg_interfaces) {
        interfaces.emplace_back(cfg, *interface);
        interfaces.back()._start();
    }

    if (interfaces.empty()) {
        INFO(l::getLogger("")) << "No interfaces configured. Quit.";
    } else {
        auto loop = InfiniteLoop();
        loop();

        INFO(l::getLogger("")) << "Server quit.";
    }
}
