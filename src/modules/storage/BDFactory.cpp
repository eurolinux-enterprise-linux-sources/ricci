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


#include "BDFactory.h"
#include "MapperFactory.h"
#include "LV.h"
#include "Partition.h"
#include "HD.h"
#include "MDRaidTarget.h"
#include "MidAir.h"
#include "utils.h"


#include <iostream>
using namespace std;


counting_auto_ptr<BD>
BDFactory::get_bd(const String& path)
{
  try {
    return counting_auto_ptr<BD>(new LV(path));
  } catch ( ... ) {}

  try {
    return counting_auto_ptr<BD>(new Partition(path));
  } catch ( ... ) {}

  try {
    return counting_auto_ptr<BD>(new MDRaidTarget(path));
  } catch ( ... ) {}

  return counting_auto_ptr<BD>(new HD(path));
}


counting_auto_ptr<BD>
BDFactory::create_bd(const BDTemplate& bd_temp)
{
  if (bd_temp.bd_type == BD_LV_TYPE)
    return create_LV(bd_temp);
  if (bd_temp.bd_type == BD_PART_TYPE)
    return create_partition(bd_temp);
  throw String("no such mapper type");
}


counting_auto_ptr<BD>
BDFactory::modify_bd(const BDParsed& parsed_bd)
{
  counting_auto_ptr<BD> old_bd = get_bd(parsed_bd.path);
  if (old_bd->state_ind() != parsed_bd.state_ind)
    throw MidAir();
  return old_bd->apply(parsed_bd);
}

counting_auto_ptr<Mapper>
BDFactory::remove_bd(const BDParsed& parsed_bd)
{
  counting_auto_ptr<BD> old_bd = get_bd(parsed_bd.path);
  if (old_bd->state_ind() != parsed_bd.state_ind)
    throw MidAir();
  if (!old_bd->removable())
    throw String("invalid call: bd not removable");

  old_bd->remove();
  return MapperFactory::get_mapper(old_bd->mapper_type(),
				   old_bd->mapper_id());
}
