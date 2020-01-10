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

#include "DBusController.h"
#include "Mutex.h"
#include "utils.h"

#include "String.h"
#include <fstream>

#include <dbus/dbus.h>

using namespace std;

#define DBUS_TIMEOUT	2147483647 // milliseconds

static DBusConnection *_dbus_conn = NULL;
static Mutex _dbus_mutex;
static int _object_counter = 0;

DBusController::DBusController()
{
	// TODO: dynamically determine,
	_mod_map["storage"]		= "modstorage_rw";
	_mod_map["cluster"]		= "modcluster_rw";
	_mod_map["rpm"]			= "modrpm_rw";
	_mod_map["log"]			= "modlog_rw";
	_mod_map["service"]		= "modservice_rw";
	_mod_map["virt"]		= "modvirt_rw";
	_mod_map["reboot"]		= "reboot";

	MutexLocker lock(_dbus_mutex);
	if (_dbus_conn == NULL) {
		DBusError error;
		dbus_error_init(&error);
		_dbus_conn = dbus_bus_get(DBUS_BUS_SYSTEM, &error);
		if (dbus_error_is_set(&error) || !_dbus_conn) {
			dbus_error_free(&error);
			_dbus_conn = NULL;
			throw String("failed to get system bus connection");
		} else
			dbus_error_free(&error);
	}
	if (_dbus_conn != NULL)
		_object_counter++;
}

DBusController::~DBusController()
{
	MutexLocker lock(_dbus_mutex);

	if (--_object_counter == 0) {
		dbus_connection_unref(_dbus_conn);
		_dbus_conn = NULL;
	}
}

static String
remove_chars(const String& str, char c)
{
	String s(str);

	String::size_type pos;
	while ((pos = s.find(c)) != s.npos)
		s.erase(pos, 1);
	return s;
}

String
DBusController::process(const String& message, const String& module_name)
{
	MutexLocker l(_dbus_mutex);

	if (_mod_map.find(module_name) == _mod_map.end())
		throw String("module not supported");

	// prepare msg
	DBusMessage *msg = dbus_message_new_method_call("com.redhat.ricci",
							"/com/redhat/ricci",
							"com.redhat.ricci",
							_mod_map[module_name].c_str());

	if (!msg)
		throw String("not enough memory to create message");

	if (message.size()) {
		String msg_clean(remove_chars(message, '\n'));
		const char *msg_clean_c_str = msg_clean.c_str();
		const void *message_dbus_ready = NULL;
		message_dbus_ready = &msg_clean_c_str;

		if (!dbus_message_append_args(msg,
				DBUS_TYPE_STRING,
				message_dbus_ready,
				DBUS_TYPE_INVALID))
		{
			throw String("error appending argument to message");
		}
	}

	DBusError error;
	dbus_error_init(&error);

	DBusMessage *resp = dbus_connection_send_with_reply_and_block(_dbus_conn,
							msg,
							DBUS_TIMEOUT,
							&error);
	dbus_message_unref(msg);

	// process response
	if (resp) {
		try {
			dbus_error_free(&error);

			int status;
			char *out;
			char *err;

			dbus_message_get_args(resp, NULL,
					DBUS_TYPE_INT32, &status,
					DBUS_TYPE_STRING, &out,
					DBUS_TYPE_STRING, &err,
					DBUS_TYPE_INVALID);

			if (status)
				throw String("module returned error code: ") + String(err);
			String ret(out);
			dbus_message_unref(resp);
			return ret;
		} catch ( ... ) {
			dbus_message_unref(resp);
			throw;
		}
	} else {
		String error_msg(error.message);
		dbus_error_free(&error);
		throw String("system bus response msg error: ") + error_msg;
	}
}

list<String>
DBusController::modules()
{
	list<String> mods;

	for (map<String, String>::const_iterator
		iter = _mod_map.begin() ;
		iter != _mod_map.end() ;
		iter++)
	{
		mods.push_back(iter->first);
	}
	return mods;
}
