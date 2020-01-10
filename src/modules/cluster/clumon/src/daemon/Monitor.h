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

#ifndef __CONGA_MODCLUSTERD_MONITOR_H
#define __CONGA_MODCLUSTERD_MONITOR_H

#include "String.h"
#include <map>

#include "XML.h"
#include "Thread.h"
#include "Communicator.h"
#include "Cluster.h"


namespace ClusterMonitoring
{

extern "C" {
#	include <sys/socket.h>
#	include <netdb.h>
#	include <arpa/inet.h>
#	include <net/if.h>
#	include <ifaddrs.h>
#	include <libcman.h>
}

class Monitor : public Thread, public CommDP
{
	public:
		Monitor(unsigned short port, const String& cluster_version);
		virtual ~Monitor();

		String request(const String&);

		virtual void msg_arrived(const String& host, const String& msg);

	protected:
		virtual void run();

	private:
		Mutex _mutex; // _cluster and _cache
		counting_auto_ptr<Cluster> _cluster;
		std::map<String, std::pair<unsigned int, XMLObject> > _cache;
		Communicator _comm;

		/*
		** cluster versions:
		**	RHEL3			= "3"
		**	RHEL4, FC4, FC5	= "4"
		**	RHEL5, FC6-F10	= "5"
		**	F11-			= "6"
		*/
		const String _cl_version;
		void update_now();

		// return (nodenames - my_nodename)
		std::vector<String> get_local_info(	String& nodename,
											String& clustername,
											String& msg);
		counting_auto_ptr<Cluster> merge_data(const String& clustername);
		XMLObject parse_cluster_conf();
		String probe_quorum();
		String nodename(const std::vector<String>& nodenames);
		std::vector<String> clustered_nodes();
		std::vector<XMLObject> services_info();

		String uptime() const;
};


};

#endif
