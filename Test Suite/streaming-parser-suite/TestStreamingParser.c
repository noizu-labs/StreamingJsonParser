#include "streaming-parser.h"
#include "unity.h"
#include "unity_fixture.h"
#include "support/array_trie.h"


TEST_GROUP(StreamingParser);

extern struct noizu_trie_definition array_test_trie;

struct test_output {
	nullable_bool_t bool_test;
	nullable_sint63_t array_test[6];
	nullable_string_t string_array_test[6];
	nullable_token_t token_array_test[6];
	nullable_bool_t enabled;
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
				json_parser__extract_sint63(parser, &out->array_test[index]);
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
				json_parser__extract_token(parser, parser->trie_state, parser->trie_definition, &out->token_array_test[index]);
				
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
				json_parser__extract_sint63(parser, &out->fields);
				break;
			case JK_ENABLED:
				json_parser__extract_bool(parser, &out->enabled);
				break;
			case JK_FEATURED:
				json_parser__extract_sint63(parser, &out->featured);
				break;

			case JK_TWO:
				json_parser__extract_sint63(parser, &out->two);
				break;
			}
		}
		if (parser->depth == 2) {
			switch (parser->token) {
			case JK_ONE:
				json_parser__extract_sint63(parser, &out->one);
				break;
			}
		}
		if (parser->depth == 3) {
			switch (parser->token) {
			case JK_CONTENTS:
				json_parser__extract_sint63(parser, &out->contents);
				break;
			}
		}
		if (parser->depth == 5) {

			switch (parser->token) {
			case JK_THREE:
				json_parser__extract_sint63(parser, &out->three);
				break;
			}
		}
	}
	}
	return JSPC_PROCEED;
}


jsp_cb_command test4b_cb(json_parse_state state, json_parser* parser) {
	switch (state) {

	case PS_LIST_ITEM_COMPLETE:
	case PS_COMPLETE:
	{
		struct test_output* out = parser->output;
		if (parser->depth == 1) {
			switch (parser->token) {
			case JK_FIELDS:
				json_parser__extract_sint63(parser, &out->fields);
				break;
			case JK_ENABLED:
				json_parser__extract_bool(parser, &out->enabled);
				break;
			case JK_FEATURED:
				json_parser__extract_sint63(parser, &out->featured);
				break;

			case JK_TWO:
				json_parser__extract_sint63(parser, &out->two);
				break;
			}
		}
		if (parser->depth == 3) {
			switch (parser->token) {
			case JK_ONE:
				json_parser__extract_sint63(parser, &out->one);
				break;
			}
		}
		if (parser->depth == 4) {
			switch (parser->token) {
			case JK_CONTENTS:
				json_parser__extract_sint63(parser, &out->contents);
				break;
			}
		}
		if (parser->depth == 6) {

			switch (parser->token) {
			case JK_THREE:
				json_parser__extract_sint63(parser, &out->three);
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
				json_parser__extract_sint63(parser, &out->one);
				break;
			}
		}
		if (parser->depth == 3) {
			switch (parser->token) {
			case JK_CONTENTS:
				json_parser__extract_sint63(parser, &out->contents);
				break;
			}
		}
		if (parser->depth == 5) {

			switch (parser->token) {
			case JK_THREE:
				json_parser__extract_sint63(parser, &out->three);
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
				json_parser__extract_sint63(parser, &out->fields);
				break;
			case JK_ENABLED:
				json_parser__extract_bool(parser, &out->enabled);

				break;
			case JK_FEATURED:
				json_parser__extract_sint63(parser, &out->featured);
				break;

			case JK_ONE:
				json_parser__extract_sint63(parser, &out->one);
				break;
			case JK_TWO:
				json_parser__extract_sint63(parser, &out->two);
				break;
			case JK_THREE:
				json_parser__extract_sint63(parser, &out->three);
				break;
			}
		}
	}
	}
	return JSPC_PROCEED;
}

