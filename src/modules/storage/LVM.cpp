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


#include "LVM.h"
#include "utils.h"
#include "Variable.h"
#include "BDFactory.h"
#include "LV.h"
#include "defines.h"
#include "ClvmdError.h"
#include "ClusterNotQuorateError.h"
#include "ClusterNotRunningError.h"
#include "LVMClusterLockingError.h"

#include <vector>

#include <iostream>


using namespace std;

class PV_data
{
public:
  PV_data() {}
  PV_data(const String& path,
	  const String& vgname,
	  long long extent_size,
	  long long size,
	  long long size_free,
	  const String& uuid,
	  const String& attrs,
	  const String& format) :
    path(path),
    vgname(vgname),
    extent_size(extent_size),
    size(size),
    size_free(size_free),
    uuid(uuid),
    attrs(attrs),
    format(format) {}

  String path;
  String vgname;
  long long extent_size;
  long long size;
  long long size_free;
  String uuid;
  String attrs;
  String format;
};


static String
get_locking_type();

static vector<String>
vg_props(const String& vgname);

static const map<String, PV_data>
probe_pvs();

static bool
cluster_quorate();



static String LVMCONF_PATH("/usr/sbin/lvmconf");


// pvs
static String PVS_OPTIONS = "pv_name,vg_name,pv_size,pv_free,pv_attr,pv_fmt,pv_uuid,vg_extent_size";
static unsigned int PVS_NAME_IDX         = 0;
static unsigned int PVS_VG_NAME_IDX      = 1;
static unsigned int PVS_SIZE_IDX         = 2;
static unsigned int PVS_FREE_IDX         = 3;
static unsigned int PVS_ATTR_IDX         = 4;
static unsigned int PVS_FMT_IDX          = 5;
static unsigned int PVS_UUID_IDX         = 6;
static unsigned int PVS_EXTENT_SIZE_IDX  = 7;
static unsigned int PVS_OPTIONS_LENGTH   = 8;  // last


// pvdisplay -c
static unsigned int PVDISPLAY_c_NAME_IDX         = 0;
static unsigned int PVDISPLAY_c_VG_NAME_IDX      = 1;
static unsigned int PVDISPLAY_c_SIZE_IDX         = 2;
static unsigned int PVDISPLAY_c_EXTENT_SIZE_IDX  = 7;
static unsigned int PVDISPLAY_c_TOT_EXT_IDX      = 8;
static unsigned int PVDISPLAY_c_FREE_EXT_IDX     = 9;
static unsigned int PVDISPLAY_c_USED_EXT_IDX     = 10;
static unsigned int PVDISPLAY_c_UUID_IDX         = 11;
static unsigned int PVDISPLAY_c_OPTIONS_LENGTH   = 12;  // last


// lvs
static String LVS_OPTIONS_STRING = "lv_name,vg_name,stripes,stripesize,lv_attr,lv_uuid,devices,origin,snap_percent,seg_start,seg_size,vg_extent_size,lv_size,vg_free_count,vg_attr";
static unsigned int LVS_NAME_IDX         = 0;
static unsigned int LVS_VG_NAME_IDX      = 1;
//static unsigned int LVS_STRIPES_NUM_IDX  = 2;
//static unsigned int LVS_STRIPE_SIZE_IDX  = 3;
static unsigned int LVS_ATTR_IDX         = 4;
static unsigned int LVS_UUID_IDX         = 5;
//static unsigned int LVS_DEVICES_IDX      = 6;
static unsigned int LVS_SNAP_ORIGIN_IDX  = 7;
static unsigned int LVS_SNAP_PERCENT_IDX = 8;
//static unsigned int LVS_SEG_START_IDX    = 9;
//static unsigned int LVS_SEG_SIZE_IDX     = 10;
static unsigned int LVS_EXTENT_SIZE_IDX  = 11;
static unsigned int LVS_SIZE_IDX         = 12;
static unsigned int LVS_VG_FREE_COUNT    = 13;
static unsigned int LVS_VG_ATTR_IDX      = 14;
//  if LVS_HAS_MIRROR_OPTIONS:
//  LVS_OPTION_STRING += ",mirror_log";
//static unsigned int LVS_MIRROR_LOG_IDX   = 13;
static unsigned int LVS_OPTIONS_LENGTH   = 15;  // last


