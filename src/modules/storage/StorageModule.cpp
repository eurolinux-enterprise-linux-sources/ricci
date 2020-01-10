/*
  Copyright Red Hat, Inc. 2006-2008

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to the
  Free Software Foundation, Inc.,  675 Mass Ave, Cambridge,
  MA 02139, USA.
*/
/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */


#include "StorageModule.h"
#include "MapperFactory.h"
#include "BDFactory.h"
#include "LVM.h"
#include "FSController.h"
#include "MountHandler.h"


using namespace std;


static list<XMLObject> _mapper_ids(const String& mapper_type);
static list<XMLObject> _mappers(const String& mapper_type, const String& mapper_id);
static list<XMLObject> _mapper_templates(const String& mapper_type);

static VarMap report(const VarMap& args);
static VarMap get_mapper_ids(const VarMap& args);
static VarMap get_mappers(const VarMap& args);
static VarMap get_mapper_templates(const VarMap& args);
static VarMap create_mapper(const VarMap& args);
static VarMap remove_mapper(const VarMap& args);
static VarMap add_sources(const VarMap& args);
static VarMap remove_source(const VarMap& args);
static VarMap modify_mapper(const VarMap& args);
static VarMap create_bd(const VarMap& args);
static VarMap get_bd(const VarMap& args);
static VarMap modify_bd(const VarMap& args);
static VarMap remove_bd(const VarMap& args);
static VarMap get_fs_group_members(const VarMap& args);
static VarMap mount_fs(const VarMap& args);
static VarMap umount_fs(const VarMap& args);
static VarMap fstab_add(const VarMap& args);
static VarMap fstab_del(const VarMap& args);

static VarMap enable_clustered_lvm(const VarMap& args);
static VarMap disable_clustered_lvm(const VarMap& args);

static ApiFcnMap build_fcn_map();


StorageModule::StorageModule() :
  Module(build_fcn_map())
{}

StorageModule::~StorageModule()
{}


ApiFcnMap
build_fcn_map()
{
  FcnMap   api_1_0;

  api_1_0["report"]                             = report;

  api_1_0["get_mapper_ids"]                     = get_mapper_ids;
  api_1_0["get_mappers"]                        = get_mappers;

  api_1_0["get_mapper_templates"]               = get_mapper_templates;
  api_1_0["create_mapper"]                      = create_mapper;
  api_1_0["modify_mapper"]                      = modify_mapper;
  api_1_0["remove_mapper"]                      = remove_mapper;
  api_1_0["add_mapper_sources"]                 = add_sources;
  api_1_0["remove_mapper_source"]               = remove_source;

  api_1_0["create_bd"]                          = create_bd;
  api_1_0["get_bd"]                             = get_bd;
  api_1_0["modify_bd"]                          = modify_bd;
  api_1_0["remove_bd"]                          = remove_bd;

  api_1_0["get_fs_group_members"]               = get_fs_group_members;

  api_1_0["enable_clustered_lvm"]               = enable_clustered_lvm;
  api_1_0["disable_clustered_lvm"]              = disable_clustered_lvm;
  api_1_0["mount_fs"]                           = mount_fs;
  api_1_0["umount_fs"]                          = umount_fs;
  api_1_0["fstab_add"]                          = fstab_add;
  api_1_0["fstab_del"]                          = fstab_del;

  ApiFcnMap   api_fcn_map;
  api_fcn_map["1.0"] = api_1_0;

  return api_fcn_map;
}


VarMap
report(const VarMap& args)
{
  Variable ids("mapper_ids", _mapper_ids(""));
  Variable mappers("mappers", _mappers("", ""));
  Variable temps("mapper_templates", _mapper_templates(""));

  VarMap ret;
  ret.insert(pair<String, Variable>(ids.name(), ids));
  ret.insert(pair<String, Variable>(mappers.name(), mappers));
  ret.insert(pair<String, Variable>(temps.name(), temps));
  return ret;
}

