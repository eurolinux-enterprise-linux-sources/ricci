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


#ifndef bd_h
#define bd_h

#include "String.h"

#include "defines.h"
#include "Content.h"
#include "Props.h"
#include "counting_auto_ptr.h"


class BD;


class BDParsed
{
 public:
  BDParsed(const XMLObject& xml);
  virtual ~BDParsed();

  counting_auto_ptr<ContentParsed> content;

  Props props;

  //  MapperType   mapper_type;
  //  String  mapper_id;
  //  BDType       bd_type;
  String  path;
  String  state_ind;


  counting_auto_ptr<BD> bd;


};  // class BDParsed


class BD
{
 public:
  virtual ~BD();

  String path() const;
  virtual String state_ind() const;
  virtual long long size() const;
  String mapper_id() const;
  String mapper_type() const;

  XMLObject xml() const;

  counting_auto_ptr<Content> content;

  Props _props;

  virtual bool removable() const;
  virtual void removable(bool removable);
  virtual void remove() = 0;

  virtual counting_auto_ptr<BD> apply(const BDParsed& bd);  // return new bd




 protected:
  BD(const String& mapper_type,
     const String& mapper_id,
     const String& bd_type,
     const String& path,
     bool check_path = true);

  String  _mapper_type;
  String  _mapper_id;
  String  _bd_type;
  String  _path;


  virtual void shrink(unsigned long long new_size,
		      const Props& new_props) = 0;
  virtual void expand(unsigned long long new_size,
		      const Props& new_props) = 0;

  virtual String apply_props_before_resize(const Props& new_props) = 0; // return path
  virtual String apply_props_after_resize(const Props& new_props) = 0;  // return path



 private:

};  // class BD


class BDTemplate
{
 public:
  BDTemplate(const XMLObject& xml);
  virtual ~BDTemplate();

  XMLObject xml() const;



  String  mapper_type;
  String  mapper_id;
  String  mapper_state_ind;
  String  bd_type;

  Props props;

  counting_auto_ptr<Content> content; // valid only if not constructed from XML

  counting_auto_ptr<ContentParsed> content_parsed; // valid only if constructed from XML

 protected:
  BDTemplate(const String& mapper_type,
	     const String& mapper_id,
	     const String& mapper_state_ind,
	     const String& bd_type);


 private:

};  // class BDTemplate


#endif  // bd_h
