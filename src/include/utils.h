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

#ifndef __CONGA_UTILS_H
#define __CONGA_UTILS_H

#include <vector>
#include "String.h"
#include <map>

#ifndef arr_elem
#	define arr_elem(x) (sizeof((x)) / sizeof((x)[0]))
#endif

class exec_cache
{
	public:
		exec_cache(	const String& command,
					const String& out,
					const String& err,
					int status,
					int exec_ret) :
			command(command),
			out(out),
			err(err),
			status(status),
			exec_ret(exec_ret) {}

		const String command;
		const String out;
		const String err;
		const int status;
		const int exec_ret;
};

class utils
{
	public:
		static String replace(	const String& what,
								const String& with,
								const String& in_str);

		static String hash_str(const String& txt);

		static String strip(String str)
		{
			return rstrip(lstrip(str));
		}
		static String lstrip(String str);
		static String rstrip(String str);

		static String strip(String str, const String& del)
		{
			return rstrip(lstrip(str, del), del);
		}
		static String lstrip(String str, const String& del);
		static String rstrip(String str, const String& del);

		static std::vector<String> split(const String& str, const String& del);
		static std::vector<String> split(const String& str);

		static String to_lower(const String& str);
		static String to_upper(const String& str);

		//static int to_int(const String& str);
		static long long to_long(const String& str);
		static float to_float(const String& str);

		static String to_string(int value);
		static String to_string(long value);
		static String to_string(long long value);
		static String to_string(bool value);

		static int execute(	const String& path,
							const std::vector<String>& args,
							String& out,
							String& err,
							int& status,
							bool caching=true);
		static void clear_cache();
		static std::map<String, exec_cache> cache;
};

inline String
command_not_found_error_msg(const String& command)
{
	return String("command \"") + command + "\" not found/not executable";
}

#endif
