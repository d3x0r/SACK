
#define DEFINE_DEFAULT_RENDER_INTERFACE
#define USE_IMAGE_INTERFACE GetImageInterface()
#include <stdhdrs.h>
#include <stdlib.h>
#include <psi.h>
#include <network.h>
#include <timers.h>
#include <sqlgetoption.h>
//#include "../intershell/vlc_hook/vlcint.h"


#ifdef __LINUX__
#define INVALID_HANDLE_VALUE -1
#endif

static struct {
	PCLIENT pc;
	PTASK_INFO task;
   SOCKADDR *sendto;
} l;

//--------------------------------------------------------------------

typedef struct MyControl *PMY_CONTROL;
typedef struct MyControl MY_CONTROL;
struct MyControl
{
	PRENDERER surface;
	struct {
		BIT_FIELD bShown : 1;
	} flags;
};

void SendUnhide( void )
{
	uint32_t buffer[2];
	buffer[0] = GetTickCount();
   buffer[1] = 6;
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
   WakeableSleep( 10 );
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
   WakeableSleep( 10 );
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

void SendUpdate( uint32_t x, uint32_t y, uint32_t w, uint32_t h )
{
	uint32_t buffer[7];
	buffer[0] = GetTickCount();
   buffer[1] = 0;
	buffer[2] = x;
	buffer[3] = y;
	buffer[4] = w;
	buffer[5] = h;
	lprintf( WIDE("update %d %d %d %d"), x, y, w, h );

   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
   WakeableSleep( 10 );
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
   WakeableSleep( 10 );
   SendUDPEx( l.pc, buffer, sizeof( buffer ), l.sendto );
}

int main( int argc, char **argv )
{
   int x, y, w, h;
	NetworkStart();

	l.pc = ConnectUDP( NULL, 2997, WIDE("127.0.0.1"), 5151, NULL, NULL );
   if( !l.pc )
		l.pc = ConnectUDP( NULL, 2996, WIDE("127.0.0.1"), 5151, NULL, NULL );
   UDPEnableBroadcast( l.pc, TRUE );
	l.sendto = CreateSockAddress( WIDE("127.0.0.1"), 2999 );

	x = atoi( DupCharToText( argv[1] ) );
	y = atoi( DupCharToText( argv[2] ) );
	w = atoi( DupCharToText( argv[3] ) );
	h = atoi( DupCharToText( argv[4] ) );
	{
		int j;
		for( j = 0; j < 200; j++ )
		{
			SendUpdate( x, y, w+((rand()*600)/RAND_MAX), h+((rand()*600)/RAND_MAX) );
		}
	}
   return 0;
}


