#include <windows.h>
#include <malloc.h>
#include <stdio.h>

#define BUFFER 450000

int done;
char *buf1;
char *buf2;

void alloc1( void )
{
   buf1 = (char *)malloc( BUFFER );
	while( !done )
	{
		int n;
		for( n = 0; n < BUFFER; n++ )
		{
			buf1[n] = n;
		}
	}
}

void alloc2( void )
{
   buf2 = (char *)malloc( BUFFER );
	while( !done )
	{
		int n;
		for( n = 0; n < BUFFER; n++ )
		{
			buf2[n] = n;
		}
	}
}

void test_buffs( void )
{
   printf( "start.\n" );
	while( done < 5 )
	{
		int n;
      //printf( "Begin tick.\n" );
		for( n = 0; n < BUFFER; n++ )
		{
			if( buf1[n] == buf2[n] )
            continue;
		}
      //printf( "tick.\n" );
	}
}


void ThreadTo( void(*proc)(void) )
{
	DWORD dwJunk;
	HANDLE hThread = CreateThread( NULL, 1024
										  , (LPTHREAD_START_ROUTINE)(proc)
										  , 0
										  , 0
										  , &dwJunk );
}


void doatexit( void )
{
   printf( "Done.\n" );
	//done++;
   _asm int 3;
}

int main( void )
{
	atexit( doatexit );
	ThreadTo( alloc1 );
	ThreadTo( alloc2 );
	while( !buf1 || !buf2 )
      Sleep( 0 );
	ThreadTo( test_buffs );
   done++;
   Sleep( 20 );
   done++;
   Sleep( 0 );
   done++;
   Sleep( 0 );
   done++;



   return 0;
}

