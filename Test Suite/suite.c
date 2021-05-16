#include "unity_fixture.h"

static void RunAllTests(void)
{
	RUN_TEST_GROUP(StreamingParser);
}


int suite(int argc, const char* argv[])
{
	return UnityMain(argc, argv, RunAllTests);
}