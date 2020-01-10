/*  
** Copyright (C) 2008-2009 Red Hat, Inc.
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#ifndef __RICCI_CONF_XML
#define __RICCI_CONF_XML

struct ricci_conn;

int xmldoc_to_str(xmlDocPtr doc, char **xml_str, size_t *len);
xmlNode *getnodename(xmlNode *root, const char *name);
int32_t increment_conf_version(xmlNode *root);
int get_cluster_nodes(xmlNode *root, hash_t *node_hash);
int ricci_xml_authed(struct ricci_conn *c, xmlDocPtr doc);
int ricci_set_conf_status(const char *hostname, xmlDocPtr response);

#endif
