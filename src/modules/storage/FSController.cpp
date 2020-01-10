/*
  Copyright Red Hat, Inc. 2005-2008

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to the
  Free Software Foundation, Inc.,  675 Mass Ave, Cambridge,
  MA 02139, USA.
*/
/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */

#include "FSController.h"
#include "ExtendedFS.h"
#include "SwapFS.h"
#include "GFS1.h"
#include "GFS2.h"
#include "UnsupportedFS.h"
#include "utils.h"
#include <errno.h>

#ifdef MY_BUILD_SYSTEM_IS_NOT_BROKEN

#include <libgroup.h>

#endif

using namespace std;

std::list<String>
FSController::get_fs_group_ids(const String& name)
{
	list<String> members;
#ifdef MY_BUILD_SYSTEM_IS_NOT_BROKEN
	group_data_t fsgroup;

	if (group_get_group(2, name.c_str(), &fsgroup) != 0)
		throw String("error retrieving group members for " + name + ": " + strerror(errno));

	if (!fsgroup.member)
		throw String("the local node does not have " + name + " mounted");

	for (int i = 0 ; i < fsgroup.member_count ; i++) {
		char buf[8];
		int ret = snprintf(buf, sizeof(buf), "%d", fsgroup.members[i]);
		if (ret < 1 || (size_t) ret >= sizeof(buf))
			throw String("invalid node id");
    	members.push_back(String(buf));
	}
#endif
	return members;
}

counting_auto_ptr<Content>
FSController::get_fs(const String& path)
{
  try {
    return counting_auto_ptr<Content>(new ExtendedFS(path));
  } catch ( ... ) {}
  try {
    return counting_auto_ptr<Content>(new GFS1(path));
  } catch ( ... ) {}
  try {
    return counting_auto_ptr<Content>(new GFS2(path));
  } catch ( ... ) {}
  try {
    return counting_auto_ptr<Content>(new SwapFS(path));
  } catch ( ... ) {}
  return counting_auto_ptr<Content>(new UnsupportedFS(path));
}

std::list<counting_auto_ptr<ContentTemplate> >
FSController::get_available_fss()
{
  list<counting_auto_ptr<ContentTemplate> > cnts;

  try {
    cnts.push_back(counting_auto_ptr<ContentTemplate>(new ExtendedFSTemplate()));
  } catch ( ... ) {}
  try {
    cnts.push_back(counting_auto_ptr<ContentTemplate>(new GFS1Template()));
  } catch ( ... ) {}
  try {
    cnts.push_back(counting_auto_ptr<ContentTemplate>(new GFS2Template()));
  } catch ( ... ) {}
  try {
    cnts.push_back(counting_auto_ptr<ContentTemplate>(new SwapFSTemplate()));
  } catch ( ... ) {}

  return cnts;
}

counting_auto_ptr<Content>
FSController::create_fs(const counting_auto_ptr<BD>& bd,
			const counting_auto_ptr<ContentTemplate>& cont_templ)
{
  if (cont_templ->type != CONTENT_FS_TYPE)
    throw String("content_template not of filesystem type");

  String fs_type = cont_templ->attrs["fs_type"];
  if (fs_type == ExtendedFS::PRETTY_NAME)
    create_extended_fs(bd->path(), cont_templ);
  else if (fs_type == GFS1::PRETTY_NAME)
    create_GFS1(bd->path(), cont_templ);
  else if (fs_type == GFS2::PRETTY_NAME)
    create_GFS2(bd->path(), cont_templ);
  else if (fs_type == SwapFS::PRETTY_NAME)
    create_swap_fs(bd->path(), cont_templ);
  else
    throw String("unknown fs type \"") + fs_type + "\"";

  utils::clear_cache();
  return get_fs(bd->path());
}
