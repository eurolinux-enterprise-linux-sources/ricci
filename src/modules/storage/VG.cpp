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


#include "VG.h"
#include "LVM.h"
#include "LV.h"
#include "MapperFactory.h"
#include "defines.h"
#include "utils.h"
#include "MidAir.h"
#include "LVMClusterLockingError.h"

#include "Time.h"

#include <vector>


using namespace std;



list<String>
get_VG_ids()
{
  LVM::check_locking();

  list<String> vgids;

  vector<String> args;
  args.push_back("vgs");
  args.push_back("--noheadings");
  args.push_back("-o");
  args.push_back("vg_name");
  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status) {
    if (err.find("Skipping clustered") != err.npos)
      throw LVMClusterLockingError();
    throw String("vgs failed");
  }
  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    String line = utils::strip(*iter);
    vector<String> words = utils::split(line, " ");
    if (words.size() == 1)
      if (!words[0].empty())
	vgids.push_back(VG_PREFIX + words[0]);
  }
  vgids.push_back(VG_PREFIX); // unused pvs holder
  return vgids;
}






//  ##### VG #####

VG::VG(const String& id) :
  Mapper(MAPPER_VG_TYPE, id),
  _vgname(id.substr(VG_PREFIX.size()))
{
  LVM::probe_vg(_vgname, _props, sources, targets);

  if (id == VG_PREFIX) {
    list<counting_auto_ptr<Mapper> > mappers =
      MapperFactory::get_mappers(MAPPER_PT_TYPE);
    for (list<counting_auto_ptr<Mapper> >::iterator iter_map = mappers.begin();
	 iter_map != mappers.end();
	 iter_map++) {
      Mapper& mapper = **iter_map;
      for (list<counting_auto_ptr<BD> >::iterator iter_bd = mapper.targets.begin();
	   iter_bd != mapper.targets.end();
	   iter_bd++) {
	if ((*iter_bd)->content->type == CONTENT_NONE_TYPE)
	  new_sources.push_back(*iter_bd);
      }
    }
  } else {
    long long size =
      _props.get("extents_total").get_int() * _props.get("extent_size").get_int();
    _props.set(Variable("size", size));

    long long free_size =
      _props.get("extents_free").get_int() * _props.get("extent_size").get_int();
    _props.set(Variable("size_free", free_size));

    bool rem = true;
    for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
	 iter != targets.end();
	 iter++)
      if (!(*iter)->removable())
	rem = false;
    removable(rem);

    // new sources
    VG unused(VG_PREFIX);
    for (list<counting_auto_ptr<BD> >::iterator iter = unused.sources.begin();
	 iter != unused.sources.end();
	 iter++)
      new_sources.push_back(*iter);
    for (list<counting_auto_ptr<BD> >::iterator iter = unused.new_sources.begin();
	 iter != unused.new_sources.end();
	 iter++)
      new_sources.push_back(*iter);


    // ### new targets ###

    if (free_size == 0)
      return;

    // new LV
    counting_auto_ptr<BDTemplate> new_lv(new LVTemplate(_mapper_id, state_ind()));

    new_lv->props.set(Variable("size",
			       free_size,
			       _props.get("extent_size").get_int(),
			       free_size,
			       _props.get("extent_size").get_int()));

    list<String> lvnames;
    for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
	 iter != targets.end();
	 iter++) {
      String lvname = (*iter)->_props.get("lvname").get_string();
      lvnames.push_back(lvname);
    }
    lvnames.push_back("_mlog");
    lvnames.push_back("_mimage");
    new_lv->props.set(Variable("lvname",
			       String("new_lv"),
			       1,
			       36,
			       NAMES_ILLEGAL_CHARS,
			       lvnames));

    new_lv->props.set(Variable("vgname", _vgname));
    new_lv->props.set(Variable("snapshot", false));
    new_lv->props.set(Variable("clustered", _props.get("clustered").get_bool()));
    new_targets.push_back(new_lv);

    // snapshots
    counting_auto_ptr<BDTemplate> new_snap(new LVTemplate(_mapper_id, state_ind()));
    new_snap->props = new_lv->props;
    new_snap->props.set(Variable("lvname",
				 String("new_snapshot"),
				 1,
				 36,
				 NAMES_ILLEGAL_CHARS,
				 lvnames));

    new_snap->content->_avail_replacements.clear();

    new_snap->props.set(Variable("snapshot", true));
    list<String> snap_origs;
    for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
	 iter != targets.end();
	 iter++)
      if (!(*iter)->_props.get("snapshot").get_bool())
	snap_origs.push_back((*iter)->_props.get("lvname").get_string());
    if (snap_origs.size()) {
      new_snap->props.set(Variable("snapshot_origin",
				   snap_origs.front(),
				   snap_origs));
      new_targets.push_back(new_snap);
    }
  }
}

VG::~VG()
{}


void
VG::apply(const MapperParsed& mp)
{
  String vgname(mp.props.get("vgname").get_string());

  // clustered
  bool clustered_old = _props.get("clustered").get_bool();
  bool clustered_new = mp.props.get("clustered").get_bool();
  if (clustered_old != clustered_new)
    LVM::vgchange(vgname, clustered_new);
}

