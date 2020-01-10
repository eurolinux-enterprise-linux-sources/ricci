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


#ifndef System_h
#define System_h

#include "Mapper.h"

#include <list>


std::list<String> get_SYS_ids();


class System : public Mapper
{
 public:
  System(const String& id);
  virtual ~System();

  virtual void apply(const MapperParsed&);

 protected:

  virtual void __add_sources(const std::list<counting_auto_ptr<BD> >& bds);
  virtual void __remove();

};


#endif  // System_h
