/*!
* @file noizu_trie.h
* @brief strategy layer.
*
* @author Keith Brings
* @copyright Noizu Labs, Inc. 2021.
*/

#ifndef __NOIZU_TRIE_H__
#define __NOIZU_TRIE_H__

#ifdef GENERIC_MODE 
#include <stdio.h>
#ifdef UNITY_TEST
#include "unity_memory.h"
#else
#include <corecrt_malloc.h>
#endif
#include <vcruntime_string.h>

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
#define NULL            ((void *)0)


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

// Unit Types
#define TRIE_TOKEN uint32_t
#define TRIE_STATE uint32_t
#define TRIE_CHAR_CODE uint8_t

// Hanlders
#define TRIE_STRUCT_TYPE 1
#define TRIE_COMPACT_TYPE 2
#define TRIE_ARRAY_TYPE 3

// Constants
#define TRIE_NO_MATCH 0x00
#define TRIE_MATCH 0x10
#define TRIE_EXACT_MATCH (TRIE_MATCH | 0x01)
//! indicates a partial match with input string going past entries in trie.  e.g. searched for apple but no trie entry after 'app' was found.
#define TRIE_PARTIAL_MATCH (TRIE_MATCH | 0x02)
//! indicates a partial match with input string going past entries in trie.  e.g. searched for apple but no trie entry after 'appl' was found.
#define TRIE_LAST_MATCH (TRIE_MATCH | 0x04)
//! indicate early exit due to sentinel
#define TRIE_PARTIAL_SENTINEL_EXIT 0x20


// TRIE RESPONSE CODES - major
#define TRIE_UNKNOWN 0x0000
#define TRIE_EXIT 0x1000
#define TRIE_ERROR 0x2000
#define TRIE_ARGUMENT_ERROR (TRIE_ERROR | 0x4000)
#define TRIE_SUCCESS 0x8000

// TRIE RESPONSE CODES - EXIT
#define TRIE_RETURN_EXIT (TRIE_EXIT | 0x0200)
#define TRIE_VALID_EXIT (TRIE_EXIT | TRIE_SUCCESS | 0x0400)
#define TRIE_ABNORMAL_EXIT (TRIE_EXIT | 0x0800)
#define TRIE_END_INPUT_EXIT (TRIE_VALID_EXIT | TRIE_RETURN_EXIT |  0x0001)
#define TRIE_END_PARSE_EXIT (TRIE_VALID_EXIT | TRIE_RETURN_EXIT |  0x0002)
#define TRIE_CONTINUE (TRIE_VALID_EXIT | 0x0003)
#define TRIE_DELIM_EXIT (TRIE_VALID_EXIT | TRIE_RETURN_EXIT |  0x0004)
#define TRIE_OVERFLOW_EXIT (TRIE_ABNORMAL_EXIT | TRIE_RETURN_EXIT | 0x0005)
#define TRIE_BUFFER_END (TRIE_VALID_EXIT | TRIE_RETURN_EXIT |0x0006)
#define TRIE_BUFFER_END_ON_SKIP (TRIE_ABNORMAL_EXIT | TRIE_RETURN_EXIT |0x0007)

// ERRORS
#define TRIE_ALLOC_FAIL (TRIE_ERROR | 0x0001)
#define TRIE_DEFINITION_ERROR (TRIE_ERROR | 0x0002)
#define TRIE_ACCESS_ERROR (TRIE_ERROR | 0x0003)
#define TRIE_ARRAY_ACCESS_ERROR TRIE_ACCESS_ERROR
#define TRIE_STRUCT_ACCESS_ERROR TRIE_ACCESS_ERROR

// Argument Errors
#define TRIE_ARGUMENT_ERROR__INVALID_TYPE (TRIE_ARGUMENT_ERROR | 0x0100)
#define TRIE_ARGUMENT_ERROR__INVALID_TYPE_STATE (TRIE_ARGUMENT_ERROR__INVALID_TYPE | 0x0001)
#define TRIE_ARGUMENT_ERROR__INVALID_TYPE_DEFINITION (TRIE_ARGUMENT_ERROR__INVALID_TYPE | 0x0002)

#define TRIE_ARGUMENT_ERROR__NULL (TRIE_ARGUMENT_ERROR | 0x0200)
#define TRIE_ARGUMENT_ERROR__NULL_STATE (TRIE_ARGUMENT_ERROR__NULL | 0x0003)
#define TRIE_ARGUMENT_ERROR__NULL_DEFINITION (TRIE_ARGUMENT_ERROR__NULL | 0x0004)
#define TRIE_ARGUMENT_ERROR__NULL_TYPE_STATE (TRIE_ARGUMENT_ERROR__NULL | 0x0005)
#define TRIE_ARGUMENT_ERROR__NULL_TYPE_DEFINITION (TRIE_ARGUMENT_ERROR__NULL | 0x0006)

