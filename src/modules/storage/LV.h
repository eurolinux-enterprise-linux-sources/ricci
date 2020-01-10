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


#ifndef LV_h
#define LV_h

#include "BD.h"


counting_auto_ptr<BD> create_LV(const BDTemplate& bd_temp);


class LV : public BD
{
 public:
  LV(const String& path);
  virtual ~LV();

  virtual String state_ind() const;

  virtual void remove();

  virtual counting_auto_ptr<BD> apply(const BDParsed& bd);  // return new bd

 protected:

  virtual void shrink(unsigned long long new_size,
		      const Props& new_props);
  virtual void expand(unsigned long long new_size,
		      const Props& new_props);

  virtual String apply_props_before_resize(const Props& new_props); // return path
  virtual String apply_props_after_resize(const Props& new_props);  // return path

 private:

};  // class LV


class LVTemplate : public BDTemplate
{
 public:
  LVTemplate(const String& mapper_id,
	     const String& mapper_state_ind);
  virtual ~LVTemplate();


 private:

};  // class LVTemplate


#endif  // LV_h
