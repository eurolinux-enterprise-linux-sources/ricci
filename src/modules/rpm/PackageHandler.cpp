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

#include "PackageHandler.h"
#include "utils.h"
#include "File.h"

#include <unistd.h>
#include <sys/utsname.h>

using namespace std;

#define RPM_PATH		"/bin/rpm"
#define YUM_PATH		"/usr/bin/yum"

// class PackageInstaller

PackageInstaller::PackageInstaller()
{
}

PackageInstaller::~PackageInstaller()
{}

bool
PackageInstaller::available()
{
	// use yum
	// nothing to check for, maybe ping repositories?
	return true;
}

map<String, String>
PackageInstaller::available_rpms()
{
	map<String, String> rpms;
	String out, err;
	int status;
	vector<String> args;

	if (!available())
		return rpms;

	args.push_back("-y");
	args.push_back("list");
	args.push_back("all");

	if (utils::execute(YUM_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(YUM_PATH);

	vector<String> lines = utils::split(utils::strip(out), "\n");

	for (vector<String>::const_iterator
			iter = lines.begin() ;
			iter != lines.end() ;
			iter++)
	{
		String line(*iter);
		line = utils::strip(line);
		vector<String> words = utils::split(line);
		vector<String>::size_type l = words.size();
		if (l < 3)
			continue;

		String name = words[0];
		String::size_type idx = name.rfind('.');
		if (idx == String::npos)
			continue;
		name = name.substr(0, idx);
		String version = words[1];
		rpms[name] = version;
	}

	return rpms;
}

bool
PackageInstaller::install(vector<String> rpms)
{
	vector<String> rpms_to_install, rpms_to_upgrade;
	String out, err;
	int status;
	vector<String> args;

	if (rpms.empty())
		return true;

	args.push_back("-y");
	args.push_back("list");
	args.push_back("installed");

	if (utils::execute(YUM_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(YUM_PATH);
	if (status)
		return false;

	vector<String> lines = utils::split(utils::strip(out), "\n");
	for (vector<String>::const_iterator
			rpm = rpms.begin() ;
			rpm != rpms.end() ;
			rpm++)
	{
		bool install = true;
		for (vector<String>::const_iterator
				iter = lines.begin() ;
				iter != lines.end() ;
				iter++)
		{
			String line(*iter);
			line = utils::strip(line);
			if (line.find(*rpm + ".") == 0)
				install = false;
		}

		if (install)
			rpms_to_install.push_back(*rpm);
		else
			rpms_to_upgrade.push_back(*rpm);
	}

	if (!rpms_to_install.empty()) {
		out = err = "";
		args.clear();

		args.push_back("-y");
		args.push_back("install");
		for (vector<String>::const_iterator
				rpm = rpms_to_install.begin() ;
				rpm != rpms_to_install.end() ;
				rpm++)
		{
			args.push_back(*rpm);
		}

		if (utils::execute(YUM_PATH, args, out, err, status, false))
			throw command_not_found_error_msg(YUM_PATH);
		if (status)
			return false;
	}

	if (!rpms_to_upgrade.empty()) {
		out = err = "";
		args.clear();

		args.push_back("-y");
		args.push_back("update");
		for (vector<String>::const_iterator
				rpm = rpms_to_upgrade.begin() ;
				rpm != rpms_to_upgrade.end() ;
				rpm++)
		{
			args.push_back(*rpm);
		}

		if (utils::execute(YUM_PATH, args, out, err, status, false))
			throw command_not_found_error_msg(YUM_PATH);
		if (status)
			return false;
		}
	return true;
}

// #### class Package ####

Package::Package()
{
}

Package::Package(const String& name) :
	name(name)
{
}

Package::~Package()
{}

XMLObject
Package::xml() const
{
	XMLObject xml("rpm");
	xml.set_attr("name", name);
	xml.set_attr("summary", summary);
	xml.set_attr("description", description);
	xml.set_attr("version", version);
	xml.set_attr("repo_version", repo_version);
	return xml;
}

// #### class PackageSet ####

PackageSet::PackageSet() :
	installed(false),
	in_repo(false),
	upgradeable(false)
{
}

PackageSet::PackageSet(const String& name) :
	name(name),
	installed(false),
	in_repo(false),
	upgradeable(false)
{
}

PackageSet::~PackageSet()
{}

XMLObject
PackageSet::xml() const
{
	XMLObject xml("set");
	xml.set_attr("name", name);
	xml.set_attr("summary", summary);
	xml.set_attr("description", description);
	xml.set_attr("installed", installed ? "true" : "false");
	xml.set_attr("in_repository", in_repo ? "true" : "false");
	xml.set_attr("installable", upgradeable ? "true" : "false");
	return xml;
}

// #### class PackageHandler ####

PackageInstaller PackageHandler::_pi;

PackageHandler::PackageHandler()
{
	// get installed packages
	String out, err;
	int status;
	vector<String> args;

	args.push_back("-qa");
	if (utils::execute(RPM_PATH, args, out, err, status, false))
		throw command_not_found_error_msg(RPM_PATH);
	if (status != 0)
		throw String("rpm -qa failed: " + err);

	vector<String> lines = utils::split(out, "\n");
	for (vector<String>::const_iterator
			iter = lines.begin() ;
			iter != lines.end() ;
			iter++)
	{
		String line(*iter);
		line = utils::strip(line);

		vector<String> words = utils::split(line, "-");
		vector<String>::size_type l = words.size();
		if (l < 3)
			continue;
		String name = words[0];

		for (unsigned int i = 1; i < l - 2 ; i++)
			name += "-" + words[i];

		Package pack(name);
		pack.version = words[l - 2] + "-" + words[l - 1];
		_packages[name] = pack;
	}

	// probe repositories
	if (repo_available()) {
		map<String, String> avail_rpms = _pi.available_rpms();
		for (map<String, String>::const_iterator
			iter = avail_rpms.begin() ;
			iter != avail_rpms.end() ;
			iter++)
		{
			String name = iter->first;
			String version = iter->second;
			Package& pack = _packages[name];
			pack.name = name;
			pack.repo_version = version;
		}
	}

	// build sets
	_sets = build_sets();
	for (map<String, PackageSet>::iterator
			iter = _sets.begin() ;
			iter != _sets.end() ;
			iter++)
	{
		populate_set(iter->second);
	}
}

PackageHandler::~PackageHandler()
{}

std::map<String, Package>&
PackageHandler::packages()
{
	return _packages;
}

std::map<String, PackageSet>&
PackageHandler::sets()
{
	return _sets;
}

std::map<String, PackageSet>
PackageHandler::build_sets()
{
	map<String, PackageSet> sets;

	PackageSet set = build_cluster_base_set();
	sets[set.name] = set;

	set = build_cluster_services_set();
	sets[set.name] = set;

	set = build_cluster_storage_set();
	sets[set.name] = set;

	set = build_linux_virtual_server_set();
	sets[set.name] = set;

	return sets;
}

PackageSet
PackageHandler::build_cluster_base_set()
{
	PackageSet set("Cluster Base");

	set.packages.push_back("cman");
	set.packages.push_back("modcluster");

	return set;
}

PackageSet
PackageHandler::build_cluster_services_set()
{
	PackageSet set("Cluster Service Manager");
	set.packages.push_back("rgmanager");
	return set;
}

PackageSet
PackageHandler::build_cluster_storage_set()
{
	struct utsname uts;
	uname(&uts);
	String kernel(uts.release);

	PackageSet set("Clustered Storage");
	set.packages.push_back("lvm2-cluster");
	set.packages.push_back("sg3_utils");
	set.packages.push_back("gfs2-utils");
	set.packages.push_back("gfs-utils");

	return set;
}

PackageSet
PackageHandler::build_linux_virtual_server_set()
{
	PackageSet set("Linux Virtual Server");
	set.packages.push_back("ipvsadm");
	set.packages.push_back("piranha");
	return set;
}

void
PackageHandler::populate_set(PackageSet& set)
{
	set.installed = true;
	set.in_repo = true;
	set.upgradeable = false;

	for (list<String>::const_iterator
			name_iter = set.packages.begin() ;
			name_iter != set.packages.end() ;
			name_iter++)
	{
		const String& name = *name_iter;
		map<String, Package>::const_iterator iter = _packages.find(name);
		if (iter == _packages.end()) {
			set.installed = false;
			set.in_repo = false;
			set.upgradeable = false;
			break;
		} else {
			const Package& pack = iter->second;
			if (pack.version.empty())
				set.installed = false;
			if (pack.repo_version.empty())
				set.in_repo = false;
			else if (pack.repo_version > pack.version)
				set.upgradeable = true;
		}
	}

	if (set.in_repo == false)
		set.upgradeable = false;
}

void
PackageHandler::install(const std::list<Package>& packages,
						const std::list<PackageSet>& sets,
						bool upgrade)
{
	vector<String> rpms;

	PackageHandler h_pre;
	for (list<Package>::const_iterator
			iter = packages.begin() ;
			iter != packages.end() ;
			iter++)
	{
		String name(iter->name);
		map<String, Package>::iterator pack_iter = h_pre.packages().find(name);
		if (pack_iter == h_pre.packages().end()) {
			throw String("Package \"") + name
					+ "\" is present neither locally nor in any available repository";
		} else {
			String curr_ver(pack_iter->second.version);
			String repo_ver(pack_iter->second.repo_version);
			if (curr_ver.empty()) {
				// not installed
				if (repo_ver.empty()) {
					throw String("Package \"") + name
							+ "\" is not present in any available repository";
				} else
					rpms.push_back(name);
			} else {
				// already installed
				if (upgrade) {
					if (repo_ver.empty()) {
						throw String("Package \"") + name
								+ "\" is not present in any available repository";
					} else if (repo_ver > curr_ver)
						rpms.push_back(name);
				}
			}
		}
	}

	for (list<PackageSet>::const_iterator
			iter = sets.begin() ;
			iter != sets.end() ;
			iter++)
	{
		String name(iter->name);
		map<String, PackageSet>::iterator set_iter = h_pre.sets().find(name);

		if (set_iter == h_pre.sets().end()) {
			throw String("Packages of set \"") + name
					+ "\" are neither present neither locally nor in any available repository";
		} else {
			PackageSet& p_set = set_iter->second;
			if (p_set.installed) {
				// already installed
				if (upgrade) {
					if (p_set.in_repo) {
						if (p_set.upgradeable) {
							for (list<String>::const_iterator
									name_iter = p_set.packages.begin() ;
									name_iter != p_set.packages.end() ;
									name_iter++)
							{
								rpms.push_back(*name_iter);
							}
						} else {
							/* Packages are already up-to-date */
						}
					}
				}
			} else {
				// not installed
				if (p_set.in_repo) {
					for (list<String>::const_iterator
							name_iter = p_set.packages.begin() ;
							name_iter != p_set.packages.end() ;
							name_iter++)
					{
						rpms.push_back(*name_iter);
					}
				} else {
					throw String("Packages of set \"") + name +
						"\" are not present in any available repository";
				}
			}
		}
	}

	if (!_pi.install(rpms)) {
		String msg("Failed to install packages");
		if (!repo_available())
			msg += ": System not configured to use repositories";
		throw msg;
	}
}

bool
PackageHandler::repo_available()
{
	return _pi.available();
}
