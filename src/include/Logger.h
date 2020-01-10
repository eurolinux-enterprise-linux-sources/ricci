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

#ifndef __CONGA_LOGGER_H
#define __CONGA_LOGGER_H

#include "counting_auto_ptr.h"
#include "String.h"

enum LogLevel {
	LogNone			= 0x00000000,
	LogBasic		= 0x00000001,
	LogMonitor		= 0x00000002,
	LogSocket		= 0x00000004,
	LogCommunicator	= 0x00000008,
	LogTransfer		= 0x00000010,
	LogExit			= 0x00000020,
	LogThread		= 0x00000040,
	LogTime			= 0x00000080,
	LogExecute		= 0x00000100,
	LogExecFail		= 0x00000200,
	LogDebug		= 0x00000400,
	LogAll			= 0xffffffff
};

class Logger
{
	public:
		Logger();
		Logger(const String& filepath, const String& domain, LogLevel level);
		Logger(int fd, const String& domain, LogLevel level);
		virtual ~Logger();

		ssize_t log(const String& msg, LogLevel level=LogBasic);
		ssize_t log_sigsafe(const char* msg, LogLevel level=LogBasic);
		void operator<< (const String& msg) { log(msg); }
		unsigned int get_mask(void) { return _level; }

	private:
		int _fd;
		char *_domain_c;
		unsigned int _level;

		void close_fd();

		Logger(const Logger&);
		Logger& operator= (const Logger&);
};

// helper functions
String operator+ (const String&, int);
String operator+ (int, const String&);
ssize_t log(const String& msg, LogLevel level=LogBasic);
ssize_t log_sigsafe(const char* msg, LogLevel level=LogBasic);
void set_logger(counting_auto_ptr<Logger>);

#endif
