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

#ifndef __CONGA_VARIABLE_H
#define __CONGA_VARIABLE_H

#include "String.h"
#include <list>
#include "XML.h"

enum VarType {
	Integer		= 1,
	IntSel		= 2, // integer selector
	Boolean		= 3,
	StringVar	= 4,
	StrSel		= 5, // string selector
	XMLVar		= 6,
	ListInt		= 7,
	ListStr		= 8,
	ListXML		= 9
};

class Validator
{
	public:
		Validator(); // always valid

		// integer
		Validator(long long min, long long max, long long step);

		// integer selector
		Validator(const std::list<long long>& valid_values);

		// string
		Validator(	long long min_length,
					long long max_length,
					const String& illegal_chars,
					const std::list<String>& reserved_words);

		// string selector
		Validator(const std::list<String>& valid_words);

		virtual ~Validator();

		bool validate(long long value) const;
		bool validate(const String& value) const;
		bool validate(bool value) const;
		bool validate(const XMLObject& value) const;
		bool validate(const std::list<long long>& value) const;
		bool validate(const std::list<String>& value) const;
		bool validate(const std::list<XMLObject>& value) const;

		void export_params(XMLObject& xml) const;

	private:
		bool _always_valid;
		bool _integer;
		long long _min;
		long long _max;
		long long _step;
		bool _int_sel;
		std::list<long long> _valid_ints;
		bool _string;
		long long _min_length;
		long long _max_length;
		String _illegal_chars;
		std::list<String> _reserved_words;
		bool _string_sel;
		std::list<String> _valid_words;
};

class Variable
{
	public:
		// integer
		Variable(const String& name, long long value);
		Variable(	const String& name,
					long long value,
					long long min,
					long long max,
					long long step);

		// integer selector
		Variable(	const String& name,
					long long value,
					const std::list<long long>& valid_values);

		// integer list
		Variable(	const String& name,
					const std::list<long long>& value,
					bool mutabl=false);


		// boolean
		Variable(const String& name, bool value, bool mutabl=false);

		// string
		Variable(const String& name, const String& value);
		Variable(	const String& name,
					const String& value,
					long long min_length,
					long long max_length,
					const String& illegal_chars,
					const std::list<String>& reserved_words);

		// string selector
		Variable(	const String& name,
					const String& value,
					const std::list<String>& valid_words);

		// string list
		Variable(	const String& name,
					const std::list<String>& value,
					bool mutabl=false);

		// xml
		Variable(const String& name, const XMLObject& value);

		// xml list
		Variable(const String& name, const std::list<XMLObject>& value);

		Variable(const XMLObject& xml);

		virtual ~Variable();

		String name() const { return _name; }
		VarType type() const { return _type; }
		bool mutabl() const { return _mutable; }
		void mutabl(bool mutabl) { _mutable = mutabl; }

		bool validate() const;
		bool validate(const Variable& var) const; // validate var against self

		bool equal(const Variable& var) const;

		XMLObject xml() const;

		void set_conditional_bool_if(const String& bool_name);
		String get_conditional_bool_if() const {
			return _cond_bool_if;
		}

		void set_conditional_bool_ifnot(const String& bool_name);
		String get_conditional_bool_ifnot() const {
			return _cond_bool_ifnot;
		}

		// values getters and setters

		void set_value(long long value);
		void set_value(bool value);
		void set_value(const String& value);
		void set_value(const XMLObject& value);
		void set_value(const std::list<long long>& value);
		void set_value(const std::list<String>& value);
		void set_value(const std::list<XMLObject>& value);

		long long get_int() const;
		bool get_bool() const;
		String get_string() const;
		XMLObject get_XML() const;
		std::list<long long> get_list_int() const;
		std::list<String> get_list_str() const;
		std::list<XMLObject> get_list_XML() const;

	private:
		String _name;
		VarType _type;

		long long _val_int;
		bool _val_bool;
		String _val_str;
		XMLObject _val_xml;
		std::list<long long> _val_list_int;
		std::list<String> _val_list_str;
		std::list<XMLObject> _val_list_XML;

		bool _mutable;

		String _cond_bool_if;
		String _cond_bool_ifnot;

		Validator _validator;
};

#endif
