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

#include "Monitor.h"
#include "executils.h"
#include "Logger.h"
#include "Time.h"
#include "utils.h"

#include <stdio.h>
#include <limits.h>
#include <sys/poll.h>
#include <sys/sysinfo.h>

#include <algorithm>

using namespace ClusterMonitoring;
using namespace std;

#define RG_STATE_STOPPED			110		/** Resource group is stopped */
#define RG_STATE_STARTING			111		/** Resource is starting */
#define RG_STATE_STARTED			112		/** Resource is started */
#define RG_STATE_STOPPING			113		/** Resource is stopping */
#define RG_STATE_FAILED				114		/** Resource has failed */
#define RG_STATE_UNINITIALIZED		115		/** Thread not running yet */
#define RG_STATE_CHECK				116		/** Checking status */
#define RG_STATE_ERROR				117		/** Recoverable error */
#define RG_STATE_RECOVER			118		/** Pending recovery */
#define RG_STATE_DISABLED			119		/** Resource not allowd to run */
#define RG_STATE_MIGRATE			120		/** Resource migrating */

#define EXECUTE_TIMEOUT				3000

static XMLObject merge_xmls(const XMLObject& what, const XMLObject& with);
static String cluster_version();

Monitor::Monitor(unsigned short port, const String& ver) :
	_comm(port, *this),
	_cl_version(ver.size() ? ver : cluster_version())
{
	log("Monitor created", LogMonitor);
}

Monitor::~Monitor()
{
	log("Stopping monitoring", LogMonitor);
	stop();
	log("Monitoring stopped", LogMonitor);
	log("Monitor deleted", LogMonitor);
}

void
Monitor::update_now()
{
	try {
		MutexLocker l(_mutex);
		String my_nodename, clustername, msg;
		vector<String> nodenames=get_local_info(my_nodename, clustername, msg);
		msg_arrived(my_nodename, msg);
		_cluster = merge_data(clustername);
	} catch (String e) {
		log(String(__FILE__) + ":" + __LINE__ + String(": caught exception: ") + e, LogCommunicator);
	} catch (...) { }
}

void
Monitor::run()
{
	log("Starting communicator", LogCommunicator);
	_comm.start();

	while (!shouldStop()) {
		unsigned int time_beg = time_sec();

		try {
			// get local info
			String my_nodename, clustername, msg;
			vector<String> nodenames = get_local_info(my_nodename,
											clustername, msg);

			// publish it
			_comm.update_peers(my_nodename, nodenames);
			_comm.send(msg + '\n');

			// merge data from all nodes (removing stale entries)
			// and update _cluster
			{
				MutexLocker l(_mutex);
				_cluster = merge_data(clustername);
			}
		} catch (String e) {
			log(String(__FILE__) + ":" + __LINE__ + String(": caught exception: ") + e, LogCommunicator);
			MutexLocker l(_mutex);
			_cluster = counting_auto_ptr<Cluster>();
		} catch ( ... ) {
			MutexLocker l(_mutex);
			_cluster = counting_auto_ptr<Cluster>();
		}

		String msg = String("monitoring iteration took ")
						+ (time_sec() - time_beg) + " seconds";
		log(msg, LogTime);

		// wait some time
		for (int i = 0 ; i < 10 ; i++) {
			if (shouldStop())
				break;
			struct pollfd nothing;
			poll(&nothing, 0, 500);
		}
	}
	log("Stopping communicator", LogCommunicator);
	_comm.stop();
	log("Communicator stopped", LogCommunicator);
}

String
Monitor::request(const String& msg)
{
	MutexLocker l(_mutex);

	if (msg == "GET") {
		XMLObject def_xml("cluster");
		def_xml.set_attr("cluster_version", _cl_version);
		String def(generateXML(def_xml) + "\n");

		if (_cluster.get() == NULL)
			update_now();
		if (_cluster.get() == NULL)
			return def;

		try {
			return cluster2xml(*_cluster) + "\n";
		} catch (String e) {
			log(String(__FILE__) + ":" + __LINE__ + String(": caught exception: ") + e, LogCommunicator);
			return def;
		} catch (...) {
			return def;
		}
	}
	throw String("invalid request");
}

