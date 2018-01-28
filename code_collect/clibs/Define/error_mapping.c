#include "error_mapping.h"

void foo()
{
	return error_strings[EINVAL];
}
