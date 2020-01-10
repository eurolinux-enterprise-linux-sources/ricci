/*
** Copyright (C) 2008-2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#ifndef __RICCI_NET_H
#define __RICCI_NET_H

#define DEFAULT_READBUF_LEN		4096
#define MAX_READBUF_LEN			1048576
#define CONNECT_TIMEOUT_LEN		30

enum {
	RICCI_STATE_WANT_CONNECT = 0,
	RICCI_STATE_WANT_AUTH_RESP,
	RICCI_STATE_WANT_SEND_AUTH,
	RICCI_STATE_WANT_SET_CONF,
	RICCI_STATE_WANT_SET_CONF_RESP,
	RICCI_STATE_DONE
};

struct ricci_conn {
	PRFileDesc *fd;
	char *hostname;
	char *cluster_name;
	char *buf;
	size_t buflen;
	size_t bufsize;

	char *wbuf;
	size_t wbuflen;
	size_t wbuf_off;

	time_t last_active;
	uint16_t port;
	int connected;
	int auth;
	int state;
};

int ricci_conn_handler(struct ricci_conn *c, PRPollDesc *pd, xmlDocPtr doc);
uint32_t ricci_clear_pollfd(PRPollDesc *pd, uint32_t target, uint32_t num_fd);

#endif
