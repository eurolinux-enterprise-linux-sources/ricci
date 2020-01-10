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

#ifndef __CONGA_EXECUTILS_H
#define __CONGA_EXECUTILS_H

#include "String.h"
#include <vector>

/*
** return 0 on success, non-zero on failure
** Kill the child process after @timeout ms has elapsed,
** if @timeout is negative, there's no no timeout
*/
int execute(const String& path,
			const std::vector<String>& args,
			String& out,
			String& err,
			int& status,
			int timeout=-1);

#endif
