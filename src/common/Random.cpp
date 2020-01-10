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

#include "Random.h"
#include "Mutex.h"
#include "Time.h"

#include <unistd.h>
#include <inttypes.h>
#include <stdlib.h>
#include <fcntl.h>
#include "String.h"

static Mutex mutex;
static uint32_t seed = 0;

int
random_generator(int min, int max)
{
	MutexLocker l(mutex);

	int fd = open("/dev/urandom", O_RDONLY);
	if (fd > 0) {
		ssize_t ret = read(fd, &seed, sizeof(seed));
		close(fd);
		if (ret > 0 && (size_t) ret != sizeof(seed))
			seed = 0;
	}

	if (!seed)
		seed = time_mil();

	int range = max - min;
	if (range <= 5)
		throw String("random_generate(min, max): range too small");

	return (int) (min + range * ((double) rand_r(&seed)) / RAND_MAX);
}
