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
	TcpServer(): quit(false), log(gcm::logging::getLogger("TcpServer")) {}
	TcpServer(const TcpServer &) = default;
	TcpServer(TcpServer &&other) = default;

	void listen(Inet6 &&address) {
		in6_listens.emplace_back(Type::Stream);
		auto &socket = in6_listens.back();
		socket.setopt(SO_REUSEADDR, 1);
		socket.bind(address);
		socket.listen();

		INFO(log) << "Listening on [" << address.get_ip() << "]:" << address.get_port();
	}

	void listen(Inet &&address) {
		in_listens.emplace_back(Type::Stream);
		auto &socket = in_listens.back();
		socket.setopt(SO_REUSEADDR, 1);
		socket.bind(address);
		socket.listen();
		
		INFO(log) << "Listening on " << address.get_ip() << ":" << address.get_port();
	}

	template<typename T, typename T6>
	void serve_forever(T &&handle, T &&handle_v6) {
		try {
			while (!quit) {
				Select select;
				for (ServerSocket<Inet6> &s: in6_listens) {
					select.add(s, AcceptHandler<std::decay_t<T>, Inet6>{s, std::forward<T6>(handle_v6)}, nullptr, nullptr);
				}

				for (ServerSocket<Inet> &s: in_listens) {
					select.add(s, AcceptHandler<std::decay_t<T>, Inet>{s, std::forward<T>(handle)}, nullptr, nullptr);
				}

				select.select();
			}
		} catch (Interrupt e) {
		}
	}

	template<typename T>
	void serve_forever(T &&handle) {
		try {
			while (!quit) {
				Select select;
				for (ServerSocket<Inet6> &s: in6_listens) {
					select.add(s, AcceptHandler<std::decay_t<T>, Inet6, AnyIpAddress>{s, std::forward<T>(handle)}, nullptr, nullptr);
				}

				for (ServerSocket<Inet> &s: in_listens) {
					select.add(s, AcceptHandler<std::decay_t<T>, Inet, AnyIpAddress>{s, std::forward<T>(handle)}, nullptr, nullptr);
				}

				// Each 0.5s, try if there is quit event.
				select.select(0, 500000);
			}
		} catch (Interrupt e) {
		}
	}

	void stop() {
		quit = true;
	}

private:
	bool quit;
	std::vector<ServerSocket<Inet6>> in6_listens;
	std::vector<ServerSocket<Inet>> in_listens;
	gcm::logging::Logger &log;

	template <typename HandlerType, typename ServerAddress, typename ClientAddress = ServerAddress>
	struct AcceptHandler {
		ServerSocket<ServerAddress> &s;
		HandlerType &handler;

		template<typename H>
		AcceptHandler(ServerSocket<ServerAddress> &s, H &&handler):
			s(s), handler(std::forward<H>(handler)) {}

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
