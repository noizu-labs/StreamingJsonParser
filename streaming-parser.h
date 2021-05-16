/*!
 * @file streaming-parser.h
 * @headerfile streaming-parser.h "streaming-parser.h"
 *
 * @author Keith Brings
 * @copyright Noizu Labs, Inc.
 */

#ifndef __STREAMING_PARSER_H__ 
#define __STREAMING_PARSER_H__
#include "noizu_trie_a.h"

#ifndef NSP_DBG_ENABLED
#define NSP_DBG_ENABLED FALSE
#endif

#ifndef MAXIMUM_NULLABLE_STRING_OVER_ALLOCATION
#define MAXIMUM_NULLABLE_STRING_OVER_ALLOCATION 128
#endif

#ifndef DEFAULT_NULLABLE_STRING_SIZE
#define DEFAULT_NULLABLE_STRING_SIZE 11
#endif 

#ifndef NULLABLE_STRING_UPSIZE_CUTOFF
#define NULLABLE_STRING_UPSIZE_CUTOFF 5
#endif 


#ifndef MINIMUM_NULLABLE_STRING_SIZE
#define MINIMUM_NULLABLE_STRING_SIZE 5
#endif 

 //------------------------------
 // Override some defines used for ESP8266 for cross compatibility
 //------------------------------
#ifdef GENERIC_MODE
#include <stdio.h>

#ifdef UNITY_TEST
#include "unity_memory.h"
#else
#include <corecrt_malloc.h>
#endif

#include <string.h>

#define ICACHE_FLASH_ATTR
typedef unsigned char       uint8_t;
typedef signed char         sint8_t;
typedef signed char         int8_t;
typedef unsigned short      uint16_t;
typedef signed short        sint16_t;
typedef signed short        int16_t;
typedef unsigned int        uint32_t;
typedef signed int          sint32_t;
typedef signed int          int32_t;
typedef signed long long    sint64_t;
typedef unsigned long long  uint64_t;
typedef unsigned long long  u_int64_t;

typedef unsigned char   bool;
#define BOOL            bool
#define true            (1)
#define false           (0)
#define TRUE            true
#define FALSE           false

typedef struct offset_buffer {
	uint8_t* buffer; // Current buffer, (may be reallocated between calls).
	uint32_t buffer_size; // Total length of buffer from buffer_pos to end of new data.
	uint32_t allocated_buffer; // Total space allocated for buffer.
	uint32_t buffer_pos; // Current starting position in buffer.
} offset_buffer;

#define os_sprintf sprintf
#define os_printf printf
#define os_printf_plus printf

#define LOG_ERROR(format, ...) os_printf("[LOG ERROR]@%s:%d: " format "\r\n", __func__, __LINE__, ##__VA_ARGS__);
#define LOG_WARN(format, ...) os_printf("[LOG WARN]@%s:%d: " format "\r\n", __func__, __LINE__, ##__VA_ARGS__);

#define os_free free
#define os_malloc malloc
#define os_calloc(s) calloc(s, 1)
#define os_realloc realloc
#define os_zalloc(s) calloc(s, 1)

#define os_memcmp memcmp
#define os_memcpy memcpy
#define os_memmove memmove
#define os_memset memset
#define os_strcat strcat
#define os_strchr strchr
#define os_strcmp strcmp
#define os_strcpy strcpy
#define os_strlen strlen
#define os_strncmp strncmp
#define os_strncpy strncpy
#define os_strstr strstr
#else
#include "osapi.h"
#include "c_types.h"
#include "mem.h"
#include "ip_addr.h"
#include "httpc.h"
typedef request_args_t offset_buffer;
#endif

typedef uint8_t noizu_realloc_code;
#define NOZ_DYNAMIC_ARRAY__ARG_ERROR 0x01
#define NOZ_DYNAMIC_ARRAY__OK_FLAG 0x20
#define NOZ_DYNAMIC_ARRAY__OK_ALLOCATED 0x21
#define NOZ_DYNAMIC_ARRAY__OK_RESIZED 0x22
#define NOZ_DYNAMIC_ARRAY__OK_UNCHANGED 0x23
#define NOZ_DYNAMIC_ARRAY__OVERFLOW 0x03
#define NOZ_DYNAMIC_ARRAY__ALLOC_FAIL 0x04;


