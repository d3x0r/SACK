#include <stdhdrs.h>

#define USE_RENDER_INTERFACE proxy_client_local.pri
#define USE_IMAGE_INTERFACE proxy_client_local.pii
#define USE_RENDER3D_INTERFACE proxy_client_local.pr3i
#define USE_IMAGE_3D_INTERFACE proxy_client_local.pi3i
#include <render.h>
#include <render3d.h>

#include "client_local.h"


static void CPROC SocketRead( PCLIENT pc, POINTER buffer, size_t size )
{
	struct client_socket_state *state;
	if( !buffer )
	{
		lprintf( "init read" );
		state = New(struct client_socket_state);
		MemSet( state, 0, sizeof( struct client_socket_state ) );
		state->flags.get_length = 1;
		state->buffer = NewArray( _8, 16024 );
		SetNetworkLong( pc, 0, (PTRSZVAL)state );
	}
	else
	{
		state = (struct client_socket_state*)GetNetworkLong( pc, 0 );
		if( state->flags.get_length )
		{
			lprintf( "length is %d", state->read_length );
			if( state->read_length < 16024 )
			{
				state->flags.get_length = 0;
			}
			else
			{

			}
		}
		else
		{
			PRENDERER r;
			struct common_message *msg = (struct common_message*)state->buffer;
			state->flags.get_length = 1;
			lprintf( "handle message %d", msg->message_id );
			switch( msg->message_id )
			{
			case PMID_Version:  // 0
				break;
			case PMID_SetApplicationTitle:   //1
				SetApplicationTitle( msg->data.text );
				break;
			case PMID_SetApplicationIcon:  // 2 (unused)
				break;
			case PMID_OpenDisplayAboveUnderSizedAt:  // 3
				SetLink( &proxy_client_local.renderers, msg->data.opendisplay_data.server_display_id
				    , r = OpenDisplayAboveUnderSizedAt( msg->data.opendisplay_data.attr
				                                    , msg->data.opendisplay_data.w
				                                    , msg->data.opendisplay_data.h
				                                    , msg->data.opendisplay_data.x
				                                    , msg->data.opendisplay_data.y
				                                    , (PRENDERER)GetLink( &proxy_client_local.renderers, msg->data.opendisplay_data.over )
				                                    , (PRENDERER)GetLink( &proxy_client_local.renderers, msg->data.opendisplay_data.under )
				                                  ) ); 
				RestoreDisplay( r );
				break;
			case PMID_CloseDisplay:  // 4
				{
					PRENDERER r = (PRENDERER)GetLink( &proxy_client_local.renderers, msg->data.close_display.server_display_id );
					if( r )
						CloseDisplay( r );
				}
				break;
			case PMID_MakeImage: // 6
				{
					if( msg->data.make_image.server_display_id != INVALID_INDEX )
					{
						SetLink( &proxy_client_local.images, msg->data.make_image.server_image_id
						        , GetDisplayImage( (PRENDERER)GetLink( &proxy_client_local.renderers, msg->data.make_image.server_display_id ) )
						       );
					}
					else
						SetLink( &proxy_client_local.images, msg->data.make_image.server_image_id
						    , MakeImageFile( msg->data.make_image.w, msg->data.make_image.h )
						    );
					break;
				}
				break;
			case PMID_MakeSubImage: // 7
				SetLink( &proxy_client_local.images, msg->data.make_subimage.server_image_id
				    , MakeSubImage( (Image)GetLink( &proxy_client_local.images, msg->data.make_subimage.server_parent_image_id )
				                    , msg->data.make_subimage.x
				                    , msg->data.make_subimage.y
				                    , msg->data.make_subimage.w
				                    , msg->data.make_subimage.h
				                    ) 
				        );
				UpdateDisplay( (PRENDERER)GetLink( &proxy_client_local.renderers, 0 ) );
				break;
			case PMID_BlatColor: // 8
				BlatColor( (Image)GetLink( &proxy_client_local.images, msg->data.blatcolor.server_image_id )
				        , msg->data.blatcolor.x
				        , msg->data.blatcolor.y
				        , msg->data.blatcolor.w
				        , msg->data.blatcolor.h 
				        , msg->data.blatcolor.color
				        );
				break;
			case PMID_BlatColorAlpha: // 9 
				BlatColorAlpha( (Image)GetLink( &proxy_client_local.images, msg->data.blatcolor.server_image_id )
				        , msg->data.blatcolor.x
				        , msg->data.blatcolor.y
				        , msg->data.blatcolor.w
				        , msg->data.blatcolor.h 
				        , msg->data.blatcolor.color
				        );
				break;
			case PMID_ImageData: // 10 - transfer local image data to client
				{
					Image tmp;
					tmp = GetLink( &proxy_client_local.images, msg->data.image_data.server_image_id );
					if( tmp )
						UnmakeImageFile( tmp );
					// reloadimage sorta
					// could include the math; the message header is 1 byte (subtract from size)
					// the image-data_data contains one byte, and that needs to (not subtract from size) 
					// so thats +0 to sizeof image_data_data
					SetLink( &proxy_client_local.images, msg->data.image_data.server_image_id
							, DecodeMemoryToImage( msg->data.image_data.data, size - ( sizeof( struct image_data_data ) ) )
						   );
				}
				break;
			case PMID_BlotImageSizedTo:  // 11 
				BlotImageSizedEx( (Image)GetLink( &proxy_client_local.images, msg->data.blot_image.server_image_id )
					, (Image)GetLink( &proxy_client_local.images, msg->data.blot_image.image_id )
				        , msg->data.blot_image.x
				        , msg->data.blot_image.y
						, msg->data.blot_image.xs
				        , msg->data.blot_image.ys
				        , msg->data.blot_image.w
				        , msg->data.blot_image.h 
						, TRUE
						, BLOT_COPY
				        );
				break;
			case PMID_BlotScaledImageSizedTo: // 12
				BlotScaledImageSizedEx( (Image)GetLink( &proxy_client_local.images, msg->data.blot_scaled_image.server_image_id )
					, (Image)GetLink( &proxy_client_local.images, msg->data.blot_scaled_image.image_id )
				        , msg->data.blot_scaled_image.x
				        , msg->data.blot_scaled_image.y
				        , msg->data.blot_scaled_image.w
				        , msg->data.blot_scaled_image.h 
						, msg->data.blot_scaled_image.xs
				        , msg->data.blot_scaled_image.ys
				        , msg->data.blot_scaled_image.ws
				        , msg->data.blot_scaled_image.hs 
						, TRUE
						, BLOT_COPY
				        );
				break;
			case PMID_DrawLine: // 13
				do_lineAlpha( (Image)GetLink( &proxy_client_local.images, msg->data.line.server_image_id )
					, msg->data.line.x1
					, msg->data.line.y1
					, msg->data.line.x2
					, msg->data.line.y2
					, msg->data.line.color );
				break;
			case PMID_UnmakeImage: // 14
				UnmakeImageFile( (Image)GetLink( &proxy_client_local.images, msg->data.unmake_image.server_image_id ) );
				SetLink( &proxy_client_local.images, msg->data.unmake_image.server_image_id, 0 );
				break;
			//case PMID_Event_Mouse // 15 (from client to server)
			//case PMID_Event_Key // 16 (from client to server)
			}
		}
	}
	if( state->flags.get_length )
	{
		lprintf( "Read next length..." );
		ReadTCPMsg( pc, &state->read_length, 4 );
	}
	else
	{
		lprintf( "read message %d", state->read_length );
		ReadTCP( pc, state->buffer, state->read_length );
	}
}

static void CPROC SocketClose( PCLIENT pc )
{
	proxy_client_local.service = NULL;
	WakeThread( proxy_client_local.main );
}

SaneWinMain( argc, argv )
{
	NetworkStart();
	proxy_client_local.service = OpenTCPClientExx( argv[1]?argv[1]:WIDE("127.0.0.1"), 4241, SocketRead, SocketClose, NULL, NULL );
	while( proxy_client_local.service )
	{
		proxy_client_local.pri = GetDisplayInterface();
		proxy_client_local.pii = GetImageInterface();
		proxy_client_local.pr3i = GetRender3dInterface();
		proxy_client_local.pi3i = GetImage3dInterface();

		proxy_client_local.main = MakeThread();
		WakeableSleep( 10000 );
	}
	return 0;
}
EndSaneWinMain()