// vgs
static String VGS_OPTIONS_STRING = "vg_name,vg_attr,vg_size,vg_extent_size,vg_free_count,max_lv,max_pv,vg_uuid";
static unsigned int VGS_NAME_IDX         = 0;
static unsigned int VGS_ATTR_IDX         = 1;
static unsigned int VGS_SIZE_IDX         = 2;
static unsigned int VGS_EXTENT_SIZE_IDX  = 3;
static unsigned int VGS_EXTENT_FREE_IDX  = 4;
static unsigned int VGS_MAX_LVS_IDX      = 5;
static unsigned int VGS_MAX_PVS_IDX      = 6;
static unsigned int VGS_UUID_IDX         = 7;
static unsigned int VGS_OPTIONS_LENGTH   = 8;  // last




String
LVM::vgname_from_lvpath(const String& lvpath)
{
  check_locking();

  vector<String> args;
  args.push_back("lvdisplay");
  args.push_back("-c");

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0) {
    if (err.find("Skipping clustered") != err.npos)
      throw LVMClusterLockingError();
    throw String("lvdisplay failed");
  }

  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    String& line = *iter;
    vector<String> words = utils::split(utils::strip(line), ":");
    if (lvpath == words[0])
      return words[1];
  }
  throw String("no such LV");
}

String
LVM::vgname_from_pvpath(const String& path)
{
  check_locking();

  map<String, PV_data> pvs(probe_pvs());
  map<String, PV_data>::const_iterator iter = pvs.find(path);
  if (iter == pvs.end())
    throw String("no such pv: ") + path;
  return iter->second.vgname;
}


void
LVM::probe_lv(const String& lvpath, Props& props)
{
  check_locking();

  vector<String> args;
  args.push_back("lvs");
  args.push_back("--nosuffix");
  args.push_back("--noheadings");
  args.push_back("--units");
  args.push_back("b");
  args.push_back("--separator");
  args.push_back(";");
  args.push_back("-o");
  args.push_back(LVS_OPTIONS_STRING);
  //  if LVS_HAS_ALL_OPTION:
  //    arglist.append("--all")
  args.push_back(lvpath);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0) {
    if (err.find("Skipping clustered") != err.npos)
      throw LVMClusterLockingError();
    throw String("lvs failed");
  }

  String line = utils::strip(out);
  vector<String> words = utils::split(line, ";");
  if (words.size() < LVS_OPTIONS_LENGTH)
    throw String("lvs failed, missing words");

  String vgname(utils::strip(words[LVS_VG_NAME_IDX]));
  props.set(Variable("vgname", vgname));

  String lvname(utils::strip(words[LVS_NAME_IDX]));
  // remove [] if there (used to mark hidden lvs)
  lvname = utils::lstrip(lvname, "[");
  lvname = utils::rstrip(lvname, "]");
  props.set(Variable("lvname", lvname));

  long long extent_size = utils::to_long(words[LVS_EXTENT_SIZE_IDX]);
  props.set(Variable("extent_size", extent_size));

  // size
  long long lv_size = utils::to_long(words[LVS_SIZE_IDX]);
  long long vg_extents_free = utils::to_long(words[LVS_VG_FREE_COUNT]);
  long long free = vg_extents_free * extent_size;
  props.set(Variable("size", lv_size, extent_size, lv_size + free, extent_size));

  props.set(Variable("uuid", words[LVS_UUID_IDX]));

  String attrs = words[LVS_ATTR_IDX];
  props.set(Variable("attrs", attrs));

  props.set(Variable("mirrored", attrs[0] == 'm' || attrs[0] == 'M'));

  // clustered
  String vg_attrs = words[LVS_VG_ATTR_IDX];
  props.set(Variable("clustered", vg_attrs[5] == 'c'));

  // snapshots
  String origin = words[LVS_SNAP_ORIGIN_IDX];
  props.set(Variable("snapshot", origin != ""));
  if (props.get("snapshot").get_bool()) {
    props.set(Variable("snapshot_origin", origin));
    long long usage = utils::to_long(words[LVS_SNAP_PERCENT_IDX]);
    props.set(Variable("snapshot_usage_percent", usage));
  }
  list<String> snapshots;
  args.clear();
  args.push_back("lvs");
  args.push_back("--noheadings");
  args.push_back("--separator");
  args.push_back(";");
  args.push_back("-o");
  args.push_back("lv_name,vg_name,origin");
  if (utils::execute(LVM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0) {
    if (err.find("Skipping clustered") != err.npos)
      throw LVMClusterLockingError();
    throw String("lvs failed");
  }
  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    String line = utils::strip(*iter);
    vector<String> words = utils::split(line, ";");
    if (words.size() != 3)
      continue;
    if (words[1] == vgname && words[2] == lvname)
      snapshots.push_back(words[0]);
  }
  props.set(Variable("snapshots", snapshots, false));
}





