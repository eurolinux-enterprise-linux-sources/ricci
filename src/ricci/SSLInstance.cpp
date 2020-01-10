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
 * Authors:
 *  Stanko Kupcevic <kupcevic@redhat.com>
 *  Ryan McCabe <rmccabe@redhat.com>
 */

#include "SSLInstance.h"
#include "Mutex.h"
#include "ricci_defines.h"
#include "Time.h"
#include "Random.h"
#include "utils.h"
#include "File.h"

#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
	#include "sys_util.h"
}

#include <iostream>
#include <list>
#include <set>

using namespace std;


static Mutex global_lock;
static bool ssl_inited = false;
static SSL_CTX* ctx = 0;
static vector<counting_auto_ptr<Mutex> > ssl_locks;


class file_cert
{
	public:
		file_cert(const String& file, const String& cert) :
			file(file),
			cert(cert) {}

		String file;
		String cert;
};

static list<file_cert> authorized_certs;

static int
verify_cert_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	return 1;
}

static void
load_client_certs()
{
	MutexLocker l(global_lock);

	// load authorized CAs
	if (!SSL_CTX_load_verify_locations(ctx, CLIENT_AUTH_CAs_PATH, NULL))
		cerr << "failed to load authorized CAs" << endl;

	STACK_OF(X509_NAME) *cert_names =
		SSL_load_client_CA_file(CLIENT_AUTH_CAs_PATH);

	if (cert_names)
		SSL_CTX_set_client_CA_list(ctx, cert_names);
	else
		cerr << "failed to load authorized CAs" << endl;

	// load saved certs

	set<String> files;
	String dir_path(CLIENT_CERTS_DIR_PATH);
	DIR* d = opendir(dir_path.c_str());
	if (d == NULL)
		throw String("unable to open directory ") + dir_path;
	try {
		while (true) {
			struct dirent* ent = readdir(d);
			if (ent == NULL) {
				closedir(d);
				break;
			}

			String kid_path = ent->d_name;
			if (kid_path == "." || kid_path == "..")
				continue;
			kid_path = dir_path + "/" + kid_path;

			struct stat st;
			if (stat(kid_path.c_str(), &st))
				continue;
			if (S_ISREG(st.st_mode))
				files.insert(kid_path);
		}
	} catch ( ... ) {
		closedir(d);
		throw;
	}

	authorized_certs.clear();

	for (set<String>::const_iterator
		iter = files.begin() ;
		iter != files.end() ;
		iter++)
	{
		try {
			String cert(File::open(*iter).read());
			if (cert.size() && cert.size() < 10 * 1024)
				authorized_certs.push_back(file_cert(*iter, cert));
		} catch ( ... ) {}
	}
}

static void
ssl_mutex_callback(int mode, int n, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
		ssl_locks[n]->lock();
	else
		ssl_locks[n]->unlock();
}

static pthread_t
ssl_id_callback(void)
{
	return pthread_self();
}

// ##### class SSLInstance #####


