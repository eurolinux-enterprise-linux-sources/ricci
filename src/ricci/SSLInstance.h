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

#ifndef __CONGA_RICCI_SSLINSTANCE_H
#define __CONGA_RICCI_SSLINSTANCE_H

#include "Socket.h"

#include "String.h"
#include <openssl/ssl.h>

// NOT THREAD SAFE

class SSLInstance
{
	public:
		SSLInstance(ClientSocket sock);
		virtual ~SSLInstance();

		bool accept(unsigned int timeout);

		String send(const String& msg, unsigned int timeout);
		String recv(unsigned int timeout);

		// return true if peer's cert authenticated
		// (either thru CA chain, or cert present)
		bool client_cert_authed();

		bool client_has_cert();
		bool save_client_cert();
		bool remove_client_cert();

		ClientSocket& socket();

	private:
		SSLInstance(const SSLInstance&);
		SSLInstance operator=(const SSLInstance&);

		ClientSocket _sock;
		SSL *_ssl;
		String _cert_pem;

		bool _accepted;
		void check_error(int value, bool& want_read, bool& want_write);
};

#endif
