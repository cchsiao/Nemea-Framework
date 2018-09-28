#ifndef _UR_FIELDS_H_
#define _UR_FIELDS_H_

/************* THIS IS AUTOMATICALLY GENERATED FILE, DO NOT EDIT *************/
#include <unirec/unirec.h>

#define F_DST_IP   0
#define F_DST_IP_T   ip_addr_t
#define F_IP   1
#define F_IP_T   ip_addr_t
#define F_SRC_IP   2
#define F_SRC_IP_T   ip_addr_t
#define F_BYTES   3
#define F_BYTES_T   uint64_t
#define F_BAR   4
#define F_BAR_T   uint32_t
#define F_FOO   5
#define F_FOO_T   uint32_t
#define F_PACKETS   6
#define F_PACKETS_T   uint32_t
#define F_DST_PORT   7
#define F_DST_PORT_T   uint16_t
#define F_SRC_PORT   8
#define F_SRC_PORT_T   uint16_t
#define F_PROTOCOL   9
#define F_PROTOCOL_T   uint8_t
#define F_MESSAGE_CMP   10
#define F_MESSAGE_CMP_T   char
#define F_STR1   11
#define F_STR1_T   char
#define F_STR2   12
#define F_STR2_T   char
#define F_URL   13
#define F_URL_T   char

extern uint16_t ur_last_id;
extern ur_static_field_specs_t UR_FIELD_SPECS_STATIC;
extern ur_field_specs_t ur_field_specs;

#endif

