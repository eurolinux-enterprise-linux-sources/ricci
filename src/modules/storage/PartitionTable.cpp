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


#include "PartitionTable.h"
#include "Partition.h"
#include "parted_wrapper.h"
#include "BDFactory.h"
#include "MapperFactory.h"
#include "utils.h"

#include <algorithm>
#include <iostream>


using namespace std;


list<String> get_PT_ids()
{
  list<String> hds = Parted::possible_paths();

  // check if a partition table is on the path
  list<String> ids;
  for (list<String>::const_iterator iter = hds.begin();
       iter != hds.end();
       iter++) {
    try {
      Parted::partitions(*iter);
      ids.push_back(PT_PREFIX + *iter);
    } catch( ... ) {}
  }
  return ids;
}

counting_auto_ptr<Mapper>
create_PT(const MapperTemplate& temp)
{
  // everything is already validated, but number of sources
  if (temp.sources.size() != 1)
    throw String("create_PT requires exactly one source");

  utils::clear_cache();

  String path = temp.sources.front()->path();
  Parted::create_label(path, temp.props.get("label").get_string());

  utils::clear_cache();

  return counting_auto_ptr<Mapper>(new PartitionTable(PT_PREFIX + path));
}



//  ##### PartitionTable #####

static void
generate_targets(const String& pt_path,
		 const list<PartedPartition>& parts,
		 list<counting_auto_ptr<BD> >& targets);
static void
generate_new_targets(const String& mapper_id,
		     const String& mapper_state_ind,
		     const list<PartedPartition>& parts,
		     list<counting_auto_ptr<BDTemplate> >& new_targets);


PartitionTable::PartitionTable(const String& id) :
  Mapper(MAPPER_PT_TYPE, id)
{
  _pt_path = _mapper_id;
  _pt_path.replace(0, PT_PREFIX.size(), "");

  list<String> hds = Parted::possible_paths();
  if (find(hds.begin(), hds.end(), _pt_path) == hds.end())
    throw String(_pt_path + "not a partition table");

  pair<String, list<PartedPartition> > p = Parted::partitions(_pt_path);
  const list<PartedPartition> parts = p.second;
  _label = p.first;
  _props.set(Variable("disklabel", _label));

  // label supported
  list<String> supp_labels = Parted::supported_labels();
  bool label_supported = (find(supp_labels.begin(),
			       supp_labels.end(),
			       _label) != supp_labels.end());

  // sources
  counting_auto_ptr<BD> source = BDFactory::get_bd(_pt_path);
  _props.set(Variable("size", source->_props.get("size").get_int()));
  sources.push_back(source);

  // targets
  generate_targets(_pt_path, parts, targets);

  bool rem = true;
  for (list<counting_auto_ptr<BD> >::const_iterator iter = targets.begin();
       iter != targets.end();
       iter++)
    if (!(*iter)->removable())
      rem = false;
  removable(rem);

  // new targets
  if (label_supported)
    generate_new_targets(id,
			 state_ind(),
			 parts,
			 new_targets);
}

PartitionTable::~PartitionTable()
{}


void
PartitionTable::apply(const MapperParsed&)
{
  // nothing to do, for now
}

void
PartitionTable::__add_sources(const list<counting_auto_ptr<BD> >& bds)
{
  throw String("PartitionTable can have only one source");
}

void
PartitionTable::__remove()
{
  Parted::remove_label(_pt_path);
}



static void
generate_targets(const String& pt_path,
		 const list<PartedPartition>& parts,
		 list<counting_auto_ptr<BD> >& targets)
{
  for (list<PartedPartition>::const_iterator iter = parts.begin();
       iter != parts.end();
       iter++) {
    if (!iter->unused_space()) {
      String part_path = Parted::generate_part_path(pt_path, *iter);
      targets.push_back(counting_auto_ptr<BD>(new Partition(part_path, *iter)));
      if (iter->extended())
	generate_targets(pt_path, iter->kids(), targets);
    }
  }
}

static void
generate_new_targets(const String& mapper_id,
		     const String& mapper_state_ind,
		     const list<PartedPartition>& parts,
		     list<counting_auto_ptr<BDTemplate> >& new_targets)
{
  for (list<PartedPartition>::const_iterator iter = parts.begin();
       iter != parts.end();
       iter++) {
    if (iter->unused_space()) {
      try {
	if (iter->primary()) {
	  PartedPartition p(0,
			    iter->begin(),
			    iter->begin() + iter->size(),
			    iter->bootable(),
			    (PPType) (PPTprimary | PPTunused),
			    iter->label());
	  counting_auto_ptr<BDTemplate> ptt(new PartitionTemplate(mapper_id,
								  mapper_state_ind,
								  p));
	  new_targets.push_back(ptt);
	}
	if (iter->extended()) {
	  PartedPartition p(0,
			    iter->begin(),
			    iter->begin() + iter->size(),
			    iter->bootable(),
			    (PPType) (PPTextended | PPTunused),
			    iter->label());
	  counting_auto_ptr<BDTemplate> ptt(new PartitionTemplate(mapper_id,
								  mapper_state_ind,
								  p));
	  new_targets.push_back(ptt);
	}
	if (iter->logical()) {
	  PartedPartition p(0,
			    iter->begin(),
			    iter->begin() + iter->size(),
			    iter->bootable(),
			    (PPType) (PPTlogical | PPTunused),
			    iter->label());
	  counting_auto_ptr<BDTemplate> ptt(new PartitionTemplate(mapper_id,
								  mapper_state_ind,
								  p));
	  new_targets.push_back(ptt);
	}
      } catch ( String e ) {
	cout << e << endl;
      } catch ( ... ) {}
    } else if (iter->extended())
      generate_new_targets(mapper_id,
			   mapper_state_ind,
			   iter->kids(),
			   new_targets);
  }
}



//  ##### PTTemplate #####

PTTemplate::PTTemplate() :
  MapperTemplate(MAPPER_PT_TYPE)
{
  Variable labels("label", String("gpt"), Parted::supported_labels());
  props.set(labels);

  // new sources
  counting_auto_ptr<Mapper> system =
    MapperFactory::get_mapper(MAPPER_SYS_TYPE,
			      SYS_PREFIX);
  for (list<counting_auto_ptr<BD> >::iterator iter = system->targets.begin();
       iter != system->targets.end();
       iter++) {
    counting_auto_ptr<BD> bd = *iter;
    if (bd->content->type == CONTENT_NONE_TYPE)
      new_sources.push_back(bd);
  }

  if (new_sources.empty())
    throw String("no available new sources");
  props.set(Variable("min_sources", (long long) 1));
  props.set(Variable("max_sources", (long long) 1));
}

PTTemplate::~PTTemplate()
{}
