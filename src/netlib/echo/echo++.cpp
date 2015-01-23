#ifdef __cplusplus
#include <stdhdrs.h>
#include <conio.h>
#include <network.h>

#define BUFFERSIZE 4096
	
	// hmm network library isn't REALLY cpp like
	// since the callbacks still have no proc reference
	// and - therefore cannot be passed as class methods...
	typedef class echo_server:public NETWORK {
		void ReadComplete( POINTER buf, int size );
	public:
		echo_server()  {}
		~echo_server() {}
	} ECHO_SERVER;

void ECHO_SERVER::ReadComplete( POINTER buf, int size )
{
    if( !buf )
		 buf = new _8[BUFFERSIZE];
    else
       Write( buf, size );
    Read( buf, BUFFERSIZE );
}

int main( char argc, char **argv )
{
    int port;
    if( argc < 2 )
    {
		printf( WIDE("usage: %s <listen port> (defaulting to telnet)\n"), argv[0] );
		port=23;
    }
    else 
   	port = atoi(argv[1]);

   ECHO_SERVER server;	

	 if( server.Listen( port ) )
	 {
		 //while(!kbhit()) Sleep(100);
       while( 1 ) Sleep( 1000 );
  	 }	
    else
		printf( WIDE("Failed to listen on port %s\n"), argv[1] );
    return 0;
}

#else
#include <stdio.h>
int main( void )
{
	return printf( WIDE("Compiled without a C++ compiler, program cannot function.") );
}
#endif
