/*
** Copyright (C) Red Hat, Inc. 2005-2009
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License version 2 as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING. If not, write to the
** Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
** MA 02139, USA.
*/

/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */

#include "Socket.h"
#include "Logger.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include "String.h"


ServerSocket::ServerSocket(const String& sock_path) :
	Socket(-1),
	_unix_sock(true),
	_sock_path(sock_path),
	sa_addr(NULL),
	sa_len(0)
{
	_sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (_sock == -1) {
		throw String("ServerSocket(sock_path=") + sock_path
				+ "): socket() failed: " + String(strerror(errno));
	}

	int t = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t))) {
		throw String("ServerSocket(sock_path=") + sock_path
				+ "): set SO_REUSEADDR, failed: " + String(strerror(errno));
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, sock_path.c_str(), sock_path.size() + 1);

	unlink(_sock_path.c_str());

	if (bind(_sock, (struct sockaddr*) &addr, sizeof(addr))) {
		throw String("ServerSocket(sock_path=") + sock_path
				+ "): bind() failed: " + String(strerror(errno));
	}

	if (listen(_sock, 5)) {
		throw String("ServerSocket(sock_path=") + sock_path
				+ "): listen() failed: " + String(strerror(errno));
	}
	//String msg = String("created unix server socket, ")
	//				+ _sock + ", " + sock_path;
	//log(msg, LogSocket);
}

ServerSocket::ServerSocket(unsigned short port) :
	Socket(-1),
	_unix_sock(false),
	_sock_path("")
{
	_sock = socket(PF_INET6, SOCK_STREAM, 0);
	if (_sock == -1) {
		throw String("ServerSocket(port=") + port
				+ "): socket() failed: " + String(strerror(errno));
	}

	int t = 1;
	if (setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t))) {
		throw String("ServerSocket(port=") + port
				+ "): set SO_REUSEADDR, failed: " + String(strerror(errno));
	}

	sa_family = AF_INET6;
	sa_len = sizeof(struct sockaddr_in6);
	sa_addr = (struct sockaddr *) calloc(1, sa_len);
	if (sa_addr == NULL)
		throw String("ServerSocket(port=") + port + "): OOM";
	((struct sockaddr_in6 *) sa_addr)->sin6_family = AF_INET6;
	((struct sockaddr_in6 *) sa_addr)->sin6_port = htons(port);
	((struct sockaddr_in6 *) sa_addr)->sin6_addr = in6addr_any;

	if (bind(_sock, sa_addr, sa_len)) {
		throw String("ServerSocket(port=") + port
				+ "): bind() failed: " + String(strerror(errno));
	}

	if (listen(_sock, 5)) {
		throw String("ServerSocket(port=") + port
				+ "): listen() failed: " + String(strerror(errno));
	}

	//String msg = String("created tcp server socket, ")
	//				+ _sock + ", port " + port;
	//log(msg, LogSocket);
}

ServerSocket::ServerSocket(const ServerSocket& s) :
	Socket(s),
	_unix_sock(s._unix_sock),
	_sock_path(s._sock_path),
	sa_addr(s.sa_addr),
	sa_family(s.sa_family),
	sa_len(s.sa_len)
{}

ServerSocket&
ServerSocket::operator= (const ServerSocket& s)
{
	if (&s != this) {
		this->Socket::operator= (s);
		_unix_sock = s._unix_sock;
		_sock_path = s._sock_path;
		sa_family = s.sa_family;
		sa_len = s.sa_len;
		if (s.sa_len > 0) {
			sa_addr = (struct sockaddr *) malloc(s.sa_len);
			if (sa_addr == NULL)
				throw String("Out of memory");
			memcpy(sa_addr, s.sa_addr, s.sa_len);
		}
	}

	return *this;
}

ServerSocket::~ServerSocket()
{
	if (*_counter == 1) {
		if (_unix_sock)
			unlink(_sock_path.c_str());
		free(sa_addr);
	}
}

ClientSocket ServerSocket::accept() {
	while (true) {
		struct sockaddr_storage ss;
		socklen_t size = sizeof(ss);

		int ret = ::accept(_sock, (struct sockaddr *) &ss, &size);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			throw String("ServerSocket(): accept() failed: ")
					+ String(strerror(errno));
		}
		//log("ServerSocket: accepted connection", LogSocket);
		return ClientSocket(ret, (struct sockaddr *) &ss, size);
	}
}

bool
ServerSocket::ready(int timeout)
{
	bool read = true;
	bool write = false;

	poll(read, write, timeout);
	return read;
}
