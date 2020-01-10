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
 * Author: Ryan McCabe <rmccabe@redhat.com>
 */


#include "ClusterModule.h"
#include "ClusterConf.h"
#include "ClusterStatus.h"
#include "Clusvcadm.h"
#include "Fence.h"
#include "XVM.h"
#include "base64.h"

#include <stdio.h>

using namespace std;


static VarMap get_cluster_conf(const VarMap& args);
static VarMap set_cluster_conf(const VarMap& args);
static VarMap set_cluster_version(const VarMap& args);
static VarMap cluster_status(const VarMap& args);
static VarMap service_start(const VarMap& args);
static VarMap service_migrate(const VarMap& args);
static VarMap service_stop(const VarMap& args);
static VarMap service_restart(const VarMap& args);
static VarMap fence_node(const VarMap& args);
static VarMap start_node(const VarMap& args);
static VarMap stop_node(const VarMap& args);

static VarMap virt_guest(const VarMap& args);
static VarMap delete_xvm_key(const VarMap& args);
static VarMap set_xvm_key(const VarMap& args);
static VarMap get_xvm_key(const VarMap& args);
static VarMap generate_xvm_key(const VarMap& args);

static ApiFcnMap build_fcn_map();


ClusterModule::ClusterModule() :
	Module(build_fcn_map())
{}

ClusterModule::~ClusterModule()
{}


ApiFcnMap
build_fcn_map()
{
	FcnMap	api_1_0;

	api_1_0["get_cluster.conf"]		= get_cluster_conf;
	api_1_0["set_cluster.conf"]		= set_cluster_conf;
	api_1_0["set_cluster_version"]	= set_cluster_version;

	api_1_0["status"]				= cluster_status;

	api_1_0["start_service"]		= service_start;
	api_1_0["stop_service"]			= service_stop;
	api_1_0["restart_service"]		= service_restart;
	api_1_0["migrate_service"]		= service_migrate;

	api_1_0["start_node"]			= start_node;
	api_1_0["stop_node"]			= stop_node;
	api_1_0["fence_node"]			= fence_node;


	api_1_0["delete_xvm_key"]		= delete_xvm_key;
	api_1_0["set_xvm_key"]			= set_xvm_key;
	api_1_0["get_xvm_key"]			= get_xvm_key;
	api_1_0["generate_xvm_key"]		= generate_xvm_key;
	api_1_0["virt_guest"]			= virt_guest;


	ApiFcnMap api_fcn_map;
	api_fcn_map["1.0"] = api_1_0;

	return api_fcn_map;
}

