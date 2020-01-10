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

#include "ClusterMonitor.h"
#include "Socket.h"
#include "Time.h"

#include <errno.h>
#include <sys/poll.h>

using namespace ClusterMonitoring;

ClusterMonitor::ClusterMonitor(const String& socket_path) :
	_sock_path(socket_path)
{}

ClusterMonitor::~ClusterMonitor()
{}

counting_auto_ptr<Cluster>
ClusterMonitor::get_cluster()
{
	try {
		ClientSocket sock(_sock_path);

		sock.nonblocking(true);

		if(sock.send("GET").size())
			throw int();

		String xml;
		unsigned int timeout = 1000;
		while (timeout > 0) {
			struct pollfd poll_data;
			poll_data.fd = sock.get_sock();
			poll_data.events = POLLIN;
			poll_data.revents = 0;

			unsigned int time_start = time_mil();

			int ret = poll(&poll_data, 1, timeout);
			int err = errno;
			timeout -= (time_mil() - time_start);
			if (ret == 0)
				continue;
			else if (ret == -1) {
				if (errno == EINTR)
					continue;
				else {
					throw String("get_cluster(): poll() error")
							+ String(strerror(err));
				}
			}

			if (poll_data.revents & POLLIN) {
				xml += sock.recv();
				if (xml.find("\n\n") != xml.npos)
					break;
				continue;
			}

			if (poll_data.revents & (POLLERR | POLLHUP | POLLNVAL))
				throw String("get_cluster(): socket error");
		}
		return xml2cluster(xml);
	} catch ( ... ) {
		return counting_auto_ptr<Cluster>();
	}
}
