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


#include "MapperFactory.h"
#include "VG.h"
#include "System.h"
#include "PartitionTable.h"
#include "MDRaid.h"
#include "defines.h"
#include "MidAir.h"


#include <iostream>
using namespace std;



list<pair<String, String> >
MapperFactory::get_mapper_ids(const String& mapper_type)
{
  list<pair<String, String> > id_pairs;
  if (mapper_type == MAPPER_VG_TYPE || mapper_type.empty()) {
    list<String> ids = get_VG_ids();
    for (list<String>::iterator iter = ids.begin();
	 iter != ids.end();
	 iter++)
      id_pairs.push_back(pair<String, String>(MAPPER_VG_TYPE, *iter));
  }
  if (mapper_type == MAPPER_SYS_TYPE || mapper_type.empty()) {
    list<String> ids = get_SYS_ids();
    for (list<String>::iterator iter = ids.begin();
	 iter != ids.end();
	 iter++)
      id_pairs.push_back(pair<String, String>(MAPPER_SYS_TYPE, *iter));
  }
  if (mapper_type == MAPPER_PT_TYPE || mapper_type.empty()) {
    list<String> ids = get_PT_ids();
    for (list<String>::iterator iter = ids.begin();
	 iter != ids.end();
	 iter++)
      id_pairs.push_back(pair<String, String>(MAPPER_PT_TYPE, *iter));
  }
  if (mapper_type == MAPPER_MDRAID_TYPE || mapper_type.empty()) {
    list<String> ids = get_MDRaid_ids();
    for (list<String>::iterator iter = ids.begin();
	 iter != ids.end();
	 iter++)
      id_pairs.push_back(pair<String, String>(MAPPER_MDRAID_TYPE, *iter));
  }
  return id_pairs;
}

counting_auto_ptr<Mapper>
MapperFactory::get_mapper(const String& mapper_type,
			  const String& mapper_id)
{
  if (mapper_type == MAPPER_VG_TYPE)
    return counting_auto_ptr<Mapper>(new VG(mapper_id));
  if (mapper_type == MAPPER_SYS_TYPE)
    return counting_auto_ptr<Mapper>(new System(mapper_id));
  if (mapper_type == MAPPER_PT_TYPE)
    return counting_auto_ptr<Mapper>(new PartitionTable(mapper_id));
  if (mapper_type == MAPPER_MDRAID_TYPE)
    return counting_auto_ptr<Mapper>(new MDRaid(mapper_id));
  throw String("no such mapper type");
}

list<counting_auto_ptr<Mapper> >
MapperFactory::get_mappers(const String& mapper_type,
			   const String& mapper_id)
{
  list<counting_auto_ptr<Mapper> > mappers;
  list<pair<String, String> > id_pairs = get_mapper_ids("");
  for (list<pair<String, String> >::iterator iter = id_pairs.begin();
       iter != id_pairs.end();
       iter++) {
    if ((mapper_type.empty() || mapper_type == iter->first) &&
	(mapper_id.empty() || mapper_id == iter->second))
      try {
	mappers.push_back(get_mapper(iter->first, iter->second));
	//      } catch ( String e ) {
	//	cout << e << endl;
      } catch ( ... ) {}
  }
  return mappers;
}

list<counting_auto_ptr<MapperTemplate> >
MapperFactory::get_mapper_templates(const String& mapper_type)
{
  list<counting_auto_ptr<MapperTemplate> > temps;
  if (mapper_type == MAPPER_VG_TYPE ||
      mapper_type.empty())
    try {
      temps.push_back(counting_auto_ptr<MapperTemplate>(new VGTemplate()));
    } catch ( ... ) {}
  if (mapper_type == MAPPER_PT_TYPE ||
      mapper_type.empty())
    try {
      temps.push_back(counting_auto_ptr<MapperTemplate>(new PTTemplate()));
    } catch ( ... ) {}
  if (mapper_type == MAPPER_MDRAID_TYPE ||
      mapper_type.empty())
    try {
      temps.push_back(counting_auto_ptr<MapperTemplate>(new MDRaidTemplate()));
    } catch ( ... ) {}
  return temps;
}

counting_auto_ptr<Mapper>
MapperFactory::create_mapper(const MapperTemplate& mapper_temp)
{
  mapper_temp.props.validate();
  if (mapper_temp.mapper_type == MAPPER_VG_TYPE)
    return create_VG(mapper_temp);
  if (mapper_temp.mapper_type == MAPPER_PT_TYPE)
    return create_PT(mapper_temp);
  if (mapper_temp.mapper_type == MAPPER_MDRAID_TYPE)
    return create_MDRaid(mapper_temp);
  throw String("no such mapper type");
}

counting_auto_ptr<Mapper>
MapperFactory::modify_mapper(const MapperParsed& mapper_parsed)
{
  mapper_parsed.mapper->apply(mapper_parsed);
  return get_mapper(mapper_parsed.mapper_type, mapper_parsed.mapper_id);
}

void
MapperFactory::remove_mapper(const MapperParsed& mapper_pars)
{
  mapper_pars.mapper->remove();
}


counting_auto_ptr<Mapper>
MapperFactory::add_sources(const String& mapper_type,
			   const String& mapper_id,
			   const String& mapper_state_ind,
			   const list<BDParsed>& bds)
{
  counting_auto_ptr<Mapper> mapper = get_mapper(mapper_type, mapper_id);
  if (mapper->state_ind() != mapper_state_ind)
    throw MidAir();

  mapper->add_sources(bds);
  return get_mapper(mapper_type, mapper_id);
}
