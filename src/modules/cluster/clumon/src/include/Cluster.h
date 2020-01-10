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

#ifndef __CONGA_MODCLUSTERD_CLUSTER_H
#define __CONGA_MODCLUSTERD_CLUSTER_H

#include <counting_auto_ptr.h>
#include "String.h"
#include <map>
#include <list>

#include "XML.h"

namespace ClusterMonitoring
{

class Node;
class Service;
class Cluster;

String cluster2xml(Cluster& cluster);
counting_auto_ptr<Cluster> xml2cluster(const String& xml);


class Cluster
{
	public:
		Cluster(const String& name,
				const String& alias,
				const String& cluster_version,
				unsigned int minQuorum=0);
		virtual ~Cluster();

		String name();
		String alias();
		String version();
		unsigned int votes();
		unsigned int minQuorum();
		bool quorate();

		counting_auto_ptr<Node> addNode(const String& name,
										unsigned int votes,
										bool online,
										bool clustered,
										const String& uptime);

		counting_auto_ptr<Service> addService(	const String& name,
										const String& nodeName,
										bool failed,
										bool autostart,
										const String& time_since_transition);

		std::list<counting_auto_ptr<Node> > nodes();
		std::list<counting_auto_ptr<Node> > clusteredNodes();
		std::list<counting_auto_ptr<Node> > unclusteredNodes();

		std::list<counting_auto_ptr<Service> > services();
		std::list<counting_auto_ptr<Service> > runningServices();
		std::list<counting_auto_ptr<Service> > stoppedServices();
		std::list<counting_auto_ptr<Service> > failedServices();

	private:
		String _name;
		String _alias;
		String _cl_version;
		unsigned int _minQuorum;
		std::map<String, counting_auto_ptr<Node> > _nodes;
};

class Node
{
	public:
		Node(	const String& name,
				const String& clustername,
				unsigned int votes,
				bool online,
				bool clustered,
				const String& uptime);
		virtual ~Node();

		String name() const;
		String clustername() const;
		unsigned int votes() const;
		bool online() const;
		bool clustered() const; // available to cluster
		String uptime() const;

		counting_auto_ptr<Service> addService(const String& name,
										bool failed,
										bool autostart,
										const String& time_since_transition);
		std::list<counting_auto_ptr<Service> > services();

	private:
		String _name;
		String _clustername;
		unsigned int _votes;
		bool _online;
		bool _clustered; // available to cluster
		String _uptime;

		std::map<String, counting_auto_ptr<Service> > _services;
};

class Service
{
	public:
		Service(const String& name,
				const String& clustername,
				const Node& node,
				bool failed,
				bool autostart,
				const String& time_since_transition);
		virtual ~Service();

		String name() const;
		String clustername() const;
		bool running() const;
		String nodename() const;
		bool failed() const;
		bool autostart() const;
		String time_since_transition() const;

	private:
		String _name;
		String _clustername;
		String _nodename;
		bool _autostart;
		bool _failed;
		String _time_since_transition;
};

};

#endif
