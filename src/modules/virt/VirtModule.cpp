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
 * Author: Ryan McCabe <rmccabe@redhat.com>
 */

#include "VirtModule.h"
#include "Virt.h"
#include "base64.h"

using namespace std;

static VarMap virt_guest(const VarMap& args);
static VarMap list_vm(const VarMap& args);

static ApiFcnMap build_fcn_map();

VirtModule::VirtModule() :
	Module(build_fcn_map())
{}

VirtModule::~VirtModule()
{}

ApiFcnMap
build_fcn_map()
{
	FcnMap	api_1_0;

	api_1_0["virt_guest"]			= virt_guest;
	api_1_0["list_vm"]				= list_vm;

	ApiFcnMap api_fcn_map;
	api_fcn_map["1.0"] = api_1_0;

	return api_fcn_map;
}

VarMap
virt_guest(const VarMap& args)
{
	Variable var("virt_guest", Virt::virt_guest());

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
list_vm(const VarMap& args)
{
	String hypervisor_uri = "";

	try {
		VarMap::const_iterator iter = args.find("hypervisor_uri");

		if (iter != args.end())
			hypervisor_uri = iter->second.get_string();
	} catch (String e) {
		throw APIerror(e);
	}

	if (!hypervisor_uri.size())
		hypervisor_uri = String(DEFAULT_HV_URI);

	std::map<String, String> vm_list = Virt::get_vm_list(hypervisor_uri);

	list<XMLObject> ids_list;
	for (	map<String,String>::iterator iter = vm_list.begin() ;
			iter != vm_list.end();
			iter++)
	{
		XMLObject id_xml("vm");
		id_xml.set_attr("domain", String(iter->first));
		id_xml.set_attr("status", String(iter->second));
		ids_list.push_back(id_xml);
	}

	Variable var("vm_list", ids_list);
	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}
