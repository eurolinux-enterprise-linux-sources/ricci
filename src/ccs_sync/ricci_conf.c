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

#include <prio.h>
#include <prnetdb.h>

#include "ricci_conf.h"
#include "ricci_queries.h"
#include "ricci_list.h"
#include "ricci_nss.h"
#include "ricci_conf_xml.h"
#include "ricci_net.h"

#define RICCI_NSS_CERT_DIR	"/var/lib/ricci/certs/"
#define CLUSTER_CONF_PATH	"/etc/cluster/cluster.conf"

static void print_help(char const * const *argv);
static void print_version(void);
static int string_compare(void *l, void *r);
static void hash_dtor(void *param __notused, void *data);
static void ricci_conn_dtor(void *param __notused, void *data);
static int ricci_conn_cmp(void *l, void *r);
static void ricci_conn_alloc(void *data, void *param);
static int sync_cluster_conf(xmlDocPtr conf_xml, hash_t *nodes);

static int verbose = 0;
static int strict = 0;
static uint16_t ricci_port = 11111;
static PRPollDesc *pd;
static int pd_off;

static const char rgetopt_args[] = "f:c:p:hivVw";

int main(int argc, const char const * const *argv) {
	int opt;
	int ret;
	char *conf_path = NULL;
	char *cert_db_dir = NULL;
	xmlDocPtr doc;
	xmlNode *root;
	hash_t node_hash;
	hash_t *dest_nodes;
	int num_nodes = 0;
	int increment_version = 0;

	ret = hash_init(&node_hash, 5, string_compare, hash_string, hash_dtor);
	if (ret == -1) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	while ((opt = getopt(argc, (char * const *) argv, rgetopt_args)) != -1) {
		switch (opt) {
			case 'f':
				if (conf_path != NULL)
					free(conf_path);
				conf_path = strdup(optarg);
				if (conf_path == NULL) {
					fprintf(stderr, "Error: Out of memory\n");
					exit(1);
				}
				break;

			case 'c':
				if (cert_db_dir != NULL)
					free(cert_db_dir);
				cert_db_dir = strdup(optarg);
				if (cert_db_dir == NULL) {
					fprintf(stderr, "Error: Out of memory\n");
					exit(1);
				}
				break;

			case 'i':
				increment_version++;
				break;

			case 'p': {
				unsigned long port;
				char *p;
				port = strtoul(optarg, &p, 10);
				if (*p != '\0' || port == 0 || (port & 0xffff) != port) {
					fprintf(stderr, "Invalid port number: %s\n", optarg);
					exit(1);
				}
				ricci_port = (uint16_t) port;
				break;
			}

			case 'v':
				verbose = ~0;
				break;

			case 'w':
				strict++;
				break;

			case 'V':
				print_version();
				exit(0);

			case 'h':
				print_help(argv);
				exit(0);

			default:
				print_help(argv);
				exit(1);
		}
	}

	if (cert_db_dir == NULL) {
		cert_db_dir = strdup(RICCI_NSS_CERT_DIR);
		if (cert_db_dir == NULL) {
			fprintf(stderr, "Error: Out of memory");
			exit(1);
		}
	}

	if (conf_path == NULL) {
		conf_path = strdup(CLUSTER_CONF_PATH);
		if (conf_path == NULL) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}
	}

	doc = xmlReadFile(conf_path, NULL,
			(~verbose & XML_PARSE_NOWARNING) | XML_PARSE_NONET);
	if (doc == NULL) {
		fprintf(stderr, "Unable to parse %s: %m\n", conf_path);
		exit(1);
	}

	root = xmlDocGetRootElement(doc);
	if (root == NULL) {
		fprintf(stderr, "Unable to find the root node in %s\n", conf_path);
		exit(1);
	}

	if (increment_version) {
		if (increment_conf_version(root) == -1) {
			fprintf(stderr, "Unable to increment the cluster.conf version\n");
			exit(1);
		}
	}

	num_nodes = get_cluster_nodes(root, &node_hash);
	if (num_nodes <= 0 && argv[optind] == NULL) {
		fprintf(stderr, "Unable to find cluster nodes in %s\n", conf_path);
		exit(1);
	}

	if (argv[optind] == NULL) {
		/* Send to all current member nodes */
		dest_nodes = &node_hash;
	} else {
		hash_t cmdline_nodes;
		ret = hash_init(&cmdline_nodes, 5, string_compare,
				hash_string, hash_dtor);
		if (ret == -1) {
			fprintf(stderr, "Out of memory\n");
			exit(1);
		}

		do {
			char *cur_node = strdup(argv[optind++]);
			if (cur_node == NULL) {
				fprintf(stderr, "Out of memory\n");
				exit (1);
			}

			if (!hash_exists(&node_hash, cur_node)) {
				fprintf(stderr, "Warning: %s is not listed as a node in %s\n",
					cur_node, conf_path);

				if (strict) {
					fprintf(stderr, "Aborting.\n");
					exit(1);
				}
			}
			if (!hash_exists(&cmdline_nodes, cur_node)) {
				hash_add(&cmdline_nodes, cur_node, cur_node);
				++num_nodes;
			} else
				free(cur_node);
		} while (argv[optind] != NULL);
		dest_nodes = &cmdline_nodes;
		if (num_nodes <= 0) {
			fprintf(stderr, "No nodes were given\n");
			exit(1);
		}
	}

	pd = calloc(num_nodes, sizeof(*pd));
	if (pd == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	if (init_libnss_ssl(cert_db_dir) == -1) {
		fprintf(stderr, "Error: Unable to initialize the crypto subsystem.\n");
		exit(1);
	}

	sync_cluster_conf(doc, dest_nodes);

	hash_destroy(&node_hash);
	if (dest_nodes != &node_hash)
		hash_destroy(dest_nodes);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	free(cert_db_dir);
	free(conf_path);
	free(pd);

	shutdown_libnss_ssl();
	exit(0);
}

