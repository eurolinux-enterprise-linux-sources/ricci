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
 * Author:
 *  Ryan McCabe <rmccabe@redhat.com>
 */

extern "C" {
	#include <unistd.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <string.h>
	#include <errno.h>

	#include "sys_util.h"
	#include "base64.h"
}

#include "XVM.h"
#include "utils.h"

using namespace std;

bool XVM::virt_guest(void) {
	try {
		String out, err;
		int status;
		vector<String> args;

		if (utils::execute(DMIDECODE_PATH, args, out, err, status, false))
			throw command_not_found_error_msg(DMIDECODE_PATH);
		if (status != 0)
			throw String("dmidecode failed: " + err);
		if (out.find("Vendor: Xen") != out.npos)
			return true;
		if (out.find("Manufacturer: Xen") != out.npos)
			return true;
	} catch ( ... ) {}

	return false;
}

bool XVM::delete_xvm_key(void) {
	return unlink(XVM_KEY_PATH) == 0;
}

bool XVM::set_xvm_key(const char *key_base64) {
	char *buf = NULL;
	size_t keylen;
	size_t keylen_dec = 0;
	bool decoded = false;
	int fd;
	ssize_t ret;
	mode_t old_mask;
	char tmpname[] = "/etc/cluster/.fence_xvm.keyXXXXXX";

	if (key_base64 == NULL)
		throw String("no key was given");

	keylen = strlen(key_base64);
	if (keylen < 1)
		throw String("no key was given");

	decoded = base64_decode_alloc(key_base64, keylen, &buf, &keylen_dec);
	if (!decoded || buf == NULL)
		throw String("an invalid key was given");

	old_mask = umask(077);

	fd = mkstemp(tmpname);
	if (fd < 0) {
		int err = errno;
		umask(old_mask);
		memset(buf, 0, keylen_dec);
		free(buf);
		throw String("error setting new key: ") + String(strerror(err));
	}
	umask(old_mask);

	fchmod(fd, 0600);
	ret = write_restart(fd, buf, keylen_dec);
	if (ret < 0) {
		unlink(tmpname);
		close(fd);
		memset(buf, 0, keylen_dec);
		free(buf);
		throw String("error setting new key: ") + String(strerror(-ret));
	}

	close(fd);
	memset(buf, 0, keylen_dec);
	free(buf);

	if (rename(tmpname, XVM_KEY_PATH) != 0) {
		int err = errno;
		unlink(tmpname);
		throw String("error setting new key: ") + String(strerror(err));
	}

	return (true);
}

bool XVM::generate_xvm_key(size_t keylen) {
	int fd;
	size_t ret;
	char buf[XVM_KEY_MAX_SIZE];
	int err = 0;

	if (keylen < XVM_KEY_MIN_SIZE || keylen > XVM_KEY_MAX_SIZE)
		throw String("invalid key length");

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw String("error generating key: ") + String(strerror(errno));

	ret = read_restart(fd, buf, keylen);
	close(fd);
	if (ret < 0)
		throw String("error generating key: ") + String(strerror(-ret));

	fd = open(XVM_KEY_PATH, O_WRONLY | O_EXCL | O_CREAT, 0600);
	if (fd < 0)
		throw String("error generating key: ") + String(strerror(errno));

	ret = write_restart(fd, buf, keylen);
	close(fd);
	if (ret < 0) {
		unlink(XVM_KEY_PATH);
		throw String("error generating key: ") + String(strerror(-ret));
	}
	return (true);
}

char *XVM::get_xvm_key(void) {
	int fd;
	ssize_t ret;
	size_t keylen_bin = 0;
	size_t keylen_base64 = 0;
	char buf[XVM_KEY_MAX_SIZE];
	struct stat st;
	char *key_out = NULL;
	int err = 0;

	fd = open(XVM_KEY_PATH, O_RDONLY);
	if (fd < 0)
		throw String("error retrieving key:") + String(strerror(errno));

	if (fstat(fd, &st) != 0) {
		close(fd);
		throw String("error retrieving key: ") + String(strerror(errno));
	}

	ret = read_restart(fd, buf, sizeof(buf));
	close(fd);
	if (ret < 0 || (off_t) ret != st.st_size) {
		memset(buf, 0, sizeof(buf));
		throw String("error retrieving key: ") + String(strerror(-ret));
	}
	keylen_bin = (size_t) ret;

	keylen_base64 = base64_encode_alloc(buf, keylen_bin, &key_out);
	memset(buf, 0, sizeof(buf));
	if (keylen_base64 < 1 || key_out == NULL)
		throw String("error retrieving key");
	return (key_out);
}
