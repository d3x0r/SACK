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
				SetLink( &l.renderers, msg->data.opendisplay_data.server_display_id
				    , r = OpenDisplayAboveUnderSizedAt( msg->data.opendisplay_data.attr
				                                    , msg->data.opendisplay_data.w
				                                    , msg->data.opendisplay_data.h
				                                    , msg->data.opendisplay_data.x
				                                    , msg->data.opendisplay_data.y
				                                    , (PRENDERER)GetLink( &l.renderers, msg->data.opendisplay_data.over )
				                                    , (PRENDERER)GetLink( &l.renderers, msg->data.opendisplay_data.under )
				                                  ) ); 
				RestoreDisplay( r );
				break;
			case PMID_CloseDisplay:  // 4
				{
					PRENDERER r = (PRENDERER)GetLink( &l.renderers, msg->data.close_display.server_display_id );
					if( r )
						CloseDisplay( r );
				}
				break;
			case PMID_MakeImage: // 6
				{
					if( msg->data.make_image.server_display_id != INVALID_INDEX )
					{
						struct client_proxy_image *image = New( struct client_proxy_image );
                  image->image = NULL;
                  image->render_id = msg->data.make_image.server_display_id;
						SetLink( &l.images, msg->data.make_image.server_image_id
						        , image
						       );
					}
					else
					{
						struct client_proxy_image *image = New( struct client_proxy_image );
                  image->image = MakeImageFile( msg->data.make_image.w, msg->data.make_image.h );
                  image->render_id = INVALID_INDEX;
						SetLink( &l.images, msg->data.make_image.server_image_id
								 , image
								 );
					}
					break;
				}
				break;
			case PMID_MakeSubImage: // 7
				{
					struct client_proxy_image *image = New( struct client_proxy_image );
					struct client_proxy_image *parent_image = GetLink( &l.images, msg->data.make_subimage.server_parent_image_id );
					image->image = MakeSubImage( parent_image->image
														, msg->data.make_subimage.x
														, msg->data.make_subimage.y
														, msg->data.make_subimage.w
														, msg->data.make_subimage.h
														);
					image->render_id = INVALID_INDEX;
				}
				break;
			case PMID_BlatColor: // 8
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blatcolor.server_image_id );
					Image real_image = image->image?image->image:GetDisplayImage( GetLink( &l.renderers, image->render_id ) );
					BlatColor( real_image
								, msg->data.blatcolor.x
								, msg->data.blatcolor.y
								, msg->data.blatcolor.w
								, msg->data.blatcolor.h
								, msg->data.blatcolor.color
								);
				}
				break;
			case PMID_BlatColorAlpha: // 9 
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blatcolor.server_image_id );
					Image real_image = image->image?image->image:GetDisplayImage( GetLink( &l.renderers, image->render_id ) );
					BlatColorAlpha( (Image)GetLink( &l.images, msg->data.blatcolor.server_image_id )
									  , msg->data.blatcolor.x
									  , msg->data.blatcolor.y
									  , msg->data.blatcolor.w
									  , msg->data.blatcolor.h
									  , msg->data.blatcolor.color
									  );
				}
				break;
			case PMID_ImageData: // 10 - transfer local image data to client
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blatcolor.server_image_id );
					Image tmp;
					if( image->image )
						UnmakeImageFile( image->image );
					// reloadimage sorta
					// could include the math; the message header is 1 byte (subtract from size)
					// the image-data_data contains one byte, and that needs to (not subtract from size) 
					// so thats +0 to sizeof image_data_data
					image->image = DecodeMemoryToImage( msg->data.image_data.data, size - ( sizeof( struct image_data_data ) ) );
				}
				break;
			case PMID_BlotImageSizedTo:  // 11
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blot_image.server_image_id );
					Image real_image = image->image?image->image:GetDisplayImage( GetLink( &l.renderers, image->render_id ) );
					lprintf( "Ouptut 5o %p",  GetLink( &l.images, msg->data.blot_image.server_image_id ) );
					lprintf( "Might output to %p", GetDisplayImage( GetLink( &l.renderers, 0 ) ) );
					BlotImageSizedEx( (Image)GetLink( &l.images, msg->data.blot_image.server_image_id )
										 , real_image
										 , msg->data.blot_image.x
										 , msg->data.blot_image.y
										 , msg->data.blot_image.xs
										 , msg->data.blot_image.ys
										 , msg->data.blot_image.w
										 , msg->data.blot_image.h
										 , TRUE
										 , BLOT_COPY
										 );
				}
				break;
			case PMID_BlotScaledImageSizedTo: // 12
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blot_scaled_image.server_image_id );
					Image real_image = image->image?image->image:GetDisplayImage( GetLink( &l.renderers, image->render_id ) );
					BlotScaledImageSizedEx( (Image)GetLink( &l.images, msg->data.blot_scaled_image.server_image_id )
												 , real_image
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
				}
				break;
			case PMID_DrawLine: // 13
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blatcolor.server_image_id );
					Image real_image = image->image?image->image:GetDisplayImage( GetLink( &l.renderers, image->render_id ) );
					do_lineAlpha( real_image
									, msg->data.line.x1
									, msg->data.line.y1
									, msg->data.line.x2
									, msg->data.line.y2
									, msg->data.line.color );
				}
				break;
			case PMID_UnmakeImage: // 14
				{
					struct client_proxy_image *image = GetLink( &l.images, msg->data.blatcolor.server_image_id );
					Image real_image = image->image?image->image:GetDisplayImage( GetLink( &l.renderers, image->render_id ) );
					UnmakeImageFile( real_image );
					SetLink( &l.images, msg->data.unmake_image.server_image_id, 0 );
					Release( image );
				}
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

