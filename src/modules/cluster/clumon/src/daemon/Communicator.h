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

#ifndef __CONGA_MODCLUSTERD_COMMUNICATOR_H
#define __CONGA_MODCLUSTERD_COMMUNICATOR_H

#include "Thread.h"
#include "Mutex.h"
#include "Socket.h"
#include "Peer.h"

#include "String.h"
#include <vector>
#include <map>

namespace ClusterMonitoring
{


class CommDP
{
	public:
		CommDP();
		virtual ~CommDP();

		virtual void msg_arrived(const String& host, const String& msg) = 0;
};


class Communicator : public Thread
{
	public:
		Communicator(unsigned short port, CommDP& delivery_point);
		virtual ~Communicator();

		void send(const String& msg);
		void update_peers(const String& self, const std::vector<String>& peers);

	private:
		unsigned short _port;
		ServerSocket _serv_sock;

		CommDP& _delivery_point;

		std::map<String, Peer> _peers;

		Mutex _mutex;
		String _my_hostname;
		std::vector<String> _out_q;
		std::vector<String> _peer_hostnames;

		void serve_sockets(std::vector<String>&hostnames);
		bool time_to_connect();
		unsigned int _connect_time;
		unsigned int _rand_state;

		void run();

		Communicator(const Communicator&);
		Communicator& operator= (const Communicator&);
};

};

#endif
