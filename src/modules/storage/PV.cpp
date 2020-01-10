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


#include "PV.h"
#include "LVM.h"
#include "MapperFactory.h"
#include "utils.h"
#include "LVMClusterLockingError.h"
#include <algorithm>


using namespace std;


PV::PV(const String& path) :
  MapperSource(MAPPER_VG_TYPE,
	       VG_PREFIX + LVM::vgname_from_pvpath(path),
	       SOURCE_PV_TYPE,
	       path)
{
  LVM::probe_pv(path, _props);

  if (unused()) {
    list<String> vgnames;

    String vgname(_props.get("vgname").get_string());
    vgnames.push_back(vgname);

    list<pair<String, String> > vg_pairs =
      MapperFactory::get_mapper_ids(MAPPER_VG_TYPE);
    for (list<pair<String, String> >::const_iterator iter = vg_pairs.begin();
	 iter != vg_pairs.end();
	 iter++) {
      String vgname_alt(iter->second.substr(VG_PREFIX.size()));
      if (find(vgnames.begin(),
	       vgnames.end(),
	       vgname_alt) == vgnames.end())
	vgnames.push_back(vgname_alt);
    }
    _props.set(Variable("vgname", vgnames.front(), vgnames));
  }
}

PV::~PV()
{}



bool
PV::unused() const
{
  if (_props.get("size").get_int() == _props.get("size_free").get_int())
    return true;
  else
    return false;
}

bool
PV::removable() const
{
  return unused();
}

void
PV::remove()
{
  if (!removable())
    throw String("PV::remove() called, while pv not removable");

  counting_auto_ptr<Mapper> vg =
    MapperFactory::get_mapper(_mapper_type,
			      _mapper_id);

  String vgname = _props.get("vgname").get_string();


  // if VG is marked as clustered, but cluster locking is not available, throw
  if (!LVM::clustered_enabled() &&
      LVM::vg_clustered(vgname))
    throw LVMClusterLockingError();


  if (vg->sources.size() == 1) {
    if (vgname.size())
      LVM::vgremove(vgname);
    LVM::pvremove(path());
  } else {
    if (vgname.size())
      LVM::vgreduce(vgname, path());
    LVM::pvremove(path());
  }
}

void
PV::apply_props_before_resize(const String& path,
			      unsigned long long old_size,
			      unsigned long long new_size,
			      const Props& new_props)
{
  String vgname_old(_props.get("vgname").get_string());
  String vgname_new(new_props.get("vgname").get_string());


  // if VG is marked as clustered, but cluster locking is not available, throw
  if (!LVM::clustered_enabled() &&
      (LVM::vg_clustered(vgname_old) ||
       LVM::vg_clustered(vgname_new)))
    throw LVMClusterLockingError();


  counting_auto_ptr<Mapper> vg_old =
    MapperFactory::get_mapper(_mapper_type,
			      _mapper_id);

  if (vgname_old != vgname_new) {
    if (vgname_old.size()) {
      if (vg_old->sources.size() == 1)
	LVM::vgremove(vgname_old);
      else
	LVM::vgreduce(vgname_old, path);
    }
    if (vgname_new.size())
      LVM::vgextend(vgname_new, list<String>(1, path));
  }
}



void
create_content_pv(const String& path,
		  const counting_auto_ptr<ContentTemplate>& templ)
{
  String vgname(templ->_props.get("vgname").get_string());


  // if VG is marked as clustered, but cluster locking is not available, throw
  if (!LVM::clustered_enabled() &&
      LVM::vg_clustered(vgname))
    throw LVMClusterLockingError();


  LVM::pvcreate(path);
  try {
    if (vgname.size())
      LVM::vgextend(vgname, list<String>(1, path));
  } catch ( ... ) {
    LVM::pvremove(path);
    throw;
  }
}


PVTemplate::PVTemplate() :
  MapperSourceTemplate(MAPPER_VG_TYPE)
{
  list<String> vgnames;
  list<pair<String, String> > vg_pairs =
    MapperFactory::get_mapper_ids(MAPPER_VG_TYPE);
  for (list<pair<String, String> >::const_iterator iter = vg_pairs.begin();
       iter != vg_pairs.end();
       iter++) {
    String vgname(iter->second.substr(VG_PREFIX.size()));
    vgnames.push_back(vgname);
  }

  // there will always be at least unused PVs holder
  _props.set(Variable("vgname", vgnames.front(), vgnames));
}

PVTemplate::~PVTemplate()
{}
