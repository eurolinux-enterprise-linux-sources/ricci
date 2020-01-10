/*
  Copyright Red Hat, Inc. 2005

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


#include "MDRaid.h"
#include "BDFactory.h"
#include "MapperFactory.h"


using namespace std;


list<String>
get_MDRaid_ids()
{
  list<String> ids;

  try {
    list<mdraid> raids = mdadm::raids();
    for (list<mdraid>::const_iterator iter = raids.begin();
	 iter != raids.end();
	 iter++)
      ids.push_back(MDRAID_PREFIX + iter->path);
  } catch ( ... ) {}

  return ids;
}




//  ##### MDRaid #####

MDRaid::MDRaid(const String& id) :
  Mapper(MAPPER_MDRAID_TYPE, id)
{
  String raid_path = id.substr(MDRAID_PREFIX.size());
  _raid = mdadm::probe_path(raid_path);
  if (_raid.path != raid_path)
    throw String("not mdraid");

  for (list<mdraid_source>::const_iterator iter = _raid.devices.begin();
       iter != _raid.devices.end();
       iter++)
    sources.push_back(BDFactory::get_bd(iter->path));

  try {
    targets.push_back(BDFactory::get_bd(_raid.path));
  } catch ( ... ) {}

  _props.set(Variable("raid", _raid.name));
  _props.set(Variable("raid_level", _raid.level));
  _props.set(Variable("uuid", _raid.uuid));
  _props.set(Variable("num_devices", _raid.num_devices));
  _props.set(Variable("num_spares", (long long) _raid.devices.size() - _raid.num_devices));

  if (targets.empty())
    _props.set(Variable("active", false, true));
  else {
    if (targets.front()->content->type == CONTENT_NONE_TYPE)
      _props.set(Variable("active", true, true));
    else
      _props.set(Variable("active", true));
    /*
    if (targets.front()->content->removable())
      _props.set(Variable("active", true, true));
    else
      _props.set(Variable("active", true));
    */

    if (targets.front()->content->removable())
      removable(true);

    list<counting_auto_ptr<Mapper> > mappers =
      MapperFactory::get_mappers(MAPPER_PT_TYPE);
    for (list<counting_auto_ptr<Mapper> >::iterator iter_map = mappers.begin();
	 iter_map != mappers.end();
	 iter_map++) {
      Mapper& mapper = **iter_map;
      for (list<counting_auto_ptr<BD> >::iterator iter_bd = mapper.targets.begin();
	   iter_bd != mapper.targets.end();
	   iter_bd++) {
	if ((*iter_bd)->content->type == CONTENT_NONE_TYPE &&
	    (*iter_bd)->size() >= targets.front()->size())
	  new_sources.push_back(*iter_bd);
      }
    }
  }
}

MDRaid::~MDRaid()
{}


void
MDRaid::apply(const MapperParsed& mp)
{
  bool active_old = _props.get("active").get_bool();
  bool active_new = mp.props.get("active").get_bool();
  if (active_old && !active_new)
    // FIXME: umount FS before stoping, disabled for now (see constructor)
    mdadm::stop_raid(_raid);
  else if (!active_old && active_new)
    mdadm::start_raid(_raid);
}

void
MDRaid::__add_sources(const list<counting_auto_ptr<BD> >& bds)
{
  for (list<counting_auto_ptr<BD> >::const_iterator iter = bds.begin();
       iter != bds.end();
       iter++)
    mdadm::add_source(_raid.path, (*iter)->path());
}

void
MDRaid::__remove()
{
  //  throw String("MDRaid::_remove() not yet implemented");

  mdadm::stop_raid(_raid);

  for (list<mdraid_source>::const_iterator iter = _raid.devices.begin();
       iter != _raid.devices.end();
       iter++)
    try {
      mdadm::zero_superblock(iter->path);
    } catch ( ... ) {}
}



counting_auto_ptr<Mapper>
create_MDRaid(const MapperTemplate& temp)
{
  //  throw String("create_MDRaid() not yet implemented");

  // everything is already validated, but number of sources
  if (temp.sources.size() < 2)
    throw String("create_MDRaid requires at least two sources");

  list<String> devs;
  for (list<counting_auto_ptr<BD> >::const_iterator iter = temp.sources.begin();
       iter != temp.sources.end();
       iter++)
    devs.push_back((*iter)->path());

  String raid_path = mdadm::create_raid(temp.props.get("level").get_string(),
					devs);

  return counting_auto_ptr<Mapper>(new MDRaid(MDRAID_PREFIX + raid_path));

  /*
  String vgname = temp.props.get("vgname").get_string();
  long long extent_size = temp.props.get("extent_size").get_int();

  try {
    utils::clear_cache();

    list<String> pv_paths;
    for (list<counting_auto_ptr<BD> >::const_iterator iter = temp.sources.begin();
	 iter != temp.sources.end();
	 iter++) {
      counting_auto_ptr<BD> bd = *iter;
      pv_paths.push_back(bd->path());
      // create PV if needed
      if (bd->content->type == CONTENT_NONE_TYPE)
	LVM::pvcreate(bd->path());
    }

    // create VG
    LVM::vgcreate(vgname, extent_size, pv_paths);
  } catch ( ... ) {
    for (list<counting_auto_ptr<BD> >::const_iterator iter = temp.sources.begin();
	 iter != temp.sources.end();
	 iter++) {
      counting_auto_ptr<BD> bd = *iter;
      if (bd->content->type == CONTENT_NONE_TYPE)
	try {
	  LVM::pvremove(bd->path());
	} catch ( ... ) {}
    }
    throw;
  }

  return counting_auto_ptr<Mapper>(new VG(VG_PREFIX + vgname));
  */
}



//  ##### MDRaidTemplate #####

MDRaidTemplate::MDRaidTemplate() :
  MapperTemplate(MAPPER_MDRAID_TYPE)
{
  //  throw String("MDRaidTemplate() not yet implemented");

  list<counting_auto_ptr<Mapper> > mappers =
    MapperFactory::get_mappers(MAPPER_PT_TYPE);
  for (list<counting_auto_ptr<Mapper> >::iterator iter_map = mappers.begin();
       iter_map != mappers.end();
       iter_map++) {
    Mapper& mapper = **iter_map;
    for (list<counting_auto_ptr<BD> >::iterator iter_bd = mapper.targets.begin();
	 iter_bd != mapper.targets.end();
	 iter_bd++) {
      if ((*iter_bd)->content->type == CONTENT_NONE_TYPE)
	new_sources.push_back(*iter_bd);
    }
  }
  if (new_sources.size() < 2)
    throw String("not enough sources for mdraid");
  props.set(Variable("min_sources", (long long) 2));
  props.set(Variable("max_sources", (long long) new_sources.size()));

  list<String> levels = mdadm::valid_raid_levels();
  props.set(Variable("level", levels.front(), levels));
}

MDRaidTemplate::~MDRaidTemplate()
{}
