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

#ifndef __CONGA_MODCLUSTER_XVM_H
#define __CONGA_MODCLUSTER_XVM_H

#include "String.h"
#include <map>

#define XVM_KEY_PATH			"/etc/cluster/fence_xvm.key"
#define XVM_KEY_MAX_SIZE		4096
#define XVM_KEY_MIN_SIZE		128
#define XVM_KEY_DEFAULT_SIZE	4096

#define DMIDECODE_PATH			"/usr/sbin/dmidecode"

class XVM {
	public:
		static bool virt_guest(void);
		static bool delete_xvm_key(void);
		static bool set_xvm_key(const char *key_base64);
		static char *get_xvm_key(void);
		static bool generate_xvm_key(size_t key_bytes);
};

#endif
