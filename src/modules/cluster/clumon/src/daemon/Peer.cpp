/*
** Copyright (C) Red Hat, Inc. 2005-2008
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

#include "Peer.h"
#include "Logger.h"


using namespace ClusterMonitoring;
using namespace std;


Peer::Peer() :
	_sock(new ClientSocket()),
	_in(new String()),
	_out(new String())
{}

Peer::Peer(const String& hostname, const ClientSocket& sock) :
	_sock(new ClientSocket(sock)),
	_hostname(hostname),
	_in(new String()),
	_out(new String())
{
	_sock->nonblocking(true);
}

Peer::Peer(const String& hostname, unsigned short port) :
	_sock(new ClientSocket(hostname, port)),
	_hostname(hostname),
	_in(new String()),
	_out(new String())
{
	_sock->nonblocking(true);
}

Peer::~Peer()
{}

bool
Peer::operator== (const Peer& p) const
{
	return (_in == p._in &&
			_out == p._out &&
			_sock == p._sock &&
			_hostname == p._hostname);
}

void
Peer::send()
{
	if (_out->empty())
		return;

	log("sending data to " + _hostname, LogTransfer);
	String rest = _sock->send(*_out);
	*_out = rest;
}

vector<String>
Peer::receive()
{
	log("receiving data from " + _hostname, LogTransfer);

	String& in = *_in;
	in += _sock->recv();

	vector<String> ret;

	while (true) {
		String::size_type idx = in.find("\n\n");
		if (idx == in.npos)
			return ret;
		idx += 2;
		ret.push_back(in.substr(0, idx));
		in = in.substr(idx);
	}
}

void
Peer::append(const String& msg)
{
	_out->append(msg);
}

int
Peer::get_sock_fd()
{
	return _sock->get_sock();
}

String
Peer::hostname()
{
	return _hostname;
}
