/*
  Copyright Red Hat, Inc. 2002-2009
  Copyright Mission Critical Linux, 2000

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
/** @file
 * daemon_init function, does sanity checks and calls daemon().
 *
 * $Id: daemon_init.c,v 1.5 2008/02/19 03:14:11 rmccabe Exp $
 *
 * Author: Jeff Moyer <moyer@mclinux.com>
 */
/*
 * TODO: Clean this up so that only one function constructs the
 *		 pidfile /var/run/loggerd.PID, and perhaps only one function
 *		 forms the /proc/PID/ path.
 *
 *		Also need to add file locking for the pid file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/mman.h>
#include <sys/errno.h>
#include <libgen.h>
#include <signal.h>

/*
 * This should ultimately go in a header file.
 */
void daemon_init(char *prog);
int check_pid_valid(pid_t pid, char *prog);
int check_process_running(char *prog, pid_t * pid);

/*
 * Local prototypes.
 */
static void update_pidfile(char *prog);
static int setup_sigmask(void);


int
check_pid_valid(pid_t pid, char *prog)
{
	FILE *fp;
	DIR *dir;
	char filename[PATH_MAX];
	char dirpath[PATH_MAX];
	char proc_cmdline[64];	/* yank this from kernel somewhere */
	char *s = NULL;

	memset(filename, 0, PATH_MAX);
	memset(dirpath, 0, PATH_MAX);

	snprintf(dirpath, sizeof (dirpath), "/proc/%d", pid);
	if ((dir = opendir(dirpath)) == NULL) {
		closedir(dir);
		return 0;	/* Pid has gone away. */
	}
	closedir(dir);

	/*
	 * proc-pid directory exists.  Now check to see if this
	 * PID corresponds to the daemon we want to start.
	 */
	snprintf(filename, sizeof (filename), "/proc/%d/cmdline", pid);
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("check_pid_valid");
		return 0;	/* Who cares.... Let's boogy on. */
	}

	if (!fgets(proc_cmdline, sizeof (proc_cmdline) - 1, fp)) {
		/*
		 * Okay, we've seen processes keep a reference to a
		 * /proc/PID/stat file and not let go.  Then when
		 * you try to read /proc/PID/cmline, you get either
		 * \000 or -1.  In either case, we can safely assume
		 * the process has gone away.
		 */
		fclose(fp);
		return 0;
	}
	fclose(fp);

	s = &(proc_cmdline[strlen(proc_cmdline)]);
	if (*s == '\n')
		*s = 0;

	/*
	 * Check to see if this is the same executable.
	 */
	if ((s = strstr(proc_cmdline, prog)) == NULL) {
		return 0;
	} else {
		return 1;
	}
}


int
check_process_running(char *prog, pid_t * pid)
{
	pid_t oldpid;
	FILE *fp;
	char filename[PATH_MAX];
	char *cmd;
	int ret;
	struct stat st;

	*pid = -1;

	/*
	 * Now see if there is a pidfile associated with this cmd in /var/run
	 */
	fp = NULL;
	memset(filename, 0, PATH_MAX);

	cmd = basename(prog);
	snprintf(filename, sizeof (filename), "/var/run/%s.pid", cmd);

	ret = stat(filename, &st);
	if ((ret < 0) || (!st.st_size))
		return 0;

	/*
	 * Read the pid from the file.
	 */
	fp = fopen(filename, "r");
	if (fp == NULL) {	/* error */
		return 0;
	}
	fscanf(fp, "%d\n", &oldpid);
	fclose(fp);
	if (check_pid_valid(oldpid, cmd)) {
		*pid = oldpid;
		return 1;
	}
	return 0;
}


static void
update_pidfile(char *prog)
{
	FILE *fp = NULL;
	char *cmd;
	char filename[PATH_MAX];

	memset(filename, 0, PATH_MAX);

	cmd = basename(prog);
	snprintf(filename, sizeof (filename), "/var/run/%s.pid", cmd);

	fp = fopen(filename, "w");
	if (fp == NULL) {
		exit(1);
	}

	fprintf(fp, "%d", getpid());
	fclose(fp);
}


static int
setup_sigmask(void)
{
	sigset_t set;

	sigfillset(&set);

	/*
	 * Dont't block signals which would cause us to dump core.
	 */
	sigdelset(&set, SIGQUIT);
	sigdelset(&set, SIGILL);
	sigdelset(&set, SIGTRAP);
	sigdelset(&set, SIGABRT);
	sigdelset(&set, SIGFPE);
	sigdelset(&set, SIGSEGV);
	sigdelset(&set, SIGBUS);

	return (sigprocmask(SIG_BLOCK, &set, NULL));
}


void
daemon_init(char *prog)
{
	uid_t uid;
	pid_t pid;

	uid = getuid();
	if (uid) {
		fprintf(stderr,
			"daemon_init: Sorry, only root wants to run this.\n");
		exit(1);
	}

	if (check_process_running(prog, &pid) && (pid != getpid())) {
		fprintf(stderr,
			"daemon_init: Process \"%s\" already running.\n",
			prog);
		exit(1);
	}
	if (setup_sigmask() < 0) {
		fprintf(stderr, "daemon_init: Unable to set signal mask.\n");
		exit(1);
	}

	daemon(0, 0);

	update_pidfile(prog);
	nice(-1);
}
