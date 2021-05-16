#include <stdio.h>

#define os_zalloc(s) calloc(s, 1)

#include "unity.h"
#include "unity_memory.h"
#include "unity_fixture.h"
#include "streaming-parser.h"

extern int suite(int argc, const char* argv[]);

int main(int argc, const char* argv[]) {
	return suite(argc, argv);
}