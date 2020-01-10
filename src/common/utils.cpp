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

#include "utils.h"
#include "executils.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <openssl/md5.h>

//#include <iostream>

using namespace std;

String
utils::replace(const String& what, const String& with, const String& in_str)
{
	vector<String> v(split(in_str, what));
	String ret(v[0]);

	for (vector<String>::size_type i = 1 ; i < v.size() ; i++)
		ret += with + v[i];
	return ret;
}

String
utils::hash_str(const String& txt)
{
	unsigned char buff[16];
	MD5((const unsigned char*) txt.c_str(), txt.size(), buff);

	String hash;
	for (size_t i = 0; i < sizeof(buff) ; i++) {
		hash += (char) ('a' + (int) ((buff[i] & 0xf0) >> 4));
		hash += (char) ('a' + (int) ((buff[i] & 0x0f) >> 4));
	}
	return hash;
}

String
utils::lstrip(String str, const String& del)
{
	if (del.empty())
		throw String("empty separator");

	while (str.find(del) == 0) {
		str = str.substr(del.size());
	}

	return str;
}

String
utils::rstrip(String str, const String& del)
{
	if (del.empty())
		throw String("empty separator");

	if (str.size() < del.size())
		return str;

	unsigned int i;
	while (str.rfind(del) == (i = (str.size() - del.size()))) {
		if (str.rfind(del) == str.npos)
			break;
		str = str.substr(0, i);
	}

	return str;
}

String
utils::lstrip(String str)
{
	while (str.find_first_of(" \n\t") == 0)
		str = str.substr(1);

	return str;
}

String
utils::rstrip(String str)
{
	unsigned int i;

	while ((i = str.size()) != 0) {
		i--;
		if (str[i] == ' ' || str[i] == '\n' || str[i] == '\t')
			str = str.substr(0, i);
		else
			break;
	}

	return str;
}

vector<String>
utils::split(const String& t, const String& del)
{
	if (del.empty())
		throw String("empty separator");

	String txt(t);

	// merge separators
	if (del == " " || del == "\n") {
		String::size_type i;
		while ((i = txt.find(del + del)) != txt.npos)
			txt.erase(i, del.size());
	}

	// split
	vector<String> lines;
	for (String::size_type from=0, to = txt.find(del) ; from != txt.size() ;) {
		String substr = txt.substr(from, to - from);
		lines.push_back(substr);
		if (to == txt.npos)
			return lines;
		from = to + del.size();
		to = txt.find(del, from);
	}

	lines.push_back(String());
	return lines;
}

vector<String>
utils::split(const String& t)
{
	String del(" ");
	String txt(t);

	// merge separators
	String::size_type i;
	while ((i = txt.find('\t')) != txt.npos)
		txt[i] = ' ';

	while ((i = txt.find(del + del)) != txt.npos)
		txt.erase(i, del.size());

	// split
	vector<String> lines;
	for (String::size_type from=0, to = txt.find(del) ; from != txt.size() ;) {
		String substr = txt.substr(from, to - from);
		lines.push_back(substr);
		if (to == txt.npos)
			return lines;
		from = to + del.size();
		to = txt.find(del, from);
	}

	return lines;
}

String
utils::to_lower(const String& str)
{
	String s;

	for (String::size_type i = 0 ; i < str.size() ; i++)
		s.push_back(tolower(str[i]));
	return s;
}

String
utils::to_upper(const String& str)
{
	String s;

	for (String::size_type i = 0 ; i < str.size() ; i++)
		s.push_back(toupper(str[i]));
	return s;
}

/*
int
utils::to_int(const String& str)
{
	return atoi(str.c_str());
}
*/

long long
utils::to_long(const String& str)
{
	char *p = NULL;
	long long ret;
	ret = strtoll(strip(str).c_str(), &p, 10);
	if (p != NULL && *p != '\0')
		throw String("Not a number: ") + str;
	if (ret == LLONG_MIN && errno == ERANGE)
		throw String("Numeric underflow: ") + str;
	if (ret == LLONG_MAX && errno == ERANGE)
		throw String("Numeric overflow: ") + str;
	return ret;
}

float
utils::to_float(const String& str)
{
	char *p = NULL;
	float ret;

	ret = strtof(strip(str).c_str(), &p);
	if (p != NULL && *p == '\0')
		throw String("Invalid floating point number: ") + str;
	if (ret == 0 && errno == ERANGE)
		throw String("Floating point underflow: ") + str;
	if ((ret == HUGE_VALF || ret == HUGE_VALL) && errno == ERANGE)
		throw String("Floating point overflow: ") + str;

	return ret;
}

String
utils::to_string(int value)
{
	char tmp[64];
	int ret;

	ret = snprintf(tmp, sizeof(tmp), "%d", value);
	if (ret < 0 || (size_t) ret >= sizeof(tmp))
		throw String("Invalid integer");
	return tmp;
}

String
utils::to_string(long value)
{
	char tmp[64];
	int ret;

	ret = snprintf(tmp, sizeof(tmp), "%ld", value);
	if (ret < 0 || (size_t) ret >= sizeof(tmp))
		throw String("Invalid long integer");
	return tmp;
}

String
utils::to_string(long long value)
{
	char tmp[64];
	int ret;

	ret = snprintf(tmp, sizeof(tmp), "%lld", value);
	if (ret < 0 || (size_t) ret >= sizeof(tmp))
		throw String("Invalid long long integer");
	return tmp;
}

String
utils::to_string(bool value)
{
	return (value) ? "true" : "false";
}

int
utils::execute(	const String& path,
				const std::vector<String>& args,
				String& out,
				String& err,
				int& status,
				bool caching)
{
	String _command = path;
	for (vector<String>::const_iterator
			iter = args.begin() ;
			iter != args.end() ;
			iter++)
	{
		_command += " " + *iter;
	}

	map<String, exec_cache>::iterator iter = cache.find(_command);
	if (iter != cache.end() && caching) {
		exec_cache &c = iter->second;
		//cout << "exec: " << _command << " cached" << endl;

		out = c.out;
		err = c.err;
		status = c.status;

		return c.exec_ret;
	} else {
		int ret = ::execute(path, args, out, err, status);
		//cout << "exec: " << _command << " executed" << endl;
		exec_cache c(_command, out, err, status, ret);

		if (caching)
			cache.insert(pair<String, exec_cache>(_command, c));

		out = c.out;
		err = c.err;
		status = c.status;

		return c.exec_ret;
	}
}

void
utils::clear_cache()
{
	cache.clear();
}

std::map<String, exec_cache> utils::cache;
