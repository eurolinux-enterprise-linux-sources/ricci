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

#include "RpmModule.h"
#include "PackageHandler.h"

using namespace std;

static VarMap lists(const VarMap& args);
static VarMap query(const VarMap& args);
static VarMap repository_configured(const VarMap& args);
static VarMap install(const VarMap& args);

static ApiFcnMap build_fcn_map();

RpmModule::RpmModule() :
	Module(build_fcn_map())
{}

RpmModule::~RpmModule()
{}

ApiFcnMap
build_fcn_map()
{
	FcnMap api_1_0;

	api_1_0["list"] = lists;
	api_1_0["query"] = query;
	api_1_0["install"] = install;
	api_1_0["repository_configured"] = repository_configured;

	ApiFcnMap api_fcn_map;
	api_fcn_map["1.0"] = api_1_0;
	return api_fcn_map;
}


VarMap
install(const VarMap& args)
{
	list<XMLObject> rpms_list, sets_list;
	bool upgrade;

	try {
		VarMap::const_iterator iter = args.find("rpms");
		if (iter != args.end())
			rpms_list = iter->second.get_list_XML();

		iter = args.find("sets");
		if (iter != args.end())
			sets_list = iter->second.get_list_XML();

		upgrade = true;
		iter = args.find("upgrade");
		if (iter != args.end())
			upgrade = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	// command
	list<Package> rpms;
	for (list<XMLObject>::const_iterator
		iter = rpms_list.begin() ;
		iter != rpms_list.end() ;
		iter++)
	{
		if (iter->tag() == "rpm") {
			String name(iter->get_attr("name"));
			if (name.size()) {
				Package pack(name);
				rpms.push_back(pack);
			}
		}
	}

	list<PackageSet> sets;
	for (list<XMLObject>::const_iterator
			iter = sets_list.begin() ;
			iter != sets_list.end() ;
			iter++)
	{
		if (iter->tag() == "set") {
			String name(iter->get_attr("name"));
			if (name.size()) {
				PackageSet set(name);
				sets.push_back(set);
			}
		}
	}

	PackageHandler::install(rpms, sets, upgrade);
	return VarMap();
}

VarMap
repository_configured(const VarMap& args)
{
	Variable var("repository_configured", PackageHandler::repo_available());

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
lists(const VarMap& args)
{
	bool rpms, sets, installed, installable, upgradeable;

	try {
		rpms = false;
		VarMap::const_iterator iter = args.find("rpms");

		if (iter != args.end())
			rpms = iter->second.get_bool();

		sets = false;
		iter = args.find("sets");
		if (iter != args.end())
			sets = iter->second.get_bool();

		installed = false;
		iter = args.find("installed");

		if (iter != args.end())
			installed = iter->second.get_bool();

		installable = false;
		iter = args.find("installable");
		if (iter != args.end())
			installable = iter->second.get_bool();

		upgradeable = false;
		iter = args.find("upgradeable");
		if (iter != args.end())
			upgradeable = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	VarMap ret;

	if (sets || rpms) {
		PackageHandler handler;

		if (rpms) {
			list<XMLObject> rpm_list;
			for (map<String, Package>::const_iterator
				iter = handler.packages().begin() ;
				iter != handler.packages().end() ;
				iter++)
			{
				const Package& pack = iter->second;
				bool add = false;

				if (installed) {
					if (pack.version.size())
						add = true;
				}

				if (installable) {
					if (pack.repo_version > pack.version &&
						pack.version.empty())
					{
						add = true;
					}
				}

				if (upgradeable) {
					if (pack.repo_version > pack.version &&
						pack.version.size())
					{
						add = true;
					}
				}

				if (add)
					rpm_list.push_back(pack.xml());
			}

			Variable var("rpms", rpm_list);
			ret.insert(pair<String, Variable>(var.name(), var));
		}

		if (sets) {
			list<XMLObject> set_list;
			for (map<String, PackageSet>::const_iterator
					iter = handler.sets().begin() ;
					iter != handler.sets().end() ;
					iter++)
			{
				const PackageSet& set = iter->second;
				bool add = false;

				if (installed) {
					if (set.installed)
						add = true;
				}

				if (installable) {
					if (set.in_repo)
						add = true;
				}

				if (upgradeable) {
					if (set.installed && set.upgradeable)
						add = true;
				}

				if (add)
					set_list.push_back(set.xml());
			}

			Variable var("sets", set_list);
			ret.insert(pair<String, Variable>(var.name(), var));
		}
	}

	return ret;
}

VarMap
query(const VarMap& args)
{
	list<XMLObject> search_list;

	try {
		VarMap::const_iterator iter = args.find("search");
		if (iter == args.end())
			throw APIerror("missing search variable");
		search_list = iter->second.get_list_XML();
	} catch ( String e ) {
		throw APIerror(e);
	}

	list<XMLObject> result_list;
	PackageHandler handler;
	for (list<XMLObject>::const_iterator
			iter = search_list.begin() ;
			iter != search_list.end() ;
			iter++)
	{
		if (iter->tag() == "rpm") {
			String name(iter->get_attr("name"));
			if (name.size()) {
				Package& pack = handler.packages()[name];
				pack.name = name;
				result_list.push_back(pack.xml());
			}
		} else if (iter->tag() == "set") {
			String name(iter->get_attr("name"));
			if (name.size()) {
				PackageSet& set = handler.sets()[name];
				set.name = name;
				result_list.push_back(set.xml());
			}
		}
	}

	Variable var("result", result_list);
	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}
