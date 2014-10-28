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

#include <exception>
#include <string>
#include <functional>
#include <vector>
#include <type_traits>
#include <sstream>

#include <cerrno>
#include <cstring>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// TODO: What about other systems?
#include <linux/un.h>

// FOR DEBUG:
#include <iostream>

namespace gcm {
namespace socket {

class SocketException: public std::exception {
public:
	SocketException(int &num) {
		std::stringstream ss;
		ss << ::strerror(num) << " (errno: " << num << ")";
		msg = ss.str();
	}
	SocketException(const std::string &msg): msg(msg) {}
	SocketException(const SocketException &other) = default;
	SocketException(SocketException &&other) = default;

	const char *what() const noexcept {
		return msg.c_str();
	}

private:
	std::string msg;
};

class Interrupt: public SocketException {
public:
	Interrupt(int &num): SocketException(num) {}
	Interrupt(const std::string &msg): SocketException(msg) {}
	Interrupt(const Interrupt &other) = default;
	Interrupt(Interrupt &&other) = default;
};

// Forward
template<typename Family>
class Socket;

template<typename Address>
class ConnectedSocket;

template<typename Address>
class ClientSocket;

template<typename Address>
class ServerSocket;

/**
 * Generic class holding address family specific members.
 * It is used to specify socket type.
 */
template <int Family, typename Addr>
class AddrFamily {
public:
	template<typename T>
	friend class Socket;

	template<typename T>
	friend class ServerSocket;

	static constexpr int family = Family;

protected:
	Addr addr;
};

/**
 * Inet class means IPv4.
 */
class Inet: public AddrFamily<AF_INET, sockaddr_in> {
public:
	Inet(): AddrFamily<AF_INET, sockaddr_in>() {
		addr.sin_family = Inet::family;
		addr.sin_port = htons(0);
		addr.sin_addr.s_addr = INADDR_ANY;
	}

	Inet(const Inet &other) = default;
	Inet(Inet &&other) = default;

	Inet(const std::string &ip, in_port_t port): AddrFamily<AF_INET, sockaddr_in>() {
		addr.sin_family = Inet::family;
		set_ip(ip);
		set_port(port);
	}

	Inet(const in_addr &ip, in_port_t port): AddrFamily<AF_INET, sockaddr_in>() {
		addr.sin_family = Inet::family;
		set_ip(ip);
		set_port(port);
	}

	Inet &operator=(const Inet &other) = default;

	void set_port(int port) {
		addr.sin_port = htons(port);
	}

	int get_port() const {
		return ntohs(addr.sin_port);
	}

	void set_ip(const in_addr &ip) {
		addr.sin_addr = ip;
	}

	void set_ip(const std::string &host) {
		int result = ::inet_pton(Inet::family, host.c_str(), &addr.sin_addr.s_addr);
		if (result == 0) {
			throw SocketException("Invalid address supplied.");
		} else if (result < 0) {
			throw SocketException(errno);
		}
	}

	const std::string get_ip() const {
		char dst[INET_ADDRSTRLEN];
		if (::inet_ntop(Inet::family, &(addr.sin_addr), dst, INET_ADDRSTRLEN) == NULL) {
			throw SocketException(errno);
		}

		return dst;
	}
};

/**
 * Inet6 class means IPv6.
 */
class Inet6: public AddrFamily<AF_INET6, sockaddr_in6> {
public:
	static constexpr int DefaultFlowInfo = 0;
	static constexpr int DefaultScopeId = 0;

	Inet6(int flowInfo = DefaultFlowInfo, int scopeId = DefaultScopeId): AddrFamily<AF_INET6, sockaddr_in6>() {
		addr.sin6_family = Inet6::family;
		addr.sin6_addr = IN6ADDR_ANY_INIT;
		addr.sin6_flowinfo = flowInfo;
		addr.sin6_scope_id = scopeId;
	}

	Inet6(const Inet6 &other): AddrFamily<AF_INET6, sockaddr_in6>(other) {
		// TODO: Do we need to copy addr.sin6_addr.s6_addr here?
		// Where is it stored?
	}

	Inet6(Inet6 &&other): AddrFamily<AF_INET6, sockaddr_in6>(other) {
		other.addr.sin6_addr = IN6ADDR_ANY_INIT;
	}

