/*
** Copyright (C) Red Hat, Inc. 2007-2009
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License version 2 as published
** by the Free Software Foundation.
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
**
** Author: Ryan McCabe <rmccabe@redhat.com>
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>

uint32_t page_size;

ssize_t read_restart(int fd, void *buf, size_t len) {
	void *buf_orig = buf;

	while (len > 0) {
		ssize_t ret = read(fd, buf, len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				break;
			return -errno;
		}

		if (ret == 0)
			break;

		buf += ret;
		len -= (size_t) ret;
	}
	return (ssize_t) (buf - buf_orig);
}

ssize_t write_restart(int fd, const void *buf, size_t len) {
	const void *buf_orig = buf;

	while (len > 0) {
		ssize_t ret = write(fd, buf, len);
		if (ret < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				break;
			return -errno;
		}

		if (ret == 0)
			break;

		buf += ret;
		len -= (size_t) ret;
	}
	return (ssize_t) (buf - buf_orig);
}

void *mallock(size_t len) {
	size_t rounded_len;

	if (!page_size) {
		int ret = sysconf(_SC_PAGESIZE);
		if (ret <= 0)
			return NULL;
		page_size = (uint32_t) ret;
	}

	/* Round up to the next multiple of the page size */
	rounded_len = (len + (page_size - 1)) & -page_size;

	void *ret = malloc(rounded_len);
	if (ret == NULL)
		return ret;

	if (mlock(ret, rounded_len) != 0) {
		free(ret);
		return NULL;
	}

	return ret;
}

int mdallock(void *mem, size_t len) {
	int ret;
	size_t rounded_len;

	rounded_len = (len + (page_size - 1)) & -page_size;
	memset(mem, 0, len);
	ret = munlock(mem, rounded_len);
	if (ret != 0)
		ret = -errno;

	free(mem);
	return ret;
}
