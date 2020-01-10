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

#ifndef __CONGA_THREAD_H
#define __CONGA_THREAD_H

#include <pthread.h>
#include "Mutex.h"

class Thread
{
	public:
		Thread();
		virtual ~Thread();

		// not to be called from run()
		virtual void start();
		virtual void stop();
		virtual bool running();

	protected:
		// kids, return from run() if true, check it often
		virtual bool shouldStop();

		// run in new thread
		virtual void run() = 0;

	private:
		bool _stop;
		bool _running;
		pthread_t _thread;
		Mutex _stop_mutex;
		Mutex _main_mutex;

		Thread(const Thread&);
		Thread& operator= (const Thread&);

	friend void *start_thread(void *);
};

#endif
