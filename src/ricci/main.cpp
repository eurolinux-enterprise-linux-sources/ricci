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
**
** Authors:
**  Stanko Kupcevic <kupcevic@redhat.com>
**  Ryan McCabe <rmccabe@redhat.com>
*/

#include "Server.h"
#include "ricci_defines.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/capability.h>
#include <sys/prctl.h>


extern "C" {
	void daemon_init(char *prog);
}

#include <iostream>


using namespace std;

bool foreground = false;
bool debug = false;
bool advertise_cluster = false;
bool self_fence = false;

void print_help(void) {
	printf("Usage: ricci [options]\n\
   -c         Announce cluster name and OS to unauthenticated clients.\n\
   -d         Enable debugging.\n\
   -f         Run in the foreground.\n\
   -F         Enable support for commands that allow clients to force\n\
              a reboot (same effect as /sbin/reboot -fn) as an alternative\n\
              to proper fencing.\n\
   -h         Print this help dialog.\n\
   -p <port>  Listen for connections on the specified port.\n\
   -u <user>   Drop root and run with the specified uid.\n");
}

int drop_privs(uid_t new_uid) {
	int ret;
	cap_value_t saved_caps[] = { CAP_SYS_BOOT, CAP_SETUID };

	/*
	** Keep the capability to reboot the system for when
	** fencing support is requested (via -F).
	*/
	cap_value_t saved_caps_final[] = { CAP_SYS_BOOT };
	cap_t caps = cap_init();

	if (caps == NULL)
		return (-errno);

	if (prctl(PR_SET_KEEPCAPS, 1) != 0)
		goto out_err;

	if (cap_clear(caps) != 0)
		goto out_err;

	ret = cap_set_flag(caps, CAP_PERMITTED,
			sizeof(saved_caps) / sizeof(saved_caps[0]), saved_caps, CAP_SET);
	if (ret != 0)
		goto out_err;

	ret = cap_set_flag(caps, CAP_EFFECTIVE,
			sizeof(saved_caps) / sizeof(saved_caps[0]), saved_caps, CAP_SET);
	if (ret != 0)
		goto out_err;

	if (cap_set_proc(caps) != 0)
		goto out_err;

	/* We don't drop gid=0 so we can read the cluster.conf file. */
	if (setuid(new_uid) != 0)
		goto out_err;

	if (cap_clear(caps) != 0)
		goto out_err;

	ret = cap_set_flag(caps, CAP_PERMITTED,
			sizeof(saved_caps_final) / sizeof(saved_caps_final[0]),
			saved_caps_final, CAP_SET);
	if (ret != 0)
		goto out_err;

	ret = cap_set_flag(caps, CAP_EFFECTIVE,
			sizeof(saved_caps_final) / sizeof(saved_caps_final[0]),
			saved_caps_final, CAP_SET);
	if (ret != 0)
		goto out_err;

	if (cap_set_proc(caps) != 0)
		goto out_err;

	cap_free(caps);
	prctl(PR_SET_KEEPCAPS, 0);
	return (0);

out_err:
	cap_free(caps);
	return (-errno);
}

int main(int argc, char **argv) {
	uint32_t uid = 0;
	int32_t ricci_port = RICCI_SERVER_PORT;
	int ret;

	while ((ret = getopt(argc, argv, "cdfFhu:p:")) != EOF) {
		switch (ret) {
			case 'c':
				advertise_cluster = true;
				break;

			case 'd':
				debug = true;
				break;

			case 'f':
				foreground = true;
				break;

			case 'F':
				self_fence = true;
				break;

			case 'h':
				print_help();
				exit(0);

			case 'p':
				if (optarg != NULL) {
					uint32_t port;
					char *e = NULL;

					port = strtoul(optarg, &e, 10);
					if (*e != '\0' || !port || (port & 0xffff) != port) {
						fprintf(stderr, "Invalid port: %s\n", optarg);
						exit(1);
					}
					ricci_port = port;
				}
				break;

			case 'u':
				if (optarg != NULL) {
					char *e = NULL;
					uint32_t temp_uid;

					temp_uid = strtoul(optarg, &e, 10);
					if (*e == '\0')
						uid = temp_uid;
					else {
						struct passwd *pw = getpwnam(optarg);
						if (pw != NULL)
							uid = pw->pw_uid;
						else {
							fprintf(stderr, "Invalid user: %s\n", optarg);
							exit(1);
						}
					}
				}
				break;

			default:
				break;
		}
	}

	if (geteuid() != 0) {
		fprintf(stderr, "You must be root to run this program.\n");
		exit(1);
	}

	try {
		ServerSocket serv_sock(ricci_port);

		if (!foreground)
			daemon_init(argv[0]);

		if (self_fence)
			mlockall(MCL_CURRENT);

		if (uid != getuid()) {
			if (self_fence)
				ret = drop_privs(uid);
			else {
				ret = setuid(uid);
				if (ret != 0)
					ret = -errno;
			}

			if (ret != 0) {
				fprintf(stderr, "Error changing uid to %u: %s\n",
					uid, strerror(-ret));
				exit(1);
			}
		}

		Server server(serv_sock);
		server.run();
	} catch ( String e ) {
		cerr	<< __FILE__ << ":" << __LINE__ << ": "
				<< "exception: " << e << endl;
		exit(1);
	} catch ( ... ) {
		cerr	<< __FILE__ << ":" << __LINE__ << ": "
				<< "exception caught" << endl;
		exit(1);
	}

	exit(0);
}