void
Monitor::msg_arrived(const String& hostname, const String& msg_in)
{
	String msg(msg_in);
	// strip \n from the beggining

	while (msg.size()) {
		if (msg[0] == '\n')
			msg = msg.substr(1);
		else
			break;
	}

	try {
		XMLObject obj = parseXML(msg);
		if (obj.tag() == "clumond") {
			String type = obj.get_attr("type");
			if (type == "clusterupdate") {
				if (obj.children().size() == 1) {
					const XMLObject& cluster = *(obj.children().begin());
					if (cluster.tag() == "cluster") {
						MutexLocker l(_mutex);
						pair<unsigned int, XMLObject> data(time_sec(), cluster);
						_cache[hostname] = data;
					}
				}
				// TODO: other msgs
			}
		}
	} catch (String e) {
		log(String(__FILE__) + ":" + __LINE__ + String(": caught exception: ") + e, LogCommunicator);
	} catch ( ... ) {}
}

vector<String>
Monitor::get_local_info(String& nodename, String& clustername, String& msg)
{
	XMLObject cluster(parse_cluster_conf());

	// nodes
	vector<String> nodes;

	for (list<XMLObject>::const_iterator
			iter = cluster.children().begin() ;
			iter != cluster.children().end() ;
			iter++)
	{
		if (iter->tag() == "node")
			nodes.push_back(iter->get_attr("name"));
	}

	// clustername
	clustername = cluster.get_attr("name");

	// nodename
	nodename = this->nodename(nodes);

	try {
		cluster.set_attr("minQuorum", probe_quorum());
	} catch (...) {}

	cluster.set_attr("cluster_version", _cl_version);

	try {
		// insert current node info
		const vector<String> clustered_nodes = this->clustered_nodes();
		for (list<XMLObject>::const_iterator
				iter = cluster.children().begin() ;
				iter != cluster.children().end() ;
				iter++)
		{
			XMLObject& kid = (XMLObject&) *iter;
			if (kid.tag() == "node") {
				String name(kid.get_attr("name"));
				if (name == nodename) {
					// insert info about this node -> self
					kid.set_attr("uptime", uptime());
				}

				if (find(clustered_nodes.begin(), clustered_nodes.end(), name) !=
					clustered_nodes.end())
				{
					kid.set_attr("online", "true");
					kid.set_attr("clustered", "true");
				}
			}
		}
	} catch (String e) {
		log(String(__FILE__) + ":" + __LINE__ + ": caught exception: " + e, LogCommunicator);
	} catch (...) { }

	// insert current service info
	try {
		const vector<XMLObject> services_info = this->services_info();
		for (vector<XMLObject>::const_iterator
				iter_i = services_info.begin() ;
				iter_i != services_info.end() ;
				iter_i++)
		{
			const XMLObject& service = *iter_i;
			for (list<XMLObject>::const_iterator
					iter_c = cluster.children().begin() ;
					iter_c != cluster.children().end() ;
					iter_c++)
			{
				XMLObject& kid = (XMLObject&) *iter_c;
				if (kid.tag() == "service") {
					if (kid.get_attr("name") == service.get_attr("name")) {
						for (map<String, String>::const_iterator
							iter = service.attrs().begin() ;
							iter != service.attrs().end() ;
							iter++)
						{
							kid.set_attr(iter->first, iter->second);
						}
					}
				}
			}
		}
	} catch (String e) {
		log(String(__FILE__) + ":" + __LINE__ + ": caught exception: " + e, LogCommunicator);
	} catch (...) { }

	// ** return values **

	// msg
	XMLObject msg_xml("clumond");
	msg_xml.set_attr("type", "clusterupdate");
	msg_xml.add_child(cluster);
	msg = generateXML(msg_xml);

	// return nodes - nodename
	vector<String>::iterator iter = find(nodes.begin(), nodes.end(), nodename);
	if (iter != nodes.end())
		nodes.erase(iter);

	return nodes;
}