void
VG::__add_sources(const list<counting_auto_ptr<BD> >& bds)
{
  // if VG is marked as clustered, but cluster locking is not available, throw
  if (_props.get("clustered").get_bool() &&
      !LVM::clustered_enabled())
    throw LVMClusterLockingError();

  String vgname;
  try {
    vgname = _props.get("vgname").get_string();
  } catch ( ... ) {}

  try {
    utils::clear_cache();

    list<String> pv_paths;
    for (list<counting_auto_ptr<BD> >::const_iterator iter = bds.begin();
	 iter != bds.end();
	 iter++) {
      counting_auto_ptr<BD> bd = *iter;
      pv_paths.push_back(bd->path());
      // create PV if needed
      if (bd->content->type == CONTENT_NONE_TYPE)
	LVM::pvcreate(bd->path());
    }

    // extend VG, if not unused PVs holder
    if (vgname.size())
      LVM::vgextend(vgname, pv_paths);

  } catch ( ... ) {
    for (list<counting_auto_ptr<BD> >::const_iterator iter = bds.begin();
	 iter != bds.end();
	 iter++) {
      counting_auto_ptr<BD> bd = *iter;
      if (bd->content->type == CONTENT_NONE_TYPE)
	try {
	  LVM::pvremove(bd->path());
	} catch ( ... ) {}
    }
    throw;
  }
}

void
VG::__remove()
{
  // if VG is marked as clustered, but cluster locking is not available, throw
  if (_props.get("clustered").get_bool() &&
      !LVM::clustered_enabled())
    throw LVMClusterLockingError();

  String vgname = _props.get("vgname").get_string();
  LVM::vgremove(vgname);
  for (list<counting_auto_ptr<BD> >::const_iterator iter = sources.begin();
       iter != sources.end();
       iter++)
    LVM::pvremove((*iter)->path());
  utils::clear_cache();
}



counting_auto_ptr<Mapper>
create_VG(const MapperTemplate& temp)
{
  // everything is already validated, but number of sources
  if (temp.sources.size() == 0)
    throw String("create_VG requires at least one source");

  String vgname = temp.props.get("vgname").get_string();
  long long extent_size = temp.props.get("extent_size").get_int();
  bool clustered = temp.props.get("clustered").get_bool();

  if (clustered &&
      !LVM::clustered_enabled())
    throw LVMClusterLockingError();

  try {
    utils::clear_cache();

    list<String> pv_paths;
    for (list<counting_auto_ptr<BD> >::const_iterator iter = temp.sources.begin();
	 iter != temp.sources.end();
	 iter++) {
      counting_auto_ptr<BD> bd = *iter;
      pv_paths.push_back(bd->path());
      // create PV if needed
      if (bd->content->type == CONTENT_NONE_TYPE)
	LVM::pvcreate(bd->path());
    }

    // create VG
    LVM::vgcreate(vgname, extent_size, clustered, pv_paths);
  } catch ( ... ) {
    for (list<counting_auto_ptr<BD> >::const_iterator iter = temp.sources.begin();
	 iter != temp.sources.end();
	 iter++) {
      counting_auto_ptr<BD> bd = *iter;
      if (bd->content->type == CONTENT_NONE_TYPE)
	try {
	  LVM::pvremove(bd->path());
	} catch ( ... ) {}
    }
    throw;
  }

  return counting_auto_ptr<Mapper>(new VG(VG_PREFIX + vgname));
}



//  ##### VGTemplate #####

VGTemplate::VGTemplate() :
  MapperTemplate(MAPPER_VG_TYPE)
{
  // vgname
  const list<String> vgids = get_VG_ids();
  list<String> vgnames;
  for (list<String>::const_iterator iter = vgids.begin();
       iter != vgids.end();
       iter++) {
    String vgname = iter->substr(VG_PREFIX.size());
    if (!vgname.empty())
      vgnames.push_back(vgname);
  }
  Variable vgname("vgname",
		  String("new_vg"),
		  1,
		  36,
		  NAMES_ILLEGAL_CHARS,
		  vgnames);
  props.set(vgname);

  // extent_size
  list<long long> ext_sizes;
  for (long long i = 8 * 1024 /* 8KB */;
       i <= 16LL * 1024 * 1024 * 1024 /* 16 GB */;
       i = 2L * i)
    ext_sizes.push_back(i);
  props.set(Variable("extent_size",
		     4 * 1024 * 1024 /* 4 MB */,
		     ext_sizes));

  // clustered
  bool use_clustered = false;
  try {
    LVM::check_locking();
    use_clustered = true;
  } catch ( ... ) { }
  props.set(Variable("clustered", use_clustered, true));

  // new sources
  VG unused(VG_PREFIX);
  for (list<counting_auto_ptr<BD> >::iterator iter = unused.sources.begin();
       iter != unused.sources.end();
       iter++)
    new_sources.push_back(*iter);
  for (list<counting_auto_ptr<BD> >::iterator iter = unused.new_sources.begin();
       iter != unused.new_sources.end();
       iter++)
    new_sources.push_back(*iter);

  if (new_sources.empty())
    throw String("no available new sources");
  props.set(Variable("min_sources", (long long) 1));
  props.set(Variable("max_sources", (long long) new_sources.size()));
}

VGTemplate::~VGTemplate()
{}
