/*!
* @file noizu_trie_c.c
* @brief struct encoded static trie implementation.
*
* @author Keith Brings
* @copyright Noizu Labs, Inc. 2019.
*/

#include "..\noizu_trie.h"
#include "noizu_trie_c.h"


TRIE_TOKEN inline EXTRACT_BIN_FIELD(uint16_t byte, uint8_t bit_offset, uint8_t bits, TRIE_CHAR_CODE* blob, uint32_t blob_length) {
	unsigned int mask = (1 << ((8 - bit_offset))) - 1;
	unsigned int raw = ( (byte < blob_length ? blob[byte] : 0) & mask);
	unsigned int scrape = 8 - bit_offset;
	while (scrape < bits) {
		byte++;
		raw = (raw << 8) | (byte < blob_length ? blob[byte] : 0);
		scrape += 8;
	}
	raw = raw >> (scrape - bits);
	return raw;
}


TRIE_TOKEN inline EXTRACT_TRIE_VALUE(uint32_t index, uint8_t record_size, uint8_t gap, uint8_t run, TRIE_CHAR_CODE* blob, uint32_t blob_length) {
	unsigned int bit = (index * record_size) + gap;
	unsigned int bit_offset = bit % 8;
	unsigned int byte = bit / 8;
	return EXTRACT_BIN_FIELD(byte, bit_offset, run, blob, blob_length);
}


#define EXTRACT_CHILD_JUMP(index, trie) EXTRACT_TRIE_VALUE((index), (trie)->bit_length, (trie)->bit_length__child_relative_offset, (trie)->bit_length__child_relative_index, (trie)->trie_raw, (trie)->trie_raw_length)
#define EXTRACT_SIB_JUMP(index, trie) EXTRACT_TRIE_VALUE((index), (trie)->bit_length, (trie)->bit_length__character_code, (trie)->bit_length__sibling_relative_index, (trie)->trie_raw, (trie)->trie_raw_length)
#define EXTRACT_CHAR(index, trie) EXTRACT_TRIE_VALUE((index), (trie)->bit_length, 0, (trie)->bit_length__character_code, (trie)->trie_raw, (trie)->trie_raw_length)

TRIE_TOKEN noizu_trie__compact__init(offset_buffer* req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out) {
	if (out->type_state && out->type && out->type != TRIE_COMPACT_TYPE) return TRIE_ARGUMENT_ERROR__INVALID_TYPE_STATE;
	out->type = TRIE_COMPACT_TYPE;
	if (out->type_state)  return TRIE_ARGUMENT_ERROR__INVALID_TYPE_STATE;
	out->position = 0;
	out->initialized = FALSE;
	return TRIE_INITIALIZED;
}

TRIE_TOKEN noizu_trie__compact__free(struct noizu_trie_state* state, struct noizu_trie_definition* definition, TRIE_TOKEN mode) {
	if (state == NULL && definition == NULL) return TRIE_SUCCESS;
	if (mode & TRIE_FREE_STATE__INNER) {
		if (state && state->type_state) {
			if (state->type != TRIE_COMPACT_TYPE) return TRIE_ARGUMENT_ERROR__INVALID_TYPE_STATE;
			os_free((struct noizu_trie__compact__state*)state->type_state);
			state->type_state = NULL;
		}
	}
	if (mode & TRIE_FREE_DEFINITION__INNER) {
		if (definition && definition->type_definition) {
			if (definition->type != TRIE_COMPACT_TYPE) return TRIE_ARGUMENT_ERROR__INVALID_TYPE_STATE;
			if (definition->constant) return TRIE_ERROR_DEALLOC_CONSTANT_DEFINITION;
			
			if (((struct noizu_trie__compact__definition*)definition->type_definition)->trie_raw) os_free(((struct noizu_trie__compact__definition*)definition->type_definition)->trie_raw);
			if (((struct noizu_trie__compact__definition*)definition->type_definition)->char_map) os_free(((struct noizu_trie__compact__definition*)definition->type_definition)->char_map);
			os_free((struct noizu_trie__compact__definition*)definition->type_definition);
			definition->type_definition = NULL;												
		}
	}
	return TRIE_SUCCESS;
}

TRIE_TOKEN noizu_trie__compact__validate(struct noizu_trie_state* state, struct noizu_trie_definition* definition) {
	if (state->type != TRIE_COMPACT_TYPE) return TRIE_ARGUMENT_ERROR__INVALID_TYPE_STATE;
	if (definition->type != TRIE_COMPACT_TYPE) return TRIE_ARGUMENT_ERROR__INVALID_TYPE_DEFINITION;
	return TRIE_VALID_EXIT;
}

