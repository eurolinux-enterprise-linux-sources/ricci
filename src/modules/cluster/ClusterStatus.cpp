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

#include "ClusterStatus.h"
#include "Socket.h"
#include "Time.h"
#include "ClusterConf.h"
#include "Clusvcadm.h"
#include "utils.h"

#include "String.h"


using namespace std;

#define INITD_DIR_PATH			"/etc/init.d/"
#define CHKCONFIG_PATH			"/sbin/chkconfig"
#define LVMCONF_PATH			"/usr/sbin/lvmconf"

#define CMAN_LEAVE_TIMEOUT		"120"	// seconds (string)
#define CLUMON_SYNC_TIME		8		// seconds
#define CMAN_SETTLE_TIME		3		// seconds


static void run_initd(const String& servname, bool start, bool fail);
static void run_chkconfig(const String& servname, bool on);

bool
is_service_vm(const XMLObject& cluster_conf, const String& name)
{
	for (list<XMLObject>::const_iterator
			iter = cluster_conf.children().begin() ;
			iter != cluster_conf.children().end() ;
			iter++)
	{
		const XMLObject& kid = *iter;
		if (kid.tag() == "rm") {
			for (list<XMLObject>::const_iterator
					iter_s = kid.children().begin() ;
					iter_s != kid.children().end() ;
					iter_s++)
			{
				if (iter_s->tag() == "vm") {
					if (iter_s->get_attr("name") == name)
						return true;
				}
			}
		}
	}
	return false;
}

XMLObject
Cluster::status()
{
	ClientSocket sock;

	try {
		ClientSocket s("/var/run/clumond.sock");
		sock = s;
	} catch ( ... ) {
		// start clumon
		run_initd("modclusterd", true, true);

		// wait for it to come up and sync
		sleep_sec(CLUMON_SYNC_TIME);

		// try again
		ClientSocket s("/var/run/clumond.sock");
		sock = s;
	}
	sock.nonblocking(true);

	// send status request
	int beg = int(time_sec());
	String request("GET");

	while ((int(time_sec()) < beg + 5)) {
		bool read = false, write = true;
		sock.ready(read, write, 500);
		if (write) {
			if ((request = sock.send(request)).empty())
				break;
		}
	}

	// receive status report
	beg = int(time_sec());
	String xml_in;

	while ((int(time_sec()) < beg + 5)) {
		bool read = true, write = false;
		sock.ready(read, write, 500);
		if (read)
			xml_in += sock.recv();
		try {
			parseXML(xml_in);
			break;
		} catch ( ... ) {}
	}

	const XMLObject status_xml(parseXML(xml_in));
	const XMLObject cluster_conf(ClusterConf::get());

	if (cluster_conf.get_attr("name") != status_xml.get_attr("name"))
		throw String("cluster names mismatch");

	// add "vm" attr to services
	XMLObject status_new(status_xml.tag());
	for (map<String, String>::const_iterator
			iter = status_xml.attrs().begin() ;
			iter != status_xml.attrs().end() ;
			iter++)
	{
		status_new.set_attr(iter->first, iter->second);
	}

	for (list<XMLObject>::const_iterator
			iter = status_xml.children().begin() ;
			iter != status_xml.children().end() ;
			iter++)
	{
		XMLObject s(*iter);
		if (s.tag() == "service")
			s.set_attr("vm", (is_service_vm(cluster_conf,
								s.get_attr("name"))) ? "true" : "false");
		status_new.add_child(s);
	}
	return status_new;
}

void
Cluster::start_node(bool cluster_startup, bool enable_services)
{
	// bail out if cluster.conf is not present
	XMLObject cluster_conf(ClusterConf::get());
	XMLObject stat = status();

	if (stat.get_attr("cluster_version") == "6") {
		try {
			run_initd("cman", true, true);
		} catch ( ... ) {
			// try again
			run_initd("cman", true, true);
		}

		run_initd("clvmd", true, false);
		run_initd("gfs2", true, false);
		run_initd("scsi_reserve", true, false);
		run_initd("rgmanager", true, true);

		if (enable_services) {
			// enable them on boot
			run_chkconfig("cman", true);
			run_chkconfig("clvmd", true);
			run_chkconfig("gfs2", true);
			run_chkconfig("scsi_reserve", true);
			run_chkconfig("rgmanager", true);
		}
	} else {
		throw String("unsupported cluster version ")
				+ stat.get_attr("cluster_version");
	}
}

void
Cluster::stop_node(	bool cluster_shutdown,
					bool purge_conf,
					bool disable_services)
{
	XMLObject stat = status();

	if (cluster_shutdown) {
		// stop all services, so they don't bounce around
		for (list<XMLObject>::const_iterator
			iter = stat.children().begin() ;
			iter != stat.children().end() ;
			iter++)
		{
			if (iter->tag() == "service") {
				if (iter->get_attr("running") == "true") {
					try {
						Clusvcadm::stop(iter->get_attr("name"));
					} catch ( ... ) {}
				}
			}
		}
	}

	if (stat.get_attr("cluster_version") == "5") {
		run_initd("rgmanager", false, true);
		run_initd("gfs2", false, false);
		run_initd("clvmd", false, false);
		run_initd("cman", false, true);

		if (disable_services) {
			// disable them on boot
			run_chkconfig("cman", false);
			run_chkconfig("clvmd", false);
			run_chkconfig("gfs2", false);
			run_chkconfig("rgmanager", false);
		}
	} else {
		throw String("unsupported cluster version ")
				+ stat.get_attr("cluster_version");
	}

	if (purge_conf) {
		ClusterConf::purge_conf();

		// disable LVM cluster locking
		try {
			String out, err;
			int status;
			vector<String> args;

			args.push_back("--disable-cluster");
			utils::execute(LVMCONF_PATH, args, out, err, status, false);
		} catch ( ... ) {}
	}
}

void
run_chkconfig(const String& servname, bool on)
{
	String out, err;
	int status;
	vector<String> args;

	args.push_back(servname);
	if (on)
		args.push_back("on");
	else
		args.push_back("off");
	utils::execute(CHKCONFIG_PATH, args, out, err, status, false);
}

void
run_initd(const String& servname, bool start, bool fail)
{
	String path(INITD_DIR_PATH);
	path += servname;

	String out, err;
	int status;
	vector<String> args;

	if (start)
		args.push_back("start");
	else
		args.push_back("stop");
	bool failed = true;

	if (utils::execute(path, args, out, err, status, false) == 0) {
		if (status == 0)
			failed = false;
	}

	if (fail && failed) {
		throw String("service ") + servname + " "
				+ String(start ? "start" : "stop") + " failed: " + err;
	}
}