//-----------------------------
// Nullable Types
//-----------------------------

// We use 0 for null value to make wiping nullable fields more straight forward with zalloc/memset 0
#define NULL_VALUE 0
#define NOT_NULL_VALUE 1

// Supported type control
#ifndef NSP_STREAMING_PARSER_TYPES
#define NSP_SUPPORT__NULLABLE_SINT7_T 1
#define NSP_SUPPORT__NULLABLE_UINT7_T 1

#define NSP_SUPPORT__NULLABLE_SINT15_T 1
#define NSP_SUPPORT__NULLABLE_UINT15_T 1

#define NSP_SUPPORT__NULLABLE_SINT31_T 1
#define NSP_SUPPORT__NULLABLE_UINT31_T 1

#define NSP_SUPPORT__NULLABLE_SINT63_T 1
#define NSP_SUPPORT__NULLABLE_UINT63_T 1

#define NSP_SUPPORT__NULLABLE_SINT64_T 1
#define NSP_SUPPORT__NULLABLE_UINT64_T 1


#define NSP_SUPPORT__NULLABLE_INT7_T 1
#define NSP_SUPPORT__NULLABLE_INT15_T 1
#define NSP_SUPPORT__NULLABLE_INT31_T 1
#define NSP_SUPPORT__NULLABLE_INT63_T 1
#define NSP_SUPPORT__NULLABLE_INT64_T 1

#define NSP_SUPPORT__NULLABLE_BOOL_T 1
#endif

#define NSP_STRUCT_SIZE_CHECK 1


#ifdef ENABLE__UNION_NULLABLE_TYPE

#if defined(NSP_SUPPORT__NULLABLE_INT7_T) && NSP_SUPPORT__NULLABLE_INT7_T
/*!
 * @brief 7 bit nullable
 */
typedef struct nullable_int7_t {
	union {
		struct {
			uint8_t null : 1;
			uint8_t value : 7;
		};
		struct {
			uint8_t _signed_null : 1;
			sint8_t signed_value : 7;
		};
	};
} nullable_int7_t;

#if defined(NSP_STRUCT_SIZE_CHECK) && NSP_STRUCT_SIZE_CHECK
int NSP_CONSTRIANT__nullable_int7_t[(1 - sizeof(nullable_int7_t)) + 1];
#endif
#endif

#if defined(NSP_SUPPORT__NULLABLE_INT15_T) && NSP_SUPPORT__NULLABLE_INT15_T
/*!
 * @brief 15 bit nullable
 */
typedef struct nullable_int15_t {
	union {
		struct {
			uint16_t null : 1;
			uint16_t value : 15;
		};
		struct {
			uint16_t _signed_null : 1;
			sint16_t signed_value : 15;
		};
	};
} nullable_int15_t;

#if defined(NSP_STRUCT_SIZE_CHECK) && NSP_STRUCT_SIZE_CHECK
int NSP_CONSTRIANT__nullable_int15_t[(2 - sizeof(nullable_int15_t)) + 1];
#endif
#endif

#if defined(NSP_SUPPORT__NULLABLE_INT31_T) && NSP_SUPPORT__NULLABLE_INT31_T
/*!
 * @brief 31 bit nullable
 */
typedef struct nullable_int31_t {
	union {
		struct {
			uint32_t null : 1;
			uint32_t value : 31;
		};
		struct {
			uint32_t _signed_null : 1;
			sint32_t signed_value : 31;
		};
	};
} nullable_int31_t;

#if defined(NSP_STRUCT_SIZE_CHECK) && NSP_STRUCT_SIZE_CHECK
int NSP_CONSTRIANT__nullable_int31_t[(4 - sizeof(nullable_int31_t)) + 1];
#endif
#endif


#if defined(NSP_SUPPORT__NULLABLE_INT63_T) && NSP_SUPPORT__NULLABLE_INT63_T
/*!
 * @brief 63 bit nullable
 */
typedef struct nullable_int63_t {
	union {
		struct {
			uint64_t null : 1;
			uint64_t value : 63;
		};
		struct {
			uint64_t _signed_null : 1;
			sint64_t signed_value : 63;
		};
	};
} nullable_int63_t;

