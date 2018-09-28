/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
// Tables are indexed by field ID
#include "fields.h"

char *ur_field_names_static[] = {
   "DST_IP",
   "IP",
   "SRC_IP",
   "BYTES",
   "BAR",
   "FOO",
   "PACKETS",
   "DST_PORT",
   "SRC_PORT",
   "PROTOCOL",
   "MESSAGE_CMP",
   "STR1",
   "STR2",
   "URL",
};
short ur_field_sizes_static[] = {
   16, /* DST_IP */
   16, /* IP */
   16, /* SRC_IP */
   8, /* BYTES */
   4, /* BAR */
   4, /* FOO */
   4, /* PACKETS */
   2, /* DST_PORT */
   2, /* SRC_PORT */
   1, /* PROTOCOL */
   -1, /* MESSAGE_CMP */
   -1, /* STR1 */
   -1, /* STR2 */
   -1, /* URL */
};
ur_field_type_t ur_field_types_static[] = {
   UR_TYPE_IP, /* DST_IP */
   UR_TYPE_IP, /* IP */
   UR_TYPE_IP, /* SRC_IP */
   UR_TYPE_UINT64, /* BYTES */
   UR_TYPE_UINT32, /* BAR */
   UR_TYPE_UINT32, /* FOO */
   UR_TYPE_UINT32, /* PACKETS */
   UR_TYPE_UINT16, /* DST_PORT */
   UR_TYPE_UINT16, /* SRC_PORT */
   UR_TYPE_UINT8, /* PROTOCOL */
   UR_TYPE_STRING, /* MESSAGE_CMP */
   UR_TYPE_STRING, /* STR1 */
   UR_TYPE_STRING, /* STR2 */
   UR_TYPE_STRING, /* URL */
};
ur_static_field_specs_t UR_FIELD_SPECS_STATIC = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 14};
ur_field_specs_t ur_field_specs = {ur_field_names_static, ur_field_sizes_static, ur_field_types_static, 14, 14, 14, NULL, UR_UNINITIALIZED};
