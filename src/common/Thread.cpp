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

#include "Thread.h"
//#include "Logger.h"
#include "String.h"

void*
start_thread(void *thread_obj)
{
	try {
		((Thread*) thread_obj)->run();
	}
	catch ( ... ) {}
	return NULL;
}

Thread::Thread() :
	_stop(true),
	_running(false)
{}

Thread::~Thread()
{
	//log(String("entered destructor of thread ") + (int) _thread, LogThread);
	Thread::stop();
}

void
Thread::start()
{
	//log("entered Thread::start()", LogThread);
	MutexLocker l1(_main_mutex);
	if (!_running) {
		{
			MutexLocker l2(_stop_mutex);
			_stop = false;
		}

		pthread_attr_t a;
		pthread_attr_init(&a);
		pthread_attr_setstacksize(&a, (1 << 18));
		int ret = pthread_create(&_thread, &a, start_thread, this);
		pthread_attr_destroy(&a);
		if (ret == 0)
			_running = true;
		else
			throw String("Error starting thread: ") + String(strerror(ret));
		//log(String("created thread ") + (int) _thread, LogThread);
	}
}

void
Thread::stop()
{
	//log(String("entered Thread::stop() for thread ") + (int) _thread, LogThread);
	MutexLocker l1(_main_mutex);
	if (_running) {
		{
			//log(String("Thread::stop(): locking stop mutex for thread ") + (int) _thread, LogThread);
			MutexLocker l2(_stop_mutex);
			_stop = true;
		}
		//log(String("entering pthread_join() for thread ") + (int) _thread, LogThread);

		if (pthread_join(_thread, NULL))
			throw String("error stopping thread");
		//log(String("stopped thread ") + (int) _thread, LogThread);
		_running = false;
	}
}

bool
Thread::running()
{
	//log(String("entered Thread::running() for thread ") + (int) _thread, LogThread);
	MutexLocker l1(_main_mutex);
	bool ret = _running;
	return ret;
}

bool
Thread::shouldStop()
{
	//log(String("entered Thread::shouldStop() for thread ") + (int) _thread, LogThread);
	MutexLocker l(_stop_mutex);
	bool ret = _stop;
	return ret;
}
