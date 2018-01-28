#include <stdio.h>
#include "static_assert.h"

void foo()
{
	char buf[1024];
	STATIC_NULL_ASSERT(sizeof(buf) == 1024);
}
