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

#include "Variable.h"
#include "XML_tags.h"
#include "utils.h"

#include <stdio.h>

#include <vector>
#include <algorithm>
using namespace std;

// ##### class Variable #####

Variable::Variable(const XMLObject& xml)
{
	if (xml.tag() != VARIABLE_TAG)
		throw String("not a variable");

	_name = xml.get_attr("name");
	if (_name == "")
		throw String("invalid variable name");

	_mutable = (xml.get_attr("mutable") == "true");

	//_validator = Validator(xml); // incoming constraints are not to be trusted anyhow

	// conditionals
	_cond_bool_if = xml.get_attr("if_bool");
	_cond_bool_ifnot = xml.get_attr("ifnot_bool");

	String type(xml.get_attr("type"));
	if (type == VARIABLE_INT) {
		_type = Integer;
		_val_int = utils::to_long(xml.get_attr("value").c_str());
	} else if (type == VARIABLE_INT_SEL) {
		_type = IntSel;
		_val_int = utils::to_long(xml.get_attr("value").c_str());
	} else if (type == VARIABLE_BOOL) {
		_type = Boolean;
		_val_bool = (xml.get_attr("value") == "true");
	} else if (type == VARIABLE_STR) {
		_type = StringVar;
		_val_str = xml.get_attr("value");
	} else if (type == VARIABLE_STR_SEL) {
		_type = StrSel;
		_val_str = xml.get_attr("value");
	} else if (type == VARIABLE_XML) {
		_type = XMLVar;
		if (xml.children().empty())
			throw String("variable missing XML value");
		else
			_val_xml = xml.children().front();
	} else if (type == VARIABLE_LIST_INT) {
		_type = ListInt;
		for (list<XMLObject>::const_iterator
				iter = xml.children().begin() ;
				iter != xml.children().end() ;
				iter++)
		{
			const XMLObject& node = *iter;

			if (node.tag() == VARIABLE_LISTENTRY)
				_val_list_int.push_back(utils::to_long(node.get_attr("value").c_str()));
		}
	} else if (type == VARIABLE_LIST_STR) {
		_type = ListStr;
		for (list<XMLObject>::const_iterator
				iter = xml.children().begin() ;
				iter != xml.children().end() ;
				iter++)
		{
			const XMLObject& node = *iter;

			if (node.tag() == VARIABLE_LISTENTRY)
				_val_list_str.push_back(node.get_attr("value"));
		}
	} else if (type == VARIABLE_LIST_XML) {
		_type = ListXML;
		for (list<XMLObject>::const_iterator
				iter = xml.children().begin() ;
				iter != xml.children().end() ;
				iter++)
		{
			_val_list_XML.push_back(*iter);
		}
	} else
		throw String("invalid variable type");
}

// integer
Variable::Variable(const String& name, long long value) :
	_name(name),
	_type(Integer),
	_mutable(false)
{
	set_value(value);
}

Variable::Variable(	const String& name,
					long long value,
					long long min,
					long long max,
					long long step) :
	_name(name),
	_type(Integer),
	_mutable(true),
	_validator(min, max, step)
{
	set_value(value);
}

// integer selector
Variable::Variable(	const String& name,
					long long value,
					const std::list<long long>& valid_values) :
	_name(name),
	_type(IntSel),
	_mutable(true),
	_validator(valid_values)
{
	set_value(value);
}

// integer list
Variable::Variable(	const String& name,
					const std::list<long long>& value,
					bool mutabl) :
	_name(name),
	_type(ListInt),
	_mutable(mutabl)
{
	set_value(value);
}


// boolean
Variable::Variable(const String& name, bool value, bool mutabl) :
	_name(name),
	_type(Boolean),
	_mutable(mutabl)
{
	set_value(value);
}

// string
Variable::Variable(const String& name, const String& value) :
	_name(name),
	_type(StringVar),
	_mutable(false)
{
	set_value(value);
}

Variable::Variable(	const String& name,
					const String& value,
					long long min_length,
					long long max_length,
					const String& illegal_chars,
					const std::list<String>& reserved_words) :
	_name(name),
	_type(StringVar),
	_mutable(true),
	_validator(min_length, max_length, illegal_chars, reserved_words)
{
	set_value(value);
}