void
LVM::probe_pv(const String& path, Props& props)
{
  check_locking();

  map<String, PV_data> pvs(probe_pvs());
  map<String, PV_data>::const_iterator iter = pvs.find(path);
  if (iter == pvs.end())
    throw String("no such pv: ") + path;
  const PV_data& data = iter->second;

  props.set(Variable("vgname", data.vgname));
  props.set(Variable("extent_size", data.extent_size));
  props.set(Variable("size", data.size));
  props.set(Variable("size_free", data.size_free));
  props.set(Variable("uuid", data.uuid));
  props.set(Variable("attrs", data.attrs));
  props.set(Variable("format", data.format));
}




void
LVM::probe_vg(const String& vgname,
	      Props& props,
	      list<counting_auto_ptr<BD> >& sources,
	      list<counting_auto_ptr<BD> >& targets)
{
  check_locking();

  // pv to vg mappings
  map<String, String> pv_to_vg;
  map<String, PV_data> pvs_data(probe_pvs());
  for (map<String, PV_data>::const_iterator iter = pvs_data.begin();
       iter != pvs_data.end();
       iter++)
    pv_to_vg[iter->first] = iter->second.vgname;

  // probe vg
  if (!vgname.empty()) {

    vector<String> words = vg_props(vgname);

    props.set(Variable("vgname", words[VGS_NAME_IDX]));

    String vg_attrs(words[VGS_ATTR_IDX]);
    props.set(Variable("attrs", vg_attrs));

    long long size = utils::to_long(words[VGS_SIZE_IDX]);
    long long extent_size = utils::to_long(words[VGS_EXTENT_SIZE_IDX]);
    long long extents_free = utils::to_long(words[VGS_EXTENT_FREE_IDX]);
    props.set(Variable("extent_size", extent_size));
    props.set(Variable("extents_total", size/extent_size));
    props.set(Variable("extents_free", extents_free));
    props.set(Variable("extents_used", size/extent_size - extents_free));

    long long max_lvs = utils::to_long(words[VGS_MAX_LVS_IDX]);
    if (max_lvs == 0)
      max_lvs = 256;
    props.set(Variable("max_lvs", max_lvs));

    long long max_pvs = utils::to_long(words[VGS_MAX_PVS_IDX]);
    if (max_pvs == 0)
      max_pvs = 256;
    props.set(Variable("max_pvs", max_pvs));

    props.set(Variable("uuid", words[VGS_UUID_IDX]));

    // clustered
    bool clustered = (vg_attrs[5] == 'c');
    props.set(Variable("clustered", clustered, true));
  }

  // PVS
  for (map<String, String>::iterator iter = pv_to_vg.begin();
       iter != pv_to_vg.end();
       iter++) {
    if (iter->second == vgname)
      sources.push_back(BDFactory::get_bd(iter->first));
  }
  if (sources.empty() && !vgname.empty())
    throw String("invalid mapper_id");


  // LVS
  String out, err;
  int status;
  vector<String> args;
  args.push_back("lvdisplay");
  args.push_back("-c");
  if (utils::execute(LVM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0) {
    if (err.find("Skipping clustered") != err.npos)
      throw LVMClusterLockingError();
    throw String("lvdisplay failed");
  }
  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    String& line = *iter;
    vector<String> words = utils::split(utils::strip(line), ":");
    if (words.size() > 2)
      if (words[1] == vgname)
	targets.push_back(counting_auto_ptr<BD>(new LV(words[0])));
  }
}


void
LVM::pvcreate(const String& path)
{
  vector<String> args;
  args.push_back("pvcreate");
  args.push_back("-y");
  args.push_back("-f");
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("pvcreate failed");
  utils::clear_cache();
}

void
LVM::pvremove(const String& path)
{
  vector<String> args;
  args.push_back("pvremove");
  args.push_back("-y");
  args.push_back("-f");
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("pvremove failed");
  utils::clear_cache();
}


