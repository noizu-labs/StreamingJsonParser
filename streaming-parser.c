#include "streaming-parser.h"

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
void ICACHE_FLASH_ATTR free_parser_children(json_parser* base);
json_parser* ICACHE_FLASH_ATTR spawn_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR drop_json_parser(json_parser* parser);
json_parser* ICACHE_FLASH_ATTR init_json_parser(offset_buffer*, noizu_trie_a* trie, json_streaming_parser_event_cb cb, void* output);
json_parser* ICACHE_FLASH_ATTR top_parser(json_parser* parser);

//--------------------------------
// Declarations: Debug Output
//--------------------------------
void ICACHE_FLASH_ATTR print_json_parser_path(json_parser* parser, uint32_t line);
void ICACHE_FLASH_ATTR print_json_parser(json_parser* parser, uint8_t offset);

//--------------------------------
// Declarations: Parsing Helpers
//--------------------------------
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
				parser->token = t;
			}
			else if (parser->value_type == JSON_LIST_VALUE) {
				parser = spawn_json_parser(parser);
				parser->bracket_track = 1;
				parser->token = t;
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
				parser->token = t;
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
	for (; (parser->req->buffer_pos + 1) < parser->req->buffer_size; parser->req->buffer_pos++) {
		//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));		
		c = *(p + parser->req->buffer_pos);
		switch (c) {
		case '"':
			parser->char_skip = 1;
			parser->key_start = parser->req->buffer_pos;
			parser->token = 0;
			parser->trie_index = 1;
			parser->parse_state = PS_PARSE_KEY;
			parser->in_d_quote = 1;
			parser->d_quote_track = 1;
			return parser->parse_state;

		case '\'':
			parser->char_skip = 1;
			parser->key_start = parser->req->buffer_pos;
			parser->token = 0;
			parser->trie_index = 1;
			parser->parse_state = PS_PARSE_KEY;
			parser->in_s_quote = 1;
			parser->s_quote_track = 1;
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
				parser->char_skip = 1;
				parser->key_start = parser->req->buffer_pos;
				parser->token = 0;
				parser->trie_index = 1;
				parser->trie_index = noizu_trie_a_advance(*(p + parser->req->buffer_pos), parser->trie_index, parser->trie);
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
	for (; (parser->req->buffer_pos + 1) < parser->req->buffer_size; parser->req->buffer_pos++) {
		c = *(p + parser->req->buffer_pos);

		if (parser->escape_char) {
			parser->escape_char = 0;
			switch (c) {
			case '\\':
				parser->trie_index = noizu_trie_a_advance('\\', parser->trie_index, parser->trie);
				break;
			case 'n':
				parser->trie_index = noizu_trie_a_advance('\n', parser->trie_index, parser->trie);
				break;
			case 'r':
				parser->trie_index = noizu_trie_a_advance('\r', parser->trie_index, parser->trie);
				break;
			case 't':
				parser->trie_index = noizu_trie_a_advance('\t', parser->trie_index, parser->trie);
				break;
			case '"':
				parser->trie_index = noizu_trie_a_advance('"', parser->trie_index, parser->trie);
				break;
			case '\'':
				parser->trie_index = noizu_trie_a_advance('\'', parser->trie_index, parser->trie);
				break;
			default:
				LOG_ERROR("[JSON] Unsupported escape %c", *(p + parser->req->buffer_pos));
			}
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

			if (s) {
				parser->escape_char = 0;
				parser->in_d_quote = 0;
				parser->in_s_quote = 0;
				parser->s_quote_track = 0;
				parser->d_quote_track = 0;
				parser->token = parser->trie[parser->trie_index][3];
				parser->parse_state = PS_ADVANCE_VALUE;
				return PS_PARSE_KEY_COMPLETE;
			}
			if (!parser->escape_char) parser->trie_index = noizu_trie_a_advance(c, parser->trie_index, parser->trie);
		}

		/*
				//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
				if (*(p + parser->req->buffer_pos) != '"') {
					//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
					//LOG_ERROR("TRIE INDEX = %d, %s", parser->trie_index, parser->trie == NULL ? "NULL" : "OK");
					parser->trie_index = noizu_trie_a_advance(*(p + parser->req->buffer_pos), parser->trie_index, parser->trie);
				} else {
					//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));

				}
				//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
				*/
	}
	//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
	if (parser->req->buffer_pos && (parser->req->buffer_pos == parser->req->buffer_size)) {
		//LOG_ERROR("INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
		parser->req->buffer_pos--;
	}
	return PS_END_OF_BUFFER;
}

json_parse_state ICACHE_FLASH_ATTR json_process__ADVANCE_VALUE(json_parser* parser) {
	uint8_t* p = parser->req->buffer;
	for (; (parser->req->buffer_pos + 1) < parser->req->buffer_size; parser->req->buffer_pos++) {
		//LOG_ERROR("[JSON] INSIDE LOOP %d:%d, [%c] - '%s'", parser->req->buffer_pos, parser->req->buffer_size, *(p + parser->req->buffer_pos), (p + parser->req->buffer_pos));
		// Advance until we encounter a '{', null, true, false, '[', '"', '\''
		uint8_t c = *(p + parser->req->buffer_pos);
		switch (c) {
		case ']':
			//LOG_ERROR("[JSON] ADVANCE VALUE Reached %c", *(p + parser->req->buffer_pos));
			//if (parser->parent_p && parser->parent_p->value_type == JSON_LIST_VALUE) {
			parser->parent_p->value_close = parser->req->buffer_pos;
			if (parser->parent_p->value_type == JSON_LIST_VALUE) {
				parser->parent_p->parse_state = PS_LIST_COMPLETE;
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
			parser->parent_p->value_close = parser->req->buffer_pos;
			parser->parent_p->parse_state = PS_COMPLETE;
			if (parser->parent_p->value_type == JSON_STRUCT_VALUE) {
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

	for (; (parser->req->buffer_pos + 1) < parser->req->buffer_size; parser->req->buffer_pos++) {
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
	noizu_trie_a* trie = parser->trie;
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
	parser->trie = trie;
	parser->output = output;
	parser->active = 1;
	parser->char_skip = skip;
	parser->trie_index = 1;
	parser->child = c;
	parser->parent_p = p;
	parser->parent = pt;
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

void ICACHE_FLASH_ATTR free_parser_children(json_parser* base) {
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
			return;
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
		parser->child = init_json_parser(parser->req, parser->trie, parser->cb, parser->output);
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

json_parser* ICACHE_FLASH_ATTR init_json_parser(offset_buffer* req, noizu_trie_a* trie, json_streaming_parser_event_cb cb, void* output) {
	json_parser* r = (json_parser*)os_zalloc(sizeof(json_parser));
	r->cb = cb;
	r->output = output;
	r->req = req;
	r->trie = trie;
	r->trie_index = 1;
	r->parse_state = PS_ADVANCE_KEY;
	r->value_type = JSON_ERROR_VALUE;
	r->active = 1;
	return r;
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

	char* prefix = (char*)os_malloc(offset + 1);
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

	os_printf("%s| trie %d, token %d, parent %d, [...]\n", prefix, parser->trie_index, parser->token, parser->parent);
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

//--------------------------------
// Functions: Parsing Helpers
//--------------------------------
uint8_t ICACHE_FLASH_ATTR  json_parser__extract_token(json_parser* parser, noizu_trie_a* trie, nullable_token_t* out) {
	out->null = NULL_VALUE;
	out->value = 0;
	if (parser->value_close > parser->value_start && (parser->value_type == JSON_DOUBLE_QUOTE_VALUE || parser->value_type == JSON_QUOTE_VALUE)) {
		uint8_t index = 1;
		uint8_t len = parser->value_close - parser->value_start;
		// Note value starts at the opening quote and ends at the closing quote. 
		for (uint8_t i = 1; i < len; i++) {
			index = noizu_trie_a_advance(*(parser->req->buffer + parser->value_start + i), index, trie);
		}
		if (trie[index][TRIE_A_TOKEN]) {
			out->null = NOT_NULL_VALUE;
			out->value = trie[index][TRIE_A_TOKEN];
			return 1;
		}
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_key(json_parser* parser, nullable_string_t* out) {
	if (out->value) {
		os_free(out->value);
		out->value = NULL;
	}
	out->null = NULL_VALUE;

	if (parser->value_type == JSON_NULL_VALUE) {
		return 1;
	}

	if (parser->key_close > parser->key_start) {
		uint16_t s = parser->key_start;
		uint16_t len = parser->key_close - parser->key_start;
		if (*(parser->req->buffer + s) == '"' || *(parser->req->buffer + s) == '\'') {
			len--;
			s++;
		}
		else {
			len++;
		}
		if (len >= 0) {
			out->value = (uint8_t*) os_zalloc(len + 1);
			if (out->value) {
				if (len) {
					os_memcpy(out->value, (parser->req->buffer + s), len);
					out->null = NOT_NULL_VALUE;
					return 1;
				}
			}
		}
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_bool(json_parser* parser, nullable_bool_t* out) {
	out->null = NULL_VALUE;
	out->value = 0;
	if (parser->value_type == JSON_NULL_VALUE) return 1;
	if (parser->value_type == JSON_TRUE_VALUE || parser->value_type == JSON_FALSE_VALUE) {
		out->null = NOT_NULL_VALUE;
		out->value = parser->value_type == JSON_TRUE_VALUE ? 1 : 0;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_string(json_parser* parser, nullable_string_t* out) {
	if (out->value) {
		os_free(out->value);
		out->value = NULL;
	}
	out->null = NULL_VALUE;

	if (parser->value_type == JSON_NULL_VALUE) {
		return 1;
	}

	if (parser->value_close > parser->value_start && (parser->value_type == JSON_DOUBLE_QUOTE_VALUE || parser->value_type == JSON_QUOTE_VALUE)) {
		uint16_t len = parser->value_close - parser->value_start - 1;
		out->value = (uint8_t*) os_zalloc(len + 1);
		if (out->value) {
			if (len) {
				os_memcpy(out->value, (parser->req->buffer + parser->value_start + 1), len);
				out->null = NOT_NULL_VALUE;
				return 1;
			}
		}
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_sint7(json_parser* parser, nullable_sint7_t* out) {
	nullable_sint64_t i = {.null = NULL_VALUE,.value = 0};
	if (json_parser__extract_sint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_sint15(json_parser* parser, nullable_sint15_t* out) {
	nullable_sint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_sint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_sint31(json_parser* parser, nullable_sint31_t* out) {
	nullable_sint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_sint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_sint63(json_parser* parser, nullable_sint63_t* out) {
	nullable_sint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_sint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_sint64(json_parser* parser, nullable_sint64_t* out) {
	if (parser->value_type == JSON_NULL_VALUE) {
		out->null = NULL_VALUE;
		out->value = 0;
		return 1;
	}
	else if (parser->value_type == JSON_NUMERIC_VALUE) {
		if (parser->value_close && parser->value_close >= parser->value_start) {
			uint8_t* pt = parser->req->buffer + parser->value_start;
			uint8_t len = (parser->value_close - parser->value_start) + 1;
			bool negative = false;
			bool has_value = false;
			int32_t acc = 0;
			uint8_t c = *pt;
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
			if (!(c < '0' || c > '9')) has_value = true;
			do {
				c = *pt;
				if (c < '0' || c > '9') break;
				else acc = (acc * 10) + (c - '0');
			} while (++pt && --len);
			out->null = has_value ? NOT_NULL_VALUE : NULL_VALUE;
			out->value = negative ? -acc : acc;
			return 1;
		}
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint7(json_parser* parser, nullable_uint7_t* out) {
	nullable_uint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_uint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint15(json_parser* parser, nullable_uint15_t* out) {
	nullable_uint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_uint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint31(json_parser* parser, nullable_uint31_t* out) {
	nullable_uint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_uint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR  json_parser__extract_uint63(json_parser* parser, nullable_uint63_t* out) {
	nullable_uint64_t i = { .null = NULL_VALUE,.value = 0 };
	if (json_parser__extract_uint64(parser, &i)) {
		out->null = i.null;
		out->value = i.value;
		return 1;
	}
	return 0;
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_uint64(json_parser* parser, nullable_uint64_t* out) {
	if (parser->value_type == JSON_NULL_VALUE) {
		out->null = NULL_VALUE;
		out->value = 0;
		return 1;
	}
	else if (parser->value_type == JSON_NUMERIC_VALUE) {
		if (parser->value_close && parser->value_close >= parser->value_start) {
			uint8_t* pt = parser->req->buffer + parser->value_start;
			uint8_t len = (parser->value_close - parser->value_start) + 1;
			bool has_value = false;
			uint32_t acc = 0;
			uint8_t c = *pt;
			if (c == '-') return 0;
			else if (c == '+') pt++, len--;
			if (!(c < '0' || c > '9')) has_value = true;
			do {
				c = *pt;
				if (c < '0' || c > '9') break;
				else acc = (acc * 10) + (c - '0');
			} while (++pt && --len);
			out->null = has_value ? NOT_NULL_VALUE : NULL_VALUE;
			out->value = acc;
			return 1;
		}
	}
	return 0;
}


uint8_t ICACHE_FLASH_ATTR json_parser__extract_float(json_parser* parser, nullable_float_t* out) {
	nullable_double_t v = { .null = 1,.value = 0.0 };
	if (json_parser__extract_double(parser, &v)) {
		out->null = v.null;
		out->value = v.value;
		return 1;
	}
	else {
		return 0;
	}
}

uint8_t ICACHE_FLASH_ATTR json_parser__extract_double(json_parser* parser, nullable_double_t* out) {
	if (parser->value_type == JSON_NULL_VALUE) {
		out->null = NULL_VALUE;
		out->value = 0;
		return 1;
	}
	else if (parser->value_type == JSON_NUMERIC_VALUE) {
		if (parser->value_close && parser->value_close >= parser->value_start) {
			uint8_t* pt = (parser->req->buffer + parser->value_start);
			uint8_t len = (parser->value_close - parser->value_start) + 1;
			bool negative = false;
			bool decimal = false;
			bool has_value = false;
			double acc = 0;
			double acc_d = 0;
			double dps = 1;
			uint8_t c = *pt;
			if (c == '-') negative = true, pt++, len--;
			else if (c == '+') pt++, len--;
			if (!(c < '0' || c > '9')) has_value = true;
			do {
				c = *pt;
				//LOG_ERROR("C = %c\n", c);

				if (c == '.') {
					if (decimal) return 0;
					else decimal = true;
				}
				else if (c < '0' || c > '9') {
					break;
				}
				else {
					if (decimal) {
						dps *= 10;
						acc_d = (acc_d * 10) + (c - '0');
					}
					else {
						acc = (acc * 10) + (c - '0');
					}
				}
			} while (++pt && --len);
			out->null = has_value ? NOT_NULL_VALUE : NULL_VALUE;
			out->value = negative ? -(acc + (acc_d / dps)) : (acc + (acc_d / dps));
			return 1;
		}
	}
	return 0;
}
