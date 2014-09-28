#include <stdhdrs.h>

#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER3D_INTERFACE l.pr3i
#define USE_IMAGE_3D_INTERFACE l.pi3i
#include <render.h>
#include <render3d.h>

#include "client_local.h"

static void HandleMessage( PCLIENT pc, struct common_message*msg, size_t size );

static void ReplayMessages( struct client_proxy_image *image )
{
	P_8 start = image->buffer;
	size_t offset = 0;
	while( offset < image->sendlen )
	{
		_32 length = ((_32*)(start + offset))[0];
		HandleMessage( NULL, (struct common_message*)(start + offset + 4), length );
		offset += length;
	}
}

static void CPROC RedrawEvent( PTRSZVAL psv, PRENDERER r )
{
	size_t sendlen;
	struct common_message *outmsg;
	// often used; sometimes unused...
	_8 *msg;
	struct client_proxy_render *render = (struct client_proxy_render *)psv;

	{
		struct client_proxy_image *image;
		INDEX idx;
		LIST_FORALL( render->images, idx, struct client_proxy_image *, image )
		{
			ReplayMessages( image );
		}
	}

	if( !render->flags.opening && 0 )
	{
		msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct close_display_data ) ) );
		((_32*)msg)[0] = (_32)(sendlen - 4);
		outmsg = (struct common_message*)(msg + 4);
		outmsg->message_id = PMID_Event_Redraw;
		outmsg->data.close_display.server_display_id = render->id;
		SendTCP( render->pc, msg, sendlen );
		Release( msg );
	}
	UpdateDisplay( render->render );
}

static PTRSZVAL CPROC EventThread( PTHREAD thread )
{
	while( 1 )
	{
		struct event_msg *msg;
		WakeableSleep( 10000000 );
		while( msg = (struct event_msg*)DequeLink( &l.events ) )
		{
			SendTCP( msg->pc, &msg->sendlen, msg->sendlen+4 );
			Release( msg );
		}
	}
	return 0;
}

static int CPROC MouseEvent( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	size_t sendlen;
	struct event_msg *event = (struct event_msg*)NewArray( _8, sizeof( PCLIENT ) + ( sendlen = ( 4 + 1 + sizeof( struct mouse_event_data ) ) ) );
	struct common_message *outmsg = &event->msg;
	struct client_proxy_render *render = (struct client_proxy_render *)psv;
	event->pc = render->pc;
	event->sendlen = (_32)(sendlen - 4);
	outmsg->message_id = PMID_Event_Mouse;
	outmsg->data.mouse_event.server_render_id = render->id;
	outmsg->data.mouse_event.x = x;
	outmsg->data.mouse_event.y = y;
	outmsg->data.mouse_event.b = b;
	SendTCP( render->pc, &event->sendlen, event->sendlen + 4 );
	//EnqueLink( &l.events, event );
	//WakeThread( l.event_thread );
	Release( event );
	return 1;
}

static int CPROC KeyEvent( PTRSZVAL psv, _32 key )
{
	size_t sendlen;
	struct event_msg *event = (struct event_msg*)NewArray( _8, sizeof( PCLIENT ) + ( sendlen = ( 4 + 1 + sizeof( struct key_event_data ) ) ) );
	struct common_message *outmsg = &event->msg;
	struct client_proxy_render *render = (struct client_proxy_render *)psv;
	event->pc = render->pc;
	event->sendlen = (_32)(sendlen - 4);
	outmsg->message_id = PMID_Event_Key;
	outmsg->data.mouse_event.server_render_id = render->id;
	outmsg->data.key_event.key = KEY_CODE( key );
	outmsg->data.key_event.pressed = IsKeyPressed( key )?1:0;
	SendTCP( render->pc, &event->sendlen, event->sendlen + 4 );
	//EnqueLink( &l.events, event );
	//WakeThread( l.event_thread );
	Release( event );
	return 1;
}