XMLObject
Monitor::parse_cluster_conf()
{
	XMLObject cluster_conf(readXML("/etc/cluster/cluster.conf"));
	if (cluster_conf.tag() != "cluster" ||
		utils::strip(cluster_conf.get_attr("name")).empty() ||
		utils::strip(cluster_conf.get_attr("config_version")).empty())
	{
		throw String("parse_cluster_conf(): invalid cluster.conf");
	}

	XMLObject cluster("cluster");
	for (map<String, String>::const_iterator
			iter = cluster_conf.attrs().begin() ;
			iter != cluster_conf.attrs().end() ;
			iter++)
	{
		cluster.set_attr(iter->first, iter->second);
	}

	if (utils::strip(cluster.get_attr("alias")).empty())
		cluster.set_attr("alias", cluster.get_attr("name"));

	for (list<XMLObject>::const_iterator
		iter = cluster_conf.children().begin() ;
		iter != cluster_conf.children().end() ;
		iter++)
	{
		const XMLObject& kid = *iter;
		if (kid.tag() == "clusternodes") {
			for (list<XMLObject>::const_iterator
					iter_n = kid.children().begin() ;
					iter_n != kid.children().end() ;
					iter_n++)
			{
				const XMLObject& node_conf = *iter_n;
				if (node_conf.tag() == "clusternode") {
					XMLObject node("node");
					for (map<String, String>::const_iterator
							iter_a = node_conf.attrs().begin() ;
							iter_a != node_conf.attrs().end() ;
							iter_a++)
					{
						node.set_attr(iter_a->first, iter_a->second);
					}
					cluster.add_child(node);
				}
			}
		} else if (kid.tag() == "rm") {
			for (list<XMLObject>::const_iterator
					iter_s = kid.children().begin() ;
					iter_s != kid.children().end() ;
					iter_s++)
			{
				const XMLObject& service_conf = *iter_s;

				if (service_conf.tag() == "service" ||
					service_conf.tag() == "vm")
				{
					XMLObject service("service");
					for (map<String, String>::const_iterator
						iter_a = service_conf.attrs().begin() ;
						iter_a != service_conf.attrs().end() ;
						iter_a++)
					{
						service.set_attr(iter_a->first, iter_a->second);
					}

					if (service_conf.tag() == "vm")
						service.set_attr("vm", "true");
					else
						service.set_attr("vm", "false");
					cluster.add_child(service);
				}
			}
		} else if (kid.tag() == "cman") {
			cluster.set_attr("locking", "cman");
			if (kid.has_attr("expected_votes"))
				cluster.set_attr("minQuorum", kid.get_attr("expected_votes"));
		}
	}

	cluster.set_attr("locking", "cman");

	return cluster;
}

