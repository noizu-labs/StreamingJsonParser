/*!
 * @file streaming-parser.c
 *
 * @author Keith Brings
 * @copyright Noizu Labs, Inc.
 */

#include "streaming-parser.h"

#pragma warning(push)
#pragma warning(disable: 4996 26451)

 //--------------------------------
 // Declarations: Main Entry Point
 //--------------------------------
jsp_cb_command ICACHE_FLASH_ATTR json_streaming_parser(json_parser* parser);

//--------------------------------
// Declarations: Parse Loop and Delegates
//--------------------------------
json_parse_state ICACHE_FLASH_ATTR json_process(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_KEY(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__PARSE_KEY(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_VALUE(json_parser* parser);
json_parse_state ICACHE_FLASH_ATTR json_process__CAPTURE_VALUE(json_parser* parser);

//--------------------------------
// Declarations: Helper Methods (tweak state)
//--------------------------------
void ICACHE_FLASH_ATTR json_parser_globals_reset();
void ICACHE_FLASH_ATTR reset_json_parser(json_parser* parser);
void ICACHE_FLASH_ATTR shift_json_parser(uint16_t shift, json_parser* parser);
json_parser* ICACHE_FLASH_ATTR spawn_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR drop_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR top_parser(json_parser* parser);

//--------------------------------
// Declarations: Parsing Helpers
//--------------------------------
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_key(json_parser* parser, nullable_string_t* out);
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_string(json_parser* parser, nullable_string_t* out);

//--------------------------------
// Declarations: nullable_string helpers
//--------------------------------
nullable_string_t* ICACHE_FLASH_ATTR copy_nullable_string(nullable_string_t* v);
uint8_t ICACHE_FLASH_ATTR free_nullable_string(nullable_string_t** v, uint8_t free_pointer);
uint8_t ICACHE_FLASH_ATTR free_nullable_string_contents(nullable_string_t* v);
uint8_t ICACHE_FLASH_ATTR recycle_nullable_string(nullable_string_t** v, uint16_t size);
uint8_t ICACHE_FLASH_ATTR allocate_nullable_string(nullable_string_t* v, uint16_t size);
uint8_t ICACHE_FLASH_ATTR strncpy_nullable_string(nullable_string_t* v, uint8_t* source, uint16_t length);

//---------------------------------
// Locals
//---------------------------------
static uint16_t previous_buffer_pos = 0;
static uint16_t buffer_pos_offset = 0;




//--------------------------------
// Functions: Main Entry Point
//--------------------------------
jsp_cb_command ICACHE_FLASH_ATTR json_streaming_parser(json_parser* base) {
	offset_buffer* req = base->req;

	if (previous_buffer_pos != req->buffer_pos) {
		uint16_t shift = previous_buffer_pos - req->buffer_pos;
		shift_json_parser(shift, base);
		previous_buffer_pos = req->buffer_pos;
	}

	uint16_t buffer_pos_retrace = req->buffer_pos;
	req->buffer_pos += buffer_pos_offset;

	// advance parser
	json_parser* parser = top_parser(base);
	jsp_cb_command cb_command = JSPC_PROCEED;

	// process buffer
	while (req->buffer_pos < req->buffer_size) {
		// [bug fix] Parser needs to rebase to top after each iteration, to track depth increases correctly.
		parser = top_parser(parser);
		json_parse_state r = json_process(parser);
		switch (r) {
		case PS_ADVANCE_KEY:
		case PS_PARSE_KEY:
		case PS_ADVANCE_VALUE:
		case PS_CAPTURE_VALUE:
		case PS_ADVANCE_LIST_ITEM:
		case PS_CAPTURE_LIST_ITEM:
			break;

		case PS_END_OF_BUFFER:
			buffer_pos_offset = req->buffer_pos - buffer_pos_retrace;
			req->buffer_pos = buffer_pos_retrace;
			return JSPC_PROCEED;
			break;

		case PS_PARSE_KEY_COMPLETE:
			parser->cb(r, parser);
			break;

		case PS_ADVANCE_VALUE_COMPLETE:
		{
			json_value_type vt = parser->value_type;
			uint16_t t = parser->token;

			json_parse_state ps = parser->parse_state;
			if (parser->value_type == JSON_STRUCT_VALUE) {
				parser = spawn_json_parser(parser);
				parser->brace_track = 1;
				parser->token = (uint8_t)t;
			}
			else if (parser->value_type == JSON_LIST_VALUE) {
				parser = spawn_json_parser(parser);
				parser->bracket_track = 1;
				parser->token = (uint8_t)t;
			}
			else {
				uint16_t ks = parser->key_start;
				uint16_t kc = parser->key_close;
				uint16_t vs = parser->value_start;
				uint16_t p = parser->parent_p ? parser->parent_p->token : 1234;
				uint8_t list_member = (parser->parent_p) ? parser->parent_p->value_type == JSON_LIST_VALUE : 0;
				uint16_t list_index = list_member ? parser->parent_p->list_members : 0;
				parser->list_index = list_index;

				reset_json_parser(parser);

				// Only if parent is a struct
				parser->key_start = ks;
				parser->key_close = kc;
				parser->token = (uint8_t)t;
				// -------------------------------
				//parser->parent = 700000 | p;
				parser->value_start = vs;
				parser->value_type = vt;
				parser->list_index = list_index;

				if (vt == JSON_DOUBLE_QUOTE_VALUE) {
					parser->d_quote_track = 1;
					parser->in_d_quote = 1;
				}
				else if (vt == JSON_QUOTE_VALUE) {
					parser->s_quote_track = 1;
					parser->in_s_quote = 1;
				}

				if (list_member) {
					parser->parse_state = PS_CAPTURE_LIST_ITEM;
				}
				else {
					parser->parse_state = PS_CAPTURE_VALUE;
				}
			}
			parser->cb(PS_EXTRACT_VALUE, parser);
		}
		break;

		case PS_LIST_ITEM_COMPLETE:
			parser->cb(PS_LIST_ITEM_COMPLETE, parser);
			if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
				parser->parent_p->list_members++;
			}
			reset_json_parser(parser);
			// advance closeable state.
			buffer_pos_retrace = req->buffer_pos;
			previous_buffer_pos = req->buffer_pos;
			break;

		case PS_LIST_COMPLETE:
		{
			parser->cb(PS_LIST_COMPLETE, parser);
			uint8_t skip = parser->char_skip;
			parser = drop_json_parser(parser);
			reset_json_parser(parser);

			if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
				parser->parent_p->list_members++;
			}
			parser->char_skip = skip;

			// advance closeable state.
			buffer_pos_retrace = req->buffer_pos;
			previous_buffer_pos = req->buffer_pos;
		}
		break;

		case PS_STRUCT_COMPLETE:
		{
			parser->cb(PS_STRUCT_COMPLETE, parser);
			uint8_t skip = parser->char_skip;
			json_parser* p = parser;
			while (p->parent_p) p = p->parent_p;

			parser = drop_json_parser(parser);
			reset_json_parser(parser);

			if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
				parser->parent_p->list_members++;
			}
			parser->char_skip = skip;


			// advance closeable state.
			buffer_pos_retrace = req->buffer_pos;
			previous_buffer_pos = req->buffer_pos;
		}
		break;

		case PS_COMPLETE:
		{
			parser->cb(PS_COMPLETE, parser);
			reset_json_parser(parser);

			if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
				parser->parent_p->list_members++;
			}

			// advance closeable state.
			buffer_pos_retrace = req->buffer_pos;
			previous_buffer_pos = req->buffer_pos;
		}
		break;

		case PS_ERROR:
			LOG_ERROR("[JSON] PS_ERROR");
			break;

		default:
			LOG_ERROR("[JSON] Unsupported Child %d", r);
		}
	}

	// manipulate buffer_pos to prevent http buffer manager from wiping data in process.
	buffer_pos_offset = req->buffer_pos - buffer_pos_retrace;
	req->buffer_pos = buffer_pos_retrace;
	return JSPC_PROCEED;
}