static P_8 GetMessageBuf( struct client_proxy_image * image, POINTER buffer, size_t size )
{
	P_8 resultbuf;
	if( ( image->buf_avail ) <= ( 4 + size + image->sendlen ) )
	{
		P_8 newbuf;
		image->buf_avail += size + 256;
		newbuf = NewArray( _8, image->buf_avail );
		if( image->buffer )
		{
			MemCpy( newbuf, image->buffer, image->sendlen );
			Release( image->buffer );
		}
		image->buffer = newbuf;
	}
	resultbuf = image->buffer + image->sendlen;
	((_32*)resultbuf)[0] = size + 4;
	MemCpy( ((_32*)resultbuf) + 1, buffer, size );
	image->sendlen += size + 4;
	return resultbuf;
}


static LOGICAL PrestoreMessage( struct common_message *msg, size_t size )
{
	switch( msg->message_id )
	{
		case PMID_BlatColor: // 8
		case PMID_BlatColorAlpha: // 9 
			{
				struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.unmake_image.server_image_id );
				if(  image->image && 
					msg->data.blatcolor.w == image->image->width && 
					msg->data.blatcolor.h == image->image->height )
				{
					//lprintf( "Clear exisitng data on %p", image );
					image->sendlen = 0;
				}
			}
		case PMID_BlotImageSizedTo:  // 11
		case PMID_BlotScaledImageSizedTo: // 12
		case PMID_DrawLine: // 13
			{
				struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.unmake_image.server_image_id );
				//lprintf( "Store message on %d", msg->data.unmake_image.server_image_id );
				if( image )
				{
					GetMessageBuf( image, msg, size );
				}
			}
			return TRUE;
	}
	return FALSE;
}

