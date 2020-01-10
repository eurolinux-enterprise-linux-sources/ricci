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

#include "executils.h"
#include "executils_test.h"
#include "Mutex.h"
#include "Time.h"
#include "Logger.h"
#include "counting_auto_ptr.h"

extern "C" {
#include "signals.h"
#include <signal.h>
}

#include <map>
#include <vector>
#include "String.h"
#include <iostream>
#include <sys/poll.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;
using namespace ClusterMonitoring;

static void nothing(int);
static void nothing_2(int);

Mutex mutex;
bool running = true;
pid_t pid;

int
main(int argc, char **argv)
{
	//set_logger(counting_auto_ptr<Logger>(new Logger(1, "executils_test", LogExecute)));

	map<String, vector<String> > oracle;
	oracle["/bin/cat"] = vector<String>(1, "/etc/cluster/cluster.conf");
	oracle["/sbin/magma_tool"] = vector<String>(1, "members");
	oracle["/usr/sbin/clustat"] = vector<String>(1, "-x");
	oracle["/sbin/ifconfig"] = vector<String>();
	//oracle["/bin/sleep"] = vector<String>(1, "4");

	setup_signal(SIGUSR1, nothing);
	setup_signal(SIGUSR2, nothing_2);

	pid = getpid();

	Killer killer;
	killer.start();

	while (true) {
		for (map<String, vector<String> >::iterator
			iter = oracle.begin() ;
			iter != oracle.end() ;
			iter++)
		{
			String out, err;
			int status;

			execute(iter->first, iter->second, out, err, status);

			//cout << iter->first << ":" << endl;
			//cout << out << endl << endl << err << endl << endl;
			MutexLocker l(mutex);
			running = true;
		}
		//poll(NULL, 0, 10);
	}
}

void
nothing(int)
{}

void
nothing_2(int)
{
	poll(NULL, 0, rand() % 20);
}

void
Killer::run()
{
	int time_to_check = time_sec() + 10;

	srand(time_sec());
	while (!shouldStop()) {
		kill(pid, SIGUSR2);
		poll(NULL, 0, rand() % 117 + 20);

		kill(pid, SIGUSR1);
		poll(NULL, 0, rand() % 43 + 20);

		MutexLocker l(mutex);
		if (time_sec() > time_to_check) {
			if (!::running) {
				cout << "hosed :)" << endl;
				return;
			}
			::running = false;
			time_to_check = time_sec() + 10;
		}
	}
}
