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


#ifndef MountHandler_h
#define MountHandler_h


#include "String.h"
#include <list>


class MountHandler;


class FstabLocker
{
 public:
  FstabLocker();
  virtual ~FstabLocker();

};


class Mountpoint
{
 public:
  virtual ~Mountpoint();

  const String devname;
  const String mountpoint;
  const String fstype;
  const std::pair<unsigned int, unsigned int> major_minor;

 private:
  Mountpoint(const String& devname,
	     const String& mountpoint,
	     const String& fstype,
	     const std::pair<unsigned int,
	     unsigned int>& major_minor);

  friend class MountHandler;
};

class MountHandler
{
 public:
  MountHandler();
  virtual ~MountHandler();

  std::pair<unsigned int, unsigned int> maj_min(const String& devname) const;

  std::list<Mountpoint> mounts() const;
  std::list<Mountpoint> mounts(const std::pair<unsigned int,
			       unsigned int>& maj_min) const;

  std::list<Mountpoint> fstabs() const;
  std::list<Mountpoint> fstabs(const std::pair<unsigned int,
			       unsigned int>& maj_min) const;

  bool mount(const String& devname,
	     const String& mountpoint,
	     const String& fstype) const;
  bool umount(const String& devname,
	      const String& mountpoint) const;

  bool fstab_add(const String& devname,
		 const String& mountpoint,
		 const String& fstype) const;
  void fstab_remove(const String& devname,
		    const String& mountpoint) const;

  std::list<String> fstypes() const; // /proc/filesystems

  std::list<String> used_dirs() const; // list of mounpoints from fstab, mtab & mounts

 private:

  std::list<Mountpoint> get_entries(const String& filename) const;

};


#endif  // MountHandler_h
