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

#include "counting_auto_ptr.h"

template<class X>
counting_auto_ptr<X>::counting_auto_ptr(X *ptr) :
	_ptr(ptr)
{
	try {
		_counter = new int(1);
	} catch ( ... ) {
		delete _ptr;
		throw;
	}

	try {
		_mutex = new Mutex();
	} catch ( ... ) {
		delete _ptr;
		delete _counter;
		throw;
	}
};

template<class X>
counting_auto_ptr<X>::counting_auto_ptr(const counting_auto_ptr<X>& o)
{
	MutexLocker l(*(o._mutex));
	_ptr = o._ptr;
	_mutex = o._mutex;
	_counter = o._counter;
	(*_counter)++;
};

template<class X>
counting_auto_ptr<X>&
counting_auto_ptr<X>::operator= (const counting_auto_ptr<X>& o)
{
	if (&o != this) {
		decrease_counter();
		MutexLocker l(*(o._mutex));
		_ptr = o._ptr;
		_mutex = o._mutex;
		_counter = o._counter;
		(*_counter)++;
	}
	return *this;
};

template<class X>
counting_auto_ptr<X>::~counting_auto_ptr()
{
	decrease_counter();
};

template<class X>
void
counting_auto_ptr<X>::decrease_counter()
{
	bool last = false;
	{
		MutexLocker l(*_mutex);
		last = (--(*_counter) == 0);
		if (*_counter < 0)
			throw int();
	}

	if (last) {
		delete _counter;
		delete _ptr;
		delete _mutex;
	}
};


template<class X>
X&
counting_auto_ptr<X>::operator*() const
{
	return *_ptr;
};

template<class X>
X*
counting_auto_ptr<X>::operator->() const
{
	return _ptr;
};

template<class X>
X*
counting_auto_ptr<X>::get() const
{
	return _ptr;
};
