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

using namespace ClusterMonitoring;
using namespace std;

Service::Service(const String& name,
		 const String& clustername,
		 const Node& node,
		 bool failed,
		 bool autostart,
		 const String& time_since_transition) :
	_name(name),
	_clustername(clustername),
	_nodename(node.name()),
	_autostart(autostart),
	_failed(failed),
	_time_since_transition(time_since_transition)
{}

Service::~Service(void)
{}

String
Service::name() const
{
	return _name;
}

String
Service::clustername() const
{
	return _clustername;
}

bool
Service::running() const
{
	return _nodename.size();
}

String
Service::nodename() const
{
	return _nodename;
}

bool
Service::failed() const
{
	return _failed;
}

bool
Service::autostart() const
{
	return _autostart;
}

String
Service::time_since_transition() const
{
	return _time_since_transition;
}
