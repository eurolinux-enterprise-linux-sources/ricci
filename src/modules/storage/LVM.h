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


#ifndef LVM_h
#define LVM_h

#include "Props.h"
#include "counting_auto_ptr.h"
#include "BD.h"

#include "String.h"
#include <list>


class LVM
{
 public:

  static String vgname_from_lvpath(const String& lvpath);
  static String vgname_from_pvpath(const String& pvpath);
  static void probe_lv(const String& lvpath, Props& props);
  static void probe_pv(const String& pvpath, Props& props);

  static void probe_vg(const String& vgname,
		       Props& props,
		       std::list<counting_auto_ptr<BD> >& sources,
		       std::list<counting_auto_ptr<BD> >& targets);


  static void pvcreate(const String& path);
  static void pvremove(const String& path);

  static void vgcreate(const String& vgname,
		       long long extent_size,
		       bool clustered,
		       const std::list<String>& pv_paths);
  static void vgremove(const String& vgname);
  static void vgextend(const String& vgname,
		       const std::list<String>& pv_paths);
  static void vgreduce(const String& vgname,
		       const String& pv_path);
  static void vgchange(const String& vgname,
		       bool clustered);

  static void lvcreate(const String& vgname,
		       const String& lvname,
		       long long size);
  static void lvcreate_snap(const String& lvname,
			    const String& origin_path,
			    long long size);
  static void lvremove(const String& path);

  static void lvreduce(const String& path, long long new_size);
  static void lvextend(const String& path, long long new_size);

  static bool vg_clustered(const String& vgname);
  static void check_locking();
  static bool clustered_enabled();
  static void enable_clustered();
  static void disable_clustered();

};


#endif  // LVM_h
