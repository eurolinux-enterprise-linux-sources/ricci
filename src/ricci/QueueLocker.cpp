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

#include "QueueLocker.h"
#include "ricci_defines.h"

#include <unistd.h>
#include <sys/file.h>
#include <errno.h>

#include "String.h"

using namespace std;

static Mutex q_lock;
static int q_counter = 0;
static int fd;

QueueLocker::QueueLocker() :
	MutexLocker(q_lock)
{
	if (q_counter++)
		return;

	try {
		fd = open(QUEUE_LOCK_PATH, O_RDONLY | O_CREAT, 0640);
		if (fd == -1) {
			throw String("unable to open queue lock file: ")
					+ String(strerror(errno));
		}

		// acquire flock
		int res;
		while ((res = flock(fd, LOCK_EX))) {
			if (errno != EINTR) {
				int err = errno;
				while ((res = close(fd))) {
					if (errno != EINTR) {
						throw String("unable to close the queue lock file: ") +
								String(strerror(errno));
					}
				}
				throw String("unable to lock the queue: ") +
						String(strerror(err));
			}
		}
	} catch ( ... ) {
		q_counter--;
		throw;
	}
}

QueueLocker::~QueueLocker()
{
	if (!--q_counter) {
		// release flock

		int res;
		while ((res = close(fd))) {
			if (errno != EINTR) {
				while ((res = flock(fd, LOCK_UN))) {
					if (errno != EINTR)
						break; // throw String("unable to unlock the queue");
					break; // throw String("unable to close the queue lock file");
				}
			}
		}
	}
}
