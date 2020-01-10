/*
  Copyright Red Hat, Inc. 2006

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


#include "GFS1.h"
#include "utils.h"
#include "defines.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>


#include <iostream>
using namespace std;


#include "gfs_ondisk.h"


const String GFS1::PRETTY_NAME("gfs1");
const static String gfs1_module("gfs");

const static String MKFS_GFS1_path("/sbin/mkfs.gfs");
const static long long DEF_JSIZE = 128 * 1024 * 1024;

static void
detect_gfs1(const String& filename,
	    long long &block_size,
	    String& locking_protocol,
	    String& locking_table);




GFS1::GFS1(const String& path) :
  ContentFS(path, PRETTY_NAME, gfs1_module)
{
  long long bs;
  String proto, table;
  detect_gfs1(path, bs, proto, table);

  _props.set(Variable("block_size", bs));

  // cluster
  String cluster = table.substr(0, table.find(":"));
  String name = table.substr(table.find(":") + 1);
  _props.set(Variable("clustered", proto != "lock_nolock"));
  Variable proto_var("locking_protocol",
		     proto.substr(proto.find("_") + 1));
  proto_var.set_conditional_bool_if("clustered");
  _props.set(proto_var);
  Variable cluster_var("cluster_name", cluster);
  cluster_var.set_conditional_bool_if("clustered");
  _props.set(cluster_var);
  Variable name_var("gfs_fsname", name);
  name_var.set_conditional_bool_if("clustered");
  _props.set(name_var);

  // mountpoints
  mount_props_probe(path,
		    _module,
		    _props);
}

GFS1::~GFS1()
{}


bool
GFS1::expandable(long long& max_size) const
{
  return false;
}

void
GFS1::expand(const String& path,
	     unsigned long long new_size,
	     const Props& new_props)
{}

bool
GFS1::shrinkable(long long& min_size) const
{
  return false;
}

void
GFS1::shrink(const String& path,
	     unsigned long long new_size,
	     const Props& new_props)
{}

void
GFS1::apply_props_before_resize(const String& path,
				unsigned long long old_size,
				unsigned long long new_size,
				const Props& new_props)
{}

void
GFS1::apply_props_after_resize(const String& path,
			       unsigned long long old_size,
			       unsigned long long new_size,
			       const Props& new_props)
{
  // mountpoints
  mount_props_apply(path, _module, _props, new_props);
}

bool
GFS1::removable() const
{
  return true;
}





void
create_GFS1(const String& path,
	    const counting_auto_ptr<ContentTemplate>& templ)
{
  String bs = utils::to_string(templ->_props.get("block_size").get_int());
  String jnum = utils::to_string(templ->_props.get("journals_num").get_int());
  long long jsize_bytes = templ->_props.get("journal_size").get_int();
  String jsize = utils::to_string(jsize_bytes / 1024 / 1024);

  bool clustered = templ->_props.get("clustered").get_bool();
  String proto("nolock");
  String table;
  if (clustered) {
    proto = templ->_props.get("locking_protocol").get_string();
    table = templ->_props.get("cluster_name").get_string() +
      ":" +
      templ->_props.get("gfs_fsname").get_string();
  }
  proto = String("lock_") + proto;


  vector<String> args;
  args.push_back("-b");
  args.push_back(bs);

  args.push_back("-J");
  args.push_back(jsize);

  args.push_back("-j");
  args.push_back(jnum);

  args.push_back("-O"); // no prompt

  args.push_back("-p");
  args.push_back(proto);

  if (!table.empty()) {
    args.push_back("-t");
    args.push_back(table);
  }

  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(MKFS_GFS1_path, args, out, err, status))
    throw command_not_found_error_msg(MKFS_GFS1_path);
  if (status)
    throw MKFS_GFS1_path + " " + path + " failed";

  // mountpoints
  ContentFSTemplate::mount_props_create(path, gfs1_module, templ->_props);
}

GFS1Template::GFS1Template() :
  ContentFSTemplate(GFS1::PRETTY_NAME)
{
  if (access(MKFS_GFS1_path.c_str(), X_OK|R_OK))
    throw String("no mkfs.gfs exists");

  // ## general options ##

  // mountpoints
  mount_props_template(gfs1_module, _props);

  // block_size
  list<long long> b_sizes;
  b_sizes.push_back(512);
  b_sizes.push_back(1024);
  b_sizes.push_back(2048);
  b_sizes.push_back(4096);
  _props.set(Variable("block_size", 4096, b_sizes));

  // journals
  _props.set(Variable("journals_num", 1, 1, 128, 1));
  list<long long> jsizes;
  long long jsize = 32 * 1024 * 1024;
  for (int i = 1; i<11; i++) {
    jsizes.push_back(jsize);
    jsize *= 2;
  }
  _props.set(Variable("journal_size", DEF_JSIZE, jsizes));


  // ## cluster options ##

  bool cluster_exists = false;
  XMLObject cluster_conf;
  try {
    cluster_conf = readXML("/etc/cluster/cluster.conf");
    if (cluster_conf.tag() != "cluster" ||
	cluster_conf.get_attr("name").empty() ||
	cluster_conf.get_attr("config_version").empty())
      throw String("invalid cluster.conf");
    cluster_exists = true;
  } catch ( ... ) {}
  String cluster = cluster_conf.get_attr("name");
  String name    = "unique_gfs_name";
  String proto   = "dlm";

  long long jnum = 1;
  if (cluster_exists) {
    for (list<XMLObject>::const_iterator iter = cluster_conf.children().begin();
	 iter != cluster_conf.children().end();
	 iter++) {
      if (iter->tag() == "clusternodes")
	jnum = iter->children().size();
      if (iter->tag() == "gulm")
	proto = "gulm";
    }
    _props.set(Variable("journals_num", jnum, 1, 128, 1));
  }

  _props.set(Variable("clustered", cluster_exists, cluster_exists));

  Variable proto_var("locking_protocol", proto);
  proto_var.set_conditional_bool_if("clustered");
  _props.set(proto_var);
  Variable cluster_var("cluster_name", cluster);
  cluster_var.set_conditional_bool_if("clustered");
  _props.set(cluster_var);
  Variable name_var("gfs_fsname",
		    name,
		    1,
		    16,
		    NAMES_ILLEGAL_CHARS,
		    list<String>());
  name_var.set_conditional_bool_if("clustered");
  _props.set(name_var);
}

GFS1Template::~GFS1Template()
{}







void
detect_gfs1(const String& filename,
	    long long &block_size,
	    String& locking_protocol,
	    String& locking_table)
{
  // IMPORTANT: gfs saves metadata as BigEndian

  int fd = open(filename.c_str(), O_RDONLY);
  if (fd < 0)
    throw String("GFS1_detect: cannot open ") + filename;

  try {
    struct gfs_sb sb;

    int ret;
    do {
      const static int sb_offset = GFS_SB_ADDR * GFS_BASIC_BLOCK;
      if (lseek(fd, sb_offset, SEEK_SET) != sb_offset)
	throw String("lseek failed for ") + filename;
      ret = read(fd, &sb, sizeof(sb));
    } while (ret == -1 &&
	     errno == EINTR);
    if (ret != (int) sizeof(sb))
      throw String("read of gfs superblock failed for ") + filename;

    if (ntohl(sb.sb_header.mh_magic) != GFS_MAGIC ||
	ntohl(sb.sb_header.mh_type) != GFS_METATYPE_SB ||
	ntohl(sb.sb_fs_format) != GFS_FORMAT_FS ||
	ntohl(sb.sb_multihost_format) != GFS_FORMAT_MULTI)
      throw filename + " does not contain gfs";

    block_size = ntohl(sb.sb_bsize);

    sb.sb_lockproto[sizeof(sb.sb_lockproto) - 1] = '\0';
    locking_protocol = String(sb.sb_lockproto);

    sb.sb_locktable[sizeof(sb.sb_locktable) - 1] = '\0';
    locking_table = String(sb.sb_locktable);

    while (close(fd))
      if (errno != EINTR)
	break;
  } catch ( ... ) {
    while (close(fd))
      if (errno != EINTR)
	break;
    throw;
  }
}
