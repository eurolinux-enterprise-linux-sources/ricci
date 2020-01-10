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


#include "Mapper.h"
#include "utils.h"
#include "BDFactory.h"
#include "MapperFactory.h"
#include "MidAir.h"
#include <algorithm>


using namespace std;


//  ##### Mapper #####

Mapper::Mapper(const String& type, const String& id) :
  _mapper_type(type),
  _mapper_id(id)
{
  removable(false);
}

Mapper::~Mapper()
{}


bool
Mapper::removable() const
{
  return _props.get("removable").get_bool();
}

void
Mapper::removable(bool removable)
{
  _props.set(Variable("removable", removable));
}


void
Mapper::add_sources(const list<BDParsed>& parsed_bds)
{
  list<String> paths;
  list<counting_auto_ptr<BD> > bds;
  for (list<BDParsed>::const_iterator iter = parsed_bds.begin();
       iter != parsed_bds.end();
       iter++)
    if (find(paths.begin(), paths.end(), iter->path) == paths.end()) {
      paths.push_back(iter->path);
      bds.push_back(iter->bd);
    }

  if (bds.empty())
    return;

  for (list<counting_auto_ptr<BD> >::const_iterator bd_iter = bds.begin();
       bd_iter != bds.end();
       bd_iter++) {
    bool found = false;
    for (list<counting_auto_ptr<BD> >::const_iterator ns_iter = new_sources.begin();
	 ns_iter != new_sources.end();
	 ns_iter++)
      if ((*ns_iter)->path() == (*bd_iter)->path() &&
	  (*ns_iter)->state_ind() == (*bd_iter)->state_ind())
	found = true;
    if (!found)
      throw MidAir();
  }

  this->__add_sources(bds);
  utils::clear_cache();
}

void
Mapper::remove()
{
  if (!_props.get("removable").get_bool())
    throw String("mapper not removable");

  for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
       iter != targets.end();
       iter++)
    (*iter)->remove();

  this->__remove();
}


String
Mapper::state_ind() const
{
  XMLObject xml("hash");
  xml.add_child(_props.xml());
  for (list<counting_auto_ptr<BD> >::const_iterator iter = sources.begin();
       iter != sources.end();
       iter++)
    xml.add_child((*iter)->xml());
  for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
       iter != targets.end();
       iter++)
    xml.add_child((*iter)->xml());
  return utils::hash_str(generateXML(xml));
}

XMLObject
Mapper::xml() const
{
  XMLObject xml(MAPPER_TYPE_TAG);

  xml.set_attr("mapper_type", _mapper_type);
  xml.set_attr("mapper_id", _mapper_id);
  xml.set_attr("state_ind", state_ind());

  xml.add_child(_props.xml());

  XMLObject sources_xml(MAPPER_SOURCES_TAG);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = sources.begin();
       iter != sources.end();
       iter++)
    sources_xml.add_child((*iter)->xml());
  xml.add_child(sources_xml);

  XMLObject targets_xml(MAPPER_TARGETS_TAG);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
       iter != targets.end();
       iter++)
    targets_xml.add_child((*iter)->xml());
  xml.add_child(targets_xml);

  /*
  XMLObject mappings(MAPPER_MAPPINGS_TAG);
  xml.add_child(mappings);
  */


  XMLObject new_sources_xml(MAPPER_NEW_SOURCES_TAG);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = new_sources.begin();
       iter != new_sources.end();
       iter++)
    new_sources_xml.add_child((*iter)->xml());
  xml.add_child(new_sources_xml);

  XMLObject new_targets_xml(MAPPER_NEW_TARGETS_TAG);
  for (list<counting_auto_ptr<BDTemplate> >::const_iterator iter = new_targets.begin();
       iter != new_targets.end();
       iter++)
    new_targets_xml.add_child((*iter)->xml());
  xml.add_child(new_targets_xml);

  return xml;
}




// ##### MapperParsed


