#include "print.h"

void
shim_print_byte_buffer (uint8_t * SHIM_RESTRICT bytes,
			size_t const 		num_bytes)
{
	SHIM_ASSERT(bytes);
	if (!num_bytes)
		return;
	uint8_t const *alias = bytes;
	size_t const backtick_one_index = num_bytes - 1;
#define FORMAT_STR_ "%02hhx"
	for (size_t i = 0; i < backtick_one_index; ++i)
		printf(FORMAT_STR_, alias[i]);
	printf(FORMAT_STR_, alias[backtick_one_index]);
} // ~ shim_print_byte_buffer (uint8_t*, size_t const)
