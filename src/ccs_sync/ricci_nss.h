/*
** Copyright (C) 2008-2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#ifndef __RICCI_NSS_H
#define __RICCI_NSS_H

SECStatus nss_connect(  PRFileDesc *ssl_sock,
						const char *addr_str,
						uint16_t port);

PRFileDesc *get_ssl_socket(const char *cert_nick, PRBool nonblocking);
int init_libnss_ssl(const char *cert_db_dir);
int shutdown_libnss_ssl(void);
PRStatus set_nss_sockopt_nonblock(PRFileDesc *sock, PRBool val);

#endif
