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


#ifndef ContentFactory_h
#define ContentFactory_h

#include "Content.h"
#include "BD.h"
#include "counting_auto_ptr.h"

#include "String.h"


class ContentFactory
{
 public:

  counting_auto_ptr<Content> probe(const String& path);

  counting_auto_ptr<Content>
    create_content(const counting_auto_ptr<BD>& bd,
		   const counting_auto_ptr<ContentParsed>& content);

};


#endif  // ContentFactory_h
