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
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */

#include "Server.h"
#include "ClientInstance.h"
#include "Ricci.h"

extern "C" {
	#include "signals.h"
	#include <signal.h>

	void daemon_init(char *prog);
}

#include <sys/poll.h>
#include <errno.h>

typedef struct pollfd poll_fd;

#include <list>
#include <iostream>

using namespace std;

static time_t last_purge = 0;
static bool shutdown_pending = false;
static void shutdown(int);

Server::Server(const ServerSocket& serv_sock) :
	_server(serv_sock)
{
	_server.nonblocking(true);
}

Server::~Server()
{}

void
Server::run()
{
	// restart unfinished jobs
	Batch::restart_batches();

	// handle clients
	list<counting_auto_ptr<ClientInstance> > clients;

	setup_signal(SIGINT, shutdown);
	setup_signal(SIGTERM, shutdown);
	setup_signal(SIGPIPE, SIG_IGN);
	unblock_signal(SIGSEGV);

	while (!shutdown_pending) {
		poll_fd poll_data;
		poll_data.fd = _server.get_sock();
		poll_data.events = POLLIN;
		poll_data.revents = 0;

		// wait for events
		int ret = poll(&poll_data, 1, 1000);
		time_t cur_time = time(NULL);

		if (ret == 0 || cur_time - last_purge >= 2) {
			last_purge = cur_time;
			// clean up clients

			list<list<counting_auto_ptr<ClientInstance> >::iterator> remove_us;
			for (list<counting_auto_ptr<ClientInstance> >::iterator
					iter = clients.begin() ;
					iter != clients.end() ;
					iter++)
			{
				if ((*iter)->done())
					remove_us.push_back(iter);
			}

			for (list<list<counting_auto_ptr<ClientInstance> >::iterator>::iterator
					iter = remove_us.begin() ;
					iter != remove_us.end() ;
					iter++)
			{
				clients.erase(*iter);
				cerr << "client removed" << endl;
			}
		}

		if (ret == 0) {
			// continue waiting
			continue;
		} else if (ret == -1) {
			if (errno == EINTR)
				continue;
			else
				throw String("poll() error: " + String(strerror(errno)));
		}

		// process events
		if (poll_data.revents & POLLIN) {
			try {
				ClientSocket sock = _server.accept();
				counting_auto_ptr<ClientInstance> client(new ClientInstance(sock, _dbus_controller));

				client->start();
				clients.push_back(client);
				cerr << "client added" << endl;
			} catch ( String e ) {
				cerr	<< __FILE__ << ":" << __LINE__ << ": "
						<< "exception: " << e << endl;
			} catch ( ... ) {}
		}

		if (poll_data.revents & (POLLERR | POLLHUP | POLLNVAL))
			throw String("server socket error");
	}
}

void
shutdown(int)
{
	shutdown_pending = true;
}
