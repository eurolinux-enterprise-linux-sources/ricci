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


#include "parted_wrapper.h"
#include "utils.h"

#include <algorithm>
#include <iostream>

using namespace std;


#define PARTED_BIN_PATH   String("/sbin/parted")


static void
fill_gaps(int types,
	  const String& label,
	  long long min_part_size,
	  list<PartedPartition>& parts,
	  long long begin,
	  long long end);
static String
__probe_pt_execute(const String& pt_path);
static list<PartedPartition>
plain_partitions(const String& path,
		 String& label,
		 long long& disk_size);
static void
__create_label(const String& path, const String& label);
static long long
parted_size_to_bytes(const String& size_str);




PartedPartition::PartedPartition(int partnum,
				 long long beg,
				 long long end,
				 bool bootable,
				 PPType type,
				 const String& label) :
  _partnum(partnum),
  _beg(beg),
  _end(end),
  _bootable(bootable),
  _type(type),
  _label(label)
{}

PartedPartition::PartedPartition(long long beg,
				 long long end,
				 PPType available_types,
				 const String& label) :
  _beg(beg),
  _end(end),
  _bootable(false),
  _type((PPType) (available_types | PPTunused)),
  _label(label)
{}

PartedPartition::~PartedPartition()
{}


long long
PartedPartition::size() const
{
  return _end - _beg;
}

long long
PartedPartition::begin() const
{
  return _beg;
}

int
PartedPartition::partnum() const
{
  if (unused_space())
    throw String("partnum(): unused space???");
  return _partnum;
}

bool
PartedPartition::bootable() const
{
  return _bootable;
}

String
PartedPartition::type() const
{
  switch (_type) {
  case PPTprimary:
    return "primary";
  case PPTextended:
    return "extended";
  case PPTlogical:
    return "logical";
  case PPTunused:
    return "unused";
  }
  throw String("invalid partition type");
}

String
PartedPartition::label() const
{
  return _label;
}

bool
PartedPartition::unused_space() const
{
  return _type & PPTunused;
}

bool
PartedPartition::primary() const
{
  return _type & PPTprimary;
}

bool
PartedPartition::extended() const
{
  return _type & PPTextended;
}

bool
PartedPartition::logical() const
{
  return _type & PPTlogical;
}

void
PartedPartition::add_kid(const PartedPartition& part)
{
  if (extended() && part.logical()) {
    _kids.push_back(part);
    _kids.sort();
  } else
    throw String("PartedPartition.add_kid() error");
}

const list<PartedPartition>&
PartedPartition::kids() const
{
  return _kids;
}

list<PartedPartition>&
PartedPartition::kids()
{
  return _kids;
}

bool
PartedPartition::operator<(const PartedPartition& p2) const
{
  return _beg < p2._beg;
}

void
PartedPartition::print(const String& indent) const
{
  cout << indent << "num" << (unused_space()) ? 100000LL : partnum();
  cout << " beg" << begin();
  cout << " size" << size();
  cout << " boot" << bootable();

  String t = "selection";
  try {
    t = type();
  } catch ( ... ) {}
  cout << " " << t;

  cout << " used" << !unused_space() << endl;
  for (list<PartedPartition>::const_iterator iter = _kids.begin();
       iter != _kids.end();
       iter++) {
    iter->print(indent + indent);
  }
}





//   ##### Parted #####

String
Parted::extract_pt_path(const String& path)
{
  if (path.find("/dev/cciss/") != path.npos) {
    unsigned int p_pos = path.find("p");
    return path.substr(0, p_pos);
  }

  unsigned int i = path.size() - 1;
  for (; isdigit(path[i]); i--)
    ;
  return path.substr(0, i+1);
}

String
Parted::generate_part_path(const String& pt_path, const PartedPartition& part)
{
  if (part.unused_space())
    throw String("generate_part_path(): unused space???");
  if (pt_path.find("/dev/cciss/") != pt_path.npos)
    return pt_path + "p" + utils::to_string(part.partnum());
  else
    return pt_path + utils::to_string(part.partnum());
}