TEST_SETUP(StreamingParser)
{
	json_parser_globals_reset();
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

TEST_TEAR_DOWN(StreamingParser)
{
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



TEST(StreamingParser, UnitTest_ResizeDynamicArray)
{

	dynamic_array* sut = os_zalloc(sizeof(dynamic_array));
	if (sut == NULL) {
		TEST_IGNORE();
	}
	noizu_realloc_code r = resize_dynamic_array(sut, sizeof(uint8_t), 256, 512, 7, NULL);
	TEST_ASSERT_EQUAL(r, NOZ_DYNAMIC_ARRAY__OK_RESIZED);
	TEST_ASSERT_NOT_NULL(sut);
	TEST_ASSERT_NOT_NULL(sut->raw_array);
	TEST_ASSERT_TRUE(sut->allocated > 256);
	TEST_ASSERT_TRUE(sut->allocated <= (256 + 7));

	sut->size = 256;
	*((uint8_t*)sut->raw_array) = 'A';
	*((uint8_t*)sut->raw_array + 255) = 'Z';

	r = resize_dynamic_array(sut, sizeof(uint8_t), 257, 512, 7, NULL);
	TEST_ASSERT_NOT_NULL(sut->raw_array);
	TEST_ASSERT_EQUAL(r, NOZ_DYNAMIC_ARRAY__OK_ALLOCATED);
	TEST_ASSERT_TRUE(sut->allocated > 257);
	TEST_ASSERT_TRUE(sut->allocated <= (257 + 7));

	TEST_ASSERT_EQUAL(*((uint8_t*)sut->raw_array), 'A');
	TEST_ASSERT_EQUAL(*((uint8_t*)sut->raw_array + 255), 'Z');

	r = resize_dynamic_array(sut, sizeof(uint8_t), 511, 512, 7, NULL);
	TEST_ASSERT_NOT_NULL(sut->raw_array);
	TEST_ASSERT_EQUAL(r, NOZ_DYNAMIC_ARRAY__OK_RESIZED);
	TEST_ASSERT_TRUE(sut->allocated > 511);
	TEST_ASSERT_TRUE(sut->allocated <= (511 + 7));

	TEST_ASSERT_EQUAL(*((uint8_t*)sut->raw_array), 'A');
	TEST_ASSERT_EQUAL(*((uint8_t*)sut->raw_array + 255), 'Z');

	r = resize_dynamic_array(sut, sizeof(uint8_t), 513, 512, 7, NULL);
	TEST_ASSERT_NOT_NULL(sut->raw_array);
	TEST_ASSERT_EQUAL(r, NOZ_DYNAMIC_ARRAY__OVERFLOW);
	TEST_ASSERT_TRUE(sut->allocated > 511);
	TEST_ASSERT_TRUE(sut->allocated <= (511 + 7));

	TEST_ASSERT_EQUAL(*((uint8_t*)sut->raw_array), 'A');
	TEST_ASSERT_EQUAL(*((uint8_t*)sut->raw_array + 255), 'Z');

}

TEST(StreamingParser, UnitTest_ParseNestedStruct) {

	uint8_t json[] = "{'enabled': 1, \"featured\": 2, fields: 3, \"inner\": {\"inner\": {apple: 1, bannana: 2, one: 5, wee: {contents: 4, empty: {}, list: [1,2,3,4,{three: 1234}, {}]}}}, two: 6}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test4b_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 1);
	TEST_ASSERT_EQUAL(out->featured.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->featured.value, 2);
	TEST_ASSERT_EQUAL(out->fields.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->fields.value, 3);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 5);
	TEST_ASSERT_EQUAL(out->two.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 6);
	TEST_ASSERT_EQUAL(out->contents.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->contents.value, 4);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 1234);


}

/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolTrue)
{
	uint8_t json[] = "true";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->bool_test.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->bool_test.value, 1);
}

/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolFalse)
{
	uint8_t json[] = "false";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->bool_test.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->bool_test.value, 0);
}



/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseNull)
{
	uint8_t json[] = "null";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->bool_test.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->bool_test.value, 0);
}





