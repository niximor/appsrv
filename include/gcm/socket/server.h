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

#include <gcm/logging/logging.h>

#include "socket.h"

namespace gcm {
namespace socket {

class TcpServer {
public:
	TcpServer(): log(gcm::logging::getLogger("TcpServer")) {}
	TcpServer(TcpServer &&other) = default;

	void listen(Inet6 &&address) {
		in6_listens.emplace_back(Type::Stream);
		in6_listens.back().bind(address);
		in6_listens.back().listen();

		INFO(log) << "Listening on [" << address.get_ip() << "]:" << address.get_port();
	}

	void listen(Inet &&address) {
		in_listens.emplace_back(Type::Stream);
		in_listens.back().bind(address);
		in_listens.back().listen();
		
		INFO(log) << "Listening on " << address.get_ip() << ":" << address.get_port();
	}

	void serve_forever(std::function<void(ConnectedSocket<Inet> &&)> handle, std::function<void(ConnectedSocket<Inet6> &&)> handle_v6) {
		try {
			while (true) {
				Select select;
				for (ServerSocket<Inet6> &s: in6_listens) {
					select.add(s, AcceptHandler<Inet6>{s, handle_v6}, nullptr, nullptr);
				}

				for (ServerSocket<Inet> &s: in_listens) {
					select.add(s, AcceptHandler<Inet>{s, handle}, nullptr, nullptr);
				}

				select.select();
			}
		} catch (Interrupt e) {
		}
	}

	void serve_forever(std::function<void(ConnectedSocket<AnyIpAddress> &&)> handle) {
		try {
			while (true) {
				Select select;
				for (ServerSocket<Inet6> &s: in6_listens) {
					select.add(s, AcceptHandler<Inet6, AnyIpAddress>{s, handle}, nullptr, nullptr);
				}

				for (ServerSocket<Inet> &s: in_listens) {
					select.add(s, AcceptHandler<Inet, AnyIpAddress>{s, handle}, nullptr, nullptr);
				}

				select.select();
			}
		} catch (Interrupt e) {
		}
	}

private:
	std::vector<ServerSocket<Inet6>> in6_listens;
	std::vector<ServerSocket<Inet>> in_listens;
	gcm::logging::Logger &log;

	template <typename ServerAddress, typename ClientAddress = ServerAddress>
	struct AcceptHandler {
		using HandlerType = std::function<void(ConnectedSocket<ClientAddress> &&)>;

		ServerSocket<ServerAddress> &s;
		HandlerType handler;

		AcceptHandler(ServerSocket<ServerAddress> &s, HandlerType handler): s(s), handler(handler) {}
		AcceptHandler(AcceptHandler &&other) = default;
		AcceptHandler(const AcceptHandler &other) = default;

		void operator()() {
			ConnectedSocket<ClientAddress> client{std::move(s.template accept<ClientAddress>())};
			handler(std::move(client));
		}
	};
};

} // namespace gcm::socket
} // namespace gcm
