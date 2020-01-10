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

#include "ClientInstance.h"
#include "Time.h"
#include "XML.h"
#include "DBusController.h"
#include "Ricci.h"

extern "C" {
	#include "signals.h"
	#include <signal.h>
}

#include <iostream>

using namespace std;

#define ACCEPT_TIMEOUT		30	// seconds
#define SEND_TIMEOUT		120	// seconds
#define RECEIVE_TIMEOUT		120	// seconds

#define MAXIMUM_CLIENTS		10

static int counter = 0;
static Mutex counter_mutex;


ClientInstance::ClientInstance(	ClientSocket sock,
								DBusController& dbus_controller) :
	_ssl(sock),
	_dbus_controller(dbus_controller),
	_done(false)
{
	bool max_reached = false;

	if (true) {
		MutexLocker l(counter_mutex);
		if (counter > MAXIMUM_CLIENTS)
			max_reached = true;
		else {
			max_reached = false;
			counter++;
		}
	}

	if (max_reached) {
		// socket is non-blocking, couple bytes should be able
		// to go out, if not, who cares
		sock.send("overload - come back later");
		throw String("maximum number of clients reached");
	}
}

ClientInstance::~ClientInstance()
{
	if (true) {
		MutexLocker l(counter_mutex);
		counter--;
	}

	stop(); // stop the thread, if running
}

bool
ClientInstance::done()
{
	MutexLocker l(_mutex);
	return _done;
}

void
ClientInstance::run()
{
	setup_signal(SIGPIPE, SIG_IGN);

	int beg_mil = int(time_mil());
	try {
		// get dispatcher
		Ricci ricci(_dbus_controller);

		// begin encryption
		encrypt_begin();

		// client needs to present certificate
		if (!_ssl.client_has_cert()) {
			try {
				send(XMLObject("Clients_SSL_certificate_required"));
			} catch ( ... ) {}
			throw String("client hasn't presented certificate");
		}

		bool authed = _ssl.client_cert_authed();
		bool was_authed = authed;

		// send hello
		send(ricci.hello(authed));

		// process requests
		bool done = false;
		while (!done && !shouldStop()) {
			bool save_cert = false;
			bool remove_cert = false;
			XMLObject request;

			try {
				request = receive();
			} catch ( ... ) {
				try {
					String out = "Timeout_reached_without_valid_XML_request";
					send(XMLObject(out));
				} catch ( ... ) {}
				throw;
			}

			XMLObject response = ricci.request(request, authed,
									save_cert, remove_cert, done);

			if (!was_authed && save_cert)
				_ssl.save_client_cert();

			if (was_authed && remove_cert)
				_ssl.remove_client_cert();
			send(response);
		}
		send(XMLObject("bye"));
	} catch ( String e ) {
		cerr 	<< __FILE__ << ":" << __LINE__ << ": "
				<< "exception: " << e << endl;
	} catch ( ... ) {
		cerr	<< __FILE__ << ":" << __LINE__ << ": "
				<< "unknown exception" << endl;
	}

	cerr	<< "request completed in " << time_mil() - beg_mil
			<< " milliseconds" << endl;

	{
		MutexLocker l(_mutex);
		_done = true;
	}
}

XMLObject
ClientInstance::receive()
{
	int beg = int(time_sec());
	String xml_in;

	while (true) {
		if (shouldStop())
			throw String("thread exiting");
		else if (int(time_sec()) > beg + RECEIVE_TIMEOUT)
			throw String("Receive timeout");
		else
			xml_in += _ssl.recv(500);

		try {
			return parseXML(xml_in);
		} catch ( ... ) {}
	}
}

void
ClientInstance::send(const XMLObject& msg)
{
	int beg = int(time_sec());
	String out(generateXML(msg));

	while (true) {
		if (shouldStop())
			throw String("thread exiting");
		else if (int(time_sec()) > beg + SEND_TIMEOUT)
			throw String("Send timeout");
		else
			if ((out = _ssl.send(out, 500)).empty())
				break;
	}
}

void
ClientInstance::encrypt_begin()
{
	try {
		int beg = int(time_sec());
		while (true) {
			if (shouldStop())
				throw String("thread exiting");
			else if (int(time_sec()) > beg + ACCEPT_TIMEOUT)
				throw String("Accept timeout");
			else {
				if (_ssl.accept(500))
					break;
			}
		}
	} catch ( ... ) {
		int beg = int(time_sec());
		String out(generateXML(XMLObject("SSL_required")));

		while (true) {
			if (shouldStop())
				throw String("thread exiting");
			else if (int(time_sec()) > beg + SEND_TIMEOUT)
				throw String("Send timeout");
			else {
				bool read = false, write = true;

				_ssl.socket().ready(read, write, 500);
				if (write) {
					if ((out = _ssl.socket().send(out)).empty())
						break;
				}
			}
		}
		throw;
	}
}