/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolTrueInner)
{
	uint8_t json[] = "{one:1,\"enabled\":true,'three':4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value,  1);
	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 1);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 4);	
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolFalseInner)
{
	uint8_t json[] = "{one:1,\"enabled\":false,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 4);
}


/*!
 *@brief
 *
 *
 */
	TEST(StreamingParser, UnitTest_ParseBoolTrueInner_Typo)
{
	uint8_t json[] = "{one:1,\"enabled\":troe,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->enabled.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 0);
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolFalseInner_Typo)
{
	uint8_t json[] = "{one:1,\"enabled\":filse,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->enabled.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 0);
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseNullInner_Typo)
{
	uint8_t json[] = "{one:1,\"two\":nill,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->two.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 0);
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolTrueInner_Overflow)
{
	uint8_t json[] = "{one:1,\"enabled\":truuuuueeee,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->enabled.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 0);
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolFalseInner_Overflow)
{
	uint8_t json[] = "{one:1,\"enabled\":faaaaaalse,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->enabled.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 0);
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseNullInner_Overflow)
{
	uint8_t json[] = "{one:1,\"two\":nuuuuullll,'three': 4}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 1);
	TEST_ASSERT_EQUAL(out->two.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 0);
}







/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseNullInner)
{
	uint8_t json[] = "{one:4,\"two\":null,'three':6}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 4);
	TEST_ASSERT_EQUAL(out->two.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 6);
}







/*!
 *@brief
 *
 * This covers a regression caused by TRUE|FALSE methods falling through to NULL handler on line break (on rentry after buffer update system would incorrectly treate double 'l' as NULL case.
 */
TEST(StreamingParser, UnitTest_ParseBoolFalse_NullFallThroughEdgeCase)
{
	uint8_t json[] = "{one:4,\"enabled\":fal";
	uint8_t json2[] = "se,'three':6}";
	uint8_t scratch[256] = { 0 };
	req->buffer = scratch;
	os_strncpy(req->buffer, json, os_strlen(json));
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	os_strncpy(req->buffer + req->buffer_size, json2, os_strlen(json2));
	req->buffer_size += os_strlen(json2);
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(parser->null_track, 0);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 4);
	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 6);
}



/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolFalseInner_SplitEdgeCase)
{
	uint8_t json[] = "{one:4,\"enabled\":false";
	uint8_t json2[] = ",'three':6}";
	uint8_t scratch[256] = { 0 };
	req->buffer = scratch;
	os_strncpy(req->buffer, json, os_strlen(json));
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	os_strncpy(req->buffer + req->buffer_size, json2, os_strlen(json2));
	req->buffer_size += os_strlen(json2);
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 4);
	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 6);
}


/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseBoolTrueInner_SplitEdgeCase)
{
	uint8_t json[] = "{one:4,\"enabled\":tru";
	uint8_t json2[] = "e,'three':6}";
	uint8_t scratch[256] = { 0 };
	req->buffer = scratch;
	os_strncpy(req->buffer, json, os_strlen(json));
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	os_strncpy(req->buffer + req->buffer_size, json2, os_strlen(json2));
	req->buffer_size += os_strlen(json2);
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 4);
	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 1);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 6);
}




/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseNullInner_SplitEdgeCase)
{
	// This previously failed due to unset skip_chart in null string detection.
	uint8_t json[] = "{one:4,\"two\":nul";
	uint8_t json2[] = "l,'three':6}";
	uint8_t scratch[256] = { 0 };
	req->buffer = scratch;
	os_strncpy(req->buffer, json, os_strlen(json));
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	
	os_strncpy(req->buffer + req->buffer_size, json2, os_strlen(json2));
	req->buffer_size += os_strlen(json2);
	json_streaming_parser(parser);
		
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 4);
	TEST_ASSERT_EQUAL(out->two.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 0);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 6);
}



/*!
 *@brief
 *
 *
 */
