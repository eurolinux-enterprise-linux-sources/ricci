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
 * Authors:
 *  Stanko Kupcevic <kupcevic@redhat.com>
 *  Ryan McCabe <rmccabe@redhat.com>
 */

#include "Clusvcadm.h"
#include "NoServiceManager.h"
#include "utils.h"
#include "XML.h"


using namespace std;

#define CLUSTAT_TOOL_PATH		"/usr/sbin/clustat"
#define CLUSVCADM_TOOL_PATH		"/usr/sbin/clusvcadm"

class ServiceStatus {
	public:
		enum state {
			RG_STATE_STOPPED		= 110,	// Resource group is stopped
			RG_STATE_STARTING		= 111,	// Resource is starting
			RG_STATE_STARTED		= 112,	// Resource is started
			RG_STATE_STOPPING		= 113,	// Resource is stopping
			RG_STATE_FAILED			= 114,	// Resource has failed
			RG_STATE_UNINITIALIZED	= 115,	// Thread not running yet
			RG_STATE_CHECK			= 116,	// Checking status
			RG_STATE_ERROR			= 117,	// Recoverable error
			RG_STATE_RECOVER		= 118,	// Pending recovery
			RG_STATE_DISABLED		= 119,	// Resource not allowd to run
			RG_STATE_MIGRATE		= 120	// Resource migrating
		};

		ServiceStatus(const String& name,
			const String& node, state status, bool vm) :
				name(name),
				node(node),
				status(status),
				vm(vm) {}
		virtual ~ServiceStatus() {}

		String name;
		String node;
		state status;
		bool vm;
};

static pair<list<String>, list<ServiceStatus> > service_states();

void Clusvcadm::start(const String& servicename, const String& nodename) {
	pair<list<String>, list<ServiceStatus> > info = service_states();
	list<String> nodes = info.first;
	list<ServiceStatus> services = info.second;

	// check if node can run services
	bool node_found = false;

	for (list<String>::const_iterator
		iter = nodes.begin() ;
		iter != nodes.end() ;
		iter++)
	{
		if (*iter == nodename) {
			node_found = true;
			break;
		}
	}

	if (!node_found && nodename.size()) {
		throw String("Node " + nodename
			+ " is unable to run cluster services. Check whether the rgmanager service is running on that node.");
	}

	// start
	for (list<ServiceStatus>::const_iterator
		iter = services.begin() ;
		iter != services.end() ;
		iter++)
	{
		if (iter->name != servicename)
			continue;
		String flag;

		if (iter->status == ServiceStatus::RG_STATE_MIGRATE)
			throw String(servicename + " is in the process of being migrated");

		/*
		** Failed services must be disabled before they can be
		** started again.
		*/
		if (iter->status == ServiceStatus::RG_STATE_FAILED) {
			try {
				Clusvcadm::stop(servicename);
			} catch (String e) {
				throw String("Unable to disable failed service "
						+ servicename + " before starting it: " + e);
			} catch ( ... ) {
				throw String("Unable to disable failed service "
						+ servicename + " before starting it");
			}
			flag = "-e";
		} else if (	iter->status == ServiceStatus::RG_STATE_STOPPED ||
					iter->status == ServiceStatus::RG_STATE_STOPPING ||
					iter->status == ServiceStatus::RG_STATE_ERROR ||
					iter->status == ServiceStatus::RG_STATE_DISABLED)
		{
			flag = "-e";
		} else if (	iter->status == ServiceStatus::RG_STATE_STARTED ||
					iter->status == ServiceStatus::RG_STATE_STARTING)
		{
			flag = "-r";
		}

		if (flag.size() < 1) {
			throw String(servicename + " is in unknown state "
					+ utils::to_string(iter->status));
		}

		String out, err;
		int status;
		vector<String> args;

		args.push_back(flag);
		if (iter->vm)
			args.push_back("vm:" + servicename);
		else
			args.push_back(servicename);

		if (nodename.size()) {
			args.push_back("-m");
			args.push_back(nodename);
		}

		if (utils::execute(CLUSVCADM_TOOL_PATH, args, out, err, status, false))
			throw command_not_found_error_msg(CLUSVCADM_TOOL_PATH);

		if (status != 0) {
			throw String("clusvcadm start failed to start "
					+ servicename + ": " + err);
		}
		return;
	}

	throw String(servicename + ": no such cluster service");
}

