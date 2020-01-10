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
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */

#ifndef __CONGA_NETWORK_H
#define __CONGA_NETWORK_H

#include "counting_auto_ptr.h"
#include "String.h"
#include <vector>

#include <netdb.h>

class Network
{
	public:
		static std::vector<String> name2IP(const char *hostname);
		static struct addrinfo *resolve_host(const char *host);
		static String localhost();


		class Hostent
		{
			public:
				struct hostent ent;
				char data[4*1024 - sizeof(struct hostent)];
				struct hostent* operator->() { return &ent; }
				struct hostent& operator*() { return ent; }
		};
};

#endif
