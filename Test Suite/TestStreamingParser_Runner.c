#include "unity.h"
#include "unity_fixture.h"

TEST_GROUP_RUNNER(StreamingParser)
{
	
	RUN_TEST_CASE(StreamingParser, UnitTest_ResizeDynamicArray);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseNestedStruct);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolTrue);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolFalse);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseNull);

	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolTrueInner);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolFalseInner);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseNullInner);
	

	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolTrueInner_Typo);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolFalseInner_Typo);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseNullInner_Typo);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolTrueInner_Overflow);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolFalseInner_Overflow);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseNullInner_Overflow);



	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolTrueInner_SplitEdgeCase);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolFalseInner_SplitEdgeCase);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseNullInner_SplitEdgeCase);

	RUN_TEST_CASE(StreamingParser, UnitTest_ParseBoolFalse_NullFallThroughEdgeCase);
	
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseArray);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseStringArray);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseTokenArray);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseStruct);
	RUN_TEST_CASE(StreamingParser, UnitTest_ParseStructHandlerSwitch);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableSint31AsNull);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableSint31AsFloat);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableSint31AsInt);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableDoubleAsNull);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableDoubleAsFloat);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableDoubleAsInt);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableDoubleAsNegativeFloat);
	RUN_TEST_CASE(StreamingParser, UnitTest_ExtractNullableDoubleAsNegativeInt);


	RUN_TEST_CASE(StreamingParser, UnitTest_BufferEnd);
	
}

