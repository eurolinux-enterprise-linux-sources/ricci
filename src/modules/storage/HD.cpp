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


#include "HD.h"
#include "utils.h"

#include <vector>


#include <iostream>
using namespace std;


HD::HD(const String& path) :
  BD(MAPPER_SYS_TYPE,
     SYS_PREFIX,
     BD_HD_TYPE,
     path)
{
  vector<String> args;
  String out, err;
  int status;

  /*
  // size
  args.push_back("--getss");
  args.push_back("--getsize");
  args.push_back(path);
  if (utils::execute("/sbin/blockdev", args, out, err, status))
    throw command_not_found_error_msg();
  if (status != 0)
    throw String("blockdev failed");
  out = utils::strip(out);
  vector<String> lines = utils::split(out, "\n");
  if (lines.size() != 2)
    throw String("invalid output from blockdev");
  long long size = utils::to_long(utils::strip(lines[0]));
  size *= utils::to_long(utils::strip(lines[1]));
  _props.set(Variable("size", size));
  */


  // vendor & model
  String vendor("unknown");
  String model(vendor);

  args.clear(); out.clear();
  args.push_back(String("/proc/ide") + path.substr(String("/dev").size()) + "/model");
  if (utils::execute("/bin/cat", args, out, err, status))
    throw command_not_found_error_msg("cat");
  if (status == 0) {
    vector<String> words = utils::split(utils::strip(out));
    if (words.size() == 1)
      model = words[0];
    if (words.size() > 1) {
      vendor = words[0];
      model = "";
      for (vector<String>::size_type i=1; i<words.size(); i++)
	model += words[i] + " ";
    }
  }

  String tmp = String("/sys/block") + path.substr(String("/dev").size()) + "/device/";
  args.clear(); out.clear();
  args.push_back(tmp + "vendor");
  if (utils::execute("/bin/cat", args, out, err, status))
    throw command_not_found_error_msg("cat");
  if (status == 0)
    vendor = utils::strip(out);
  args.clear(); out.clear();
  args.push_back(tmp + "model");
  if (utils::execute("/bin/cat", args, out, err, status))
    throw command_not_found_error_msg("cat");
  if (status == 0)
    model = utils::strip(out);
  _props.set(Variable("vendor", vendor));
  _props.set(Variable("model", model));

  String type("ide");

  // scsi_id
  args.clear(); out.clear();
  args.push_back("-g");
  args.push_back("-u");
  args.push_back("-i");
  args.push_back("-s");
  String sys_path = path.substr(String("/dev").size());
  sys_path = "/block" + sys_path;
  args.push_back(sys_path);
  if (utils::execute("/sbin/scsi_id", args, out, err, status))
    throw command_not_found_error_msg("scsi_id");
  if (status == 0) {
    out = utils::strip(out);
    vector<String> words = utils::split(out);
    if (words.size() == 2) {
      type = "scsi";
      String scsi_id = words[1];
      if (scsi_id.size())
	_props.set(Variable("scsi_id", scsi_id));
      String scsi_addr = words[0];
      if (scsi_addr.size()) {
	if (scsi_addr[scsi_addr.size() - 1] == ':')
	  scsi_addr = scsi_addr.substr(0, scsi_addr.size() - 1);
	_props.set(Variable("scsi_bus", scsi_addr));
      }
    }
  }
  _props.set(Variable("type", type));
}

HD::~HD()
{}


void
HD::shrink(unsigned long long new_size,
	   const Props& new_props)
{
  throw String("HD::shrink() not implemented");
}
void
HD::expand(unsigned long long new_size,
	   const Props& new_props)
{
  throw String("HD::expand() not implemented");
}
String
HD::apply_props_before_resize(const Props& new_props)
{
  return path();
}
String
HD::apply_props_after_resize(const Props& new_props)
{
  return path();
}

void
HD::remove()
{
  throw String("HD remove not implemented");
}
