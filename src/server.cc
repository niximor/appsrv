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

#include <gcm/socket/server.h>
#include <gcm/logging/logging.h>
#include <gcm/config/config.h>

namespace s = gcm::socket;
namespace l = gcm::logging;
namespace c = gcm::config;

void client(s::ConnectedSocket<s::AnyIpAddress> &&client) {
	(void)client;
}

void setup_logging() {
	auto &log = l::getLogger("");
	log.add_handler(l::StdErrHandler(l::Formatter(
			l::field::Date(), " appsrv.", l::field::Name(), "[", l::field::Pid(), "] ", 
			l::field::Severenity(), ": ", l::field::Message(), " {",
			l::field::File(), ":", l::field::Line(), "}"
	)));
}

int main(void) {
	setup_logging();

	/*auto server = s::TcpServer{};
	server.listen(s::Inet6{"::", 12345});
	server.listen(s::Inet{"0.0.0.0", 12346});

	server.serve_forever(client);*/

	{
		c::Config cfg("../conf/appsrv.conf");
	}
}
