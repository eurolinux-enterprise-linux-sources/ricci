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

#ifndef __CONGA_MODCLUSTER_CLUSTERCONF_H
#define __CONGA_MODCLUSTER_CLUSTERCONF_H

#include "XML.h"

class ClusterConf
{
	public:
		static XMLObject get();
		static void set(const XMLObject& xml, bool propagate=true);
		static void set_version(int);
		static void purge_conf();

		static bool is_gulm(const XMLObject& cluster_conf);
		static bool is_cman(const XMLObject& cluster_conf);
};

#endif