MapperParsed::MapperParsed(const XMLObject& xml)
{
  if (xml.tag() != MAPPER_TYPE_TAG)
    throw String("not Mapper");

  mapper_type = xml.get_attr("mapper_type");
  mapper_id = xml.get_attr("mapper_id");
  state_ind = xml.get_attr("state_ind");
  if (mapper_type.empty() ||
      mapper_id.empty() ||
      state_ind.empty())
    throw String("MapperParsed missing identification");

  // get Mapper
  try {
    mapper = MapperFactory::get_mapper(mapper_type, mapper_id);
  } catch ( ... ) {
    throw MidAir();
  }

  for (list<XMLObject>::const_iterator iter = xml.children().begin();
       iter != xml.children().end();
       iter++) {
    try {
      props = Props(*iter);
      continue;
    } catch ( ... ) {}
  }

  // ### validate ###
  if (mapper->state_ind() != state_ind)
    throw MidAir();
  mapper->_props.validate(props);;

}

MapperParsed::~MapperParsed()
{}




//  ##### MapperTemplate #####

MapperTemplate::MapperTemplate(const String& type) :
  mapper_type(type)
{}

MapperTemplate::MapperTemplate(const XMLObject& xml)
{
  if (xml.tag() != MAPPER_TEMPLATE_TYPE_TAG)
    throw String("not mapper template");

  list<counting_auto_ptr<MapperTemplate> > mapper_temps =
    MapperFactory::get_mapper_templates(xml.get_attr("mapper_type"));
  if (mapper_temps.size() != 1)
    throw String("invalid number of mapper_templates returned");
  counting_auto_ptr<MapperTemplate> orig_mapp = mapper_temps.front();
  mapper_type = orig_mapp->mapper_type;

  const list<XMLObject>& kids = xml.children();
  for (list<XMLObject>::const_iterator kid_iter = kids.begin();
       kid_iter != kids.end();
       kid_iter++) {
    const XMLObject& kid = *kid_iter;

    try {
      props = Props(kid);
      continue;
    } catch ( ... ) {}

    if (kid.tag() == MAPPER_SOURCES_TAG) {
      for (list<XMLObject>::const_iterator bd_iter = kid.children().begin();
	   bd_iter != kid.children().end();
	   bd_iter++) {
	BDParsed bdp(*bd_iter);
	counting_auto_ptr<BD> bd;
	for (list<counting_auto_ptr<BD> >::const_iterator obd_iter =
	       orig_mapp->new_sources.begin();
	     obd_iter != orig_mapp->new_sources.end();
	     obd_iter++) {
	  if ((*obd_iter)->path() == bdp.path) {
	    bd = *obd_iter;
	    break;
	  }
	}
	if (bd.get() == NULL)
	  throw MidAir();
	if (bd->state_ind() != bdp.state_ind)
	  throw MidAir();
	else
	  sources.push_back(bd);
      }
    }
  }
  // validate props
  orig_mapp->props.validate(props);
}

MapperTemplate::~MapperTemplate()
{}


XMLObject
MapperTemplate::xml() const
{
  XMLObject xml(MAPPER_TEMPLATE_TYPE_TAG);

  xml.set_attr("mapper_type", mapper_type);
  //  xml.set_attr("state_ind", state_ind());

  xml.add_child(props.xml());

  XMLObject sources_xml(MAPPER_SOURCES_TAG);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = sources.begin();
       iter != sources.end();
       iter++)
    sources_xml.add_child((*iter)->xml());
  xml.add_child(sources_xml);

  XMLObject targets_xml(MAPPER_TARGETS_TAG);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
       iter != targets.end();
       iter++)
    targets_xml.add_child((*iter)->xml());
  xml.add_child(targets_xml);

  /*
  XMLObject mappings(MAPPER_MAPPINGS_TAG);
  xml.add_child(mappings);
  */



  XMLObject new_sources_xml(MAPPER_NEW_SOURCES_TAG);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = new_sources.begin();
       iter != new_sources.end();
       iter++)
    new_sources_xml.add_child((*iter)->xml());
  xml.add_child(new_sources_xml);

  XMLObject new_targets_xml(MAPPER_NEW_TARGETS_TAG);
  for (list<counting_auto_ptr<BDTemplate> >::const_iterator iter = new_targets.begin();
       iter != new_targets.end();
       iter++)
    new_targets_xml.add_child((*iter)->xml());
  xml.add_child(new_targets_xml);

  return xml;
}