VarMap
get_mapper_ids(const VarMap& args)
{
  String mapper_type;
  try {
    VarMap::const_iterator iter = args.find("mapper_type");
    if (iter != args.end())
      mapper_type = iter->second.get_string();
  } catch ( String e ) {
    throw APIerror(e);
  }

  Variable var("mapper_ids", _mapper_ids(mapper_type));
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
get_mappers(const VarMap& args)
{
  String mapper_type, mapper_id;
  try {
    VarMap::const_iterator iter = args.find("mapper_type");
    if (iter != args.end())
      mapper_type = iter->second.get_string();

    iter = args.find("mapper_id");
    if (iter != args.end())
      mapper_id = iter->second.get_string();
  } catch ( String e ) {
    throw APIerror(e);
  }

  Variable var("mappers", _mappers(mapper_type, mapper_id));
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
get_mapper_templates(const VarMap& args)
{
  String mapper_type;
  try {
    VarMap::const_iterator iter = args.find("mapper_type");
    if (iter != args.end())
      mapper_type = iter->second.get_string();
  } catch ( String e ) {
    throw APIerror(e);
  }

  Variable var("mapper_templates", _mapper_templates(mapper_type));
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
create_mapper(const VarMap& args)
{
  XMLObject mapper_xml;
  try {
    VarMap::const_iterator iter = args.find("mapper");
    if (iter == args.end())
      throw APIerror("missing mapper variable");
    mapper_xml = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  MapperTemplate mapper_template(mapper_xml);
  counting_auto_ptr<Mapper> mapper = MapperFactory::create_mapper(mapper_template);

  Variable var("mapper", mapper->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
remove_mapper(const VarMap& args)
{
  XMLObject mapper_xml;
  try {
    VarMap::const_iterator iter = args.find("mapper");
    if (iter == args.end())
      throw APIerror("missing mapper variable");
    mapper_xml = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  MapperParsed mapper(mapper_xml);
  MapperFactory::remove_mapper(mapper);

  VarMap ret;
  return ret;
}

VarMap
modify_mapper(const VarMap& args)
{
  XMLObject mapper_xml;
  try {
    VarMap::const_iterator iter = args.find("mapper");
    if (iter == args.end())
      throw APIerror("missing mapper variable");
    mapper_xml = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  MapperParsed mp(mapper_xml);
  counting_auto_ptr<Mapper> mapper = MapperFactory::modify_mapper(mp);

  Variable var("mapper", mapper->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
add_sources(const VarMap& args)
{
  String mapper_type, mapper_id, mapper_state_ind;
  list<XMLObject> bds_list;
  try {
    VarMap::const_iterator iter = args.find("mapper_type");
    if (iter == args.end())
      throw APIerror("missing mapper_type variable");
    mapper_type = iter->second.get_string();

    iter = args.find("mapper_id");
    if (iter == args.end())
      throw APIerror("missing mapper_id variable");
    mapper_id = iter->second.get_string();

    iter = args.find("mapper_state_ind");
    if (iter == args.end())
      throw APIerror("missing mapper_state_ind variable");
    mapper_state_ind = iter->second.get_string();

    iter = args.find("bds");
    if (iter == args.end())
      throw APIerror("missing bds variable");
    bds_list = iter->second.get_list_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  list<BDParsed> parsed_bds;
  for (list<XMLObject>::const_iterator iter = bds_list.begin();
       iter != bds_list.end();
       iter++)
    parsed_bds.push_back(BDParsed(*iter));
  counting_auto_ptr<Mapper> mapper = MapperFactory::add_sources(mapper_type,
								mapper_id,
								mapper_state_ind,
								parsed_bds);

  Variable var("mapper", mapper->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
remove_source(const VarMap& args)
{
  String mapper_type, mapper_id, mapper_state_ind;
  XMLObject bd;
  try {
    VarMap::const_iterator iter = args.find("mapper_type");
    if (iter == args.end())
      throw APIerror("missing mapper_type variable");
    mapper_type = iter->second.get_string();

    iter = args.find("mapper_id");
    if (iter == args.end())
      throw APIerror("missing mapper_id variable");
    mapper_id = iter->second.get_string();

    iter = args.find("mapper_state_ind");
    if (iter == args.end())
      throw APIerror("missing mapper_state_ind variable");
    mapper_state_ind = iter->second.get_string();

    iter = args.find("bd");
    if (iter == args.end())
      throw APIerror("missing bd variable");
    bd = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  throw String("remove_mapper_source() function not implemented");

  /*
  BDParsed parsed_bd(bd);
  counting_auto_ptr<Mapper> mapper = MapperFactory::remove_source(mapper_type,
								  mapper_id,
								  mapper_state_ind,
								  parsed_bd);

  Variable var("mapper", mapper->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
  */
}


VarMap
create_bd(const VarMap& args)
{
  XMLObject bd_xml;
  try {
    VarMap::const_iterator iter = args.find("bd");
    if (iter == args.end())
      throw APIerror("missing bd variable");
    bd_xml = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  // bd
  BDTemplate bd_template(bd_xml);
  counting_auto_ptr<BD> bd = BDFactory::create_bd(bd_template);

  // mapper
  counting_auto_ptr<Mapper> mapper = MapperFactory::get_mapper(bd->mapper_type(),
							       bd->mapper_id());

  Variable var_bd("bd", bd->xml());
  Variable var_map("mapper", mapper->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var_bd.name(), var_bd));
  ret.insert(pair<String, Variable>(var_map.name(), var_map));
  return ret;
}

VarMap
get_bd(const VarMap& args)
{
  String path;
  try {
    VarMap::const_iterator iter = args.find("path");
    if (iter == args.end())
    throw APIerror("missing path variable");
    path = iter->second.get_string();
  } catch ( String e ) {
    throw APIerror(e);
  }

  counting_auto_ptr<BD> bd = BDFactory::get_bd(path);

  Variable var("bd", bd->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
modify_bd(const VarMap& args)
{
  XMLObject bd_xml;
  try {
    VarMap::const_iterator iter = args.find("bd");
    if (iter == args.end())
      throw APIerror("missing bd variable");
    bd_xml = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  BDParsed bd_parsed(bd_xml);
  counting_auto_ptr<BD> bd = BDFactory::modify_bd(bd_parsed);

  Variable var("bd", bd->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
remove_bd(const VarMap& args)
{
  XMLObject bd_xml;
  try {
    VarMap::const_iterator iter = args.find("bd");
    if (iter == args.end())
      throw APIerror("missing bd variable");
    bd_xml = iter->second.get_XML();
  } catch ( String e ) {
    throw APIerror(e);
  }

  BDParsed bd_parsed(bd_xml);
  counting_auto_ptr<Mapper> mapper = BDFactory::remove_bd(bd_parsed);

  Variable var("mapper", mapper->xml());
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
get_fs_group_members(const VarMap& args)
{
  String fsname;
  try {
    VarMap::const_iterator iter = args.find("fsname");
    if (iter == args.end())
      throw APIerror("missing fsname variable");
    fsname = iter->second.get_string();
    if (fsname == "")
		throw APIerror("missing fsname value");
  } catch ( String e ) {
    throw APIerror(e);
  }

  list<XMLObject> ids_list;
  list<String> group_ids = FSController().get_fs_group_ids(fsname);

  for (list<String>::iterator iter = group_ids.begin() ; iter != group_ids.end() ; iter++)
  {
	XMLObject id_xml("group_member");
	id_xml.set_attr("nodeid", String(*iter));
	ids_list.push_back(id_xml);
  }

  Variable var("group_member_list", ids_list);
  VarMap ret;
  ret.insert(pair<String, Variable>(var.name(), var));
  return ret;
}

VarMap
mount_fs(const VarMap& args)
{
	String device, mountpoint, fstype;

	try {
		VarMap::const_iterator iter = args.find("device");
		if (iter == args.end())
			throw APIerror("missing device variable");
		device = iter->second.get_string();

		iter = args.find("mountpoint");
		if (iter == args.end())
			throw APIerror("missing mountpoint variable");
		mountpoint = iter->second.get_string();

		iter = args.find("fstype");
		if (iter == args.end())
			throw APIerror("missing fstyle variable");
		fstype = iter->second.get_string();

		MountHandler mh;
		if (!mh.mount(device, mountpoint, fstype)) {
			throw APIerror("mounting " + device + " at "
					+ mountpoint + " failed");
		}
	} catch (String e) {
		throw APIerror(e);
	}

	VarMap ret;
	return ret;
}

VarMap 
umount_fs(const VarMap& args)
{
	String device(""), mountpoint("");

	try {
		VarMap::const_iterator iter = args.find("device");
		if (iter != args.end())
			device = iter->second.get_string();

		iter = args.find("mountpoint");
		if (iter != args.end())
			mountpoint = iter->second.get_string();

		if (!device.size() && !mountpoint.size())
			throw APIerror("no device or mount point variable");

		MountHandler mh;
		if (mountpoint.size() > 0) {
			if (mh.umount((device.size() > 0 ? device : ""), mountpoint)) {
				VarMap ret;
				return ret;
			}
		}

		if (device.size() > 0) {
			if (mh.umount(device, device)) {
				VarMap ret;
				return ret;
			}
		}
	} catch (String e) {
		throw APIerror(e);
	}

	throw APIerror("unable to unmount "
			+ (mountpoint.size() ? mountpoint : device));
}

VarMap 
fstab_add(const VarMap& args)
{
	String device, mountpoint, fstype;

	try {
		VarMap::const_iterator iter = args.find("device");
		if (iter == args.end())
			throw APIerror("missing device variable");
		device = iter->second.get_string();

		iter = args.find("mountpoint");
		if (iter == args.end())
			throw APIerror("missing mountpoint variable");
		mountpoint = iter->second.get_string();

		iter = args.find("fstype");
		if (iter == args.end())
			throw APIerror("missing fstyle variable");
		fstype = iter->second.get_string();

		MountHandler mh;
		if (!mh.fstab_add(device, mountpoint, fstype)) {
			throw APIerror("adding " + device + " at "
					+ mountpoint + " to fstab failed");
		}
	} catch (String e) {
		throw APIerror(e);
	}

	VarMap ret;
	return ret;
}

VarMap 
fstab_del(const VarMap& args)
{
	String device, mountpoint;

	try {
		VarMap::const_iterator iter = args.find("device");
		if (iter == args.end())
			throw APIerror("missing device variable");
		device = iter->second.get_string();

		iter = args.find("mountpoint");
		if (iter == args.end())
			throw APIerror("missing mountpoint variable");
		mountpoint = iter->second.get_string();
		MountHandler mh;
		mh.fstab_remove(device, mountpoint);
	} catch (String e) {
		throw APIerror(e);
	}

	VarMap ret;
	return ret;
}

list<XMLObject>
_mapper_ids(const String& mapper_type)
{

  list<XMLObject> ids_list;

  list<pair<String, String> > id_pairs = MapperFactory::get_mapper_ids(mapper_type);
  for (list<pair<String, String> >::iterator iter = id_pairs.begin();
       iter != id_pairs.end();
       iter++) {
    XMLObject id_xml("mapper_id");
    id_xml.set_attr("mapper_type", iter->first);
    id_xml.set_attr("mapper_id", iter->second);
    ids_list.push_back(id_xml);
  }

  return ids_list;
}

list<XMLObject>
_mappers(const String& mapper_type, const String& mapper_id)
{

  list<XMLObject> mapper_list;

  list<counting_auto_ptr<Mapper> > mappers =
    MapperFactory::get_mappers(mapper_type, mapper_id);
  for (list<counting_auto_ptr<Mapper> >::iterator iter = mappers.begin();
       iter != mappers.end();
       iter++)
    mapper_list.push_back((*iter)->xml());

  return mapper_list;
}

list<XMLObject>
_mapper_templates(const String& mapper_type)
{

  list<XMLObject> temp_list;

  list<counting_auto_ptr<MapperTemplate> > mapper_templates =
    MapperFactory::get_mapper_templates(mapper_type);
  for (list<counting_auto_ptr<MapperTemplate> >::iterator iter = mapper_templates.begin();
       iter != mapper_templates.end();
       iter++)
    temp_list.push_back((*iter)->xml());

  return temp_list;
}

VarMap
enable_clustered_lvm(const VarMap& args)
{
  LVM::enable_clustered();

  VarMap ret;
  return ret;
}

VarMap
disable_clustered_lvm(const VarMap& args)
{
  LVM::disable_clustered();

  VarMap ret;
  return ret;
}
