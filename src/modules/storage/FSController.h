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


#ifndef FSController_h
#define FSController_h

#include "counting_auto_ptr.h"
#include "Content.h"
#include "BD.h"
#include <list>


class FSController
{
 public:
  std::list<String> get_fs_group_ids(const String& name);

  counting_auto_ptr<Content> get_fs(const String& path);

  std::list<counting_auto_ptr<ContentTemplate> > get_available_fss();

  counting_auto_ptr<Content> create_fs(const counting_auto_ptr<BD>& bd,
				       const counting_auto_ptr<ContentTemplate>& cont_templ);

 private:





};


#endif  // FSController_h
