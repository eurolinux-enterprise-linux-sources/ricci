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


#ifndef __CONGA_RICCI_CLIENTINSTANCE_H
#define __CONGA_RICCI_CLIENTINSTANCE_H

#include "DBusController.h"
#include "Socket.h"
#include "Thread.h"
#include "Mutex.h"
#include "SSLInstance.h"
#include "XML.h"

class ClientInstance : public Thread
{
	public:
		ClientInstance(ClientSocket sock, DBusController& dbus_controller);
		virtual ~ClientInstance();
		virtual bool done();

	protected:
		virtual void run();

	private:
		SSLInstance _ssl;
		DBusController& _dbus_controller;
		Mutex _mutex;
		bool _done;

		XMLObject receive();
		void send(const XMLObject& msg);
		void encrypt_begin();
};

#endif