// HandleMessage will be called to replay network events also
// in that case pc will be NULL.
// In this routine; pc is used when opening the renderer for events to go back to the same
// server...
void HandleMessage( PCLIENT pc, struct common_message*msg, size_t size )
{
	lprintf( "handle message %d on %d", msg->message_id, msg->data.unmake_image.server_image_id  );
	if( !pc || !PrestoreMessage( msg, size ) )
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
		{
			struct client_proxy_render *render = New( struct client_proxy_render );
			render->flags.opening = 1;
			render->images = NULL; // init PLIST
			render->id = msg->data.opendisplay_data.server_display_id;
			render->pc = pc;
			render->render = OpenDisplayAboveUnderSizedAt( msg->data.opendisplay_data.attr
					                            , msg->data.opendisplay_data.w
					                            , msg->data.opendisplay_data.h
					                            , msg->data.opendisplay_data.x
					                            , msg->data.opendisplay_data.y
					                            , (PRENDERER)GetLink( &l.renderers, msg->data.opendisplay_data.over )
					                            , (PRENDERER)GetLink( &l.renderers, msg->data.opendisplay_data.under )
					                            ); 
			SetMouseHandler( render->render, MouseEvent, (PTRSZVAL)render );
			SetKeyboardHandler( render->render, KeyEvent, (PTRSZVAL)render );
			SetRedrawHandler( render->render, RedrawEvent, (PTRSZVAL)render );
			SetLink( &l.renderers, msg->data.opendisplay_data.server_display_id, render );

			RestoreDisplay( render->render );
			render->flags.opening = 0;
					
		}
		break;
	case PMID_MoveSizeDisplay: // 18
		{
			struct client_proxy_render* r = (struct client_proxy_render*)GetLink( &l.renderers, msg->data.close_display.server_display_id );
			if( r )
				MoveSizeDisplay( r->render, msg->data.move_size_display.x
								, msg->data.move_size_display.y
								, msg->data.move_size_display.w
								, msg->data.move_size_display.h );
		}
		break;
	case PMID_CloseDisplay:  // 4
		{
			struct client_proxy_render* r = (struct client_proxy_render*)GetLink( &l.renderers, msg->data.close_display.server_display_id );
			if( r )
				CloseDisplay( r->render );
		}
		break;
	case PMID_MakeImage: // 6
		{
			if( msg->data.make_image.server_display_id != INVALID_INDEX )
			{
				struct client_proxy_image *image = New( struct client_proxy_image );
				MemSet( image, 0, sizeof( struct client_proxy_image ) );
				image->render_id = msg->data.make_image.server_display_id;
				{
					struct client_proxy_render *render = (struct client_proxy_render *)GetLink( &l.renderers, image->render_id );
					if( render )
						AddLink( &render->images, image );
				}
				SetLink( &l.images, msg->data.make_image.server_image_id
						, image
						);
			}
			else
			{
				struct client_proxy_image *image = New( struct client_proxy_image );
				MemSet( image, 0, sizeof( struct client_proxy_image ) );
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
			struct client_proxy_image *parent_image = (struct client_proxy_image *)GetLink( &l.images, msg->data.make_subimage.server_parent_image_id );
			INDEX render_id;
			Image real_image;
			if( !parent_image )
				break;
			MemSet( image, 0, sizeof( struct client_proxy_image ) );
			if( !parent_image->image )
			{
				struct client_proxy_render *render = (struct client_proxy_render*)GetLink( &l.renderers, render_id = parent_image->render_id );
				real_image = GetDisplayImage( render->render );
				AddLink( &render->images, image );
			}
			else
			{
				if( ( render_id = parent_image->render_id ) != INVALID_INDEX )
				{
					struct client_proxy_render *render = (struct client_proxy_render*)GetLink( &l.renderers, render_id = parent_image->render_id );
					AddLink( &render->images, image );
				}
				real_image = parent_image->image;
			}
			//lprintf( "sub is %d %p", msg->data.make_subimage.server_parent_image_id, real_image );
			image->image = MakeSubImage( real_image
										, msg->data.make_subimage.x
										, msg->data.make_subimage.y
										, msg->data.make_subimage.w
										, msg->data.make_subimage.h
											);
			image->render_id = render_id;
			SetLink( &l.images, msg->data.make_subimage.server_image_id, image );
		}
		break;
	case PMID_BlatColor: // 8
		{
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			Image real_image = image->image?image->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image->render_id ))->render );
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
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			Image real_image = image->image?image->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image->render_id ))->render );
			//lprintf( "blat color to %p", image );
			BlatColorAlpha( real_image
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
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			if( image->image )
				UnmakeImageFile( image->image );
			if( image->in_buflen )
			{
            lprintf( "already had some data collected... %d %d", image->in_buflen, image->in_buf_avail );
				if( ( image->in_buflen + size ) > image->in_buf_avail )
				{
					_8 *newbuf = NewArray( _8, image->in_buflen + size );
					MemCpy( newbuf, image->in_buffer, image->in_buflen );
					MemCpy( newbuf + image->in_buflen, msg->data.image_data.data, size - ( sizeof( struct image_data_data ) ) );
					image->in_buf_avail = image->in_buflen + size;
					Release( image->in_buffer );
					image->in_buffer = newbuf;
				}
				else
				{
               lprintf( "buffer is already big enough to hold this..." );
				}
            image->in_buflen += size - ( sizeof( struct image_data_data ) );
				image->image = DecodeMemoryToImage( image->in_buffer, image->in_buflen );
			}
			else
			{
				// reloadimage sorta
				// could include the math; the message header is 1 byte (subtract from size)
				// the image-data_data contains one byte, and that needs to (not subtract from size)
				// so thats +0 to sizeof image_data_data
				image->image = DecodeMemoryToImage( msg->data.image_data.data, size - ( sizeof( struct image_data_data ) ) );
			}
		}
		break;
	case PMID_ImageDataFrag: // 23 - transfer local image data to client
		{
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			if( image->image )
			{
				UnmakeImageFile( image->image );
				image->image = NULL;
			}
         image->in_buflen = 0;
			if( ( image->in_buflen + size ) > image->in_buf_avail )
			{
				_8 *newbuf = NewArray( _8, image->in_buflen + size );
				MemCpy( newbuf, image->in_buffer, image->in_buflen );
				MemCpy( newbuf + image->in_buflen, msg->data.image_data.data, size - ( sizeof( struct image_data_data ) ) );
            image->in_buf_avail = image->in_buflen + size;
            Release( image->in_buffer );
            image->in_buffer = newbuf;
			}
         image->in_buflen += size - ( sizeof( struct image_data_data ) );
		}
		break;
	case PMID_ImageDataFragMore: // 23 - transfer local image data to client
		{
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			if( ( image->in_buflen + size ) > image->in_buf_avail )
			{
				_8 *newbuf = NewArray( _8, image->in_buflen + size );
				MemCpy( newbuf, image->in_buffer, image->in_buflen );
				MemCpy( newbuf + image->in_buflen, msg->data.image_data.data, size - ( sizeof( struct image_data_data ) ) );
            image->in_buf_avail = image->in_buflen + size;
            Release( image->in_buffer );
            image->in_buffer = newbuf;
			}
         image->in_buflen += size - ( sizeof( struct image_data_data ) );
		}
		break;
	case PMID_BlotImageSizedTo:  // 11
		{
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blot_image.server_image_id );
			struct client_proxy_image *image_source = (struct client_proxy_image *)GetLink( &l.images, msg->data.blot_image.image_id );
			Image real_image = image->image?image->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image->render_id ))->render );
			Image real_image_source = image_source->image?image_source->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image_source->render_id ))->render );
			//lprintf( "output image %p to %p  %d,%d  %d,%d", image_source, image
			//						, msg->data.blot_image.x
			//						, msg->data.blot_image.y
			//						, msg->data.blot_image.w
			//						, msg->data.blot_image.h );
			BlotImageSizedEx( real_image
									, real_image_source
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
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blot_scaled_image.server_image_id );
			struct client_proxy_image *image_source = (struct client_proxy_image *)GetLink( &l.images, msg->data.blot_scaled_image.image_id );
			Image real_image_source = image_source->image?image_source->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image_source->render_id ))->render );
			Image real_image = image->image?image->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image->render_id ))->render );
			BlotScaledImageSizedEx( real_image
											, real_image_source
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
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			Image real_image = image->image?image->image:GetDisplayImage( ((struct client_proxy_render*) GetLink( &l.renderers, image->render_id ))->render );
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
			struct client_proxy_image *image = (struct client_proxy_image *)GetLink( &l.images, msg->data.blatcolor.server_image_id );
			Image real_image = image->image;
			if( real_image )
				UnmakeImageFile( real_image );
			SetLink( &l.images, msg->data.unmake_image.server_image_id, 0 );
			Release( image );
		}
		break;
	//case PMID_Event_Mouse // 15 (from client to server)
	//case PMID_Event_Key // 16 (from client to server)
	case PMID_Flush_Draw: // 17 (server has finished sending draw commands)
		{
			struct client_proxy_render * r = (struct client_proxy_render *)GetLink( &l.renderers, msg->data.close_display.server_display_id );
			if( r )
				UpdateDisplay( r->render );
		}
		break;
	}
}