TRIE_TOKEN noizu_trie__compact__advance(struct noizu_trie_state* state, struct noizu_trie_definition* definition) {
	// Advance to next node in trie.
	TRIE_CHAR_CODE c = 0;
	TRIE_TOKEN r = noizu_trie__next_char(state, definition, &c);
	if (!(r & TRIE_SUCCESS)) return r;

	//----------------------------------
	// Nested raw advance (other implementations move this to it's own method.)
	//----------------------------------
	TRIE_TOKEN advance_code = TRIE_NOT_FOUND;
	// Advance to next node in trie.
	TRIE_CHAR_CODE raw_char;
	uint8_t has_token = 0;
	struct noizu_trie__compact__definition* compact_trie = (struct noizu_trie__compact__definition*)definition->type_definition;
	uint32_t last_index = state->position;
	uint32_t next_index = last_index;
		
	if ((!(state->options.hard_delim && state->options.delimiter == c)) && !(state->position == TRIE_NOT_FOUND && state->initialized)) {
		TRIE_CHAR_CODE scan_for = ((struct noizu_trie__compact__definition*)definition->type_definition)->char_code(c);
		if (scan_for != TRIE_NOT_FOUND) {
			// 1. Advance to child if TRIE_INDEX != TRIE_NOT_FOUND or uninitilized.
			state->initialized = TRUE;			
			uint16_t advance = (uint16_t)EXTRACT_CHILD_JUMP(next_index, compact_trie);
			// 2. Find sibling that matches input. 				
			while (advance != TRIE_NOT_FOUND) {
				next_index += advance;
				raw_char = (TRIE_CHAR_CODE)EXTRACT_CHAR(next_index, compact_trie);
				if (raw_char == TRIE_NOT_FOUND || raw_char > compact_trie->characters) break;				
				if (raw_char == scan_for) {
					advance_code = TRIE_PARTIAL_MATCH;
					break;
				}

				if (compact_trie->char_map[raw_char - 1] < c) {
					advance = (uint16_t)EXTRACT_SIB_JUMP(next_index, compact_trie);
				}
				else {
					next_index = 0;
					break;
				}
			}
		}
	}

	//-----------------
	// Advance 
	//-----------------
	if (next_index >= compact_trie->size) {
		state->error_code = TRIE_ARRAY_ACCESS_ERROR;
		state->match_type = TRIE_ABNORMAL_EXIT;
		return TRIE_NO_MATCH;
	}

	// Update State
	state->position = next_index;
	if (advance_code == TRIE_NOT_FOUND) {
		state->terminator = c;
		if (state->options.keep_last_token) {
			if ((state->terminator == '\0' || state->terminator == state->options.delimiter) && state->token) state->match_type = TRIE_MATCH;
			else if (state->options.json_delim && (c == ':' || c == '"' || c == '\'')) state->match_type = TRIE_MATCH;
			else state->match_type = (state->last_token || state->token) ? TRIE_PARTIAL_MATCH : TRIE_NO_MATCH;
		}
		else {
			// end of input, if not tracking last token grab previous input to check if last char before walking off trie was a valid token.
			TRIE_TOKEN token = compact_trie->token_code(last_index, definition, &has_token);
			if (has_token) {
				state->token = token;
				state->token_index = last_index;
				state->token_pos = state->req_position;
				// TRIE match if end of input, otherwise last match if not end of string but end of trie with last value matching.
				if (state->terminator == '\0' || state->terminator == state->options.delimiter) state->match_type = TRIE_MATCH;
				else state->match_type = (state->options.json_delim && (c == ':' || c == '"' || c == '\'')) ? TRIE_MATCH : TRIE_LAST_MATCH;
			}
			else state->match_type = TRIE_NO_MATCH;
		}
		if (state->terminator == '\0') return TRIE_END_INPUT_EXIT;
		else if (state->terminator == state->options.delimiter) return TRIE_DELIM_EXIT;
		else if (state->options.json_delim && (c == ':' || c == '"' || c == '\'')) return TRIE_DELIM_EXIT;
		else return TRIE_END_PARSE_EXIT;
	}

	// Track token matches if option set
	if (state->options.keep_last_token) {
		if (state->token) {
			state->last_token = state->token;
			state->last_token_index = state->token_index;
		}		
		state->token = compact_trie->token_code(next_index, definition, &has_token);
		state->token_index = next_index;
		state->token_pos = state->req_position;
	}

	// Advance Position/Detect end of buffer state.
	if (state->req_position + 1 < state->req->buffer_size) {
		state->req_position++;
		return TRIE_CONTINUE;
	}
	else {
		// check for token if end of buffer and not already checked due to keep_last_token flow.
		if (state->options.end_of_buffer_token && !state->options.keep_last_token) {
			state->token = compact_trie->token_code(next_index, definition, &has_token);
			state->token_index = has_token ? next_index : 0;
			state->token_pos = has_token ? state->req_position : 0;
			state->match_type = has_token ? TRIE_LAST_MATCH : TRIE_NO_MATCH;
		}
		state->skip_next = 1;
		return TRIE_BUFFER_END;
	}
}