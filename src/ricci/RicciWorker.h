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

#ifndef __CONGA_RICCI_RICCIWORKER_H
#define __CONGA_RICCI_RICCIWORKER_H

#include "DBusController.h"
#include "XML.h"
#include "counting_auto_ptr.h"
#include "RebootModule.h"

class BatchWorker;

class ProcessWorker
{
	public:
		ProcessWorker(	DBusController& dbus,
						const XMLObject&,
						BatchWorker& batch,
						RebootModule& rm);
		virtual ~ProcessWorker();

		virtual bool done() const;
		virtual bool completed() const;
		virtual bool scheduled() const;
		virtual bool in_progress() const;
		virtual bool failed() const;
		virtual bool removed() const;
		virtual void remove();

		virtual XMLObject report() const;

		virtual void process();

		enum state {
			st_done			= 0,	// completed successfully
			st_sched		= 1,	// scheduled
			st_prog			= 2,	// in progress
			st_mod_fail		= 3,	// module failure
			st_req_fail		= 4,	// request failure, module succeeded
			st_removed		= 5		// removed from scheduler
		};

	protected:
		DBusController& _dbus;
		RebootModule& _rm;
		mutable XMLObject _report;
		state _state;
		BatchWorker& _batch;

	private:
		bool check_response(const XMLObject& resp);

		ProcessWorker(const ProcessWorker&);
		ProcessWorker& operator=(const ProcessWorker&);
};

class BatchWorker
{
	public:
		BatchWorker(DBusController& dbus, const String& path);
		virtual ~BatchWorker();

		virtual XMLObject report() const;
		virtual void process();

	private:
		RebootModule _rm;

		std::list<counting_auto_ptr<ProcessWorker> > _procs;
		XMLObject _xml;
		ProcessWorker::state _state;
		String _path;

		int _fd;
		void close_fd(int fd);
		void save();

		BatchWorker(const BatchWorker&);
		BatchWorker& operator=(const BatchWorker&);

	friend class ProcessWorker;
};

#endif
