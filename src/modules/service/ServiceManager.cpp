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

#include "ServiceManager.h"
#include "utils.h"
#include "File.h"

#include <vector>

using namespace std;

#define CHKCONFIG_PATH		"/sbin/chkconfig"
#define SERVICE_PATH		"/sbin/service"

#define INITD_DIR_PATH		"/etc/init.d/"

#define DESC_SIG			String("# description:")

Service::Service()
{
}

Service::Service(const String& name, bool enabled) :
	_name(counting_auto_ptr<String>(new String(name))),
	_enabled(counting_auto_ptr<bool>(new bool(enabled)))
{
	if (_name->empty())
		throw String("no service name given");
}

Service::~Service()
{}

XMLObject
Service::xml(bool descr) const
{
	if (!_name.get())
		throw String("internal: service not initialized");

	XMLObject xml("service");
	xml.set_attr("name", name());
	xml.set_attr("enabled", enabled() ? "true" : "false");
	xml.set_attr("running", running() ? "true" : "false");
	xml.set_attr("description", descr ? description() : "");

	return xml;
}

String
Service::name() const
{
	if (!_name.get())
		throw String("internal: service not initialized");
	return *_name;
}

bool
Service::enabled() const
{
	if (!_enabled.get())
		throw String("internal: service not initialized");
	return *_enabled;
}

bool
Service::running() const
{
	if (!_running.get())
		_running = counting_auto_ptr<bool>(new bool(service_running(name())));
	return *_running;
}

String
Service::description() const
{
	if (!_descr.get()) {
		String path(INITD_DIR_PATH);
		path += name();

		String initd(File::open(path));

		list<String> desc_lines;

		vector<String> lines = utils::split(initd, "\n");
		for (vector<String>::const_iterator
				iter = lines.begin() ;
				iter != lines.end() ;
				iter++)
		{
			String line(utils::strip(*iter));
			if (line.empty())
				continue;
			if (line.find(DESC_SIG) != 0)
				continue;

			desc_lines.push_back(line);
			while (desc_lines.back()[desc_lines.back().size() - 1] == '\\' && ++iter != lines.end())
			{
				if (iter->size())
					desc_lines.push_back(*iter);
				else
					break;
			}

			break;
		}

		String desc;
		for (list<String>::const_iterator
				l_iter = desc_lines.begin() ;
				l_iter != desc_lines.end() ;
				l_iter++)
		{
			String s = utils::rstrip(*l_iter, "\\");
			s = utils::lstrip(s, DESC_SIG);
			s = utils::lstrip(s, "#");
			s = utils::lstrip(s);
			desc += s;
		}

		_descr = counting_auto_ptr<String>(new String(desc));
	}

	return *_descr;
}

void
Service::enable()
{
	if (!enabled()) {
		enable_service(name(), true);
		*_enabled = true;
	}
}

void
Service::disable()
{
	if (enabled()) {
		enable_service(name(), false);
		*_enabled = false;
	}
}

void
Service::start()
{
	running();
	run_service(name(), START);
	*_running = true;
}

void
Service::restart()
{
	running();
	run_service(name(), RESTART);
	*_running = true;
}

void
Service::stop()
{
	running();
	run_service(name(), STOP);
	*_running = false;
}