void
LVM::vgcreate(const String& vgname,
	      long long extent_size,
	      bool clustered,
	      const list<String>& pv_paths)
{
  if (clustered &&
      !clustered_enabled())
    throw LVMClusterLockingError();

  vector<String> args;
  args.push_back("vgcreate");
  args.push_back("--physicalextentsize");
  args.push_back(utils::to_string(extent_size/1024) + "k");
  args.push_back("-c");
  args.push_back(clustered ? "y" : "n");
  args.push_back(vgname);
  for (list<String>::const_iterator iter = pv_paths.begin();
       iter != pv_paths.end();
       iter++)
    args.push_back(*iter);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("vgcreate failed");
  utils::clear_cache();
}

void
LVM::vgremove(const String& vgname)
{
  vector<String> args;
  args.push_back("vgremove");
  args.push_back(vgname);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("vgremove failed");
  utils::clear_cache();
}

void
LVM::vgextend(const String& vgname,
	      const std::list<String>& pv_paths)
{
  vector<String> args;
  args.push_back("vgextend");
  args.push_back(vgname);
  for (list<String>::const_iterator iter = pv_paths.begin();
       iter != pv_paths.end();
       iter++)
    args.push_back(*iter);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("vgextend failed");
  utils::clear_cache();
}

void
LVM::vgreduce(const String& vgname,
	      const String& pv_path)
{
  vector<String> args;
  args.push_back("vgreduce");
  args.push_back(vgname);
  args.push_back(pv_path);
  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("vgreduce failed");
  utils::clear_cache();
}

void
LVM::vgchange(const String& vgname,
	      bool clustered)
{
  if (clustered &&
      !clustered_enabled())
    throw LVMClusterLockingError();

  vector<String> args;
  args.push_back("vgchange");
  args.push_back("-c");
  args.push_back(clustered ? "y" : "n");
  args.push_back(vgname);
  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("vgchange failed");
  utils::clear_cache();
}


void
LVM::lvcreate(const String& vgname,
	      const String& lvname,
	      long long size)
{
  vector<String> args;
  args.push_back("lvcreate");
  args.push_back("--name");
  args.push_back(lvname);
  args.push_back("--size");
  args.push_back(utils::to_string(size / 1024) + "k");
  args.push_back(vgname);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("lvcreate failed");
  utils::clear_cache();
}
void
LVM::lvcreate_snap(const String& lvname,
		   const String& origin_path,
		   long long size)
{
  vector<String> args;
  args.push_back("lvcreate");
  args.push_back("--snapshot");
  args.push_back("--name");
  args.push_back(lvname);
  args.push_back("--size");
  args.push_back(utils::to_string(size / 1024) + "k");
  args.push_back(origin_path);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("lvcreate failed");
  utils::clear_cache();
}

void
LVM::lvremove(const String& path)
{
	vector<String> args;
	args.push_back("lvchange");
	args.push_back("-an");
	args.push_back(path);

	String out, err;
	int status;

	if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(LVM_BIN_PATH);

	if (status != 0) {
		bool ignore_err = false;

		try {
			Props props;
			std::list<counting_auto_ptr<BD> > sources;
			std::list<counting_auto_ptr<BD> > targets;
			probe_vg(path, props, sources, targets);
			if (props.get("snapshot").get_bool() ||
				props.get("mirror").get_bool())
			{
				ignore_err = true;
			}
		} catch (...) {
			ignore_err = false;
		}

		if (!ignore_err)
			throw String("Unable to deactivate LV (might be in use by other cluster nodes)");
	}

  try {
    args.clear();
    args.push_back("lvremove");
    args.push_back("--force");
    args.push_back(path);

    if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
      throw command_not_found_error_msg(LVM_BIN_PATH);
    if (status != 0)
      throw String("lvremove failed");
    utils::clear_cache();
  } catch ( ... ) {
    args.clear();
    args.push_back("lvchange");
    args.push_back("-ay");
    args.push_back(path);
    utils::execute(LVM_BIN_PATH, args, out, err, status, false);
    utils::clear_cache();
    throw;
  }
}

void
LVM::lvreduce(const String& path, long long new_size)
{
  long long size_k = new_size / 1024;

  vector<String> args;
  args.push_back("lvreduce");
  args.push_back("-f");
  args.push_back("-L");
  args.push_back(utils::to_string(size_k) + "k");
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("lvreduce failed");
  utils::clear_cache();
}

