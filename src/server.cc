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

namespace s = gcm::socket;
namespace l = gcm::logging;

void client(s::ConnectedSocket<s::AnyIpAddress> &&client) {
	std::string readed;
	readed.reserve(1024);

	

	std::cout << "Client " << client.get_client_address().get_ip() << ":" << client.get_client_address().get_port() << " connected." << std::endl;

	while (!client.eof()) {
		client >> readed;

		std::cout << "Received: '" << readed << "'" << std::endl;

		client << readed;
	}

	std::cout << "Client disconnected." << std::endl;
}

int main(void) {
	auto &log = l::getLogger("");
	log.add_handler(l::StdErrHandler(l::Formatter(
			l::field::Date(), " appsrv.", l::field::Name(), "[", l::field::Pid(), "] ", 
			l::field::Severenity(), ": ", l::field::Message(), " {",
			l::field::File(), ":", l::field::Line(), "}"
	)));

	DEBUG(log) << "Test message";

	auto server = s::TcpServer{};
	server.listen(s::Inet6{"::", 12345});
	server.listen(s::Inet{"0.0.0.0", 12346});

	server.serve_forever(client);
}

