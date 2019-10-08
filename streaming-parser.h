#ifndef __STREAMING_PARSER_H__ 
#define __STREAMING_PARSER_H__
#include "noizu_trie_a.h"

//------------------------------
// Override some defines used for ESP8266 for cross compatibility
//------------------------------
#ifdef GENERIC_MODE
#include <stdio.h>
#include <corecrt_malloc.h>
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
#include "httpc.h"
typedef request_args_t offset_buffer;
#endif


//-----------------------------
// Nullable Types
//-----------------------------

// We use 0 for null value to make wiping nullable fields more straight forward with zalloc/memset 0
#define NULL_VALUE 0
#define NOT_NULL_VALUE 1

/*!
 * @brief 7 bit nullable signed value
 */
typedef struct nullable_sint7_t {
	sint8_t value : 7;
	uint8_t null : 1;
} nullable_sint7_t;

/*!
 * @brief 7 bit nullable unsigned value
 */
typedef struct nullable_uint7_t {
	uint8_t value : 7;
	uint8_t null : 1;
} nullable_uint7_t;

typedef nullable_uint7_t nullable_bool_t;

/*!
 * @brief 15 bit nullable signed value
 */
typedef struct nullable_sint15_t {
	sint16_t value : 15;
	uint8_t null : 1;
} nullable_sint15_t;

/*!
 * @brief 15 bit nullable unsigned value
 */
typedef struct nullable_uint15_t {
	uint16_t value : 15;
	uint8_t null : 1;
} nullable_uint15_t;

/*!
 * @brief 31 bit nullable signed value
 */
typedef struct nullable_sint31_t {
	sint32_t value : 31;
	uint8_t null : 1;
} nullable_sint31_t;

/*!
 * @brief 31 bit nullable unsigned value
 */
typedef struct nullable_uint31_t {
	uint32_t value : 31;
	uint8_t null : 1;
} nullable_uint31_t;

/*!
 * @brief 63 bit nullable signed value
 */
typedef struct nullable_sint63_t {
	sint64_t value : 63;
	uint8_t null : 1;
} nullable_sint63_t;

/*!
 * @brief 63 bit nullable unsigned value
 */
typedef struct nullable_uint63_t {
	uint64_t value : 63;
	uint8_t null : 1;
} nullable_uint63_t;

/*!
 * @brief 64 bit nullable signed value
 */
typedef struct nullable_sint64_t {
	sint64_t value;
	uint8_t null;
} nullable_sint64_t;

/*!
 * @brief 64 bit nullable unsigned value
 */
typedef struct nullable_uint64_t {
	uint64_t value;
	uint8_t null;
} nullable_uint64_t;


typedef struct nullable_token_t {
	TRIE_A_UNIT value;
	uint8_t null;
} nullable_token_t;

/*!
 * @brief float with nullable flag
 */
typedef struct nullable_float_t {
	float value;
	uint8_t null;
} nullable_float_t;

/*!
 * @brief double with nullable flag
 */
typedef struct nullable_double_t {
	double value;
	uint8_t null;
} nullable_double_t;

/*!
 * @brief string pointer with nullable flag
 */
typedef struct nullable_string_t {
	uint8_t* value;
	uint8_t null;
} nullable_string_t;


//-----------------------------
// Enums
//-----------------------------

/*!
 * Json types
 */
typedef enum json_value_type {
	JSON_ERROR_VALUE = 0,
	JSON_TRUE_VALUE,
	JSON_FALSE_VALUE,
	JSON_NUMERIC_VALUE,
	JSON_QUOTE_VALUE,
	JSON_DOUBLE_QUOTE_VALUE,
	JSON_LIST_VALUE,
	JSON_STRUCT_VALUE,
	JSON_NULL_VALUE,
} json_value_type;

/*!
 * Parser CallBack Response Commands
 */
typedef enum jsp_cb_command {
	JSPC_PROCEED = 0,
	JSPC_RETURN = 1,
	JSPC_REBASE = 2,
	JSPC_POP = 3,
} jsp_cb_command;


/*!
 * Streaming parser state.
 */
typedef enum json_parse_state {
	PS_ADVANCE_KEY = 0,
	PS_PARSE_KEY,
	PS_PARSE_KEY_COMPLETE,
	PS_ADVANCE_VALUE,
	PS_ADVANCE_VALUE_COMPLETE,
	PS_CAPTURE_VALUE,
	PS_ADVANCE_LIST_ITEM,
	PS_CAPTURE_LIST_ITEM,
	PS_LIST_COMPLETE,
	PS_LIST_ITEM_COMPLETE,
	PS_STRUCT_COMPLETE,
	PS_COMPLETE,
	PS_EXTRACT_VALUE,
	PS_END_OF_BUFFER,
	PS_ERROR
} json_parse_state;


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
void ICACHE_FLASH_ATTR free_parser_children(json_parser* base);
json_parser* ICACHE_FLASH_ATTR spawn_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR drop_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR init_json_parser(offset_buffer*, noizu_trie_a* trie, json_streaming_parser_event_cb cb, void* output);
json_parser* ICACHE_FLASH_ATTR top_parser(json_parser* parser);

// Debug Output
void ICACHE_FLASH_ATTR print_json_parser_path(json_parser* parser, uint32_t line);
void ICACHE_FLASH_ATTR print_json_parser(json_parser* parser, uint8_t offset);

// Parsing Helpers
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_token(json_parser* parser, noizu_trie_a* trie, nullable_token_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_key(json_parser* parser, nullable_string_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_bool(json_parser* parser, nullable_bool_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_string(json_parser* parser, nullable_string_t* out);

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_sint7(json_parser* parser, nullable_sint7_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_sint15(json_parser* parser, nullable_sint15_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_sint31(json_parser* parser, nullable_sint31_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_sint63(json_parser* parser, nullable_sint63_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_sint64(json_parser* parser, nullable_sint64_t* out);

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint7(json_parser* parser, nullable_uint7_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint15(json_parser* parser, nullable_uint15_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint31(json_parser* parser, nullable_uint31_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint63(json_parser* parser, nullable_uint63_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint64(json_parser* parser, nullable_uint64_t* out);

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_float(json_parser* parser, nullable_float_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_double(json_parser* parser, nullable_double_t* out);

#endif