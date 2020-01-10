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

#ifndef __CONGA_RICCI_H
#define __CONGA_RICCI_H

#include "DBusController.h"
#include "XML.h"

enum RicciRetCode {
	RRC_SUCCESS					= 0,
	RRC_MISSING_VERSION			= 1,
	RRC_UNSUPPORTED_VERSION		= 2,
	RRC_MISSING_FUNCTION		= 3,
	RRC_INVALID_FUNCTION		= 4,
	RRC_NEED_AUTH				= 5,
	RRC_INTERNAL_ERROR			= 6,
	RRC_AUTH_FAIL				= 10,
	RRC_MISSING_BATCH			= 11,
	RRC_INVALID_BATCH_ID		= 12,
	RRC_MISSING_MODULE			= 13,	// remove
	RRC_MODULE_FAILURE			= 14	// remove
};

class Ricci
{
	public:
		Ricci(DBusController& dbus);
		virtual ~Ricci();
		XMLObject hello(bool authed) const;

		XMLObject request(const XMLObject& req,
					bool& authenticated,
					bool& save_cert,
					bool& remove_cert,
					bool& done);

	private:
		DBusController& _dbus;
		int _fail_auth_attempt;
		XMLObject ricci_header(bool authed, bool full=false) const;
};

class Batch
{
	public:
		Batch(const XMLObject&);
		Batch(long long id);

		virtual ~Batch();
		virtual long long id() const;
		virtual bool done() const;
		virtual XMLObject report() const;

		// start workers on existing batch files
		static void restart_batches();

	private:
		XMLObject _report;
		String _path;
		long long _id;
		long long _state;

		static void start_worker(const String& path);

		Batch(const Batch&);
		Batch& operator=(const Batch&);
};

#endif
