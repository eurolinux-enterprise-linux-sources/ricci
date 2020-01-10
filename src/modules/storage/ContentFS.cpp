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


#include "ContentFS.h"
#include "MountHandler.h"
#include "UMountError.h"
#include "ContentNone.h"
#include "BDFactory.h"
#include "defines.h"
#include "utils.h"
#include <algorithm>


using namespace std;


#define ILLEGAL_MOUNT_CHARS   String(" <>?'\":;|}]{[)(*&^%$#@!~`+=")



ContentFS::ContentFS(const String& path,
		     const String& name,
		     const String& module) :
  Content(CONTENT_FS_TYPE, path),
  _name(name),
  _module(module)
{}

ContentFS::~ContentFS()
{}


bool
ContentFS::removable() const
{
  return false;
}

void
ContentFS::remove()
{
  if (!this->removable())
    throw String("FS not removable");

  FstabLocker lock;
  MountHandler mh;

  // mountpoint
  list<Mountpoint> l = mh.mounts(mh.maj_min(path()));
  list<Mountpoint> backup;
  for (list<Mountpoint>::const_iterator iter = l.begin();
       iter != l.end();
       iter++) {
    if (mh.umount(iter->devname, iter->mountpoint))
      backup.push_back(*iter);
    else {
      for (list<Mountpoint>::const_iterator iter_b = backup.begin();
	   iter_b != backup.end();
	   iter_b++)
	mh.mount(iter_b->devname, iter->mountpoint, iter->fstype);
      throw UMountError(iter->mountpoint);
    }
  }

  list<Mountpoint> f = mh.fstabs(mh.maj_min(path()));
  for (list<Mountpoint>::const_iterator iter = f.begin();
       iter != f.end();
       iter++)
    mh.fstab_remove(iter->devname, iter->mountpoint);

  create_content_none(BDFactory::get_bd(path()));
}


XMLObject
ContentFS::xml() const
{
  XMLObject xml = this->Content::xml();
  xml.set_attr("fs_type", _name);
  return xml;
}




void
ContentFS::mount_props_probe(const String& path,
			     const String& fsname,
			     Props& props)
{
  FstabLocker l;
  MountHandler mh;

  // mountpoint
  pair<unsigned int, unsigned int> maj_min = mh.maj_min(path);
  list<Mountpoint> mounts = mh.mounts(maj_min);
  String mountpoint;
  if (mounts.size())
    mountpoint = mounts.front().mountpoint;
  list<String> ill_mnts = mh.used_dirs();
  ill_mnts.remove(mountpoint);
  // fstab
  list<Mountpoint> fstabs = mh.fstabs(maj_min);
  String fstabpoint;
  if (fstabs.size())
    fstabpoint = fstabs.front().mountpoint;
  ill_mnts.remove(fstabpoint);



  bool mountable = mount_fs_supported(fsname);
  props.set(Variable("mountable", mountable));
  if (mountable)
    props.set(Variable("mountpoint",
		       mountpoint,
		       0,
		       128,
		       ILLEGAL_MOUNT_CHARS,
		       ill_mnts));
  if (mountable ||
      !fstabpoint.empty())
    props.set(Variable("fstabpoint",
		       fstabpoint,
		       0,
		       128,
		       ILLEGAL_MOUNT_CHARS,
		       ill_mnts));
}

void
ContentFS::mount_props_apply(const String& path,
			     const String& fsname,
			     const Props& old_props,
			     const Props& new_props)
{
  FstabLocker l;

  // mountpoint
  if (old_props.has("mountpoint")) {
    String mnt_curr = old_props.get("mountpoint").get_string();
    String mnt_new = new_props.get("mountpoint").get_string();
    if (mnt_curr != mnt_new) {
      MountHandler mh;
      if (mnt_curr.size())
	if (!mh.umount(path, mnt_curr))
	  throw UMountError(mnt_curr);
      if (mnt_new.size())
	if (!mh.mount(path, mnt_new, fsname))
	  throw String("mount failed");
    }
  }

  // fstab
  if (old_props.has("fstabpoint")) {
    String mnt_curr = old_props.get("fstabpoint").get_string();
    String mnt_new = new_props.get("fstabpoint").get_string();
    if (mnt_curr != mnt_new) {
      MountHandler mh;
      if (mnt_curr.size()) {
	list<Mountpoint> l = mh.fstabs(mh.maj_min(path));
	for (list<Mountpoint>::const_iterator iter = l.begin();
	     iter != l.end();
	     iter++)
	  if (iter->mountpoint == mnt_curr)
	    mh.fstab_remove(iter->devname, mnt_curr);
      }
      if (mnt_new.size())
	mh.fstab_add(path, mnt_new, fsname);
    }
  }
}

bool
ContentFS::mount_props_mounted(const Props& props)
{
  if (props.has("mountpoint"))
    return props.get("mountpoint").get_string().size();
  return false;
}









ContentFSTemplate::ContentFSTemplate(const String& name) :
  ContentTemplate(CONTENT_FS_TYPE),
  _name(name)
{
  attrs["fs_type"] = _name;
}

ContentFSTemplate::~ContentFSTemplate()
{}



void
ContentFSTemplate::mount_props_template(const String& fsname,
					Props& props)
{
  bool mountable = ContentFS::mount_fs_supported(fsname);
  props.set(Variable("mountable", mountable));

  if (mountable) {
    list<String> illegal_mps = MountHandler().used_dirs();
    props.set(Variable("mountpoint",
		       "",
		       0,
		       128,
		       ILLEGAL_MOUNT_CHARS,
		       illegal_mps));
    props.set(Variable("fstab", false, true));
    props.set(Variable("mount", false, true));
  }
}

void
ContentFSTemplate::mount_props_create(const String& path,
				      const String& fsname,
				      const Props& props)
{
  if (props.has("mountpoint")) {
    String mountpoint = props.get("mountpoint").get_string();
    String label;
    try {
      label = props.get("label").get_string();
    } catch ( ... ) {}
    bool mount = props.get("mount").get_bool();
    bool fstab = props.get("fstab").get_bool();

    if (mountpoint.size()) {
      FstabLocker l;
      if (mount)
	MountHandler().mount(path,
			     mountpoint,
			     fsname);
      if (fstab)
	MountHandler().fstab_add(label.empty() ? path : String("LABEL=") + label,
				 mountpoint,
				 fsname);
    }
  }
}





bool
ContentFS::mount_fs_supported(const String& fsname)
{
  list<String> l = MountHandler().fstypes();
  if (find(l.begin(), l.end(), fsname) != l.end())
    return true;

  String out, err;
  int status;
  vector<String> args;
  args.push_back(fsname);
  if (utils::execute("/sbin/modinfo", args, out, err, status))
    throw command_not_found_error_msg("modinfo");
  return !status;
}