String
__probe_pt_execute(const String& pt_path)
{
  vector<String> args;
  args.push_back(pt_path);
  args.push_back("unit");
  args.push_back("B");
  args.push_back("print");
  args.push_back("-s");
  String out, err;
  int status;
  if (utils::execute(PARTED_BIN_PATH, args, out, err, status))
    throw command_not_found_error_msg(PARTED_BIN_PATH);
  if (status)
    throw String("parted failed") + " " + pt_path;
  return out;
}

list<PartedPartition>
plain_partitions(const String& path,
		 String& label,
		 long long& disk_size)
{
  list<PartedPartition> parts;
  label = "";
  disk_size = 0;

  String parted_output = utils::strip(__probe_pt_execute(path));
  vector<String> lines = utils::split(parted_output, "\n");
  for (vector<String>::const_iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    vector<String> words = utils::split(utils::strip(*iter));
    if (words.size() < 3)
      continue;

    // label
    if ((words[0] == "Disk" && words[1] == "label" && words[2] == "type:") ||
	(words[0] == "Partition" && words[1] == "Table:")) {
      label = words.back();
      continue;
    }

    // disk size
    if (words[0] == "Disk" && words[1] == (path + ":")) {
      disk_size = parted_size_to_bytes(words[2]);
      continue;
    }
    if (words.size() > 4) {
      if (words[0] == "Disk" &&
	  words[1] == "geometry" &&
	  words[2] == "for" &&
	  words[3] == (path + ":")) {
	String s = words[4];
	String::size_type idx = s.find("-");
	if (idx != s.npos)
	  s = s.substr(idx + 1);
	else
	  s = words.back();
	disk_size = parted_size_to_bytes(s);
	continue;
      }
    }

    // partition
    if (!isdigit(words[0][0]))
      continue;
    int partnum = utils::to_long(words[0]);
    long long beg = parted_size_to_bytes(words[1]);
    long long end = parted_size_to_bytes(words[2]);
    bool bootable = false;
    PPType type = PPTprimary;
    for (vector<String>::const_iterator word_iter = words.begin();
	 word_iter != words.end();
	 word_iter++) {
      if (*word_iter == "boot")
	bootable = true;
      else if (*word_iter == "primary")
	type = PPTprimary;
      else if (*word_iter == "extended")
	type = PPTextended;
      else if (*word_iter == "logical")
	type = PPTlogical;
    }
    parts.push_back(PartedPartition(partnum,
				    beg,
				    end,
				    bootable,
				    type,
				    label));
  }

  if (label.empty() || label == "loop")
    throw String("not a partition table");
  if (disk_size == 0)
    throw String("disk_size == 0???");

  return parts;
}





// return label & partitions
pair<String, list<PartedPartition> >
Parted::partitions(const String& path)
{
  String label;
  long long disk_size;
  list<PartedPartition> parts = plain_partitions(path, label, disk_size);

  int primary_count = 0;

  // put logical into extended
  PartedPartition extended(0, 0, PPTunused, label);
  for (list<PartedPartition>::const_iterator iter = parts.begin();
       iter != parts.end();
       iter++) {
    if (iter->extended())
      extended = *iter;
    if (iter->primary() || iter->extended())
      primary_count++;
  }
  list<PartedPartition> sorted;
  for (list<PartedPartition>::const_iterator iter = parts.begin();
       iter != parts.end();
       iter++) {
    if (iter->primary())
      sorted.push_back(*iter);
    else if (iter->logical())
      extended.add_kid(*iter);
  }
  if (!extended.unused_space())
    sorted.push_back(extended);
  sorted.sort();

  /*
  cout << "\n\npt_path " << path << endl;
  for (list<PartedPartition>::const_iterator iter = sorted.begin();
       iter != sorted.end();
       iter++)
    iter->print("\t");
  */

  // fill gaps
  bool more_primaries = false;
  if (label == "gpt" ||
      label == "bsd" ||
      label == "sun" ||
      label == "mac" ||
      (label == "msdos" && primary_count < 4) )
    more_primaries = true;
  PPType types = PPTunused;
  if (more_primaries) {
    types = (PPType) (types | PPTprimary);
    if (label == "msdos" && extended.unused_space())
      types = (PPType) (types | PPTextended);
  }
  fill_gaps(types,
	    label,
	    min_part_size(label),
	    sorted,
	    0,
	    disk_size);

  /*
  cout << "\n\npt_path " << path << endl;
  for (list<PartedPartition>::const_iterator iter = sorted.begin();
       iter != sorted.end();
       iter++)
    iter->print("\t");
  */

  return pair<String, list<PartedPartition> >(label, sorted);
}

