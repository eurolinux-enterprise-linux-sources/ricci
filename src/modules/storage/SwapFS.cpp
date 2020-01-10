/*
  Copyright Red Hat, Inc. 2005-2008

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


#include "SwapFS.h"
#include "utils.h"
#include "MountHandler.h"
#include "UMountError.h"
#include "FileMagic.h"


using namespace std;


const String SwapFS::PRETTY_NAME("swap");


SwapFS::SwapFS(const String& path) :
  ContentFS(path, PRETTY_NAME, "swap")
{
  String magic(FileMagic::get_description(path));
  if (magic.find("swap") == magic.npos)
    throw String(path + " not swap");

  // swapon
  MountHandler mh;
  FstabLocker l;
  if (mh.mounts(mh.maj_min(path)).empty())
    _props.set(Variable("swapon", false, true));
  else
    _props.set(Variable("swapon", true, true));
  // fstab
  if (mh.fstabs(mh.maj_min(path)).empty())
    _props.set(Variable("fstab", false, true));
  else
    _props.set(Variable("fstab", true, true));
}

SwapFS::~SwapFS()
{}


bool
SwapFS::expandable(long long& max_size) const
{
  return false;
}

bool
SwapFS::shrinkable(long long& min_size) const
{
  return false;
}

void
SwapFS::shrink(const String& path,
	       unsigned long long new_size,
	       const Props& new_props)
{}

void
SwapFS::expand(const String& path,
	       unsigned long long new_size,
	       const Props& new_props)
{}

void
SwapFS::apply_props_before_resize(const String& path,
				  unsigned long long old_size,
				  unsigned long long new_size,
				  const Props& new_props)
{
  bool fstab_old = _props.get("fstab").get_bool();
  bool fstab_new = new_props.get("fstab").get_bool();
  bool swapon_old = _props.get("swapon").get_bool();
  bool swapon_new = new_props.get("swapon").get_bool();

  MountHandler mh;
  FstabLocker lock;

  if (fstab_old && !fstab_new) {
    list<Mountpoint> l = mh.fstabs(mh.maj_min(path));
    for (list<Mountpoint>::const_iterator iter = l.begin();
	 iter != l.end();
	 iter++)
      if (iter->mountpoint == "swap")
	mh.fstab_remove(iter->devname, iter->mountpoint);
  } else if (!fstab_old && fstab_new)
    mh.fstab_add(path, "swap", "swap");

  if (swapon_old && !swapon_new) {
    list<Mountpoint> l = mh.mounts(mh.maj_min(path));
    for (list<Mountpoint>::const_iterator iter = l.begin();
	 iter != l.end();
	 iter++)
      if (iter->mountpoint == "swap")
	if (!mh.umount(iter->devname, iter->mountpoint))
	  throw UMountError(iter->devname);
  } else if (!swapon_old && swapon_new)
    mh.mount(path, "swap", "swap");
}

void
SwapFS::apply_props_after_resize(const String& path,
				 unsigned long long old_size,
				 unsigned long long new_size,
				 const Props& new_props)
{}

bool
SwapFS::removable() const
{
  return true;
}



void
create_swap_fs(const String& path,
	       const counting_auto_ptr<ContentTemplate>& templ)
{
  String label = templ->_props.get("label").get_string();
  bool swapon = templ->_props.get("swapon").get_bool();
  bool fstab = templ->_props.get("fstab").get_bool();

  vector<String> args;
  if (label.size()) {
    args.push_back("-L");
    args.push_back(label);
  }
  args.push_back(path);

  String out, err;
  int status;
  if (utils::execute("/sbin/mkswap", args, out, err, status))
    throw command_not_found_error_msg("mkswap");
  if (status != 0)
    throw String("mkswap failed: ") + err;

  if (swapon) {
    args.clear();
    args.push_back(path);
    if (utils::execute("/sbin/swapon", args, out, err, status))
      throw command_not_found_error_msg("swapon");
    if (status != 0)
      throw String("swapon failed: ") + err;
  }

  if (fstab) {
    /*
    MountHandler().fstab_add(label.empty() ? path : String("LABEL=") + label,
			     "swap",
			     "swap");
    */
    MountHandler().fstab_add(path, "swap", "swap");
  }
}


SwapFSTemplate::SwapFSTemplate() :
  ContentFSTemplate(SwapFS::PRETTY_NAME)
{
  // label
  _props.set(Variable("label",
		      "",
		      0,
		      16,
		      ILLEGAL_LABEL_CHARS,
		      list<String>()));

  // swapon
  _props.set(Variable("swapon", true, true));

  // fstab
  _props.set(Variable("fstab", true, true));
}

SwapFSTemplate::~SwapFSTemplate()
{}
