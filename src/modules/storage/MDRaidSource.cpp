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


#include "MDRaidSource.h"


using namespace std;


MDRaidSource::MDRaidSource(const String& path) :
  MapperSource(MAPPER_MDRAID_TYPE,
	       "BUG",
	       SOURCE_MDRAID_TYPE,
	       path),
  _raid(mdadm::probe_path(path))
{
  if (_raid.path == path)
    throw String("not MDRaidSource, but target");
  for (list<mdraid_source>::const_iterator iter = _raid.devices.begin();
       iter != _raid.devices.end();
       iter++)
    if (iter->path == path)
      _raid_source = counting_auto_ptr<mdraid_source>(new mdraid_source(*iter));
  if ( ! _raid_source.get())
    throw String("MDRaidSource, null pointer");

  _mapper_id = MDRAID_PREFIX + _raid.path;

  _props.set(Variable("raid", _raid.name));
  _props.set(Variable("raid_level", _raid.level));

  String state;
  switch (_raid_source->type) {
  case MDRAID_S_ACTIVE:
    state = "active";
    break;
  case MDRAID_S_SPARE:
    state = "spare";
    break;
  case MDRAID_S_FAILED:
    state = "failed";
    break;
  default:
    throw String("invalid mdraid_source_type");
  }
  _props.set(Variable("state", state));

  if (_raid_source->type == MDRAID_S_ACTIVE)
    _props.set(Variable("failed", false, true));
  else if (_raid_source->type == MDRAID_S_SPARE)
    _props.set(Variable("failed", false));
  else
    _props.set(Variable("failed", true));
}

MDRaidSource::~MDRaidSource()
{}


bool
MDRaidSource::removable() const
{
  bool res = _raid_source->type == MDRAID_S_FAILED ||
    _raid_source->type == MDRAID_S_SPARE;
  return res;
}

void
MDRaidSource::remove()
{
  if ( ! removable())
    throw String("MDRaidSource::remove() called on unremovable mdraid source");
  mdadm::remove_source(_raid.path, path());
}

void
MDRaidSource::apply_props_before_resize(const String& path,
					unsigned long long old_size,
					unsigned long long new_size,
					const Props& new_props)
{
  bool failed_old = _props.get("failed").get_bool();
  bool failed_new = new_props.get("failed").get_bool();
  if (!failed_old && failed_new)
    mdadm::fail_source(_raid.path, path);
}

