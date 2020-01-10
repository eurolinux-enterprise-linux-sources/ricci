/*
  Copyright Red Hat, Inc. 2005

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


#include "MDRaidTarget.h"
#include "BDFactory.h"
#include "FSController.h"
#include "defines.h"
#include "utils.h"
#include "ContentNone.h"


using namespace std;



MDRaidTarget::MDRaidTarget(const String& path) :
  BD(MAPPER_MDRAID_TYPE,
     "BUG",
     BD_MDRAID_TYPE,
     path)
{
  _raid = mdadm::probe_path(path);
  if (_raid.path != path)
    throw String("not MDRaidTarget, but source");
  _mapper_id = MDRAID_PREFIX + _raid.path;

  // props
  _props.set(Variable("raid", _raid.name));
  _props.set(Variable("raid_level", _raid.level));

  // content
  if (content->removable()) {
    //    removable(true);
    if (content->type == CONTENT_NONE_TYPE) {
      list<counting_auto_ptr<ContentTemplate> > fss = FSController().get_available_fss();
      for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter = fss.begin();
	   iter != fss.end();
	   iter++) {
	content->add_replacement(*iter);
      }
    } else
      content->add_replacement(counting_auto_ptr<ContentTemplate>(new ContentNoneTemplate()));
  }
}

MDRaidTarget::~MDRaidTarget()
{}



void
MDRaidTarget::shrink(unsigned long long new_size,
	   const Props& new_props)
{
  throw String("MDRaidTarget not resizable");
}
void
MDRaidTarget::expand(unsigned long long new_size,
	   const Props& new_props)
{
  throw String("MDRaidTarget not resizable");
}
String
MDRaidTarget::apply_props_before_resize(const Props& new_props)
{
  return path();
}
String
MDRaidTarget::apply_props_after_resize(const Props& new_props)
{
  return path();
}

void
MDRaidTarget::remove()
{
  content->remove();

  // nothing to be done, will be removed as part of MDRaid removal,
  // MDRaidTarget should not be marked as removable, because itself cannot be removed
}
