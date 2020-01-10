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


#include "FileMagic.h"
#include <magic.h>



static String
__get_description(const String& filename,
		  const String& magic_filename = "")
{
  const char* buff = 0;
  magic_t cookie = 0;

  try {
    cookie = magic_open(MAGIC_SYMLINK|MAGIC_DEVICES|MAGIC_ERROR);
    if (!cookie)
      throw String("initialization of libmagic failure");

    if (magic_filename.empty()) {
      if (magic_load(cookie, NULL))
	throw String("initialization of libmagic failure: invalid default magic file");
    } else
      if (magic_load(cookie, magic_filename.c_str()))
	throw String("initialization of libmagic failure: invalid magic file: ") + magic_filename;

    String descr;
    buff = magic_file(cookie, filename.c_str());
    if (buff)
      descr = buff;

    if (descr.empty())
      throw String("nothing detected");
    if (descr == "data")
      throw String("unknown data detected");

    magic_close(cookie); cookie = 0;
    //    free((char*) buff); buff = 0;

    return descr;
  } catch ( ... ) {
    if (cookie)
      magic_close(cookie);
    //    if (buff)
    //      free((char*) buff);
    throw;
  }
}

String
FileMagic::get_description(const String& filename)
{
  return __get_description(filename);
}