//--------------------------------
// Functions: Parse Loop and Delegates
//--------------------------------
json_parse_state ICACHE_FLASH_ATTR json_process(json_parser* parser) {
	// Skip first char. 
	if (parser->char_skip) {
		if ((parser->req->buffer_pos + 1) < parser->req->buffer_size) {
			parser->char_skip = 0;
			parser->req->buffer_pos++;
		}
		else {
			//LOG_ERROR("PS_END_OF_BUFFER");
			return PS_END_OF_BUFFER;
		}
	}

	switch (parser->parse_state) {
	case PS_ADVANCE_KEY:	return json_process__ADVANCE_KEY(parser);
	case PS_PARSE_KEY: return json_process__PARSE_KEY(parser);

	case PS_ADVANCE_VALUE:
	case PS_ADVANCE_LIST_ITEM:
		return json_process__ADVANCE_VALUE(parser);

	case PS_CAPTURE_VALUE:
	case PS_CAPTURE_LIST_ITEM:
		return json_process__CAPTURE_VALUE(parser);

	case PS_LIST_COMPLETE:
	case PS_COMPLETE:
		return parser->parse_state;

	default:
		return PS_ERROR;
	}
}

json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_KEY(json_parser* parser) {
	uint8_t* p = parser->req->buffer;
	uint8_t c = 0;
	for (; (parser->req->buffer_pos) < parser->req->buffer_size; parser->req->buffer_pos++) {
		//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));		
		c = *(p + parser->req->buffer_pos);
		switch (c) {
		case '"':
			parser->char_skip = 1;
			parser->key_start = parser->req->buffer_pos;
			parser->token = 0;
			parser->parse_state = PS_PARSE_KEY;
			parser->in_d_quote = 1;
			parser->d_quote_track = 1;

			// reset
			noizu_trie__reset(parser->trie_definition, parser->trie_state->options, parser->trie_state);
			parser->trie_state->options.delimiter = '"';

			return parser->parse_state;

		case '\'':
			parser->char_skip = 1;
			parser->key_start = parser->req->buffer_pos;
			parser->token = 0;
			parser->parse_state = PS_PARSE_KEY;
			parser->in_s_quote = 1;
			parser->s_quote_track = 1;

			// reset
			noizu_trie__reset(parser->trie_definition, parser->trie_state->options, parser->trie_state);
			parser->trie_state->options.delimiter = '\'';

			return parser->parse_state;


		case '}':
			// todo we technically should perform tracking logic here. 
			//LOG_ERROR("Additional logic needed. %s", parser->req->buffer);
			parser->char_skip = 1;
			return PS_STRUCT_COMPLETE;
		case ']':
			//LOG_ERROR("Additional logic needed.");
			parser->char_skip = 1;
			return PS_LIST_COMPLETE;

		default:
			if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
				parser->char_skip = 0;
				parser->key_start = parser->req->buffer_pos;
				parser->token = 0;

				// reset
				noizu_trie__reset(parser->trie_definition, parser->trie_state->options, parser->trie_state);
				parser->trie_state->options.delimiter = ':';

				parser->parse_state = PS_PARSE_KEY;
				return parser->parse_state;
			}

			//default:
				// proceed.
		}
	}
	if (parser->req->buffer_pos && (parser->req->buffer_pos == parser->req->buffer_size)) {
		parser->req->buffer_pos--;
	}
	return PS_END_OF_BUFFER;
}

json_parse_state ICACHE_FLASH_ATTR json_process__PARSE_KEY(json_parser* parser) {
	uint8_t* p = parser->req->buffer;
	uint8_t s = 0;
	uint8_t c;

	//-------- @TODO ---------------------------------------
	// tokenize, on failure forward to end of string
	// tokenize, on success confirm terminator 
	parser->trie_state->req_position = parser->req->buffer_pos;

	TRIE_TOKEN o = noizu_trie__tokenize(parser->trie_state, parser->trie_definition, 0);

	// if did not terminate at end of input advance buffer to end of unknown key.
	if (parser->trie_state->terminator != parser->trie_state->options.delimiter) {
		if (o == TRIE_BUFFER_END) {
			parser->req->buffer_pos = parser->trie_state->req_position;
			return PS_END_OF_BUFFER;
		}
		else {

			for (; (parser->req->buffer_pos) < parser->req->buffer_size; parser->req->buffer_pos++) {
				c = *(p + parser->req->buffer_pos);

				if (parser->escape_char) {
					parser->escape_char = 0;
				}
				else {
					switch (c) {
					case '\\':
						parser->escape_char = 1;
						break;
					case '"':
						if (!parser->in_s_quote) {
							s = 1;
							parser->char_skip = 1;
							parser->key_close = parser->req->buffer_pos;
						}
						break;
					case '\'':
						if (!parser->in_d_quote) {
							s = 1;
							parser->char_skip = 1;
							parser->key_close = parser->req->buffer_pos;
						}
						break;

					default:
						if (!(parser->in_d_quote || parser->in_s_quote)) {
							if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) {
								s = 1;
								parser->char_skip = 0;
								if (parser->req->buffer_pos) {
									parser->key_close = parser->req->buffer_pos - 1;
								}
								else {
									// unexpected state
									LOG_ERROR("INVALID_STATE");
									parser->key_close = parser->req->buffer_pos;
								}
							}
						}
					}
				}
				if (s) {
					parser->escape_char = 0;
					parser->in_d_quote = 0;
					parser->in_s_quote = 0;
					parser->s_quote_track = 0;
					parser->d_quote_track = 0;
					parser->parse_state = PS_ADVANCE_VALUE;
					return PS_PARSE_KEY_COMPLETE;
				}
			}

			// walked to end of buffer
			if (parser->req->buffer_pos && (parser->req->buffer_pos == parser->req->buffer_size)) {
				parser->req->buffer_pos--;
			}
			return PS_END_OF_BUFFER;
		}
	}
	else {
		parser->req->buffer_pos = parser->trie_state->req_position;
		parser->key_close = parser->trie_state->req_position;
		if (parser->in_d_quote || parser->in_s_quote) {
			//parser->key_close--;
			parser->char_skip = 1;
		}
		parser->escape_char = 0;
		parser->in_d_quote = 0;
		parser->in_s_quote = 0;
		parser->s_quote_track = 0;
		parser->d_quote_track = 0;
		parser->token = parser->trie_state->token;
		parser->parse_state = PS_ADVANCE_VALUE;
		return PS_PARSE_KEY_COMPLETE;
	}
}

json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_VALUE(json_parser* parser) {
	uint8_t* p = parser->req->buffer;
	for (; (parser->req->buffer_pos) < parser->req->buffer_size; parser->req->buffer_pos++) {
		//LOG_ERROR("[JSON] INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
		// Advance until we encounter a '{', null, true, false, '[', '"', '\''
		uint8_t c = *(p + parser->req->buffer_pos);
		switch (c) {
		case ']':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			//if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {			
			if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
				parser->parent_p->parse_state = PS_LIST_COMPLETE;
				parser->parent_p->value_close = parser->req->buffer_pos;
			}
			else {
				LOG_ERROR("MALFORMED_JSON");
			}

			parser->value_close = parser->req->buffer_pos;
			parser->char_skip = 1;
			return PS_LIST_COMPLETE;
			//}
			//else {
				//LOG_ERROR("MALFORMED_JSON| Unexpected ']'");
			//}

		case '}':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			//if (parser->parent_p && parser->parent_p->value_type == JSON_STRUCT_VALUE) {
			if (parser->parent_p && parser->parent_p->value_type == JSON_STRUCT_VALUE) {
				parser->parent_p->value_close = parser->req->buffer_pos;
				parser->parent_p->parse_state = PS_COMPLETE;
				parser->parent_p->parse_state = PS_STRUCT_COMPLETE;
			}
			else {
				LOG_ERROR("MALFORMED_JSON");
			}
			parser->value_close = parser->req->buffer_pos;
			parser->char_skip = 1;
			return PS_STRUCT_COMPLETE;
			//}
			//else {
				//LOG_ERROR("MALFORMED_JSON| Unexpected '}'");
			//}
		case '{':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_STRUCT_VALUE;
			parser->brace_track++;
			break;

		case '[':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_LIST_VALUE;
			parser->bracket_track++;
			break;

		case '"':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_DOUBLE_QUOTE_VALUE;
			parser->d_quote_track++;
			parser->in_d_quote = 1;
			break;

		case '\'':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_QUOTE_VALUE;
			parser->s_quote_track++;
			parser->in_s_quote = 1;
			break;

		case 't':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_TRUE_VALUE;
			break;

		case 'f':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_FALSE_VALUE;
			break;

		case 'n':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			parser->value_type = JSON_NULL_VALUE;
			break;

		default:
			if (c >= '0' && c <= '9' || c == '-') {
				//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
				parser->value_type = JSON_NUMERIC_VALUE;
			}
		}

		if (parser->value_type != JSON_ERROR_VALUE) {
			parser->char_skip = 1;
			parser->parse_state = PS_CAPTURE_VALUE;
			parser->value_start = parser->req->buffer_pos;
			//LOG_ERROR("[JSON] ADVANCE COMPLETE Type=%d", parser->value_type);
			return PS_ADVANCE_VALUE_COMPLETE;
		}
	}
	if (parser->req->buffer_pos && (parser->req->buffer_pos == parser->req->buffer_size)) {
		parser->req->buffer_pos--;
	}
	return PS_END_OF_BUFFER;
}

