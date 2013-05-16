#include <windows.h>

int done;
int thread_finished;
int thread_working;

void Shutdown( void )
{
   printf( "signal leaving.\n" );
	done = 1;
	while( !thread_finished )
		Sleep( 0 );
	printf( "thread finished\n" );
}

#ifdef __WATCOMC__
static void *ThreadWrapper( void* pThread )
#else
static size_t __cdecl ThreadWrapper( void* pThread )
#endif
{
   printf( "thread working...\n" );
   thread_working = 1;
	while( !done )
		Sleep( 0 );
   printf( "thread leaving...\n" );
   thread_finished = 1;
}

int main( void )
{
	HANDLE hThread;
	DWORD dwJunk;
	atexit( Shutdown );
	hThread = CreateThread( NULL
								 , 1024
								 , (LPTHREAD_START_ROUTINE)(ThreadWrapper)
								 , NULL
								 , 0
								 , &dwJunk );
   printf( "wait for thread start...\n" );
	while( !thread_working )
      Sleep( 0 );
   printf( "thread started\n" );
	return 0;
}

