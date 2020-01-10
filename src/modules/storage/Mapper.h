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


#ifndef Mapper_h
#define Mapper_h

#include "defines.h"
#include "XML.h"
#include "BD.h"
#include "Props.h"
#include "counting_auto_ptr.h"

#include "String.h"
#include <list>


class MapperParsed;


class Mapper
{
 public:
  virtual ~Mapper();

  String state_ind() const;

  Props _props;

  virtual XMLObject xml() const;



  std::list<counting_auto_ptr<BD> > sources;
  std::list<counting_auto_ptr<BD> > targets;

  std::list<counting_auto_ptr<BD> > new_sources;
  std::list<counting_auto_ptr<BDTemplate> > new_targets;

  virtual bool removable() const;
  virtual void removable(bool removable);
  virtual void remove();

  virtual void apply(const MapperParsed&) = 0;

  virtual void add_sources(const std::list<BDParsed>& bds);

 protected:
  Mapper(const String& type, const String& id);

  String _mapper_type;
  String _mapper_id;

  virtual void __add_sources(const std::list<counting_auto_ptr<BD> >& bds) = 0;
  virtual void __remove() = 0;


 private:


};  // class Mapper


class MapperParsed
{
 public:
  MapperParsed(const XMLObject& xml);
  virtual ~MapperParsed();

  Props props;

  String mapper_type;
  String mapper_id;
  String state_ind;

  counting_auto_ptr<Mapper> mapper;

};


class MapperTemplate
{
 public:
  MapperTemplate(const XMLObject& xml);
  virtual ~MapperTemplate();

  virtual XMLObject xml() const;


  std::list<counting_auto_ptr<BD> > sources;
  std::list<counting_auto_ptr<BD> > targets;

  std::list<counting_auto_ptr<BD> > new_sources;
  std::list<counting_auto_ptr<BDTemplate> > new_targets;


  String mapper_type;

  Props props;

 protected:
  MapperTemplate(const String& type);

 private:


};  // class MapperTemplate


#endif  // Mapper_h
