#include <stdhdrs.h>

#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER3D_INTERFACE l.pr3i
#define USE_IMAGE_3D_INTERFACE l.pi3i
#include <render.h>
#include <render3d.h>

#include "client_local.h"


static void CPROC SocketRead( PCLIENT pc, POINTER buffer, size_t size )
{
	struct client_socket_state *state;
	if( !buffer )
	{
		state = New(struct client_socket_state);
		MemSet( state, 0, sizeof( struct client_socket_state ) );
		state->flags.get_length = 1;
		state->buffer = NewArray( _8, 1024 );
		SetNetworkLong( pc, 0, (PTRSZVAL)state );
	}
	else
	{
		state = (struct client_socket_state*)GetNetworkLong( pc, 0 );
		if( state->flags.get_length )
		{
			if( state->read_length < 255 )
			{
				state->flags.get_length = 0;
			}
			else
			{

			}
		}
		else
		{
			struct common_message *msg = (struct common_message*)state->buffer;
			state->flags.get_length = 1;
			switch( msg->message_id )
			{
			case PMID_SetApplicationTitle:
            SetApplicationTitle( msg->data.text );
            break;
			}
		}
	}
	if( state->flags.get_length )
		ReadTCPMsg( pc, &state->read_length, 4 );
	else
		ReadTCP( pc, state->buffer, state->read_length );
}

static void CPROC SocketClose( PCLIENT pc )
{
	l.service = NULL;
	WakeThread( l.main );
}

SaneWinMain( argc, argv )
{
	NetworkStart();
	l.service = OpenTCPClientExx( argv[1]?argv[1]:WIDE("127.0.0.1"), 4241, SocketRead, SocketClose, NULL, NULL );
	while( l.service )
	{
		l.pri = GetDisplayInterface();
		l.pii = GetImageInterface();
		l.pr3i = GetRender3dInterface();
      l.pi3i = GetImage3dInterface();

      l.main = MakeThread();
      WakeableSleep( 10000 );
	}
   return 0;
}
EndSaneWinMain()

