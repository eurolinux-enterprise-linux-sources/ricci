/*
** Copyright (C) Red Hat, Inc. 2005-2009
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
 * Authors:
 *  Stanko Kupcevic <kupcevic@redhat.com>
 *  Ryan McCabe <rmccabe@redhat.com>
 */

#include "ClusterConf.h"
#include "utils.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

extern "C" {
#include <libcman.h>
}

#include <fstream>
using namespace std;

#define CLUSTER_CONF_DIR		String("/etc/cluster/")
#define CLUSTER_CONF_NAME		String("cluster.conf")
#define CLUSTER_CONF_PATH		(CLUSTER_CONF_DIR + CLUSTER_CONF_NAME)

XMLObject
ClusterConf::get()
{
	return readXML(CLUSTER_CONF_PATH);
}

void
ClusterConf::set_version(int conf_version) {
	cman_version_t cman_version;
	cman_handle_t cman_handle;
	int ret;

	if (conf_version < 1) {
		char err[128];
		snprintf(err, sizeof(err), "error updating configuration version: invalid version number provided: %d", conf_version);
		throw String(err);
	} 

	cman_handle = cman_admin_init(NULL);
	if (cman_handle != NULL) {
		if (cman_get_version(cman_handle, &cman_version) < 0) {
			cman_finish(cman_handle);
			throw String("error updating configuration version: could not retrieve current version");
		}

		cman_version.cv_config = conf_version;
		ret = cman_set_version(cman_handle, &cman_version);
		cman_finish(cman_handle);
		if (ret < 0) {
			throw String("error updating configuration version");
		}
	} else {
		throw String("error updating configuration version: cannot create admin connection to cman");
	}
}

void
ClusterConf::set(const XMLObject& xml, bool propagate) {
	char cconf_path[] = "/etc/cluster/cluster.conf.tmp.ricciXXXXXX";
	int err = 0;

	// sanity check
	if (xml.tag() != "cluster")
		throw String("invalid cluster.conf: no cluster tag");
	if (xml.get_attr("name").empty())
		throw String("invalid cluster.conf: no cluster name attribute");

	long long conf_version = utils::to_long(xml.get_attr("config_version"));
	if (conf_version == 0)
		throw String("invalid cluster.conf: no config_version attribute");

	// create dir, if it doesn't exist
	DIR *dir = opendir(CLUSTER_CONF_DIR.c_str());
	if (dir)
		closedir(dir);
	else {
		if (errno == ENOENT) {
			if (mkdir(CLUSTER_CONF_DIR.c_str(), 0755))
				throw String("failed to create " + CLUSTER_CONF_DIR
						+ ": " + String(strerror(errno)));
			} else
				throw String("opendir() error: ") + String(strerror(errno));
	}

	mode_t old_umask = umask(0027);
	int conf_fd = mkstemp(cconf_path);
	err = errno;
	umask(old_umask);

	if (conf_fd < 0) {
		throw String("error creating temporary cluster.conf: "
				+ String(strerror(err)));
	}

	String conf_xml(generateXML(xml));
	ssize_t ret = write(conf_fd, conf_xml.c_str(), conf_xml.size());
	err = errno;
	fchmod(conf_fd, 0640);
	close(conf_fd);

	if (ret != (ssize_t) conf_xml.size()) {
		throw String("error creating temporary cluster.conf: "
				+ String(strerror(err)));
	}

	if (rename(cconf_path, CLUSTER_CONF_PATH.c_str())) {
		int errno_saved = errno;
		unlink(cconf_path);
		throw String("failed to rename cluster.conf: ")
			+ String(strerror(errno_saved));
	}

	if (propagate) {
		try {
			ClusterConf::set_version(conf_version);
		} catch (String e) {
			unlink(cconf_path);
			throw (e);
		}
	}
}

void ClusterConf::purge_conf() {
	int ret;
	do {
		ret = unlink(CLUSTER_CONF_PATH.c_str());
	} while (ret == -1 && errno == EINTR);

	if (ret != 0)
		throw String("Unable to delete " + CLUSTER_CONF_PATH + ": " + String(strerror(errno)));
}
