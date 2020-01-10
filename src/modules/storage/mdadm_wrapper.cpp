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


#include "mdadm_wrapper.h"
#include "utils.h"


#define MDADM_BIN_PATH   String("/sbin/mdadm")


#include <iostream>
using namespace std;


static list<mdraid> probe_raids();
static list<mdraid_source> probe_sources(vector<String> paths);
static pair<String, list<mdraid_source> > probe_source(const String& path);


mdraid_source::mdraid_source(const String& path,
			     mdraid_source_type type) :
  path(path),
  type(type)
{}




mdraid::mdraid()
{}



std::list<String>
mdadm::valid_raid_levels()
{
  list<String> levels;
  levels.push_back("raid5");
  levels.push_back("raid1");
  return levels;
  levels.push_back("raid4");
  levels.push_back("raid0");
  return levels;
}


list<mdraid>
mdadm::raids()
{
  return probe_raids();
}

mdraid
mdadm::probe_path(const String& path)
{
  list<mdraid> raids = mdadm::raids();

  for (list<mdraid>::const_iterator iter = raids.begin();
       iter != raids.end();
       iter++) {
    // check source
    for (list<mdraid_source>::const_iterator so_iter = iter->devices.begin();
	 so_iter != iter->devices.end();
	 so_iter++)
      if (so_iter->path == path)
	return *iter;
    // check raid
    if (iter->path == path)
      return *iter;
  }

  throw String("not mdraid path");
}


void
mdadm::remove_source(const String& raid_path,
		     const String& path)
{
  vector<String> args;
  args.push_back(raid_path);
  args.push_back("--remove");
  args.push_back(path);
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();

  zero_superblock(path);
}

void
mdadm::zero_superblock(const String& path)
{
  String out, err;
  int status;
  vector<String> args;
  args.push_back("--zero-superblock");
  args.push_back(path);
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();
}

void
mdadm::add_source(const String& raid_path,
		  const String& path)
{
  vector<String> args;
  args.push_back(raid_path);
  args.push_back("--add");
  args.push_back(path);
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();
}

void
mdadm::fail_source(const String& raid_path,
		   const String& path)
{
  vector<String> args;
  args.push_back(raid_path);
  args.push_back("--fail");
  args.push_back(path);
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();
}

void
mdadm::start_raid(const mdraid& raid)
{
  vector<String> args;
  args.push_back("--assemble");
  args.push_back(raid.path);
  for (list<mdraid_source>::const_iterator iter = raid.devices.begin();
       iter != raid.devices.end();
       iter++)
    args.push_back(iter->path);
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();
}

void
mdadm::stop_raid(const mdraid& raid)
{
  vector<String> args;
  args.push_back("--stop");
  args.push_back(raid.path);
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();
}

String
mdadm::create_raid(const String& level,
		   const list<String>& dev_paths)
{
  String md_path_templ = "/dev/md";
  String new_md_path = md_path_templ;

  list<mdraid> raids = mdadm::raids();
  for (long long i=1; i<30; i++) {
    String tmp_path = md_path_templ + utils::to_string(i);
    bool found = false;
    for (list<mdraid>::const_iterator iter = raids.begin();
	 iter != raids.end();
	 iter++)
      if (iter->path == tmp_path)
	found = true;
    if (!found) {
      new_md_path = tmp_path;
      break;
    }
  }
  if (new_md_path == md_path_templ)
    throw String("no more raid devices allowed");

  list<String>::size_type raid_devices = 3;
  if (level == "raid1")
    raid_devices = 2;
  else if (level == "raid5")
    raid_devices = 3;
  else
    throw String("unsupported raid level");
  raid_devices = (raid_devices < dev_paths.size()) ? dev_paths.size() : raid_devices;

  vector<String> args;
  args.push_back("--create");
  args.push_back(new_md_path);
  args.push_back("-R");
  args.push_back(String("--level=") + level);
  args.push_back(String("--raid-devices=") + utils::to_string((long long) raid_devices));
  args.push_back(String("--spare-devices=0"));
  for (list<String>::const_iterator iter = dev_paths.begin();
       iter != dev_paths.end();
       iter++)
    args.push_back(*iter);
  for (unsigned int i=0;
       i < raid_devices - dev_paths.size();
       i++)
    args.push_back("missing");

  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");
  utils::clear_cache();

  return new_md_path;
}