json_parse_state ICACHE_FLASH_ATTR json_process__CAPTURE_VALUE(json_parser* parser) {
	uint8_t* p = parser->req->buffer;
	uint8_t list_member = (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE);

	for (; (parser->req->buffer_pos) < parser->req->buffer_size; parser->req->buffer_pos++) {
		//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
		uint8_t c = *(p + parser->req->buffer_pos);
		switch (c) {
		case '\\':
			parser->escape_char = parser->escape_char ? 0 : 1;
			break;

		case '\'':
			if (parser->escape_char) {
				parser->escape_char = 0;
			}
			else {
				if (parser->in_d_quote) {
					// ignore	
				}
				else if (parser->in_s_quote) {
					parser->s_quote_track--;
				}
				else {
					parser->s_quote_track++;
				}
			}
			break;

		case '"':
			if (parser->escape_char) {
				parser->escape_char = 0;
			}
			else {
				if (parser->in_s_quote) {
					// ignore	
				}
				else if (parser->in_d_quote) {
					parser->d_quote_track--;
				}
				else {
					parser->d_quote_track++;
				}
			}
			break;

		case '[':
			if (!(parser->in_s_quote || parser->in_d_quote)) {
				parser->bracket_track++;
			}
			break;

		case ']':
			if (!(parser->in_s_quote || parser->in_d_quote)) {
				parser->bracket_track--;
				// Special Case check for end of list
				if (list_member && parser->bracket_track == 0) {
					LOG_ERROR("BRACKET EXIT");
					// Note we don't advance value_end as the value before the closing ']' was the last list_item;
					parser->parse_state = PS_LIST_COMPLETE;
					//parser->list_entries++;
					return parser->parse_state;
				}
			}
			break;

		case '{':
			if (!(parser->in_s_quote || parser->in_d_quote)) {
				parser->brace_track++;
			}
			break;

		case '}':
			if (!(parser->in_s_quote || parser->in_d_quote)) {
				parser->brace_track--;
			}
			break;

		default:
			parser->escape_char = 0;
		}



		// Check for end condition
		switch (parser->value_type) {
		case JSON_FALSE_VALUE:
		case JSON_TRUE_VALUE:
			if (c == 'e') {
				parser->char_skip = 1;
				parser->value_close = parser->req->buffer_pos;
				if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos;
				if (list_member) {
					parser->parse_state = PS_ADVANCE_LIST_ITEM;
					return PS_LIST_ITEM_COMPLETE;
				}
				else {
					parser->parse_state = PS_COMPLETE;
					return PS_COMPLETE;
				}
			}
			break;

		case JSON_NULL_VALUE:
			if (c == 'l') {
				if (parser->null_track) {
					parser->char_skip = 1;
					parser->parse_state = PS_COMPLETE;
					parser->value_close = parser->req->buffer_pos;
					if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos;
					if (list_member) {
						parser->parse_state = PS_ADVANCE_LIST_ITEM;
						return PS_LIST_ITEM_COMPLETE;
					}
					else {
						parser->parse_state = PS_COMPLETE;
						return PS_COMPLETE;
					}
				}
				else {
					parser->char_skip = 1;
					parser->null_track = 1;
				}
			}
			break;

		case JSON_NUMERIC_VALUE:
			if (!((c >= '0' && c <= '9') || c == '.')) {
				//LOG_ERROR("NUMERIC END");
				//parser->char_skip = 1;
				// note we do not advance value_end as the previous numeric value was the actual last character.
				if (parser->req->buffer_pos > 0) {
					parser->value_close = parser->req->buffer_pos - 1;
					if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos - 1;
				}
				else {
					LOG_ERROR("This shouldn't happen");
				}

				if (list_member) {
					parser->parse_state = PS_ADVANCE_LIST_ITEM;
					return PS_LIST_ITEM_COMPLETE;
				}
				else {
					parser->parse_state = PS_COMPLETE;
					return PS_COMPLETE;
				}
			}
			break;

		case JSON_QUOTE_VALUE:
			if (parser->s_quote_track == 0) {
				parser->char_skip = 1;
				parser->value_close = parser->req->buffer_pos;
				if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos;
				if (list_member) {
					parser->parse_state = PS_ADVANCE_LIST_ITEM;
					return PS_LIST_ITEM_COMPLETE;
				}
				else {
					parser->parse_state = PS_COMPLETE;
					return PS_COMPLETE;
				}
			}
			break;

		case JSON_DOUBLE_QUOTE_VALUE:
			if (parser->d_quote_track == 0) {
				parser->char_skip = 1;
				parser->value_close = parser->req->buffer_pos;
				if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos;
				if (list_member) {
					parser->parse_state = PS_ADVANCE_LIST_ITEM;
					return PS_LIST_ITEM_COMPLETE;
				}
				else {
					parser->parse_state = PS_COMPLETE;
					return PS_COMPLETE;
				}
			}
			break;

		case JSON_LIST_VALUE:
			if (parser->bracket_track == 0) {
				parser->char_skip = 1;
				parser->value_close = parser->req->buffer_pos;
				if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos;
				if (list_member) {
					parser->parse_state = PS_ADVANCE_LIST_ITEM;
					return PS_LIST_ITEM_COMPLETE;
				}
				else {
					parser->parse_state = PS_COMPLETE;
					return PS_COMPLETE;
				}
			}
			break;

		case JSON_STRUCT_VALUE:
			if (parser->brace_track == 0) {
				parser->char_skip = 1;
				parser->value_close = parser->req->buffer_pos;
				if (parser->parent_p) parser->parent_p->value_close = parser->req->buffer_pos;
				if (list_member) {
					parser->parse_state = PS_ADVANCE_LIST_ITEM;
					return PS_LIST_ITEM_COMPLETE;
				}
				else {
					parser->parse_state = PS_COMPLETE;
					return PS_COMPLETE;
				}
			}
			break;
		}
		parser->value_close = parser->req->buffer_pos;
	}
	if (parser->req->buffer_pos && (parser->req->buffer_pos == parser->req->buffer_size)) {
		parser->req->buffer_pos--;
	}
	return PS_END_OF_BUFFER;
}


//--------------------------------
// Functions: Helper Methods (tweak state)
//--------------------------------
void ICACHE_FLASH_ATTR json_parser_globals_reset() {
	previous_buffer_pos = 0;
	buffer_pos_offset = 0;
}

void ICACHE_FLASH_ATTR  ICACHE_FLASH_ATTR reset_json_parser(json_parser* parser) {
	json_parser* c = parser->child;
	json_parser* p = parser->parent_p;
	offset_buffer* req = parser->req;
	uint16_t pt = parser->parent;
	//uint16_t list_entries = parser->list_index;
	uint8_t skip = parser->char_skip;

	// deprecated
	struct noizu_trie_state* state = parser->trie_state;
	struct noizu_trie_definition* definition = parser->trie_definition;


	uint8_t depth = parser->depth;
	void* output = parser->output;
	json_value_type vt = parser->value_type;
	if (vt == JSON_ERROR_VALUE) {
		//vt = parser->previous_value_type;		
	}
	// When we reset revert to parent's handler to simplify ability to inject at the { }, [ ], level. 
	json_streaming_parser_event_cb cb = parser->cb;

	os_memset(parser, 0, sizeof(json_parser));

	parser->previous_value_type = vt;
	parser->cb = cb;
	parser->trie_state = state;
	parser->trie_definition = definition;
	parser->output = output;
	parser->active = 1;
	parser->char_skip = skip;
	parser->child = c;
	parser->parent_p = p;
	parser->parent = (uint8_t)pt;
	parser->req = req;
	parser->depth = depth;
	parser->list_index = 0;
	parser->list_members = 0;
	parser->parse_state = PS_ADVANCE_KEY;
	if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) parser->parse_state = PS_ADVANCE_LIST_ITEM;
	parser->value_type = JSON_ERROR_VALUE;
}

