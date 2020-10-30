#include "args.h"
#include <stdbool.h>

int SHIM_PUBLIC
shim_argtype (char const * str) {
	int num_hyphens = 0;
	while( *str++ == '-' )
		++num_hyphens;
	switch( num_hyphens ) {
		case 1:
			return SHIM_ARGTYPE_SHORT;
		case 2:
			return SHIM_ARGTYPE_LONG;
		default:
			return SHIM_ARGTYPE_NONE;
	}
}

void SHIM_PUBLIC
shim_process_args (int const argc, char ** argv,
		   Shim_Arg_Processor_t * const processor,
		   void * SHIM_RESTRICT state)
{
	for( int i = 1; i < argc; ++i ) {
		char ** p = argv + i;
		if( *p ) {
			Shim_Arg_Parser_t * parser = processor( *p, state );
			if( parser ) {
				Shim_Arg_Handler_t * handler = parser( *p );
				if( handler )
					handler( p, argc - i, state );
			}
		}
	}
}