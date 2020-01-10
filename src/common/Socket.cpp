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
#include "Time.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>

#include <sys/poll.h>

typedef struct pollfd poll_fd;

Socket::Socket(int sock) :
	_sock(sock)
{
	try {
		_counter = counting_auto_ptr<int>(new int(1));
	} catch ( ... ) {
		close();
		throw String("Socket(int sock) failed");
	}
}

Socket::Socket(const Socket& s) :
	_sock(s._sock),
	_counter(s._counter)
{
	(*_counter)++;
}

Socket&
Socket::operator= (const Socket& s)
{
	if (&s != this) {
		decrease_counter();
		_sock = s._sock;
		_counter = s._counter;
		(*_counter)++;
	}

	return *this;
}

Socket::~Socket()
{
	decrease_counter();
}

void
Socket::decrease_counter()
{
	if (--(*_counter) == 0)
		close();
}

void
Socket::close()
{
	if (_sock != -1) {
		log(String("closing socket ") + _sock, LogSocket);
		shutdown(_sock, SHUT_RDWR);

		int e;
		do {
			e = ::close(_sock);
		} while (e && (errno == EINTR));
	}
	_sock = -1;
}

bool
Socket::operator== (const Socket& obj)
{
	return obj._sock == _sock;
}

int
Socket::get_sock()
{
	return _sock;
}

bool
Socket::nonblocking()
{
	if (!valid())
		throw String("socket not valid");

	int flags = fcntl(_sock, F_GETFL);
	if (flags == -1)
		throw String("fcntl(F_GETFL): " + String(strerror(errno)));
	return (flags & O_NONBLOCK) != 0;
}

bool
Socket::nonblocking(bool mode)
{
	if (!valid())
		throw String("socket not valid");

	int old_flags = fcntl(_sock, F_GETFL);
	if (old_flags == -1)
		throw String("fcntl(F_GETFL): " + String(strerror(errno)));

	int new_flags;
	if (mode)
		new_flags = old_flags | O_NONBLOCK;
	else
		new_flags = old_flags & ~O_NONBLOCK;

	if (fcntl(_sock, F_SETFL, new_flags))
		throw String("fcntl(F_SETFL): " + String(strerror(errno)));
	return (old_flags & O_NONBLOCK) != 0;
}

void
Socket::poll(bool& read, bool& write, int timeout)
{
	if (!valid())
		throw String("socket not valid");

	poll_fd poll_data;
	poll_data.fd = _sock;
	poll_data.events = (read ? POLLIN : 0) | (write ? POLLOUT : 0);

	read = write = false;
	int beg = time_mil();

	while (true) {
		int time2wait;
		if (timeout <= 0)
			time2wait = timeout;
		else {
			time2wait = beg + timeout - time_mil();
			if (time2wait < 0)
				return;
		}
		poll_data.revents = 0;

		int ret = ::poll(&poll_data, 1, time2wait);
		if (ret == 0)
			return;
		else if (ret == -1) {
			if (errno == EINTR)
				continue;
			else
				throw String("poll() error: " + String(strerror(errno)));
		} else {
			if (poll_data.revents & POLLIN)
				read = true;
			if (poll_data.revents & POLLOUT)
				write = true;
			if (poll_data.revents & (POLLERR | POLLHUP | POLLNVAL))
				read = write = true;
			return;
		}
	}
}