void
fill_gaps(int types,
	  const String& label,
	  long long min_part_size,
	  list<PartedPartition>& parts,
	  long long begin,
	  long long end)
{
  long long current = begin;
  list<PartedPartition>::iterator iter = parts.begin();
  while (current < end) {
    if (iter == parts.end()) {
      PartedPartition n(current,
			end,
			PPType(types),
			label);
      if (n.size() >= min_part_size)
	parts.insert(iter, n);
      current = n.begin() + n.size();
      continue;
    }

    if (iter->extended())
      fill_gaps(PPTlogical,
		label,
		min_part_size,
		iter->kids(),
		iter->begin(),
		iter->begin() + iter->size());

    if (iter->begin() <= current) {
      current = iter->begin() + iter->size();
      iter++;
    } else {
      PartedPartition n(current,
			iter->begin(),
			PPType(types),
			label);
      if (n.size() > min_part_size)
	parts.insert(iter, n);
      current = n.begin() + n.size();
    }
  }
  /*
  for (list<PartedPartition>::const_iterator iter = parts.begin();
       iter != parts.end();
       iter++) {
    cout << iter->begin() << " " << iter->begin() + iter->size();
    cout << " " << (bool) iter->unused_space() << endl;
  }
  cout << endl;
  */
}



std::list<String>
Parted::possible_paths()
{
  list<String> hds;

  // parse blockdev
  vector<String> args;
  args.push_back("--report");
  String out, err;
  int status;
  if (utils::execute("/sbin/blockdev", args, out, err, status))
    throw command_not_found_error_msg("blockdev");
  if (status)
    throw String("blockdev failed");
  vector<String> lines = utils::split(out, "\n");
  for (vector<String>::const_iterator iter = lines.begin();
       iter != lines.end();
       iter++) {
    vector<String> words = utils::split(utils::strip(*iter));
    if (words.size() != 7 || words[0] == "RO")
      continue;
    String path = words[6];

    if (path == Parted::extract_pt_path(path))
      hds.push_back(path);
  }

  return hds;
}


std::list<String>
Parted::supported_labels()
{
  list<String> labels;
  labels.push_back("bsd");
  labels.push_back("gpt");
  //  labels.push_back("mac");
  //  labels.push_back("mips");
  labels.push_back("msdos");
  //  labels.push_back("pc98");
  //  labels.push_back("sun");

  return labels;
}