	Inet6(const std::string &ip, in_port_t port): AddrFamily<AF_INET6, sockaddr_in6>() {
		addr.sin6_family = Inet6::family;
		addr.sin6_flowinfo = DefaultFlowInfo;
		addr.sin6_scope_id = DefaultScopeId;

		set_ip(ip);
		set_port(port);
	}

	Inet6(const in6_addr &ip, in_port_t port): AddrFamily<AF_INET6, sockaddr_in6>() {
		addr.sin6_family = Inet6::family;
		addr.sin6_flowinfo = DefaultFlowInfo;
		addr.sin6_scope_id = DefaultScopeId;

		set_ip(ip);
		set_port(port);
	}

	Inet6 &operator=(const Inet6 &other) = default;

	void set_port(in_port_t port) {
		addr.sin6_port = htons(port);
	}

	in_port_t get_port() const {
		return ntohs(addr.sin6_port);
	}

	void set_ip(const in6_addr &ip) {
		addr.sin6_addr = ip;
	}

	void set_ip(const std::string &host) {
		switch (::inet_pton(Inet6::family, host.c_str(), &addr.sin6_addr.s6_addr)) {
			case 0: throw SocketException("Invalid address supplied.");
			case -1: throw SocketException(errno);
		}
	}

	const std::string get_ip() const {
		char dst[INET6_ADDRSTRLEN];
		if (::inet_ntop(Inet6::family, &(addr.sin6_addr), dst, INET6_ADDRSTRLEN) == NULL) {
			throw SocketException(errno);
		}

		return dst;
	}
};

struct AnyIpStorage {
	std::string address;
	in_port_t port;
};

class AnyIpAddress: public AddrFamily<0, AnyIpStorage> {
public:
	enum class Type {
		IPv4, IPv6
	};

	AnyIpAddress(Inet &&other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv4} {
		addr.address = other.get_ip();
		addr.port = other.get_port();
	}

	AnyIpAddress(const Inet &other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv4} {
		addr.address = other.get_ip();
		addr.port = other.get_port();
	}

	AnyIpAddress(Inet6 &&other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv6} {
		addr.address = other.get_ip();
		addr.port = other.get_port();
	}

	AnyIpAddress(const Inet6 &other): AddrFamily<0, AnyIpStorage>(), current_type{Type::IPv6} {
		addr.address = other.get_ip();
		addr.port = other.get_port();
	}

	AnyIpAddress(const AnyIpAddress &other) = default;
	AnyIpAddress(AnyIpAddress &&other) = default;

	operator Inet() {
		if (current_type == Type::IPv4) {
			return std::move(Inet(addr.address, addr.port));
		} else {
			throw SocketException("Trying to convert IPv6 to IPv4.");
		}
	}

	operator Inet6() {
		if (current_type == Type::IPv6) {
			return std::move(Inet6(addr.address, addr.port));
		} else {
			throw SocketException("Trying to convert IPv4 to IPv6.");
		}
	}

	Type get_type() const { return current_type; }
	const std::string &get_ip() const { return addr.address; }
	in_port_t get_port() const { return addr.port; }
	int get_family() const {
		switch (current_type) {
			case Type::IPv4: return AF_INET;
			case Type::IPv6: return AF_INET6;
		}
	}

protected:
	Type current_type;
};

/**
 * Unix socket
 */
class Unix: AddrFamily<AF_UNIX, sockaddr_un> {
public:
	Unix(): AddrFamily<AF_UNIX, sockaddr_un>() {
		addr.sun_family = AF_UNIX;
		::memset(addr.sun_path, 0, UNIX_PATH_MAX);
	}

	Unix(const Unix &other): AddrFamily<AF_UNIX, sockaddr_un>(other) {
		::memcpy(addr.sun_path, other.addr.sun_path, UNIX_PATH_MAX);
	}

	Unix(Unix &&other) = default;

	void set_path(const std::string &path) {
		size_t len = path.size() + 1;
		if (len > UNIX_PATH_MAX) {
			len = UNIX_PATH_MAX;
		}

		::memcpy(addr.sun_path, path.c_str(), len);

		// Prevent buffer overflow
		if (len == UNIX_PATH_MAX) {
			addr.sun_path[UNIX_PATH_MAX - 1] = '\0';
		}
	}

