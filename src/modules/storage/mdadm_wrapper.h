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


#ifndef mdadm_wrapper_h
#define mdadm_wrapper_h

#include "String.h"
#include <list>
#include <vector>


enum mdraid_source_type { MDRAID_S_ACTIVE,
			  MDRAID_S_SPARE,
			  MDRAID_S_FAILED};


class mdraid_source
{
 public:
  mdraid_source(const String& path,
		mdraid_source_type type);

  String path;
  mdraid_source_type type;

};



class mdraid
{
 public:
  mdraid();

  String path;
  String name;

  String level;
  String uuid;
  long long num_devices;

  std::list<mdraid_source> devices;

};



class mdadm
{
 public:

  static std::list<String> valid_raid_levels();

  static std::list<mdraid> raids();
  static mdraid probe_path(const String& path);

  static void add_source(const String& raid_path,
			 const String& path);
  static void fail_source(const String& raid_path,
			  const String& path);
  static void remove_source(const String& raid_path,
			    const String& path);
  static void zero_superblock(const String& path);


  static void start_raid(const mdraid& raid);
  static void stop_raid(const mdraid& raid);

  static String create_raid(const String& level,
				 const std::list<String>& dev_paths);


};  // class mdadm


#endif  // mdadm_wrapper_h
