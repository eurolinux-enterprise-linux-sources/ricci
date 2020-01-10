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


#include "UnsupportedFS.h"
#include "FileMagic.h"
#include "utils.h"


UnsupportedFS::UnsupportedFS(const String& path) :
  ContentFS(path, "bug", "bug")
{
  String magic(FileMagic::get_description(path));
  magic = utils::to_lower(magic);
  if (magic.find("minix filesystem") != magic.npos) {
    _name   = "minix";
    _module = "minix";
  } else if (magic.find("unix fast file") != magic.npos) {
    _name   = "ufs";
    _module = "ufs";
  } else if (magic.find("sgi xfs") != magic.npos) {
    _name   = "xfs";
    _module = "xfs";
  } else if (magic.find("iso 9660 cd-rom") != magic.npos) {
    _name   = "isofs";
    _module = "isofs";
  } else if (magic.find("linux compressed rom file system") != magic.npos) {
    _name   = "cramfs";
    _module = "cramfs";
  } else if (magic.find("reiserfs") != magic.npos) {
    _name   = "reiserfs";
    _module = "reiserfs";
  } else if (magic.find("linux journalled flash file system") != magic.npos) {
    _name   = "jffs";
    _module = "jffs";
  } else if (magic.find("jffs2") != magic.npos) {
    _name   = "jffs2";
    _module = "jffs2";
  } else if (magic.find("squashfs") != magic.npos) {
    _name   = "squashfs";
    _module = "squashfs";
  } else if (magic.find("fat (12 bit)") != magic.npos ||
	     magic.find("fat (16 bit)") != magic.npos ||
	     magic.find("fat (32 bit)") != magic.npos) {
    _name   = "vfat";
    _module = "vfat";
  } else if (magic.find("msdos") != magic.npos ||
	     magic.find("ms-dos") != magic.npos) {
    _name   = "msdos";
    _module = "msdos";
  } else if (magic.find("affs") != magic.npos) {
    _name   = "affs";
    _module = "affs";
  } else if (magic.find("befs") != magic.npos) {
    _name   = "befs";
    _module = "befs";
  } else if (magic.find("bfs") != magic.npos) {
    _name   = "bfs";
    _module = "bfs";
  } else if (magic.find("jfs") != magic.npos) {
    _name   = "jfs";
    _module = "jfs";
  } else if (magic.find("efs") != magic.npos) {
    _name   = "efs";
    _module = "efs";
  } else if (magic.find("vxfs") != magic.npos) {
    _name   = "freevxfs";
    _module = "freevxfs";
  } else if (magic.find("hfsplus") != magic.npos) {
    _name   = "hfsplus";
    _module = "hfsplus";
  } else if (magic.find("hfs") != magic.npos) {
    _name   = "hfs";
    _module = "hfs";
  } else if (magic.find("ncpfs") != magic.npos) {
    _name   = "ncpfs";
    _module = "ncpfs";
  } else if (magic.find("ocfs2") != magic.npos) {
    _name   = "ocfs2";
    _module = "ocfs2";
  } else if (magic.find("relayfs") != magic.npos) {
    _name   = "relayfs";
    _module = "relayfs";
  } else if (magic.find("udf") != magic.npos) {
    _name   = "udf";
    _module = "udf";



  } else
    throw String("not even an unsupported FS");

  // mountpoints
  mount_props_probe(path, _module, _props);
}

UnsupportedFS::~UnsupportedFS()
{}


bool
UnsupportedFS::expandable(long long& max_size) const
{
  return false;
}

void
UnsupportedFS::expand(const String& path,
	     unsigned long long new_size,
	     const Props& new_props)
{}

bool
UnsupportedFS::shrinkable(long long& min_size) const
{
  return false;
}

void
UnsupportedFS::shrink(const String& path,
	     unsigned long long new_size,
	     const Props& new_props)
{}

void
UnsupportedFS::apply_props_before_resize(const String& path,
				unsigned long long old_size,
				unsigned long long new_size,
				const Props& new_props)
{}

void
UnsupportedFS::apply_props_after_resize(const String& path,
			       unsigned long long old_size,
			       unsigned long long new_size,
			       const Props& new_props)
{
  // mountpoints
  mount_props_apply(path, _module, _props, new_props);
}

bool
UnsupportedFS::removable() const
{
  return true;
}
