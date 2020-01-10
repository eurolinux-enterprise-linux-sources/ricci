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


#include "ExtendedFS.h"
#include "utils.h"
#include "MountHandler.h"
#include "UMountError.h"
#include "FileMagic.h"

#include <algorithm>
#include <iostream>

using namespace std;


const static String MKE2FS_path("/sbin/mke2fs");
const String ExtendedFS::PRETTY_NAME("ext");



ExtendedFS::ExtendedFS(const String& path) :
  ContentFS(path,
	    PRETTY_NAME,
	    "ext2"),
  _journaled(false)
{
  String magic(FileMagic::get_description(path));
  if (magic.find("ext3") == magic.npos &&
      magic.find("ext2") == magic.npos)
    throw path + ": not an " + _name;

  // dumpe2fs
  String out, err;
  int status;
  vector<String> args;
  args.push_back("-h");
  args.push_back(path);
  if (utils::execute("/sbin/dumpe2fs", args, out, err, status))
    throw command_not_found_error_msg("dumpe2fs");
  if (status)
    throw String("dumpe2fs failed");
  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    String& line = *iter;
    vector<String> words = utils::split(utils::strip(line));
    if (words.size() < 3)
      continue;

    if (words[0] == "Filesystem" && words[1] == "features:") {
      if (find(words.begin(), words.end(), "dir_index") != words.end())
	_props.set(Variable("dir_index", true, true));
      else
	_props.set(Variable("dir_index", false, true));
      if (find(words.begin(), words.end(), "has_journal") != words.end())
	_journaled = true;
      continue;
    }

    if (words.size() == 3) {
      if (words[0] == "Block" && words[1] == "size:")
	_props.set(Variable("block_size", utils::to_long(words[2])));
      else if (words[0] == "Block" && words[1] == "count:")
	_props.set(Variable("block_count", utils::to_long(words[2])));
      else if (words[0] == "Filesystem" && words[1] == "state:")
	_props.set(Variable("state", words[2]));
      else if (words[0] == "Filesystem" && words[1] == "UUID:") {
	String uuid(words[2]);
	if (uuid == "<none>")
	  uuid.clear();
	_props.set(Variable("uuid", uuid));
      }
    } else if (words.size() == 4) {
      if (words[0] == "Filesystem" && words[1] == "volume" && words[2] == "name:") {
	String label = words[3];
	if (label == "<none>")
	  label.clear();
	//	_props.set(Variable("label", label, 0, 16, ILLEGAL_LABEL_CHARS, list<String>()));
	_props.set(Variable("label", label));
      }
    }
  }

  long long block_count = _props.get("block_count").get_int();

  if (_journaled)
    _module = "ext3";
  else
    _module = "ext2";

  // journaling
  _props.set(Variable("has_journal",
		      _journaled,
		      (!_journaled && block_count > 1024)));

  mount_props_probe(path,
		    _module,
		    _props);
}

ExtendedFS::~ExtendedFS()
{}


bool
ExtendedFS::expandable(long long& max_size) const
{
  bool mounted = mount_props_mounted(_props);

  long long bs = _props.get("block_size").get_int();
  long long bc = _props.get("block_count").get_int();
  long long size = bs * bc;

  long long step = size;
  if (bs == 1024)
    step = 256LL * 1024 * 1024; // 256 MB
  else if (bs == 2048)
    step = 2LL * 1024 * 1024 * 1024; // 2GB
  else if (bs == 4096)
    step = 16LL * 1024 * 1024 * 1024; // 16GB

  max_size = ((size + step - 1) / step) * step;
  return mounted && _journaled;
}

void
ExtendedFS::expand(const String& path,
		   unsigned long long new_size,
		   const Props& new_props)
{
  String extend_cmd("/usr/sbin/ext2online");
  if (access(extend_cmd.c_str(), X_OK))
    extend_cmd = "/sbin/resize2fs";

  vector<String> args;
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(extend_cmd, args, out, err, status))
    throw command_not_found_error_msg(extend_cmd);
  if (status)
    throw String(extend_cmd + " failed");
}

bool
ExtendedFS::shrinkable(long long& min_size) const
{
  return false;
}

void
ExtendedFS::shrink(const String& path,
		   unsigned long long new_size,
		   const Props& new_props)
{}

