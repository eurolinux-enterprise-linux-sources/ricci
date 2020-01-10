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


#ifndef parted_wrapper_h
#define parted_wrapper_h

#include "String.h"
#include <list>


enum PPType {PPTprimary  = 1,
	     PPTextended = 2,
	     PPTlogical  = 4,
	     PPTunused   = 8};


class PartedPartition
{
 public:
  PartedPartition(int partnum,
		  long long beg,
		  long long end,
		  bool bootable,
		  PPType type,
		  const String& label);
  // unused
  PartedPartition(long long beg,
		  long long end,
		  PPType available_types,
		  const String& label);
  virtual ~PartedPartition();

  long long begin() const;
  long long size() const;
  int partnum() const;
  bool bootable() const;

  String type() const;
  String label() const;

  bool unused_space() const;

  // if unused(), possible part types
  bool primary() const;
  bool extended() const;
  bool logical() const;

  void add_kid(const PartedPartition& part);
  std::list<PartedPartition>& kids();
  const std::list<PartedPartition>& kids() const;


  bool operator<(const PartedPartition&) const;


  void print(const String& indent) const;


 private:
  int         _partnum;
  long long   _beg;
  long long   _end;
  bool        _bootable;
  PPType      _type;
  String _label;

  std::list<PartedPartition> _kids;

};  // class PartedPartition



class Parted
{
 public:

  static String extract_pt_path(const String& path);
  static String generate_part_path(const String& pt_path,
					const PartedPartition& part);

  static std::list<String> possible_paths();

  static std::list<String> supported_labels();

  static long long min_part_size(const String& label);


  static std::pair<String, std::list<PartedPartition> >
    partitions(const String& pt_path);

  static void create_label(const String& path, const String& label);
  static void remove_label(const String& path);

  // return path of new partition
  static String create_partition(const String& pt_path,
				      const String& part_type,
				      long long seg_begin,
				      long long size);

  static void remove_partition(const String& pt_path,
			       const PartedPartition& partition);


};  // class Parted


#endif  // parted_wrapper_h
