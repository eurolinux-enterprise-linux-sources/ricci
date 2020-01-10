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


#include "Partition.h"
#include "BDFactory.h"
#include "FSController.h"
#include "utils.h"
#include "ContentFactory.h"
#include "ContentNone.h"
#include "ContentExtendedPartition.h"
#include "PV.h"

#include <algorithm>

using namespace std;





Partition::Partition(const String& path) :
  BD(MAPPER_PT_TYPE,
     PT_PREFIX + Parted::extract_pt_path(path),
     BD_PART_TYPE,
     path,
     false),
  _pt_path(Parted::extract_pt_path(path))
{
  pair<String, list<PartedPartition> > p = Parted::partitions(_pt_path);
  list<PartedPartition> parts = p.second;
  for (list<PartedPartition>::iterator iter = parts.begin();
       iter != parts.end();
       iter++) {
    if (iter->unused_space())
      continue;
    if (_path == Parted::generate_part_path(_pt_path, *iter)) {
      initialize(*iter);
      return;
    } else if (iter->extended()) {
      // traverse kids
      for (list<PartedPartition>::const_iterator log_iter = iter->kids().begin();
	   log_iter != iter->kids().end();
	   log_iter++) {
	if (log_iter->unused_space())
	  continue;
	if (_path == Parted::generate_part_path(_pt_path, *log_iter)) {
	  initialize(*log_iter);
	  return;
	}
      }
    }
  }
  throw String(_path + " not a partition");
}

Partition::Partition(const String& path,
		     const PartedPartition& parted_part) :
  BD(MAPPER_PT_TYPE,
     PT_PREFIX + Parted::extract_pt_path(path),
     BD_PART_TYPE,
     path,
     false),
  _pt_path(Parted::extract_pt_path(path))
{
  initialize(parted_part);
}

void
Partition::initialize(const PartedPartition& part)
{
  partition = counting_auto_ptr<PartedPartition>(new PartedPartition(part));

  _props.set(Variable("size", part.size()));
  _props.set(Variable("partition_num", (long long) part.partnum()));
  _props.set(Variable("partition_type", part.type()));
  _props.set(Variable("bootable", part.bootable()));
  _props.set(Variable("partition_begin", part.begin()));

  if (content->removable()) {
    removable(true);
    if (content->type == CONTENT_NONE_TYPE) {
      list<counting_auto_ptr<ContentTemplate> > fss = FSController().get_available_fss();
      for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter = fss.begin();
	   iter != fss.end();
	   iter++) {
	content->add_replacement(*iter);
      }

      // add PV
      content->add_replacement(counting_auto_ptr<ContentTemplate>(new PVTemplate()));
    } else
      content->add_replacement(counting_auto_ptr<ContentTemplate>(new ContentNoneTemplate()));
  }

  if (partition->extended()) {
    // replace content with extended content
    content = counting_auto_ptr<Content>(new ContentExtendedPartition(_path));

    // TODO: fix removable (extended s/b removable if all it's parts are removable)
    bool rem = true;
    for (list<PartedPartition>::const_iterator iter = partition->kids().begin();
	 iter != partition->kids().end();
	 iter++)
      if ( ! iter->unused_space())
	rem = false;
    removable(rem);
  }

  if (partition->logical()) {
    // TODO: logical removal changes paths, fix it out
    // for now, only last logical can be removed
    int max_logical = part.partnum();
    pair<String, list<PartedPartition> > p = Parted::partitions(_pt_path);
    const list<PartedPartition> parts = p.second;
    for (list<PartedPartition>::const_iterator iter = parts.begin();
	 iter != parts.end();
	 iter++) {
      if (iter->extended()) {
	for (list<PartedPartition>::const_iterator log_iter = iter->kids().begin();
	     log_iter != iter->kids().end();
	     log_iter++)
	  if ( ! log_iter->unused_space())
	    if (log_iter->partnum() > max_logical)
	      max_logical = log_iter->partnum();
      }
    }
    if (max_logical != part.partnum())
      removable(false);
  }

  // if label not supported, parts not removable
  list<String> supp_labels = Parted::supported_labels();
  bool label_supported = (find(supp_labels.begin(),
			       supp_labels.end(),
			       part.label()) != supp_labels.end());
  if ( ! label_supported)
    removable(false);

}

Partition::~Partition()
{}


void
Partition::shrink(unsigned long long new_size,
	   const Props& new_props)
{
  throw String("Partition::shrink() not implemented");
}
void
Partition::expand(unsigned long long new_size,
	   const Props& new_props)
{
  throw String("Partition::expand() not implemented");
}
String
Partition::apply_props_before_resize(const Props& new_props)
{
  return path();
}
String
Partition::apply_props_after_resize(const Props& new_props)
{
  return path();
}

void
Partition::remove()
{
  if (!partition.get())
    throw String("Partition::remove() null pointer");

  content->remove();
  Parted::remove_partition(_pt_path,
			   *(partition.get()));
}



counting_auto_ptr<BD>
create_partition(const BDTemplate& bd_temp)
{
  // everything is already validated :)

  String pt_path = bd_temp.mapper_id.substr(PT_PREFIX.size());

  String part_type = bd_temp.props.get("partition_type").get_string();

  long long seg_begin = bd_temp.props.get("partition_begin").get_int();
  long long size = bd_temp.props.get("size").get_int();

  String part_path = Parted::create_partition(pt_path,
					      part_type,
					      seg_begin,
					      size);

  utils::clear_cache();

  counting_auto_ptr<BD> bd = BDFactory::get_bd(part_path);
  ContentFactory().create_content(bd, bd_temp.content_parsed);

  return BDFactory::get_bd(part_path);
}


PartitionTemplate::PartitionTemplate(const String& mapper_id,
				     const String& mapper_state_ind,
				     const PartedPartition& part) :
  BDTemplate(MAPPER_PT_TYPE,
	     mapper_id,
	     mapper_state_ind,
	     BD_PART_TYPE)
{
  if (!part.unused_space())
    throw String("not unused space");

  props.set(Variable("partition_begin", part.begin()));

  String type;
  if (part.primary())
    type = "primary";
  else if (part.extended())
    type = "extended";
  else if (part.logical())
    type = "logical";
  else
    throw String("uncreatable");
  props.set(Variable("partition_type", type));

  long long min_part_size = Parted::min_part_size(part.label());
  long long part_size = (part.size() / min_part_size) * min_part_size;
  Variable size("size",
		part_size,
		min_part_size,
		part_size,
		min_part_size);
  props.set(size);

  if (part.extended()) {


    // TODO: extended partition's content


  } else {
    if (content->type == CONTENT_NONE_TYPE) {
      list<counting_auto_ptr<ContentTemplate> > fss = FSController().get_available_fss();
      for (list<counting_auto_ptr<ContentTemplate> >::const_iterator iter = fss.begin();
	   iter != fss.end();
	   iter++) {
	content->add_replacement(*iter);
      }

      // add PV
      content->add_replacement(counting_auto_ptr<ContentTemplate>(new PVTemplate()));
    }
  }
}

PartitionTemplate::~PartitionTemplate()
{}