static void CPROC SocketRead( PCLIENT pc, POINTER buffer, size_t size )
{
	struct client_socket_state *state;
	if( !buffer )
	{
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
			//lprintf( "length is %d", state->read_length );
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
			struct common_message *msg = (struct common_message*)state->buffer;
			state->flags.get_length = 1;
			//lprintf( "handle message %d", msg->message_id );
			HandleMessage( pc, msg, size );
		}
	}
	if( state->flags.get_length )
	{
		ReadTCPMsg( pc, &state->read_length, 4 );
	}
	else
	{
		//lprintf( "read message %d byte(s)", state->read_length );
		ReadTCPMsg( pc, state->buffer, state->read_length );
	}
}

static void CPROC SocketClose( PCLIENT pc )
{
	l.service = NULL;
	WakeThread( l.main );
}

SaneWinMain( argc, argv )
{
#ifdef UNICODE
	TEXTCHAR *addr = argv[1]?DupCStr( argv[1] ):WIDE("127.0.0.1");
#else
   char *addr = argv[1]?argv[1]:WIDE("127.0.0.1");
#endif
	NetworkStart();
	l.event_thread = ThreadTo( EventThread, 0 );
	l.service = OpenTCPClientExx( addr, 4241, SocketRead, SocketClose, NULL, NULL );
	while( l.service )
	{
		l.pri = GetDisplayInterface();
		l.pii = GetImageInterface();
		//l.pr3i = GetRender3dInterface();
		//l.pi3i = GetImage3dInterface();

		l.main = MakeThread();
		WakeableSleep( 10000 );
	}
	return 0;
}
EndSaneWinMain()

