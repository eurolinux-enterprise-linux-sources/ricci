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


#ifndef PartitionTable_h
#define PartitionTable_h

#include "Mapper.h"


std::list<String> get_PT_ids();
counting_auto_ptr<Mapper> create_PT(const MapperTemplate& temp);


class PartitionTable : public Mapper
{
 public:
  PartitionTable(const String& id);
  virtual ~PartitionTable();

  virtual void apply(const MapperParsed&);

 protected:

  virtual void __add_sources(const std::list<counting_auto_ptr<BD> >& bds);
  virtual void __remove();

 private:
  String _pt_path;
  String _label;


};  // PartitionTable



class PTTemplate : public MapperTemplate
{
 public:
  PTTemplate();
  virtual ~PTTemplate();

 private:


};


#endif  // PartitionTable_h
