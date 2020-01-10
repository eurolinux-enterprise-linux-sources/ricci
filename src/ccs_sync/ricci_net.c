/*
** Copyright (C) 2008-2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

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
#include "ricci_nss.h"
#include "ricci_net.h"
#include "ricci_list.h"
#include "ricci_conf_xml.h"
#include "ricci_queries.h"

static int ricci_conn_flush_writebuf(struct ricci_conn *c) {
	int ret;

	do {
		ret = PR_Write(c->fd, &c->wbuf[c->wbuf_off], c->wbuflen);
		if (ret == 0)
			return (-1);
		if (ret < 0) {
			if (errno == EINTR || errno == EAGAIN)
				return (0);
			return (-1);
		}

		time(&c->last_active);

		c->wbuf_off += ret;
		if (c->wbuf_off >= c->wbuflen) {
			/* There may have been passwords in the write buffer */
			memset(c->wbuf, 0, c->wbuflen);
			free(c->wbuf);
			c->wbuf = NULL;
			c->wbuf_off = 0;
			c->wbuflen = 0;
		}
	} while (ret > 0 && c->wbuflen > 0);

	return (0);
}

static int ricci_conn_read(struct ricci_conn *c, xmlDocPtr *resp_xml) {
	int ret;

	ret = PR_Read(c->fd, &c->buf[c->buflen], c->bufsize - c->buflen);
	if (ret == 0)
		return (-1);

	if (ret < 0) {
		if (errno == EINTR || errno == EAGAIN)
			return (0);
		return (-1);
	}

	time(&c->last_active);
	c->buflen += ret;

	*resp_xml = xmlReadMemory(c->buf, c->buflen, c->hostname, NULL,
					XML_PARSE_NOWARNING | XML_PARSE_NOERROR | XML_PARSE_NONET);

	/* We may not have received all of the XML response yet. */
	if (*resp_xml == NULL) {
		if (c->bufsize >= MAX_READBUF_LEN) {
			/* Give up. Too much incomplete/invalid data read */
			return (-2);
		}

		if (c->buflen >= (c->bufsize / 2)) {
			size_t new_bufsize = c->bufsize * 2;
			char *new_buf = realloc(c->buf, new_bufsize);
			if (new_buf != NULL) {
				c->buf = new_buf;
				c->bufsize = new_bufsize;
			}
		}
	} else {
		memset(c->buf, 0, c->buflen);
		c->buflen = 0;
		return (1);
	}

	return (0);
}

static int ricci_conn_want_read(struct ricci_conn *c, xmlDocPtr *resp) {
	int ret;

	ret = ricci_conn_read(c, resp);
	if (ret < 0) {
		if (ret == -2) {
			fprintf(stderr,
				"Failed to receive valid XML from %s after reading %lu bytes. Closing connection.\n",
				c->hostname, c->buflen + 1);
		} else {
			fprintf(stderr, "The connection to %s died unexpectedly\n",
				c->hostname);
		}
		return (ret);
	}

	time(&c->last_active);
	if (resp == NULL)
		return (1);

	/*
	** We were able to parse the XML response, so we've
	** received the whole message.
	*/
	return (0);
}

static int ricci_conn_want_write(struct ricci_conn *c, PRPollDesc *pd) {
	int ret;

	pd->in_flags |= PR_POLL_WRITE;
	if (!(pd->out_flags & PR_POLL_WRITE))
		return (0);

	ret = ricci_conn_flush_writebuf(c);
	if (ret < 0)
		return (ret);

	time(&c->last_active);
	if (c->wbuf == NULL) {
		/* We finished sending everything in the write buffer. */
		pd->in_flags = PR_POLL_READ | PR_POLL_EXCEPT;
		return (1);
	}
	return (0);
}

static int ricci_conn_handler_want_connect(	struct ricci_conn *c,
											PRPollDesc *pd,
											xmlDocPtr xmldoc __notused)
{
	PRStatus status = PR_ConnectContinue(pd->fd, pd->out_flags);
	if (status != PR_SUCCESS) {
		PRErrorCode err_code = PR_GetError();
		if (err_code == PR_IN_PROGRESS_ERROR) {
			pd->in_flags = PR_POLL_READ | PR_POLL_WRITE | PR_POLL_EXCEPT;
		} else if (err_code == PR_CONNECT_REFUSED_ERROR) {
			fprintf(stderr, "Failed to connect to %s: Connection refused.\n",
				c->hostname);
			return (-1);
		} else if (err_code == PR_NETWORK_UNREACHABLE_ERROR) {
			fprintf(stderr, "Failed to connect to %s: Network unreachable.\n",
				c->hostname);
			return (-1);
		} else if (err_code == PR_CONNECT_TIMEOUT_ERROR) {
			fprintf(stderr, "Failed to connect to %s: Connection timed out.\n",
				c->hostname);
			return (-1);
		}

		if (time(NULL) - c->last_active >= CONNECT_TIMEOUT_LEN) {
			fprintf(stderr, "Failed to connect to %s after %d seconds.\n",
				c->hostname, CONNECT_TIMEOUT_LEN);
			return (-1);
		}
	} else {
		time(&c->last_active);
		pd->in_flags = PR_POLL_READ | PR_POLL_EXCEPT;
		c->state = RICCI_STATE_WANT_AUTH_RESP;
	}
	return (0);
}

