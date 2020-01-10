/*
  Copyright Red Hat, Inc. 2006

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


#ifndef GFS1_h
#define GFS1_h

#include "ContentFS.h"
#include "String.h"


void create_GFS1(const String& path,
		 const counting_auto_ptr<ContentTemplate>& templ);


class GFS1 : public ContentFS
{
 public:
  GFS1(const String& path);
  virtual ~GFS1();

  const static String PRETTY_NAME;

  virtual bool expandable(long long& max_size) const;
  virtual bool shrinkable(long long& min_size) const;
  virtual void shrink(const String& path,
		      unsigned long long new_size,
		      const Props& new_props);
  virtual void expand(const String& path,
		      unsigned long long new_size,
		      const Props& new_props);

  virtual void apply_props_before_resize(const String& path,
					 unsigned long long old_size,
					 unsigned long long new_size,
					 const Props& new_props);
  virtual void apply_props_after_resize(const String& path,
					unsigned long long old_size,
					unsigned long long new_size,
					const Props& new_props);

  virtual bool removable() const;

 private:

};


class GFS1Template : public ContentFSTemplate
{
 public:
  GFS1Template();
  virtual ~GFS1Template();

 private:


};


#endif // GFS1_h
