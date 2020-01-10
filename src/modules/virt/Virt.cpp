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

extern "C" {
	#include <unistd.h>
	#include <stdlib.h>
	#include <fcntl.h>
	#include <sys/stat.h>
	#include <string.h>
	#include <errno.h>
#if defined(VIRT_SUPPORT) && if VIRT_SUPPORT == 1
	#include <libvirt/libvirt.h>
#endif

	#include "sys_util.h"
	#include "base64.h"
}

#include "Virt.h"
#include "utils.h"

using namespace std;

bool Virt::virt_guest(void) {
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

#if defined(VIRT_SUPPORT) && if VIRT_SUPPORT == 1

map<String, String> Virt::get_vm_list(const String &hvURI) {
	std::map<String, String> vm_list;
	int i;
	int ret;
	int num_doms;
	virConnectPtr con = NULL;

	con = virConnectOpenReadOnly(hvURI.c_str());
	if (con == NULL)
		throw String("unable to connect to virtual machine manager");


	num_doms = virConnectNumOfDefinedDomains(con);
	if (num_doms < 1) {
		virConnectClose(con);
		throw String("Unable to get the number of defined domains");
	}

	if (num_doms > 0) {
		char **dom = (char **) calloc(num_doms, sizeof(char *));
		if (!dom) {
			virConnectClose(con);
			throw String("Out of memory");
		}

		ret = virConnectListDefinedDomains(con, dom, num_doms);
		if (ret < 0) {
			free(dom);
			virConnectClose(con);
			throw String("Unable to list defined domains");
		}

		for (i = 0 ; i < ret ; i++) {
			if (dom[i] != NULL) {
				vm_list.insert(
					pair<String,String>(String(dom[i]), String("inactive")));
				free(dom[i]);
			}
		}

		free(dom);
	}

	num_doms = virConnectNumOfDomains(con);
	if (num_doms < 0) {
		virConnectClose(con);
		throw String("Unable to get the number of defined domains");
	}

	if (num_doms > 0) {
		int *active_doms = (int *) calloc(sizeof(int), num_doms);
		ret = virConnectListDomains(con, active_doms, num_doms);
		if (ret > 0) {
			for (i = 0 ; i < ret ; i++) {
				const char *name;
				if (active_doms[i] == 0) {
					/* Skip dom0 */
					continue;
				}

				virDomainPtr vdp = virDomainLookupByID(con, active_doms[i]);
				if (vdp == NULL)
					continue;

				name = virDomainGetName(vdp);
				if (name != NULL) {
					vm_list.insert(
						pair<String,String>(String(name), String("active")));
				}
			}
		}
		free(active_doms);
	}

	virConnectClose(con);
	return vm_list;
}

#else

map<String, String> Virt::get_vm_list(const String &hvURI) {
	std::map<String, String> vm_list;
	throw String("Unsupported on this architecture.");
	return vm_list;
}

#endif
