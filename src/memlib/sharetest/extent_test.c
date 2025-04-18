#include <stdio.h>
#include <sharemem.h>
#include <logging.h>


int main( int argc, char **argv )
{
	char *p[640];
	int n;
	SetSystemLog( SYSLOG_FILE, stdout );
	SetHeapUnit( 4096 );
	for( n = 0; n < 640; n++ )
	{
		Log2( "%d To allocate %d", n, 4097 );
		p[n] = Allocate( 4097 );
	}
	{
		uint32_t free, alloc, block, freeblock;
		GetMemStats( &free, &alloc, &block, &freeblock );
		printf( "Stats: %d free %d allocated %d blocks and %d freeblocks\n", free, alloc, block, freeblock );
	}
	for( n = 0; n < 640; n++ )
	{
		Log1( "%d To Release", n );
		Release( p[n] );
	}
	{
		uint32_t free, alloc, block, freeblock;
		GetMemStats( &free, &alloc, &block, &freeblock );
		printf( "Stats: %d free %d allocated %d blocks and %d freeblocks\n", free, alloc, block, freeblock );
	}

	for( n = 0; n < 64; n++ )
	{
		Log1( "To allocate %d", n*3737 );
		p[n] = Allocate( n * 3737 );
	}
	{
		uint32_t free, alloc, block, freeblock;
		GetMemStats( &free, &alloc, &block, &freeblock );
		printf( "Stats: %d free %d allocated %d blocks and %d freeblocks\n", free, alloc, block, freeblock );
	}
	for( n = 0; n < 64; n++ )
		Release( p[n] );
	{
		uint32_t free, alloc, block, freeblock;
		GetMemStats( &free, &alloc, &block, &freeblock );
		printf( "Stats: %d free %d allocated %d blocks and %d freeblocks\n", free, alloc, block, freeblock );
	}
	for( n = 0; n < 64; n++ )
	{
		Log1( "To allocate %d", n*3737 );
		p[n] = Allocate( n * 3737 );
	}
	{
		uint32_t free, alloc, block, freeblock;
		GetMemStats( &free, &alloc, &block, &freeblock );
		printf( "Stats: %d free %d allocated %d blocks and %d freeblocks\n", free, alloc, block, freeblock );
	}
	for( n = 0; n < 64; n++ )
		Release( p[n] );
	{
		uint32_t free, alloc, block, freeblock;
		GetMemStats( &free, &alloc, &block, &freeblock );
		printf( "Stats: %d free %d allocated %d blocks and %d freeblocks\n", free, alloc, block, freeblock );
	}

}

// $Log: extent_test.c,v $
// Revision 1.3  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