void
__create_label(const String& path, const String& label)
{
  if (Parted::extract_pt_path(path) != path)
    throw String("partition table on partition??? Not for now.");

  list<String> labels = Parted::supported_labels();
  if (find(labels.begin(), labels.end(), label) == labels.end())
    throw String("unsuported label type");

  // create label
  vector<String> args;
  args.push_back("-s");
  args.push_back(path);
  args.push_back("mklabel");
  args.push_back(label);
  String out, err;
  int status;
  if (utils::execute(PARTED_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(PARTED_BIN_PATH);
  if (status)
    throw String("parted failed");
}

void
Parted::create_label(const String& path, const String& label)
{
  // don't overwrite existing label
  bool in_use = false;
  try {
    __probe_pt_execute(path); // throws if empty (no PT or known FS)
    in_use = true;
  } catch ( ... ) {}
  if (in_use)
    throw String(path + " already has a partition table");

  // create label
  __create_label(path, label);
}

void
Parted::remove_label(const String& path)
{
  __create_label(path, "msdos");

  vector<String> args;
  String out, err;
  int status;
  args.push_back("if=/dev/zero");
  args.push_back(String("of=") + path);
  args.push_back("bs=1");
  args.push_back("seek=447");
  args.push_back("count=64");
  if (utils::execute("/bin/dd", args, out, err, status))
    throw command_not_found_error_msg("dd");
  if (status != 0)
    throw String("dd failed");
  utils::clear_cache();
}


long long
Parted::min_part_size(const String& label)
{
  return 2 * 1024 * 1024;  // 2 MB // FIXME
}



String
Parted::create_partition(const String& pt_path,
			 const String& part_type,
			 long long seg_begin,
			 long long size)
{
  String aaa;
  long long bbb;
  list<PartedPartition> parts = plain_partitions(pt_path, aaa, bbb);

  // parted defines 1KB as 1000 bytes
  seg_begin = seg_begin / 1000 / 1000;
  long long seg_end   = seg_begin + size / 1000 / 1000;

  vector<String> args;
  args.push_back("-s");
  args.push_back(pt_path);
  args.push_back("mkpart");
  args.push_back(part_type);
  args.push_back(utils::to_string(seg_begin));
  args.push_back(utils::to_string(seg_end));
  String out, err;
  int status;
  if (utils::execute(PARTED_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(PARTED_BIN_PATH);
  if (status)
    throw String("parted failed");

  utils::clear_cache();

  // find new partnum
  list<PartedPartition> new_parts = plain_partitions(pt_path, aaa, bbb);
  for (list<PartedPartition>::const_iterator iter_n = new_parts.begin();
       iter_n != new_parts.end();
       iter_n++) {
    if (iter_n->unused_space())
      continue;

    bool new_part = true;
    for (list<PartedPartition>::const_iterator iter_o = parts.begin();
	 iter_o != parts.end();
	 iter_o++) {
      if (iter_o->unused_space())
	continue;
      if (iter_n->partnum() == iter_o->partnum())
	new_part = false;
    }

    if (new_part)
      return generate_part_path(pt_path, *iter_n);
  }

  throw String("partition creation failed");
}

void
Parted::remove_partition(const String& pt_path,
			 const PartedPartition& partition)
{
  vector<String> args;
  args.push_back("-s");
  args.push_back(pt_path);
  args.push_back("rm");
  args.push_back(utils::to_string(partition.partnum()));
  String out, err;
  int status;
  if (utils::execute(PARTED_BIN_PATH, args, out, err, status, false))
    throw command_not_found_error_msg(PARTED_BIN_PATH);
  if (status)
    throw String("parted failed");

  utils::clear_cache();
}


long long
parted_size_to_bytes(const String& size_str)
{
  String s = utils::to_lower(utils::strip(size_str));
  long long multiplier;
  // parted defines 1KB as 1000 bytes.
  if (s.find("b") == s.npos) {
    multiplier = 1000 * 1000;  // by old parted behavior, size is in MB
    return (long long) utils::to_long(s) * multiplier;
  } else {
    if (s.size() < 3)
      throw String("parted size has an invalid value: ") + s;
	/* This path should never be hit on RHEL5 and later. */
    multiplier = 1;
    if (s[s.size()-2] == 'k')
      multiplier = 1000;
    else if (s[s.size()-2] == 'm')
      multiplier = 1000 * 1000;
    else if (s[s.size()-2] == 'g')
      multiplier = 1000 * 1000 * 1000;
    else if (s[s.size()-2] == 't')
      multiplier = (long long) 1000 * 1000 * 1000 * 1000;
    return (long long) utils::to_float(s) * multiplier;
  }
}