void Clusvcadm::migrate(const String& servicename, const String& nodename) {
	pair<list<String>, list<ServiceStatus> > info = service_states();
	list<String> nodes = info.first;
	list<ServiceStatus> services = info.second;

	// check if node can run services
	bool node_found = false;
	for (list<String>::const_iterator
		iter = nodes.begin() ;
		iter != nodes.end() ;
		iter++)
	{
		if (*iter == nodename) {
			node_found = true;
			break;
		}
	}

	if (!node_found && nodename.size())
		throw String("Node " + nodename + " is unable to run cluster services. Check whether the rgmanager service is running on that node.");

	// start
	for (list<ServiceStatus>::const_iterator
		iter = services.begin() ;
		iter != services.end() ;
		iter++)
	{
		if (!iter->vm)
			continue;
		if (iter->name != servicename)
			continue;

		String flag;
		if (iter->status == ServiceStatus::RG_STATE_MIGRATE)
			throw String(servicename +
					" is already in the process of being migrated");

		if (iter->status == ServiceStatus::RG_STATE_FAILED) {
			try {
				Clusvcadm::stop(servicename);
			} catch (String e) {
				throw String("Unable to disable failed service "
						+ servicename + " before starting it: " + e);
			} catch ( ... ) {
				throw String("Unable to disable failed service "
						+ servicename + " before starting it");
			}
			flag = "-e";
		} else if (	iter->status == ServiceStatus::RG_STATE_STOPPED ||
					iter->status == ServiceStatus::RG_STATE_STOPPING ||
					iter->status == ServiceStatus::RG_STATE_ERROR ||
					iter->status == ServiceStatus::RG_STATE_DISABLED)
		{
			flag = "-e";
		} else if (	iter->status == ServiceStatus::RG_STATE_STARTED ||
					iter->status == ServiceStatus::RG_STATE_STARTING)
		{
			flag = "-M";
		}

		if (flag.size() < 1) {
			throw String(servicename + " is in unknown state "
					+ utils::to_string(iter->status));
		}

		String out, err;
		int status;
		vector<String> args;

		args.push_back(flag);
		args.push_back("vm:" + servicename);

		if (nodename.size()) {
			args.push_back("-m");
			args.push_back(nodename);
		}

		if (utils::execute(CLUSVCADM_TOOL_PATH, args, out, err, status, false))
			throw command_not_found_error_msg(CLUSVCADM_TOOL_PATH);

		if (status != 0) {
			throw String("clusvcadm failed to migrate "
					+ servicename + ": " + err);
		}
		return;
	}

	throw String(servicename + ": no such virtual machine service");
}

void Clusvcadm::stop(const String& servicename) {
	pair<list<String>, list<ServiceStatus> > info = service_states();
	list<String> nodes = info.first;
	list<ServiceStatus> services = info.second;

	for (list<ServiceStatus>::const_iterator
		iter = services.begin() ;
		iter != services.end() ;
		iter++)
	{
		if (iter->name != servicename)
			continue;

		if (iter->status == ServiceStatus::RG_STATE_STARTING ||
			iter->status == ServiceStatus::RG_STATE_FAILED ||
			iter->status == ServiceStatus::RG_STATE_STARTED)
		{
			String out, err;
			int status;
			vector<String> args;

			args.push_back("-d");
			if (iter->vm)
				args.push_back("vm:" + servicename);
			else
				args.push_back(servicename);

			if (utils::execute(CLUSVCADM_TOOL_PATH, args, out, err, status, false))
				throw command_not_found_error_msg(CLUSVCADM_TOOL_PATH);

			if (status != 0) {
				throw String("clusvcadm failed to stop "
						+ servicename + ": " + err);
			}
			return;
		}
	}

	throw String(servicename + ": no such cluster service");
}