void
ExtendedFS::apply_props_before_resize(const String& path,
				      unsigned long long old_size,
				      unsigned long long new_size,
				      const Props& new_props)
{
  // has_journal
  bool j_old = _props.get("has_journal").get_bool();
  bool j_new = new_props.get("has_journal").get_bool();
  if (!j_old && j_new)
    enable_journal(path);

  // dir_index
  bool index_old = _props.get("dir_index").get_bool();
  bool index_new = new_props.get("dir_index").get_bool();
  if (index_old != index_new) {
    vector<String> args;
    String out, err, bin("/sbin/tune2fs");
    int status;
    if (!index_old && index_new) {
      args.push_back("-O");
      args.push_back("+dir_index");
    } else if (index_old && !index_new) {
      args.push_back("-O");
      args.push_back("^dir_index");
    }
    args.push_back(path);
    if (utils::execute(bin, args, out, err, status, false))
      throw command_not_found_error_msg(bin);
    if (status)
      throw bin + " failed";
  }



  // label
  /*
  String old_label = _props.get("label").get_string();
  String new_label = new_props.get("label").get_string();
  if (old_label != new_label) {
    FstabLocker lock;
    MountHandler mh;

    list<Mountpoint> l = mh.fstabs(mh.maj_min(path));

    vector<String> args;
    args.push_back("-L");
    args.push_back(new_label);
    args.push_back(path);
    String out, err;
    int status;
    if (utils::execute("/sbin/tune2fs", args, out, err, status, false))
      throw command_not_found_error_msg();
    if (status)
      throw String("tune2fs failed");

    args.clear();
    args.push_back("1");
    if (utils::execute("/bin/sleep", args, out, err, status, false))
      throw command_not_found_error_msg();
    if (status)
      throw String("sleep failed");

    for (list<Mountpoint>::const_iterator iter = l.begin();
	 iter != l.end();
	 iter++)
      if (iter->devname == (String("LABEL=") + old_label)) {
	mh.fstab_remove(iter->devname, iter->mountpoint);
	mh.fstab_add(String("LABEL=") + new_label, iter->mountpoint, "ext3");
      }
  }
  */
}

void
ExtendedFS::enable_journal(const String& path)
{
  FstabLocker lock;
  MountHandler mh;

  list<Mountpoint> mounts = mh.mounts(mh.maj_min(path));
  list<Mountpoint> fstabs = mh.fstabs(mh.maj_min(path));

  list<Mountpoint> rollback;
  try {
    // umount
    for (list<Mountpoint>::const_iterator iter = mounts.begin();
	 iter != mounts.end();
	 iter++) {
      if (!mh.umount(iter->devname, iter->mountpoint))
	throw UMountError(iter->mountpoint);
      rollback.push_back(*iter);
    }

    // add journal
    vector<String> args;
    String out, err, bin("/sbin/tune2fs");
    int status;
    args.push_back("-j");
    args.push_back(path);
    if (utils::execute(bin, args, out, err, status, false))
      throw command_not_found_error_msg(bin);
    if (status)
      throw bin + " failed";
    _module = "ext3";

  } catch ( ... ) {
    for (list<Mountpoint>::const_iterator iter = rollback.begin();
	 iter != rollback.end();
	 iter++)
      try {
	mh.mount(iter->devname, iter->mountpoint, _module);
      } catch ( ... ) {}
    throw;
  }

  // modify fstab
  for (list<Mountpoint>::const_iterator iter = fstabs.begin();
       iter != fstabs.end();
       iter++) {
    mh.fstab_remove(iter->devname, iter->mountpoint);
    mh.fstab_add(iter->devname, iter->mountpoint, _module);
  }

  // mount
  for (list<Mountpoint>::const_iterator iter = mounts.begin();
       iter != mounts.end();
       iter++)
    if (!mh.mount(iter->devname, iter->mountpoint, _module))
      throw String("mount failed");
}

void
ExtendedFS::apply_props_after_resize(const String& path,
				     unsigned long long old_size,
				     unsigned long long new_size,
				     const Props& new_props)
{
  mount_props_apply(path,
		    _module,
		    _props,
		    new_props);
}

bool
ExtendedFS::removable() const
{
  return true;
}





void
create_extended_fs(const String& path,
		   const counting_auto_ptr<ContentTemplate>& templ)
{
  String label = templ->_props.get("label").get_string();
  String bs = utils::to_string(templ->_props.get("block_size").get_int());
  bool dir_index = templ->_props.get("dir_index").get_bool();
  bool has_journal = templ->_props.get("has_journal").get_bool();

  vector<String> args;
  if (!label.empty()) {
    args.push_back("-L");
    args.push_back(label);
  }
  if (!bs.empty()) {
    args.push_back("-b");
    args.push_back(bs);
  }
  if (dir_index) {
    args.push_back("-O");
    args.push_back("dir_index");
  }
  if (has_journal)
    args.push_back("-j");
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute(MKE2FS_path, args, out, err, status))
    throw command_not_found_error_msg(MKE2FS_path);
  if (status != 0)
    throw MKE2FS_path + " failed";

  // mountpoints
  ContentFSTemplate::mount_props_create(path,
					(has_journal) ? "ext3" : "ext2",
					templ->_props);
}

ExtendedFSTemplate::ExtendedFSTemplate() :
  ContentFSTemplate(ExtendedFS::PRETTY_NAME)
{
  if (access(MKE2FS_path.c_str(), X_OK|R_OK))
    throw String("no mke2fs exists");

  bool ext3_supported = ContentFS::mount_fs_supported("ext3");

  _props.set(Variable("label",
		      "",
		      0,
		      16,
		      ILLEGAL_LABEL_CHARS,
		      list<String>()));

  _props.set(Variable("dir_index", true, true));

  _props.set(Variable("has_journal",
		      ext3_supported,
		      ext3_supported));

  mount_props_template(ext3_supported ? "ext3" : "ext2",
		       _props);


  // block_size
  list<long long> b_sizes;
  b_sizes.push_back(1024);
  b_sizes.push_back(2048);
  b_sizes.push_back(4096);
  _props.set(Variable("block_size", 4096, b_sizes));
}

ExtendedFSTemplate::~ExtendedFSTemplate()
{}