counting_auto_ptr<Cluster>
Monitor::merge_data(const String& clustername)
{
	MutexLocker l(_mutex);

	XMLObject cluster("cluster");
	cluster.set_attr("name", clustername);
	cluster.set_attr("config_version", "0");
	unsigned int config_version = 0;

	vector<map<String, pair<unsigned int, XMLObject> >::iterator> stales;
	vector<String> online_nodes;

	for (map<String, pair<unsigned int, XMLObject> >::iterator
			iter = _cache.begin() ;
			iter != _cache.end() ;
			iter++)
	{
		if (iter->second.first < time_sec() - 8)
			stales.push_back(iter);
		else {
			online_nodes.push_back(iter->first);
			const XMLObject& cl2 = iter->second.second;
			if (cl2.has_attr("name") &&
				cl2.get_attr("name") == cluster.get_attr("name"))
			{
				unsigned int v;
				int ret;

				ret = sscanf(cl2.get_attr("config_version").c_str(), "%u", &v);
				if (ret != 1)
					continue;

				if (v == config_version)
					cluster = merge_xmls(cluster, cl2);
				else if (v > config_version) {
					config_version = v;
					cluster = cl2;
				}
			}
		}
	}

	for (unsigned int i = 0 ; i < stales.size() ; i++)
		_cache.erase(stales[i]);

	if (_cache.size() == 0)
		return counting_auto_ptr<Cluster>();

	// build cluster
	counting_auto_ptr<Cluster> cluster_ret;
	String name = cluster.get_attr("name");
	String alias = cluster.get_attr("alias");
	String clu_version = cluster.get_attr("cluster_version");
	unsigned int minQuorum = 0;

	if (sscanf(cluster.get_attr("minQuorum").c_str(), "%u", &minQuorum) != 1) {
		cluster_ret =
			counting_auto_ptr<Cluster> (new Cluster(name, alias, clu_version));
	} else {
		cluster_ret = counting_auto_ptr<Cluster> (new Cluster(name, alias, clu_version, minQuorum));
	}

	// nodes
	for (list<XMLObject>::const_iterator
			iter = cluster.children().begin() ;
			iter != cluster.children().end() ;
			iter++)
	{
		const XMLObject& obj = *iter;
		if (obj.tag() == "node") {
			String node_name = obj.get_attr("name");
			if (node_name.empty())
				throw String("merge_data(): node missing 'name' attr");
			unsigned int votes;
			if (sscanf(obj.get_attr("votes").c_str(), "%u", &votes) != 1)
				votes = 1;
			bool online;
			if (obj.has_attr("online"))
				online = obj.get_attr("online") == "true";
			else
				online = find(online_nodes.begin(), online_nodes.end(), node_name) != online_nodes.end();

			bool clustered = obj.get_attr("clustered") == "true";
			String uptime = obj.get_attr("uptime");

			// add node to cluster
			cluster_ret->addNode(node_name, votes, online, clustered, uptime);
		}
	}

	// services
	for (list<XMLObject>::const_iterator
			iter = cluster.children().begin() ;
			iter != cluster.children().end() ;
			iter++)
	{
		const XMLObject& obj = *iter;
		if (obj.tag() == "service") {
			// name
			String service_name = obj.get_attr("name");
			if (service_name.empty())
				throw String("merge_data(): service missing 'name' attr");

			bool running = obj.get_attr("running") == "true";
			String nodename = obj.get_attr("nodename");
			if (running && nodename.empty())
				throw String("merge_data(): running service missing 'nodename' attr");
			bool failed = obj.get_attr("failed") == "true";
			bool autostart = obj.get_attr("autostart") != "0";
			String time_since_trans = obj.get_attr("time_since_transition");

			// add service to cluster
			cluster_ret->addService(service_name, nodename, failed,
				autostart, time_since_trans);
		}
	}

	return cluster_ret;
}

vector<String>
Monitor::clustered_nodes()
{
	vector<String> running;
	int ret_nodes = -1;
	int ret;
	cman_node_t *node_array;

	cman_handle_t ch = cman_init(NULL);
	if (ch == NULL)
		throw String("Unable to communicate with cman");

	ret = cman_get_node_count(ch);
	if (ret <= 0) {
		cman_finish(ch);
		throw String("Unable to communicate with cman");
	}
	
	node_array = (cman_node_t *) calloc(ret, sizeof(*node_array));
	if (node_array == NULL) {
		cman_finish(ch);
		throw String("Out of memory");
	}

	if (cman_get_nodes(ch, ret, &ret_nodes, node_array) != 0) {
		cman_finish(ch);
		free(node_array);
		throw String("Unable to communicate with cman");
	}
	cman_finish(ch);

	try {
		for (int i = 0 ; i < ret_nodes ; i++) {
			if (node_array[i].cn_nodeid == 0) {
				/* qdisk */;
				continue;
			}
			if (node_array[i].cn_member)
				running.push_back(String(node_array[i].cn_name));
		}
	} catch (...) {
		free(node_array);
		throw String("error getting node names");
	}
	free(node_array);

	return running;
}