#if defined(NSP_STRUCT_SIZE_CHECK) && NSP_STRUCT_SIZE_CHECK
int NSP_CONSTRIANT__nullable_int63_t[(8 - sizeof(nullable_int63_t)) + 1];
#endif
#endif


#if defined(NSP_SUPPORT__NULLABLE_INT64_T) && NSP_SUPPORT__NULLABLE_INT64_T
/*!
 * @brief 64 bit nullable
 */
typedef struct nullable_int64_t {
	union {
		struct {
			uint8_t null : 1;
			uint64_t value;
		};
		struct {
			uint8_t _signed_null : 1;
			sint64_t signed_value;
		};
	};
} nullable_int64_t;

#if defined(NSP_STRUCT_SIZE_CHECK) && NSP_STRUCT_SIZE_CHECK
int NSP_CONSTRIANT__nullable_int64_t[(17 - sizeof(nullable_int64_t)) + 1];
#endif
#endif





#endif






#if defined(NSP_SUPPORT__NULLABLE_SINT7_T) && NSP_SUPPORT__NULLABLE_SINT7_T
/*!
 * @brief 7 bit nullable signed value
 */
typedef struct nullable_sint7_t {
	uint8_t null : 1;
	sint8_t value : 7;
} nullable_sint7_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_UINT7_T) && NSP_SUPPORT__NULLABLE_UINT7_T
/*!
 * @brief 7 bit nullable unsigned value
 */
typedef struct nullable_uint7_t {
	uint8_t null : 1;
	uint8_t value : 7;
} nullable_uint7_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_SINT15_T) && NSP_SUPPORT__NULLABLE_SINT15_T
/*!
 * @brief 15 bit nullable signed value
 */
typedef struct nullable_sint15_t {
	uint16_t null : 1;
	sint16_t value : 15;
} nullable_sint15_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_UINT15_T) && NSP_SUPPORT__NULLABLE_UINT15_T
/*!
 * @brief 15 bit nullable unsigned value
 */
typedef struct nullable_uint15_t {
	uint16_t value : 15;
	uint16_t null : 1;
} nullable_uint15_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_SINT31_T) && NSP_SUPPORT__NULLABLE_UINT31_T
/*!
 * @brief 31 bit nullable signed value
 */
typedef struct nullable_sint31_t {
	sint32_t value : 31;
	uint32_t null : 1;
} nullable_sint31_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_UINT31_T) && NSP_SUPPORT__NULLABLE_UINT31_T
/*!
 * @brief 31 bit nullable unsigned value
 */
typedef struct nullable_uint31_t {
	uint32_t value : 31;
	uint32_t null : 1;
} nullable_uint31_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_SINT63_T) && NSP_SUPPORT__NULLABLE_SINT63_T
/*!
 * @brief 63 bit nullable signed value
 */
typedef struct nullable_sint63_t {
	sint64_t value : 63;
	uint64_t null : 1;
} nullable_sint63_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_UINT63_T) && NSP_SUPPORT__NULLABLE_UINT63_T
/*!
 * @brief 63 bit nullable unsigned value
 */
typedef struct nullable_uint63_t {
	uint64_t value : 63;
	uint64_t null : 1;
} nullable_uint63_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_SINT64_T) && NSP_SUPPORT__NULLABLE_SINT64_T
/*!
 * @brief 64 bit nullable signed value
 */
typedef struct nullable_sint64_t {
	sint64_t value;
	uint8_t null;
} nullable_sint64_t;
#endif

#if defined(NSP_SUPPORT__NULLABLE_UINT64_T) && NSP_SUPPORT__NULLABLE_UINT64_T
/*!
 * @brief 64 bit nullable unsigned value
 */
typedef struct nullable_uint64_t {
	uint64_t value;
	uint8_t null;
} nullable_uint64_t;
#endif


