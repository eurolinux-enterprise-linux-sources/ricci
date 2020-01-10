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


#include "BD.h"
#include "Mapper.h"
#include "utils.h"
#include "ContentNone.h"
#include "ContentFactory.h"
#include "MapperFactory.h"
#include "BDFactory.h"
#include "MidAir.h"
#include "ValidationError.h"


#include <unistd.h>


using namespace std;



//  ##### BD #####

BD::BD(const String& mapper_type,
       const String& mapper_id,
       const String& bd_type,
       const String& path,
       bool check_path) :
  content(ContentFactory().probe(path)),
  _mapper_type(mapper_type),
  _mapper_id(mapper_id),
  _bd_type(bd_type),
  _path(path)
{
  if (check_path) {
    //    if (access(path.c_str(), F_OK) != 0)
    //      throw String("file non-existant");

    // size (can be overwritten by descendants)
    String out, err;
    int status;
    vector<String> args;
    args.push_back("--getss");
    args.push_back("--getsize");
    args.push_back(path);
    if (utils::execute("/sbin/blockdev", args, out, err, status))
      throw command_not_found_error_msg("blockdev");
    if (status != 0)
      throw String("blockdev failed");
    out = utils::strip(out);
    vector<String> lines = utils::split(out, "\n");
    if (lines.size() != 2)
      throw String("invalid output from blockdev");
    long long size = utils::to_long(utils::strip(lines[0]));
    size *= utils::to_long(utils::strip(lines[1]));
    if (size == 0)
      throw String("invalid size");
    else
      _props.set(Variable("size", size));
  }

  _props.set(Variable("path", path));
  removable(false);

}

BD::~BD()
{}


String
BD::path() const
{
  return _props.get("path").get_string();
}

long long
BD::size() const
{
  return _props.get("size").get_int();
}

String
BD::state_ind() const
{
  XMLObject t;
  t.add_child(_props.xml());
  t.add_child(content->xml());

  return utils::hash_str(generateXML(t) + path() + _mapper_id);
}

String
BD::mapper_id() const
{
  return _mapper_id;
}

String
BD::mapper_type() const
{
  return _mapper_type;
}


bool
BD::removable() const
{
  return _props.get("removable").get_bool();
}

void
BD::removable(bool removable)
{
  _props.set(Variable("removable", removable));
}

counting_auto_ptr<BD>
BD::apply(const BDParsed& bd_parsed)
{
  if (bd_parsed.content->_replacement.get()) {
    // replace content

    content->remove();

    // first
    this->apply_props_before_resize(bd_parsed.props);
    // second - size
    long long orig_size = this->size();
    long long new_size = bd_parsed.props.get("size").get_int();
    if (orig_size > new_size) {
      this->shrink(new_size, bd_parsed.props);
    } else if (orig_size < new_size) {
      this->expand(new_size, bd_parsed.props);
    }
    // third
    this->apply_props_after_resize(bd_parsed.props);

    counting_auto_ptr<BD> new_bd = BDFactory().get_bd(path());
    ContentFactory().create_content(new_bd, bd_parsed.content);
  } else {
    // modify content

    long long orig_size = this->size();
    long long new_size = bd_parsed.props.get("size").get_int();

    // first
    this->apply_props_before_resize(bd_parsed.props);
    content->apply_props_before_resize(path(),
				       orig_size,
				       new_size,
				       bd_parsed.content->props);

    // second - size
    if (orig_size > new_size) {
      content->shrink(path(), new_size, bd_parsed.content->props);
      this->shrink(new_size, bd_parsed.props);
    } else if (orig_size < new_size) {
      this->expand(new_size, bd_parsed.props);
      try {
	content->expand(path(), new_size, bd_parsed.content->props);
      } catch ( ... ) {
	this->shrink(orig_size, bd_parsed.props);
	throw;
      }
    }

    // third
    this->apply_props_after_resize(bd_parsed.props);
    content->apply_props_after_resize(path(),
				      orig_size,
				      new_size,
				      bd_parsed.content->props);
  }

  utils::clear_cache();
  return BDFactory().get_bd(path());
}