TEST(StreamingParser, UnitTest_ParseArray)
{
	uint8_t json[] = "[-54321,4321,-321,null,-1,-0]";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->array_test[0].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->array_test[0].value, -54321);

	TEST_ASSERT_EQUAL(out->array_test[1].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->array_test[1].value, 4321);

	TEST_ASSERT_EQUAL(out->array_test[2].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->array_test[2].value, -321);

	TEST_ASSERT_EQUAL(out->array_test[3].null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->array_test[3].value, 0);

	TEST_ASSERT_EQUAL(out->array_test[4].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->array_test[4].value, -1);

	TEST_ASSERT_EQUAL(out->array_test[5].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->array_test[5].value, 0);
}

TEST(StreamingParser, UnitTest_ParseStringArray)
{
	uint8_t json[] = "['Aaa',      null, 'BbB', \"CcC\", 123,null]";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test2_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->string_array_test[0].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL_STRING(out->string_array_test[0].value, "Aaa");

	TEST_ASSERT_EQUAL(out->string_array_test[1].null, NULL_VALUE);
	TEST_ASSERT_EQUAL_STRING(out->string_array_test[1].value, NULL);

	TEST_ASSERT_EQUAL(out->string_array_test[2].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL_STRING(out->string_array_test[2].value, "BbB");

	TEST_ASSERT_EQUAL(out->string_array_test[3].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL_STRING(out->string_array_test[3].value, "CcC");

	TEST_ASSERT_EQUAL(out->string_array_test[4].null, NULL_VALUE);
	TEST_ASSERT_EQUAL_STRING(out->string_array_test[4].value, NULL);

	TEST_ASSERT_EQUAL(out->string_array_test[5].null, NULL_VALUE);
	TEST_ASSERT_EQUAL_STRING(out->string_array_test[5].value, NULL);

}

TEST(StreamingParser, UnitTest_ParseTokenArray)
{

	uint8_t json[] = "['Rubbage', null, 'degrees_celsius', 'relative_humidity', 1234]";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test3_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->token_array_test[0].null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->token_array_test[0].value, 0);

	TEST_ASSERT_EQUAL(out->token_array_test[1].null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->token_array_test[1].value, 0);

	TEST_ASSERT_EQUAL(out->token_array_test[2].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->token_array_test[2].value, JV_DEGREES_CELSIUS);

	TEST_ASSERT_EQUAL(out->token_array_test[3].null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->token_array_test[3].value, JV_RELATIVE_HUMIDITY);

	TEST_ASSERT_EQUAL(out->token_array_test[4].null, NULL_VALUE);
	TEST_ASSERT_EQUAL(out->token_array_test[4].value, 0);
}


TEST(StreamingParser, UnitTest_ParseStruct)
{
	uint8_t json[] = "{'enabled': 1, \"featured\": 2, fields: 3, inner: {apple: 1, bannana: 2, one: 5, wee: {contents: 4, empty: {}, list: [1,2,3,4,{three: 1234}, {}]}}, two: 6}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test4_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 1);
	TEST_ASSERT_EQUAL(out->featured.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->featured.value, 2);
	TEST_ASSERT_EQUAL(out->fields.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->fields.value, 3);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 5);
	TEST_ASSERT_EQUAL(out->two.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 6);
	TEST_ASSERT_EQUAL(out->contents.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->contents.value, 4);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 1234);
}


TEST(StreamingParser, UnitTest_ParseStructHandlerSwitch)
{

	uint8_t json[] = "{'enabled': 1, \"featured\": 2, fields: 3, options: {apple: 1, bannana: 2, one: 5, wee: {contents: 4, empty: {}, list: [1,2,3,4,{three: 1234}, {}]}}, two: 6}";
	req->buffer = json;
	req->buffer_size = ((uint32_t)os_strlen(json)) + 1;
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 1);
	TEST_ASSERT_EQUAL(out->featured.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->featured.value, 2);
	TEST_ASSERT_EQUAL(out->fields.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->fields.value, 3);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 5);
	TEST_ASSERT_EQUAL(out->two.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 6);
	TEST_ASSERT_EQUAL(out->contents.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->contents.value, 4);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 1234);
}



