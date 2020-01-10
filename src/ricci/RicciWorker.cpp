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

#include "RicciWorker.h"
#include "utils.h"
#include "QueueLocker.h"
#include "Time.h"
#include "XML_tags.h"
#include "RebootModule.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

extern "C" {
	#include "sys_util.h"
}

#include <iostream>

using namespace std;

void
usage(const char *progname)
{
	cerr << "Usage: " << progname << " -f <path to batch file>" << endl;
}

int
main(int argc, char **argv)
{
	if (argc != 3) {
		usage(argv[0]);
		exit(1);
	}

	if (String(argv[1]) != "-f") {
		usage(argv[0]);
		exit(1);
	}

	String path(argv[2]);

	if (daemon(0, 0)) {
		cerr << "daemon() failed" << endl;
		exit(1);
	}

	try {
		DBusController dbus;
		BatchWorker batch(dbus, path);
		batch.process();
		exit(0);
	} catch (String e) {
		cerr	<< __FILE__ << ":" << __LINE__ << ": "
				<< "exception: " << e << endl;
	} catch ( ... ) {
		cerr << "unknown exception" << endl;
	}

	exit(2);
}

// ############ ProcessWorker ##############


ProcessWorker::ProcessWorker(	DBusController& dbus,
								const XMLObject& xml,
								BatchWorker& batch,
								RebootModule& rm) :
	_dbus(dbus),
	_rm(rm),
	_report(xml),
	_batch(batch)
{
	String state_str = _report.get_attr("status");
	if (state_str.empty())
		_state = st_sched;
	else
		_state = (state) utils::to_long(state_str);
}

ProcessWorker::~ProcessWorker()
{}

bool
ProcessWorker::scheduled() const
{
	return _state == st_sched;
}

bool
ProcessWorker::in_progress() const
{
	return _state == st_prog;
}

bool
ProcessWorker::done() const
{
	return _state == st_done;
}

bool
ProcessWorker::completed() const
{
	return (_state != st_sched && _state != st_prog);
}

bool
ProcessWorker::failed() const
{
	return _state == st_req_fail || _state == st_mod_fail;
}

bool
ProcessWorker::removed() const
{
	return _state == st_removed;
}

void
ProcessWorker::remove()
{
	if (_state == st_sched)
		_state = st_removed;
}

XMLObject
ProcessWorker::report() const
{
	_report.set_attr("status", utils::to_string(_state));
	return _report;
}

void
ProcessWorker::process()
{
	if (completed())
		return;
	else
		_state = st_prog;

	if (_report.children().empty()) {
		_state = st_done;
		return;
	}

	String module_name(_report.get_attr("name"));
	XMLObject module_header("module");
	module_header.set_attr("name", module_name);

	try {
		XMLObject request = _report.children().front();
		XMLObject mod_resp;

		if (module_name == "reboot") {
			mod_resp = _rm.process(request);
			if (_rm.block() && check_response(mod_resp)) {
				if (mod_resp.tag() == "internal_error")
					throw int();
				module_header.add_child(mod_resp);
				_report = module_header;
				_state = st_done;
				_batch.save();

				// sleep while the machine reboots
				// ricci will start a new worker thread after the reboot
				// that will pickup where it left off
				select(0, NULL, NULL, NULL, NULL);
				return;
			}
		} else {
			String message = generateXML(request);
			String ret = _dbus.process(message, module_name);
			mod_resp = parseXML(ret);
		}

		if (mod_resp.tag() == "internal_error")
			throw int();
		module_header.add_child(mod_resp);
	} catch ( ... ) {
		_state = st_mod_fail;
		return;
	}

	// check status within response
	bool funcs_succeeded = check_response(module_header.children().front());

	if (funcs_succeeded)
		_state = st_done;
	else
		_state = st_req_fail;
	_report = module_header;
}

bool
ProcessWorker::check_response(const XMLObject& resp)
{
	bool funcs_succeeded = true;

	if (resp.tag() == "API_error")
		funcs_succeeded = false;
	else {
		for (list<XMLObject>::const_iterator
				func_iter = resp.children().begin() ;
				func_iter != resp.children().end() ;
				func_iter++)
		{
			const XMLObject& func = *func_iter;
			if (func.tag() == FUNC_RESPONSE_TAG) {
				for (list<XMLObject>::const_iterator
						var_iter = func.children().begin() ;
						var_iter != func.children().end() ;
						var_iter++)
				{
					const XMLObject& var = *var_iter;
					if (var.tag() == VARIABLE_TAG) {
						if (var.get_attr("name") == "success" &&
							var.get_attr("value") == "false")
						{
							funcs_succeeded = false;
						}
					}
				}
			}
		}
	}

	return funcs_succeeded;
}

// ############ BatchWorker ##############

