#include "noizu_trie.h"

TRIE_TOKEN noizu_trie__next_char(struct noizu_trie_state* state, struct noizu_trie_definition* definition, uint8_t* out_c) {	
	if (state->skip_next) {
		if ((state->req_position + state->skip_next) < state->req->buffer_size) {
			state->req_position += state->skip_next;
			state->skip_next = 0;
		}
		else return TRIE_BUFFER_END_ON_SKIP;
	}

	uint8_t c = *(state->req->buffer + state->req_position);
	if ((c == '\\' && state->options.escape_chars) || state->escape) {
		if (state->escape) {
			state->escape = 0;
			if (c == '\\') c = '\\';
			else if (c == 'n') c = '\n';
			else if (c == 'r') c = '\r';
			else if (c == 't') c = '\t';
			else if (c == '"') c = '"';
			else if (c == '\'') c = '\'';
			else {
				// error state
			}
		}
		else {
			state->skip_next = 1;
			state->escape = 1;
			return noizu_trie__next_char(state, definition, out_c);
		}
	}
	*out_c = c;
	return TRIE_SUCCESS;
}


TRIE_TOKEN noizu_trie__init(offset_buffer* req, struct noizu_trie_definition* definition, struct noizu_trie_options options, struct noizu_trie_state* out) {
	if (out == NULL) return TRIE_ARGUMENT_ERROR__NULL_STATE;
	void* p = out->type_state;
	TRIE_TOKEN t = out->type;

	os_memset(out, 0, sizeof(noizu_trie_state));
	out->type = t;
	out->type_state = p;
	out->req = req;
	out->options = options;	
	out->req_position = req->buffer_pos;
	return definition->trie_init ? definition->trie_init(req, definition, options, out) : TRIE_ARGUMENT_ERROR__TYPE_INIT_NOT_SET;
}

TRIE_TOKEN noizu_trie__free(struct noizu_trie_state* state, struct noizu_trie_definition* definition, TRIE_TOKEN mode) {
	if (state == NULL && definition == NULL) return TRIE_SUCCESS;
	TRIE_TOKEN o = TRIE_UNKNOWN;

	if (mode & TRIE_FREE_STATE__INNER) {		
		if (state) {

			if (state->type_state) {
				if (definition == NULL) o |= TRIE_ARGUMENT_ERROR__NULL_DEFINITION;
				else o |= definition->trie_free(state, definition, TRIE_FREE_STATE__INNER);
			}

			// free regardless of delegate's outcome to reduce as much memory as possible.
			if ((mode & TRIE_FREE_STATE) == TRIE_FREE_STATE) {				
				os_free(state);
				state = NULL;
			}
		}
	}
	if (mode & TRIE_FREE_DEFINITION__INNER) {
		if (definition) {
			if (definition->constant) return TRIE_ERROR_DEALLOC_CONSTANT_DEFINITION;
			o |= definition->trie_free(state, definition, TRIE_FREE_DEFINITION__INNER);
			// free regardless of delegate's outcome to reduce as much memory as possible.
			if ((mode & TRIE_FREE_DEFINITION) == TRIE_FREE_DEFINITION) os_free(definition);
		}
	}
	return (o & TRIE_ERROR) ? o : (o | TRIE_SUCCESS);
}

TRIE_TOKEN noizu_trie__validate(struct noizu_trie_state* state, struct noizu_trie_definition* definition) {
	if (state == NULL) return TRIE_ARGUMENT_ERROR__NULL_STATE;
	if (state->type_state == NULL) return TRIE_ARGUMENT_ERROR__NULL_TYPE_STATE;
	if (definition == NULL) return TRIE_ARGUMENT_ERROR__NULL_DEFINITION;
	if (state->type != definition->type) return TRIE_ARGUMENT_ERROR__TYPE_MISMATCH;
	if (definition->type_definition == NULL) return TRIE_ARGUMENT_ERROR__NULL_TYPE_DEFINITION;
	if ((state->req_position + state->skip_next) == state->req->buffer_size) return (TRIE_BUFFER_END | TRIE_ERROR);
	if ((state->req_position + state->skip_next) > state->req->buffer_size) return (TRIE_OVERFLOW_EXIT | TRIE_ERROR);		
	if (definition->trie_init == NULL) return TRIE_ARGUMENT_ERROR__TYPE_INIT_NOT_SET;
	if (definition->trie_free == NULL) return TRIE_ARGUMENT_ERROR__TYPE_FREE_NOT_SET;
	if (definition->trie_advance == NULL) return TRIE_ARGUMENT_ERROR__TYPE_ADVANCE_NOT_SET;
	// tokenize not required.
	return definition->trie_validate ? definition->trie_validate(state, definition) : TRIE_ARGUMENT_ERROR__TYPE_VALIDATE_NOT_SET;
}

TRIE_TOKEN noizu_trie__advance(struct noizu_trie_state* state, struct noizu_trie_definition* definition) {
	return definition->trie_advance(state, definition);
}

TRIE_TOKEN noizu_trie__tokenize(struct noizu_trie_state* state, struct noizu_trie_definition* definition, tokenizer_sentinel sentinel) {
	if (definition->trie_tokenize) return definition->trie_tokenize(state, definition, sentinel);
	else {
		TRIE_TOKEN sentinel_code, advance;		
		while (true) {
			advance = noizu_trie__advance(state, definition);
			if (sentinel && (sentinel_code = sentinel(advance, state, definition))) {
				// sentinel responsible for any state manipulation/token set operations.
				state->sentinel_exit_code = sentinel_code;
				return TRIE_PARTIAL_SENTINEL_EXIT;
			}
			if (advance != TRIE_CONTINUE) return advance;
		}
	}
}