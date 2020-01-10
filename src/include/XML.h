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

#ifndef __CONGA_XML_H
#define __CONGA_XML_H

#include "String.h"
#include <map>
#include <list>

class XMLObject
{
	public:
		XMLObject(const String& elem_name = "TagName");
		virtual ~XMLObject();

		String tag() const {
			return _tag;
		};

		// attributes
		bool has_attr(const String& attr_name) const;

		// return old value
		String set_attr(const String& attr_name, const String& value);

		String get_attr(const String& attr_name) const;

		const std::map<String, String>& attrs() const {
			return _attrs;
		}

		// kids
		XMLObject& add_child(const XMLObject& child);
		bool remove_child(const XMLObject& child);

		const std::list<XMLObject>& children() const {
			return _kids;
		}

		bool operator== (const XMLObject&) const;

	private:
		String _tag;
		std::list<XMLObject> _kids;
		std::map<String, String> _attrs;
		void generate_xml(String& xml, const String& indent) const;
	friend String generateXML(const XMLObject& obj);
};


XMLObject readXML(const String& filename);
XMLObject parseXML(const String& xml);
String generateXML(const XMLObject& obj);

#endif
