/*
** Copyright (C) Red Hat, Inc. 2006-2009
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

#ifndef __CONGA_MODLOG_LOGPARSER_H
#define __CONGA_MODLOG_LOGPARSER_H

#include "XML.h"

#include "String.h"
#include <set>
#include <list>

class LogEntry
{
	public:
		LogEntry(const String& entry);

		long long age;
		String domain;
		String msg;
		String pid;

		std::set<String> tags;
		std::set<String> matched_tags;

		XMLObject xml() const;

		bool operator < (const LogEntry&) const;

	private:
		String compare_str() const;
};

class LogParser
{
	public:
		std::set<LogEntry> get_entries(	long long age,
										const std::list<String>& tags,
										const std::list<String>& paths);
};

#endif
