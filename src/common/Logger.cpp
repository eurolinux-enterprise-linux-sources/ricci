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

#include "Logger.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

extern "C" {
	#include "sys_util.h"
}

#include "String.h"

using namespace std;

static counting_auto_ptr<Logger> logger(new Logger());

Logger::Logger() :
	_fd(-1),
	_domain_c(NULL)
{}

Logger::Logger(	const String& filepath,
				const String& domain,
				LogLevel level) :
	_level(level)
{
	const char *c_str = domain.c_str();
	const char *path_c = filepath.c_str();

	_domain_c = (char *) malloc(domain.size() + 1);
	if (_domain_c == NULL)
		throw String("Logger::Logger(): malloc() failed");
	strcpy(_domain_c, c_str);

	_fd = open(path_c, O_CREAT | O_WRONLY | O_APPEND, 0640);
	if (_fd == -1) {
		free(_domain_c);
		throw String("Logger::Logger(): open() failed");
	}
}

Logger::Logger(int fd, const String& domain, LogLevel level) :
	_fd(fd),
	_level(level)
{
	const char *c_str = domain.c_str();

	_domain_c = (char *) malloc(domain.size() + 1);
	if (_domain_c == NULL) {
		close_fd();
		throw String("Logger::Logger(): malloc() failed");
	}
	strcpy(_domain_c, c_str);
}

void
Logger::close_fd()
{
	if (_fd > -1)
		fsync(_fd);

	if (_fd > 2) {
		int e;
		do {
			e = close(_fd);
		} while (e == -1 && (errno == EINTR));
		_fd = -1;
	}
}

Logger::~Logger()
{
	close_fd();
	free(_domain_c);
}

ssize_t
Logger::log(const String& msg, LogLevel level)
{
	return log_sigsafe(msg.c_str(), level);
}

ssize_t
Logger::log_sigsafe(const char *msg, LogLevel level)
{
	if (_fd > 0 && (_level & level)) {
		char cur_time[64];
		time_t t = time(NULL);
		char buf[4096];
		char *p;
		int ret;
		size_t buflen;

		ctime_r(&t, cur_time);
		cur_time[sizeof(cur_time) - 1] = '\0';

		p = strchr(cur_time, '\n');
		if (p != NULL)
			*p = '\0';

		if (_fd > 2 && _domain_c != NULL) {
			ret = snprintf(buf, sizeof(buf), "%s %s: %s\n",
					cur_time, _domain_c, msg);
		} else
			ret = snprintf(buf, sizeof(buf), "%s: %s\n", cur_time, msg);

		if (ret < 0)
			return -ENOMEM;
		buflen = (size_t) ret;

		if (buflen >= sizeof(buf)) {
			buf[sizeof(buf) - 1] = '\0';
			buflen = strlen(buf);
		}
		return write_restart(_fd, buf, buflen);
	}

	return 0;
}


// ### helper functions ###

String
operator+ (const String& s, int i)
{
	char buff[128];
	snprintf(buff, sizeof(buff), "%d", i);
	return s + buff;
}

String
operator+ (int i, const String& s)
{
	char buff[128];
	snprintf(buff, sizeof(buff), "%d", i);
	return String(buff) + s;
}

ssize_t
log(const String& msg, LogLevel level)
{
	return logger->log(msg, level);
}

ssize_t
log_sigsafe(const char *msg, LogLevel level)
{
	return logger->log_sigsafe(msg, level);
}

void
set_logger(counting_auto_ptr<Logger> l)
{
	if (l.get() == NULL)
		l = counting_auto_ptr<Logger>(new Logger());
	logger = l;
}