void ICACHE_FLASH_ATTR shift_json_parser(uint16_t shift, json_parser* parser) {
	while (parser) {
		if (parser->key_start) {
			parser->key_start -= shift;
		}
		if (parser->key_close) {
			parser->key_close -= shift;
		}
		if (parser->value_start) {
			parser->value_start -= shift;
		}
		if (parser->value_close) {
			parser->value_close -= shift;
		}

		if (parser->child) {
			parser = parser->child;
		}
		else {
			return;
		}
	}
}

void ICACHE_FLASH_ATTR free_json_parser(json_parser* base, BOOL free_pointer) {
	json_parser* p = base;
	json_parser* trail = p;
	while (p) {
		if (p->child) {
			if (trail != base) {
				trail->parent_p = NULL;
				trail->child = NULL;
				os_free(trail);
				trail = NULL;
			}
			trail = p;
			p = p->child;
			trail->child = NULL;
		}
		else {
			if (trail != base) {
				trail->parent_p = NULL;
				trail->child = NULL;
				os_free(trail);
				trail = NULL;
			}
			if (p != base) {
				p->parent_p = NULL;
				p->child = NULL;
				os_free(p);
			}
			break;
		}
	}
	if (free_pointer) {
		json_parser_globals_reset();
		if (base) {
			// note must only be invoked on top level parser or we will double free state/definition.
			noizu_trie__free(base->trie_state, base->trie_definition, TRIE_FREE_DEFINITION || TRIE_FREE_STATE);
			os_free(base);
		}
	}
}


json_parser* ICACHE_FLASH_ATTR spawn_json_parser(json_parser* parser) {
	//print_json_parser_path(parser, __LINE__);
	uint8_t skip = parser->char_skip;
	if (parser->child) {
		//LOG_ERROR("[JSON] Restore Child");
		reset_json_parser(parser->child);
		if (parser->child->parent_p != parser) {
			LOG_ERROR("[JSON] Invalid State");
			parser->child->parent_p = parser;
		}
	}
	else {
		//LOG_ERROR("[JSON] Spawn new child");
		parser->child = init_json_parser(parser->req, parser->trie_state->options, parser->trie_definition, parser->cb, parser->output);
		parser->child->depth = parser->depth + 1;
		parser->child->parent_p = parser;
		if (parser->value_type == JSON_LIST_VALUE) {
			parser->child->parse_state = PS_ADVANCE_LIST_ITEM;
		}
	}

	//uint16_t list_index = parser->parent_p ? parser->parent_p->list_members : 0;
	//uint16_t list_index = parser->list_members;
	parser->list_index = parser->parent_p ? parser->parent_p->list_members : 0;
	parser->child->list_index = parser->list_index;
	if (parser->value_type == JSON_STRUCT_VALUE) {
		parser->child->key_start = parser->key_start;
		parser->child->key_close = parser->key_close;
		parser->child->value_start = parser->value_start;
		parser->child->parent = parser->token;
		parser->child->token = 0;
		parser->child->list_members = 0;
		parser->child->parse_state = PS_ADVANCE_KEY;
		//parser->child->list_index = list_index;
	}
	else if (parser->value_type == JSON_LIST_VALUE) {
		parser->child->key_start = parser->key_start;
		parser->child->key_close = parser->key_close;
		parser->child->value_start = parser->value_start;
		parser->child->parent = parser->token;
		parser->child->token = 0;
		parser->child->parse_state = PS_ADVANCE_LIST_ITEM;
		// reset list entries
		parser->child->list_members = 0;
		//parser->child->list_index = list_index;
	}
	parser->child->cb = parser->cb;
	parser->child->char_skip = skip;
	parser->child->previous_value_type = parser->value_type;
	//print_json_parser_path(parser, __LINE__);
	return parser->child;
}

json_parser* ICACHE_FLASH_ATTR drop_json_parser(json_parser* parser) {
	uint8_t skip = parser->char_skip;

	if (parser->parent_p) {
		json_parser* p = parser->parent_p;
		reset_json_parser(parser);

		parser->list_index = 0;
		parser->list_members = 0;
		parser->parent = 0;
		parser->token = 0;
		parser->char_skip = 0;
		parser->active = false;


		//print_json_parser_path(parser, __LINE__);
		return p;
	}
	else {
		LOG_ERROR("[JSON] Invalid State");
		reset_json_parser(parser);
		parser->char_skip = skip;
		//print_json_parser_path(parser, __LINE__);
		return parser;
	}
}

json_parser* ICACHE_FLASH_ATTR init_json_parser(offset_buffer* req, struct noizu_trie_options options, struct noizu_trie_definition* definition, json_streaming_parser_event_cb cb, void* output) {
	json_parser* r = (json_parser*)os_zalloc(sizeof(json_parser));
	if (r) {
		r->trie_state = os_zalloc(sizeof(struct noizu_trie_state));
		if (r->trie_state) {
			noizu_trie__init(req, definition, options, r->trie_state);
			r->trie_definition = definition;

			r->cb = cb;
			r->output = output;
			r->req = req;
			r->parse_state = PS_ADVANCE_KEY;
			r->value_type = JSON_ERROR_VALUE;
			r->active = 1;
			return r;
		}
		else {
			os_free(r);
		}
	}
	return NULL;
}

json_parser* ICACHE_FLASH_ATTR top_parser(json_parser* parser) {
	while (parser->child) {
		if (parser->child->active) {
			parser = parser->child;
		}
		else {
			break;
		}
	}
	return parser;
}

//--------------------------------
// Functions: Debug Output
//--------------------------------

#if defined(NSP_DBG_ENABLED) && NSP_DBG_ENABLED

void ICACHE_FLASH_ATTR print_json_parser_path(json_parser* parser, uint32_t line) {
	json_parser* p = parser;
	while (p->parent_p) p = p->parent_p;
	os_printf("[%d] ", line);
	while (p) {
		char key[32] = { 0 };
		if (p->key_close && p->key_close < p->req->buffer_size && p->key_start < p->req->buffer_size) {
			os_memcpy(key, parser->req->buffer + parser->key_start, parser->key_close - parser->key_start + 1);
			os_printf("(%s|%d)", key, p->value_type);
		}
		else {
			os_printf("(%d|%d)", p->token, p->value_type);
		}
		if (p->child) {
			os_printf("->");
		}
		p = p->child;
		if (p && p->active == 0) break;
	}
	os_printf("\n");
}


