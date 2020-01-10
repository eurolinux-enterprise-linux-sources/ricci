/*
** Copyright (C) Red Hat, Inc. 2005-2009
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

#include "XML.h"
#include "Mutex.h"
#include "utils.h"
#include "File.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <algorithm>

//#include <iostream>
using namespace std;

static String escape_chars(const String&);
static String invert_chars(const String&);

XMLObject::XMLObject(const String& elem_name) :
	_tag(elem_name)
{}

XMLObject::~XMLObject()
{}

bool
XMLObject::operator== (const XMLObject& obj) const
{
	if (children() != obj.children())
		return false;
	if (tag() != obj.tag())
		return false;
	if (attrs() != obj.attrs())
		return false;
	return true;
}

bool
XMLObject::has_attr(const String& attr_name) const
{
	return _attrs.find(attr_name) != _attrs.end();
}

String
XMLObject::set_attr(const String& attr_name, const String& value)
{
	String ret = _attrs[attr_name];
	_attrs[attr_name] = value;
	return ret;
}

String
XMLObject::get_attr(const String& attr_name) const
{
	map<String, String>::const_iterator iter = _attrs.find(attr_name);
	if (iter == _attrs.end())
		return "";
	else
		return iter->second;
}

XMLObject&
XMLObject::add_child(const XMLObject& child)
{
	_kids.push_back(child);
	return _kids.back();
}

bool
XMLObject::remove_child(const XMLObject& child)
{
	list<XMLObject>::iterator iter = find(_kids.begin(), _kids.end(), child);
	if (iter == _kids.end())
		return false;
	else {
		_kids.erase(iter);
		return true;
	}
}

void
XMLObject::generate_xml(String& xml, const String& indent) const
{
	xml += indent + "<" + _tag;
	for (map<String, String>::const_iterator
		iter = attrs().begin() ;
		iter != attrs().end() ;
		iter++)
	{
		const String& name = iter->first;
		const String value = escape_chars(iter->second);
		xml += " " + name + "=\"" + value + "\"";
	}

	if (children().empty())
		xml += "/>\n";
	else {
		xml += ">\n";
		for (list<XMLObject>::const_iterator
				iter = children().begin() ;
				iter != children().end() ;
				iter++)
		{
			iter->generate_xml(xml, indent + "\t");
		}

		xml += indent + "</" + _tag + ">\n";
	}
}

// *** GLOBAL FUNCTIONS ***

static void
_parseXML(XMLObject& parent, xmlNode* children)
{
	for (xmlNode *curr_node = children; curr_node ; curr_node = curr_node->next)
	{
		if (curr_node->type == XML_ELEMENT_NODE) {
			XMLObject me((const char *) curr_node->name);

			// attrs
			for (xmlAttr* curr_attr = curr_node->properties ;
					curr_attr ;
					curr_attr = curr_attr->next)
			{
				if (curr_attr->type == XML_ATTRIBUTE_NODE) {
					const xmlChar *name = curr_attr->name;
					const xmlChar *value = xmlGetProp(curr_node, name);

					if (!value)
						throw String("xmlGetProp() returned NULL");

					try {
						const String name_str((const char *) name);
						const String value_str =
							invert_chars((const char *) value);

						me.set_attr(name_str, value_str);
						xmlFree((void *) value);
					} catch ( ... ) {
						xmlFree((void *) value);
						throw;
					}
				}
			}

			// kids
			_parseXML(me, curr_node->children);
			parent.add_child(me);
		}
	}
}

XMLObject
parseXML(const String& xml)
{
	static bool initialized = false;

	if (!initialized) {
		LIBXML_TEST_VERSION;
		initialized = true;
	}

	xmlDoc *doc = xmlReadMemory(xml.c_str(),
					xml.size(),
					"noname.xml",
					NULL,
					XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);

	if (!doc)
		throw String("parseXML(): couldn't parse xml");

	XMLObject root("if you see this, something wrong happened");

	try {
		_parseXML(root, xmlDocGetRootElement(doc));
		xmlFreeDoc(doc);
		return *(root.children().begin());
	} catch ( ... ) {
		xmlFreeDoc(doc);
		throw String("parseXML(): low memory");
	}
}

String
generateXML(const XMLObject& obj)
{
	String xml("<?xml version=\"1.0\"?>\n");
	obj.generate_xml(xml, "");

	// verify xml
	xmlDoc* doc = xmlReadMemory(xml.c_str(),
					xml.size(),
					"noname.xml",
					NULL,
					XML_PARSE_NONET | XML_PARSE_NOERROR | XML_PARSE_NOWARNING);
	if (!doc) {
		//cout << xml << endl;
		throw String("generateXML(): internal error");
	}
	xmlFreeDoc(doc);

	return xml;
}

XMLObject
readXML(const String& filename)
{
	return parseXML(File::open(filename));
}

String
escape_chars(const String& str)
{
	const String amp_repl("______AMP_REPLACEMENT_XML_KOJIKOJIKOJIKO______");
	const String lt_repl("______LT_REPLACEMENT_XML_KOJIKOJIKOJIKO______");
	const String gt_repl("______GT_REPLACEMENT_XML_KOJIKOJIKOJIKO______");
	const String apos_repl("______APOS_REPLACEMENT_XML_KOJIKOJIKOJIKO______");
	const String quot_repl("______QUOT_REPLACEMENT_XML_KOJIKOJIKOJIKO______");

	String ret = utils::replace("&amp;", amp_repl, str);
	ret = utils::replace("&lt;", lt_repl, ret);
	ret = utils::replace("&gt;", gt_repl, ret);
	ret = utils::replace("&apos;", apos_repl, ret);
	ret = utils::replace("&quot;", quot_repl, ret);

	ret = utils::replace("&", "&amp;", ret);
	ret = utils::replace("<", "&lt;", ret);
	ret = utils::replace(">", "&gt;", ret);
	ret = utils::replace("'", "&apos;", ret);
	ret = utils::replace("\"", "&quot;", ret);

	ret = utils::replace(amp_repl, "&amp;", ret);
	ret = utils::replace(lt_repl, "&lt;", ret);
	ret = utils::replace(gt_repl, "&gt;", ret);
	ret = utils::replace(apos_repl, "&apos;", ret);
	ret = utils::replace(quot_repl, "&quot;", ret);

	return ret;
}

String
invert_chars(const String& str)
{
	String ret = utils::replace("&amp;", "&", str);
	ret = utils::replace("&lt;", "<", ret);
	ret = utils::replace("&gt;", ">", ret);
	ret = utils::replace("&apos;", "'", ret);
	ret = utils::replace("&quot;", "\"", ret);
	return ret;
}