// string selector
Variable::Variable(	const String& name,
					const String& value,
					const std::list<String>& valid_values) :
	_name(name),
	_type(StrSel),
	_mutable(true),
	_validator(valid_values)
{
	set_value(value);
}

// string list
Variable::Variable(	const String& name,
					const std::list<String>& value,
					bool mutabl) :
	_name(name),
	_type(ListStr),
	_mutable(mutabl)
{
	set_value(value);
}


// XML
Variable::Variable(const String& name, const XMLObject& value) :
	_name(name),
	_type(XMLVar),
	_mutable(false)
{
	set_value(value);
}

// XML list
Variable::Variable(const String& name, const std::list<XMLObject>& value) :
	_name(name),
	_type(ListXML),
	_mutable(false)
{
	set_value(value);
}

Variable::~Variable()
{}

void
Variable::set_conditional_bool_if(const String& bool_name)
{
	if (name() == bool_name)
		throw String("circular conditional: ") + bool_name;
	_cond_bool_if = bool_name;
}

void
Variable::set_conditional_bool_ifnot(const String& bool_name)
{
	if (name() == bool_name)
		throw String("circular conditional: ") + bool_name;
	_cond_bool_ifnot = bool_name;
}

long long
Variable::get_int() const
{
	if (_type != Integer && _type != IntSel) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_INT + " type";
	}
	return _val_int;
}

void
Variable::set_value(long long value)
{
	if (_type != Integer && _type != IntSel) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_INT + " type";
	}
	_validator.validate(value);
	_val_int = value;
}

bool
Variable::get_bool() const
{
	if (_type != Boolean) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_BOOL + " type";
	}
	return _val_bool;
}

void
Variable::set_value(bool value)
{
	if (_type != Boolean) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_BOOL + " type";
	}
	_validator.validate(value);
	_val_bool = value;
}

String
Variable::get_string() const
{
	if (_type != StringVar && _type != StrSel) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_STR + " type";
	}
	return _val_str;
}

void
Variable::set_value(const String& value)
{
	if (_type != StringVar && _type != StrSel) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_STR + " type";
	}
	_validator.validate(value);
	_val_str = value;
}

XMLObject
Variable::get_XML() const
{
	if (_type != XMLVar) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_XML + " type";
	}
	return _val_xml;
}

void
Variable::set_value(const XMLObject& value)
{
	if (_type != XMLVar) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_XML + " type";
	}
	_validator.validate(value);
	_val_xml = value;
}

std::list<long long>
Variable::get_list_int() const
{
	if (_type != ListInt) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_LIST_INT + " type";
	}
	return _val_list_int;
}

void
Variable::set_value(const std::list<long long>& value)
{
	if (_type != ListInt) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_LIST_INT + " type";
	}
	_validator.validate(value);
	_val_list_int = value;
}

std::list<String>
Variable::get_list_str() const
{
	if (_type != ListStr) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_LIST_STR + " type";
	}
	return _val_list_str;
}

void
Variable::set_value(const std::list<String>& value)
{
	if (_type != ListStr) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_LIST_STR + " type";
	}
	_validator.validate(value);
	_val_list_str = value;
}

std::list<XMLObject>
Variable::get_list_XML() const
{
	if (_type != ListXML) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_LIST_XML + " type";
	}
	return _val_list_XML;
}

void
Variable::set_value(const std::list<XMLObject>& value)
{
	if (_type != ListXML) {
		throw String("variable ") + name() + " is not of "
				+ VARIABLE_LIST_XML + " type";
	}
	_validator.validate(value);
	_val_list_XML = value;
}

bool
Variable::equal(const Variable& var) const
{
	if (type() != var.type() ||
		name() != var.name() ||
		get_conditional_bool_if() != var.get_conditional_bool_if() ||
		get_conditional_bool_ifnot() != var.get_conditional_bool_ifnot())
	{
		return false;
	}

	switch (var.type()) {
		case Integer:
		case IntSel:
			return get_int() == var.get_int();

		case Boolean:
			return get_bool() == var.get_bool();

		case StringVar:
		case StrSel:
			return get_string() == var.get_string();

		case XMLVar:
			return get_XML() == var.get_XML();

		case ListInt:
			return get_list_int() == var.get_list_int();

		case ListStr:
			return get_list_str() == var.get_list_str();

		default:
			return false;
	}
	return false;
}

