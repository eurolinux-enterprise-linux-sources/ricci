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

#include "Auth.h"
#include "Mutex.h"
#include <sasl/sasl.h>

static int
sasl_getopts_callback(	void* context,
						const char *plugin_name,
						const char *option,
						const char **result,
						unsigned int *len);


static Mutex mutex; // global sasl_lib protection mutex
static bool inited = false; // sasl_lib initialized?

const static
sasl_callback_t callbacks[] = {
	{ SASL_CB_GETOPT,	(int (*)()) sasl_getopts_callback,	NULL },
	{ SASL_CB_LIST_END,	NULL,								NULL },
};

Auth::Auth()
{
	if (!initialize_auth_system())
		throw String("Failed to initialize authentication engine");
}

Auth::~Auth()
{}

bool
Auth::authenticate(const String& passwd) const
{
	MutexLocker l(mutex);
	sasl_conn_t *conn = NULL;

	try {
		bool success = false;
		int ret = sasl_server_new("ricci", // servicename
					NULL,		// hostname
					NULL,		// realm
					NULL,		// local ip:port
					NULL,		// remote ip:port
					callbacks,
					0,			// connection flags
					&conn);

		if (ret != SASL_OK)
			throw String("authentication engine error");

		ret = sasl_checkpass(conn, "root", 4, passwd.c_str(), passwd.size());
		if (ret == SASL_OK)
			success = true;
		else {
			if (ret != SASL_BADAUTH)
				throw String("authentication engine error");
		}

		sasl_dispose(&conn);
		conn = NULL;
		return success;
	} catch ( ... ) {
		if (conn) {
			sasl_dispose(&conn);
			conn = NULL;
		}
		throw;
	}
}

bool
Auth::initialize_auth_system()
{
	MutexLocker l(mutex);

	if (!inited) {
		int ret = sasl_server_init(callbacks, "ricci");
		inited = (ret == SASL_OK);
	}
	return inited;
}

int
sasl_getopts_callback(	void *context,
						const char *plugin_name,
						const char *option,
						const char **result,
						unsigned *len)
{

	try {
		static const char authd_option[] = "pwcheck_method";
		static const char authd_result[] = "saslauthd";
		static const char authd_version_option[] = "saslauthd_version";
		static const char authd_version_result[] = "2";

		if (result) {
			*result = 0;
			if (!strcmp(option, authd_option))
				*result = authd_result;
			else if (!strcmp(option, authd_version_option))
				*result = authd_version_result;
			else {
				// modify more options we'd like to use
			}
		}

		if (len)
			*len = 0;

		return SASL_OK;
	} catch ( ... ) {
		return SASL_FAIL;
	}
}
