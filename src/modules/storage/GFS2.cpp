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


#include "GFS2.h"
#include "utils.h"
#include "defines.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>


#include <iostream>
using namespace std;


#define __be64 uint64_t
#define __be32 uint32_t
#define __be16 uint16_t
#define __u32  uint32_t
#define __u16  uint16_t
#define __u8   uint8_t
#include "gfs2_ondisk.h"


const String GFS2::PRETTY_NAME("gfs2");
const static String gfs2_module("gfs2");

const static String MKFS_GFS2_path("/sbin/mkfs.gfs2");
const static long long DEF_JSIZE = 32 * 1024 * 1024;

static void
detect_gfs2(const String& filename,
	    long long &block_size,
	    String& locking_protocol,
	    String& locking_table);




GFS2::GFS2(const String& path) :
  ContentFS(path, PRETTY_NAME, gfs2_module)
{
  long long bs;
  String proto, table;
  detect_gfs2(path, bs, proto, table);

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
  mount_props_probe(path, _module, _props);
}

GFS2::~GFS2()
{}


bool
GFS2::expandable(long long& max_size) const
{
  return false;
}

void
GFS2::expand(const String& path,
	     unsigned long long new_size,
	     const Props& new_props)
{}

bool
GFS2::shrinkable(long long& min_size) const
{
  return false;
}

void
GFS2::shrink(const String& path,
	     unsigned long long new_size,
	     const Props& new_props)
{}

void
GFS2::apply_props_before_resize(const String& path,
				unsigned long long old_size,
				unsigned long long new_size,
				const Props& new_props)
{}

void
GFS2::apply_props_after_resize(const String& path,
			       unsigned long long old_size,
			       unsigned long long new_size,
			       const Props& new_props)
{
  // mountpoints
  mount_props_apply(path, _module, _props, new_props);
}

bool
GFS2::removable() const
{
  return true;
}





void
create_GFS2(const String& path,
	    const counting_auto_ptr<ContentTemplate>& templ)
{
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
  if (utils::execute(MKFS_GFS2_path, args, out, err, status))
    throw command_not_found_error_msg(MKFS_GFS2_path);
  if (status)
    throw MKFS_GFS2_path + " " + path + " failed";

  // mountpoints
  ContentFSTemplate::mount_props_create(path, gfs2_module, templ->_props);
}

GFS2Template::GFS2Template() :
  ContentFSTemplate(GFS2::PRETTY_NAME)
{
  if (access(MKFS_GFS2_path.c_str(), X_OK|R_OK))
    throw String("no mkfs.gfs2 exists");

  // ## general options ##

  // mountpoints
  mount_props_template(gfs2_module, _props);

  // journals
  _props.set(Variable("journals_num", 1, 1, 128, 1));
  list<long long> jsizes;
  long long jsize = 8*1024*1024;
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
  String name = "unique_gfs_name";

  long long jnum = 1;
  if (cluster_exists) {
    for (list<XMLObject>::const_iterator iter = cluster_conf.children().begin();
	 iter != cluster_conf.children().end();
	 iter++)
      if (iter->tag() == "clusternodes")
	jnum = iter->children().size();
    _props.set(Variable("journals_num", jnum, 1, 128, 1));
  }

  _props.set(Variable("clustered", cluster_exists, cluster_exists));

  Variable proto_var("locking_protocol", String("dlm"));
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

GFS2Template::~GFS2Template()
{}







void
detect_gfs2(const String& filename,
	    long long &block_size,
	    String& locking_protocol,
	    String& locking_table)
{
  // IMPORTANT: gfs2 saves metadata as BigEndian

  int fd = open(filename.c_str(), O_RDONLY);
  if (fd < 0)
    throw String("GFS2_detect: cannot open ") + filename;

  try {
    struct gfs2_sb sb;

    int ret;
    do {
      const static int sb_offset = GFS2_SB_ADDR * GFS2_BASIC_BLOCK;
      if (lseek(fd, sb_offset, SEEK_SET) != sb_offset)
	throw String("lseek failed for ") + filename;
      ret = read(fd, &sb, sizeof(sb));
    } while (ret == -1 &&
	     errno == EINTR);
    if (ret != (int) sizeof(sb))
      throw String("read of gfs2 superblock failed for ") + filename;

    if (ntohl(sb.sb_header.mh_magic) != GFS2_MAGIC ||
	ntohl(sb.sb_header.mh_type) != GFS2_METATYPE_SB ||
	ntohl(sb.sb_fs_format) != GFS2_FORMAT_FS ||
	ntohl(sb.sb_multihost_format) != GFS2_FORMAT_MULTI)
      throw filename + " does not contain gfs2";

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
