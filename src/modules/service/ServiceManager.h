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

#ifndef __CONGA_MODSERVICE_SERVICEMANAGER_H
#define __CONGA_MODSERVICE_SERVICEMANAGER_H

#include "XML.h"
#include "counting_auto_ptr.h"

#include <list>
#include "String.h"
#include <map>


class ServiceManager;


class Service
{
	public:
		Service();
		virtual ~Service();

		String name() const;
		bool enabled() const;
		bool running() const;
		String description() const;

		void enable();
		void disable();
		void restart();
		void start();
		void stop();

		XMLObject xml(bool descriptions) const;

	private:
		Service(const String& name, bool enabled);

		mutable counting_auto_ptr<String> _name;
		mutable counting_auto_ptr<String> _descr;
		mutable counting_auto_ptr<bool> _enabled;
		mutable counting_auto_ptr<bool> _running;

		enum ActionState {
			START,
			STOP,
			RESTART
		};

		static void enable_service(const String& name, bool on);
		static bool service_running(const String& name);
		static void run_service(const String& name, ActionState state);

	friend class ServiceManager;
};

class ServiceSet
{
	public:
		ServiceSet();
		ServiceSet(const String& name, const String& description);
		virtual ~ServiceSet();

		String name() const;
		bool enabled() const;
		bool running() const;
		String description() const;

		void enable();
		void disable();
		void start();
		void restart();
		void stop();

		std::list<Service> servs;
		XMLObject xml(bool descriptions) const;

	private:
		mutable counting_auto_ptr<String> _name;
		mutable counting_auto_ptr<String> _descr;
};

class ServiceManager
{
	public:
		ServiceManager();
		virtual ~ServiceManager();

		void enable(const std::list<String>& services, const std::list<String>& sets);
		void disable(const std::list<String>& services, const std::list<String>& sets);

		void start(const std::list<String>& services, const std::list<String>& sets);
		void restart(const std::list<String>& services, const std::list<String>& sets);
		void stop(const std::list<String>& services, const std::list<String>& sets);

		void lists(std::list<Service>& services, std::list<ServiceSet>& sets);

		void query(	const std::list<String>& serv_names,
					const std::list<String>& set_names,
					std::list<Service>& services,
					std::list<ServiceSet>& sets);

	private:
		std::map<String, Service> _servs;
		std::map<String, ServiceSet> _sets;
		std::map<String, ServiceSet> generate_sets();

		bool populate_set(ServiceSet& ss, std::list<String> servs);
};

#endif
