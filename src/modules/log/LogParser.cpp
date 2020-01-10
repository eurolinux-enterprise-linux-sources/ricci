/*
** Copyright (C) Red Hat, Inc. 2006-2009
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License version 2 as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING. If not, write to the
** Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
** MA 02139, USA.
*/
/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */

#include "LogParser.h"
#include "utils.h"
#include "Mutex.h"

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>

using namespace std;

static const char *cluster[] = {
	"cluster",
	"modclusterd",
	"dlm",
	"gulm",
	"lock_gulmd",
	"lock_gulmd_main",
	"lock_gulmd_core",
	"lock_gulmd_LT",
	"lock_gulmd_LTPX",
	"cman",
	"cman_tool",
	"ccs",
	"ccs_tool",
	"ccsd",
	"fence",
	"fenced",
	"clvmd",
	"gfs",
	"gfs2",
	"openais",
	"groupd",
	"qdiskd",
	"dlm_controld",
	"gfs_controld",
	"clurgmgrd",
	"rgmanager"
};

static const char *cluster_service_manager[] = {
	"clurgmgrd",
	"rgmanager"
};

static const char *LVS[] = {
	"ipvs",
	"ipvsadm",
	"piranha",
	"piranha-gui"
};

static const char *storage[] = {
	"gfs",
	"gfs2",
	"lvm",
	"clvm",
	"clvmd",
	"end_request",
	"buffer",
	"scsi",
	"md",
	"raid0",
	"raid1",
	"raid4",
	"raid5",
	"cdrom",
	"ext2",
	"ext3",
	"ext3-fs",
	"swap",
	"mount",
	"automount"
};

static const char *selinux[] = {
	"selinux",
	"security",
	"pam",
	"audit"
};

class tag_set
{
public:
	tag_set(const String& name, const set<String>& elements) :
		name(name),
		elements(elements) {}

	String name;
	set<String> elements;
	set<String> match;
};

static const vector<tag_set>&
get_sets()
{
	static vector<tag_set> sets;
	static Mutex mutex;
	static bool init = false;

	MutexLocker l(mutex);
	if (!init) {
		init = true;
		tag_set clu("cluster",
			set<String>(cluster, cluster + arr_elem(cluster)));
		clu.match.insert("fence_");
		sets.push_back(clu);

		sets.push_back(tag_set("cluster service manager",
			set<String>(cluster_service_manager,
				cluster_service_manager + arr_elem(cluster_service_manager))));

		sets.push_back(tag_set("lvs", set<String>(LVS, LVS + arr_elem(LVS))));

		sets.push_back(tag_set("storage",
			set<String>(storage, storage + arr_elem(storage))));

		tag_set SEL("selinux",
			set<String>(selinux, selinux + arr_elem(selinux)));
		SEL.match.insert("pam_");
		sets.push_back(SEL);
	}

	return sets;
}


static time_t
parse_time(const String& time_rep)
{
	struct tm tm;
	if (!strptime(time_rep.c_str(), "%b %d %T %Y", &tm))
		throw String("invalid log entry format");
	return mktime(&tm);
}

set<String>&
get_files(const String& path_, set<String>& files, time_t age_time)
{
	String path = utils::rstrip(utils::strip(path_), "/");
	if (path.empty() ||
		path.find_first_of(" ; & $ ` ? > < ' \" ; | \\ * \n \t") != path.npos)
	{
		return files;
	}

	if (path[0] != '/')
		return files;

	struct stat st;
	if (stat(path.c_str(), &st))
		return files;

	if (S_ISREG(st.st_mode)) {
		if (st.st_mtime >= age_time)
			files.insert(path);

		// get rotated logs
		for (int i = 0 ; i < 25 ; i++)
			get_files(path + "." + utils::to_string(i), files, age_time);

		return files;
	} else if (S_ISDIR(st.st_mode))
		; // process directory
	else
		return files;

	DIR *d = opendir(path.c_str());
	if (d == NULL) {
		throw String("unable to open directory ")
				+ path + ":" + String(strerror(errno));
	}

	try {
		while (true) {
			struct dirent *ent = readdir(d);
			if (ent == NULL) {
				closedir(d);
				return files;
			}

			String kid_path = ent->d_name;
			if (kid_path == "." || kid_path == "..")
				continue;

			kid_path = path + "/" + kid_path;
			get_files(kid_path, files, age_time);
		}
	} catch ( ... ) {
		closedir(d);
		throw;
	}
}

