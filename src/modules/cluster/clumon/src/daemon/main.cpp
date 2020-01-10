/*
** Copyright (C) Red Hat, Inc. 2005-2008
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

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>

#include <sys/poll.h>
#include <errno.h>
#include <libcman.h>

typedef struct pollfd poll_fd;

extern "C" {
#include "signals.h"
void daemon_init(char *prog);
}

#include "clumond_globals.h"
#include "Monitor.h"
#include "Socket.h"
#include "array_auto_ptr.h"
#include "Logger.h"

#include <map>
#include <iostream>


using namespace ClusterMonitoring;
using namespace std;


static bool shutdown_pending = false;
static void shutdown(int);
static void segfault(int);
static void sigchild(int);
static void serve_clients(Monitor& monitor, ServerSocket& server);

class ClientInfo
{
	public:
		ClientInfo() {}
		ClientInfo(ClientSocket& sock, String str="") :
			sock(sock), str(str) {}

		ClientSocket sock;
		String str;
};

int
main(int argc, char **argv)
{
	bool debug = false, foreground = false;
	int v_level = -1;
	int rv;
	String clu_version("");

	while ((rv = getopt(argc, argv, "c:fdv:")) != EOF) {
		switch (rv) {
			case 'c': {
				char *p;
				long cv = strtol(optarg, &p, 10);
				if (*p != '\0' || cv < 3 || cv > 5) {
					fprintf(stderr, "Invalid cluster version: %s\n", optarg);
					exit(-1);
				}
				clu_version = String(optarg);
			}

			case 'd':
				debug = true;
				break;

			case 'f':
				foreground = true;
				break;

			case 'v':
				if (sscanf(optarg, "%d", &v_level) != 1) {
					fprintf(stderr, "Invalid verbosity level: %s\n", optarg);
					v_level = -1;
				}
				break;

			default:
				break;
		}
	}

	if (v_level < 0) {
		if (debug) {
			cout << "Setting verbosity level to LogAll" << endl;
		} else {
			cout << "Setting verbosity level to LogBasic" << endl;
			v_level = LogBasic;
		}
	}

	if (foreground)
		set_logger(counting_auto_ptr<Logger>(new Logger(1, "clumond", LogLevel(v_level))));
	else
		set_logger(counting_auto_ptr<Logger>(new Logger(LOG_FILE, "clumond", LogLevel(v_level))));

	log("started");
	try {
		ServerSocket server(MONITORING_CLIENT_SOCKET);
		server.nonblocking(true);

		Monitor monitor(COMMUNICATION_PORT, clu_version);

		if (!foreground && (geteuid() == 0))
			daemon_init(argv[0]);

		setup_signal(SIGINT, shutdown);
		setup_signal(SIGTERM, shutdown);
		setup_signal(SIGCHLD, sigchild);
		setup_signal(SIGPIPE, SIG_IGN);

		if (debug)
			setup_signal(SIGSEGV, segfault);
		else
			unblock_signal(SIGSEGV);

		serve_clients(monitor, server);
	} catch (String e) {
		log("unhandled exception in main(): " + e);
		log("died", LogAll);
		return 1;
	} catch ( ... ) {
		log("unhandled unknown exception in main()");
		log("died", LogAll);
		return 1;
	}

	unlink("/var/run/clumond.pid");
	log("exited", LogAll);
	return 0;
}

void
serve_clients(Monitor& monitor, ServerSocket& server)
{
	map<int, ClientInfo> clients;

	log("Starting monitor", LogMonitor);
	monitor.start();

	while (!shutdown_pending) {
		unsigned int socks_num = clients.size() + 1;

		// prepare poll structs
		array_auto_ptr<poll_fd> poll_data(new poll_fd[socks_num]);
		poll_data[0].fd = server.get_sock();
		poll_data[0].events = POLLIN;
		poll_data[0].revents = 0;

		map<int, ClientInfo>::iterator iter = clients.begin();
		for (unsigned int i = 1 ; i < socks_num ; i++) {
			poll_data[i].fd = iter->first;
			poll_data[i].events = POLLIN;
			if (!iter->second.str.empty())
				poll_data[i].events |= POLLOUT;
			poll_data[i].revents = 0;
			iter++;
		}

		// wait for events
		int ret = poll(poll_data.get(), socks_num, 500);
		if (ret == 0)
			continue;
		else if (ret == -1) {
			if (errno == EINTR)
				continue;
			else {
				throw String("serve_clients(): poll() error: "
						+ String(strerror(errno)));
			}
		}

		// process events
		for (unsigned int i = 0 ; i < socks_num && !shutdown_pending ; i++) {
			poll_fd& poll_info = poll_data[i];

			// server socket
			if (poll_info.fd == server.get_sock()) {
				if (poll_info.revents & POLLIN) {
					try {
						ClientSocket sock = server.accept();
						sock.nonblocking(true);
						clients[sock.get_sock()] = ClientInfo(sock);
					} catch ( ... ) {}
				}
				if (poll_info.revents & (POLLERR | POLLHUP | POLLNVAL)) {
					throw String("serve_clients(): poll: " +
							String(strerror(errno)));
				}
			} else {
				// client socket
				if (poll_info.revents & POLLIN) {
					ClientInfo& info = clients[poll_info.fd];
					try {
						String msg = info.sock.recv();
						if (msg.size())
							info.str = monitor.request(msg);
					} catch ( ... ) {
						clients.erase(poll_info.fd);
					}
					continue;
				}

				if (poll_info.revents & (POLLERR | POLLHUP | POLLNVAL)) {
					clients.erase(poll_info.fd);
					continue;
				}

				if (poll_info.revents & POLLOUT) {
					ClientInfo& info = clients[poll_info.fd];
					try {
						info.str = info.sock.send(info.str);
					} catch ( ... ) {
						clients.erase(poll_info.fd);
						continue;
					}
				}
			}
		}
	}
}

void
shutdown(int)
{
	log_sigsafe("exit requested", LogExit);
	shutdown_pending = true;
}

void
segfault(int)
{
	char msg[128];

	snprintf(msg, sizeof(msg), "PID %d Thread %d: SIGSEGV, waiting forensics",
		getpid(), (int) pthread_self());
	log_sigsafe(msg, LogAll);
	select(0, NULL, NULL, NULL, NULL);
}

void
sigchild(int)
{
	// do nothing
}
