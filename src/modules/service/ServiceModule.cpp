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

#include "ServiceModule.h"
#include "ServiceManager.h"

using namespace std;

static VarMap enable(const VarMap& args);
static VarMap disable(const VarMap& args);
static VarMap start(const VarMap& args);
static VarMap restart(const VarMap& args);
static VarMap stop(const VarMap& args);
static VarMap lists(const VarMap& args);
static VarMap query(const VarMap& args);

static ApiFcnMap build_fcn_map();

ServiceModule::ServiceModule() :
	Module(build_fcn_map())
{}

ServiceModule::~ServiceModule()
{}

ApiFcnMap
build_fcn_map()
{
	FcnMap api_1_0;

	api_1_0["enable"] = enable;
	api_1_0["disable"] = disable;
	api_1_0["start"] = start;
	api_1_0["restart"] = restart;
	api_1_0["stop"] = stop;
	api_1_0["list"] = lists;
	api_1_0["query"] = query;

	ApiFcnMap api_fcn_map;
	api_fcn_map["1.0"] = api_1_0;

	return api_fcn_map;
}

VarMap
enable(const VarMap& args)
{
	list<XMLObject> serv_list;

	try {
		VarMap::const_iterator iter = args.find("services");
		if (iter == args.end())
			throw APIerror("missing services variable");
		serv_list = iter->second.get_list_XML();
	} catch ( String e ) {
		throw APIerror(e);
	}

	list<String> services, sets;
	for (list<XMLObject>::const_iterator
			iter = serv_list.begin() ;
			iter != serv_list.end() ;
			iter++)
	{
		if (iter->tag() == "service")
			services.push_back(iter->get_attr("name"));
		else if (iter->tag() == "set")
			sets.push_back(iter->get_attr("name"));
	}

	ServiceManager().enable(services, sets);
	return VarMap();
}

VarMap
disable(const VarMap& args)
{
	list<XMLObject> serv_list;

	try {
		VarMap::const_iterator iter = args.find("services");
		if (iter == args.end())
			throw APIerror("missing services variable");
		serv_list = iter->second.get_list_XML();
	} catch ( String e ) {
		throw APIerror(e);
	}

	list<String> services, sets;
	for (list<XMLObject>::const_iterator
			iter = serv_list.begin() ;
			iter != serv_list.end() ;
			iter++)
	{
		if (iter->tag() == "service")
			services.push_back(iter->get_attr("name"));
		else if (iter->tag() == "set")
			sets.push_back(iter->get_attr("name"));
	}

	ServiceManager().disable(services, sets);
	return VarMap();
}

VarMap
start(const VarMap& args)
{
	list<XMLObject> serv_list;

	try {
		VarMap::const_iterator iter = args.find("services");
		if (iter == args.end())
			throw APIerror("missing services variable");
		serv_list = iter->second.get_list_XML();
	} catch ( String e ) {
		throw APIerror(e);
	}

	list<String> services, sets;
	for (list<XMLObject>::const_iterator
			iter = serv_list.begin() ;
			iter != serv_list.end() ;
			iter++)
	{
		if (iter->tag() == "service")
			services.push_back(iter->get_attr("name"));
		else if (iter->tag() == "set")
			sets.push_back(iter->get_attr("name"));
	}

	ServiceManager().start(services, sets);
	return VarMap();
}

VarMap
restart(const VarMap& args)
{
	list<XMLObject> serv_list;

	try {
		VarMap::const_iterator iter = args.find("services");
		if (iter == args.end())
			throw APIerror("missing services variable");
		serv_list = iter->second.get_list_XML();
	} catch ( String e ) {
		throw APIerror(e);
	}

	list<String> services, sets;
	for (list<XMLObject>::const_iterator
			iter = serv_list.begin() ;
			iter != serv_list.end() ;
			iter++)
	{
		if (iter->tag() == "service")
			services.push_back(iter->get_attr("name"));
		else if (iter->tag() == "set")
			sets.push_back(iter->get_attr("name"));
	}

	ServiceManager().restart(services, sets);
	return VarMap();
}

VarMap
stop(const VarMap& args)
{
	list<XMLObject> serv_list;

	try {
		VarMap::const_iterator iter = args.find("services");
		if (iter == args.end())
			throw APIerror("missing services variable");
		serv_list = iter->second.get_list_XML();
	} catch ( String e ) {
		throw APIerror(e);
	}

	list<String> services, sets;
	for (list<XMLObject>::const_iterator
			iter = serv_list.begin() ;
			iter != serv_list.end() ;
			iter++)
	{
		if (iter->tag() == "service")
			services.push_back(iter->get_attr("name"));
		else if (iter->tag() == "set")
			sets.push_back(iter->get_attr("name"));
	}

	ServiceManager().stop(services, sets);
	return VarMap();
}

VarMap
lists(const VarMap& args)
{
	bool descr;

	try {
		descr = false;
		VarMap::const_iterator iter = args.find("description");

		if (iter != args.end())
			descr = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	// command
	list<XMLObject> xml_list;

	list<Service> services;
	list<ServiceSet> sets;
	ServiceManager().lists(services, sets);

	for (list<Service>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		xml_list.push_back(iter->xml(descr));
	}

	for (list<ServiceSet>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		xml_list.push_back(iter->xml(descr));
	}

	// response
	Variable var("services", xml_list);
	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
query(const VarMap& args)
{
	list<XMLObject> search_list;
	bool descr;

	try {
		VarMap::const_iterator iter = args.find("search");
		if (iter == args.end())
			throw APIerror("missing search variable");
		search_list = iter->second.get_list_XML();

		descr = false;
		iter = args.find("description");
		if (iter != args.end())
			descr = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	// command
	list<String> serv_names, set_names;
	for (list<XMLObject>::const_iterator
			iter = search_list.begin() ;
			iter != search_list.end() ;
			iter++)
	{
		if (iter->tag() == "service")
			serv_names.push_back(iter->get_attr("name"));
		else if (iter->tag() == "set")
			set_names.push_back(iter->get_attr("name"));
	}

	list<Service> services;
	list<ServiceSet> sets;
	ServiceManager().query(serv_names, set_names, services, sets);

	// response
	list<XMLObject> result_list;
	for (list<Service>::const_iterator
			iter = services.begin() ;
			iter != services.end() ;
			iter++)
	{
		result_list.push_back(iter->xml(descr));
	}

	for (list<ServiceSet>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		result_list.push_back(iter->xml(descr));
	}

	Variable var("result", result_list);

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}
