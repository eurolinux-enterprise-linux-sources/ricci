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

#include "Ricci.h"
#include "Auth.h"
#include "utils.h"
#include "Random.h"
#include "ricci_defines.h"
#include "RicciWorker.h"
#include "QueueLocker.h"
#include "Time.h"
#include "XML_tags.h"
#include "File.h"
#include "Network.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

extern "C" {
	#include "sys_util.h"
}

#include <fstream>

using namespace std;

static bool dom0();
static pair<String, String> clusterinfo();
static String os_release();

extern bool advertise_cluster;

Ricci::Ricci(DBusController& dbus) :
	_dbus(dbus),
	_fail_auth_attempt(0)
{}

Ricci::~Ricci()
{}

XMLObject
Ricci::ricci_header(bool authed, bool full) const
{
	XMLObject header("ricci");
	header.set_attr("version", "1.0");

	if (authed)
		header.set_attr("authenticated", "true");
	else
		header.set_attr("authenticated", "false");

	if (full || advertise_cluster) {
		String name = Network::localhost();
		if (name.size())
			header.set_attr("hostname", name);

		pair<String, String> c_info = clusterinfo();
		if (c_info.first.size())
			header.set_attr("clustername", c_info.first);
		if (c_info.second.size())
			header.set_attr("clusteralias", c_info.second);

		if (authed) {
			String os = os_release();
			if (os.size())
				header.set_attr("os", os);

			header.set_attr("xen_host", dom0() ? "true" : "false");
		}
	}

	return header;
}

XMLObject
Ricci::hello(bool authed) const
{
	return ricci_header(authed, true);
}

XMLObject
Ricci::request(	const XMLObject& req,
				bool& authenticated,
				bool& save_cert,
				bool& remove_cert,
				bool& done)
{
	save_cert = false;
	remove_cert = false;
	done = false;

	if (req.tag() != "ricci") {
		done = true;
		return XMLObject("not_ricci_message");
	}

	XMLObject resp = ricci_header(authenticated);

	// version check
	String version = req.get_attr("version");
	if (version.empty()) {
		resp.set_attr("success", utils::to_string(RRC_MISSING_VERSION));
		return resp;
	} else if (req.get_attr("version") != "1.0") {
		resp.set_attr("success", utils::to_string(RRC_MISSING_VERSION));
		return resp;
	}

	RicciRetCode success = RRC_INTERNAL_ERROR;
	String function = req.get_attr("function");
	if (function == "") {
		success = RRC_MISSING_FUNCTION;
	} else if (function == "authenticate") {
		String passwd = req.get_attr("password");
		bool passwd_ok = false;
		if (passwd.size()) {
			try {
				passwd_ok = Auth().authenticate(passwd);
			} catch ( ... ) {}
		}

		if (passwd_ok) {
			resp = ricci_header(true, true);
			success = RRC_SUCCESS;
			save_cert = true;
			authenticated = true;
		} else {
			if (_fail_auth_attempt++ == 3)
				done = true;
			success = RRC_AUTH_FAIL;
		}
	} else if (function == "unauthenticate") {
		if (!authenticated) {
			success = RRC_SUCCESS;
		} else {
			resp = ricci_header(false);
			success = RRC_SUCCESS;
			remove_cert = true;
		}
		authenticated = false;
	} else if (function == "force_reboot" || function == "self_fence") {
		if (reboot(RB_AUTOBOOT) == 0)
			success = RRC_SUCCESS;
		else
			success = RRC_INTERNAL_ERROR;
	} else if (function == "list_modules") {
		// available modules
		if (!authenticated) {
			success = RRC_NEED_AUTH;
		} else {
			list<String> modules = _dbus.modules();
			for (list<String>::const_iterator
					iter = modules.begin() ;
					iter != modules.end() ;
					iter++)
			{
				XMLObject x("module");
				x.set_attr("name", *iter);
				resp.add_child(x);
			}
			success = RRC_SUCCESS;
		}
	} else if (function == "process_batch") {
		if (!authenticated) {
			success = RRC_NEED_AUTH;
		} else {
			bool async = (req.get_attr("async") == "true");

			const XMLObject* batch_xml = NULL;
			for (list<XMLObject>::const_iterator
					iter = req.children().begin() ;
					iter != req.children().end() ;
					iter++)
			{
				if (iter->tag() == "batch") {
					batch_xml = &(*iter);
					break;
				}
			}

			if (batch_xml) {
				try {
					long long id;

					if (true) {
						Batch batch(*batch_xml);
						id = batch.id();

						if (async) {
							resp.add_child(batch.report());
							success = RRC_SUCCESS;
						}
					}

					if (!async) {
						bool batch_done;
						do {
							sleep_mil(100);
							Batch batch(id);
							if ((batch_done = batch.done()) == true) {
								resp.add_child(batch.report());
								success = RRC_SUCCESS;
							}
						} while (!batch_done);
					}
				} catch ( ... ) {
					success = RRC_INTERNAL_ERROR;
				}
			} else
				success = RRC_MISSING_BATCH;
		}
	} else if (function == "batch_report") {
		// get report
		if (!authenticated) {
			success = RRC_NEED_AUTH;
		} else {
			long long id = utils::to_long(req.get_attr("batch_id"));
			if (id == 0)
				success = RRC_INVALID_BATCH_ID;
			else {
				try {
					Batch batch(id);
					resp.add_child(batch.report());
					success = RRC_SUCCESS;
				} catch ( ... ) {
					success = RRC_INVALID_BATCH_ID;
				}
			}
		}
	} else {
		// invalid function name
		success = RRC_INVALID_FUNCTION;
	}

	resp.set_attr("success", utils::to_string(success));
	return resp;
}

