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

#include "LoggingModule.h"
#include "LogParser.h"
#include "utils.h"

using namespace std;

static VarMap get(const VarMap& args);

static ApiFcnMap build_fcn_map();

LoggingModule::LoggingModule() :
	Module(build_fcn_map())
{}

LoggingModule::~LoggingModule()
{}

ApiFcnMap
build_fcn_map()
{
	FcnMap api_1_0;
	api_1_0["get"] = get;

	ApiFcnMap api_fcn_map;
	api_fcn_map["1.0"] = api_1_0;

	return api_fcn_map;
}

VarMap
get(const VarMap& args)
{
	long long age;
	list<String> tags;
	list<String> paths;
	bool intersection = false;

	try {
		VarMap::const_iterator iter = args.find("age");
		if (iter == args.end())
			throw APIerror("missing age variable");
		age = iter->second.get_int();

		iter = args.find("tags");
		if (iter != args.end())
			tags = iter->second.get_list_str();

		iter = args.find("paths");
		if (iter != args.end())
			paths = iter->second.get_list_str();

		iter = args.find("intersection");
		if (iter != args.end())
			intersection = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	// clean up

	list<String> t;
	for (list<String>::const_iterator
		iter = tags.begin() ;
		iter != tags.end() ;
		iter++)
	{
		String s(utils::strip(utils::to_lower(*iter)));
		if (s.size())
			t.push_back(s);
	}

	tags.swap(t);
	t.clear();

	for (list<String>::const_iterator
			iter = paths.begin() ;
			iter != paths.end() ;
			iter++)
	{
		String s(utils::strip(*iter));
		if (s.size())
			t.push_back(s);
	}
	paths.swap(t);

	// command
	set<LogEntry> entries = LogParser().get_entries(age, tags, paths);

	// union or intersection?
	if (intersection) {
		// LogParser::get returns union
		set<LogEntry> intersect;

		for (set<LogEntry>::const_iterator
				iter = entries.begin() ;
				iter != entries.end() ;
				iter++)
		{
			if (iter->matched_tags.size() == tags.size())
				intersect.insert(*iter);
		}
		entries.swap(intersect);
	}

	// response
	list<XMLObject> result_list;
	for (set<LogEntry>::const_iterator
			iter = entries.begin() ;
			iter != entries.end() ;
			iter++)
	{
		result_list.push_back(iter->xml());
	}

	Variable var("log_entries", result_list);

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}