bool
Variable::validate() const
{
	return validate(*this);
}

bool
Variable::validate(const Variable& var) const
{
	if (name() != var.name())
		throw String("different variable names");
	if (type() != var.type())
		throw String("invalid variable type");

	if (get_conditional_bool_if() != var.get_conditional_bool_if() ||
		get_conditional_bool_ifnot() != var.get_conditional_bool_ifnot())
	{
		throw String("invalid bool conditional");
	}

	switch (var.type()) {
		case Integer:
		case IntSel:
			return _validator.validate(var.get_int());

		case Boolean:
			return _validator.validate(var.get_bool());

		case StringVar:
		case StrSel:
			return _validator.validate(var.get_string());

		case XMLVar:
			return _validator.validate(var.get_XML());

		case ListInt:
			return _validator.validate(var.get_list_int());

		case ListStr:
			return _validator.validate(var.get_list_str());

		default:
			return false;
	}
}

XMLObject
Variable::xml() const
{
	XMLObject xml(VARIABLE_TAG);

	xml.set_attr("name", name());
	xml.set_attr("mutable", (_mutable) ? "true" : "false");

	int i = 0;
	switch (_type) {
		case Integer:
			xml.set_attr("type", VARIABLE_INT);
			xml.set_attr("value", utils::to_string(_val_int));
			break;

		case IntSel:
			xml.set_attr("type", VARIABLE_INT_SEL);
			xml.set_attr("value", utils::to_string(_val_int));
			break;

		case Boolean:
			xml.set_attr("type", VARIABLE_BOOL);
			xml.set_attr("value", utils::to_string(_val_bool));
			break;

		case StringVar:
			xml.set_attr("type", VARIABLE_STR);
			xml.set_attr("value", _val_str);
			break;

		case StrSel:
			xml.set_attr("type", VARIABLE_STR_SEL);
			xml.set_attr("value", _val_str);
			break;

		case XMLVar:
			xml.set_attr("type", VARIABLE_XML);
			xml.add_child(_val_xml);
			break;

		case ListInt:
			xml.set_attr("type", VARIABLE_LIST_INT);
			i = 0;
			for (list<long long>::const_iterator
					iter = _val_list_int.begin() ;
					iter != _val_list_int.end() ;
					iter++, i++)
			{
				XMLObject xml_t = XMLObject(VARIABLE_LISTENTRY);
				//xml_t.set_attr("index", utils::to_string(i));
				xml_t.set_attr("value", utils::to_string(*iter));
				xml.add_child(xml_t);
			}
			break;

		case ListStr:
			xml.set_attr("type", VARIABLE_LIST_STR);

			i = 0;
			for (list<String>::const_iterator
					iter = _val_list_str.begin() ;
					iter != _val_list_str.end() ;
					iter++, i++)
			{
				XMLObject xml_t = XMLObject(VARIABLE_LISTENTRY);
				//xml_t.set_attr("index", utils::to_string(i));
				xml_t.set_attr("value", *iter);
				xml.add_child(xml_t);
			}
			break;

		case ListXML:
			xml.set_attr("type", VARIABLE_LIST_XML);
			i = 0;
			for (list<XMLObject>::const_iterator
					iter = _val_list_XML.begin() ;
					iter != _val_list_XML.end() ;
					iter++, i++)
			{
				xml.add_child(*iter);
			}
			break;

		default:
			throw String("invalid variable type");
			break;
	}

	if (_mutable)
		_validator.export_params(xml);

	if (!_cond_bool_if.empty())
		xml.set_attr("if_bool", _cond_bool_if);
	if (!_cond_bool_ifnot.empty())
		xml.set_attr("ifnot_bool", _cond_bool_ifnot);

	return xml;
}


// ##### class Validator #####


// always valid
Validator::Validator() :
	_always_valid(true),
	_integer(false),
	_int_sel(false),
	_string(false),
	_string_sel(false)
{}