Batch::Batch(const XMLObject& xml) :
	_report(xml.tag()),
	_state(ProcessWorker::st_sched)
{
	QueueLocker lock;

	// id
	String path_tmp;
	do {
		_id = random_generator(1, 2147483647);
		_path = String(QUEUE_DIR_PATH) + utils::to_string(_id);
		path_tmp = _path + ".tmp";
		if (access(_path.c_str(), F_OK))
			break;
	} while (true);

	// generate request
	for (map<String, String>::const_iterator
			iter = xml.attrs().begin() ;
			iter != xml.attrs().end() ;
			iter++)
	{
		_report.set_attr(iter->first, iter->second);
	}

	_report.set_attr("batch_id", utils::to_string(_id));
	_report.set_attr("status", utils::to_string(_state));

	for (list<XMLObject>::const_iterator
			iter = xml.children().begin() ;
			iter != xml.children().end() ;
			iter++)
	{
		XMLObject child(*iter);

		if (iter->tag() == "module")
			child.set_attr("status", utils::to_string(_state));
		_report.add_child(child);
	}

	// create file
	int fd = open(path_tmp.c_str(), O_RDWR | O_CREAT | O_EXCL, 0640);
	if (fd == -1)
		throw String("unable to create batch file: ") + String(strerror(errno));

	try {
		// save request
		String xml_str(generateXML(_report));
		ssize_t ret = write_restart(fd, xml_str.c_str(), xml_str.size());
		if (ret < 0) {
			throw String("unable to write batch request: ")
					+ String(strerror(-ret));
		}

		close(fd);

		if (rename(path_tmp.c_str(), _path.c_str())) {
			throw String("failed to rename batch file: ")
					+ String(strerror(errno));
		}
	} catch ( ... ) {
		close(fd);
		unlink(path_tmp.c_str());
		throw;
	}

	try {
		start_worker(_path);
	} catch ( ... ) {
		unlink(_path.c_str());
		throw;
	}
}

Batch::Batch(long long id) :
	_id(id)
{
	QueueLocker lock;
	String batch;

	// read file
	_path = String(QUEUE_DIR_PATH) + utils::to_string(_id);
	FILE *file = fopen(_path.c_str(), "r");
	if (!file)
		throw String("unable to open batch file: ") + String(strerror(errno));

	try {
		do {
			char buff[4096];
			size_t res = fread(buff, 1, sizeof(buff), file);
			int err = errno;
			batch.append(buff, res);
			memset(buff, 0, sizeof(buff));

			if (res < sizeof(buff)) {
				if (ferror(file)) {
					throw String("unable to read batch file: ")
							+ String(strerror(err));
				} else
					break;
			}
		} while (true);
		fclose(file);
	} catch ( ... ) {
		fclose(file);
		throw;
	}

	_report = parseXML(batch);
	if (utils::to_long(_report.get_attr("batch_id")) != _id)
		throw String("ID doesn't match");
	_state = utils::to_long(_report.get_attr("status"));
}

Batch::~Batch()
{
	QueueLocker lock;

	if (_state != ProcessWorker::st_sched &&
		_state != ProcessWorker::st_prog)
	{
		try {
			File f(File::open(_path, true));
			f.shred();
			f.unlink();
		} catch ( ... ) {}
	}
}

long long
Batch::id() const
{
	return _id;
}

bool
Batch::done() const
{
	bool done = ((_state != ProcessWorker::st_sched) &&
					(_state != ProcessWorker::st_prog));
	return done;
}

XMLObject
Batch::report() const
{
	if (done())
		return _report;

	XMLObject rep = _report;

	// TODO: clean-up modules if st_sched || st_prog
	return rep;
}

void
Batch::start_worker(const String& path)
{
	String out, err;
	int status;
	vector<String> args;

	args.push_back("-f");
	args.push_back(path);

	if (utils::execute(RICCI_WORKER_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(RICCI_WORKER_PATH);

	if (status)
		throw String("execution of ricci-worker failed: " + err);
}

void
Batch::restart_batches()
{
	QueueLocker lock;
	DIR *dir = opendir(QUEUE_DIR_PATH);
	if (!dir) {
		throw String("unable to open queue directory: ")
				+ String(strerror(errno));
	}

	struct dirent *file_entry;
	while ((file_entry = readdir(dir))) {
		try {
			String name(file_entry->d_name);
			// check name
			if (name.find_first_not_of("0123456789") == name.npos) {
				// start worker
				start_worker(String(QUEUE_DIR_PATH) + name);
			}
		} catch ( ... ) {}
	}
	closedir(dir);
}

pair<String, String>
clusterinfo()
{
	try {
		XMLObject xml(readXML("/etc/cluster/cluster.conf"));
		String name = xml.get_attr("name");
		String alias = xml.get_attr("alias");

		if (utils::strip(alias).empty())
			alias = name;
		return pair<String, String>(name, alias);
	} catch ( ... ) {
		return pair<String, String>("", "");
	}
}

String
os_release()
{
	try {
		return utils::strip(File::open("/etc/redhat-release"));
	} catch ( ... ) {
		return "";
	}
}

bool
dom0()
{
	try {
		String out, err;
		int status;
		vector<String> args;

		args.push_back("nodeinfo");
		if (utils::execute("/usr/bin/virsh", args, out, err, status, false))
			throw command_not_found_error_msg("/usr/bin/virsh");
		if (status == 0)
			return true;
	} catch ( ... ) {}

	return false;
}
