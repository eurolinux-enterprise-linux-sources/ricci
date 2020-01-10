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

#include "executils.h"
#include "Logger.h"
#include "Time.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

static void read_data(struct pollfd& poll_info, bool& fd_closed, String& data);
static void close_fd(int fd);

int
execute(const String& path,
		const std::vector<String>& args,
		String& out,
		String& err,
		int& status,
		int timeout)
{
	if (access(path.c_str(), X_OK))
		return 1;

	out = err = "";

	int _stdout_pipe[2];
	int _stderr_pipe[2];

	/*
	** Prepare argv before forking in case we were called from a thread.
	*/
	unsigned int size = args.size() + 2;
	char **argv = (char **) malloc(sizeof(char *) * size);
	if (argv == NULL)
		return 3;

	try {
		argv[0] = (char *) path.c_str();
		for (unsigned int i = 0 ; i < args.size() ; i++)
			argv[i + 1] = (char *) args[i].c_str();
		argv[size - 1] = NULL;
	} catch (...) {
		free(argv);
		return 3;
	}

	if (pipe(_stdout_pipe) == -1) {
		free(argv);
		return 2;
	}

	if (pipe(_stderr_pipe) == -1) {
		free(argv);
		close_fd(_stdout_pipe[0]);
		close_fd(_stdout_pipe[1]);
		return 2;
	}

	setenv("LANG", "C", 1);
	setenv("LC_ALL", "C", 1);

	int pid = fork();
	if (pid == -1) {
		free(argv);
		close_fd(_stdout_pipe[0]);
		close_fd(_stdout_pipe[1]);
		close_fd(_stderr_pipe[0]);
		close_fd(_stderr_pipe[1]);
		return 3;
	}

	if (pid == 0) {
		/* child */

		close_fd(1);
		close_fd(_stdout_pipe[0]);
		dup2(_stdout_pipe[1], 1);
		close_fd(_stdout_pipe[1]);

		close_fd(2);
		close_fd(_stderr_pipe[0]);
		dup2(_stderr_pipe[1], 2);
		close_fd(_stderr_pipe[1]);

		int devnull = open("/dev/null", O_RDWR);
		if (devnull == -1) {
			fprintf(stderr, "ricci::execute(): Can't open /dev/null: %s\n",
				strerror(errno));
			_exit(1);
		}

		close_fd(0);
		dup2(devnull, 0);
		close_fd(devnull);

		// close open fds
		for (unsigned int i = 3 ; i < __FD_SETSIZE ; i++)
			close_fd(i);

		// restore signals
		for (int x = 1 ; x < _NSIG ; x++)
			signal(x, SIG_DFL);

		sigset_t set;
		sigfillset(&set);
		sigprocmask(SIG_UNBLOCK, &set, NULL);

		execv(path.c_str(), argv);
		_exit(1);
	}

	/* parent */
	unsigned int time_beg = time_mil();
	unsigned int time_to_kill = time_beg + timeout;
	bool out_closed = false, err_closed = false;

	free(argv);
	close_fd(_stdout_pipe[1]);
	close_fd(_stderr_pipe[1]);

	while (true) {
		// kill child if timeout elapsed
		if (timeout >= 0 && time_mil() > time_to_kill) {
			kill(pid, SIGTERM);
			sleep(1);
			kill(pid, SIGKILL);
		}

		// prepare poll structs
		struct pollfd poll_data[2];
		int s = 0;
		if (!out_closed) {
			poll_data[s].fd = _stdout_pipe[0];
			poll_data[s].events = POLLIN;
			poll_data[s].revents = 0;
			s += 1;
		}

		if (!err_closed) {
			poll_data[s].fd = _stderr_pipe[0];
			poll_data[s].events = POLLIN;
			poll_data[s].revents = 0;
			s += 1;
		}

		if (s == 0)
			break;

		// wait for events
		int ret = poll(poll_data, s, 500);
		if (ret == 0)
			continue;
		else if (ret == -1) {
			if (errno == EINTR)
				continue;
			else {
				if (!out_closed)
					close_fd(_stdout_pipe[0]);
				if (!err_closed)
					close_fd(_stderr_pipe[0]);
				return 4;
			}
		}

		// process events
		for (int i = 0 ; i < s ; i++) {
			struct pollfd& poll_info = poll_data[i];

			if (poll_info.fd == _stdout_pipe[0])
				read_data(poll_info, out_closed, out);
			if (poll_info.fd == _stderr_pipe[0])
				read_data(poll_info, err_closed, err);
		}
	}

	// command
	//String comm(path);
	//for (unsigned int i = 0 ; i < args.size() ; i++)
	//	comm += " " + args[i];

	// get status
	int ret;
	do {
		ret = waitpid(pid, &status, 0);
	} while ((ret < 0) && (errno == EINTR));

	if (WIFEXITED(status)) {
		//log("executed \"" + comm + "\" in " + (time_mil() - time_beg) + " milliseconds", LogExecute);
		status = WEXITSTATUS(status);
		return 0;
	} else if (WIFSIGNALED(status)) {
		//log("\"" + comm + "\" killed after " + (time_mil() - time_beg) + " milliseconds", LogExecute);
		return 5;
	} else {
		//log("unknown cause of \"" + comm + "\"'s exit after " + (time_mil() - time_beg) + " milliseconds", LogExecute);
		return 6;
	}
}

void
read_data(struct pollfd& poll_info, bool& fd_closed, String& data)
{
	int fd = poll_info.fd;

	if (poll_info.revents & POLLIN) {
		try {
			char data_in[4096];
			int ret = read(fd, data_in, sizeof(data_in));
			if (ret < 0)
				return;
			if (ret == 0) {
				close_fd(fd);
				fd_closed = true;
				return;
			}
			data.append(data_in, ret);
			if (ret == sizeof(data_in))
				return;
		} catch ( ... ) {
			close_fd(fd);
			fd_closed = true;
		}
	}

	if (poll_info.revents & (POLLERR | POLLHUP | POLLNVAL)) {
		close_fd(fd);
		fd_closed = true;
		return;
	}
}

void
close_fd(int fd)
{
	int e;
	do {
		e = close(fd);
	} while (e && (errno == EINTR));
}

/*
#include <iostream>
using namespace std;

int
main(int argc, char** argv)
{
	String out, err;
	int status;
	vector<String> arguments;

	String path;
	if (argc < 2) {
		cout << "enter path to execute: ";
		cin >> path;
	} else
		path = argv[1];

	for (int i=2; i<argc; i++)
		arguments.push_back(argv[i]);

	cout << "executing " << path << endl;
	cout << execute(path, arguments, out, err, status) << endl;

	cout << "stdout:" << endl << out << endl << endl;
	cout << "stderr:" << endl << err << endl << endl;
	cout << "status: " << status << endl;
}
*/
