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

#ifndef __CONGA_SOCKET_H
#define __CONGA_SOCKET_H

#include "counting_auto_ptr.h"

#include "String.h"
#include <vector>

// NOT THREAD SAFE
// provide external locking

class Socket
{
	public:
		Socket(const Socket&);
		virtual Socket& operator= (const Socket&);
		virtual ~Socket();

		virtual bool operator== (const Socket&);
		virtual bool server() = 0;

		int get_sock();
		bool valid() { return _sock != -1; }

		bool nonblocking(); // return whether O_NONBLOCK is set
		bool nonblocking(bool mode); // set O_NONBLOCK, return old state

	protected:
		Socket(int sock); // takes ownership of sock
		int _sock;
		void close();
		counting_auto_ptr<int> _counter;

		void poll(bool& read, bool& write, int timeout); // milliseconds

	private:
		void decrease_counter();
};

class ServerSocket;

class ClientSocket : public Socket
{
	public:
		ClientSocket();

		// UNIX domain socket
		ClientSocket(const String& sock_path);

		ClientSocket(	const String& hostname,
						unsigned short port,
						unsigned int timeout_ms=0);
		ClientSocket(const ClientSocket&);
		virtual ClientSocket& operator= (const ClientSocket&);
		virtual ~ClientSocket();

		virtual String recv();
		virtual String recv(int timeout);

		// return what is left to send
		virtual String send(const String& msg);

		virtual String send(const String& msg, int timeout);

		virtual void ready(bool& recv, bool& send, int timeout);
		virtual bool server() { return false; }
		virtual bool connected_to(const String& hostname);

	protected:
		struct sockaddr *sa_addr;
		int sa_family;
		size_t sa_len;

		// takes ownership of sock
		ClientSocket(int sock, struct sockaddr *addr=NULL, size_t len=0);

	friend class ServerSocket;
};

class ServerSocket : public Socket
{
	public:
		ServerSocket(const String& sock_path); // UNIX socket
		ServerSocket(unsigned short port); // TCP socket
		ServerSocket(const ServerSocket&);
		virtual ServerSocket& operator= (const ServerSocket&);
		virtual ~ServerSocket();

		ClientSocket accept();

		virtual bool ready(int timeout);

		virtual bool server() { return true; }

	private:
		bool _unix_sock;
		String _sock_path;
		struct sockaddr *sa_addr;
		int sa_family;
		size_t sa_len;
};

#endif