#if defined(NSP_SUPPORT__NULLABLE_BOOL_T) && NSP_SUPPORT__NULLABLE_BOOL_T
#if defined(NSP_SUPPORT__NULLABLE_UINT7_T) && NSP_SUPPORT__NULLABLE_UINT7_T
typedef nullable_uint7_t nullable_bool_t;
#elif defined(NSP_SUPPORT__NULLABLE_UINT15_T) && NSP_SUPPORT__NULLABLE_UINT15_T
typedef nullable_uint15_t nullable_bool_t;
#elif defined(NSP_SUPPORT__NULLABLE_UINT31_T) && NSP_SUPPORT__NULLABLE_UINT31_T
typedef nullable_uint31_t nullable_bool_t;
#else
typedef struct nullable_bool_t {
	uint8_t value : 7;
	uint8_t null : 1;
} nullable_sint15_t;
#endif
#endif


//------------------------------
// Nullable Trie Token
//------------------------------
#if ((TRIE_A_UNIT_BITS % 4) == 0) 
#define NULLABLE_TOKEN_BITS 4
#else 
#define NULLABLE_TOKEN_BITS (TRIE_A_UNIT_BITS % 4) 
#endif

#if TRIE_A_UNIT_BITS < 64 
#if TRIE_A_UNIT_SIGNED 
typedef struct nullable_token_t {
	sint64_t value : TRIE_A_UNIT_BITS;
	uint8_t null : NULLABLE_TOKEN_BITS;
} nullable_token_t;
#else 
typedef struct nullable_token_t {
	TRIE_A_UNIT value : TRIE_A_UNIT_BITS;
	uint8_t null : NULLABLE_TOKEN_BITS;
} nullable_token_t;
#endif
#else 
typedef struct nullable_token_t {
	TRIE_A_UNIT value;
	uint8_t null;
} nullable_token_t;
#endif

/*!
 * @brief float with nullable flag
 */
typedef struct nullable_float_t {
	float value;
	uint8_t null;
	uint8_t unit_code;
} nullable_float_t;

/*!
 * @brief double with nullable flag
 */
typedef struct nullable_double_t {
	double value;
	uint8_t null;
	uint8_t unit_code; // Due to memory alignment no additional size is introduced by this field. 
} nullable_double_t;

/*!
 * @brief string pointer with nullable flag
 */
typedef struct nullable_string_t {
	uint16_t null : 1;
	uint16_t free_lock : 1;
	uint16_t allocated : 14;
	uint16_t index;
	uint8_t counter;
	uint8_t* value;
} nullable_string_t;




//! @brief struct for tracking time of day (hour, minute) with null flag.
typedef struct nullable_time {
	uint8_t null : 4;
	uint8_t hour : 6;
	uint8_t minute : 6;
} nullable_time;

//! @brief struct for tracking time and date (year, month, day, hour, minute, second) with null flag.
typedef struct nullable_date_time {
	uint8_t null : 1;
	uint16_t year : 12;
	uint8_t month : 4;
	uint8_t day : 5;
	uint8_t hour : 6;
	uint8_t minute : 6;
	uint8_t second : 6;

	// offset
	uint8_t offset_sign : 1;
	uint8_t offset_hour : 7;
	uint8_t offset_minute;
} nullable_date_time;



/**
 * @brief pseudo template struct to support dynamic arrays.
 * To use call NOIZU_DYNAMIC_ARRAY(type, name) macro which will create a untion of the dynamic_array named raw and the same fields and pointer using the specified type and name.
 *  typedef struct test_da {
 *  NOIZU_DYNAMIC_ARRAY(uint8_t, str);
 * } test_da;
 *
 */
typedef struct dynamic_array {
	uint16_t allocated;
	uint16_t size;
	void* raw_array;
} dynamic_array;

#define NOIZU_DYNAMIC_ARRAY(PT_TYPE, PT_NAME) union { \
   dynamic_array _bare_dynamic_array; \
   struct { \
		uint16_t allocated; \
		uint16_t size; \
        PT_TYPE * PT_NAME; \
	}; \
}




#define DECREMENT_ONLY 0x00
#define FREE_CONTENTS 0x01
#define FORCE_FREE_CONTENTS 0x02
#define FREE_LOCKED 0x04

#define FREE_STRUCT 0x10
#define FORCE_FREE_STRUCT 0x20
#define RELEASE_STRUCT 0x40

#define FREE_DEFAULT (FREE_CONTENTS | FREE_STRUCT | RELEASE_STRUCT)


//-----------------------------
// Enums
//-----------------------------