// integer
Validator::Validator(long long min, long long max, long long step) :
	_always_valid(false),
	_integer(true),
	_int_sel(false),
	_string(false),
	_string_sel(false)
{
	_min = min;
	_max = max;
	_step = step;
}

// integer selector
Validator::Validator(const std::list<long long>& valid_values) :
	_always_valid(false),
	_integer(false),
	_int_sel(true),
	_string(false),
	_string_sel(false)
{
	_valid_ints = valid_values;
}

// string
Validator::Validator(	long long min_length,
						long long max_length,
						const String& illegal_chars,
						const std::list<String>& reserved_words) :
	_always_valid(false),
	_integer(false),
	_int_sel(false),
	_string(true),
	_string_sel(false)
{
	_min_length = min_length;
	_max_length = max_length;
	_illegal_chars = illegal_chars;
	_reserved_words = reserved_words;
}

// string selector
Validator::Validator(const std::list<String>& valid_words) :
	_always_valid(false),
	_integer(false),
	_int_sel(false),
	_string(false),
	_string_sel(true)
{
	_valid_words = valid_words;
}

Validator::~Validator()
{}

bool
Validator::validate(long long value) const
{
	if (_always_valid)
		return true;
	else if (_integer) {
		if (value >= _min && value <= _max && value % _step == 0)
			return true;
		else
			return false;
	} else if (_int_sel) {
		if (find(_valid_ints.begin(), _valid_ints.end(), value) == _valid_ints.end())
			return false;
		else
			return true;
	} else
		throw String("not long long");
}

bool
Validator::validate(const String& value) const
{
	if (_always_valid)
		return true;
	else if (_string) {
		if ((long long) value.size() >= _min_length &&
			(long long) value.size() <= _max_length &&
			value.find_first_of(_illegal_chars) == value.npos &&
			find(_reserved_words.begin(), _reserved_words.end(), value) ==
				_reserved_words.end())
		{
			return true;
		} else
			return false;
	} else if (_string_sel) {
		if (find(_valid_words.begin(), _valid_words.end(), value) == _valid_words.end())
			return false;
		else
			return true;
	} else
		throw String("not string");
}

bool
Validator::validate(bool value) const
{
	if (_always_valid)
		return true;

	return false;
}

bool
Validator::validate(const XMLObject& value) const
{
	if (_always_valid)
		return true;

	return false;
}

bool
Validator::validate(const std::list<long long>& value) const
{
	if (_always_valid)
		return true;

	return false;
}

bool
Validator::validate(const std::list<String>& value) const
{
	if (_always_valid)
		return true;

	return false;
}

bool
Validator::validate(const std::list<XMLObject>& value) const
{
	if (_always_valid)
		return true;

	return false;
}

void
Validator::export_params(XMLObject& xml) const
{
	if (_integer) {
		xml.set_attr("min", utils::to_string(_min));
		xml.set_attr("max", utils::to_string(_max));
		xml.set_attr("step", utils::to_string(_step));
	} else if (_int_sel) {
		for (list<long long>::const_iterator
				iter = _valid_ints.begin() ;
				iter != _valid_ints.end() ;
				iter++)
		{
			XMLObject entry("listentry");
			entry.set_attr("value", utils::to_string(*iter));
			xml.add_child(entry);
		}
	} else if (_string) {
		xml.set_attr("min_length", utils::to_string(_min_length));
		xml.set_attr("max_length", utils::to_string(_max_length));
		xml.set_attr("illegal_chars", _illegal_chars);
		String reserved;
		for (list<String>::const_iterator
				iter = _reserved_words.begin() ;
				iter != _reserved_words.end() ;
				iter++)
		{
			if (!reserved.empty())
				reserved += ";";
			reserved += *iter;
		}
		xml.set_attr("reserved_words", reserved);
	} else if (_string_sel) {
		for (list<String>::const_iterator
				iter = _valid_words.begin() ;
				iter != _valid_words.end() ;
				iter++)
		{
			XMLObject entry("listentry");
			entry.set_attr("value", *iter);
			xml.add_child(entry);
		}
	}
}
