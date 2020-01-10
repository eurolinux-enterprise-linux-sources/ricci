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


#ifndef ContentNone_h
#define ContentNone_h

#include "Content.h"
#include "counting_auto_ptr.h"
#include "BD.h"
#include "String.h"
#include <list>



void create_content_none(const counting_auto_ptr<BD>& bd);


class ContentNone : public Content
{
 public:
  ContentNone(const String& path);
  virtual ~ContentNone();

  virtual XMLObject xml() const;


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
  virtual void remove();


 private:

};


class ContentNoneTemplate : public ContentTemplate
{
 public:
  ContentNoneTemplate();
  virtual ~ContentNoneTemplate();
};


#endif  // ContentNone_h