#define TRIE_ARGUMENT_ERROR__TYPE (TRIE_ARGUMENT_ERROR | 0x0400)
#define TRIE_ARGUMENT_ERROR__TYPE_MISMATCH (TRIE_ARGUMENT_ERROR__TYPE | 0x0007)
#define TRIE_ARGUMENT_ERROR__TYPE_INIT_NOT_SET (TRIE_ARGUMENT_ERROR__TYPE | 0x0008)
#define TRIE_ARGUMENT_ERROR__TYPE_VALIDATE_NOT_SET (TRIE_ARGUMENT_ERROR__TYPE | 0x0009)
#define TRIE_ARGUMENT_ERROR__TYPE_ADVANCE_NOT_SET (TRIE_ARGUMENT_ERROR__TYPE | 0x000A)
#define TRIE_ARGUMENT_ERROR__TYPE_TOKENIZE_NOT_SET (TRIE_ARGUMENT_ERROR__TYPE | 0x000B)
#define TRIE_ARGUMENT_ERROR__TYPE_FREE_NOT_SET (TRIE_ARGUMENT_ERROR__TYPE | 0x000C)
#define TRIE_ARGUMENT_ERROR__TYPE_RESET_NOT_SET (TRIE_ARGUMENT_ERROR__TYPE | 0x000D)

#define TRIE_ERROR_DEALLOC_CONSTANT_DEFINITION (TRIE_ERROR | 0x0010)



// TRIE SUCCESS
#define TRIE_INITIALIZED (TRIE_SUCCESS | 0x0001)




/*!
 * @brief string not found special index value.
 * This special trie index functions as termination well to halt processing of non matched input strings.
 * In generated tries trie head begin at index position (1) with a non parsed special token value. (*) by default.
 */
#define TRIE_NOT_FOUND 0

#define TRIE_FREE_STATE 0x11
#define TRIE_FREE_STATE__INNER 0x01


#define TRIE_FREE_DEFINITION 0x22
#define TRIE_FREE_DEFINITION__INNER 0x02
#define TRIE_FREE_DEFINITION_AND_STATE (TRIE_FREE_STATE | TRIE_FREE_DEFINITION)

// Forward Declaration
struct noizu_trie_state;
struct noizu_trie_definition;
struct noizu_trie_options;

// custom/special case tokenizer early exit/selection
typedef TRIE_TOKEN(*tokenizer_sentinel)(TRIE_TOKEN advance_flag, struct noizu_trie_state* state, struct noizu_trie_definition* definition);

// Per Implementation callbacks
typedef TRIE_TOKEN(*tokenizer_init)(offset_buffer* req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out);
typedef TRIE_TOKEN(*tokenizer_reset)(struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* state);
typedef TRIE_TOKEN(*tokenizer_free)(struct noizu_trie_state* out, struct noizu_trie_definition* definition, TRIE_TOKEN mode);
typedef TRIE_TOKEN(*tokenizer_validate)(struct noizu_trie_state* state, struct noizu_trie_definition* definition);
typedef TRIE_TOKEN(*tokenizer_advance)(struct noizu_trie_state* state, struct noizu_trie_definition* definition);
typedef TRIE_TOKEN(*tokenizer_tokenize)(struct noizu_trie_state*, struct noizu_trie_definition*, tokenizer_sentinel sentinel);

/*!
* @brief contains type specific trie implementation and handler methods.
*/
typedef struct noizu_trie_definition {
	uint8_t constant : 1;
	uint8_t type: 7;
	
	void* type_definition;

	
	tokenizer_init trie_init;
	tokenizer_reset trie_reset;
	tokenizer_free trie_free;
	tokenizer_validate trie_validate;
	tokenizer_advance trie_advance;
	tokenizer_tokenize trie_tokenize;
} noizu_trie_definition;

/*!
 * @brief trie config settings.
 */
typedef struct noizu_trie_options {
	TRIE_CHAR_CODE delimiter;
	uint8_t keep_last_token : 1;
	uint8_t end_of_buffer_token : 1;
	uint8_t hard_delim : 1;
	uint8_t json_delim : 1;
	uint8_t escape_chars : 1;
} noizu_trie_options;

/*!
 * @brief trie state.
 * Last token, current position on buffer, last termination node, match type, etc.
 */
typedef struct noizu_trie_state {
	uint8_t type;

	// Stream
	offset_buffer* req;
	uint8_t skip_next : 7;
	uint8_t escape : 1;

	// Parse Details
	TRIE_STATE match_type;
	TRIE_TOKEN error_code;
	TRIE_TOKEN sentinel_exit_code;
	
	TRIE_CHAR_CODE terminator; //!< end char / last char.
	unsigned char initialized;

	uint32_t position;
	uint32_t req_position;

	TRIE_TOKEN token;
	uint32_t token_index;
	uint32_t token_pos;
	
	TRIE_TOKEN last_token;
	uint32_t last_token_index;	
	uint32_t last_token_pos;
	


	// Type specific state.
	void* type_state;

	// Options
	noizu_trie_options options;
} noizu_trie_state;

TRIE_TOKEN noizu_trie__next_char(struct noizu_trie_state* state, struct noizu_trie_definition* definition, uint8_t* c);

/*!
 * Init trie state for processing.
 */
TRIE_TOKEN noizu_trie__init(offset_buffer *req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out);
TRIE_TOKEN noizu_trie__reset(struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* state);
TRIE_TOKEN noizu_trie__free(struct noizu_trie_state* state, struct noizu_trie_definition* definition, TRIE_TOKEN mode);

/*!
 * Validate definition and state
 */
TRIE_TOKEN noizu_trie__validate(struct noizu_trie_state* state, struct noizu_trie_definition* definition);

/*!
 * Advance trie one character of input.
 */
TRIE_TOKEN noizu_trie__advance(struct noizu_trie_state* state, struct noizu_trie_definition* definition);

/*!
 * Tokenize input stream.
 */
TRIE_TOKEN noizu_trie__tokenize(struct noizu_trie_state* state, struct noizu_trie_definition* definition, tokenizer_sentinel sentinel);

#endif