typedef uint16_t nullable_type;
#define INT_TYPE_FLAG 0x10
#define SIGNED_TYPE_FLAG 0x20
#define UNION_NULLABLE_TYPE_FLAG 0x40
#define DECIMAL_TYPE_FLAG 0x80
#define BOOL_TYPE_FLAG 0x100
#define STRING_TYPE_FLAG 0x200
#define TOKEN_TYPE_FLAG 0x200
#define TIME_TYPE_FLAG 0x400

#define UINT7_TYPE (INT_TYPE_FLAG | 0x01)
#define UINT15_TYPE (INT_TYPE_FLAG | 0x02)
#define UINT31_TYPE (INT_TYPE_FLAG | 0x03)
#define UINT63_TYPE (INT_TYPE_FLAG | 0x04)
#define UINT64_TYPE (INT_TYPE_FLAG | 0x05)
#define SINT7_TYPE (INT_TYPE_FLAG | SIGNED_TYPE_FLAG | 0x01)
#define SINT15_TYPE (INT_TYPE_FLAG | SIGNED_TYPE_FLAG |0x02)
#define SINT31_TYPE (INT_TYPE_FLAG | SIGNED_TYPE_FLAG |0x03)
#define SINT63_TYPE (INT_TYPE_FLAG | SIGNED_TYPE_FLAG |0x04)
#define SINT64_TYPE (INT_TYPE_FLAG | SIGNED_TYPE_FLAG |0x05)
#define FLOAT_TYPE (DECIMAL_TYPE_FLAG | 0x01)
#define DOUBLE_TYPE (DECIMAL_TYPE_FLAG | 0x02)
#define BOOL_TYPE (BOOL_TYPE_FLAG | 0x01)
#define DATE_TIME_TYPE (TIME_TYPE_FLAG | 0x02)
#define TIME_TYPE (TIME_TYPE_FLAG | 0x01)


/*!
* Json types
*/
typedef uint8_t json_value_type;
#define JSON_ERROR_VALUE 0
#define JSON_TRUE_VALUE 1
#define JSON_FALSE_VALUE 2
#define JSON_NUMERIC_VALUE 3
#define JSON_QUOTE_VALUE 4
#define JSON_DOUBLE_QUOTE_VALUE 5
#define JSON_LIST_VALUE 6
#define JSON_STRUCT_VALUE 7
#define JSON_NULL_VALUE 8


/*!
* Parser CallBack Response Commands
*/
typedef uint8_t jsp_cb_command;
#define JSPC_PROCEED 0
#define JSPC_RETURN 1
#define JSPC_REBASE 2
#define JSPC_POP 3



/*!
* Streaming parser state.
*/
typedef uint8_t json_parse_state;
#define PS_ADVANCE_KEY 0
#define PS_PARSE_KEY 1
#define PS_PARSE_KEY_COMPLETE 2
#define PS_ADVANCE_VALUE 3
#define PS_ADVANCE_VALUE_COMPLETE 4
#define PS_CAPTURE_VALUE 5
#define PS_ADVANCE_LIST_ITEM 6
#define PS_CAPTURE_LIST_ITEM 7
#define PS_LIST_COMPLETE 8
#define PS_LIST_ITEM_COMPLETE 9
#define PS_STRUCT_COMPLETE 10
#define PS_COMPLETE 11
#define PS_EXTRACT_VALUE 12
#define PS_END_OF_BUFFER 13
#define PS_ERROR 14


//-----------------------------
// Forward Declaration
//-----------------------------
struct json_parser;

//-----------------------------
// Call Back TypeDefs
//-----------------------------

/*!
 * Streaming Parser CallBack Definition
 */
typedef jsp_cb_command(*json_streaming_parser_event_cb)(json_parse_state state, struct json_parser* parser);

//-----------------------------
// Structs
//-----------------------------

/*!
 * @brief json_praser structure.
 *
 */
