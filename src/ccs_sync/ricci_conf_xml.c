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
#include <errno.h>

#include <nspr.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "ricci_list.h"
#include "ricci_net.h"
#include "ricci_conf_xml.h"

int xmldoc_to_str(xmlDocPtr doc, char **xml_str, size_t *len) {
	int ret;
	xmlBuffer *buf;
	xmlNode *root;

	*xml_str = NULL;

	root = xmlDocGetRootElement(doc);
	if (root == NULL)
		return (-1);

	buf = xmlBufferCreate();
	ret = xmlNodeDump(buf, doc, root, 1, 0);
	if (ret == -1) {
		xmlBufferFree(buf);
		return (-1);
	}

	if (buf->content != NULL) {
		*xml_str = (char *) buf->content;
		*len = buf->use;
		buf->content = NULL;
	}

	xmlBufferFree(buf);
	return (0);	
}

xmlNode *getnodename(xmlNode *root, const char *name) {
	xmlNode *cur;

	for (cur = root->children ; cur != NULL ; cur = cur->next) {
		if (cur->type == XML_ELEMENT_NODE &&
			!strcasecmp((const char *) cur->name, name))
		{
			return (cur);
		}
	}

	return (NULL);
}

xmlNode *getnodenameprop(xmlNode *root, const char *name, const char *prop) {
	xmlNode *cur;

	for (cur = root->children ; cur != NULL ; cur = cur->next) {
		if (cur->type == XML_ELEMENT_NODE &&
			!strcasecmp((const char *) cur->name, name))
		{
			xmlChar *c = xmlGetProp(cur, BAD_CAST "name");
			if (c != NULL) {
				if (!strcasecmp((const char *) c, prop)) {
					xmlFree(c);
					return (cur);
				}
				xmlFree(c);
			}
		}
	}

	return (NULL);
}

int32_t increment_conf_version(xmlNode *root) {
	char *version_str;
	unsigned long version;
	char *p;
	int ret;

	if (strcasecmp((const char *) root->name, "cluster"))
		return (-1);

	version_str = (char *) xmlGetProp(root, (const xmlChar *) "config_version");
	if (version_str == NULL)
		return (-1);

	version = strtoul(version_str, &p, 10);
	if (*p != '\0') {
		free(version_str);
		return (-1);
	}
	free(version_str);
	version++;

	version_str = NULL;
	ret = asprintf(&version_str, "%lu", version);
	if (ret < 1 || version_str == NULL)
		return (-1);
	xmlSetProp(root, (const xmlChar *) "config_version",
		(const xmlChar *) version_str);

	free(version_str);
	return (version);
}

int get_cluster_nodes(xmlNode *root, hash_t *node_hash) {
	xmlNode *cur;
	xmlChar *nodename;
	int count = 0;

	xmlNode *cnodes = getnodename(root, "clusternodes");
	if (cnodes == NULL)
		return (-1);

	for (cur = cnodes->children ; cur != NULL ; cur = cur->next) {
		if (cur->type != XML_ELEMENT_NODE ||
			strcasecmp((const char *) cur->name, "clusternode"))
		{
			continue;
		}

		nodename = xmlGetProp(cur, BAD_CAST "name");
		if (nodename != NULL) {
			hash_add(node_hash, nodename, nodename);
			++count;
		}
	}
	return (count);
}

int ricci_set_conf_status(const char *hostname, xmlDocPtr response) {
	xmlNode *root;
	xmlNode *cur;
	xmlNode *fresp_node;
	xmlNode *cmd_status;;
	xmlChar *prop_val = NULL;
	int ret;

	root = xmlDocGetRootElement(response);
	if (root == NULL || strcasecmp((const char *) root->name, "ricci")) {
		fprintf(stderr,
			"%s: Remote Error: expected \"ricci\" tag, got \"%s\"\n",
			hostname, root->name);
		return (-1);
	}
	
	cur = getnodename(root, "batch");
	if (cur == NULL)
		return (-1);

	cur = getnodename(cur, "module");
	if (cur == NULL)
		return (-1);

	cur = getnodename(cur, "response");
	if (cur == NULL)
		return (-1);

	fresp_node = getnodename(cur, "function_response");
	if (fresp_node == NULL)
		return (-1);

	prop_val = xmlGetProp(fresp_node, BAD_CAST "function_name");
	if (prop_val == NULL)
		return (-1);

	ret = strcasecmp((const char *) prop_val, "set_cluster.conf");
	xmlFree(prop_val);
	if (ret != 0)
		return (-1);

	cmd_status = getnodename(fresp_node, "var");
	if (cmd_status == NULL)
		return (-1);

	prop_val = xmlGetProp(cmd_status, BAD_CAST "name");
	if (prop_val == NULL)
		return (-1);

	ret = strcasecmp((const char *) prop_val, "success");
	xmlFree(prop_val);
	if (ret != 0)
		return (-1);

	prop_val = xmlGetProp(cmd_status, BAD_CAST "value");
	if (prop_val == NULL)
		return (-1);
	ret = !strcasecmp((const char *) prop_val, "true");
	xmlFree(prop_val);

	if (!ret) {
		xmlNode *error_node;
		xmlChar *err_str = NULL;

		error_node = getnodenameprop(fresp_node, "var", "error_description");
		if (error_node != NULL)
			err_str = xmlGetProp(error_node, BAD_CAST "value");

		if (err_str != NULL) {
			fprintf(stderr, "%s: Remote Error: %s\n",
				hostname, (const char *) err_str);
			xmlFree(err_str);
		}
	}

	return (ret);
}

int ricci_xml_authed(struct ricci_conn *c, xmlDocPtr doc) {
	xmlChar *auth_str;
	xmlChar *cluster_name;
	int authed = 0;

	xmlNode *root = xmlDocGetRootElement(doc);
	if (root == NULL || strcasecmp((const char *) root->name, "ricci"))
		return (-1);

	auth_str = xmlGetProp(root, BAD_CAST "authenticated");
	if (auth_str == NULL)
		return (-1);

	if (c->cluster_name != NULL) {
		free(c->cluster_name);
		c->cluster_name = NULL;
	}

	cluster_name = xmlGetProp(root, BAD_CAST "clustername");
	if (cluster_name != NULL) {
		c->cluster_name = strdup((const char *) cluster_name);
		xmlFree(cluster_name);
	}

	if (!strcasecmp((const char *) auth_str, "true"))
		authed = 1;
	else
		authed = 0;

	free(auth_str);
	return (authed);
}