VarMap
cluster_status(const VarMap& args)
{
	Variable var("status", Cluster::status());

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
get_cluster_conf(const VarMap& args)
{
	Variable var("cluster.conf", ClusterConf::get());

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
set_cluster_conf(const VarMap& args)
{
	XMLObject conf;
	bool propagate;

	try {
		VarMap::const_iterator iter = args.find("cluster.conf");
		if (iter == args.end())
			throw APIerror("missing cluster.conf variable");
		conf = iter->second.get_XML();

		propagate = false;
		iter = args.find("propagate");
		if (iter != args.end())
			propagate = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	ClusterConf::set(conf, propagate);
	VarMap ret;
	return ret;
}

VarMap
set_cluster_version(const VarMap& args)
{
	try {
		VarMap::const_iterator iter = args.find("version");

		if (iter != args.end()) {
			int conf_version = iter->second.get_int();
			ClusterConf::set_version(conf_version);
		} else {
			throw String("No cluster version was specified");
		}
	} catch ( String e ) {
		throw APIerror(e);
	}
	VarMap ret;
	return ret;
}

VarMap
service_stop(const VarMap& args)
{
	String name;

	try {
		VarMap::const_iterator iter = args.find("servicename");
		if (iter == args.end())
			throw APIerror("missing servicename variable");
		name = iter->second.get_string();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Clusvcadm::stop(name);
	VarMap ret;
	return ret;
}

VarMap
service_start(const VarMap& args)
{
	String service_name, node_name;

	try {
		VarMap::const_iterator iter = args.find("servicename");
		if (iter == args.end())
			throw APIerror("missing servicename variable");
		service_name = iter->second.get_string();

		iter = args.find("nodename");
		if (iter != args.end())
			node_name = iter->second.get_string();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Clusvcadm::start(service_name, node_name);
	VarMap ret;
	return ret;
}

VarMap
service_migrate(const VarMap& args)
{
	String service_name, node_name;
	try {
		VarMap::const_iterator iter = args.find("servicename");
		if (iter == args.end())
			throw APIerror("missing servicename variable");
		service_name = iter->second.get_string();

		iter = args.find("nodename");
		if (iter != args.end())
			node_name = iter->second.get_string();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Clusvcadm::migrate(service_name, node_name);
	VarMap ret;
	return ret;
}

VarMap
service_restart(const VarMap& args)
{
	String name;
	try {
		VarMap::const_iterator iter = args.find("servicename");
		if (iter == args.end())
			throw APIerror("missing servicename variable");
		name = iter->second.get_string();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Clusvcadm::restart(name);
	VarMap ret;
	return ret;
}

VarMap
virt_guest(const VarMap& args)
{
	Variable var("virt_guest", XVM::virt_guest());

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
delete_xvm_key(const VarMap& args) {
	XVM::delete_xvm_key();
	VarMap ret;
	return ret;
}

VarMap
set_xvm_key(const VarMap& args) {
	String key_base64;

	try {
		VarMap::const_iterator iter = args.find("key_base64");

		if (iter != args.end()) {
			key_base64 = iter->second.get_string();
		} else {
			throw String("missing key_base64 variable");
		}
	} catch ( String e ) {
		throw APIerror(e);
	}

	XVM::set_xvm_key(key_base64.c_str());
	VarMap ret;
	return ret;
}

VarMap
get_xvm_key(const VarMap& args) {
	char *key_base64 = NULL;

	key_base64 = XVM::get_xvm_key();
	Variable var("key_base64", String(key_base64));
	memset(key_base64, 0, strlen(key_base64));
	free(key_base64);

	VarMap ret;
	ret.insert(pair<String, Variable>(var.name(), var));
	return ret;
}

VarMap
generate_xvm_key(const VarMap& args) {
	size_t key_bytes = XVM_KEY_DEFAULT_SIZE;

	try {
		VarMap::const_iterator iter = args.find("size");

		if (iter != args.end()) {
			int bytes = iter->second.get_int();

			if (bytes < XVM_KEY_MIN_SIZE) {
				char err[64];
				snprintf(err, sizeof(err),
					"The minimum fence_xvm key size is %u bytes",
					XVM_KEY_MIN_SIZE);
				throw String(err);
			}

			if (bytes > XVM_KEY_MAX_SIZE) {
				char err[64];
				snprintf(err, sizeof(err),
					"The maximum fence_xvm key size is %u bytes",
					XVM_KEY_MAX_SIZE);
				throw String(err);
			}
			key_bytes = (size_t) bytes;
		}
	} catch ( String e ) {
		throw APIerror(e);
	}

	XVM::generate_xvm_key(key_bytes);
	VarMap ret;
	return ret;
}

VarMap
fence_node(const VarMap& args)
{
	String name;

	try {
		VarMap::const_iterator iter = args.find("nodename");
		if (iter == args.end())
			throw APIerror("missing nodename variable");
		name = iter->second.get_string();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Fence::fence_node(name);
	VarMap ret;
	return ret;
}

VarMap
start_node(const VarMap& args)
{
	bool cluster_startup = false;
	bool enable_services = true;

	try {
		VarMap::const_iterator iter = args.find("cluster_startup");
		if (iter != args.end())
			cluster_startup = iter->second.get_bool();

		iter = args.find("enable_services");
		if (iter != args.end())
			enable_services = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Cluster::start_node(cluster_startup, enable_services);

	VarMap ret;
	return ret;
}

VarMap
stop_node(const VarMap& args)
{
	bool cluster_shutdown = false;
	bool purge_conf = false;
	bool disable_services = true;

	try {
		VarMap::const_iterator iter = args.find("cluster_shutdown");
		if (iter != args.end())
			cluster_shutdown = iter->second.get_bool();

		iter = args.find("purge_conf");
		if (iter != args.end())
			purge_conf = iter->second.get_bool();

		iter = args.find("disable_services");
		if (iter != args.end())
			disable_services = iter->second.get_bool();
	} catch ( String e ) {
		throw APIerror(e);
	}

	Cluster::stop_node(cluster_shutdown, purge_conf, disable_services);
	VarMap ret;
	return ret;
}
