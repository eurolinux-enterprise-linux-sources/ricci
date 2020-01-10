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


#ifndef PV_h
#define PV_h

#include "MapperSource.h"



void create_content_pv(const String& path,
		       const counting_auto_ptr<ContentTemplate>& templ);


class PV : public MapperSource
{
 public:
  PV(const String& path);
  virtual ~PV();



  virtual bool removable() const;
  virtual void remove();

  virtual void apply_props_before_resize(const String& path,
					 unsigned long long old_size,
					 unsigned long long new_size,
					 const Props& new_props);

 private:

  bool unused() const;

};


class PVTemplate : public MapperSourceTemplate
{
 public:
  PVTemplate();
  virtual ~PVTemplate();

 private:


};


#endif  // PV_h
