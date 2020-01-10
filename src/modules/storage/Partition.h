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


#ifndef Partition_h
#define Partition_h

#include "BD.h"
#include "parted_wrapper.h"


counting_auto_ptr<BD> create_partition(const BDTemplate& bd_temp);


class Partition : public BD
{
 public:
  Partition(const String& path);
  Partition(const String& path, const PartedPartition& parted_part);
  virtual ~Partition();

  virtual void remove();

 protected:
  virtual void shrink(unsigned long long new_size,
		      const Props& new_props);
  virtual void expand(unsigned long long new_size,
		      const Props& new_props);

  virtual String apply_props_before_resize(const Props& new_props); // return path
  virtual String apply_props_after_resize(const Props& new_props);  // return path

 private:
  void initialize(const PartedPartition& part);

  String _pt_path;

  counting_auto_ptr<PartedPartition> partition;

};  // class Partition



class PartitionTemplate : public BDTemplate
{
 public:
  PartitionTemplate(const String& mapper_id,
		    const String& mapper_state_ind,
		    const PartedPartition& part);
  virtual ~PartitionTemplate();


};  // class PartitionTemplate


#endif  // Partition_h