	const std::string get_path() const {
		return addr.sun_path;
	}
};

/**
 * Socket type
 */
enum class Type {
	Stream = SOCK_STREAM, // TCP in case of Inet family
	Datagram = SOCK_DGRAM, // UDP in case of Inet family
	Raw = SOCK_RAW // RAW socket. Usually requires root privileges to open this type of socket.
};

/**
 * Generic socket class
 */
template<typename Address>
class Socket {
public:
	friend class Select;

	static constexpr int DefaultListenBacklog = 50;
	typedef Address Family;

	Socket(Type type): fd(::socket(Address::family, static_cast<int>(type), 0)), bind_address() {
		if (fd <= 0) {
			throw SocketException(errno);
		}
	}

	Socket(Socket<Address> &&other): fd(other.fd), bind_address(std::move(other.bind_address)) {
		other.fd = 0;
	}

	~Socket() {
		if (fd > 0) {
			close(fd);
		}
	}

	const Address &get_bind_address() {
		return bind_address;
	}

	void bind(Address bind_addr) {
		if (::bind(fd, reinterpret_cast<const sockaddr *>(&bind_addr.addr), sizeof(bind_addr.addr)) < 0) {
			throw SocketException(errno);
		}
		bind_address = bind_addr;
	}

protected:
	int fd;
	Address bind_address;

	Socket(int fd, Address &&addr): fd(fd), bind_address(addr)
	{}

	Socket(int fd, const Address &addr): fd(fd), bind_address(addr)
	{}

	Socket(int fd): fd(fd)
	{}
};

/**
 * Server that can accept connections.
 */
template<typename Address>
class ServerSocket: public Socket<Address> {
public:
	static constexpr int DefaultListenBacklog = 50;

	ServerSocket(Type type): Socket<Address>(type) {}
	ServerSocket(const ServerSocket &other) = default;
	ServerSocket(ServerSocket &&other) = default;

	void listen(int backlog = DefaultListenBacklog) {
		if (::listen(this->fd, backlog) < 0) {
			throw SocketException(errno);
		}
	}

	template<typename T>
	ConnectedSocket<T> accept() {
		Address client;
		socklen_t addrlen = sizeof(client.addr);

		int newfd = ::accept(this->fd, reinterpret_cast<sockaddr *>(&(client.addr)), &addrlen);
		if (newfd <= 0) {
			throw SocketException(errno);
		}

		return ConnectedSocket<T>(*this, newfd, std::move(client));
	}
};

template<typename T, std::size_t size>
constexpr std::size_t ArraySize(T (&)[size]) {
	return size;
}

template<typename Address>
class WritableSocket: public Socket<Address> {
public:
	WritableSocket(WritableSocket &&other) = default;

	bool eof() { return eof_flag; }

	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value, WritableSocket &>::type
	operator <<(const T &val) {
		ssize_t written = ::write(this->fd, dynamic_cast<void *>(&val), sizeof(val));

		if (written < 0) {
			throw SocketException(errno);
		}

		return *this;
	}

	template<typename T>
	WritableSocket &operator <<(const std::basic_string<T> &val) {
		using s = std::basic_string<T>;
		ssize_t written = ::write(this->fd, val.c_str(), val.size() * sizeof(typename s::value_type));

		if (written < 0) {
			throw SocketException(errno);
		}

		return *this;
	}

	// Read from socket, for arithmetic types
	template<typename T>
	typename std::enable_if<std::is_arithmetic<T>::value, WritableSocket &>::type
	operator >>(T &val) {
		ssize_t readed = ::read(this->fd, dynamic_cast<void *>(&val), sizeof(val));
		if (readed < 0) {
			throw SocketException(errno);
		}

		if (readed == 0) {
			eof_flag = true;
		}

		return *this;
	}

	// Read from socket for arrays
	template<typename T>
	typename std::enable_if<std::is_array<T>::value, WritableSocket &>::type
	operator >>(T val) {
		ssize_t readed = ::read(this->fd, dynamic_cast<void *>(val), ArraySize(val));

		if (readed < 0) {
			throw SocketException(errno);
		}

		if (readed == 0) {
			eof_flag = true;
		}

		return *this;
	}