list<mdraid>
probe_raids()
{
  vector<String> args;
  args.push_back("--examine");
  args.push_back("--scan");
  args.push_back("--brief");
  args.push_back("--config=partitions");
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");

  // remove '\n'
  String::size_type i = 0;
  while ((i = out.find('\n', i)) != out.npos)
    out.replace(i, 1, " ");

  list<mdraid> raids;

  vector<String> words = utils::split(utils::strip(out));
  for (vector<String>::iterator iter = words.begin();
       iter != words.end();
       iter++) {
    if (iter->empty())
      continue;
    if (*iter == "ARRAY") {
      raids.push_back(mdraid());
      continue;
    }
    if (raids.empty())
      continue;
    mdraid& raid = raids.back();

    if ((*iter)[0] == '/')
      raid.path = *iter;
    else if (iter->find("level=") == 0)
      raid.level = iter->substr(String("level=").size());
    else if (iter->find("num-devices=") == 0)
      raid.num_devices = utils::to_long(iter->substr(String("num-devices=").size()));
    else if (iter->find("UUID=") == 0)
      raid.uuid = iter->substr(String("UUID=").size());
    else if (iter->find("devices=") == 0) {
      String devices = iter->substr(String("devices=").size());
      vector<String> devs = utils::split(devices, ",");
      raid.devices = probe_sources(devs);
    }
  }

  for (list<mdraid>::iterator iter = raids.begin();
       iter != raids.end();
       iter++) {
    String path = iter->path;
    iter->name = path.replace(0, String("/dev/").size(), "");
  }

  return raids;
}



list<mdraid_source>
probe_sources(vector<String> paths)
{
  list<mdraid_source> devs;

  String date("0000");
  list<mdraid_source> probed_sources;

  for (vector<String>::const_iterator iter = paths.begin();
       iter != paths.end();
       iter++) {
    pair<String, list<mdraid_source> > tmp =
      probe_source(*iter);
    if (tmp.first > date) {
      date = tmp.first;
      probed_sources = tmp.second;
    }
  }

  for (vector<String>::const_iterator path_iter = paths.begin();
       path_iter != paths.end();
       path_iter++) {
    bool path_found = false;
    for (list<mdraid_source>::const_iterator md_iter = probed_sources.begin();
	 md_iter != probed_sources.end();
	 md_iter++)
      if (*path_iter == md_iter->path) {
	path_found = true;
	devs.push_back(*md_iter);
      }
    if (!path_found)
      devs.push_back(mdraid_source(*path_iter, MDRAID_S_FAILED));
  }

  return devs;
}

pair<String, list<mdraid_source> >
probe_source(const String& path)
{
  vector<String> args;
  args.push_back("--examine");
  args.push_back(path);
  String out, err;
  int status;
  if (utils::execute(MDADM_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(MDADM_BIN_PATH);
  if (status)
    throw String("mdadm failed");


  String update_time;
  list<mdraid_source> devs;

  bool devs_section = false;
  vector<String> lines = utils::split(utils::strip(out), "\n");
  for (vector<String>::iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    String& line = *iter;
    vector<String> words = utils::split(utils::strip(line));
    if (words.empty())
      continue;

    if (devs_section) {
      if (line.find("active") != line.npos)
	devs.push_back(mdraid_source(words.back(), MDRAID_S_ACTIVE));
      else if (line.find("spare") != line.npos)
	devs.push_back(mdraid_source(words.back(), MDRAID_S_SPARE));
      continue;
    } else if (words[0] == "this") {
      devs_section = true;
      continue;
    }

    if (words.size() < 8)
      continue;
    if (words[0] == "Update" && words[1] == "Time") {
      String month = words[4];
      String day = words[5];
      String time = words[6];
      String year = words[7];

      if (month == "Jan") month = "01";
      else if (month == "Feb") month = "02";
      else if (month == "Mar") month = "03";
      else if (month == "Apr") month = "04";
      else if (month == "May") month = "05";
      else if (month == "Jun") month = "06";
      else if (month == "Jul") month = "07";
      else if (month == "Aug") month = "08";
      else if (month == "Sep") month = "09";
      else if (month == "Oct") month = "10";
      else if (month == "Nov") month = "11";
      else month = "12";

      if (day.size() == 1)
	day = "0" + day;

      vector<String> time_words = utils::split(time, ":");
      if (time_words.size() != 3)
	throw String("invalid mdadm output");
      if (time_words[0].size() == 1) time_words[0] = "0" + time_words[0];
      if (time_words[1].size() == 1) time_words[1] = "0" + time_words[1];
      if (time_words[2].size() == 1) time_words[2] = "0" + time_words[2];
      time = time_words[0] + time_words[1] + time_words[2];

      update_time = year + " " + month + " " + day + " " + time;
    }
  }


  return pair<String, list<mdraid_source> >(update_time,
					    devs);
}
