/*
** Copyright (C) Red Hat, Inc. 2007-2009
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
 * 	Stanko Kupcevic <kupcevic@redhat.com>
 * 	Ryan McCabe <rmccabe@redhat.com>
 */

#include "Network.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct addrinfo *Network::resolve_host(const char *hostname) {
	int ret;
	struct addrinfo *res = NULL;

	ret = getaddrinfo(hostname, NULL, NULL, &res);
	if (ret != 0) {
		if (res != NULL)
			freeaddrinfo(res);
		return (NULL);
	}
	return (res);
}

std::vector<String>
Network::name2IP(const char *hostname) {
	std::vector<String> addrs;
	struct addrinfo *addr_list = NULL;
	struct addrinfo *cur;

	addr_list = resolve_host(hostname);
	for (cur = addr_list ; cur != NULL ; cur = cur->ai_next) {
		char buf[INET6_ADDRSTRLEN + 1];
		int ret;

		ret = getnameinfo(cur->ai_addr, cur->ai_addrlen,
				buf, sizeof(buf), NULL, 0, NI_NUMERICHOST);
		if (ret == 0) {
			try {
				addrs.push_back(buf);
			} catch (...) {}
		}
	}

	freeaddrinfo(addr_list);
	return addrs;
}

String
Network::localhost()
{
	// get hostname
	char name[1024];
	if (gethostname(name, sizeof(name) - 1))
		return "";
	name[sizeof(name) - 1] = '\0';

	try {
		struct addrinfo *addr_list;

		addr_list = resolve_host(name);
		if (addr_list != NULL && addr_list->ai_canonname != NULL) {
			String ret(addr_list->ai_canonname);
			freeaddrinfo(addr_list);
			return (ret);
		}
		freeaddrinfo(addr_list);
	} catch ( ... ) {
	}

	return (String(name));
}
