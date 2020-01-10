/*
** Copyright (C) Red Hat, Inc. 2006-2009
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

#ifndef __CONGA_RICCI_REBOOTMODULE_H
#define __CONGA_RICCI_REBOOTMODULE_H

#include "Module.h"
#include "DBusController.h"

class RebootModule : public Module
{
	public:
		RebootModule(DBusController& dbus);
		virtual ~RebootModule();
		virtual XMLObject process(const XMLObject& request);
		bool block();

	private:
		DBusController& _dbus;
};

#endif
