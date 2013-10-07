#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <idle.h>
#include <controls.h>
#include <network.h>
#include "vlcint.h"

static struct {
	PCLIENT pc;
	PRENDERER transparent;
	_32 last_message_tick;
	LOGICAL bHidden;
	LOGICAL bPendingChange;
	S_32 x, y;
	_32 w, h;
   PTHREAD change_thread;
} l;

PTRSZVAL CPROC DoUpdateThread( PTHREAD thread )
{
	while( 1 )
	{
		if( l.bPendingChange )
		{
         lprintf( "woke to find a change pending. %d %d %d %d", l.x, l.y, l.w, l.h );
			if( l.w == 0 && l.h == 0 )
			{
				if( !l.bHidden )
				{
					lprintf( "hiding display." );
					l.bHidden = 1;
					HideDisplay( l.transparent );
				}
			}
			else
			{
            lprintf( "update thread triggered." );
				HoldUpdates();
				//HideDisplay( l.transparent );
				//while( !IsDisplayHidden( l.transparent ) )
				//{
				//   lprintf( "Waiting for display to hide..." );
				//	Relinquish();
				//}
				MoveSizeDisplay( l.transparent, l.x, l.y, l.w, l.h  );
				if( l.bHidden )
				{
               l.bHidden = 0;
					RestoreDisplay( l.transparent );
				}
				//RestoreDisplay( l.transparent );
				ReleaseUpdates();
			}
         l.bPendingChange = 0;
		}
		else
         WakeableSleep( SLEEP_FOREVER );

	}
}

void CPROC NetworkUpdate( PCLIENT pc, POINTER buffer, int size, SOCKADDR *saFrom )
{
	if( !buffer )
	{
      lprintf( "alloyc buffer..." );
      buffer = Allocate( 1024 );
	}
	else
	{
		_32 *array = (_32*)buffer;
      //lprintf( "Received data message." );
		if( array[0] != l.last_message_tick )
		{
         LogBinary( array, 4*5 );
         //lprintf( "Received unique message..." );
			{
				l.x = array[1];
				l.y = array[2];
				l.w = array[3];
				l.h = array[4];
				l.bPendingChange = 1;
            lprintf( "Just posting change event." );
            WakeThread( l.change_thread );
			}
			l.last_message_tick = array[0];
		}
	}
   //lprintf( "do read." );
   ReadUDP( pc, buffer, 1024 );
}

int main( int argc, char ** argv )
{
   char *file_to_play;
	_32 w, h;
	PLIST names = NULL;
	if( argc < 2 )
		return 0;

   l.change_thread = ThreadTo( DoUpdateThread, 0 );
   NetworkStart();
   l.pc = ServeUDP( NULL, 2999, NetworkUpdate, NULL );

	SetSystemLog( SYSLOG_FILE, stderr );
	GetDisplaySize( &w, &h );

	{
		int n;
		for( n = 1; n < argc; n++ )
		{
			AddLink( &names, argv[n] );
		}
	}
	{
		l.transparent = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED|DISPLAY_ATTRIBUTE_NO_MOUSE, 100, 100, 0, 0 );
      DisableMouseOnIdle( l.transparent, TRUE );
		UpdateDisplay( l.transparent );
		MakeTopmost( l.transparent );
		{
			static TEXTCHAR buf[4096];
			int n;
         int ofs = 0;
			for(n = 2; n < argc; n++ )
			{
				if( strchr( argv[n], ' ' ) )
					ofs += snprintf( buf, sizeof( buf ) - ofs, "\"%s\" ", argv[n] );
				else
					ofs += snprintf( buf, sizeof( buf ) - ofs, "%s ", argv[n] );
			}
			PlayItemOnEx( l.transparent, argv[1], NULL /*buf*/ );
		}
		while( 1 )
		{
			//int n =
			IdleFor( 250 );
			//lprintf( "Idle result: %d", n );
			if( !DisplayIsValid( l.transparent ) )
            break;
		}
	}
   return 0;
}
