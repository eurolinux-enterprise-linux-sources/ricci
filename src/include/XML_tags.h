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

#ifndef __CONGA_XML_TAGS_H
#define __CONGA_XML_TAGS_H

#include "String.h"

// XML tags for various objects

// ### Variable ###

#define VARIABLE_TAG		String("var")

#define VARIABLE_INT		String("int")
#define VARIABLE_INT_SEL	String("int_select")
#define VARIABLE_BOOL		String("boolean")
#define VARIABLE_STR		String("string")
#define VARIABLE_STR_SEL	String("string_select")
#define VARIABLE_XML		String("xml")
#define VARIABLE_LIST_STR	String("list_str")
#define VARIABLE_LIST_INT	String("list_int")
#define VARIABLE_LIST_XML	String("list_xml")
#define VARIABLE_LISTENTRY	String("listentry")

#define REQUEST_TAG			String("request")
#define RESPONSE_TAG		String("response")
#define SEQUENCE_TAG		String("sequence")

#define FUNC_CALL_TAG		String("function_call")
#define FUNC_RESPONSE_TAG	String("function_response")

#define MOD_VERSION_TAG		String("API_version")

#endif
