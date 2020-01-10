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

#ifndef __CONGA_MODRPM_PACKAGEHANDLER_H
#define __CONGA_MODRPM_PACKAGEHANDLER_H

#include "XML.h"
#include "String.h"
#include <map>
#include <vector>

class Package
{
	public:
		Package();
		Package(const String& name);
		virtual ~Package();

		String name;
		String summary;
		String description;
		String version;
		String repo_version;

		XMLObject xml() const;

	private:
};

class PackageSet
{
	public:
		PackageSet();
		PackageSet(const String& name);
		virtual ~PackageSet();

		String name;
		String summary;
		String description;
		bool installed;
		bool in_repo;
		bool upgradeable;

		std::list<String> packages;

		XMLObject xml() const;

	private:
};

class PackageInstaller
{
	public:
		PackageInstaller();
		virtual ~PackageInstaller();

		bool available();
		std::map<String, String> available_rpms();
		bool install(std::vector<String> rpm_names);
};

class PackageHandler
{
	public:
		PackageHandler();
		virtual ~PackageHandler();

		std::map<String, Package>& packages();
		std::map<String, PackageSet>& sets();

		void populate_set(PackageSet& set);

		static bool repo_available();

		static std::map<String, PackageSet> build_sets();
		static PackageSet build_cluster_base_set();
		static PackageSet build_cluster_base_gulm_set();
		static PackageSet build_cluster_services_set();
		static PackageSet build_cluster_storage_set();
		static PackageSet build_linux_virtual_server_set();

		static void install(const std::list<Package>& packages,
							const std::list<PackageSet>& sets,
							bool upgrade=true);

	private:
		std::map<String, Package> _packages;
		std::map<String, PackageSet> _sets;
		static PackageInstaller _pi;
};

#endif