XMLObject
BD::xml() const
{
  XMLObject bd(BD_TYPE_TAG);
  //  bd.set_attr("type", "bd_type");
  bd.set_attr("path", path());
  bd.set_attr("mapper_type", _mapper_type);
  bd.set_attr("mapper_id", _mapper_id);
  bd.set_attr("state_ind", state_ind());
  bd.add_child(_props.xml());
  bd.add_child(content->xml());

  return bd;
}









//  ##### BDTemplate #####

BDTemplate::BDTemplate(const String& mapper_type,
		       const String& mapper_id,
		       const String& mapper_state_ind,
		       const String& bd_type) :
  mapper_type(mapper_type),
  mapper_id(mapper_id),
  mapper_state_ind(mapper_state_ind),
  bd_type(bd_type),
  content(counting_auto_ptr<Content>(new ContentNone("")))
{

}

BDTemplate::BDTemplate(const XMLObject& xml)
{
  if (xml.tag() != BD_TEMPLATE_TYPE_TAG)
    throw String("not BD template");

  mapper_type = xml.get_attr("mapper_type");
  mapper_id = xml.get_attr("mapper_id");
  mapper_state_ind = xml.get_attr("mapper_state_ind");

  counting_auto_ptr<Mapper> mapper =
    MapperFactory::get_mapper(mapper_type, mapper_id);
  if (mapper->state_ind() != mapper_state_ind)
    throw MidAir();
  if (mapper_type == MAPPER_VG_TYPE)
    bd_type = BD_LV_TYPE;
  else if (mapper_type == MAPPER_PT_TYPE)
    bd_type = BD_PART_TYPE;
  else
    throw String("BDTemplate(): not implemented");

  XMLObject content_xml;
  const list<XMLObject>& kids = xml.children();
  for (list<XMLObject>::const_iterator kid_iter = kids.begin();
       kid_iter != kids.end();
       kid_iter++) {
    const XMLObject& kid = *kid_iter;
    try {
      props = Props(kid);
      continue;
    } catch ( ... ) {}
    if (kid.tag() == CONTENT_TYPE_TAG)
      content_xml = kid;
  }

  // validate props and content

  list<counting_auto_ptr<BDTemplate> > bd_candidates;
  for (list<counting_auto_ptr<BDTemplate> >::const_iterator t_iter =
	 mapper->new_targets.begin();
       t_iter != mapper->new_targets.end();
       t_iter++) {
    try {
      (*t_iter)->props.validate(props);
      bd_candidates.push_back(*t_iter);
    } catch ( ... ) {}
  }

  // bd_candidates have valid props, verify content
  bool valid = false;
  for (list<counting_auto_ptr<BDTemplate> >::const_iterator iter = bd_candidates.begin();
       iter != bd_candidates.end();
       iter++) {
    try {
      content_parsed =
	counting_auto_ptr<ContentParsed>(new ContentParsed(content_xml,
							   (*iter)->content));
      valid = true;
      break;
    } catch ( ... ) {}
  }
  if (!valid)
    throw ValidationError();
}

BDTemplate::~BDTemplate()
{}


XMLObject
BDTemplate::xml() const
{
  XMLObject bd(BD_TEMPLATE_TYPE_TAG);
  bd.set_attr("mapper_type", mapper_type);
  bd.set_attr("mapper_id", mapper_id);
  bd.set_attr("mapper_state_ind", mapper_state_ind);
  bd.add_child(props.xml());
  bd.add_child(content->xml());

  return bd;
}






//  ##### BDParsed #####

BDParsed::BDParsed(const XMLObject& xml)
{
  if (xml.tag() != BD_TYPE_TAG)
    throw String("not BD");

  path = xml.get_attr("path");
  state_ind = xml.get_attr("state_ind");
  if (path.empty() ||
      state_ind.empty())
    throw String("BDParsed missing identification");

  // get BD
  try {
    bd = BDFactory::get_bd(path);
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
    try {
      content = counting_auto_ptr<ContentParsed>(new ContentParsed(*iter, bd->content));
    } catch (Except e) {
      throw;
    } catch ( ... ) {}
  }

  // ### validate ###
  if (bd->state_ind() != state_ind)
    throw MidAir();
  bd->_props.validate(props);;
  if (!content.get())
    throw String("missing content tag");
  // content validated self



}

BDParsed::~BDParsed()
{}
