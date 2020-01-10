/*
  Copyright Red Hat, Inc. 2005-2008

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


#include "MountHandler.h"
#include "Mutex.h"
#include "utils.h"

#include <vector>

#include <stdio.h>
#include <mntent.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/file.h>
#include <errno.h>

#include <algorithm>
#include <iostream>

using namespace std;



static String mtab_path("/etc/mtab");
static String fstab_path("/etc/fstab");
static String mounts_path("/proc/mounts");
static String findfs_path("/sbin/findfs");

static Mutex mutex;

static int locker_fd = 0;
static int locker_counter = 0;


static list<String>
follow_links(const String& path);
static String
find_path(const String& devname);
static void
create_dir(const String& path);



Mountpoint::Mountpoint(const String& devname,
		       const String& mountpoint,
		       const String& fstype,
		       const std::pair<unsigned int, unsigned int>& major_minor) :
  devname(devname),
  mountpoint(mountpoint),
  fstype(fstype),
  major_minor(major_minor)
{}

Mountpoint::~Mountpoint()
{}




MountHandler::MountHandler()
{}

MountHandler::~MountHandler()
{}


std::pair<unsigned int, unsigned int>
MountHandler::maj_min(const String& devname) const
{
  MutexLocker l1(mutex);

  String path(follow_links(find_path(devname)).back());

  struct stat st;
  if (stat(path.c_str(), &st))
    throw String("stat(") + path + ") failed: " + String(strerror(errno));
  if (!S_ISBLK(st.st_mode))
    throw path + " is not a block device";

  return pair<unsigned int, unsigned int>(major(st.st_rdev),
					  minor(st.st_rdev));
}

std::list<Mountpoint>
MountHandler::get_entries(const String& filename) const
{
  MutexLocker l1(mutex);
  FstabLocker l2;

  char buff[1024];
  struct mntent mntbuff;
  list<Mountpoint> ret;

  FILE* fstab = setmntent(filename.c_str(), "r");
  if (!fstab)
    throw String("unable to open ") + filename + ": " + String(strerror(errno));
  try {
    while (getmntent_r(fstab, &mntbuff, buff, sizeof(buff)))
      try {
	ret.push_back(Mountpoint(mntbuff.mnt_fsname,
				 mntbuff.mnt_dir,
				 mntbuff.mnt_type,
				 maj_min(mntbuff.mnt_fsname)));
      } catch ( ... ) {}
  } catch ( ... ) {
    endmntent(fstab);
    throw;
  }
  endmntent(fstab);
  return ret;
}


std::list<Mountpoint>
MountHandler::mounts() const
{
  MutexLocker l1(mutex);

  list<Mountpoint> ret = get_entries(mtab_path);


  FILE* file = fopen("/proc/swaps", "r");
  if (!file)
    throw String("unable to open /proc/swaps: ") + String(strerror(errno));
  try {
    bool done = false;
    while (!done) {
      char* ptr = NULL;
      int size;
      size_t tmp;
      if ((size = getline(&ptr, &tmp, file)) == -1) {
	if (feof(file)) {
	  done = true;
	  free(ptr);
	  continue;
	} else {
	  free(ptr);
	  throw String("error reading /proc/swaps: ") + String(strerror(errno));
	}
      }
      String line;
      try {
	line = utils::strip(String(ptr, size));
	free(ptr);
      } catch ( ... ) {
	free(ptr);
	throw;
      }

      if (line.empty())
	continue;
      if (line[0] == '/') {
	String path(utils::split(line)[0]);
	ret.push_back(Mountpoint(path,
				 "swap",
				 "swap",
				 maj_min(path)));
      }
    }
    while (fclose(file) && errno == EINTR)
      ;
  } catch ( ... ) {
    while (fclose(file) && errno == EINTR)
      ;
    throw;
  }

  return ret;
}

std::list<Mountpoint>
MountHandler::mounts(const std::pair<unsigned int, unsigned int>& maj_min) const
{
  MutexLocker l1(mutex);

  list<Mountpoint> ret, l = mounts();
  for (list<Mountpoint>::const_iterator iter = l.begin();
       iter != l.end();
       iter++)
    if (iter->major_minor == maj_min)
      ret.push_back(*iter);
  return ret;
}

std::list<Mountpoint>
MountHandler::fstabs() const
{
  MutexLocker l1(mutex);

  return get_entries(fstab_path);
}

std::list<Mountpoint>
MountHandler::fstabs(const std::pair<unsigned int, unsigned int>& maj_min) const
{
  MutexLocker l1(mutex);

  list<Mountpoint> ret, l = fstabs();
  for (list<Mountpoint>::const_iterator iter = l.begin();
       iter != l.end();
       iter++)
    if (iter->major_minor == maj_min)
      ret.push_back(*iter);
  return ret;
}

bool
MountHandler::mount(const String& devname,
		    const String& mountpoint,
		    const String& fstype) const
{
  MutexLocker l1(mutex);

  String out, err, bin;
  int status;
  vector<String> args;

  if (fstype == "swap") {
    bin = "/sbin/swapon";
    args.clear();
    args.push_back(devname);
  } else {
    // make sure dir exists
    create_dir(mountpoint);

    bin = "/bin/mount";
    args.clear();
    args.push_back(devname);
    args.push_back(mountpoint);
    args.push_back("-t");
    args.push_back(fstype);
  }

  if (utils::execute(bin,
		     args,
		     out,
		     err,
		     status,
		     false))
    throw command_not_found_error_msg(bin);
  return !status;
}

bool
MountHandler::umount(const String& devname,
		     const String& mountpoint) const
{
  MutexLocker l1(mutex);

  String out, err, bin;
  int status;
  vector<String> args;

  if (mountpoint == "swap") {
    bin = "/sbin/swapoff";
    args.clear();
    args.push_back(devname);
  } else {
    bin = "/bin/umount";
    args.clear();
    args.push_back(mountpoint);
  }

  if (utils::execute(bin,
		     args,
		     out,
		     err,
		     status,
		     false))
    throw command_not_found_error_msg(bin);
  return !status;
}

bool
MountHandler::fstab_add(const String& devname,
			const String& mountpoint,
			const String& fstype) const
{
  MutexLocker l1(mutex);
  FstabLocker l2;

  String dev(utils::strip(devname));
  String mnt(utils::strip(mountpoint));
  String type(utils::strip(fstype));

  // verify devname
  pair<unsigned int, unsigned int> mm = maj_min(dev);

  // make sure dir exists
  if (type != "swap")
    create_dir(mnt);

  list<Mountpoint> tabs(fstabs());
  for (list<Mountpoint>::const_iterator iter = tabs.begin();
       iter != tabs.end();
       iter++)
    if (mnt == iter->mountpoint) {
      if (mm == iter->major_minor)
	return true;
      else
	if (mnt != "swap")
	  throw mnt + String("is already in fstab");
    }

  FILE* fstab = setmntent(fstab_path.c_str(), "r+");
  if (!fstab)
    throw String("unable to open ") + fstab_path + ": " + String(strerror(errno));
  bool success;
  try {
    struct mntent st;
    st.mnt_fsname = (char*) dev.c_str();
    st.mnt_dir    = (char*) mnt.c_str();
    st.mnt_type   = (char*) type.c_str();
    st.mnt_opts   = (char*) "defaults";
    st.mnt_freq   = 0;
    st.mnt_passno = 0;

    success = !addmntent(fstab, &st);
  } catch ( ... ) {
    endmntent(fstab);
    throw;
  }
  endmntent(fstab);
  return success;
}

void
MountHandler::fstab_remove(const String& devname,
			   const String& mountpoint) const
{
  MutexLocker l1(mutex);
  FstabLocker l2;

  String dev(utils::strip(devname));
  String mnt(utils::strip(mountpoint));

  String buff;
  bool modified = false;
  FILE* fstab = fopen(fstab_path.c_str(), "r");
  if (!fstab)
    throw String("unable to open ") + fstab_path + ": " + String(strerror(errno));
  try {
    bool done = false;
    while (!done) {
      String line;
      char* ptr = NULL;
      int size;
      size_t tmp;
      if ((size = getline(&ptr, &tmp, fstab)) == -1) {
	if (feof(fstab)) {
	  done = true;
	  free(ptr);
	  continue;
	} else {
	  free(ptr);
	  throw String("error reading fstab: ") + String(strerror(errno));
	}
      }
      try {
	line = String(ptr, size);
	free(ptr);
      } catch ( ... ) {
	free(ptr);
	throw;
      }

      vector<String> words = utils::split(utils::strip(line));
      if (words.size() < 2)
	buff += line;
      else if (words[0].empty())
	buff += line;
      else if (words[0][0] == '#')
	buff += line;
      else if (words[0] == dev && words[1] == mnt)
	modified = true;
      else
	buff += line;
    }
    while (fclose(fstab) && errno == EINTR)
      ;
  } catch ( ... ) {
    while (fclose(fstab) && errno == EINTR)
      ;
    throw;
  }

  // write changes
  if (modified) {
    fstab = fopen(fstab_path.c_str(), "w");
    if (!fstab)
      throw String("unable to open ") + fstab_path + " for writing: " + String(strerror(errno));
    try {
      if (!fwrite(buff.c_str(), buff.size(), 1, fstab))
	throw String("error writing fstab: ") + String(strerror(errno));
      while (fclose(fstab) && errno == EINTR)
	;
    } catch ( ... ) {
      while (fclose(fstab) && errno == EINTR)
	;
      throw;
    }
  }
}

std::list<String>
MountHandler::fstypes() const
{
  MutexLocker l1(mutex);

  list<String> fss;
  fss.push_back("swap");

  FILE* file = fopen("/proc/filesystems", "r");
  if (!file)
    throw String("unable to open /proc/filesystems: " + String(strerror(errno)));
  try {
    bool done = false;
    while (!done) {
      String line;
      char* ptr = NULL;
      int size;
      size_t tmp;
      if ((size = getline(&ptr, &tmp, file)) == -1) {
	if (feof(file)) {
	  done = true;
	  free(ptr);
	  continue;
	} else {
	  free(ptr);
	  throw String("error reading /proc/filesystems: ") + String(strerror(errno));
	}
      }
      try {
	line = utils::strip(String(ptr, size));
	free(ptr);
      } catch ( ... ) {
	free(ptr);
	throw;
      }

      if (line.empty())
	continue;
      if (line.find_first_of(" \t") == line.npos)
	fss.push_back(line);
    }
    while (fclose(file) && errno == EINTR)
      ;
  } catch ( ... ) {
    while (fclose(file) && errno == EINTR)
      ;
    throw;
  }

  return fss;
}

static std::list<String>
_used_dirs(const String& filename)
{
  MutexLocker l1(mutex);
  FstabLocker l2;

  char buff[1024];
  struct mntent mntbuff;
  list<String> ret;

  FILE* fstab = setmntent(filename.c_str(), "r");
  if (!fstab)
    throw String("unable to open ") + filename + ": " + String(strerror(errno));
  try {
    while (getmntent_r(fstab, &mntbuff, buff, sizeof(buff))) {
      String mp(mntbuff.mnt_dir);
      if (!mp.empty())
	if (mp[0] == '/')
	  ret.push_back(mp);
    }
  } catch ( ... ) {
    endmntent(fstab);
    throw;
  }
  endmntent(fstab);
  return ret;
}
std::list<String>
MountHandler::used_dirs() const
{
  MutexLocker l1(mutex);
  FstabLocker l2;

  list<String> ret = _used_dirs(fstab_path.c_str());

  list<String> tmp = _used_dirs(mtab_path.c_str());
  for (list<String>::const_iterator iter = tmp.begin();
       iter != tmp.end();
       iter++)
    if (find(ret.begin(), ret.end(), *iter) == ret.end())
      ret.push_back(*iter);

  tmp = _used_dirs(mounts_path.c_str());
  for (list<String>::const_iterator iter = tmp.begin();
       iter != tmp.end();
       iter++)
    if (find(ret.begin(), ret.end(), *iter) == ret.end())
      ret.push_back(*iter);

  return ret;
}





FstabLocker::FstabLocker()
{
  MutexLocker l(mutex);

  if (locker_counter == 0) {
    locker_fd = open(fstab_path.c_str(), O_RDWR);
    if (locker_fd == -1)
      throw String("unable to open fstab: ") + String(strerror(errno));
    int ret;
    while ((ret = flock(locker_fd, LOCK_EX)) && errno == EINTR)
      ;
    if (ret) {
      while (close(locker_fd) == -1 && errno == EINTR)
	;
      throw String("unable to flock fstab: ") + String(strerror(errno));
    }
  }

  locker_counter++;
}

FstabLocker::~FstabLocker()
{
  MutexLocker l(mutex);

  if (locker_counter == 1) {
    int ret;
    while ((ret = flock(locker_fd, LOCK_UN)) && errno == EINTR)
      ;
    //if (ret)
    //  throw String("unable to unflock fstab");
    while (close(locker_fd) == -1 && errno == EINTR)
      ;
  }

  locker_counter--;
}





list<String>
follow_links(const String& path)
{
  char buff[1024];
  list<String> paths;

  paths.push_back(path);

  int ret;
  while ((ret = readlink(paths.back().c_str(), buff, sizeof(buff))) > 0)
    paths.push_back(String(buff, ret));

  return paths;
}

String
find_path(const String& devname)
{
  if (devname.find("LABEL=") == 0) {
    String out, err;
    int status;
    if (utils::execute(findfs_path,
		       vector<String>(1, devname),
		       out,
		       err,
		       status,
		       false))
      throw command_not_found_error_msg(findfs_path);
    if (status != 0)
      throw String("unable to find path for ") + devname +
	" " + out + " " + err + " " + utils::to_string(status);
    String path = utils::split(out, "\n")[0];
    return utils::strip(path);
  } else if (devname[0] == '/')
    return devname;
  else
    throw String("find_path(") + devname + ") not implemented";
}

void
create_dir(const String& path)
{
  MutexLocker l1(mutex);

  if (path.empty())
    throw String("dir path is empty");
  if (path[0] != '/')
    throw String("dir must be an absolute path");

  String out, err, bin;
  int status;
  vector<String> args;
  args.push_back("-p");
  args.push_back(utils::strip(path));
  if (utils::execute("/bin/mkdir",
		     args,
		     out,
		     err,
		     status,
		     false))
    throw command_not_found_error_msg("mkdir");
  if (status)
    throw String("creation of ") + path + " failed: " + err;
}