void ICACHE_FLASH_ATTR print_json_parser(json_parser* parser, uint8_t offset) {


	/*
	[offset] ----- Parser ------
	[offset] state: %s, token_type: %s
	[offset] [token] ([key]|[pending])
	[offset] key_start=%06d, key_close=%06d
	[offset] val_start=%06d, val_start=%06d
	[offset] Errata:
	[offset] esc=%d, null_track=%d, char_skip=%s, index=%d
	[offset] Trackers: '=%d, "=%d, {=%d, [=%d
	[offset] in_s_quote=%s, in_d_quote=%d
	[offset] --------------------
	[offset] \
	[offset]  \
	[offset    ]----------------- Parser ------------
	*/

	char* prefix = (char*)os_zalloc(offset + 1);
	os_memset(prefix, ' ', offset);
	*(prefix + offset) = '\0';






	if (parser == NULL) {
		os_printf("%s|------------ json_parser (NULL) -------------\n", prefix);
		os_printf("%s|                   NULL\n\n", prefix);
		return;
	}

	char key[32] = { 0 };
	if (parser->key_close && parser->key_close < parser->req->buffer_size && parser->key_start < parser->req->buffer_size) {
		os_memcpy(key, parser->req->buffer + parser->key_start, parser->key_close - parser->key_start + 1);
	}
	else {
		if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
			os_sprintf(key, "#%d", parser->list_index);
		}
		else {
			os_strcpy(key, "???");
		}
	}

	os_printf("%s|------------ json_parser (key=%s|depth=%d) -------------\n", prefix, key, parser->depth);
	os_printf("%s| state: , ", prefix);
	switch (parser->parse_state) {
	case PS_ADVANCE_KEY: os_printf("PS_ADVANCE_KEY"); break;
	case PS_PARSE_KEY: os_printf("PS_PARSE_KEY"); break;
	case PS_PARSE_KEY_COMPLETE: os_printf("PS_PARSE_KEY_COMPLETE"); break;
	case PS_ADVANCE_VALUE: os_printf("PS_ADVANCE_VALUE"); break;
	case PS_ADVANCE_VALUE_COMPLETE: os_printf("PS_ADVANCE_VALUE_COMPLETE"); break;
	case PS_CAPTURE_VALUE: os_printf("PS_CAPTURE_VALUE"); break;
	case PS_ADVANCE_LIST_ITEM: os_printf("PS_ADVANCE_LIST_ITEM"); break;
	case PS_CAPTURE_LIST_ITEM: os_printf("PS_CAPTURE_LIST_ITEM"); break;
	case PS_LIST_COMPLETE: os_printf("PS_LIST_COMPLETE"); break;
	case PS_LIST_ITEM_COMPLETE: os_printf("PS_LIST_ITEM_COMPLETE"); break;
	case PS_STRUCT_COMPLETE: os_printf("PS_STRUCT_COMPLETE"); break;
	case PS_COMPLETE: os_printf("PS_COMPLETE"); break;
	case PS_END_OF_BUFFER: os_printf("PS_END_OF_BUFFER"); break;
	case PS_ERROR: os_printf("PS_ERROR"); break;
	}
	os_printf(", value_type: ");
	switch (parser->value_type) {
	case JSON_ERROR_VALUE: os_printf("JSON_ERROR_VALUE"); break;
	case JSON_TRUE_VALUE: os_printf("JSON_TRUE_VALUE"); break;
	case JSON_FALSE_VALUE: os_printf("JSON_FALSE_VALUE"); break;
	case JSON_NUMERIC_VALUE: os_printf("JSON_NUMERIC_VALUE"); break;
	case JSON_QUOTE_VALUE: os_printf("JSON_QUOTE_VALUE"); break;
	case JSON_DOUBLE_QUOTE_VALUE: os_printf("JSON_DOUBLE_QUOTE_VALUE"); break;
	case JSON_LIST_VALUE: os_printf("JSON_LIST_VALUE"); break;
	case JSON_STRUCT_VALUE: os_printf("JSON_STRUCT_VALUE"); break;
	case JSON_NULL_VALUE: os_printf("JSON_NULL_VALUE"); break;
	}
	os_printf("\n");

	os_printf("%s| trie %d, token %d, parent %d, [...]\n", prefix, 0, parser->token, parser->parent);
	os_printf("%s| key_start = %06d, key_close = %06d\n", prefix, parser->key_start, parser->key_close);
	os_printf("%s| val_start = %06d, val_close = %06d\n", prefix, parser->value_start, parser->value_close);
	os_printf("%s| Eratta:\n", prefix);
	os_printf("%s| esc=%d, null=%d, skip=%d, list index=%d, list_members=%d \n", prefix, parser->escape_char, parser->null_track, parser->char_skip, parser->list_index, parser->list_members);
	os_printf("%s| Trackers: '=%03d, \"=%03d, {=%03d, [=%03d\n", prefix, parser->s_quote_track, parser->d_quote_track, parser->brace_track, parser->bracket_track);
	os_printf("%s| in_s_quote=%s,  in_d_quote=%s\n", prefix, parser->in_s_quote ? "T" : "F", parser->in_d_quote ? "T" : "F");
	os_printf("%s|---------------------------------\n", prefix);
	if (parser->child && parser->child->active) {
		os_printf("%s\\\n", prefix);
		os_printf("%s \\\n", prefix);
		os_printf("%s  \\\n", prefix);
		print_json_parser(parser->child, offset + 3);
	}
}
#endif

//--------------------------------
// Functions: Parsing Helpers
//--------------------------------

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_nullable(json_parser* parser, nullable_type type, void* out) {
	uint8_t response_code = 0;
	bool clear = true;
	if (out) {
		if (type & INT_TYPE_FLAG || type & DECIMAL_TYPE_FLAG) {
			if (parser->value_type == JSON_NUMERIC_VALUE && parser->value_close && parser->value_close >= parser->value_start) {
				clear = false;
				response_code = extract_nullable(parser->req->buffer + parser->value_start, (parser->value_close - parser->value_start) + 1, type, out);
			}
			else if (parser->value_type == JSON_NULL_VALUE) {
				response_code = 1;
			}
			else if (parser->value_type == JSON_QUOTE_VALUE || parser->value_type == JSON_DOUBLE_QUOTE_VALUE) {
				response_code = extract_nullable(parser->req->buffer + parser->value_start + 1, (parser->value_close - parser->value_start), type, out);
				response_code = (response_code && *(parser->req->buffer + parser->value_start + 1 + response_code) == *(parser->req->buffer + parser->value_start));
				clear = response_code ? 0 : 1;
			}
		}
		else if (type & BOOL_TYPE_FLAG) {
			if (parser->value_type == JSON_NULL_VALUE || parser->value_type == JSON_TRUE_VALUE || parser->value_type == JSON_FALSE_VALUE) {
				((nullable_bool_t*)out)->null = (parser->value_type == JSON_NULL_VALUE) ? NULL_VALUE : NOT_NULL_VALUE;
				((nullable_bool_t*)out)->value = (parser->value_type == JSON_TRUE_VALUE);
				response_code = 1;
				clear = false;
			}
			else {
				response_code = extract_nullable(parser->req->buffer + parser->value_start, (parser->value_close - parser->value_start) + 1, type, out);
				clear = false;
			}
		}
	}

	if (clear) {
		extract_nullable(NULL, 0, type, out);
	}

	return response_code;
}

