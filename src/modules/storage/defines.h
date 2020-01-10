/*
  Copyright Red Hat, Inc. 2005-2008

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2, or (at your option) any
  later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; see the file COPYING.  If not, write to the
  Free Software Foundation, Inc.,  675 Mass Ave, Cambridge,
  MA 02139, USA.
*/
/*
 * Author: Stanko Kupcevic <kupcevic@redhat.com>
 */


#ifndef defines_h
#define defines_h

#include "String.h"



// XML tags for various objects



#define MAPPER_TYPE_TAG           String("mapper")
#define MAPPER_SYS_TYPE           String("hard_drives")
#define MAPPER_VG_TYPE            String("volume_group")
#define MAPPER_PT_TYPE            String("partition_table")
#define MAPPER_MDRAID_TYPE        String("mdraid")
#define MAPPER_ATARAID_TYPE       String("ataraid")
#define MAPPER_MULTIPATH_TYPE     String("multipath")
#define MAPPER_CRYPTO_TYPE        String("crypto")
#define MAPPER_iSCSI_TYPE         String("iSCSI")

#define MAPPER_TEMPLATE_TYPE_TAG  String("mapper_template")

#define MAPPER_SOURCES_TAG        String("sources")
#define MAPPER_TARGETS_TAG        String("targets")
#define MAPPER_MAPPINGS_TAG       String("mappings")
#define MAPPER_NEW_SOURCES_TAG    String("new_sources")
#define MAPPER_NEW_TARGETS_TAG    String("new_targets")



#define BD_TYPE_TAG            String("block_device")
#define BD_HD_TYPE             String("hard_drive")
#define BD_LV_TYPE             String("logical_volume")
#define BD_PART_TYPE           String("partition")
#define BD_MDRAID_TYPE         String("mdraid_target")

#define BD_TEMPLATE_TYPE_TAG   String("block_device_template")




#define CONTENT_TYPE_TAG       String("content")
#define CONTENT_MS_TYPE        String("mapper_source")
#define CONTENT_FS_TYPE        String("filesystem")
#define CONTENT_NONE_TYPE      String("none")
#define CONTENT_UNUSABLE_TYPE  String("hidden")

#define CONTENT_TEMPLATE_TYPE_TAG    String("content_template")

#define CONTENT_REPLACEMENTS_TAG     String("available_contents")
#define CONTENT_NEW_CONTENT_TAG      String("new_content")


#define SOURCE_PV_TYPE     String("physical_volume")
#define SOURCE_PT_TYPE     String("partition_table_source")
#define SOURCE_MDRAID_TYPE String("mdraid_source")



#define LVM_BIN_PATH        String("/sbin/lvm")


#define SYS_PREFIX          (MAPPER_SYS_TYPE + ":")
#define VG_PREFIX           (MAPPER_VG_TYPE + ":")
#define PT_PREFIX           (MAPPER_PT_TYPE + ":")
#define MDRAID_PREFIX       (MAPPER_MDRAID_TYPE + ":")



#define REQUEST_TAG         String("request")
#define RESPONSE_TAG        String("response")
#define SEQUENCE_TAG        String("sequence")


#define FUNC_CALL_TAG       String("function_call")
#define FUNC_RESPONSE_TAG   String("function_response")


const static String NAMES_ILLEGAL_CHARS("~`!@#$%^&*()+=[]{}|\\:;\"'<>?");



#endif  // defines_h
