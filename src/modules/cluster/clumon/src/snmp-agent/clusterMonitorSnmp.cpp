/*
** Copyright (C) Red Hat, Inc. 2005-2008
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

#include "clusterMonitorSnmp.h"
#include "clusterMIB.h"
#include "nodesMIB.h"
#include "servicesMIB.h"
#include "clumond_globals.h"

ClusterMonitoring::ClusterMonitor monitor;

void
init_RedHatCluster(void)
{
	try {
		initialize_clusterMIB();
		initialize_nodesMIB();
		initialize_servicesMIB();

		// TODO: initialize others
	} catch ( ... ) {}
}
