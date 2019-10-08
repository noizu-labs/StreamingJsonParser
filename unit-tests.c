/*!
* @file unit-tests
* @brief test coverage for json_parser library
*
* @note relies on a proprietary test framework, test cases need to be ported to ACE Unit or similiar. 
* @author Keith Brings
* @copyright Noizu Labs, Inc 2019
*/

#include "streaming-parser.h"

#ifdef UNIT_TESTS

struct test_output {
	nullable_bool_t bool_test;
	nullable_sint63_t array_test[6];
	nullable_string_t string_array_test[6];
	nullable_uint15_t token_array_test[6];
	nullable_sint63_t enabled;
	nullable_sint63_t featured;
	nullable_sint63_t fields;
	nullable_sint63_t one;
	nullable_sint63_t two;
	nullable_sint63_t three;
	nullable_sint63_t contents;
};


struct test_output* out = NULL;
json_parser* parser = NULL;
offset_buffer* req = NULL;

void free_test_output() {
	if (out) {
		for (int i = 0; i < 6; i++) {
			if (out->string_array_test[i].value) {
				os_free(out->string_array_test[i].value);
				out->string_array_test[i].value = NULL;
			}
		}
		os_free(out);
		out = NULL;
	}
}


//! test setup method.
void ICACHE_FLASH_ATTR json_parser_test_setup() {
	json_parser_reset();
	if (req) {
		os_free(req);
	}
	req = os_zalloc(sizeof(offset_buffer));
	if (parser) {
		os_free(parser);
		parser = NULL;
	}
	free_test_output();
	out = os_zalloc(sizeof(struct test_output));
}

//! test teardown method
void ICACHE_FLASH_ATTR json_parser_test_teardown() {
	if (req) {
		os_free(req);
	}
	req = NULL;
	if (parser) {
		os_free(parser);
		parser = NULL;
	}
	free_test_output();
}

jsp_cb_command test_cb(json_parse_state state, json_parser* parser) {
	switch (state) {
		// Extract data from completed json values.
	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 0) {
			json_parser__extract_bool(parser, &out->bool_test);
		}

		if (parser->depth == 1) {
			if (parser->parent_p->value_type == JSON_LIST_VALUE) {
				int index = parser->list_index;
				json_parser__extract_long(parser, &out->array_test[index]);
			}
		}
	}
	}
	return JSPC_PROCEED;
}


jsp_cb_command test2_cb(json_parse_state state, json_parser* parser) {
	switch (state) {
		// Extract data from completed json values.
	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 1) {
			if (parser->parent_p->value_type == JSON_LIST_VALUE) {
				int index = parser->list_index;
				json_parser__extract_string(parser, &out->string_array_test[index]);
			}
		}
	}
	}
	return JSPC_PROCEED;
}


jsp_cb_command test3_cb(json_parse_state state, json_parser* parser) {
	switch (state) {
	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 1) {
			if (parser->parent_p->value_type == JSON_LIST_VALUE) {
				int index = parser->list_index;
				json_parser__extract_token(parser, value_and_units_trie(), &out->token_array_test[index]);
			}
		}
	}
	}
	return JSPC_PROCEED;
}


jsp_cb_command test4_cb(json_parse_state state, json_parser* parser) {
	switch (state) {
	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 1) {
			switch (parser->token) {
			case JK_FIELDS:
				json_parser__extract_long(parser, &out->fields);
				break;
			case JK_ENABLED:
				json_parser__extract_long(parser, &out->enabled);
				break;
			case JK_FEATURED:
				json_parser__extract_long(parser, &out->featured);
				break;

			case JK_TWO:
				json_parser__extract_long(parser, &out->two);
				break;
			}
		}
		if (parser->depth == 2) {
			switch (parser->token) {
			case JK_ONE:
				json_parser__extract_long(parser, &out->one);
				break;
			}
		}
		if (parser->depth == 3) {
			switch (parser->token) {
			case JK_CONTENTS:
				json_parser__extract_long(parser, &out->contents);
				break;
			}
		}
		if (parser->depth == 5) {

			switch (parser->token) {
			case JK_THREE:
				json_parser__extract_long(parser, &out->three);
				break;
			}
		}
	}
	}
	return JSPC_PROCEED;
}

jsp_cb_command test5b_cb(json_parse_state state, json_parser* parser) {
	switch (state) {
	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 2) {
			switch (parser->token) {
			case JK_ONE:
				json_parser__extract_long(parser, &out->one);
				break;
			}
		}
		if (parser->depth == 3) {
			switch (parser->token) {
			case JK_CONTENTS:
				json_parser__extract_long(parser, &out->contents);
				break;
			}
		}
		if (parser->depth == 5) {

			switch (parser->token) {
			case JK_THREE:
				json_parser__extract_long(parser, &out->three);
				break;
			}
		}
	}
	}
	return JSPC_PROCEED;
}


jsp_cb_command test5_cb(json_parse_state state, json_parser* parser) {
	switch (state) {
	case PS_EXTRACT_VALUE:
		switch (parser->token) {
		case JK_OPTIONS:
			if (parser->depth == 2) {
				// Avoid capture a null case. 
				parser->cb = test5b_cb;
			}
			break;
		}
		break;

	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 1) {
			switch (parser->token) {
			case JK_FIELDS:
				json_parser__extract_long(parser, &out->fields);
				break;
			case JK_ENABLED:
				json_parser__extract_long(parser, &out->enabled);
				break;
			case JK_FEATURED:
				json_parser__extract_long(parser, &out->featured);
				break;

			case JK_TWO:
				json_parser__extract_long(parser, &out->two);
				break;
			}
		}
	}
	}
	return JSPC_PROCEED;
}

