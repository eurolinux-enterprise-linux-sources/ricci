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

#include "RebootModule.h"

using namespace std;

// potential bug, if there are two different dbuss in use
static DBusController* dbus = NULL;
static bool block = false;

static VarMap reboot(const VarMap& args);
static ApiFcnMap build_fcn_map();

RebootModule::RebootModule(DBusController& dbus) :
	Module(build_fcn_map()),
	_dbus(dbus)
{
	::dbus = &_dbus;
}

RebootModule::~RebootModule()
{}

XMLObject
RebootModule::process(const XMLObject& request)
{
	return this->Module::process(request);
}

bool
RebootModule::block()
{
	return ::block;
}

ApiFcnMap
build_fcn_map()
{
	FcnMap api_1_0;
	api_1_0["reboot_now"] = reboot;

	ApiFcnMap api_fcn_map;
	api_fcn_map["1.0"] = api_1_0;

	return api_fcn_map;
}

VarMap
reboot(const VarMap& args)
{
	dbus->process("", "reboot");
	block = true;

	return VarMap();
}