void
Service::enable_service(const String& name, bool on)
{
	String out, err;
	int status;
	vector<String> args;

	args.push_back(name);

	if (on)
		args.push_back("on");
	else
		args.push_back("off");

	if (utils::execute(CHKCONFIG_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(CHKCONFIG_PATH);
	if (status != 0)
		throw String("chkconfig failed for service ") + name + ": " + err;
}

bool
Service::service_running(const String& name)
{
	String path(INITD_DIR_PATH);
	path += name;

	String out, err;
	int status;
	vector<String> args;

	args.push_back("status");
	if (utils::execute(path, args, out, err, status, false) != 0)
		throw command_not_found_error_msg(path);
	return status == 0;
}

void
Service::run_service(const String& name, ActionState state)
{
	String path(INITD_DIR_PATH);
	path += name;

	String out, err;
	int status;
	vector<String> args;

	switch (state) {
		case START:
			args.push_back("start");
			break;

		case STOP:
			args.push_back("stop");
			break;

		case RESTART:
			args.push_back("restart");
			break;
	}

	if (utils::execute(path, args, out, err, status, false) != 0)
		throw command_not_found_error_msg(path);

	if (status) {
		bool running = service_running(name);
		if (state == START || state == RESTART) {
			if (!running) {
				throw String("service ") + name + " "
						+ String(state == START ? "start" : "restart")
						+ " failed: " + err;
			}
		} else {
			if (running)
				throw String("service ") + name + " stop failed: " + err;
		}
	}
}

ServiceSet::ServiceSet()
{
}

ServiceSet::ServiceSet(const String& name, const String& description) :
	_name(counting_auto_ptr<String>(new String(name))),
	_descr(counting_auto_ptr<String>(new String(description)))
{
	if (_name->empty())
		throw String("no ServiceSet name");
}

ServiceSet::~ServiceSet()
{}

XMLObject
ServiceSet::xml(bool descr) const
{
	XMLObject xml("set");
	xml.set_attr("name", name());
	xml.set_attr("enabled", enabled() ? "true" : "false");
	xml.set_attr("running", running() ? "true" : "false");
	xml.set_attr("description", descr ? description() : "");
	return xml;
}

String
ServiceSet::name() const
{
	if (!_name.get() || servs.empty())
		throw String("internal: ServiceSet not initialized");
	return *_name;
}

bool
ServiceSet::enabled() const
{
	name();

	for (list<Service>::const_iterator
			iter = servs.begin() ;
			iter != servs.end() ;
			iter++)
	{
		if (!iter->enabled())
			return false;
	}

	return true;
}

bool
ServiceSet::running() const
{
	name();

	for (list<Service>::const_iterator
			iter = servs.begin() ;
			iter != servs.end() ;
			iter++)
	{
		if (!iter->running())
			return false;
	}
	
	return true;
}

String
ServiceSet::description() const
{
	name();
	return *_descr;
}

void
ServiceSet::enable()
{
	name();

	try {
		for (list<Service>::iterator
				iter = servs.begin() ;
				iter != servs.end() ;
				iter++)
		{
			iter->enable();
		}
	} catch (String e) {
		throw String("service set '") + name() + "' failed to enable: " + e;
	}
}

void
ServiceSet::disable()
{
	name();

	try {
		for (list<Service>::iterator
				iter = servs.begin() ;
				iter != servs.end() ;
				iter++)
		{
			iter->disable();
		}
	} catch (String e) {
		throw String("service set '") + name() + "' failed to disable: " + e;
	}
}

void
ServiceSet::start()
{
	name();

	try {
		for (list<Service>::iterator
				iter = servs.begin() ;
				iter != servs.end() ;
				iter++)
		{
			iter->start();
		}
	} catch (String e) {
		throw String("service set '") + name() + "' failed to start: " + e;
	}
}

void
ServiceSet::restart()
{
	name();

	try {
		// ordered sequence: last started, first to be stoped
		stop();
		start();
	} catch (String e) {
		throw String("service set '") + name() + "' failed to restart: " + e;
	}
}

void
ServiceSet::stop()
{
	name();

	try {
		for (list<Service>::reverse_iterator
				iter = servs.rbegin() ;
				iter != servs.rend() ;
				iter++)
		{
			iter->stop();
		}
	} catch (String e) {
		throw String("service set '") + name() + "' failed to stop: " + e;
	}
}

ServiceManager::ServiceManager()
{
	String out, err;
	int status;
	vector<String> args;

	args.push_back("--list");
	if (utils::execute(CHKCONFIG_PATH, args, out, err, status, true))
		throw command_not_found_error_msg(CHKCONFIG_PATH);
	if (status)
		throw String("chkconfig failed: " + err);

	vector<String> lines = utils::split(out, "\n");
	for (vector<String>::const_iterator
			iter = lines.begin() ;
			iter != lines.end() ;
			iter++)
	{
		vector<String> words = utils::split(utils::strip(*iter));
		if (words.size() != 8)
			continue;

		String name = words[0];
		bool enabled = false;
		for (vector<String>::size_type i = 2; i < words.size() - 1; i++) {
			if (words[i].find("on") != String::npos)
				enabled = true;
		}
		_servs[name] = Service(name, enabled);
	}

	_sets = generate_sets();
}

ServiceManager::~ServiceManager()
{}

map<String, ServiceSet>
ServiceManager::generate_sets()
{
	map<String, ServiceSet> sets;

	list<String> servs;
	String name = "Cluster Base";
	String descr = "Cluster infrastructure: cman";

	servs.push_back("cman");

	ServiceSet s(name, descr);
	if (populate_set(s, servs))
		sets[name] = s;

	servs.clear();
	name = "Cluster Service Manager";
	descr = "Cluster Service Manager: rgmanager";

	s = ServiceSet(name, descr);
	servs.push_back("rgmanager");
	if (populate_set(s, servs))
		sets[name] = s;

	servs.clear();
	name = "Clustered Storage";
	descr = "Shared Storage: clvmd, gfs2";
	servs.push_back("clvmd");
	servs.push_back("gfs2");
	servs.push_back("scsi_reserve");

	s = ServiceSet(name, descr);
	if (populate_set(s, servs))
		sets[name] = s;

	servs.clear();
	name = "Linux Virtual Server";
	descr = "Red Hat's LVS implementation: pulse, piranha";
	s = ServiceSet(name, descr);
	servs.push_back("pulse");
	servs.push_back("piranha-gui");
	if (populate_set(s, servs))
		sets[name] = s;

	return sets;
}

bool
ServiceManager::populate_set(ServiceSet& ss, std::list<String> servs)
{
	for (list<String>::iterator
			n_iter = servs.begin() ;
			n_iter != servs.end() ;
			n_iter++)
	{
		if (_servs.find(*n_iter) == _servs.end())
			return false;
		else
			ss.servs.push_back(_servs[*n_iter]);
	}

	return true;
}


void
ServiceManager::enable(	const std::list<String>& services,
						const std::list<String>& sets)
{
	// check
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		if (_servs.find(*iter) == _servs.end())
			throw String("no such service: ") + *iter;
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		if (_sets.find(*iter) == _sets.end())
			throw String("no such service set: ") + *iter;
	}

	// apply
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		_servs[*iter].enable();
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		_sets[*iter].enable();
	}
}