//========================================
// Json Parser Tests
//========================================






/*!
 *@brief
 *
 *
 */
UNIT_TEST(parse_bool_true) {
	uint8_t json[] = "true";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	ASSERT_EQUAL(out->bool_test.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->bool_test.value, 1);
}



/*!
 *@brief
 *
 *
 */
UNIT_TEST(parse_bool_false) {
	uint8_t json[] = "false";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	ASSERT_EQUAL(out->bool_test.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->bool_test.value, 0);
}


/*!
 *@brief
 *
 *
 */
UNIT_TEST(parse_array) {
	uint8_t json[] = "[-54321,4321,-321,null,-1,-0]";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	ASSERT_EQUAL(out->array_test[0].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->array_test[0].value, -54321);

	ASSERT_EQUAL(out->array_test[1].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->array_test[1].value, 4321);

	ASSERT_EQUAL(out->array_test[2].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->array_test[2].value, -321);

	ASSERT_EQUAL(out->array_test[3].null, NULL_VALUE);
	ASSERT_EQUAL(out->array_test[3].value, 0);

	ASSERT_EQUAL(out->array_test[4].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->array_test[4].value, -1);

	ASSERT_EQUAL(out->array_test[5].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->array_test[5].value, 0);
}


UNIT_TEST(parse_string_array) {
	uint8_t json[] = "['Aaa',      null, 'BbB', \"CcC\", 123,null]";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test2_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	ASSERT_EQUAL(out->string_array_test[0].null, NOT_NULL_VALUE);
	ASSERT_STR_EQUAL(out->string_array_test[0].value, "Aaa");

	ASSERT_EQUAL(out->string_array_test[1].null, NULL_VALUE);
	ASSERT_STR_EQUAL(out->string_array_test[1].value, NULL);

	ASSERT_EQUAL(out->string_array_test[2].null, NOT_NULL_VALUE);
	ASSERT_STR_EQUAL(out->string_array_test[2].value, "BbB");

	ASSERT_EQUAL(out->string_array_test[3].null, NOT_NULL_VALUE);
	ASSERT_STR_EQUAL(out->string_array_test[3].value, "CcC");

	ASSERT_EQUAL(out->string_array_test[4].null, NULL_VALUE);
	ASSERT_STR_EQUAL(out->string_array_test[4].value, NULL);

	ASSERT_EQUAL(out->string_array_test[5].null, NULL_VALUE);
	ASSERT_STR_EQUAL(out->string_array_test[5].value, NULL);
}


UNIT_TEST(parse_token_array) {
	uint8_t json[] = "['Rubbage', null, 'degrees_celsius', 'relative_humidity', 1234]";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test3_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	ASSERT_EQUAL(out->token_array_test[0].null, NULL_VALUE);
	ASSERT_EQUAL(out->token_array_test[0].value, 0);

	ASSERT_EQUAL(out->token_array_test[1].null, NULL_VALUE);
	ASSERT_EQUAL(out->token_array_test[1].value, 0);

	ASSERT_EQUAL(out->token_array_test[2].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->token_array_test[2].value, JV_DEGREES_CELSIUS);

	ASSERT_EQUAL(out->token_array_test[3].null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->token_array_test[3].value, JV_RELATIVE_HUMIDITY);

	ASSERT_EQUAL(out->token_array_test[4].null, NULL_VALUE);
	ASSERT_EQUAL(out->token_array_test[4].value, 0);
}


UNIT_TEST(parse_struct) {
	uint8_t json[] = "{'enabled': 1, \"featured\": 2, fields: 3, inner: {apple: 1, bannana: 2, one: 5, wee: {contents: 4, empty: {}, list: [1,2,3,4,{three: 1234}, {}]}}, two: 6}";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test4_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->enabled.value, 1);
	ASSERT_EQUAL(out->featured.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->featured.value, 2);
	ASSERT_EQUAL(out->fields.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->fields.value, 3);
	ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->one.value, 5);
	ASSERT_EQUAL(out->two.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->two.value, 6);
	ASSERT_EQUAL(out->contents.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->contents.value, 4);
	ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->three.value, 1234);
}


UNIT_TEST(parse_struct_handler_switch) {
	uint8_t json[] = "{'enabled': 1, \"featured\": 2, fields: 3, options: {apple: 1, bannana: 2, one: 5, wee: {contents: 4, empty: {}, list: [1,2,3,4,{three: 1234}, {}]}}, two: 6}";
	req->buffer = &json;
	req->buffer_size = os_strlen(json) + 1;
	json_parser* parser = init_json_parser(req, json_trie(), test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->enabled.value, 1);
	ASSERT_EQUAL(out->featured.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->featured.value, 2);
	ASSERT_EQUAL(out->fields.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->fields.value, 3);
	ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->one.value, 5);
	ASSERT_EQUAL(out->two.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->two.value, 6);
	ASSERT_EQUAL(out->contents.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->contents.value, 4);
	ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	ASSERT_EQUAL(out->three.value, 1234);
}

/*!
 *@brief
 *
 *
 */
test_outcome ICACHE_FLASH_ATTR json_parser_suite() {
	test_outcome test_run = { 0 };
	test_setup_ptr s = json_parser_test_setup;
	test_teardown_ptr td = json_parser_test_teardown;


	// DataStream Revamp
	run(test_parse_bool_true, &test_run, s, td);
	run(test_parse_bool_false, &test_run, s, td);
	run(test_parse_array, &test_run, s, td);
	run(test_parse_string_array, &test_run, s, td);
	run(test_parse_token_array, &test_run, s, td);
	run(test_parse_struct, &test_run, s, td);
	run(test_parse_struct_handler_switch, &test_run, s, td);


	return test_run;
}

#endif