#pragma warning( push )
#pragma warning( disable : 4146 )
uint8_t ICACHE_FLASH_ATTR extract_nullable(uint8_t* pt, uint16_t len, nullable_type type, void* out) {
	uint8_t return_code = 0;
	uint8_t has_value = NULL_VALUE;
	uint8_t* p = pt;
	uint8_t c;
	//----------------------------------
	// Nulable Int
	//----------------------------------
	if (type & INT_TYPE_FLAG || type & DECIMAL_TYPE_FLAG) {
		uint64_t acc = 0;
		bool negative = false;
		// Auto Length
		len = len ? len : 128;

		if (type & INT_TYPE_FLAG) {
			if (pt) {
				if (*pt == 'n' && *(pt + 1) == 'u' && *(pt + 2) == 'l' && *(pt + 3) == 'l') {
					pt += 3;
					return_code = 1;
				}
				else {
					c = *pt;
					if (c == '-') {
						negative = true;
						pt++;
						len--;
						c = *pt;
					}
					else if (c == '+') {
						pt++;
						len--;
						c = *pt;
					}
					if (!(c < '0' || c > '9')) has_value = NOT_NULL_VALUE;
					do {
						c = *pt;
						if (c < '0' || c > '9') break;
						else acc = (acc * 10) + (c - '0');
					} while (++pt && --len);
					return_code = (has_value == NOT_NULL_VALUE);
				}
			}


			// Invalid Sign
			if (!(type & SIGNED_TYPE_FLAG) && negative) {
				acc = return_code = 0;
				has_value = NULL_VALUE;
			}


#ifdef ENABLE__UNION_NULLABLE_TYPE
			if (type & UNION_NULLABLE_TYPE_FLAG && negative) {
				//-------------------------------
				// New Nullable Int Type
				//-------------------------------
				if (type & SIGNED_TYPE_FLAG) {
					if (type == SINT7_TYPE) {
						((nullable_int7_t*)out)->null = has_value;
						((nullable_int7_t*)out)->signed_value = (sint8_t)(negative ? -acc : acc);
					}
					else if (type == SINT15_TYPE) {
						((nullable_int15_t*)out)->null = has_value;
						((nullable_int15_t*)out)->signed_value = (sint16_t)(negative ? -acc : acc);
					}
					else if (type == SINT31_TYPE) {
						((nullable_int31_t*)out)->null = has_value;
						((nullable_int31_t*)out)->signed_value = (sint32_t)(negative ? -acc : acc);
					}
					else if (type == SINT63_TYPE) {
						((nullable_int63_t*)out)->null = has_value;
						((nullable_int63_t*)out)->signed_value = (sint64_t)(negative ? -acc : acc);
					}
					else if (type == SINT64_TYPE) {
						((nullable_int64_t*)out)->null = has_value;
						((nullable_int64_t*)out)->signed_value = (sint64_t)(negative ? -acc : acc);
					}
					else {
						return return_code = 0;
					}
				}
				else {
					if (type == UINT7_TYPE) {
						((nullable_int7_t*)out)->null = has_value;
						((nullable_int7_t*)out)->value = (uint8_t)acc;
					}
					else if (type == UINT15_TYPE) {
						((nullable_int15_t*)out)->null = has_value;
						((nullable_int15_t*)out)->value = (uint16_t)acc;
					}
					else if (type == UINT31_TYPE) {
						((nullable_int31_t*)out)->null = has_value;
						((nullable_int31_t*)out)->value = (uint32_t)acc;
					}
					else if (type == UINT63_TYPE) {
						((nullable_int63_t*)out)->null = has_value;
						((nullable_int63_t*)out)->value = (uint64_t)acc;
					}
					else if (type == UINT64_TYPE) {
						((nullable_int64_t*)out)->null = has_value;
						((nullable_int64_t*)out)->value = (uint64_t)acc;
					}
					else {
						return return_code = 0;
					}
				}
			}
			else

#endif
				//-------------------------------
				// Legacy Nullable Int Type
				//-------------------------------
				if (type & SIGNED_TYPE_FLAG) {
					if (type == SINT7_TYPE) {
						((nullable_sint7_t*)out)->null = has_value;
						((nullable_sint7_t*)out)->value = (sint8_t)(negative ? -acc : acc);
					}
					else if (type == SINT15_TYPE) {
						((nullable_sint15_t*)out)->null = has_value;
						((nullable_sint15_t*)out)->value = (sint16_t)(negative ? -acc : acc);
					}
					else if (type == SINT31_TYPE) {
						((nullable_sint31_t*)out)->null = has_value;
						((nullable_sint31_t*)out)->value = (sint32_t)(negative ? -acc : acc);
					}
					else if (type == SINT63_TYPE) {
						((nullable_sint63_t*)out)->null = has_value;
						((nullable_sint63_t*)out)->value = (negative ? -acc : acc);
					}
					else if (type == SINT64_TYPE) {
						//LOG_CRITICAL("ASSIGN TO SINT64: %c, %d 0x%x", negative ? '-' : '+', acc, acc);
						((nullable_sint64_t*)out)->null = has_value;
						((nullable_sint64_t*)out)->value = (negative ? -acc : acc);
						//LOG_CRITICAL("ASSIGNED TO SINT64: %d", ((nullable_sint64_t*)out)->value);
					}
					else {
						return return_code = 0;
					}
				}
				else {
					if (type == UINT7_TYPE) {
						((nullable_uint7_t*)out)->null = has_value;
						((nullable_uint7_t*)out)->value = (uint8_t)acc;
					}
					else if (type == UINT15_TYPE) {
						((nullable_uint15_t*)out)->null = has_value;
						((nullable_uint15_t*)out)->value = (uint16_t)acc;
					}
					else if (type == UINT31_TYPE) {
						((nullable_uint31_t*)out)->null = has_value;
						((nullable_uint31_t*)out)->value = (uint32_t)acc;
					}
					else if (type == UINT63_TYPE) {
						((nullable_uint63_t*)out)->null = has_value;
						((nullable_uint63_t*)out)->value = (uint64_t)acc;
					}
					else if (type == UINT64_TYPE) {
						//LOG_CRITICAL("ASSIGN TO UINT64: %c, %lu", negative ? '-' : '+', acc);
						((nullable_uint64_t*)out)->null = has_value;
						((nullable_uint64_t*)out)->value = (uint64_t)acc;
					}
					else {
						return return_code = 0;
					}
				}

		}
		else if (type & DECIMAL_TYPE_FLAG) {
			bool post_decimal = false;
			uint64_t deci_divisor = 1;
			uint64_t acc2 = 0;
			if (pt) {

				if (*pt == 'n' && *(pt + 1) == 'u' && *(pt + 2) == 'l' && *(pt + 3) == 'l') {
					pt += 3;
					return_code = 1;
				}
				else {

					// check sign
					c = *pt;
					if (c == '-') {
						negative = true;
						pt++;
						len--;
					}
					else if (c == '+') {
						pt++;
						len--;
					}

					// scan whole part.
					c = *pt;
					if (!(c < '0' || c > '9')) has_value = NOT_NULL_VALUE;
					do {
						c = *pt;
						if (c == '.') {
							post_decimal = true;
						}
						else if (c < '0' || c > '9') {
							break;
						}
						else {
							acc = (acc * 10) + (c - '0');
						}
					} while (pt++ && len-- && !post_decimal);

					// scan decimal part
					if (post_decimal) {
						c = *pt;
						if (!(c < '0' || c > '9')) has_value = NOT_NULL_VALUE;
						do {
							c = *pt;
							if (c < '0' || c > '9') {
								break;
							}
							else {
								deci_divisor *= 10;
								acc2 = (acc2 * 10) + (c - '0');
							}
						} while (pt++ && len--);
					}

					// set return code.
					return_code = (has_value == NOT_NULL_VALUE);
				}
			}

			// Assign		
			if (type == FLOAT_TYPE) {
				((nullable_float_t*)out)->null = has_value;
				((nullable_float_t*)out)->unit_code = 0;
				((nullable_float_t*)out)->value = (float)((has_value == NOT_NULL_VALUE) ? (negative ? -(acc + (((double)acc2) / ((double)deci_divisor))) : (acc + (((double)acc2) / ((double)deci_divisor)))) : 0);
			}
			else if (type == DOUBLE_TYPE) {
				((nullable_double_t*)out)->null = has_value;
				((nullable_double_t*)out)->unit_code = 0;
				((nullable_double_t*)out)->value = (double)((has_value == NOT_NULL_VALUE) ? (negative ? -(acc + (((double)acc2) / ((double)deci_divisor))) : (acc + (((double)acc2) / ((double)deci_divisor)))) : 0);
			}


		}
	}
	else if (type & BOOL_TYPE_FLAG) {
		uint8_t acc = 0;
		if (pt) {
			return_code = 1;
			if (os_strncmp(pt, "null", 4) == 1) acc = 2;
			else if (os_strncmp(pt, "true", 4) == 0) acc = 1;
			else if (os_strncmp(pt, "false", 5) == 0) acc = 0;
			else if (*pt >= '0' && *pt <= '9') {
				nullable_sint64_t temp = { 0 };
				return_code = 0;
				acc = extract_nullable_sint64(pt, &temp);
				if (acc) {
					pt += acc;
					acc = temp.value == 0 ? 0 : 1;
					return_code = 2;
				}
			}
			else if (*pt == '"' || *pt == '\'') {
				acc = extract_nullable(pt + 1, len - 1, type, out);
				if (acc) {
					// true/false/numeric entry must terminate
					if (*(pt + 2 + acc) == *pt) return (uint8_t)(pt - p);
					acc = 0;
					return_code = 0;
				}
			}
			else return_code = 0;
			pt += return_code == 1 ? (3 + (acc == 0)) : 0;
		}
		((nullable_bool_t*)out)->null = (return_code == 0 || acc == 2) ? NULL_VALUE : NOT_NULL_VALUE;
		((nullable_bool_t*)out)->value = (acc == 1);
	}
	else if (type & TIME_TYPE_FLAG) {
		if (type == TIME_TYPE) {
			uint8_t hour = 0;
			uint8_t minute = 0;
			if (os_strncmp(pt, "null", 4) == 0) {
				pt += 3;
				return_code = 1;
			}
			else {
				if (*pt == '\'' || *pt == '"') pt++;

				// 00:00 or 00|00 or 0:00 or 0|00 accepted (to support set alm and non standard alarm json response from go service). 
				if ((*pt >= '0' && *pt <= '9')) {
					uint8_t divider = 0;
					hour = *pt - '0';
					pt++;

					if ((*pt >= '0' && *pt <= '9')) {
						hour = hour * 10 + (*pt - '0');
						pt++;
					}

					if (*pt == ':' || *pt == '|') {
						pt++;

						if ((*pt >= '0' && *pt <= '9') &&
							(*(pt + 1) >= '0' && *(pt + 1) <= '9')) {
							has_value = NOT_NULL_VALUE;
							minute = ((*(pt + 0) - '0') * 10) + (*(pt + 1) - '0');
							pt += 2;
							return_code = 1;
						}
						else hour = 0; // malformed. 						
					}
					else hour = 0; // malformed					
				}
			}
			((nullable_time*)out)->null = has_value;
			((nullable_time*)out)->hour = hour;
			((nullable_time*)out)->minute = minute;
		}
		else if (type == DATE_TIME_TYPE) {
			uint8_t p = 0;
			nullable_sint15_t nv = { 0 };
			nullable_date_time temp = { 0 };
			uint8_t r = 1;

			if (*pt == '\'' || *pt == '"') pt++;
			while (*pt != '\0' && r) {
				r = extract_nullable_sint15(pt, &nv);
				if (p == 0) temp.year = nv.value;
				else if (p == 1) temp.month = (uint8_t)nv.value;
				else if (p == 2) temp.day = (uint8_t)nv.value;
				else if (p == 3) temp.hour = (uint8_t)nv.value;
				else if (p == 4) temp.minute = (uint8_t)nv.value;
				else if (p == 5) temp.second = (uint8_t)nv.value;
				else if (p == 6) {
					if (r == 4) {
						temp.offset_minute = nv.value % 100;
						temp.offset_hour = (nv.value - temp.offset_minute) / 100;
						has_value = NOT_NULL_VALUE;
						break;
					}
					else if (r == 2) temp.offset_hour = (uint8_t)nv.value;
					else break;
				}
				else if (p == 7) temp.offset_minute = (uint8_t)nv.value, has_value == NOT_NULL_VALUE;
				pt += r;

				// Seperator Check				
				if (((p == 0 || p == 1) && *pt == '-') || (p == 2 && (*pt == 'T' || *pt == ' ')) || ((p == 3 || p == 4) && *pt == ':') || (p == 6 && *pt == ':')) pt++;
				else if (p == 5) {
					if (*pt == '\'' || *pt == '"' || *pt == 'Z') has_value = NOT_NULL_VALUE;
					// the following will cause required break with smaller code size.
					if (*pt == '+') temp.offset_sign = 0;
					else if (*pt == '-') temp.offset_sign = 1;
					else break;
					pt++;
				}
				else break;
				p++;
			};

			if (has_value == NOT_NULL_VALUE) os_memcpy(((nullable_date_time*)out), &temp, sizeof(nullable_date_time));
			else os_memset(((nullable_date_time*)out), 0, sizeof(nullable_date_time));
			((nullable_date_time*)out)->null = has_value;
		}

	}

	return (uint8_t)(return_code ? (pt - p) : 0);
}
#pragma warning( pop )



