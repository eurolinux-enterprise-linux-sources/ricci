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


#ifndef ContentFS_h
#define ContentFS_h

#include "Content.h"
#include "String.h"


#define ILLEGAL_LABEL_CHARS    String(" <>?'\":;|}]{[)(*&^%$#@!~`+=")


class ContentFS : public Content
{
 public:
  virtual ~ContentFS();

  virtual XMLObject xml() const;

  virtual bool removable() const;
  virtual void remove();



  static bool mount_fs_supported(const String& fsname); // true if mountable
  // these fcns extract/place info from/into props
  static bool mount_props_mounted(const Props& props); // true if mounted
  static void mount_props_probe(const String& path,
				const String& module,
				Props& props);
  static void mount_props_apply(const String& path,
				const String& module,
				const Props& old_props,
				const Props& new_props);


 protected:
  ContentFS(const String& path, const String& name, const String& module);
  String _name;
  String _module;

};


class ContentFSTemplate : public ContentTemplate
{
 public:
  virtual ~ContentFSTemplate();

  static void mount_props_template(const String& module,
				   Props &props);
  static void mount_props_create(const String& path,
				 const String& module,
				 const Props &props);


 protected:
  ContentFSTemplate(const String& name);
  String _name;


};


#endif  // ContentFS_h
