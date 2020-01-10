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

#include "Cluster.h"

#include <stdio.h>

extern "C" {
	#include <limits.h>
	#include <libcman.h>
}

using namespace std;
using namespace ClusterMonitoring;


Cluster::Cluster(	const String &name,
					const String &alias,
					const String &cluster_version,
					unsigned int minQuorum) :
	_name(name),
	_alias(alias),
	_cl_version(cluster_version),
	_minQuorum(minQuorum)
{
	// add no-node node
	addNode("", 0, false, false, "");
}

Cluster::~Cluster(void)
{}

String
Cluster::name()
{
	return _name;
}

String
Cluster::alias()
{
	return _alias;
}

String
Cluster::version()
{
	return _cl_version;
}

unsigned int
Cluster::votes()
{
	cman_handle_t ch = cman_init(NULL);
	if (ch != NULL) {
		char info[PIPE_BUF];
		cman_extra_info_t *cman_ei = (cman_extra_info_t *) info;
		unsigned int total_votes = 0;

		if (cman_get_extra_info(ch, cman_ei, sizeof(info)) == 0)
			total_votes = cman_ei->ei_total_votes;
		cman_finish(ch);
		if (total_votes > 0)
			return (total_votes);
	}

	unsigned int votes = 0;
	for (map<String, counting_auto_ptr<Node> >::iterator
			iter = _nodes.begin() ;
			iter != _nodes.end() ;
			iter++)
	{
		Node& node = *(iter->second);
		if (node.clustered())
			votes += node.votes();
	}
	return votes;
}

unsigned int
Cluster::minQuorum()
{
	cman_handle_t ch = cman_init(NULL);
	if (ch != NULL) {
		char info[PIPE_BUF];
		cman_extra_info_t *cman_ei = (cman_extra_info_t *) info;
		int minq = -1;

		if (cman_get_extra_info(ch, cman_ei, sizeof(info)) == 0)
			minq = cman_ei->ei_quorum;
		cman_finish(ch);

		if (minq != -1)
			return minq;
	}
	return 0;
}

bool
Cluster::quorate()
{
	cman_handle_t ch = cman_init(NULL);
	if (ch != NULL) {
		int quorate = cman_is_quorate(ch);
		cman_finish(ch);
		return quorate;
	}
	return false;
}

counting_auto_ptr<Node>
Cluster::addNode(	const String& name,
					unsigned int votes,
					bool online,
					bool clustered,
					const String& uptime)
{
	counting_auto_ptr<Node> node(new Node(name, _name, votes, online, clustered, uptime));
	if (_nodes.insert(pair<String, counting_auto_ptr<Node> >(name, node)).second)
		return node;
	else {
		// already present
		return _nodes[name];
	}
}

counting_auto_ptr<Service>
Cluster::addService(const String& name,
					const String& nodeName,
					bool failed,
					bool autostart,
					const String& time_since_transition)
{
	map<String, counting_auto_ptr<Node> >::iterator iter = _nodes.find(nodeName);
	if (iter == _nodes.end())
		throw String("Cluster::addService(): add node first");
	return iter->second->addService(name, failed, autostart, time_since_transition);
}

list<counting_auto_ptr<Node> >
Cluster::nodes()
{
	list<counting_auto_ptr<Node> > ret;

	for (map<String, counting_auto_ptr<Node> >::iterator
			iter = _nodes.begin() ;
			iter != _nodes.end() ;
			iter++)
	{
		counting_auto_ptr<Node>& node = iter->second;
		if (!node->name().empty())
			ret.push_back(node);
	}
	return ret;
}

std::list<counting_auto_ptr<Node> >
Cluster::clusteredNodes()
{
	list<counting_auto_ptr<Node> > ret;

	for (map<String, counting_auto_ptr<Node> >::iterator
			iter = _nodes.begin() ;
			iter != _nodes.end() ;
			iter++)
	{
		counting_auto_ptr<Node>& node = iter->second;
		if (node->name().size() && node->clustered())
			ret.push_back(node);
	}
	return ret;
}

list<counting_auto_ptr<Node> >
Cluster::unclusteredNodes()
{
	list<counting_auto_ptr<Node> > ret;

	for (map<String, counting_auto_ptr<Node> >::iterator
			iter = _nodes.begin() ;
			iter != _nodes.end() ;
			iter++)
	{
		counting_auto_ptr<Node>& node = iter->second;
		if (node->name().size() && !node->clustered())
			ret.push_back(node);
	}
	return ret;
}

list<counting_auto_ptr<Service> >
Cluster::services()
{
	list<counting_auto_ptr<Service> > ret;

	for (map<String, counting_auto_ptr<Node> >::iterator
		iter = _nodes.begin() ;
		iter != _nodes.end() ;
		iter++)
	{
		list<counting_auto_ptr<Service> > services = iter->second->services();
		ret.insert(ret.end(), services.begin(), services.end());
	}
	return ret;
}

list<counting_auto_ptr<Service> >
Cluster::runningServices()
{
	list<counting_auto_ptr<Service> > ret;

	list<counting_auto_ptr<Node> > nodes = this->nodes();
	for (list<counting_auto_ptr<Node> >::iterator
			iter = nodes.begin() ;
			iter != nodes.end() ;
			iter++)
	{
		counting_auto_ptr<Node>& node = *iter;
		list<counting_auto_ptr<Service> > services = node->services();
		if (node->name().size())
			ret.insert(ret.end(), services.begin(), services.end());
	}
	return ret;
}

std::list<counting_auto_ptr<Service> >
Cluster::stoppedServices()
{
	list<counting_auto_ptr<Service> > ret;
	list<counting_auto_ptr<Service> > services =
		_nodes.find("")->second->services();

	for (list<counting_auto_ptr<Service> >::iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		counting_auto_ptr<Service>& service = *iter;
		if (!service->running() && !service->failed())
			ret.push_back(service);
	}
	return ret;
}

