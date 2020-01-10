/*
** Copyright (C) Red Hat, Inc. 2006-2009
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

#ifndef __CONGA_MODULE_H
#define __CONGA_MODULE_H

#include "XML.h"
#include "Variable.h"
#include "APIerror.h"

#include "String.h"
#include <list>
#include <map>

// name->variable map
typedef std::map<String, Variable> VarMap;

// name->function map
typedef std::map<String, VarMap (*)(const VarMap& args)> FcnMap;

// api->name->function map
typedef std::map<String, FcnMap> ApiFcnMap;

class Module
{
	public:
		virtual ~Module();
		virtual XMLObject process(const XMLObject& request);

	protected:
		Module(const ApiFcnMap& api_fcns);
};

int stdin_out_module_driver(Module& module, int argc, char **argv);

#endif
