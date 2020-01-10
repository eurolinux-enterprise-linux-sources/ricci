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


#include "Content.h"
#include "utils.h"
#include "MidAir.h"
#include "ValidationError.h"


using namespace std;



// ### class Content ###

Content::Content(const String& type, const String& path) :
  type(type)
{
  _props.set(Variable("path", path));
}

Content::~Content()
{}


String
Content::path() const
{
  return _props.get("path").get_string();
}

String
Content::state_ind() const
{
  XMLObject xml;
  xml.add_child(_props.xml());

  return utils::hash_str(generateXML(xml) + path());
}

void
Content::add_replacement(const counting_auto_ptr<ContentTemplate>& replacement)
{
  _avail_replacements.push_back(replacement);
}

bool
Content::removable() const
{
  return false;
}

XMLObject
Content::xml() const
{
  XMLObject xml(CONTENT_TYPE_TAG);
  xml.set_attr("type", type);
  //  xml.set_attr("path", path());
  //  xml.set_attr("state_ind", state_ind());
  xml.add_child(_props.xml());

  XMLObject replacements(CONTENT_REPLACEMENTS_TAG);
  for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter = _avail_replacements.begin();
       iter != _avail_replacements.end();
       iter++)
    replacements.add_child((*iter)->xml());
  xml.add_child(replacements);

  XMLObject replacement(CONTENT_NEW_CONTENT_TAG);
  if (_replacement.get())
    replacement.add_child(_replacement->xml());
  xml.add_child(replacement);

  return xml;
}




// ### class ContentTemplate ###

ContentTemplate::ContentTemplate(const String& type) :
  type(type)
{
  attrs["type"] = type;
}

ContentTemplate::ContentTemplate(const XMLObject& xml,
				 counting_auto_ptr<Content>& content)
{
  if (xml.tag() != CONTENT_TEMPLATE_TYPE_TAG)
    throw String("not Content template");

  attrs = xml.attrs();

  type = xml.get_attr("type");


  const list<XMLObject>& kids = xml.children();
  for (list<XMLObject>::const_iterator kid_iter = kids.begin();
       kid_iter != kids.end();
       kid_iter++) {
    try {
      _props = Props(*kid_iter);
      break;
    } catch ( ... ) {}
  }

  // validate props
  counting_auto_ptr<ContentTemplate> orig_ct;
  for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter =
	 content->_avail_replacements.begin();
       iter != content->_avail_replacements.end();
       iter++)
    if ((*iter)->attrs == attrs)
      orig_ct = *iter;

  if (orig_ct.get()) {
    orig_ct->_props.validate(_props);
  } else
    throw MidAir();
}

ContentTemplate::~ContentTemplate()
{}

XMLObject
ContentTemplate::xml() const
{
  XMLObject xml(CONTENT_TEMPLATE_TYPE_TAG);

  xml.set_attr("type", type);
  for (map<String, String>::const_iterator iter = attrs.begin();
       iter != attrs.end();
       iter++)
    xml.set_attr(iter->first, iter->second);

  xml.add_child(_props.xml());

  return xml;
}




// ### class ContentParsed ###

ContentParsed::ContentParsed(const XMLObject& xml,
			     counting_auto_ptr<Content>& content) :
  content(content)
{
  if (xml.tag() != CONTENT_TYPE_TAG)
    throw String("not Content") + xml.tag();
  if (!content.get())
    throw String("content null pointer!!!");

  const XMLObject& orig_xml = content->xml();
  for (std::map<String, String>::const_iterator iter = orig_xml.attrs().begin();
       iter != orig_xml.attrs().end();
       iter++)
    if (xml.get_attr(iter->first) != iter->second)
      throw String("not a matching content");

  for (list<XMLObject>::const_iterator iter = xml.children().begin();
       iter != xml.children().end();
       iter++) {
    try {
      props = Props(*iter);
      continue;
    } catch ( ... ) {}

    // replacement
    if (iter->tag() == CONTENT_NEW_CONTENT_TAG) {
      if (iter->children().size() > 1)
	throw String("invalid number of replacement contents");
      else if (iter->children().size() == 1)
	_replacement =
	  counting_auto_ptr<ContentTemplate>(new ContentTemplate(iter->children().front(), content));
    }
  }

  // ### validation ###

  content->_props.validate(props);;

  // validate replacement
  if (_replacement.get()) {
    bool replacement_found = false;
    for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter =
	   content->_avail_replacements.begin();
	 iter != content->_avail_replacements.end();
	 iter++) {
      const ContentTemplate& temp_repl = *(*iter);
      XMLObject r_xml = _replacement->xml();
      XMLObject t_xml = temp_repl.xml();
      if (r_xml.tag() == t_xml.tag() &&
	  r_xml.attrs() == t_xml.attrs()) {
	replacement_found = true;
	temp_repl._props.validate(_replacement->_props);
      }
    }
    if (!replacement_found)
      throw MidAir();
  }
}

ContentParsed::~ContentParsed()
{}