SSLInstance::SSLInstance(ClientSocket sock) :
	_sock(sock),
	_accepted(false)
{
	{
		MutexLocker l(global_lock);
		if (!ssl_inited) {
			// init library

			SSL_library_init();
			// TODO: random number generator,
			// not on systems with /dev/urandom (eg. Linux)

			// thread support
			ssl_locks.clear();
			for (int i = 0; i < CRYPTO_num_locks() + 1 ; i++)
				ssl_locks.push_back(counting_auto_ptr<Mutex>(new Mutex()));

			CRYPTO_set_locking_callback(ssl_mutex_callback);
			CRYPTO_set_id_callback(ssl_id_callback);

			// create context
			if (!ctx)
				ctx = SSL_CTX_new(SSLv23_server_method());
			if (!ctx)
				throw String("SSL context creation failed");

			// set verify_callback() function
			SSL_CTX_set_verify(ctx,
				SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE,
				verify_cert_callback);

			// set mode
			SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
			SSL_CTX_set_mode(ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

			// load key
			if (!SSL_CTX_use_PrivateKey_file(ctx,
				SERVER_KEY_PATH, SSL_FILETYPE_PEM))
			{
				throw String("error importing server's cert key file");
			}

			// load server cert
			if (!SSL_CTX_use_certificate_file(ctx,
				SERVER_CERT_PATH, SSL_FILETYPE_PEM))
			{
				throw String("error importing server's cert file");
			}

			// load client certs
			load_client_certs();

			ssl_inited = true;
		}

		// create SSL object, giving it context
		_ssl = SSL_new(ctx);
		if (!_ssl)
			throw String("creation of ssl object failed");
	}

	// make socket non-blocking
	try {
		_sock.nonblocking(true);
	} catch ( ... ) {
		SSL_free(_ssl);
		throw;
	}

	// assign fd to _ssl
	if (!SSL_set_fd(_ssl, _sock.get_sock())) {
		SSL_free(_ssl);
		throw String("fd assignment to ssl_obj failed");
	}
}

SSLInstance::~SSLInstance()
{
	SSL_shutdown(_ssl);
	SSL_free(_ssl);
}

bool
SSLInstance::accept(unsigned int timeout)
{
	if (_accepted)
		return _accepted;

	unsigned int beg = time_mil();
	while (time_mil() < beg + timeout) {
		int ret = SSL_accept(_ssl);
		if (ret == 1) {
			_accepted = true;
			break;
		} else {
			bool want_read, want_write;
			check_error(ret, want_read, want_write);
			socket().ready(want_read, want_write, 250);
		}
	}

	return _accepted;
}

String
SSLInstance::send(const String& msg, unsigned int timeout)
{
	if (!_accepted)
		throw String("cannot send, yet: SSL connection not accepted");

	if (msg.empty())
		return msg;

	unsigned int beg = time_mil();
	while (time_mil() < beg + timeout) {
		int ret = SSL_write(_ssl, msg.c_str(), msg.size());
		if (ret > 0) {
			return msg.substr(ret);
		} else {
			bool want_read, want_write;
			check_error(ret, want_read, want_write);
			socket().ready(want_read, want_write, 250);
		}
	}

	return msg;
}

String
SSLInstance::recv(unsigned int timeout)
{
	if (!_accepted)
		throw String("cannot receive, yet: SSL connection not accepted");

	char buff[4096];
	unsigned int beg = time_mil();
	while (time_mil() < beg + timeout) {
		int ret = SSL_read(_ssl, buff, sizeof(buff));
		if (ret > 0) {
			String data(buff, ret);
			memset(buff, 0, sizeof(buff));
			return data;
		} else {
			bool want_read, want_write;
			check_error(ret, want_read, want_write);
			socket().ready(want_read, want_write, 250);
		}
	}

	return "";
}

bool
SSLInstance::client_has_cert()
{
	if (!_accepted)
		throw String("cannot determine if client has certificate: SSL connection not accepted");

	if (_cert_pem.size())
		return true;

	X509 *cert = SSL_get_peer_certificate(_ssl);
	if (!cert)
		return false;

	// load cert into _cert_pem
	FILE* f = NULL;
	try {
		if (!(f = tmpfile()))
			throw String("unable to open temp file");

		if (!PEM_write_X509(f, cert))
			throw String("unable to write cert to tmp file");
		X509_free(cert);
		cert = NULL;

		// read cert
		rewind(f);

		while (true) {
			/*
			** By default, certificate files are usually about 1400 bytes long.
			*/
			char buff[2048];

			size_t i = fread(buff, sizeof(char), sizeof(buff), f);
			_cert_pem.append(buff, i);
			if (i == 0) {
				if (feof(f))
					break;
				else
					throw String("error while reading certificate from temp file");
			}
		}
		fclose(f);
		f = NULL;
	} catch ( ... ) {
		if (cert)
			X509_free(cert);

		if (f)
			fclose(f);
		_cert_pem.clear();
		throw;
	}

	return true;
}

bool
SSLInstance::client_cert_authed()
{
	// signed by authorized CAs?
	X509* cert = SSL_get_peer_certificate(_ssl);
	if (!cert)
		return false;

	X509_free(cert);
	if (SSL_get_verify_result(_ssl) == X509_V_OK)
		return true;

	// cert present among saved certs?
	client_has_cert(); // make sure cert is saved in _cert_pem
	MutexLocker l(global_lock);
	for (list<file_cert>::const_iterator
		iter = authorized_certs.begin() ;
		iter != authorized_certs.end() ;
		iter++)
	{
		if (iter->cert == _cert_pem)
			return true;
	}

	return false;
}

bool
SSLInstance::save_client_cert()
{
	MutexLocker l(global_lock);

	if (!client_has_cert())
		throw String("client did not present cert");

	String f_name(CLIENT_CERTS_DIR_PATH);
	f_name += "/client_cert_XXXXXX";

	int fd = -1;
	char *buff = new char[f_name.size() + 1];

	try {
		// pick a filename
		strcpy(buff, f_name.c_str());

		if ((fd = mkstemp(buff)) == -1)
			throw String("unable to generate random file");
		f_name = buff;

		delete[] buff;
		buff = NULL;

		String data(_cert_pem);
		ssize_t i = write_restart(fd, data.c_str(), data.size());
		if (i < 0) {
			throw String("error writing certificate: ")
					+ String(strerror(-i));
		}

		while (close(fd) && errno == EINTR)
			;
	} catch ( ... ) {
		if (buff)
			delete[] buff;

		if (fd != -1) {
			while (close(fd) && errno == EINTR)
				;
		}
		unlink(f_name.c_str());
		return false;
	}

	load_client_certs();
	return true;
}

bool
SSLInstance::remove_client_cert()
{
	MutexLocker l(global_lock);

	if (!client_has_cert())
		throw String("client did not present cert");

	for (list<file_cert>::const_iterator
			iter = authorized_certs.begin() ;
			iter != authorized_certs.end() ;
			iter++)
	{
		if (iter->cert == _cert_pem) {
			if (unlink(iter->file.c_str()) != 0)
				throw String("error removing certificate");
		}
	}

	load_client_certs();
	return true;
}

ClientSocket&
SSLInstance::socket()
{
	return _sock;
}

void
SSLInstance::check_error(int value, bool& want_read, bool& want_write)
{
	want_read = want_write = false;

	int err = errno;

	String e;
	switch (SSL_get_error(_ssl, value)) {
		case SSL_ERROR_NONE:
			e = "SSL_ERROR_NONE";
			break;
		case SSL_ERROR_ZERO_RETURN:
			e = "SSL_ERROR_ZERO_RETURN";
			break;
		case SSL_ERROR_WANT_READ:
			want_read = true;
			return;
		case SSL_ERROR_WANT_WRITE:
			want_write = true;
			return;
		case SSL_ERROR_WANT_CONNECT:
			e = "SSL_ERROR_WANT_CONNECT";
			break;
		case SSL_ERROR_WANT_ACCEPT:
			e = "SSL_ERROR_WANT_ACCEPT";
			break;
		case SSL_ERROR_WANT_X509_LOOKUP:
			e = "SSL_ERROR_WANT_X509_LOOKUP";
			break;
		case SSL_ERROR_SYSCALL:
			if (err == EAGAIN || err == EINTR)
				return;
			e = "SSL_ERROR_SYSCALL";
			break;
		case SSL_ERROR_SSL:
			e = "SSL_ERROR_SSL";
			break;
	}
	throw String("SSL_read() error: ") + e + ": " + String(strerror(err));
}
