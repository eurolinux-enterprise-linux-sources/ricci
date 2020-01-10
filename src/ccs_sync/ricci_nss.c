/*
** Copyright (C) 2008-2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Modeled after the libnss 3.14 SSLsample program.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <errno.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <nspr.h>
#include <nss.h>
#include <nss3/ssl.h>
#include <nss3/sslproto.h>
#include <pk11func.h>

#include <prio.h>
#include <prnetdb.h>

#include "ricci_conf.h"

static void nss_handshake_cb(PRFileDesc *sock __notused, void *arg __notused) {
}

static SECStatus nss_bad_cert_cb(	void *arg __notused,
									PRFileDesc *sock __notused)
{
	return (SECSuccess);
}

static SECStatus nss_auth_data_cb(	void *arg,
									PRFileDesc *sock,
									struct CERTDistNamesStr *caNames __notused,
									struct CERTCertificateStr **pRetCert,
									struct SECKEYPrivateKeyStr **pRetKey)
{
	CERTCertificate *cert;
	SECKEYPrivateKey *privkey = NULL;
	const char *cert_nick = (char *) arg;
	void *proto_win = NULL;
	SECStatus sec_status = SECFailure;

	proto_win = SSL_RevealPinArg(sock);

	if (cert_nick != NULL) {
		cert = PK11_FindCertFromNickname(cert_nick, proto_win);
		if (cert != NULL) {
			privkey = PK11_FindKeyByAnyCert(cert, proto_win);
			if (privkey != NULL)
				sec_status = SECSuccess;
			else {
				CERT_DestroyCertificate(cert);
				sec_status = SECFailure;
			}
		}
	} else {
		CERTCertNicknames *names;
		int i;

		names = CERT_GetCertNicknames(CERT_GetDefaultCertDB(),
					SEC_CERT_NICKNAMES_USER, proto_win);

		if (names != NULL) {
			for (i = 0 ; i < names->numnicknames ; i++) {
				cert = PK11_FindCertFromNickname(names->nicknames[i],
						proto_win);
				if (cert == NULL)
					continue;

				/* Only check unexpired certs */
				if (CERT_CheckCertValidTimes(cert, PR_Now(), PR_FALSE) != secCertTimeValid)
				{
					CERT_DestroyCertificate(cert);
					continue;
				}

				privkey = PK11_FindKeyByAnyCert(cert, proto_win);
				if (privkey != NULL)
					sec_status = SECSuccess;
				else
					sec_status = SECFailure;
				break;
			}
			CERT_FreeNicknames(names);
		}
	}

	if (sec_status == SECSuccess) {
		*pRetCert = cert;
		*pRetKey = privkey;
	}

	return sec_status;
}

PRStatus set_nss_sockopt_nonblock(PRFileDesc *sock, PRBool val) {
	PRSocketOptionData sock_opt;

	sock_opt.option = PR_SockOpt_Nonblocking;
	sock_opt.value.non_blocking = val;
	return (PR_SetSocketOption(sock, &sock_opt));
}

PRFileDesc *get_ssl_socket(const char *cert_nick, PRBool nonblocking) {
	PRFileDesc *tcp_sock;
	PRFileDesc *ssl_sock;
	PRSocketOptionData sock_opt;
	PRStatus pr_status;
	SECStatus sec_status;

	tcp_sock = PR_NewTCPSocket();
	if (tcp_sock == NULL)
		return (NULL);

	sock_opt.option = PR_SockOpt_Nonblocking;
	sock_opt.value.non_blocking = nonblocking;

	pr_status = PR_SetSocketOption(tcp_sock, &sock_opt);
	if (pr_status != PR_SUCCESS) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	ssl_sock = SSL_ImportFD(NULL, tcp_sock);
	if (ssl_sock == NULL) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	sec_status = SSL_OptionSet(ssl_sock, SSL_SECURITY, PR_TRUE);
	if (sec_status != SECSuccess) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	sec_status = SSL_OptionSet(ssl_sock, SSL_HANDSHAKE_AS_CLIENT, PR_TRUE);
	if (sec_status != SECSuccess) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	sec_status = SSL_GetClientAuthDataHook(ssl_sock,
					(SSLGetClientAuthData) nss_auth_data_cb,
					(void *) cert_nick);
	if (sec_status != SECSuccess) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	sec_status = SSL_BadCertHook(ssl_sock,
					(SSLBadCertHandler) nss_bad_cert_cb, NULL);
	if (sec_status != SECSuccess) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	sec_status = SSL_HandshakeCallback(ssl_sock, nss_handshake_cb, NULL);
	if (sec_status != SECSuccess) {
		PR_Close(tcp_sock);
		return (NULL);
	}

	return (ssl_sock);
}

PRStatus nss_connect(PRFileDesc *sock, const char *addr_str, uint16_t port) {
	char buf[PR_NETDB_BUF_SIZE];
	PRStatus pr_status;
	SECStatus sec_status;
	PRIntn hostenum;
	PRHostEnt pr_hostent;
	PRNetAddr addr;

	sec_status = SSL_SetURL(sock, addr_str);
	if (sec_status != SECSuccess)
		return (PR_FAILURE);

	pr_status = PR_GetHostByName(addr_str, buf, sizeof(buf), &pr_hostent);
	if (pr_status != PR_SUCCESS)
		return (pr_status);

	hostenum = PR_EnumerateHostEnt(0, &pr_hostent, port, &addr);
	if (hostenum == -1)
		return (pr_status);

	pr_status = PR_Connect(sock, &addr, PR_INTERVAL_NO_TIMEOUT);
	if (pr_status != PR_SUCCESS) {
		if (errno != EINPROGRESS)
			return (pr_status);
	}

	sec_status = SSL_ResetHandshake(sock, PR_FALSE);
	if (sec_status != SECSuccess)
		return (PR_FAILURE);

	return (PR_SUCCESS);
}

int init_libnss_ssl(const char *cert_db_dir) {
	SECStatus status;

	PR_Init(PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);

	status = NSS_Init(cert_db_dir);
	if (status != SECSuccess)
		return (-1);
	NSS_SetDomesticPolicy();
	SSL_CipherPrefSetDefault(SSL_RSA_WITH_NULL_MD5, PR_TRUE);
	return (0);
}

int shutdown_libnss_ssl(void) {
	if (NSS_Shutdown() != SECSuccess)
		return (-1);
	PR_Cleanup();
	return (0);
}