uint8_t ICACHE_FLASH_ATTR  json_parser__extract_token(json_parser* parser, struct noizu_trie_state* state, struct noizu_trie_definition* definition, nullable_token_t* out) {
	out->null = NULL_VALUE;
	out->value = 0;
	if (parser->value_close > parser->value_start && (parser->value_type == JSON_DOUBLE_QUOTE_VALUE || parser->value_type == JSON_QUOTE_VALUE)) {
		uint8_t index = 1;
		uint8_t len = parser->value_close - parser->value_start;
		// Note value starts at the opening quote and ends at the closing quote. 
		uint8_t i = 0;
		// reset not init
		noizu_trie__reset(parser->trie_definition, parser->trie_state->options, parser->trie_state);
		state->req_position = parser->value_start + 1;
		TRIE_TOKEN o = noizu_trie__tokenize(state, definition, NULL);
		if (state->req_position == (parser->value_start + len) && !(o & TRIE_ERROR) && state->token) {
			out->null = NOT_NULL_VALUE;
			out->value = state->token;
			return 1;
		}
	}
	return 0;
}


uint8_t ICACHE_FLASH_ATTR json_parser__extract_key(json_parser* parser, nullable_string_t* out) {
	if (out == NULL) return FALSE;
	out->null = NULL_VALUE;
	if (parser->key_close > parser->key_start) {
		uint16_t s = parser->key_start;
		uint16_t len = parser->key_close - parser->key_start;
		if (*(parser->req->buffer + s) == '"' || *(parser->req->buffer + s) == '\'') len--, s++;
		//else len++;

		if (len) return strncpy_nullable_string(out, parser->req->buffer + s, len) ? TRUE : FALSE;
	}
	return FALSE;
}

/*
		if (!out->is_pointer) out->string = os_zalloc(sizeof(nullable_string_t));
		if (out->string && json_parser__extract_string(parser, out->string)) {
			out->null = out->string->null;
			out->is_pointer = true;
		}
		else if (out->string) {
			free_nullable_string(&out->string, TRUE);
			out->string = NULL;
			out->variant = ERROR_VARIANT;
		}
		else out->variant = ERROR_VARIANT;
		*/




uint8_t ICACHE_FLASH_ATTR json_parser__extract_string(json_parser* parser, nullable_string_t* out) {
	if (out == NULL) return FALSE;
	out->null = NULL_VALUE;
	if (parser->value_type == JSON_NULL_VALUE) {
		if (out->value) *out->value = '\0';
		return TRUE;
	}
	else if (parser->value_close > parser->value_start && (parser->value_type == JSON_DOUBLE_QUOTE_VALUE || parser->value_type == JSON_QUOTE_VALUE)) {
		if ((parser->value_close - parser->value_start - 1) == 0) {
			allocate_nullable_string(out, 1);
			out->null = NOT_NULL_VALUE;
			return TRUE;
		}
		else {
			return strncpy_nullable_string(out, parser->req->buffer + parser->value_start + 1, (parser->value_close - parser->value_start - 1));
		}
	}
	return FALSE;
}



uint8_t ICACHE_FLASH_ATTR json_parser__extract_date_time(json_parser* parser, nullable_date_time* out) {
	if (parser == NULL || out == NULL) return 0;
	os_memset(out, 0, sizeof(nullable_date_time));
	out->null = NULL_VALUE;

	if (parser->value_type == JSON_NULL_VALUE) return 1;
	else if (parser->value_type == JSON_DOUBLE_QUOTE_VALUE || parser->value_close == JSON_QUOTE_VALUE) {
		uint8_t* pt = parser->req->buffer + parser->value_start;
		return extract_nullable_date_time(pt, out) ? 1 : 0;
	}
	return 0;
}


uint8_t ICACHE_FLASH_ATTR json_parser__extract_time(json_parser* parser, nullable_time* out) {
	if (parser == NULL || out == NULL) return 0;
	out->null = NULL_VALUE;
	out->hour = 0;
	out->minute = 0;
	if (parser->value_type == JSON_NULL_VALUE) {
		return 1;
	}
	else if (parser->value_type == JSON_DOUBLE_QUOTE_VALUE || parser->value_close == JSON_QUOTE_VALUE) {
		uint8_t* pt = parser->req->buffer + parser->value_start;
		return extract_nullable_time(pt, out) ? 1 : 0;
	}
	return 0;
}


/*!
 * Very basic reference counter dealloc of allocated string.
 */
