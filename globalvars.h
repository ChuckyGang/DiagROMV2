#pragma once
#include <stdint.h>
typedef struct GlobalVars
{
	void*		stack_mem;
	uint32_t 	stack_size;
	uint32_t	current_vhpos;
} GlobalVars;
