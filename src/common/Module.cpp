/*
** Copyright (C) Red Hat, Inc. 2006-2009
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

#include "Module.h"
#include "Variable.h"
#include "XML_tags.h"
#include "Except.h"
#include "APIerror.h"
#include "Time.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/poll.h>
#include <errno.h>

extern "C" {
	#include "sys_util.h"
}

typedef struct pollfd poll_fd;

#include <iostream>

using namespace std;

static const unsigned int timeout = 3000; // milliseconds
static ApiFcnMap _api_fcns; // api->name->function map

static VarMap list_APIs(const VarMap& args);
static VarMap extract_vars(const XMLObject& xml);
static void insert_vars(const VarMap& vars, XMLObject& xml);

// ######## Module ########

#define APIs_FUNC_NAME			"APIs"

Module::Module(const ApiFcnMap& api_fcns)
{
	for (ApiFcnMap::const_iterator
			api_iter = api_fcns.begin();
			api_iter != api_fcns.end();
			api_iter++)
	{
		const String& api_vers = api_iter->first;

		if (api_vers.empty())
			continue;

		FcnMap funcs = api_iter->second;
		funcs[APIs_FUNC_NAME] = list_APIs;
		_api_fcns[api_vers] = funcs;
	}
}

Module::~Module()
{}

XMLObject
Module::process(const XMLObject& request)
{
	try {
		if (request.tag() != REQUEST_TAG)
			throw APIerror("missing request tag");

		String version = request.get_attr(MOD_VERSION_TAG);
		if (_api_fcns.find(version) == _api_fcns.end())
			throw APIerror("unsupported API version");

		if (request.children().size() != 1)
			throw APIerror(String("missing ") + FUNC_CALL_TAG);

		const XMLObject& func_xml = request.children().front();
		if (func_xml.tag() != FUNC_CALL_TAG)
			throw APIerror(String("missing ") + FUNC_CALL_TAG);

		String fcn_name = func_xml.get_attr("name");
		if (fcn_name.empty())
			throw APIerror("missing function name");

		FcnMap& fcns = _api_fcns[version];
		if (fcns.find(fcn_name) == fcns.end()) {
			throw APIerror(String("function '") + fcn_name
					+ "' not in API '" + version + "'");
		}

		// construct response xml
		XMLObject response(RESPONSE_TAG);
		response.set_attr(MOD_VERSION_TAG, version);
		response.set_attr(SEQUENCE_TAG, request.get_attr(SEQUENCE_TAG));

		XMLObject func_resp_xml(FUNC_RESPONSE_TAG);
		func_resp_xml.set_attr("function_name", fcn_name);

		try {
			map<String, Variable> in_vars = extract_vars(func_xml);
			map<String, Variable> out_vars = (fcns[fcn_name])(in_vars);

			insert_vars(out_vars, func_resp_xml);
			func_resp_xml.add_child(Variable("success", true).xml());
		} catch (Except e) {
			func_resp_xml.add_child(Variable("success", false).xml());
			func_resp_xml.add_child(Variable("error_code", e.code()).xml());
			func_resp_xml.add_child(Variable("error_description",
				e.description()).xml());
		} catch ( String e ) {
			func_resp_xml.add_child(Variable("success", false).xml());
			func_resp_xml.add_child(Variable("error_code",
				Except::generic_error).xml());
			func_resp_xml.add_child(Variable("error_description", e).xml());
		} catch ( APIerror e ) {
			throw;
		} catch ( ... ) {
			func_resp_xml.add_child(Variable("success", false).xml());
			func_resp_xml.add_child(Variable("error_code",
				Except::generic_error).xml());
			func_resp_xml.add_child(Variable("error_description",
				String("No description")).xml());
		}
		response.add_child(func_resp_xml);
		return response;
	} catch ( APIerror e ) {
		XMLObject err_resp("API_error");
		err_resp.set_attr("description", e.msg);
		insert_vars(list_APIs(map<String, Variable>()), err_resp);
		return err_resp;
	} catch ( ... ) {
		XMLObject err_resp("internal_error");
		return err_resp;
	}
}

VarMap
extract_vars(const XMLObject& xml)
{
	map<String, Variable> args;

	for (list<XMLObject>::const_iterator
			iter = xml.children().begin() ;
			iter != xml.children().end() ;
			iter++)
	{
		try {
			Variable var(*iter);
			args.insert(pair<String, Variable>(var.name(), var));
		} catch ( ... ) {}
	}

	return args;
}

void
insert_vars(const VarMap& vars, XMLObject& xml)
{
	for (VarMap::const_iterator
			iter = vars.begin() ;
			iter != vars.end() ;
			iter++)
	{
		xml.add_child(iter->second.xml());
	}
}

VarMap
list_APIs(const VarMap& args)
{
	list<String> apis;
	for (ApiFcnMap::const_iterator
			iter = _api_fcns.begin() ;
			iter != _api_fcns.end() ;
			iter++)
	{
		apis.push_back(iter->first);
	}

	Variable api_var("APIs", apis);
	VarMap ret;
	ret.insert(pair<String, Variable>(api_var.name(), api_var));
	return ret;
}


// ################ ModuleDriver ######################

#include <fcntl.h>

static void close_fd(int fd);
static int __stdin_out_module_driver(Module& module);


int
stdin_out_module_driver(Module& module, int argc, char** argv)
{
	bool display_err = false;
	int rv;

	while ((rv = getopt(argc, argv, "e")) != EOF) {
		switch (rv) {
			case 'e':
				display_err = true;
				break;
			default:
				break;
		}
	}

	int old_err = -1;
	if (!display_err) {
		// redirect stderr to /dev/null
		old_err = dup(2);
		int devnull = open("/dev/null", O_RDWR);
		if (devnull == -1) {
			perror("stdin_out_module_driver(): Can't open /dev/null");
			exit(1);
		}
		dup2(devnull, 2);
		close_fd(devnull);
	}

	try {
		return __stdin_out_module_driver(module);
	} catch ( ... ) {
		if (!display_err) {
			// restore stderr
			dup2(old_err, 2);
			close_fd(old_err);
		}
		throw;
	}
}

int
__stdin_out_module_driver(Module& module)
{
	unsigned int time_beg = time_mil();
	String data;

	while (time_mil() < time_beg + timeout) {
		poll_fd poll_data;
		poll_data.fd = 0;
		poll_data.events = POLLIN;
		poll_data.revents = 0;

		// wait for events
		int ret = poll(&poll_data, 1, 500);

		if (ret == 0) {
			/*
			** We may be done if the total input length is a multiple of
			** the buffer size.
			*/
			if (data.length() > 0) {
				try {
					XMLObject request = parseXML(data);
					XMLObject response = module.process(request);
					cout << generateXML(response) << endl;
					return 0;
				} catch ( ... ) { }
			}
			// continue waiting
			continue;
		} else if (ret == -1) {
			if (errno == EINTR)
				continue;
			else
				throw String("poll() error: ") + String(strerror(errno));
		}

		// process event
		if (poll_data.revents & POLLIN) {
			char buff[4096];
			int ret;

			ret = read_restart(poll_data.fd, buff, sizeof(buff));
			if (ret < 0)
				throw String("error reading stdin: ") + String(strerror(-ret));

			if (ret > 0) {
				data.append(buff, ret);
				memset(buff, 0, sizeof(buff));
			}

			if ((size_t) ret < sizeof(buff)) {
				try {
					XMLObject request = parseXML(data);
					XMLObject response = module.process(request);
					cout << generateXML(response) << endl;
					return 0;
				} catch ( ... ) { }
			}
			continue;
		}

		if (poll_data.revents & (POLLERR | POLLHUP | POLLNVAL))
			throw String("stdin error: ") + String(strerror(errno));
	}

	throw String("invalid input");
}

void
close_fd(int fd)
{
	int e;
	do {
		e = close(fd);
	} while (e && (errno == EINTR));
}