void
LVM::lvextend(const String& path, long long new_size)
{
  long long size_k = new_size / 1024;

  vector<String> args;
  args.push_back("lvextend");
  args.push_back("-L");
  args.push_back(utils::to_string(size_k) + "k");
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(LVM_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVM_BIN_PATH);
  if (status != 0)
    throw String("lvextend failed");
  utils::clear_cache();
}


bool
LVM::clustered_enabled()
{
  String locking_type = get_locking_type();
  return (locking_type == "2" || locking_type == "3");
}

void
LVM::enable_clustered()
{
  String out, err;
  int status;
  vector<String> args;
  args.push_back("--enable-cluster");
  if (utils::execute(LVMCONF_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVMCONF_PATH);
  if (status != 0)
    throw String("Failed to enable LVM's clustered locking");
}

void
LVM::disable_clustered()
{
  String out, err;
  int status;
  vector<String> args;
  args.push_back("--disable-cluster");
  if (utils::execute(LVMCONF_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(LVMCONF_PATH);
  if (status != 0)
    throw String("Failed to disable LVM's clustered locking");
}


bool
cluster_quorate()
{
  // called only if cluster locking is enabled

  bool use_magma = true;
  if (access("/sbin/magma_tool", X_OK))
    use_magma = false;

  if (use_magma) {
    // use magma_tool
    String out, err;
    int status;
    vector<String> args;
    args.push_back("quorum");
    if (utils::execute("/sbin/magma_tool", args, out, err, status))
      throw command_not_found_error_msg("magma_tool");
    if (status)
      throw ClusterNotRunningError();
    return out.find("Quorate") != out.npos;
  } else {
    // use cman_tool
    String cman_tool_path = "/sbin/cman_tool";
    if (access(cman_tool_path.c_str(), X_OK))
      cman_tool_path = "/usr/sbin/cman_tool";

    String out, err;
    int status;
    vector<String> args;
    args.push_back("status");
    if (utils::execute(cman_tool_path, args, out, err, status))
      throw command_not_found_error_msg("cman_tool");
    if (status)
      throw ClusterNotRunningError();

    long long quorum = -1;
    long long votes = -1;
    vector<String> lines = utils::split(utils::strip(out), "\n");
    for (vector<String>::const_iterator iter = lines.begin();
	 iter != lines.end();
	 iter++) {
      vector<String> words = utils::split(*iter);
      if (words.size() < 2)
	continue;
      if (words[0] == "Quorum:")
	quorum = utils::to_long(words[1]);
      if (words[0] == "Total_votes:")
	votes = utils::to_long(words[1]);
      if (words.size() < 3)
	continue;
      if (words[0] == "Total" &&
	  words[1] == "votes:")
	votes = utils::to_long(words[2]);
    }

    if (quorum <= 0 ||
	votes < 0)
      throw String("Unable to determine cluster quorum status");
    return votes >= quorum;
  }
}

void
LVM::check_locking()
{
  if (clustered_enabled()) {
    if (!cluster_quorate())
      throw ClusterNotQuorateError();

    // try to start clvmd, if not running
    String out, err;
    int status;
    vector<String> args;
    args.push_back("clvmd");
    args.push_back("start");
    if (utils::execute("/sbin/service", args, out, err, status))
      throw command_not_found_error_msg("service");
    if (status)
      throw ClvmdError();
  }
}

String
get_locking_type()
{
  String out, err;
  int status;
  vector<String> args;
  args.push_back("locking_type");
  args.push_back("/etc/lvm/lvm.conf");
  if (utils::execute("/bin/grep", args, out, err, status))
    throw command_not_found_error_msg("grep");
  vector<String> lines(utils::split(utils::strip(out), "\n"));
  for (vector<String>::const_iterator line = lines.begin();
       line != lines.end();
       line++) {
    vector<String> words(utils::split(utils::strip(*line)));
    if (words.size() < 3)
      continue;
    if (words[0] == "locking_type" &&
	words[1] == "=") {
      return words[2];
    }
  }
  throw String("locking_type not found in lvm.conf!!!");
}

vector<String>
vg_props(const String& vgname)
{
  String out, err;
  int status;
  vector<String> args;
  args.push_back("vgs");
  args.push_back("--nosuffix");
  args.push_back("--noheadings");
  args.push_back("--units");
  args.push_back("b");
  args.push_back("--separator");
  args.push_back(";");
  args.push_back("-o");
  args.push_back(VGS_OPTIONS_STRING);
  //    args.push_back(vgname);
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
    vector<String> words = utils::split(line, ";");
    if (words.size() < VGS_OPTIONS_LENGTH)
      continue;
    if (words[VGS_NAME_IDX] == vgname) {
      return words;
    }
  }
  throw String("no such vg");
}

bool
LVM::vg_clustered(const String& vgname)
{
  if (vgname.empty())
    return false;
  return vg_props(vgname)[VGS_ATTR_IDX][5] == 'c';
}



const map<String, PV_data>
probe_pvs()
{
  map<String, PV_data> pvs;

  String out, err;
  int status;
  vector<String> args;
  args.push_back("pvs");
  args.push_back("--nosuffix");
  args.push_back("--noheadings");
  args.push_back("--units");
  args.push_back("b");
  args.push_back("--separator");
  args.push_back(";");
  args.push_back("-o");
  args.push_back(PVS_OPTIONS);
  if (utils::execute(LVM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(LVM_BIN_PATH);

  bool use_pvdisplay = false;
  if (status) {
    if (err.find("Skipping clustered") != err.npos)
      throw LVMClusterLockingError();
    // wouldn't `pvdisplay -c` fail if `pvs` has already failed?
    // `pvs` fails if it cannot read one hard drive (common in SANs),
    // while pvdisplay reports without failure
    use_pvdisplay = true;
  }

  if (use_pvdisplay) {
    args.clear();
    args.push_back("pvdisplay");
    args.push_back("-c");
    if (utils::execute(LVM_BIN_PATH, args, out, err, status))
      throw command_not_found_error_msg(LVM_BIN_PATH);
    if (status) {
      if (err.find("Skipping clustered") != err.npos)
	throw LVMClusterLockingError();
      throw String("pvs and pvdisplay failed");
    }

    vector<String> lines = utils::split(utils::strip(out), "\n");
    for (vector<String>::iterator iter = lines.begin();
	 iter != lines.end();
	 iter++) {
      vector<String> words = utils::split(utils::strip(*iter), ":");
      if (words.size() >= PVDISPLAY_c_OPTIONS_LENGTH) {
	String path(utils::strip(words[PVDISPLAY_c_NAME_IDX]));
	String vgname(utils::strip(words[PVDISPLAY_c_VG_NAME_IDX]));
	long long extent_size = utils::to_long(words[PVDISPLAY_c_EXTENT_SIZE_IDX]);
	extent_size *= 1024;
	long long size = utils::to_long(words[PVDISPLAY_c_SIZE_IDX]);
	size = size / 2 * 1024;
	long long size_free = utils::to_long(words[PVDISPLAY_c_FREE_EXT_IDX]);
	size_free *= extent_size;
	if (vgname.empty())
	  size_free = size;
	String uuid(words[PVDISPLAY_c_UUID_IDX]);

	// pvdisplay doesn't report attr and format
	// guess
	// FIXME: probe somewhere else
	String attrs;
	if (vgname.empty())
	  attrs = "--";
	else
	  attrs = "a-";
	String format = "lvm2";

	pvs[path] = PV_data(path,
			    vgname,
			    extent_size,
			    size,
			    size_free,
			    uuid,
			    attrs,
			    format);
      }
    }

  } else {
    vector<String> lines = utils::split(utils::strip(out), "\n");
    for (vector<String>::iterator iter = lines.begin();
	 iter != lines.end();
	 iter++) {
      vector<String> words = utils::split(utils::strip(*iter), ";");
      if (words.size() >= PVS_OPTIONS_LENGTH) {
	String path(utils::strip(words[PVS_NAME_IDX]));
	String vgname(utils::strip(words[PVS_VG_NAME_IDX]));
	long long extent_size = utils::to_long(words[PVS_EXTENT_SIZE_IDX]);
	long long size = utils::to_long(words[PVS_SIZE_IDX]);
	long long size_free = utils::to_long(words[PVS_FREE_IDX]);
	String uuid(words[PVS_UUID_IDX]);
	String attrs(words[PVS_ATTR_IDX]);
	String format(words[PVS_FMT_IDX]);

	pvs[path] = PV_data(path,
			    vgname,
			    extent_size,
			    size,
			    size_free,
			    uuid,
			    attrs,
			    format);
      }
    }
  }

  return pvs;
}