LogEntry::LogEntry(const String& entry)
{
	vector<String> words = utils::split(entry);
	if (words.size() < 6)
		throw String("invalid log entry format");

	if (words[4] == "last" &&
		words[5] == "message")
	{
		throw String("LogEntry: last message repetition");
	}

	if (words[5] == "printk:" &&
		entry.find("suppressed") != entry.npos)
	{
		throw String("LogEntry: printk repetition");
	}

	// get current year (not present in log files)
	char buff[64];
	time_t current_time = ::time(NULL);
	struct tm tm;

	localtime_r(&current_time, &tm);
	if (strftime(buff, sizeof(buff), "%Y", &tm) != 4)
		throw String("failed to get current year");
	String year(buff, 4);
	String time_rep = words[0] + " " + words[1] + " " + words[2] + " " + year;
	age = (long long) difftime(current_time, parse_time(time_rep));

	if (age < 0) {
		// beginning of the year fix, since year is not present in log files
		// FIXME: works only for log files shorter than 2 years
		year = utils::to_string(utils::to_long(year) - 1);
		time_rep = words[0] + " " + words[1] + " " + words[2] + " " + year;
		age = (long long) difftime(current_time, parse_time(time_rep));
	}

	if (age < 0)
		throw String("error in LogEntry() - negative age");

	// domain & pid
	String d = utils::rstrip(utils::to_lower(words[4]), ":");
	String::size_type i = d.find("[");
	domain = d.substr(0, i);
	tags.insert(domain);

	if (i != d.npos) {
		pid = utils::strip(d.substr(i));
		pid = utils::lstrip(pid, "[");
		pid = utils::rstrip(pid, "]");
		tags.insert(pid);
	}

	vector<String>::size_type idx = 5;
	if (domain == "kernel") {
		domain = utils::rstrip(utils::to_lower(words[5]), ":");
		tags.insert(domain);

		if (words[5][words[5].size() - 1] == ':')
			idx = 6;
	}

	// message
	for ( ; idx < words.size() ; idx++)
		msg += words[idx] + " ";
	msg = utils::strip(msg);

	// tags (misc)
	for (vector<String>::size_type j = 4 ; j < 6 ; j++) {
		String t(utils::strip(utils::to_lower(words[j])));

		if (t.empty())
			continue;

		if (t[t.size() - 1] != ':')
			continue;

		t = utils::rstrip(t, ":");
		tags.insert(t);

		if ((i = t.find("(")) != t.npos) {
			tags.insert(utils::strip(t.substr(0, i)));
			String tmp_tag = utils::strip(t.substr(i));
			tmp_tag = tmp_tag.substr(0, tmp_tag.find(")"));
			tmp_tag = utils::lstrip(tmp_tag, "(");
			tmp_tag = utils::rstrip(tmp_tag, ")");
			tags.insert(tmp_tag);
		}

		if ((i = t.find("[")) != t.npos) {
			tags.insert(utils::strip(t.substr(0, i)));
			String tmp_tag = utils::strip(t.substr(i));
			tmp_tag = tmp_tag.substr(0, tmp_tag.find("]"));
			tmp_tag = utils::lstrip(tmp_tag, "[");
			tmp_tag = utils::rstrip(tmp_tag, "]");
			tags.insert(tmp_tag);
		}
	}

	// tags (from tag sets)
	vector<tag_set> sets = get_sets();
	set<String> tags_c(tags);

	for (set<String>::const_iterator
			iter = tags_c.begin() ;
			iter != tags_c.end() ;
			iter++)
	{
		for (vector<tag_set>::const_iterator
				ts_iter = sets.begin() ;
				ts_iter != sets.end() ;
				ts_iter++)
		{
			if (ts_iter->elements.find(*iter) != ts_iter->elements.end())
				tags.insert(ts_iter->name);

			for (set<String>::const_iterator
					m_iter = ts_iter->match.begin() ;
					m_iter != ts_iter->match.end() ;
					m_iter++)
			{
				if (iter->find(*m_iter) == 0)
					tags.insert(ts_iter->name);
			}
		}
	}

	if (tags.find("") != tags.end())
		tags.erase("");
}

String
LogEntry::compare_str() const
{
	String b(utils::to_string(age));
	b += domain + pid + msg;
	return b;
}

bool
LogEntry::operator < (const LogEntry& obj) const
{
	return compare_str() < obj.compare_str();
}

XMLObject
LogEntry::xml() const
{
	XMLObject x("logentry");
	x.set_attr("domain", domain);
	x.set_attr("pid", pid);
	x.set_attr("age", utils::to_string(age));
	x.set_attr("msg", msg);

	for (set<String>::const_iterator
			iter = matched_tags.begin() ;
			iter != matched_tags.end() ;
			iter++)
	{
		XMLObject t("match");
		t.set_attr("tag", *iter);
		x.add_child(t);
	}

	return x;
}

set<LogEntry>
LogParser::get_entries(	long long age,
						const std::list<String>& domains,
						const list<String>& paths)
{
	set<LogEntry> ret;
	time_t age_time = time(NULL);

	if ((long long) age_time - age < 0)
		age_time = 0;
	else
		age_time -= age;


	// set of requested tags
	set<String> req_tags(domains.begin(), domains.end());

	// get log paths
	set<String> files;

	for (list<String>::const_iterator
			iter = paths.begin() ;
			iter != paths.end() ;
			iter++)
	{
		get_files(*iter, files, age_time);
	}

	if (files.empty()) {
		get_files("/var/log/messages", files, age_time);
		get_files("/var/log/syslog", files, age_time);
	}

	// process log files
	for (set<String>::const_iterator
			iter = files.begin() ;
			iter != files.end() ;
			iter++)
	{
		ifstream log(iter->c_str());
		if (!log.is_open())
			throw String("failed to open ") + *iter;

		while (log.good()) {
			char buff[4096];

			try {
				log.getline(buff, sizeof(buff));
				LogEntry e(buff);

				if (e.age > age)
					continue;

				if (req_tags.empty())
					ret.insert(e);
				else {
					bool add = false;
					for (set<String>::const_iterator
							t_iter = req_tags.begin() ;
							t_iter != req_tags.end() ;
							t_iter++)
					{
						if (e.tags.find(*t_iter) != e.tags.end()) {
							add = true;
							e.matched_tags.insert(*t_iter);
						}

						if (add)
							ret.insert(e);
					}
				}
			} catch ( ... ) {}
		}
	}

	return ret;
}