String Monitor::nodename(const vector<String>& nodenames) {
	struct ifaddrs *if_list = NULL;
	struct addrinfo hints;

	cman_handle_t ch = cman_init(NULL);
	if (ch != NULL) {
		cman_node_t this_node;
		if (cman_get_node(ch, CMAN_NODEID_US, &this_node) == 0) {
			cman_finish(ch);
			return String(this_node.cn_name);
		}
		cman_finish(ch);
	}

	if (getifaddrs(&if_list) != 0) 
		throw ("Unable to determine local node name");

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;

	for (vector<String>::const_iterator
			iter = nodenames.begin() ;
			iter != nodenames.end() ;
			iter++)
	{
		struct addrinfo *res;
		if (getaddrinfo(iter->c_str(), NULL, &hints, &res) == 0) {
			struct addrinfo *ai;
			for (ai = res ; ai != NULL ; ai = ai->ai_next) {
				struct ifaddrs *cur;

				for (cur = if_list ; cur != NULL ; cur = cur->ifa_next) {
					if (!cur->ifa_addr || !(cur->ifa_flags & IFF_UP))
						continue;
					if (cur->ifa_flags & IFF_LOOPBACK)
						continue;
					if (cur->ifa_addr->sa_family != AF_INET)
						continue;
					if (!memcmp(cur->ifa_addr, ai->ai_addr, ai->ai_addrlen)) {
						freeaddrinfo(res);
						freeifaddrs(if_list);
						return *iter;
					}
				}
			}
			freeaddrinfo(res);
		}
	}
	freeifaddrs(if_list);

	return "";
}

vector<XMLObject>
Monitor::services_info()
{
	vector<XMLObject> services;

	try {
		String out, err;
		int status;
		vector<String> args;

		args.clear();
		args.push_back("-f");
		args.push_back("-x");
		if (execute("/usr/sbin/clustat", args, out, err,
			status, EXECUTE_TIMEOUT))
		{
			throw String("services_info(): missing clustat");
		}

		if (status)
			throw String("services_info(): `clustat -x` failed: " + err);

		XMLObject clustat = parseXML(out);
		for (list<XMLObject>::const_iterator
				iter_c = clustat.children().begin() ;
				iter_c != clustat.children().end() ;
				iter_c++)
		{
			if (iter_c->tag() == "groups") {
				const XMLObject& groups = *iter_c;
				for (list<XMLObject>::const_iterator
						iter = groups.children().begin() ;
						iter != groups.children().end() ;
						iter++)
				{
					const XMLObject& group = *iter;
					XMLObject service("service");
					service.set_attr("vm", "false");

					// name
					String name(group.get_attr("name"));
					String::size_type idx = name.find(":");
					if (idx != name.npos) {
						if (name.substr(0, idx) == "vm")
							service.set_attr("vm", "true");
						name = name.substr(idx + 1);
					}
					service.set_attr("name", name);

					// state
					bool failed, running;
					int state_code;
					if (sscanf(group.get_attr("state").c_str(), "%i",
						&state_code) != 1)
					{
						continue;
					}
					switch (state_code) {
						case RG_STATE_STOPPED:
						case RG_STATE_STOPPING:
						case RG_STATE_UNINITIALIZED:
						case RG_STATE_ERROR:
						case RG_STATE_RECOVER:
						case RG_STATE_DISABLED:
							running = failed = false;
							break;

						case RG_STATE_FAILED:
							running = false;
							failed = true;
							break;

						case RG_STATE_MIGRATE:
						case RG_STATE_STARTING:
						case RG_STATE_STARTED:
						case RG_STATE_CHECK:
							running = true;
							failed = false;
							break;

						default:
							continue;
					}
					service.set_attr("failed", (failed) ? "true" : "false");
					service.set_attr("running", (running) ? "true" : "false");
					if (running)
						service.set_attr("nodename", group.get_attr("owner"));

					// last_transition
					time_t since = (time_t)
						utils::to_long(group.get_attr("last_transition"));
					if (since != 0) {
						time_t current = (time_t) time_sec();
						service.set_attr("time_since_transition",
							utils::to_string(current - since));
					}

					services.push_back(service);
				}
			}
		}
	} catch (String e) {
		log(String(__FILE__) + ":" + __LINE__ + String(": caught exception: ") + e, LogCommunicator);
	} catch ( ... ) {}

	return services;
}