	// Read from socket for pointers
	/*template<typename T>
	std::enable_if<std::is_pointer<T>::value, size_t>::type
	read(T val, size_t length) {
		ssize_t readed = ::read(this->fd, dynamic_cast<void *>(val), length);
		if (readed < 0) {
			throw SocketError(errno);
		}

		return readed;
	}*/

	// Read from socket for strings.
	template<typename T>
	WritableSocket &operator >>(std::basic_string<T> &val) {
		using s = std::basic_string<T>;
		auto capacity = val.capacity();
		if (capacity <= 0) {
			capacity = 1;
		}

		std::vector<typename s::value_type> dt;
		dt.reserve(capacity);

		// Need to convert capacity to bytes and read specified number of bytes.
		ssize_t received_size = ::read(this->fd, &dt[0], capacity * sizeof(typename s::value_type));
		if (received_size < 0) {
			throw SocketException(errno);
		}

		if (received_size == 0) {
			eof_flag = true;
		}

		val.assign(&dt[0], received_size / sizeof(typename s::value_type));

		return *this;
	}

protected:
	WritableSocket(int fd, Address &&addr): Socket<Address>(fd, addr), eof_flag(false) {}
	WritableSocket(int fd, const Address &addr): Socket<Address>(fd, addr), eof_flag(false) {}

private:
	bool eof_flag;
};

/**
 * Client's connection to server.
 */
template<typename Address>
class ConnectedSocket: public WritableSocket<Address> {
public:
	template<typename T>
	friend class ServerSocket;

	const Address &get_client_address() const {
		return client_address;
	}

protected:
	template<typename ServerAddress>
	ConnectedSocket(ServerSocket<ServerAddress> &server, int fd, Address &&addr):
		WritableSocket<Address>(fd, std::move(Address(server.get_bind_address()))),
		client_address(addr)
	{}

protected:
	Address client_address;
};

/**
 * Connection to server.
 */
template<typename Address>
class ClientSocket: public WritableSocket<Address> {
public:
	ClientSocket(const std::string &ip, in_port_t port): WritableSocket<Address>(Type::Stream)
	{
		connect(ip, port);
	}

	ClientSocket(ClientSocket &&other) = default;

	const Address &get_server_address() const {
		return server_address;
	}

	void connect(Address address, in_port_t port) {
		if (::connect(this->fd, &server_address.addr, sizeof(server_address.addr)) < 0) {
			throw SocketException(errno);
		}

		server_address = address;
	}

protected:
	Address server_address;
};

class Select {
public:
	using Callback = std::function<void()>;

	template<typename Address>
	void add(const Socket<Address> &socket, Callback rd, Callback wr, Callback ex) {
		calls.emplace_back(socket.fd, rd, wr, ex);
	}

	void select(long timeout = 0, long usec_timeout = 0) {
		fd_set rds;
		fd_set wrs;
		fd_set exs;

		FD_ZERO(&rds);
		FD_ZERO(&wrs);
		FD_ZERO(&exs);

		int highest = 0;
		for (auto call: calls) {
			if (highest < call.fd) {
				highest = call.fd;
			}

			FD_SET(call.fd, &rds);
			FD_SET(call.fd, &wrs);
			FD_SET(call.fd, &exs);
		}

		timeval tm;
		tm.tv_sec = timeout;
		tm.tv_usec = usec_timeout;

		if (::select(highest + 1, &rds, &wrs, &exs, &tm) < 0) {
			switch (errno) {
				case EINTR:	throw Interrupt(errno);
				default: throw SocketException(errno);
			}
		}

		for (auto call: calls) {
			if (FD_ISSET(call.fd, &rds)) {
				call.rd();
			}

			if (FD_ISSET(call.fd, &wrs)) {
				call.wr();
			}

			if (FD_ISSET(call.fd, &exs)) {
				call.ex();
			}
		}
	}

private:
	struct SocketCall {
		const int fd;
		Callback rd;
		Callback wr;
		Callback ex;

		SocketCall(const int fd, Callback rd, Callback wr, Callback ex): fd(fd), rd(rd), wr(wr), ex(ex) {}
	};

	std::vector<SocketCall> calls;
};

} // namespace gcm::socket
} // namespace gcm