void
ServiceManager::disable(const std::list<String>& services,
						const std::list<String>& sets)
{
	// check
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		if (_servs.find(*iter) == _servs.end())
			throw String("no such service: ") + *iter;
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		if (_sets.find(*iter) == _sets.end())
			throw String("no such service set: ") + *iter;
	}

	// apply
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		_servs[*iter].disable();
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		_sets[*iter].disable();
	}
}

void
ServiceManager::start(	const std::list<String>& services,
						const std::list<String>& sets)
{
	// check
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		if (_servs.find(*iter) == _servs.end())
			throw String("no such service: ") + *iter;
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		if (_sets.find(*iter) == _sets.end())
			throw String("no such service set: ") + *iter;
	}

	// apply
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		_servs[*iter].start();
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		_sets[*iter].start();
	}
}

void
ServiceManager::restart(const std::list<String>& services,
						const std::list<String>& sets)
{
	// check
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		if (_servs.find(*iter) == _servs.end())
			throw String("no such service: ") + *iter;
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		if (_sets.find(*iter) == _sets.end())
			throw String("no such service set: ") + *iter;
	}

	// apply
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		_servs[*iter].restart();
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		_sets[*iter].restart();
	}
}

void
ServiceManager::stop(	const std::list<String>& services,
						const std::list<String>& sets)
{
	// check
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		if (_servs.find(*iter) == _servs.end())
			throw String("no such service: ") + *iter;
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		if (_sets.find(*iter) == _sets.end())
			throw String("no such service set: ") + *iter;
	}

	// apply
	for (list<String>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		_servs[*iter].stop();
	}

	for (list<String>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		_sets[*iter].stop();
	}
}

void
ServiceManager::lists(	std::list<Service>& services,
						std::list<ServiceSet>& sets)
{
	services.clear();
	sets.clear();

	for (map<String, Service>::const_iterator
			iter = _servs.begin() ;
			iter != _servs.end() ;
			iter++)
	{
		services.push_back(iter->second);
	}

	for (map<String, ServiceSet>::const_iterator
			iter = _sets.begin() ;
			iter != _sets.end() ;
			iter++)
	{
		sets.push_back(iter->second);
	}
}

void
ServiceManager::query(	const std::list<String>& serv_names,
						const std::list<String>& set_names,
						std::list<Service>& services,
						std::list<ServiceSet>& sets)
{
	services.clear();
	sets.clear();

	for (list<String>::const_iterator
			iter = serv_names.begin() ;
			iter != serv_names.end() ;
			iter++)
	{
		if (_servs.find(*iter) != _servs.end())
			services.push_back(_servs[*iter]);
	}

	for (list<String>::const_iterator iter = set_names.begin() ;
			iter != set_names.end() ;
			iter++)
	{
		if (_sets.find(*iter) != _sets.end())
			sets.push_back(_sets[*iter]);
	}
}
