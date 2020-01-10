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


#include "ContentFactory.h"
#include "counting_auto_ptr.h"
#include "PV.h"
#include "PTSource.h"
#include "FSController.h"
#include "ContentNone.h"
#include "utils.h"
#include "MDRaidSource.h"


using namespace std;


counting_auto_ptr<Content>
ContentFactory::probe(const String& path)
{
  try {
    return counting_auto_ptr<Content>(new PV(path));
  } catch ( ... ) {}

  try {
    return counting_auto_ptr<Content>(new PTSource(path));
  } catch ( ... ) {}

  try {
    return counting_auto_ptr<Content>(new MDRaidSource(path));
  } catch ( ... ) {}

  try {
    return FSController().get_fs(path);
  } catch ( ... ) {}

  return counting_auto_ptr<Content>(new ContentNone(path));
}

counting_auto_ptr<Content>
ContentFactory::create_content(const counting_auto_ptr<BD>& bd,
			       const counting_auto_ptr<ContentParsed>& content)
{
  if (content->_replacement.get()) {
    if (content->_replacement->type == CONTENT_FS_TYPE)
      return FSController().create_fs(bd, content->_replacement);
    else if (content->_replacement->type == CONTENT_NONE_TYPE) {
      create_content_none(bd);
      return probe(bd->path());
    } else if (content->_replacement->type == CONTENT_MS_TYPE) {
      XMLObject x = content->_replacement->xml();
      if (x.get_attr("mapper_type") == MAPPER_VG_TYPE) {
	create_content_pv(bd->path(), content->_replacement);
	return probe(bd->path());
      }
    }
    throw String("creation of content ") + content->_replacement->type +
      " not implemented";
  }

  return bd->content;
}
