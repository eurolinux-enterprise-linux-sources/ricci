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

#ifndef __CONGA_MODCLUSTERD_PEER_H
#define __CONGA_MODCLUSTERD_PEER_H

#include "counting_auto_ptr.h"
#include "Socket.h"

#include <vector>
#include "String.h"

namespace ClusterMonitoring
{


class Peer
{
	public:
		Peer();
		Peer(const String& hostname, const ClientSocket&);
		Peer(const String& hostname, unsigned short port);
		virtual ~Peer();

		void send();
		std::vector<String> receive();

		bool outq_empty() { return _out->empty(); }

		void append(const String& msg);

		int get_sock_fd();

		String hostname();

		bool operator== (const Peer&) const;

	private:
		counting_auto_ptr<ClientSocket> _sock;
		const String _hostname;

		counting_auto_ptr<String> _in;
		counting_auto_ptr<String> _out;
};


};

#endif
