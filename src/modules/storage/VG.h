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


#ifndef VG_h
#define VG_h

#include "Mapper.h"
#include <list>
#include "String.h"


std::list<String> get_VG_ids();
counting_auto_ptr<Mapper> create_VG(const MapperTemplate& temp);


class VG : public Mapper
{
 public:
  VG(const String& id);
  virtual ~VG();

  virtual void apply(const MapperParsed&);

 protected:

  virtual void __add_sources(const std::list<counting_auto_ptr<BD> >& bds);
  virtual void __remove();

 private:

  String _vgname;


};  // VG


class VGTemplate : public MapperTemplate
{
 public:
  VGTemplate();
  virtual ~VGTemplate();

 private:


};


#endif  // VG_h
