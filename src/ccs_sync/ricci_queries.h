/*
** Copyright (C) 2008-2009 Red Hat, Inc.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License, version 2,
** as published by the Free Software Foundation.
**
** Author: Ryan McCabe <ryan@redhat.com>
*/

#ifndef __RICCI_QUERIES_H
#define __RICCI_QUERIES_H

static const char ricci_set_conf_str[] =
	"<?xml version=\"1.0\" ?><ricci version=\"1.0\" function=\"process_batch\" async=\"false\"><batch><module name=\"cluster\"><request API_version=\"1.0\"><function_call name=\"set_cluster.conf\"><var type=\"boolean\" name=\"propagate\" mutable=\"false\" value=\"false\"/><var type=\"xml\" mutable=\"false\" name=\"cluster.conf\">%s</var></function_call></request></module></batch></ricci>\n";


static const char ricci_auth_str[] =
	"<?xml version=\"1.0\" ?><ricci version=\"1.0\" function=\"authenticate\" password=\"%s\"/>\n";

static const char ricci_unauth_str[] =
	"<?xml version=\"1.0\" ?><ricci version=\"1.0\" function=\"unauthenticate\" />\n";

#endif