static int string_compare(void *l, void *r) {
	return (strcasecmp((const char *) l, (const char *) r));
}

static void hash_dtor(void *param __notused, void *data) {
	free(data);
}

static void ricci_conn_dtor(void *param __notused, void *data) {
	struct ricci_conn *c = (struct ricci_conn *) data;

	memset(c->buf, 0, c->bufsize);
	free(c->hostname);
	free(c->buf);
	free(c->cluster_name);
	PR_Close(c->fd);
	free(c);
}

static int ricci_conn_cmp(void *l, void *r) {
	struct ricci_conn *right = (struct ricci_conn *) r;

	if (l == right->fd)
		return (0);

	return (-1);
}

static void ricci_conn_alloc(void *data, void *param) {
	hash_t *conn_hash = (hash_t *) param;
	const char *name = (const char *) data;
	struct ricci_conn *c;
	PRStatus status;

	if (name == NULL || conn_hash == NULL)
		return;

	c = calloc(1, sizeof(*c));
	if (c == NULL)
		goto oom0;

	c->fd = get_ssl_socket(NULL, PR_TRUE);
	if (c->fd == NULL)
		goto oom1;

	c->hostname = strdup(name);
	if (c->hostname == NULL)
		goto oom2;

	c->buf = malloc(DEFAULT_READBUF_LEN);
	if (c->buf == NULL)
		goto oom3;

	c->bufsize = DEFAULT_READBUF_LEN;
	c->port = ricci_port;
	time(&c->last_active);

	status = nss_connect(c->fd, c->hostname, c->port);
	if (status == PR_FAILURE) {
		fprintf(stderr, "Unable to connect to %s\n", c->hostname);
		ricci_conn_dtor(NULL, c);
		return;
	}

	hash_add(conn_hash, c->fd, c);
	pd[pd_off].fd = c->fd;
	pd[pd_off++].in_flags = PR_POLL_WRITE | PR_POLL_EXCEPT;

	return;

oom3:
	free(c->hostname);
oom2:
	PR_Close(c->fd);
oom1:
	free(c);
oom0:
	fprintf(stderr, "Out of memory for %s\n", name);
	return;
}

static int sync_cluster_conf(xmlDocPtr conf_xml, hash_t *nodes) {
	hash_t conn_hash;
	int ret;

	ret = hash_init(&conn_hash, 5, ricci_conn_cmp, hash_int32, ricci_conn_dtor);
	if (ret == -1) {
		fprintf(stderr, "Out of memory\n");
		return (-1);
	}

	hash_iterate(nodes, ricci_conn_alloc, &conn_hash);

	while (pd_off > 0) {
		int i;
		ret = PR_Poll(pd, pd_off, PR_SecondsToInterval(1));
		if (ret == -1) {
			fprintf(stderr, "Error: poll: %m\n");
			break;
		}

		if (ret == 0)
			continue;

		/*
		** Iterate backwards to make pulling an invalid fd out of the
		** set (via ricci_clear_pollfd) faster.
		*/
		for (i = pd_off - 1 ; i >= 0 ; i--) {
			struct ricci_conn *c = hash_find(&conn_hash, pd[i].fd);
			if (c == NULL) {
				fprintf(stderr, "Unknown connection: %p.\n", pd[i].fd);
				hash_destroy(&conn_hash);
				return (-1);
			}

			if (pd[i].out_flags & PR_POLL_NVAL) {
				fprintf(stderr, "The connection to %s died unexpectedly\n",
					c->hostname);
				hash_remove(&conn_hash, pd[i].fd);
				pd_off = ricci_clear_pollfd(pd, i, pd_off);
				continue;
			}

			ret = ricci_conn_handler(c, &pd[i], conf_xml);
			if (ret < 0 || c->state == RICCI_STATE_DONE) {
				hash_remove(&conn_hash, pd[i].fd);
				pd_off = ricci_clear_pollfd(pd, i, pd_off);
			}
		}
	}

	hash_destroy(&conn_hash);
	return (0);
}

static void print_help(char const * const *argv) {
	fprintf(stderr,
"Usage: %s [options] [<host0> [... <hostN>]]\n\
   -f <cluster_conf>            Path to the cluster.conf file to propagate.\n\
   -c <NSS certificate DB>      Path to your cacert.pem file.\n\
   -i                           Increment the configuration version before propagating.\n\
   -h                           Print this help dialog.\n\
   -p <port number>             Connect to ricci on the specified port.\n\
   -w                           Exit with failure status if any warnings are issued.\n\
   -v                           Print version information.\n", argv[0]);
	exit(0);
}

static void print_version(void) {
	printf("ccs_sync v1.0\n\
Copyright (C) 2008-2009 Red Hat, Inc.\n\
License GPLv2: GNU GPL version 2\n");
}