void Clusvcadm::restart(const String& servicename) {
	pair<list<String>, list<ServiceStatus> > info = service_states();
	list<String> nodes = info.first;
	list<ServiceStatus> services = info.second;

	for (list<ServiceStatus>::const_iterator
		iter = services.begin() ;
		iter != services.end() ;
		iter++)
	{
		if (iter->name != servicename)
			continue;
		if (iter->status == ServiceStatus::RG_STATE_MIGRATE)
			throw String(servicename + " is in the process of being migrated");
		if (iter->status == ServiceStatus::RG_STATE_STARTING)
			throw String(servicename + " is in the process of being started");

		String flag;
		if (iter->status == ServiceStatus::RG_STATE_FAILED) {
			try {
				Clusvcadm::stop(servicename);
			} catch (String e) {
				throw String("Unable to disable failed service "
						+ servicename + " before starting it: " + e);
			} catch ( ... ) {
				throw String("Unable to disable failed service "
						+ servicename + " before starting it");
			}
			flag = "-e";
		} else if (	iter->status == ServiceStatus::RG_STATE_STOPPED ||
					iter->status == ServiceStatus::RG_STATE_STOPPING ||
					iter->status == ServiceStatus::RG_STATE_ERROR ||
					iter->status == ServiceStatus::RG_STATE_DISABLED)
		{
			flag = "-e";
		} else if (iter->status == ServiceStatus::RG_STATE_STARTED)
			flag = "-R";

		if (flag.size() < 1) {
			throw String(servicename + " is in unknown state "
					+ utils::to_string(iter->status));
		}

		String out, err;
		int status;
		vector<String> args;
		args.push_back(flag);

		if (iter->vm)
			args.push_back("vm:" + servicename);
		else
			args.push_back(servicename);

		if (utils::execute(CLUSVCADM_TOOL_PATH, args, out, err, status, false))
			throw command_not_found_error_msg(CLUSVCADM_TOOL_PATH);
		if (status != 0)
			throw String("clusvcadm failed to restart cluster service " + servicename + ": " + err);
		return;
	}

	throw String(servicename + ": no such cluster service");
}

pair<list<String>, list<ServiceStatus> > service_states() {
	String out, err;
	int status;
	vector<String> args;

	args.clear();
	args.push_back("-f");
	args.push_back("-x");
	if (utils::execute(CLUSTAT_TOOL_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(CLUSTAT_TOOL_PATH);
	if (status)
		throw String("clustat -fx failed: " + err);

	XMLObject xml = parseXML(out);
	if (xml.tag() != "clustat")
		throw String("invalid clustat output (expecting 'clustat' tag)");

	XMLObject nodes_xml("noname"), groups_xml("noname"), quorum_xml("noname");
	for (list<XMLObject>::const_iterator
		iter = xml.children().begin() ;
		iter != xml.children().end() ;
		iter++)
	{
		if (iter->tag() == "nodes")
			nodes_xml = *iter;
		else if (iter->tag() == "groups")
			groups_xml = *iter;
		else if (iter->tag() == "quorum")
			quorum_xml = *iter;
	}

	if (quorum_xml.get_attr("groupmember") != "1")
		throw NoServiceManager();

	list<String> nodes;
	for (list<XMLObject>::const_iterator
		iter = nodes_xml.children().begin() ;
		iter != nodes_xml.children().end() ;
		iter++)
	{
		if (iter->tag() == "node")
			nodes.push_back(iter->get_attr("name"));
	}

	list<ServiceStatus> services;
	for (list<XMLObject>::const_iterator
		iter = groups_xml.children().begin() ;
		iter != groups_xml.children().end() ;
		iter++)
	{
		if (iter->tag() == "group") {
			bool vm = false;
			String name(iter->get_attr("name"));
			String::size_type idx = name.find(":");
			if (idx != name.npos) {
				if (name.substr(0, idx) == "vm")
					vm = true;
				name = name.substr(idx + 1);
			}
			String node(iter->get_attr("owner"));
			ServiceStatus::state state =
				(ServiceStatus::state) utils::to_long(iter->get_attr("state"));
			services.push_back(ServiceStatus(name, node, state, vm));
		}
	}

	return pair<list<String>, list<ServiceStatus> >(nodes, services);
}