String
Monitor::uptime() const
{
	struct sysinfo s_info;
	if (sysinfo(&s_info))
		return "";
	return String(utils::to_string(s_info.uptime));
}

String Monitor::probe_quorum() {
	String ret;

	cman_handle_t ch = cman_init(NULL);
	if (ch == NULL)
		throw String("quorum not found");

	if (cman_is_quorate(ch))
		ret = "Quorate";
	else
		ret = "Not Quorate";

	cman_finish(ch);
	return ret;
}

String cluster_version(void) {
	cman_handle_t ch;

	ch = cman_init(NULL);
	if (ch != NULL) {
		int ret;
		cman_version_t cman_version;
		String clu_version = "";

	    ret = cman_get_version(ch, &cman_version);
		if (ret >= 0) {
			if (cman_version.cv_major == 6) {
				if (cman_version.cv_minor >= 2)
					clu_version = "6";
				else
					clu_version = "5";
			} else if (cman_version.cv_major == 5)
				clu_version = "4";
			else {
				char version[32];
				snprintf(version, sizeof(version), "%d.%d.%d",
					cman_version.cv_major, cman_version.cv_minor,
					cman_version.cv_patch);
				cman_finish(ch);
				throw "unsupported cluster version: " + String(version);
			}
		}
		cman_finish(ch);
		return (clu_version);
	}

	return "6";
}

XMLObject
merge_xmls(const XMLObject& what, const XMLObject& with)
{
	if (what.tag() != with.tag())
		throw String("merge_xmls(): tag mismatch: \"" + what.tag() + "\" \"" + with.tag() + "\"");

	XMLObject new_xml(what.tag());
	for (map<String, String>::const_iterator
		iter = what.attrs().begin() ;
		iter != what.attrs().end() ;
		iter++)
	{
		new_xml.set_attr(iter->first, iter->second);
		for (map<String, String>::const_iterator
				iter = with.attrs().begin() ;
				iter != with.attrs().end() ;
				iter++)
		{
			new_xml.set_attr(iter->first, iter->second);
		}
	}

	list<XMLObject> kids_left = with.children();

	for (list<XMLObject>::const_iterator
			iter_o = what.children().begin() ;
			iter_o != what.children().end() ;
			iter_o++)
	{
		XMLObject new_kid(*iter_o);
		for (list<XMLObject>::const_iterator
				iter = with.children().begin() ;
				iter != with.children().end() ;
				iter++)
		{
			const XMLObject& kid = *iter;
			if (kid.tag() == new_kid.tag() &&
				kid.has_attr("name") &&
				new_kid.has_attr("name") &&
				kid.get_attr("name") == new_kid.get_attr("name"))
			{
				// same tag and name -->> merge
				new_kid = merge_xmls(new_kid, kid);
				kids_left.remove(kid);
			}
		}
		new_xml.add_child(new_kid);
	}

	for (list<XMLObject>::const_iterator
			iter = kids_left.begin() ;
			iter != kids_left.end() ;
			iter++)
	{
		new_xml.add_child(*iter);
	}

	return new_xml;
}
