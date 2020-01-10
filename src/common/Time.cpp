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

#include "Time.h"

#include <errno.h>
#include <sys/time.h>

unsigned int time_sec() {
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec;
}

unsigned int time_mil() {
	struct timeval t;
	gettimeofday(&t, NULL);

	return t.tv_sec * 1000 + t.tv_usec / 1000;
}

String time_formated() {
	char time[64];
	time_t t = time_sec();

	ctime_r(&t, time);
	String m(time);
	return m.substr(0, m.size() - 1);
}

void sleep_mil(unsigned int num) {
	struct timespec req, rem;

	rem.tv_sec = 0;
	rem.tv_nsec = num * 1000 * 1000;

	do {
		req.tv_sec = rem.tv_sec;
		req.tv_nsec = rem.tv_nsec;
	} while (nanosleep(&req, &rem) == -1 && errno == EINTR);
}

void sleep_sec(unsigned int num) {
	sleep_mil(1000 * num);
}
