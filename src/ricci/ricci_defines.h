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

#ifndef __CONGA_RICCI_DEFINES_H
#define __CONGA_RICCI_DEFINES_H

#define RICCI_SERVER_PORT		11111

#define SERVER_CERT_PATH		"/var/lib/ricci/certs/cacert.pem"
#define SERVER_KEY_PATH			"/var/lib/ricci/certs/privkey.pem"
#define CLIENT_AUTH_CAs_PATH	"/var/lib/ricci/certs/auth_CAs.pem"
#define CLIENT_CERTS_DIR_PATH	"/var/lib/ricci/certs/clients/"

#define QUEUE_DIR_PATH			"/var/lib/ricci/queue/"
#define QUEUE_LOCK_PATH			"/var/lib/ricci/queue/lock"

#define RICCI_WORKER_PATH		"/usr/libexec/ricci/ricci-worker"

#endif
