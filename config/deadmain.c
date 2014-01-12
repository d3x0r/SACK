

#include <sack_types.h>
#include "deadstart.h"
PRELOAD(test1)
{
	printf("blah\n");
}

int main( void )
{
#define paste(a,b) a##b
#define paste2(a,b) paste(a,b)
#define DeclareList(n) paste2(n,TARGET_LABEL)
#ifndef __WATCOMC__
	extern struct rt_init DeclareList( begin_deadstart_ );
	extern struct rt_init DeclareList( end_deadstart_ );
	struct rt_init *begin = &DeclareList( begin_deadstart_ );
	struct rt_init *end = &DeclareList( end_deadstart_ );
	struct rt_init *current;
	printf( "rt_init size=%ld\n", sizeof( struct rt_init ) );
	printf( "begin=%p\n", begin );
	printf( "end=%p\n", end );
	printf( "extra=%ld\n", ((PTRSZVAL)end - (PTRSZVAL)begin)%sizeof( struct rt_init ) );
	for( current = begin; current < end; current++ )
	{
		printf( "Entry %p(%d) = %s %s(%d)\n"
				, current, (current-begin)
				, current->file, current->funcname
				, current->line
				);
	}
#endif
}