uint8_t ICACHE_FLASH_ATTR free_nullable_string(nullable_string_t** v, uint8_t option_flag) {
	if (v == NULL || *v == NULL) {
		//LOG_CRITICAL("FREE N.S. NULL");
		return FALSE;
	}
	uint8_t r = 0;

	// Reduce Reference as we are freeing our pointer (regardless of whether or not we actually modify the inner contents)

	//LOG_CRITICAL("FREE N.S (%d:%d), %s", (*v)->counter, option_flag, (*v)->value);

	if ((*v)->counter) (*v)->counter--;

	if (option_flag & FREE_LOCKED) r |= FREE_LOCKED;
	else if (((*v)->counter == 0) && (option_flag & (FREE_CONTENTS | FREE_STRUCT | FORCE_FREE_CONTENTS | FORCE_FREE_STRUCT | RELEASE_STRUCT))) r |= FREE_CONTENTS;
	else if (option_flag & FORCE_FREE_CONTENTS) r |= (FORCE_FREE_CONTENTS | FREE_CONTENTS);



	// Purge Contents if requested (with exception for locked pointers)
	if (r & FREE_CONTENTS) {
		// if free lock is set we must not free the underlying pointer as it may be pointed to memory that will be double freed or is otherwise nonfreeable or reserved. 
		if ((*v)->free_lock) r |= FREE_LOCKED;

		if ((*v)->free_lock) {
		}

		// Skip free operation if free locked, simply remove this specific reference to the pointer.
		if ((r & FREE_LOCKED)) {
			//LOG_CRITICAL("FREE LOCK %s", (*v)->value);
			(*v)->value = NULL;
		}
		else if ((*v)->value) {
			//LOG_CRITICAL("FREE STR %s", (*v)->value);
			os_free((*v)->value);
			(*v)->null = NULL_VALUE;
			(*v)->value = NULL;
		}
		(*v)->free_lock = FALSE;
		(*v)->allocated = 0;
	}

	// Unlink and Purge Structure if requested.
	// always dealloc if FORCE_FREE_STRUCT or if we were the last reference and caller has requested FREE or RELEASE.
	if ((option_flag & FREE_LOCKED)) {
		if ((*v)->counter == 0 && (option_flag & FREE_STRUCT)) r |= RELEASE_STRUCT;
		else if (option_flag & (FORCE_FREE_STRUCT | RELEASE_STRUCT)) r |= RELEASE_STRUCT;
	}
	else {
		if ((*v)->counter == 0 && (option_flag & (FREE_STRUCT | FORCE_FREE_STRUCT | RELEASE_STRUCT))) r |= (FREE_STRUCT | RELEASE_STRUCT);
		else if (option_flag & FORCE_FREE_STRUCT) r |= (FORCE_FREE_STRUCT | FREE_STRUCT | RELEASE_STRUCT);
		else if (option_flag & RELEASE_STRUCT) r |= RELEASE_STRUCT;
	}

	if (r & FREE_STRUCT) {
		if ((*v)->counter) {
			//LOG_CRITICAL("FREE N.S (%d:%d), %s", (*v)->counter, option_flag, (*v)->value);
			LOG_ERROR("Free Has Ref");
		}
		os_free(*v);
		*v = NULL;
	}
	else if (r & RELEASE_STRUCT) {
		//LOG_CRITICAL("RELEASE STRUCT ONLY");
		*v = NULL;
	}
	//else {
		//LOG_CRITICAL("UNCAUGHT");
	//}

	// Return code (includes information on actions taken).
	return r;
}

/*!
 * Free inner string if allocated and counter fully dereferenced.
 */
uint8_t ICACHE_FLASH_ATTR free_nullable_string_contents(nullable_string_t* v) {
	return free_nullable_string(&v, FREE_CONTENTS);
}

uint8_t ICACHE_FLASH_ATTR strncpy_nullable_string(nullable_string_t* v, uint8_t* source, uint16_t length) {
	if (v == NULL) return FALSE;
	if (source == NULL) return FALSE;

	if (length == 0) length = (uint16_t)os_strlen(source);
	if (allocate_nullable_string(v, length + 1)) {
		if (length) {
			os_strncpy(v->value, source, length);
		}
		v->null = NOT_NULL_VALUE;
		return TRUE;
	}
	return FALSE;
}

uint8_t ICACHE_FLASH_ATTR allocate_nullable_string(nullable_string_t* v, uint16_t size) {
	if (v) {
		v->null = NULL_VALUE;
		if (size == 0) {
			return TRUE;
		}
		else if (size <= v->allocated && v->value && (v->allocated - size) < MAXIMUM_NULLABLE_STRING_OVER_ALLOCATION) {
			// No need to realloc as existing buffer is sufficient and not excessively over allocated.
			os_memset(v->value, '\0', size); // only zero specified size to reduce operations;
			*(v->value + (v->allocated - 1)) = '\0'; // zero last element as failsafe to avoid string over runs. 
		}
		else {
			// increment ref counter on initial allocation.
			if (v->counter == 0) v->counter = 1;
			// if (v->value == NULL) v->counter++;

			// We must realloc as existing buffer too small or excessively large. 									
			v->allocated = 0;
			if (!v->free_lock && v->value) os_free(v->value);
			v->value = NULL;

			// Ensure minimum size allocated (this reduces need for frequent reallocation).
			// if size is smaller than minimum threshold use minimum threshold.
			// if size is almost default size but slightly smaller simply use default size instead.
			if (size < MINIMUM_NULLABLE_STRING_SIZE) size = MINIMUM_NULLABLE_STRING_SIZE;
			else if (size < DEFAULT_NULLABLE_STRING_SIZE && (DEFAULT_NULLABLE_STRING_SIZE - size) <= NULLABLE_STRING_UPSIZE_CUTOFF) size = DEFAULT_NULLABLE_STRING_SIZE;

			// Allocate

			// We can't set the true allocation due to bin types. 
			v->value = os_zalloc(size);
			if (v->value) v->allocated = size;
		}
	}
	return (v && v->value) ? TRUE : FALSE;
}

uint8_t ICACHE_FLASH_ATTR recycle_nullable_string(nullable_string_t** v, uint16_t size) {
	if (v == NULL) return FALSE;
	if ((*v)->counter > 1) {
		// nullable_string has other references, decrement counter and clear pointer so we may allocate a new entry.
		(*v)->counter--;
		*v = os_zalloc(sizeof(nullable_string_t));
		if (*v == NULL) return FALSE;
	}
	(*v)->counter = 1;
	return allocate_nullable_string(*v, size);
}

nullable_string_t* ICACHE_FLASH_ATTR copy_nullable_string(nullable_string_t* v) {
	if (v) v->counter++;
	else {
		v = os_zalloc(sizeof(nullable_string_t));
		if (v) {
			v->counter = 1;
			allocate_nullable_string(v, DEFAULT_NULLABLE_STRING_SIZE);
		}
	}
	return v;
}


noizu_realloc_code resize_dynamic_array(dynamic_array* raw, uint16_t entry_size, uint16_t required_length, uint16_t max_size, uint16_t increment, post_resize_cb cb) {
	uint16_t new_size;
	void* resize;
	if (raw == NULL) return NOZ_DYNAMIC_ARRAY__ARG_ERROR;
	if (max_size && max_size < required_length) return NOZ_DYNAMIC_ARRAY__OVERFLOW;
	if (required_length <= raw->allocated) return NOZ_DYNAMIC_ARRAY__OK_ALLOCATED;
	increment = increment ? increment : 5;
	new_size = raw->allocated;
	while (new_size <= required_length) new_size += increment;
	if (max_size && new_size > max_size) new_size = max_size;

	uint32_t al = (entry_size * (new_size + 1));
	resize = os_zalloc(al);
	if (resize == NULL) {
		return NOZ_DYNAMIC_ARRAY__ALLOC_FAIL;
	}

	if (raw->raw_array) {
		os_memcpy(resize, raw->raw_array, raw->size * entry_size);
	}
	os_free(raw->raw_array);
	raw->raw_array = resize;
	noizu_realloc_code r = NOZ_DYNAMIC_ARRAY__OK_RESIZED;
	if (cb) {
		r = cb(raw, new_size);
	}
	else {
		raw->allocated = new_size;
	}
	return r;
}

uint8_t ICACHE_FLASH_ATTR extract_nullable_token(struct noizu_trie_state* state, struct noizu_trie_definition* definition, nullable_token_t* out) {
	TRIE_TOKEN o = noizu_trie__tokenize(state, definition, NULL);
	if (state->terminator == '\0' && !(o & TRIE_ERROR) && state->token) {
		out->null = NOT_NULL_VALUE;
		out->value = state->token;
		return 1;
	}
	else {
		out->null = NULL_VALUE;
		out->value = 0;
		return 0;
	}
}
#pragma warning(pop)