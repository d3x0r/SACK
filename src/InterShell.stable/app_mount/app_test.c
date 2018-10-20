


#include <stdhdrs.h>
#include <deadstart.h>
#include <network.h>
#include "../intershell_registry.h"
#include "../intershell_export.h"

typedef struct MyControl *PMY_CONTROL;
typedef struct MyControl MY_CONTROL;
struct MyControl
{
	struct {
		BIT_FIELD bShown : 1;
		BIT_FIELD bWantShow : 1;
	} flags;
	CTEXTSTR app_window_name;
   CTEXTSTR app_class_name;
};

static struct {
   PTHREAD waiting;
	PLIST controls;
	PCLIENT udp_socket;
   SOCKADDR *sendto;
} local_application_mount;
#define l local_application_mount

static void CPROC read_complete( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *sa )
{
	if( !buffer )
	{
      buffer = Allocate( 4096 );
	}
	else
	{
	}
   ReadUDP( pc, buffer, 4096 );
}

void SendHide( void )
{
   SendUDPEx( l.udp_socket, "<hide/>", 7, l.sendto );
}

void SendShow( void )
{
   SendUDPEx( l.udp_socket, "<show/>", 7, l.sendto );
}

void SendPosition( int x, int y, int w, int h )
{
	char tmp[128];
	int len;
#undef snprintf
#ifdef _MSC_VER
#define snprintf _snprintf
#endif

   len = snprintf( tmp, sizeof( tmp ), "<move x=\"%d\" y=\"%d\" width=\"%d\" Height=\"%d\"/>", x, y, w, h );
   SendUDPEx( l.udp_socket, tmp, len, l.sendto );
}

PRELOAD( InitAppMount )
{
   NetworkStart();
	l.udp_socket = ServeUDP( WIDE("127.0.0.1:2996"), 2996, read_complete, NULL );
	UDPEnableBroadcast( l.udp_socket, TRUE );
	l.sendto = CreateSockAddress( WIDE("127.0.0.1:2997"), 2997 );
}


int main( int argc, char **argv )
{
#undef atoi
	if( argc < 2 )
	{
		printf( WIDE("%s {S|H|M} \n" )
				WIDE( " S - show\n" )
				WIDE( " H - hide\n" )
             WIDE(" M - requires 4 additional parameters < x y width height >\n" ), argv[0] );
      return 0;
	}
	else if( argv[1][0] == 'S' || argv[1][0] == 's' )
      SendShow();
	else if( argv[1][0] == 'H' || argv[1][0] == 'h' )
      SendHide();
	else if( argv[1][0] == 'M' || argv[1][0] == 'm' )
		SendPosition( atoi( argv[2] ), atoi( argv[3] ), atoi( argv[4] ), atoi( argv[5] ) );
   WakeableSleep( 250 );
   return 0;
}
