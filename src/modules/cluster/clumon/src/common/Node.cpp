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

#include "Cluster.h"

using namespace std;
using namespace ClusterMonitoring;

Node::Node(	const String& name,
			const String& clustername,
			unsigned int votes,
			bool online,
			bool clustered,
			const String& uptime) :
	_name(name),
	_clustername(clustername),
	_votes(votes),
	_online(online),
	_clustered(clustered),
	_uptime(uptime)
{}

Node::~Node(void)
{}


String
Node::name() const
{
	return _name;
}

String
Node::clustername() const
{
	return _clustername;
}

unsigned int
Node::votes() const
{
	return _votes;
}

bool
Node::online() const
{
	return _online;
}

bool
Node::clustered() const
{
	return _clustered;
}

String
Node::uptime() const
{
	return _uptime;
}


counting_auto_ptr<Service>
Node::addService(	const String& name,
					bool failed,
					bool autostart,
					const String& time_since_transition)
{
	counting_auto_ptr<Service> service(new Service(name, _clustername, *this, failed, autostart, time_since_transition));
	_services.insert(pair<String, counting_auto_ptr<Service> >(name, service));
	return service;
}

list<counting_auto_ptr<Service> >
Node::services()
{
	list<counting_auto_ptr<Service> > ret;

	for (map<String, counting_auto_ptr<Service> >::iterator
			iter = _services.begin() ;
			iter != _services.end() ;
			iter++)
	{
		ret.push_back(iter->second);
	}
	return ret;
}
