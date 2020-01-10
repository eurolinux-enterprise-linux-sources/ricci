

REQUEST_TAG   ='request'
RESPONSE_TAG  ='response'

FUNC_CALL_TAG ="function_call"
FUNC_RESP_TAG ="function_response"
SEQUENCE_TAG  ='sequence'


VARIABLE_TAG  ='var'

VARIABLE_TYPE_INT        = 'int'
VARIABLE_TYPE_INT_SEL    = 'int_select'
VARIABLE_TYPE_BOOL       = 'boolean'
VARIABLE_TYPE_STRING     = 'string'
VARIABLE_TYPE_STRING_SEL = 'string_select'
VARIABLE_TYPE_XML        = 'xml'

VARIABLE_TYPE_LIST_INT   = 'list_int'
VARIABLE_TYPE_LIST_STR   = 'list_str'
VARIABLE_TYPE_LIST_XML   = 'list_xml'


VARIABLE_TYPE_LISTENTRY  = 'listentry'
VARIABLE_TYPE_FLOAT      = 'float'




BD_TYPE = "block_device"
BD_HD_TYPE = "hard_drive"
BD_LV_TYPE = "logical_volume"
BD_PARTITION_TYPE = "partition"

BD_TEMPLATE = 'block_device_template'



MAPPER_TYPE = "mapper"
MAPPER_SYS_TYPE = "hard_drives"
MAPPER_VG_TYPE = "volume_group"
MAPPER_PT_TYPE = "partition_table"

SYSTEM_PREFIX = MAPPER_SYS_TYPE + ":"
VG_PREFIX     = MAPPER_VG_TYPE + ":"
PT_PREFIX     = MAPPER_PT_TYPE + ":"

MAPPER_SOURCES_TAG = "sources"
MAPPER_TARGETS_TAG = "targets"
MAPPER_MAPPINGS_TAG = "mappings"
MAPPER_NEW_SOURCES_TAG = "new_sources"
MAPPER_NEW_TARGETS_TAG = "new_targets"



CONTENT_TYPE = "content"
CONTENT_FS_TYPE = "filesystem"
CONTENT_NONE_TYPE = "none"
CONTENT_MS_TYPE = 'mapper_source'
CONTENT_HIDDEN_TYPE = 'hidden'



SOURCE_PV_TYPE = 'physical_volume'
SOURCE_PT_TYPE = 'partition_table_source'



PROPS_TAG = "properties"