std::list<counting_auto_ptr<Service> >
Cluster::failedServices()
{
	list<counting_auto_ptr<Service> > ret;
	list<counting_auto_ptr<Service> > services =
		_nodes.find("")->second->services();

	for (list<counting_auto_ptr<Service> >::iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		counting_auto_ptr<Service>& service = *iter;
		if (service->failed())
			ret.push_back(service);
	}
	return ret;
}

String
ClusterMonitoring::cluster2xml(Cluster& cluster)
{
	char buff[64];

	// cluster
	XMLObject clu("cluster");
	clu.set_attr("name", cluster.name());
	clu.set_attr("alias", cluster.alias());
	clu.set_attr("cluster_version", cluster.version());

	snprintf(buff, sizeof(buff), "%u", cluster.votes());
	clu.set_attr("votes", buff);

	snprintf(buff, sizeof(buff), "%u", cluster.minQuorum());
	clu.set_attr("minQuorum", buff);

	clu.set_attr("quorate", (cluster.quorate()) ? "true" : "false");

	// nodes
	std::list<counting_auto_ptr<Node> > nodes = cluster.nodes();
	for (std::list<counting_auto_ptr<Node> >::iterator
		iter = nodes.begin() ;
		iter != nodes.end() ;
		iter++)
	{
		Node& node = **iter;
		XMLObject n("node");
		n.set_attr("name", node.name());

		snprintf(buff, sizeof(buff), "%u", node.votes());
		n.set_attr("votes", buff);

		n.set_attr("online", (node.online()) ? "true" : "false");
		n.set_attr("clustered", (node.clustered()) ? "true" : "false");
		n.set_attr("uptime", node.uptime());
		clu.add_child(n);
	}

	// services
	std::list<counting_auto_ptr<Service> > services = cluster.services();
	for (std::list<counting_auto_ptr<Service> >::iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		Service& service = **iter;
		XMLObject s("service");
		s.set_attr("name", service.name());
		s.set_attr("running", (service.running()) ? "true" : "false");
		if (service.running())
			s.set_attr("nodename", service.nodename());
		else
			s.set_attr("nodename", "");
		s.set_attr("failed", (service.failed()) ? "true" : "false");
		s.set_attr("autostart", (service.autostart()) ? "true" : "false");
		s.set_attr("time_since_transition", service.time_since_transition());
		clu.add_child(s);
	}

	return generateXML(clu);
}

counting_auto_ptr<Cluster>
ClusterMonitoring::xml2cluster(const String& xml)
{
	XMLObject clu = parseXML(xml);
	if (clu.tag() != "cluster")
		throw String("xml2cluster(): invalid xml");

	// cluster
	String name = clu.get_attr("name");
	if (name.empty())
		throw String("xml2cluster(): missing cluster name");

	unsigned int minQuorum = 0;
	if (sscanf(clu.get_attr("minQuorum").c_str(), "%u", &minQuorum) != 1)
		throw String("xml2cluster(): invalid value for cluster's minQuorum");

	String alias = clu.get_attr("alias");
	String cl_version = clu.get_attr("cluster_version");

	counting_auto_ptr<Cluster> cluster(new Cluster(name, alias, cl_version, minQuorum));

	// nodes
	for (list<XMLObject>::const_iterator
			iter = clu.children().begin() ;
			iter != clu.children().end() ;
			iter++)
	{
		const XMLObject& obj = *iter;
		if (obj.tag() == "node") {
			// name
			String node_name = obj.get_attr("name");
			if (node_name.empty())
				throw String("xml2cluster(): node missing 'name' attr");

			// votes
			unsigned int votes;
			if (sscanf(obj.get_attr("votes").c_str(), "%u", &votes) != 1)
				throw String("xml2cluster(): invalid value for node's votes");

			// online
			String online_str = obj.get_attr("online");
			bool online = online_str == "true";
			if (online_str.empty())
				throw String("xml2cluster(): node missing 'online' attr");

			// clustered
			String clustered_str = obj.get_attr("clustered");
			bool clustered = clustered_str == "true";
			if (clustered_str.empty())
				throw String("xml2cluster(): node missing 'clustered' attr");

			// uptime
			String uptime = obj.get_attr("uptime");

			// add node to cluster
			cluster->addNode(node_name, votes, online, clustered, uptime);
		}
	}

	// services
	for (list<XMLObject>::const_iterator
			iter = clu.children().begin() ;
			iter != clu.children().end() ;
			iter++)
	{
		const XMLObject& obj = *iter;
		if (obj.tag() == "service") {
			// name
			String service_name = obj.get_attr("name");
			if (service_name.empty())
				throw String("xml2cluster(): service missing 'name' attr");

			// running
			String running_str = obj.get_attr("running");
			bool running = running_str == "true";
			if (running_str.empty())
				throw String("xml2cluster(): service missing 'running' attr");

			// nodename
			String nodename = obj.get_attr("nodename");
			if (running && nodename.empty())
				throw String("xml2cluster(): service missing 'nodename' attr");

			// failed
			String failed_str = obj.get_attr("failed");
			bool failed = failed_str == "true";
			if (failed_str.empty())
				throw String("xml2cluster(): service missing 'failed' attr");

			// autostart
			String autostart_str = obj.get_attr("autostart");
			bool autostart = autostart_str == "true";
			if (autostart_str.empty())
				throw String("xml2cluster(): service missing 'autostart' attr");

			// time since last transition
			String tst = obj.get_attr("time_since_transition");

			// add service to cluster
			cluster->addService(service_name, nodename, failed, autostart, tst);
		}
	}

	return cluster;
}