typedef struct json_parser {
	// Token
	noizu_trie_a* trie;
	TRIE_A_UNIT trie_index;
	TRIE_A_UNIT token;
	TRIE_A_UNIT parent;

	// Parsing
	json_parse_state parse_state;
	uint8_t active : 1;
	uint8_t char_skip : 1;
	uint8_t escape_char : 1;
	uint8_t null_track : 1;

	// Quote trackers and in quote should be collapsible. in_quote additionally is likely redundant for quote_track
	uint8_t in_s_quote : 2; // 1 == single quote, 2 == double quote, 0 = no
	uint8_t in_d_quote : 2;
	uint8_t s_quote_track;
	uint8_t d_quote_track;
	uint8_t brace_track;
	uint8_t bracket_track;

	// List trackers
	uint16_t list_index;
	uint16_t list_members;

	// Value Type
	json_value_type value_type;
	json_value_type previous_value_type;

	// Buffer Offsets
	uint16_t key_start;
	uint16_t key_close;
	uint16_t value_start;
	uint16_t value_close;

	// Stack Navigation & Depth
	struct json_parser* child;
	struct json_parser* parent_p;
	uint8_t depth;

	// Data Handler
	json_streaming_parser_event_cb cb;

	// Incoming data stream, using a dynamically allocated buffer and offset tracking mechanism. 
	offset_buffer* req;

	// output reference.
	void* output;
} json_parser;

/*!
 * @brief post realloc callback for preparing memory for nested pointers that require it.
 */
typedef noizu_realloc_code(post_resize_cb)(dynamic_array* pt, uint16_t new_size);

//-------------------------------------
// API
//-------------------------------------

// Main Entry Point
jsp_cb_command ICACHE_FLASH_ATTR json_streaming_parser(json_parser* parser);