static int ricci_conn_handler_want_send_auth(	struct ricci_conn *c,
												PRPollDesc *pd,
												xmlDocPtr xmldoc __notused)
{
	int ret;

	if (c->wbuf == NULL) {
		char prompt[128];
		char *auth_cmd = NULL;
		int auth_cmd_len = 0;
		char *passwd;

		snprintf(prompt, sizeof(prompt),
			"Enter the root password for %s: ", c->hostname);
		printf("You have not authenticated to the ricci daemon on %s\n",
			c->hostname);

		passwd = getpass(prompt);
		if (passwd == NULL || strlen(passwd) < 1) {
			fprintf(stderr, "No password for %s was given\n", c->hostname);
			return (0);
		}

		auth_cmd_len = asprintf(&auth_cmd, ricci_auth_str, passwd);
		memset(passwd, 0, strlen(passwd));
		if (auth_cmd_len < 0 || auth_cmd == NULL) {
			fprintf(stderr, "Out of memory for %s\n", c->hostname);
			return (0);
		}

		c->wbuf = auth_cmd;
		c->wbuflen = auth_cmd_len;
		c->wbuf_off = 0;
	}

	ret = ricci_conn_want_write(c, pd);
	if (ret > 0)
		c->state = RICCI_STATE_WANT_AUTH_RESP;
	return (ret);
}

static int ricci_conn_handler_want_auth_resp(	struct ricci_conn *c,
												PRPollDesc *pd,
												xmlDocPtr xmldoc __notused)
{
	int ret;
	xmlDocPtr resp = NULL;

	if (!(pd->out_flags & PR_POLL_READ))
		return (0);

	ret = ricci_conn_want_read(c, &resp);
	if (ret < 0)
		return (ret);
	if (ret > 0 || resp == NULL)
		return (0);

	ret = ricci_xml_authed(c, resp);
	if (ret == 1) {
		c->state = RICCI_STATE_WANT_SET_CONF;
		pd->in_flags |= PR_POLL_WRITE;
	} else if (ret == 0) {
		c->state = RICCI_STATE_WANT_SEND_AUTH;
		pd->in_flags |= PR_POLL_WRITE;
	}

	return (0);
}

static int ricci_conn_handler_want_set_conf(struct ricci_conn *c,
											PRPollDesc *pd,
											xmlDocPtr conf_xml)
{
	int ret;

	if (c->wbuf == NULL) {
		char *conf_str = NULL;
		size_t conf_len;

		ret = xmldoc_to_str(conf_xml, &conf_str, &conf_len);
		if (conf_str != NULL && ret != -1 && conf_len > 0) {
			char *set_conf_cmd = NULL;
			int cmd_len = asprintf(&set_conf_cmd, ricci_set_conf_str, conf_str);
			if (cmd_len < 0 || set_conf_cmd == NULL) {
				memset(conf_str, 0, conf_len);
				free(conf_str);
				return (0);
			}

			memset(conf_str, 0, conf_len);
			free(conf_str);

			if (set_conf_cmd != NULL && cmd_len > 0) {
				c->wbuf = set_conf_cmd;
				c->wbuflen = cmd_len;
				c->wbuf_off = 0;
			}
		}
	}

	ret = ricci_conn_want_write(c, pd);
	if (ret > 0)
		c->state = RICCI_STATE_WANT_SET_CONF_RESP;
	return (ret);
}

static int ricci_conn_handler_want_set_conf_resp(	struct ricci_conn *c,
													PRPollDesc *pd,
													xmlDocPtr xmldoc __notused)
{
	int ret;
	xmlDocPtr resp = NULL;

	pd->in_flags |= PR_POLL_READ;
	if (!(pd->out_flags & PR_POLL_READ))
		return (0);

	ret = ricci_conn_want_read(c, &resp);
	if (ret < 0)
		return (ret);
	if (ret > 0 || resp == NULL)
		return (0);

	ret = ricci_set_conf_status(c->hostname, resp);
	if (ret == 1) {
		c->state = RICCI_STATE_DONE;
		return (0);
	}

	fprintf(stderr, "Unable to set the cluster.conf for %s\n", c->hostname);
	return (-1);
}

int ricci_conn_handler(struct ricci_conn *c, PRPollDesc *pd, xmlDocPtr doc) {
	static int (* const f[])(struct ricci_conn *, PRPollDesc *, xmlDocPtr) = {
		ricci_conn_handler_want_connect,
		ricci_conn_handler_want_auth_resp,
		ricci_conn_handler_want_send_auth,
		ricci_conn_handler_want_set_conf,
		ricci_conn_handler_want_set_conf_resp
	};
	return (f[c->state](c, pd, doc));
}

uint32_t ricci_clear_pollfd(PRPollDesc *pd, uint32_t target, uint32_t num_fd) {
	if (num_fd < 1)
		return (0);
	--num_fd;

	if (target != num_fd) {
		pd[target].fd = pd[num_fd].fd;
		pd[target].in_flags = pd[num_fd].in_flags;
		pd[target].out_flags = 0;
	}

	return (num_fd);
}
