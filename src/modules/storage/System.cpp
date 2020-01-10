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


#include "System.h"
#include "defines.h"
#include "utils.h"
#include "HD.h"


#include <iostream>


using namespace std;


list<String>
get_SYS_ids()
{
  list<String> ids;
  ids.push_back(SYS_PREFIX);
  return ids;
}



System::System(const String& id) :
  Mapper(MAPPER_SYS_TYPE, id)
{
  if (_mapper_id != SYS_PREFIX)
    throw String("invalid mapper_id: " + _mapper_id);

  // parse blockdev
  vector<String> args;
  args.push_back("--report");

  String out, err;
  int status;
  if (utils::execute("/sbin/blockdev", args, out, err, status))
    throw command_not_found_error_msg("blockdev");
  if (status)
    throw String("blockdev failed: ") + err;

  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    vector<String> words = utils::split(utils::strip(*iter));
    if (words.size() != 7 || words[0] == "RO")
      continue;
    String path = words[6];
    if ( ! isdigit(path[path.size()-1]))
      targets.push_back(counting_auto_ptr<BD>(new HD(path)));
  }
}

System::~System()
{}


void
System::apply(const MapperParsed&)
{
  // nothing to do, for now
}

void
System::__add_sources(const list<counting_auto_ptr<BD> >& bds)
{
  throw String("System can have no sources");
}

void
System::__remove()
{
  throw String("System mapper is not removable");
}