/*!
 * @brief Verify null value parsing in extract_nullable_sint31_t
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableSint31AsNull)
{
	uint8_t input[] = "null";
	uint8_t* buf = input;
	nullable_sint31_t sut;

	extract_nullable_sint31(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, 0);
}

/*!
 * @brief Verify decimal parsing in extract_nullable_sint31_t.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableSint31AsFloat)
{
	uint8_t input[] = "123.32";
	uint8_t* buf = input;
	nullable_sint31_t sut;

	extract_nullable_sint31(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, 123);
}

/*!
 * @brief Verify integer parsing in extract_nullable_sint31_t.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableSint31AsInt)
{
	uint8_t input[] = "951";
	uint8_t* buf = input;
	nullable_sint31_t sut;

	extract_nullable_sint31(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, 951);
}

/*!
 * @brief Verify null parsing in extract_nullable_numeric.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableDoubleAsNull)
{

	uint8_t input[] = "null";
	uint8_t* buf = input;
	nullable_double_t sut;

	extract_nullable_double(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, 0);
}

/*!
 * @brief Verify decimal parsing in extract_nullable_numeric.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableDoubleAsFloat)
{
	uint8_t input[] = "123.45";
	uint8_t* buf = input;
	nullable_double_t sut;

	extract_nullable_double(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, 123.45);
}

/*!
 * @brief Verify integer parsing in extract_nullable_numeric.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableDoubleAsInt)
{
	uint8_t input[] = "951";
	uint8_t* buf = input;
	nullable_double_t sut;

	extract_nullable_double(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, 951);
}

/*!
 * @brief Verify negative decimal parsing in extract_nullable_numeric.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableDoubleAsNegativeFloat)
{
	uint8_t input[] = "-123.45";
	uint8_t* buf = input;
	nullable_double_t sut;

	extract_nullable_double(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, -123.45);
}

/*!
 * @brief Verify negative integer parsing in extract_nullable_numeric.
 *
 * See @ref parsers "Parser Notes".
 */
TEST(StreamingParser, UnitTest_ExtractNullableDoubleAsNegativeInt)
{
	uint8_t input[] = "-951";
	uint8_t* buf = input;
	nullable_double_t sut;

	extract_nullable_double(buf, &sut);
	TEST_ASSERT_EQUAL(sut.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(sut.value, -951);
}


TEST(StreamingParser, UnitTest_BufferEnd)
{

	uint8_t json[] = "{'enabled': 1, \"featur";
	uint8_t json2[] = "ed\": 2, fields: 3, options: {apple: 1, bannana: 2, one: 5, wee: {contents: 4, empty: {}, list: [1,2,3,4,{three: 1234}, {}]}}, two: 6}";
	req->buffer = os_zalloc(512);
	os_strncpy(req->buffer, json, os_strlen(json));
	req->buffer_size = ((uint32_t)os_strlen(json));
	noizu_trie_options options = { 0 };
	json_parser* parser = init_json_parser(req, options, &array_test_trie, test5_cb, out);
	parser->parse_state = PS_ADVANCE_VALUE;
	json_streaming_parser(parser);
	os_strncpy(req->buffer + req->buffer_size , json2, os_strlen(json2));
	req->buffer_size += os_strlen(json2) + 1;
	json_streaming_parser(parser);

	TEST_ASSERT_EQUAL(out->enabled.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->enabled.value, 1);
	TEST_ASSERT_EQUAL(out->featured.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->featured.value, 2);
	TEST_ASSERT_EQUAL(out->fields.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->fields.value, 3);
	TEST_ASSERT_EQUAL(out->one.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->one.value, 5);
	TEST_ASSERT_EQUAL(out->two.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->two.value, 6);
	TEST_ASSERT_EQUAL(out->contents.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->contents.value, 4);
	TEST_ASSERT_EQUAL(out->three.null, NOT_NULL_VALUE);
	TEST_ASSERT_EQUAL(out->three.value, 1234);
}