BatchWorker::BatchWorker(DBusController& dbus, const String& path) :
	_rm(dbus),
	_path(path)
{
	QueueLocker lock;
	struct stat st;

	_fd = open(_path.c_str(), O_RDONLY);
	if (_fd == -1)
		throw String("unable to open batch file: ") + String(strerror(errno));

	try {
		// lock file
		while (flock(_fd, LOCK_EX | LOCK_NB)) {
			if (errno == EINTR)
				continue;
			else if (errno == EWOULDBLOCK)
				throw String("file is in use by other worker");
			else {
				throw String("unable to acquire flock: ")
						+ String(strerror(errno));
			}
		}

		if (fstat(_fd, &st) != 0)
			throw String("Unable to stat file: ") + String(strerror(errno));

		// read file
		String xml_str;

		while ((off_t) xml_str.size() < st.st_size) {
			char buff[4096];
			ssize_t res;

			res = read_restart(_fd, buff, sizeof(buff));
			if (res <= 0) {
				throw String("error reading batch file: ")
					+ String(strerror(-res));
			}
			xml_str.append(buff, res);
			memset(buff, 0, sizeof(buff));
		}

		// _xml
		_xml = parseXML(xml_str);
		if (_xml.tag() != "batch")
			throw String("not a batch file: opening tag is ") + _xml.tag();

		String state_str = _xml.get_attr("status");
		if (state_str.empty())
			throw String("missing status attr");
		_state = (ProcessWorker::state) utils::to_long(state_str);

		// parse xml and generate subprocesses
		for (list<XMLObject>::const_iterator
				iter = _xml.children().begin() ;
				iter != _xml.children().end() ;
				iter++)
		{
			if (iter->tag() == "module")
				_procs.push_back(counting_auto_ptr<ProcessWorker>(new ProcessWorker(dbus, *iter, *this, _rm)));
		}
	} catch ( ... ) {
		close_fd(_fd);
		throw;
	}
}

BatchWorker::~BatchWorker()
{
	QueueLocker lock;

	close_fd(_fd);
}

void
BatchWorker::close_fd(int fd)
{
	if (fd >= 0) {
		while (close(fd)) {
			if (errno != EINTR)
				break;
		}
	}
}

void
BatchWorker::process()
{
	if (_state == ProcessWorker::st_sched || _state == ProcessWorker::st_prog)
		_state = ProcessWorker::st_prog;
	else
		return;

	// process subprocesses
	for (list<counting_auto_ptr<ProcessWorker> >::iterator
		iter = _procs.begin() ;
		iter != _procs.end() ;
		iter++)
	{
		save();

		ProcessWorker& proc = **iter;
		proc.process();

		if (proc.failed()) {
			for (iter++ ; iter != _procs.end() ; iter++)
				(*iter)->remove();
			_state = ProcessWorker::st_req_fail;
			save();
			return;
		}
	}

	_state = ProcessWorker::st_done;
	save();
}

XMLObject
BatchWorker::report() const
{
	XMLObject result(_xml.tag());

	for (map<String, String>::const_iterator
		iter = _xml.attrs().begin() ;
		iter != _xml.attrs().end() ;
		iter++)
	{
		result.set_attr(iter->first, iter->second);
	}

	for (list<counting_auto_ptr<ProcessWorker> >::const_iterator
			iter = _procs.begin() ;
			iter != _procs.end() ;
			iter++)
	{
		result.add_child((*iter)->report());
	}

	result.set_attr("status", utils::to_string(_state));
	return result;
}

void
BatchWorker::save()
{
	QueueLocker lock;

	String path_tmp(_path + ".tmp");
	int fd_tmp = open(path_tmp.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0640);
	if (fd_tmp == -1) {
		throw String("unable to create tmp batch file: ")
				+ String(strerror(errno));
	}

	try {
		// lock path_tmp
		while (flock(fd_tmp, LOCK_EX)) {
			if (errno != EINTR) {
				throw String("unable to lock the tmp batch file: ")
						+ String(strerror(errno));
			}
		}

		// write to tmp file
		String out(generateXML(report()));
		do {
			int res = write(fd_tmp, out.c_str(), out.size());
			if (res == -1) {
				if (errno != EINTR) {
					throw String("unable to write batch file: ")
							+ String(strerror(errno));
				}
			} else
				out = out.substr(res);
		} while (out.size());

		// rename path_tmp to _path
		if (rename(path_tmp.c_str(), _path.c_str())) {
			throw String("unable to rename batch file: ")
					+ String(strerror(errno));
		}

		// close _fd, and replace it with fd_tmp
		close_fd(_fd);
		_fd = fd_tmp;
	} catch ( ... ) {
		close_fd(fd_tmp);
		unlink(path_tmp.c_str());
		throw;
	}
}
