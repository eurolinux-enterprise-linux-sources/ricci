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
#include "Network.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <String.h>
#include <netdb.h>

extern "C" {
	#include "sys_util.h"
}


ClientSocket::ClientSocket() :
	Socket(-1),
	sa_addr(NULL),
	sa_len(0)
{}

ClientSocket::ClientSocket(int sock, struct sockaddr *addr, size_t len) :
	Socket(sock)
{
	sa_len = len;
	if (len > 0) {
		sa_addr = (struct sockaddr *) malloc(len);
		if (sa_addr == NULL)
			throw String("Out of memory");
		memcpy(sa_addr, addr, len);
	}
}

ClientSocket::ClientSocket(const String& sock_path) :
	Socket(-1),
	sa_addr(NULL),
	sa_len(0)
{
	_sock = socket(PF_UNIX, SOCK_STREAM, 0);
	if (_sock == -1) {
		throw String("ClientSocket(String): socket() failed: ")
				+ String(strerror(errno));
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	if (sock_path.size() >= sizeof(addr.sun_path))
		throw String("path to client unix socket is too long");
	memcpy(addr.sun_path, sock_path.c_str(), sock_path.size()+1);

	if (connect(_sock, (struct sockaddr*) &addr, sizeof(addr))) {
		throw String("ClientSocket(String): connect() failed: ")
				+ String(strerror(errno));
	}

	//String msg = String("created client socket ") + _sock;
	//msg += ", and connected to " + sock_path;
	//log(msg, LogSocket);
}

ClientSocket::ClientSocket(	const String& hostname,
							unsigned short port,
							unsigned int timeout_ms) :
	Socket(-1)
{
	struct addrinfo *cur;
	struct addrinfo *addr_list = Network::resolve_host(hostname.c_str());

	for (cur = addr_list ; cur != NULL ; cur = cur->ai_next) {
		_sock = socket(cur->ai_socktype, SOCK_STREAM, 0);
		if (_sock == -1)
			continue;

		if (timeout_ms)
			nonblocking(true);
			
		if (connect(_sock, cur->ai_addr, cur->ai_addrlen) == 0) {
			// connected
			nonblocking(false);

			sa_family = cur->ai_family;
			sa_len = cur->ai_addrlen;
			sa_addr = (struct sockaddr *) malloc(sa_len);
			if (sa_addr != NULL)
				memcpy(sa_addr, cur->ai_addr, sa_len);
			else
				break;

			freeaddrinfo(addr_list);
			return;
		}

		// connect() error
		if (errno != EINPROGRESS) {
			::close(_sock);
			continue;
		}

		bool can_read = false, can_write = true;
		poll(can_read, can_write, timeout_ms);

		if (can_write == false) {
			// connect() not completed
			::close(_sock);
			throw String("ClientSocket(hostname, port, timeout): connect() timed out") + String(strerror(errno));
		}

		// connect() completed, check status
		int err = 1;
		socklen_t err_size = sizeof(err);
		getsockopt(_sock, SOL_SOCKET, SO_ERROR, &err, &err_size);

		if (err) {
			::close(_sock);
			continue;
		}

		// connected
		nonblocking(false);

		sa_family = cur->ai_family;
		sa_len = cur->ai_addrlen;

		sa_addr = (struct sockaddr *) malloc(sa_len);
		if (sa_addr != NULL)
			memcpy(sa_addr, cur->ai_addr, sa_len);
		else
			break;

		freeaddrinfo(addr_list);
		return;
	}

	if (addr_list != NULL)
		freeaddrinfo(addr_list);

	throw String("ClientSocket(hostname, port, timeout): connect() failed");
}

ClientSocket::ClientSocket(const ClientSocket& s) :
	Socket(s),
	sa_addr(s.sa_addr),
	sa_family(s.sa_family),
	sa_len(s.sa_len)
{}

ClientSocket&
ClientSocket::operator= (const ClientSocket& s)
{
	if (&s != this) {
		this->Socket::operator= (s);
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

ClientSocket::~ClientSocket() {
	if (*_counter == 1)
		free(sa_addr);
}

bool
ClientSocket::connected_to(const String& hostname)
{
	struct addrinfo *addr_list = NULL;

	try {
		struct addrinfo *cur;

		addr_list = Network::resolve_host(hostname.c_str());
		for (cur = addr_list ; cur != NULL ; cur = cur->ai_next) {
			if (cur->ai_addrlen == sa_len && cur->ai_family == sa_family) {
				if (!memcmp(cur->ai_addr, sa_addr, sa_len)) {
					freeaddrinfo(addr_list);
					return (true);
				}
			}
		}
	} catch ( ... ) {}

	freeaddrinfo(addr_list);
	return false;
}

String
ClientSocket::recv()
{
	if (_sock == -1)
		throw String("ClientSocket::recv(): socket already closed");

	char buffer[4096];
	int ret;

	ret = read_restart(_sock, buffer, sizeof(buffer));
	if (ret < 0) {
		if (ret == -EAGAIN)
			return "";
		throw String("ClientSocket::recv(): recv error: ")
				+ String(strerror(-ret));
	}

	if (ret == 0) {
		close();
		throw String("ClientSocket::recv(): socket has been shutdown");
	}

	//log(String("received ") + ret + " bytes from socket " + _sock,
	//LogLevel(LogSocket|LogTransfer));
	String data(buffer, ret);
	memset(buffer, 0, ret);
	return data;
}

String
ClientSocket::recv(int timeout)
{
	bool in = true, out = false;

	poll(in, out, timeout);
	if (in)
		return recv();
	else
		return "";
}

String
ClientSocket::send(const String& msg)
{
	if (_sock == -1)
		throw String("ClientSocket::send(): socket already closed");

	int ret = write_restart(_sock, msg.c_str(), msg.size());
	if (ret < 0) {
		if (ret == -EAGAIN || ret == -EWOULDBLOCK)
			return msg;
		throw String("ClientSocket::send(): socket error: ")
				+ String(strerror(-ret));
	}

	//log(String("sent ") + ret + " bytes thru socket " + _sock,
	//LogLevel(LogSocket|LogTransfer));
	return msg.substr(ret);
}

String
ClientSocket::send(const String& msg, int timeout)
{
	bool in = false, out = true;

	poll(in, out, timeout);
	if (out)
		return send(msg);
	else
		return msg;
}

void
ClientSocket::ready(bool& recv, bool& send, int timeout)
{
	poll(recv, send, timeout);
}