// Parse Loop and Delegates
json_parse_state ICACHE_FLASH_ATTR json_process(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_KEY(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__PARSE_KEY(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_VALUE(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__CAPTURE_VALUE(json_parser* parser);

// Helper Methods (tweak state)
void ICACHE_FLASH_ATTR json_parser_globals_reset();
void ICACHE_FLASH_ATTR reset_json_parser(json_parser* parser);
void ICACHE_FLASH_ATTR shift_json_parser(uint16_t shift, json_parser* parser);
void ICACHE_FLASH_ATTR free_json_parser(json_parser* base, BOOL free_pointer);
json_parser* ICACHE_FLASH_ATTR spawn_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR drop_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR init_json_parser(offset_buffer*, noizu_trie_a* trie, json_streaming_parser_event_cb cb, void* output);
json_parser* ICACHE_FLASH_ATTR top_parser(json_parser* parser);

// Debug Output
#if defined(NSP_DBG_ENABLED) && NSP_DBG_ENABLED
void ICACHE_FLASH_ATTR print_json_parser_path(json_parser* parser, uint32_t line);
void ICACHE_FLASH_ATTR print_json_parser(json_parser* parser, uint8_t offset);
#else 
#define print_json_parser_path(parser, line)
#define print_json_parser(json_parser, offset)
#endif



// Parsing Helpers
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_token(json_parser* parser, noizu_trie_a* trie, nullable_token_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_key(json_parser* parser, nullable_string_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_string(json_parser* parser, nullable_string_t* out);

//-----------------------------------------
// Input parser
//------------------------------------------
// Streamlined (for code size) parsers. 
uint8_t ICACHE_FLASH_ATTR json_parser__extract_nullable(json_parser* parser, nullable_type type, void* out);
uint8_t ICACHE_FLASH_ATTR extract_nullable(uint8_t* pt, uint16_t length, nullable_type type, void* out);

uint8_t ICACHE_FLASH_ATTR json_parser__extract_time(json_parser* parser, nullable_time* out);
uint8_t ICACHE_FLASH_ATTR json_parser__extract_date_time(json_parser* parser, nullable_date_time* out);
//-----------------------------------------
// Backwards compatibility macros
//------------------------------------------
#define json_parser__extract_bool(parser, out) json_parser__extract_nullable((parser), BOOL_TYPE, (void*)(out))

#define json_parser__extract_float(parser, out) json_parser__extract_nullable((parser), FLOAT_TYPE, (void*)(out))
#define json_parser__extract_double(parser, out) json_parser__extract_nullable((parser), DOUBLE_TYPE, (void*)(out))

#define json_parser__extract_sint7(parser, out) json_parser__extract_nullable((parser), SINT7_TYPE, (void*)(out))
#define json_parser__extract_sint15(parser, out) json_parser__extract_nullable((parser), SINT15_TYPE, (void*)(out))
#define json_parser__extract_sint31(parser, out) json_parser__extract_nullable((parser), SINT31_TYPE, (void*)(out))
#define json_parser__extract_sint63(parser, out) json_parser__extract_nullable((parser), SINT63_TYPE, (void*)(out))
#define json_parser__extract_sint64(parser, out) json_parser__extract_nullable((parser), SINT64_TYPE, (void*)(out))

#define json_parser__extract_uint7(parser, out) json_parser__extract_nullable((parser), UINT7_TYPE, (void*)(out))
#define json_parser__extract_uint15(parser, out) json_parser__extract_nullable((parser), UINT15_TYPE, (void*)(out))
#define json_parser__extract_uint31(parser, out) json_parser__extract_nullable((parser), UINT31_TYPE, (void*)(out))
#define json_parser__extract_uint63(parser, out) json_parser__extract_nullable((parser), UINT63_TYPE, (void*)(out))
#define json_parser__extract_uint64(parser, out) json_parser__extract_nullable((parser), UINT64_TYPE, (void*)(out))

#define extract_nullable_float(pt, out) extract_nullable((pt), 0, FLOAT_TYPE, (void*)(out))
#define extract_nullable_double(pt, out) extract_nullable((pt), 0, DOUBLE_TYPE, (void*)(out))

#define extract_nullable_bool(pt, out) extract_nullable((pt), 0, BOOL_TYPE, (void*)(out))

#define extract_nullable_sint7(pt, out) extract_nullable((pt), 0, SINT7_TYPE, (void*)(out))
#define extract_nullable_sint15(pt, out) extract_nullable((pt), 0, SINT15_TYPE, (void*)(out))
#define extract_nullable_sint31(pt, out) extract_nullable((pt), 0, SINT31_TYPE, (void*)(out))
#define extract_nullable_sint63(pt, out) extract_nullable((pt), 0, SINT63_TYPE, (void*)(out))
#define extract_nullable_sint64(pt, out) extract_nullable((pt), 0, SINT64_TYPE, (void*)(out))

#define extract_nullable_uint7(pt, out) extract_nullable((pt), 0, UINT7_TYPE, (void*)(out))
#define extract_nullable_uint15(pt, out) extract_nullable((pt), 0, UINT15_TYPE, (void*)(out))
#define extract_nullable_uint31(pt, out) extract_nullable((pt), 0, UINT31_TYPE, (void*)(out))
#define extract_nullable_uint63(pt, out) extract_nullable((pt), 0, UINT63_TYPE, (void*)(out))
#define extract_nullable_uint64(pt, out) extract_nullable((pt), 0, UINT64_TYPE, (void*)(out))

#define extract_nullable_time(pt, out) extract_nullable((pt), 0, TIME_TYPE, (void*)(out))
#define extract_nullable_date_time(pt, out) extract_nullable((pt), 0, DATE_TIME_TYPE, (void*)(out))

uint8_t ICACHE_FLASH_ATTR extract_nullable_token(uint8_t* buffer, noizu_trie_a* trie, nullable_token_t* out);


//------------------------------------------
// Nullable String Helpers
//------------------------------------------
nullable_string_t* ICACHE_FLASH_ATTR copy_nullable_string(nullable_string_t* v);
uint8_t ICACHE_FLASH_ATTR free_nullable_string(nullable_string_t** v, uint8_t free_pointer);
uint8_t ICACHE_FLASH_ATTR free_nullable_string_contents(nullable_string_t* v);
uint8_t ICACHE_FLASH_ATTR recycle_nullable_string(nullable_string_t** v, uint16_t size);
uint8_t ICACHE_FLASH_ATTR allocate_nullable_string(nullable_string_t* v, uint16_t size);
uint8_t ICACHE_FLASH_ATTR strncpy_nullable_string(nullable_string_t* v, uint8_t* source, uint16_t length);

//------------------------------------------
// Dynamic Array
//------------------------------------------
noizu_realloc_code resize_dynamic_array(dynamic_array* raw, uint16_t entry_size, uint16_t required_length, uint16_t max_size, uint16_t increment, post_resize_cb cb);


#endif