#define NEED_REAL_IMAGE_STRUCTURE
#include <imglib/imagestruct.h>

#include <render.h>
#include <render3d.h>
#include <image3d.h>
#include <sqlgetoption.h>
#include <html5.websocket.h>
#include <json_emitter.h>

#include <zlib.h>

#include "instance_server_local.h"

IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif
extern LOGICAL PngImageFile ( Image pImage, _8 ** buf, size_t *size);
#ifdef __cplusplus
};
using namespace sack::image::loader;
#endif
IMAGE_NAMESPACE_END


static IMAGE_INTERFACE InstanceProxyImageInterface;


static void FormatColor( PVARTEXT pvt, CPOINTER data )
{
	vtprintf( pvt, WIDE("\"rgba(%u,%u,%u,%g)\"")
		, ((*(PCDATA)data) >> 16) & 0xFF
		, ((*(PCDATA)data) >> 8) & 0xFF
		, ((*(PCDATA)data) >> 0) & 0xFF 
		, (((*(PCDATA)data) >> 24) & 0xFF)/255.0
		);
}

static struct json_context_object *WebSockInitReplyJson( enum proxy_message_id message )
{
	struct json_context_object *cto;
	struct json_context_object *cto_data;
	int ofs = 4;  // first thing is length, but that is not encoded..
	if( !l.json_reply_context )
		l.json_reply_context = json_create_context();
	cto = json_create_object( l.json_reply_context, 0 );
	SetLink( &l.messages, (int)message, cto );
	json_add_object_member( cto, WIDE("MsgID"), 0, JSON_Element_Unsigned_Integer_8, 0 );
	cto_data = json_add_object_member( cto, WIDE( "data" ), 1, JSON_Element_Object, 0 );

	ofs = 0;
	switch( message )
	{
	case PMID_ClientSessionId :
		json_add_object_member( cto_data, WIDE("client_id"), ofs = 0, JSON_Element_String, 0 );
		break;
	case PMID_Reply_OpenDisplayAboveUnderSizedAt:
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("client_render_id"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_Event_Mouse:
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("b"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		break;
	case PMID_Event_Key:
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("key"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("pressed"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		break;
	}
	return cto;
}

static struct json_context_object *WebSockInitJson( enum proxy_message_id message )
{
	struct json_context_object *cto;
	struct json_context_object *cto_data;
	int ofs = 4;  // first thing is length, but that is not encoded..
	if( !l.json_context )
		l.json_context = json_create_context();
	cto = json_create_object( l.json_context, 0 );
	SetLink( &l.messages, (int)message, cto );
	json_add_object_member( cto, WIDE("MsgID"), 0, JSON_Element_Unsigned_Integer_8, 0 );
	cto_data = json_add_object_member( cto, WIDE( "data" ), 1, JSON_Element_Object, 0 );

	ofs = 0;
	switch( message )
	{
	case PMID_Version:
		json_add_object_member( cto_data, WIDE("version"), 0, JSON_Element_Unsigned_Integer_32, 0 );
		break;
	case PMID_SetApplicationTitle:
		json_add_object_member( cto_data, WIDE("title"), 0, JSON_Element_CharArray, 0 );
		break;
	case PMID_TransferSubImages:
		json_add_object_member( cto_data, WIDE("image_to_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("image_from_id"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_OpenDisplayAboveUnderSizedAt:
		json_add_object_member( cto_data, WIDE("x"), ofs = 0, JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("attrib"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = ofs + sizeof(_32), JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("over_render_id"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL_BLANK_0, 0 );
		json_add_object_member( cto_data, WIDE("under_render_id"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL_BLANK_0, 0 );
		break;
	case PMID_CloseDisplay:
		json_add_object_member( cto_data, WIDE("server_render_id"), 0, JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_Flush_Draw:
		json_add_object_member( cto_data, WIDE("server_render_id"), 0, JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_MoveSizeDisplay:
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		break;
	case PMID_MakeImage:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof( PTRSZVAL), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("server_render_id"), ofs = ofs + sizeof(_32), JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_Move_Image:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x"), ofs = ofs + sizeof( PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(_32), JSON_Element_Integer_32, 0 );
		break;
	case PMID_Size_Image:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof( PTRSZVAL), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		break;
	case PMID_MakeSubImage:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("server_parent_image_id"), ofs = ofs + sizeof(_32), JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_BlatColor:
	case PMID_BlatColorAlpha:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member_user_routine( cto_data, WIDE("color"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0, FormatColor );
		break;
	case PMID_BlotScaledImageSizedTo:
	case PMID_BlotImageSizedTo:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("width"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("height"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("xs"), ofs = ofs + sizeof(_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("ys"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		if( message == PMID_BlotScaledImageSizedTo )
		{
			json_add_object_member( cto_data, WIDE("ws"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0 );
			json_add_object_member( cto_data, WIDE("hs"), ofs = ofs + sizeof(_32), JSON_Element_Unsigned_Integer_32, 0 );
		}
		json_add_object_member( cto_data, WIDE("image_id"), ofs = ofs + sizeof(_32), JSON_Element_PTRSZVAL, 0 );
		break;
	case PMID_ImageData:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("data"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_CharArray, 0 );
		break;
	case PMID_DrawBlock:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs = 0, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("length"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("data"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_CharArray, 0 );
		break;
	case PMID_DrawLine:
		json_add_object_member( cto_data, WIDE("server_image_id"), ofs, JSON_Element_PTRSZVAL, 0 );
		json_add_object_member( cto_data, WIDE("x1"), ofs = ofs + sizeof(PTRSZVAL), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y1"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("x2"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member( cto_data, WIDE("y2"), ofs = ofs + sizeof(S_32), JSON_Element_Integer_32, 0 );
		json_add_object_member_user_routine( cto_data, WIDE("color"), ofs = ofs + sizeof(S_32), JSON_Element_Unsigned_Integer_32, 0, FormatColor );
		break;
	}
	return cto;
}

static const char *base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static void encodeblock( unsigned char in[3], TEXTCHAR out[4], size_t len )
{
	out[0] = base64[ in[0] >> 2 ];
	out[1] = base64[ ((in[0] & 0x03) << 4) | ( ( len > 0 ) ? ((in[1] & 0xf0) >> 4) : 0 ) ];
	out[2] = (len > 1 ? base64[ ((in[1] & 0x0f) << 2) | ( ( len > 2 ) ? ((in[2] & 0xc0) >> 6) : 0 ) ] : base64[64]);
	out[3] = (len > 2 ? base64[ in[2] & 0x3f ] : base64[64]);
}

static char *Encode64Image( CTEXTSTR mime, P_8 buf, LOGICAL bmp, size_t length, size_t *outsize )
{
	TEXTCHAR * real_output;
	int mimelen = StrLen( mime );
	real_output = NewArray( TEXTCHAR, 13 + mimelen + ( ( length * 4 + 2) / 3 ) + 1 + 1 + 1 + (bmp?1:0) );
	snprintf( real_output, 13 + mimelen, WIDE("data:%s;base64,"), mime );
	//if( bmp )
	//	strcpy( real_output, "data:image/bmp;base64," );
	//else
	//	strcpy( real_output, "data:image/png;base64," );
	{
		size_t n;
		for( n = 0; n < (length+2)/3; n++ )
		{
			size_t blocklen;
			blocklen = length - n*3;
			if( blocklen > 3 )
				blocklen = 3;
			encodeblock( ((P_8)buf) + n * 3, real_output + 13 + mimelen + n*4, blocklen );
		}
		(*outsize) = 13 + mimelen + n*4 + 1;
		real_output[13 + mimelen + n*4] = 0;
	}
	return real_output;
}

static P_8 EncodeImage( int ID, Image image, LOGICAL bmp, size_t *outsize )
{
	if( !bmp )
	{
		if( image )
		{
			_8 *buf;
			if( PngImageFile( image, &buf, outsize ) )
			{
				if( 1 )
				{
					TEXTCHAR tmpname[32];
					static int n;
					FILE *out;
					snprintf( tmpname, 32, WIDE("blah%d.png"), ID );
					out = sack_fopen( 0, tmpname, WIDE("wb") );
					fwrite( buf, 1, *outsize, out );
					fclose( out );
				}
				return buf;
			}
		}
		(*outsize) = 0;
		return NULL;
	}
#if defined( WIN32 ) && !defined( MINGW_SUX )
	else
	{
		// code to generate raw bitmap; 32 bit bitmaps still don't have alpha channel in browsers; weak.
		BITMAPFILEHEADER *header;
		BITMAPV5HEADER *output;
		size_t length;
		header = (BITMAPFILEHEADER*)NewArray( _8, length = ( ( image->width * image->height * sizeof( CDATA ) ) + sizeof( BITMAPV5HEADER ) + sizeof( BITMAPFILEHEADER ) ) );
		MemSet( header, 0, sizeof( BITMAPFILEHEADER ) + sizeof( BITMAPV5HEADER ) );
		header->bfType = 'MB';
		header->bfSize = (DWORD)length;
		header->bfOffBits = sizeof( BITMAPV5HEADER ) + sizeof( BITMAPFILEHEADER );
		output = (BITMAPV5HEADER*)(header + 1);

		output->bV5Size = sizeof( BITMAPV5HEADER );
		output->bV5Width = image->width;
		output->bV5Height = image->height;
		output->bV5Planes = 1;
		output->bV5BitCount = 32;
		output->bV5XPelsPerMeter = 120;
		output->bV5YPelsPerMeter = 120;
		//output->bV5Intent = LCS_CALIBRATED_RGB;	// 0
		output->bV5CSType = LCS_sRGB;
		{
			PCDATA color_out = (PCDATA)(output + 1);
			int n;
			for( n = 0; n < image->height; n++ )
				MemCpy( color_out + image->width * n, image->image + image->pwidth * n, sizeof( CDATA ) * image->width );
		}

		if( 0 )
		{
			TEXTCHAR tmpname[32];
			static int n;
			FILE *out;
			snprintf( tmpname, 32, WIDE("blah%d.bmp"), n++ );
			out = sack_fopen( 0, tmpname, WIDE("wb") );
			fwrite( header, 1, length, out );
			fclose( out );
		}

		return (P_8)header;
	}
#endif
	 return NULL;
}

static void ClearDirtyFlag( PVPImage image )
{
	//lprintf( WIDE("Clear dirty on %p  (%p) %08x"), image, (image)?image->image:NULL, image?image->image->flags:0 );
	for( ; image; image = image->next )
	{
		if( image->image )
		{
			image->image->flags &= ~IF_FLAG_UPDATED;
			//lprintf("Clear dirty on %08x"), (image)?(image)->image->flags:0 );
		}
		if( image->child )
			ClearDirtyFlag( image->child );
	}
}



static void SendTCPMessageV( struct server_socket_state *state, LOGICAL websock, enum proxy_message_id message, ... );
static void SendTCPMessage( struct server_socket_state *state, LOGICAL websock, enum proxy_message_id message, va_list args )
{
	TEXTSTR json_msg;
	struct json_context_object *cto;
	size_t sendlen;
	//struct server_socket_state *state = (struct server_socket_state *)GetNetworkLong( pc, 0 );
	struct common_message *outmsg;
	// often used; sometimes unused...
	PVPRENDER render;
	PVPImage image;
	_8 *msg;
	EnterCriticalSec( &l.message_formatter );
	if( websock )
	{
		cto = (struct json_context_object *)GetLink( &l.messages, message );
		if( !cto )
			cto = WebSockInitJson( message );
	}
	else
		cto = NULL;
	switch( message )
	{
	case PMID_Version:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
			memcpy( msg + 1, l.application_title, sizeof( TEXTCHAR ) * StrLen( l.application_title ) );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			if( websock )
			{
				json_msg = json_build_message( cto, msg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_SetApplicationTitle:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + StrLen( l.application_title ) + 1 ) );
			StrCpy( msg + 1, l.application_title );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			if( websock )
			{
				json_msg = json_build_message( cto, msg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_Flush_Draw:
		{
			render = va_arg( args, PVPRENDER );
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct opendisplay_data ) ) );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			((struct close_display_data*)(msg+5))->server_display_id = render->id;
			if( websock )
			{
				json_msg = json_build_message( cto, msg + 4 );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_OpenDisplayAboveUnderSizedAt:
		{
			render = va_arg( args, PVPRENDER );
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct opendisplay_data ) ) );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			render->flags.open = 1;
			((struct opendisplay_data*)(msg+5))->x = render->x;
			((struct opendisplay_data*)(msg+5))->y = render->y;
			((struct opendisplay_data*)(msg+5))->w = render->w;
			((struct opendisplay_data*)(msg+5))->h = render->h;
			((struct opendisplay_data*)(msg+5))->attr = render->attributes;
			((struct opendisplay_data*)(msg+5))->server_display_id = (PTRSZVAL)FindLink( &state->application_instance->renderers, render );

			if( render->above )
				((struct opendisplay_data*)(msg+5))->over = render->above->id;
			else
				((struct opendisplay_data*)(msg+5))->over = 0;
			if( render->under )
				((struct opendisplay_data*)(msg+5))->under = render->under->id;
			else
				((struct opendisplay_data*)(msg+5))->under = 0;

			if( websock )
			{
				json_msg = json_build_message( cto, msg + 4 );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
				{
					while( json_msg = (TEXTSTR)DequeLink( &state->pending_operations ) )
					{
						WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
					}
				}
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_CloseDisplay:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct close_display_data ) ) );
			render = va_arg( args, PVPRENDER );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			if( ((struct close_display_data*)(msg+5))->server_display_id = render->id )
				if( websock )
				{
					json_msg = json_build_message( cto, msg + 4 );
					WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
					Release( json_msg );
				}
				else
					SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_MoveSizeDisplay:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct move_size_display_data ) ) );
			render = va_arg( args, PVPRENDER );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			msg[4] = message;
			((struct move_size_display_data*)(msg+5))->server_display_id = render->id;
			((struct move_size_display_data*)(msg+5))->x = render->x;
			((struct move_size_display_data*)(msg+5))->y = render->y;
			((struct move_size_display_data*)(msg+5))->w = render->w;
			((struct move_size_display_data*)(msg+5))->h = render->h;
			if( websock )
			{
				json_msg = json_build_message( cto, msg + 4 );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_Move_Image : 
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct make_image_data ) ) );
			image = va_arg( args, PVPImage );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			outmsg = (struct common_message*)(msg + 4);
			outmsg->message_id = message;
			outmsg->data.move_image.x = image->x;
			outmsg->data.move_image.y = image->y;
			outmsg->data.make_image.server_image_id = image->id;

			if( websock )
			{
				json_msg = json_build_message( cto, outmsg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_Size_Image : 
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct make_image_data ) ) );
			image = va_arg( args, PVPImage );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			outmsg = (struct common_message*)(msg + 4);
			outmsg->message_id = message;
			outmsg->data.size_image.w = image->w;
			outmsg->data.size_image.h = image->h;
			outmsg->data.make_image.server_image_id = image->id;

			if( websock )
			{
				json_msg = json_build_message( cto, outmsg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_MakeImage:
		{
			LOGICAL send_data;
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct make_image_data ) ) );
			image = va_arg( args, PVPImage );
			render = va_arg( args, PVPRENDER );
			send_data = va_arg( args, LOGICAL );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			outmsg = (struct common_message*)(msg + 4);
			outmsg->message_id = message;
			outmsg->data.make_image.w = image->w;
			outmsg->data.make_image.h = image->h;
			outmsg->data.make_image.server_image_id = image->id;
			outmsg->data.make_image.server_display_id = image->render_id;

			if( websock )
			{
				json_msg = json_build_message( cto, outmsg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
			if( image->render_id == INVALID_INDEX && send_data )
				SendTCPMessageV( state, websock, PMID_ImageData, image );
		}
		break;
	case PMID_MakeSubImage:
		{
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct make_subimage_data ) ) );
			image = va_arg( args, PVPImage );
			((_32*)msg)[0] = (_32)(sendlen - 4);
			outmsg = (struct common_message*)(msg + 4);
			outmsg->message_id = message;
			outmsg->data.make_subimage.x = image->x;
			outmsg->data.make_subimage.y = image->y;
			outmsg->data.make_subimage.w = image->w;
			outmsg->data.make_subimage.h = image->h;
			outmsg->data.make_subimage.server_image_id = image->id;
			outmsg->data.make_subimage.server_parent_image_id = image->parent?image->parent->id:INVALID_INDEX;
			lprintf( WIDE("Make image %d on %d"), image->id, outmsg->data.make_subimage.server_parent_image_id );
			if( websock )
			{
				json_msg = json_build_message( cto, outmsg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	case PMID_ImageData:
		{
			_32 offset = 0;
			P_8 raw_image;
			char * encoded_image;
			size_t outlen;
			image = va_arg( args, PVPImage );
			if( !image->image )
				break;
			while( image && image->parent ) 
				image = image->parent;
			ClearDirtyFlag( image );
			raw_image = EncodeImage( image->id, image->image, FALSE, &outlen );
			if( outlen > (16000-5) && !websock )
			{
				lprintf( WIDE( "need to send image in parts : %d" ), outlen );
				msg = NewArray( _8, 16004 );
				outmsg = (struct common_message*)(msg + 4);

				while( outlen > (16000-5) )
				{
					MemCpy( outmsg->data.image_data.data, raw_image + offset, 16000 - 5/*header size, msgid,imgid*/ );

					((_32*)msg)[0] = (_32)(16000);
					outmsg = (struct common_message*)(msg + 4);
					outmsg->message_id = offset?PMID_ImageDataFragMore:PMID_ImageDataFrag;
					outmsg->data.image_data.server_image_id = image->id;
					lprintf( WIDE("Send Image %p %d  %d"), image, image->id, sendlen );
					SendTCP( state->pc, msg, 16004 );
					outlen -= (16000 - 5);
					offset += (16000 - 5);
				}
				sendlen = ( 4 + 1 + sizeof( struct image_data_data ) - 1 + outlen );
			}
			else
			{
				msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct image_data_data ) - 1 + outlen ) );
				outmsg = (struct common_message*)(msg + 4);
			}

			if( websock )
			{
				encoded_image = Encode64Image( WIDE("image/png"), raw_image, FALSE, outlen, &outlen );
				msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct image_data_data ) + outlen * sizeof( TEXTCHAR ) ) );
				outmsg = (struct common_message*)(msg + 4);
				MemCpy( outmsg->data.image_data.data, encoded_image, outlen * sizeof( TEXTCHAR ) );
			}
			else
			{
				lprintf( WIDE("outlen = %d"), outlen );
				MemCpy( outmsg->data.image_data.data, raw_image + offset, outlen * sizeof( TEXTCHAR ) );
			}
			((_32*)msg)[0] = (_32)(sendlen - 4);
			outmsg = (struct common_message*)(msg + 4);
			outmsg->message_id = message;
			outmsg->data.image_data.server_image_id = image->id;
			// include nul in copy
			lprintf( WIDE("Send Image %p %d  %d"), image, image->id, sendlen );
			if( websock )
			{
				json_msg = json_build_message( cto, outmsg );
				WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				Release( json_msg );
			}
			else
				SendTCP( state->pc, msg, sendlen );
			Release( msg );
		}
		break;
	}
	LeaveCriticalSec( &l.message_formatter );

}

void SendTCPMessageV( struct server_socket_state *state, LOGICAL websock, enum proxy_message_id message, ... )
{
	va_list args;
	va_start( args, message );
	SendTCPMessage( state, websock, message, args );
}

void InvokeNewDisplay( struct vidlib_proxy_application *app )
{
	void (CPROC *NewDisplayConnected)(struct vidlib_proxy_application *,struct vidlib_proxy_application_local ***);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( WIDE( "/sack/render/remote display" ), WIDE( "connect" ) );
	CTEXTSTR name;
	ThreadNetworkState.flags.during_connect = 1;
	//DumpRegisteredNamesFrom( WIDE( "/sack/render" ) );
	for( name = GetFirstRegisteredName( event_root, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		struct vidlib_proxy_application_instance *inst = New( struct vidlib_proxy_application_instance );
		NewDisplayConnected = GetRegisteredProcedureExx( data,(CTEXTSTR)name,void,WIDE("new_display_connect"),(struct vidlib_proxy_application *,struct vidlib_proxy_application_local***));
		if( NewDisplayConnected )
		{
			inst->ppLocal = NULL;
			NewDisplayConnected( app, &inst->ppLocal );
			if( inst->ppLocal )
				inst->pLocal = (*inst->ppLocal); // save the current local instance....
			AddLink( &app->local_instances, inst );
		}
	}
	ThreadNetworkState.flags.during_connect = 0;
}

static void SendCompressedBuffer( PCLIENT pc, PVPImage image )
{
	if( image->websock_buffer && image->websock_sendlen )
	{
		//lprintf( "Send image %p %p %d", image, image->websock_buffer, image->websock_sendlen );
		image->websock_buffer[image->websock_sendlen] = ']';
#ifdef _UNICODE
		image->websock_buffer[image->websock_sendlen+1] = 0;
		image->websock_buffer[image->websock_sendlen+2] = 0;
		image->websock_buffer[image->websock_sendlen+3] = 0;
#endif						
		{
			_8 *msg;
			struct common_message *outmsg;
			char * encoded_data;
			Bytef *output = NewArray( Bytef, image->websock_sendlen + 1 );
			size_t sendlen;
			size_t outlen;
			uLongf destlen = image->websock_sendlen + 1;
#ifdef _UNICODE
			encoded_data = CStrDup( image->websock_buffer );
			// do not include the NULL...
			compress2( output, &destlen, encoded_data, CStrLen( encoded_data ), Z_BEST_COMPRESSION );
			Deallocate( char *, encoded_data );
#else
			// this is includnig the close ] of the buffer to this state...
			compress2( output, &destlen, image->websock_buffer, image->websock_sendlen + 1, Z_BEST_COMPRESSION );
#endif

			encoded_data = Encode64Image( WIDE("application/zip"), output, TRUE, destlen, &outlen );
			
			lprintf( WIDE("would have only been %d and is %d but really %d")
				, image->websock_sendlen + 1
				, destlen
				, outlen
				);
			//LogBinary( image->websock_buffer, image->websock_sendlen );
			msg = NewArray( _8, sendlen = ( 4 + 1 + sizeof( struct draw_block_data ) + outlen * sizeof( TEXTCHAR ) ) );
			outmsg = (struct common_message*)(msg + 4);
			MemCpy( outmsg->data.draw_block.data, encoded_data, outlen * sizeof( TEXTCHAR ) );
			Release( encoded_data );

			((_32*)msg)[0] = (_32)(sendlen - 4);
			outmsg = (struct common_message*)(msg + 4);
			outmsg->message_id = PMID_DrawBlock;
			outmsg->data.draw_block.server_image_id = image->id;
			outmsg->data.draw_block.length = destlen;
			// include nul in copy
			lprintf( WIDE("Send Image %p %d  %d"), image, image->id, sendlen );
			{
				TEXTSTR json_msg;
				struct json_context_object *cto;
				cto = (struct json_context_object *)GetLink( &l.messages, outmsg->message_id );
				if( !cto )
					cto = WebSockInitJson( outmsg->message_id );
				json_msg = json_build_message( cto, outmsg );
				WebSocketSendText( pc, json_msg, StrLen( json_msg ) );
				Release( output );
				Release( json_msg );
			}
			Release( msg );
		}

		//WebSocketSendText( pcNew, image->websock_buffer, image->websock_sendlen + 1 );
	}
}


static void SendInitialImage( struct server_socket_state*state, LOGICAL websock, PLIST *sent, PVPImage image, LOGICAL send )
{
	if( image->parent )
	{
		if( FindLink( sent, image->parent ) == INVALID_INDEX )
		{
			SendInitialImage(state, websock, sent, image->parent, send );
		}
		if( send )
			SendTCPMessageV( state, websock, PMID_MakeSubImage, image );
	}
	else if( send )
		SendTCPMessageV( state, websock, PMID_MakeImage, image, image->render_id );
	AddLink( sent,  image );
}


static void SendImageBuffers( struct server_socket_state *state, int send_websock, int send_initial )
{
		{
			PLIST sent = NULL;
			INDEX idx;
			PVPImage image;
			// this sorts the images to draw... so parent buffers are flushed before child buffers
			LIST_FORALL( state->application_instance->images, idx, PVPImage, image )
			{
				//lprintf( "Send Image %p(%d)", image, image->id );
				SendInitialImage( state, send_websock, &sent, image, send_initial );
			}
			LIST_FORALL( sent, idx, PVPImage, image )
			{
				if( !send_initial && !( image->image->flags & IF_FLAG_UPDATED ) )
					continue;
				if( !send_initial )
				{
					image->image->flags &= ~IF_FLAG_UPDATED;
				}
				if( send_websock )
					SendCompressedBuffer( state->pc, image );
				else
					if( image->buffer && image->sendlen )
					{
						//lprintf( "Send image %p %p %d", image, image->buffer, image->sendlen );
						SendTCP( state->pc, image->buffer, image->sendlen );
					}
			}
			DeleteList( &sent );
		}
}

static void SendClientMessage( enum proxy_message_id message, ... )
{
	va_list args;
	va_list tmpargs;
	va_start( args, message );
	va_start( tmpargs, message );
	if( message == PMID_Flush_Draw )
	{
		PVPRENDER render = va_arg( tmpargs, PVPRENDER );
		lprintf( WIDE("Start consuming mouse...") );
		render->flags.consume_mouse = 1;
		SendImageBuffers( ThreadNetworkState.app, ThreadNetworkState.app->websock, FALSE );
	}
	SendTCPMessage( ThreadNetworkState.app, ThreadNetworkState.app->websock, message, args );
}

static void CPROC SocketRead( PCLIENT pc, POINTER buffer, size_t size )
{
	struct server_socket_state *state = (struct server_socket_state *)GetNetworkLong( pc, 0 );
	ThreadNetworkState.app = state;
	{
		INDEX idx; 
		struct vidlib_proxy_application_instance *inst;
		LIST_FORALL( state->application_instance->local_instances, idx, struct vidlib_proxy_application_instance *, inst )
		{
			if( inst->ppLocal )
			{
				(*inst->ppLocal) = inst->pLocal;
			}
		}
	}

	if( !buffer )
	{
		lprintf( WIDE("init read") );
		state->flags.get_length = 1;
		state->buffer = NewArray( _8, 2048 );
		//SetNetworkLong( pc, 0, (PTRSZVAL)state );
	}
	else
	{
		state = (struct server_socket_state*)GetNetworkLong( pc, 0 );
		if( state->flags.get_length )
		{
			//lprintf( WIDE("length is %d"), state->read_length );
			if( state->read_length < 2048 )
			{
				state->flags.get_length = 0;
			}
			else
			{
				lprintf( WIDE("Message is too long to read...") );
			}
		}
		else
		{
			struct common_message *msg = (struct common_message*)state->buffer;
			state->flags.get_length = 1;
			switch( msg->message_id )
			{
			case PMID_Event_Redraw:
				{
					PVPRENDER render = (PVPRENDER)GetLink( &state->application_instance->renderers
						                                 , msg->data.mouse_event.server_render_id );
					if( render && render->redraw )
						render->redraw( render->psv_redraw, (PRENDERER)render );
					SendTCPMessageV( state, FALSE, PMID_Flush_Draw, render );
				}
				break;
			case PMID_Event_Flush_Finished:
				{
					PVPRENDER render = (PVPRENDER)GetLink( &state->application_instance->renderers
									, msg->data.flush_event.server_render_id );
					lprintf( WIDE("Received flush finished...") );
					if( render )
					{
						if( render->flags.pending_mouse )
						{
							if( render->mouse_callback )
							{
								render->mouse_callback( render->psv_mouse_callback, render->mouse_x, render->mouse_y, render->_b);
							}
							render->flags.pending_mouse = 0;
						}
						lprintf( WIDE("Done consuming mouse, got back the flush finished.... so all draw commands were read?") );
						render->flags.consume_mouse = 0;
					}
				}
				break;
			case PMID_Event_Mouse:
				{
					int do_mouse = 0;
					PVPRENDER render = (PVPRENDER)GetLink( &state->application_instance->renderers
					                                     , msg->data.mouse_event.server_render_id );
					if( render && !render->flags.did_first_mouse )
					{
						do_mouse = 1;
						render->flags.did_first_mouse = 1;
					}
					else if( render )
					{
						if( render->_b != msg->data.mouse_event.b )
						{
							lprintf( WIDE("mouse button change, must send") );
							do_mouse = 1;
						}
						else if( render->flags.consume_mouse )
						{
							lprintf( WIDE("consuming that event..") );
							render->flags.pending_mouse = TRUE;
							render->mouse_x = msg->data.mouse_event.x;
							render->mouse_y = msg->data.mouse_event.y;
						}
						else
						{
							lprintf( WIDE("not consuming mouse...") );
							do_mouse = 1;
						}
					}
					if( do_mouse && render && render->mouse_callback )
					{
						
						render->mouse_callback( render->psv_mouse_callback
						                      , msg->data.mouse_event.x
						                      , msg->data.mouse_event.y
						                      , msg->data.mouse_event.b );
						render->_b = msg->data.mouse_event.b;
					}
					else
					{
						lprintf( WIDE( "delaying event : %d,%d,%08x" ), render->mouse_x, render->mouse_y, render->_b );
					}
				}
				break;
			case PMID_Event_Key:
				{
					PVPRENDER render = (PVPRENDER)GetLink( &state->application_instance->renderers, msg->data.mouse_event.server_render_id );
					if (msg->data.key_event.pressed)	// test keydown...
					{
						l.key_states[msg->data.key_event.key & 0xFF] |= 0x80;	// set this bit (pressed)
						l.key_states[msg->data.key_event.key & 0xFF] ^= 1;	// toggle this bit...
					}
					else
					{
						l.key_states[msg->data.key_event.key & 0xFF] &= ~0x80;  //(unpressed)
					}

					if( render && render->key_callback )
						render->key_callback( render->psv_key_callback, (msg->data.key_event.pressed?KEY_PRESSED:0)|msg->data.key_event.key );
				}
				break;
			}
		}
	}
	if( state->flags.get_length )
	{
		//lprintf( WIDE("Read next length...") );
		ReadTCPMsg( pc, &state->read_length, 4 );
	}
	else
	{
		//lprintf( WIDE("read message %d"), state->read_length );
		ReadTCPMsg( pc, state->buffer, state->read_length );
	}
}


static void SendInitalWebSockMessages( struct server_socket_state *state )
{

	EnterCriticalSec( &l.message_formatter );

	if( l.application_title )
		SendTCPMessageV( state, TRUE, PMID_SetApplicationTitle );
	{
		INDEX idx;
		PVPRENDER render;
		LIST_FORALL( state->application_instance->renderers, idx, PVPRENDER, render )
		{
			SendTCPMessageV( state, TRUE, PMID_OpenDisplayAboveUnderSizedAt, render );
		}

		{
			PLIST sent = NULL;
			INDEX idx;
			PVPImage image;
			LIST_FORALL( state->application_instance->images, idx, PVPImage, image )
			{
				SendInitialImage( state, TRUE, &sent, image, TRUE );
			}
			LIST_FORALL( sent, idx, PVPImage, image )
			{
				SendCompressedBuffer( state->pc, image );
			}
			DeleteList( &sent );
		}
		LIST_FORALL( state->application_instance->renderers, idx, PVPRENDER, render )
		{
			SendTCPMessageV( state, state->websock, PMID_Flush_Draw, render );
		}
	}
	LeaveCriticalSec( &l.message_formatter );
}


static void CPROC Connected( PCLIENT pcServer, PCLIENT pcNew )
{
	struct server_socket_state *client = New( struct server_socket_state );
	SetNetworkLong( pcNew, 0, (PTRSZVAL)client );
	client->pending_operations = NULL;
	client->pc = pcNew;
	client->websock = FALSE;
	client->application_instance = New( struct vidlib_proxy_application );
	MemSet( client->application_instance, 0, sizeof( struct vidlib_proxy_application ) );

	SetNetworkReadComplete( pcNew, SocketRead );

	AddLink( &l.clients, client );
	//idx = FindLink ( &l.clients, client );

	// wait for client to identify itself before doing anything
	// Schedule a timer for socket deletion inactivity
	// create application

}

static PTRSZVAL WebSockOpen( PCLIENT pc, PTRSZVAL psv )
{
	struct server_socket_state *client = New( struct server_socket_state );
	client->pending_operations = NULL;
	client->pc = pc;
	client->websock = TRUE;
	client->application_instance = NULL;
	AddLink( &l.clients, client );
	return (PTRSZVAL)client;
}

static void WebSockClose( PCLIENT pc, PTRSZVAL psv )
{
}

static void WebSockError( PCLIENT pc, PTRSZVAL psv, int error )
{
}

static void WebSockEvent( PCLIENT pc, PTRSZVAL psv, POINTER buffer, int msglen )
{
	POINTER msg = NULL;
	struct server_socket_state *client= (struct server_socket_state *)psv;
	struct json_context_object *json_object;
#ifdef _UNICODE
	CTEXTSTR buf;
#endif
	ThreadNetworkState.app = client;

#ifdef _UNICODE
	buf = CharWConvertExx( buffer, msglen DBG_SRC );
	lprintf( WIDE("Received:%*.*S"), msglen,msglen,buffer );
	if( json_parse_message( l.json_reply_context, buf, msglen, &json_object, &msg ) )
	//lprintf( WIDE("Received:%*.*") _cstring_f, msglen,msglen,buffer );
#else
	lprintf( WIDE("Received:%*.*s"), msglen,msglen,buffer );
	if( json_parse_message( l.json_reply_context, buffer, msglen, &json_object, &msg ) )
#endif
	{
		struct common_message *message = (struct common_message *)msg;
		if( message->message_id != PMID_ClientSessionId )
		{
			if( !client->application_instance )
			{
				lprintf( WIDE("Fataility; message without app context") );
				RemoveClient( pc );
			}
		}
		else
		{
			if( client->application_instance )
			{
				lprintf( WIDE("Fataility; instance already identified") );
				RemoveClient( pc );
			}
		}
		switch( message->message_id )
		{
		case PMID_ClientSessionId: // get unique client ID... create application instance......
			{
				INDEX idx;
				struct vidlib_proxy_application *app;
				LIST_FORALL( l.applications, idx, struct vidlib_proxy_application *, app )
				{
					if( StrCmp( message->data.client_ident.client_id, app->client_id ) == 0 )
						break;
				}
				if( app )
				{
					client->application_instance = app;
					SendInitalWebSockMessages( client );
				}
				else
				{
					client->application_instance = New( struct vidlib_proxy_application );
					MemSet( client->application_instance, 0, sizeof( struct vidlib_proxy_application ) );
					client->application_instance->client_id = StrDup( message->data.client_ident.client_id );

					InvokeNewDisplay( client->application_instance );
					AddLink( &l.applications, client->application_instance );
				}
			}
			break;
		case PMID_Reply_OpenDisplayAboveUnderSizedAt:  // depricated; server does not keep client IDs
			{
				PVPRENDER render = (PVPRENDER)GetLink( &ThreadNetworkState.app->application_instance->renderers, message->data.open_display_reply.server_display_id );
				SetLink( &render->remote_render_id, FindLink( &l.clients, client ), message->data.open_display_reply.client_display_id );
			}
			break;
		case PMID_Event_Flush_Finished:
			{
				PVPRENDER render = (PVPRENDER)GetLink( &ThreadNetworkState.app->application_instance->renderers
								, message->data.flush_event.server_render_id );
				if( render )
				{
					if( render->flags.pending_mouse )
					{
						if( render->mouse_callback )
						{
							render->mouse_callback( render->psv_mouse_callback, render->mouse_x, render->mouse_y, render->_b);
						}
						render->flags.pending_mouse = 0;
					}
					lprintf( WIDE("got flush done... so allow consuming") );
					render->flags.consume_mouse = 0;
				}
			}
			break;
		case PMID_Event_Mouse:
			{
				int do_mouse = 0;
				PVPRENDER render = (PVPRENDER)GetLink( &ThreadNetworkState.app->application_instance->renderers, message->data.mouse_event.server_render_id );
				if( render && !render->flags.did_first_mouse )
				{
					do_mouse = 1;
					render->flags.did_first_mouse = 1;
				}
				else if( render )
				{
					if( render->_b != message->data.mouse_event.b )
					{
						lprintf( WIDE("forced change ... dispatching event") );
						render->flags.pending_mouse = FALSE;
						do_mouse = 1;
					}
					else if( render->flags.consume_mouse )
					{
						lprintf( WIDE("consuming event") );
						render->flags.pending_mouse = TRUE;
						render->mouse_x = message->data.mouse_event.x;
						render->mouse_y = message->data.mouse_event.y;
					}
					else
					{
						lprintf( WIDE("dispatching event") );
						do_mouse = 1;
					}
				}
				if( do_mouse && render && render->mouse_callback )
				{
					render->mouse_callback( render->psv_mouse_callback, message->data.mouse_event.x, message->data.mouse_event.y, message->data.mouse_event.b );
					render->_b = message->data.mouse_event.b;
				}
			}
			break;
		case PMID_Event_Key:
			{
				int used = 0;
				PVPRENDER render = (PVPRENDER)GetLink( &ThreadNetworkState.app->application_instance->renderers, message->data.key_event.server_render_id );
				if (message->data.key_event.pressed)	// test keydown...
				{
					l.key_states[message->data.key_event.key & 0xFF] |= 0x80;	// set this bit (pressed)
					l.key_states[message->data.key_event.key & 0xFF] ^= 1;	// toggle this bit...
				}
				else
				{
					l.key_states[message->data.key_event.key & 0xFF] &= ~0x80;  //(unpressed)
				}
#if defined( __LINUX__ )
            // windows doesn't need this translation
				lprintf( WIDE("so.... do the state...") );
				SACK_Vidlib_ProcessKeyState( message->data.key_event.pressed, message->data.key_event.key, &used );
#endif
				if( !used && render && render->key_callback )
					render->key_callback( render->psv_key_callback, (message->data.key_event.pressed?KEY_PRESSED:0)|message->data.key_event.key );
			}
			break;
		}
		//lprintf( WIDE("Success") );
		json_dispose_message( json_object,  msg );
	}
	else
		lprintf( WIDE("Failed to reverse map message...") );
}


static void InitService( void )
{
	if( !l.listener )
	{
		NetworkStart();
		l.listener = OpenTCPListenerAddrEx( CreateSockAddress( WIDE("0.0.0.0"), 4241 ), Connected );
		l.web_listener = WebSocketCreate( WIDE("ws://0.0.0.0:4240/Sack/Vidlib/Proxy")
													, WebSockOpen
													, WebSockEvent
													, WebSockClose
													, WebSockError
													, 0 );
	}
}

static int CPROC VidlibProxy_InitDisplay( void )
{
	return TRUE;
}

static void CPROC VidlibProxy_SetApplicationIcon( Image icon )
{
	// no support
}

static LOGICAL CPROC VidlibProxy_RequiresDrawAll( void )
{
	// force application to mostly draw itself...
	return FALSE;
}
static LOGICAL CPROC SACK_Vidlib_RenderIsInstanced( void )
{
	// force application to mostly draw itself...
	return TRUE;
}

static LOGICAL CPROC VidlibProxy_AllowsAnyThreadToUpdate( void )
{
	return FALSE;
}

static void CPROC VidlibProxy_SetApplicationTitle( CTEXTSTR title )
{
	if( l.application_title )
		Release( l.application_title );
	l.application_title = StrDup( title );
	//SendClientMessage( PMID_SetApplicationTitle );
}

static void CPROC VidlibProxy_GetDisplaySize( _32 *width, _32 *height )
{
	if( width )
		(*width) = SACK_GetProfileInt( WIDE("SACK/Vidlib"), WIDE("Default Display Width"), 1024 );
	if( height )
		(*height) = SACK_GetProfileInt( WIDE("SACK/Vidlib"), WIDE("Default Display Height"), 768 );
}

static void CPROC VidlibProxy_SetDisplaySize		( _32 width, _32 height )
{
	SACK_WriteProfileInt( WIDE("SACK/Vidlib"), WIDE("Default Display Width"), width );
	SACK_WriteProfileInt( WIDE("SACK/Vidlib"), WIDE("Default Display Height"), height );
}


static PVPImage Internal_MakeImageFileEx ( INDEX iRender, _32 Width, _32 Height, INDEX client_id DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = Width;
	image->h = Height;
	image->render_id = iRender;
	image->image = l.real_interface->_MakeImageFileEx( Width, Height DBG_RELAY );
	image->image->reverse_interface = &InstanceProxyImageInterface;
	image->image->reverse_interface_instance = image;
	if( iRender != INVALID_INDEX )
		image->image->flags |= IF_FLAG_FINAL_RENDER;
	AddLink( &ThreadNetworkState.app->application_instance->images, image );
	image->id = FindLink( &ThreadNetworkState.app->application_instance->images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make proxy image %p %d(%d,%d)", image, image->id, Width, Height );
	SendClientMessage( PMID_MakeImage, image, iRender, FALSE );
	return image;
}

static PVPImage WrapImageFile( Image native )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->w = native->width;
	image->h = native->height;
	image->render_id = INVALID_INDEX;
	image->image = native;
	image->image->reverse_interface = &InstanceProxyImageInterface;
	image->image->reverse_interface_instance = image;
	AddLink( &ThreadNetworkState.app->application_instance->images, image );
	image->id = FindLink( &ThreadNetworkState.app->application_instance->images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make wrapped proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
	SendClientMessage( PMID_MakeImage, image, INVALID_INDEX, TRUE );
	//SendClientMessage( PMID_ImageData, image );
	return image;
}


static Image CPROC VidlibProxy_MakeImageFileEx (_32 Width, _32 Height DBG_PASS)
{
	return (Image)Internal_MakeImageFileEx( INVALID_INDEX, Width, Height, INVALID_INDEX DBG_RELAY );
}


static PRENDERER CPROC VidlibProxy_OpenDisplayAboveUnderSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under )
{

	PVPRENDER Renderer = New( struct vidlib_proxy_renderer );
	MemSet( Renderer, 0, sizeof( struct vidlib_proxy_renderer ) );
	AddLink( &ThreadNetworkState.app->application_instance->renderers, Renderer );
	Renderer->id = FindLink( &ThreadNetworkState.app->application_instance->renderers, Renderer );
	Renderer->x = x;
	Renderer->y = y;
	Renderer->w = width;
	Renderer->h = height;
	Renderer->attributes = attributes;
	Renderer->above = (PVPRENDER)above;
	Renderer->under = (PVPRENDER)under;
	Renderer->image = Internal_MakeImageFileEx( Renderer->id, width, height, INVALID_INDEX DBG_SRC );
	
	SendClientMessage( PMID_OpenDisplayAboveUnderSizedAt, Renderer );
	return (PRENDERER)Renderer;
}

static PRENDERER CPROC VidlibProxy_OpenDisplayAboveSizedAt( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above )
{
	return VidlibProxy_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, above, NULL );
}

static PRENDERER CPROC VidlibProxy_OpenDisplaySizedAt	  ( _32 attributes, _32 width, _32 height, S_32 x, S_32 y )
{
	return VidlibProxy_OpenDisplayAboveUnderSizedAt( attributes, width, height, x, y, NULL, NULL );
}


/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
	
	Internal
	Interface index 10																	*/  IMAGE_PROC_PTR( Image,VidlibProxy_LoadImageFileEx)  ( CTEXTSTR name DBG_PASS );
static  void CPROC VidlibProxy_UnmakeImageFileEx( Image pif DBG_PASS )
{
	if( pif )
	{
		PVPImage image = (PVPImage)pif;
		SendClientMessage( PMID_UnmakeImage, pif );
		//lprintf( "UNMake proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
		SetLink( &ThreadNetworkState.app->application_instance->images, ((PVPImage)pif)->id, NULL );
		if( ((PVPImage)pif)->image->reverse_interface )
		{
			((PVPImage)pif)->image->reverse_interface = NULL;
			((PVPImage)pif)->image->reverse_interface_instance = 0;
			l.real_interface->_UnmakeImageFileEx( ((PVPImage)pif)->image DBG_RELAY );
		}
		Release( pif );
	}
}

static void CPROC  VidlibProxy_CloseDisplay ( PRENDERER Renderer )
{
	VidlibProxy_UnmakeImageFileEx( (Image)(((PVPRENDER)Renderer)->image) DBG_SRC );
	SendClientMessage( PMID_CloseDisplay, Renderer );
	DeleteLink( &ThreadNetworkState.app->application_instance->renderers, Renderer );
	Release( Renderer );
}

static void CPROC VidlibProxy_UpdateDisplayPortionEx( PRENDERER r, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	// no-op; it will ahve already displayed(?)
	SendClientMessage( PMID_Flush_Draw, r );
}

static void CPROC VidlibProxy_UpdateDisplayEx( PRENDERER r DBG_PASS)
{
	// no-op; it will ahve already displayed(?)
	SendClientMessage( PMID_Flush_Draw, r );

}

static void CPROC VidlibProxy_GetDisplayPosition ( PRENDERER r, S_32 *x, S_32 *y, _32 *width, _32 *height )
{
	PVPRENDER pRender = (PVPRENDER)r;
	if( x )
		(*x) = pRender->x;
	if( y )
		(*y) = pRender->y;
	if( width )
		(*width) = pRender->w;
	if( height ) 
		(*height) = pRender->h;
}

static void CPROC VidlibProxy_MoveSizeDisplay( PRENDERER r
													 , S_32 x, S_32 y
													 , S_32 w, S_32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	pRender->x = x;
	pRender->y = y;
	pRender->w = w;
	pRender->h = h;
	SendClientMessage( PMID_MoveSizeDisplay, r );
}

static void CPROC VidlibProxy_MoveDisplay		  ( PRENDERER r, S_32 x, S_32 y )
{
	PVPRENDER pRender = (PVPRENDER)r;
	VidlibProxy_MoveSizeDisplay( r, 
								x,
								y,
								pRender->w,
								pRender->h
								);
}

static void CPROC VidlibProxy_MoveDisplayRel( PRENDERER r, S_32 delx, S_32 dely )
{
	PVPRENDER pRender = (PVPRENDER)r;
	VidlibProxy_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w,
								pRender->h
								);
}

static void CPROC VidlibProxy_SizeDisplay( PRENDERER r, _32 w, _32 h )
{
	PVPRENDER pRender = (PVPRENDER)r;
	VidlibProxy_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								w,
								h
								);
}

static void CPROC VidlibProxy_SizeDisplayRel( PRENDERER r, S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	VidlibProxy_MoveSizeDisplay( r, 
								pRender->x,
								pRender->y,
								pRender->w + delw,
								pRender->h + delh
								);
}

static void CPROC VidlibProxy_MoveSizeDisplayRel( PRENDERER r
																 , S_32 delx, S_32 dely
																 , S_32 delw, S_32 delh )
{
	PVPRENDER pRender = (PVPRENDER)r;
	VidlibProxy_MoveSizeDisplay( r, 
								pRender->x + delx,
								pRender->y + dely,
								pRender->w + delw,
								pRender->h + delh
								);
}

static void CPROC VidlibProxy_PutDisplayAbove		( PRENDERER r, PRENDERER above )
{
	lprintf( "window ordering is not implemented" );
}

static Image CPROC VidlibProxy_GetDisplayImage( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return (Image)pRender->image;
}

static void CPROC VidlibProxy_SetCloseHandler	 ( PRENDERER r, CloseCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetMouseHandler  ( PRENDERER r, MouseCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->mouse_callback = c;
	render->psv_mouse_callback = p;
}

static void CPROC VidlibProxy_SetRedrawHandler  ( PRENDERER r, RedrawCallback c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->redraw = c;
	render->psv_redraw = p;
}

static void CPROC VidlibProxy_SetKeyboardHandler	( PRENDERER r, KeyProc c, PTRSZVAL p )
{
	PVPRENDER render = (PVPRENDER)r;
	render->key_callback = c;
	render->psv_key_callback = p;
}

static void CPROC VidlibProxy_SetLoseFocusHandler  ( PRENDERER r, LoseFocusCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_GetMousePosition	( S_32 *x, S_32 *y )
{
}

static void CPROC VidlibProxy_SetMousePosition  ( PRENDERER r, S_32 x, S_32 y )
{
}

static LOGICAL CPROC VidlibProxy_HasFocus		 ( PRENDERER  r )
{
	return TRUE;
}

static TEXTCHAR CPROC VidlibProxy_GetKeyText		 ( int key )
{ 
	int c;
	char ch[5];
#ifdef __LINUX__
	{
		int used = 0;
		CTEXTSTR text = SACK_Vidlib_GetKeyText( IsKeyPressed( key ), KEY_CODE( key ), &used );
		if( used && text )
		{
			return text[0];
		}
	}
   return 0;
#else
	if( key & KEY_MOD_DOWN )
		return 0;
	key ^= 0x80000000;
	c =  
#  ifndef UNDER_CE
		ToAscii (key & 0xFF, ((key & 0xFF0000) >> 16) | (key & 0x80000000),
					l.key_states, (unsigned short *) ch, 0);
#  else
		key;
#  endif
	if (!c)
	{
		// check prior key bindings...
		//printf( WIDE("no translation\n") );
		return 0;
	}
	else if (c == 2)
	{
		//printf( WIDE("Key Translated: %d %d\n"), ch[0], ch[1] );
		return 0;
	}
	else if (c < 0)
	{
		//printf( WIDE("Key Translation less than 0\n") );
		return 0;
	}
	//printf( WIDE("Key Translated: %d(%c)\n"), ch[0], ch[0] );
	return ch[0];
#endif
}

static _32 CPROC VidlibProxy_IsKeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static _32 CPROC VidlibProxy_KeyDown		  ( PRENDERER r, int key )
{
	return 0;
}

static LOGICAL CPROC VidlibProxy_DisplayIsValid ( PRENDERER r )
{
	return (r != NULL);
}

static void CPROC VidlibProxy_OwnMouseEx ( PRENDERER r, _32 Own DBG_PASS)
{

}

static int CPROC VidlibProxy_BeginCalibration ( _32 points )
{
	return 0;
}

static void CPROC VidlibProxy_SyncRender( PRENDERER pDisplay )
{
}

static void CPROC VidlibProxy_MakeTopmost  ( PRENDERER r )
{
}

static void CPROC VidlibProxy_HideDisplay	 ( PRENDERER r )
{
}

static void CPROC VidlibProxy_RestoreDisplay  ( PRENDERER r )
{
}

static void CPROC VidlibProxy_ForceDisplayFocus ( PRENDERER r )
{
}

static void CPROC VidlibProxy_ForceDisplayFront( PRENDERER r )
{
}

static void CPROC VidlibProxy_ForceDisplayBack( PRENDERER r )
{
}

static int CPROC  VidlibProxy_BindEventToKey( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv )
{
	return 0;
}

static int CPROC VidlibProxy_UnbindKey( PRENDERER pRenderer, _32 scancode, _32 modifier )
{
	return 0;
}

static int CPROC VidlibProxy_IsTopmost( PRENDERER r )
{
	return 0;
}

static void CPROC VidlibProxy_OkaySyncRender( void )
{
	// redundant thing?
}

static int CPROC VidlibProxy_IsTouchDisplay( void )
{
	return 0;
}

static void CPROC VidlibProxy_GetMouseState( S_32 *x, S_32 *y, _32 *b )
{
}

static PSPRITE_METHOD CPROC VidlibProxy_EnableSpriteMethod(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	return NULL;
}

static void CPROC VidlibProxy_WinShell_AcceptDroppedFiles( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser )
{
}

static void CPROC VidlibProxy_PutDisplayIn(PRENDERER r, PRENDERER hContainer)
{
}

static void CPROC VidlibProxy_SetRendererTitle( PRENDERER render, const TEXTCHAR *title )
{
}

static void CPROC VidlibProxy_DisableMouseOnIdle(PRENDERER r, LOGICAL bEnable )
{
}

static void CPROC VidlibProxy_SetDisplayNoMouse( PRENDERER r, int bNoMouse )
{
}

static void CPROC VidlibProxy_Redraw( PRENDERER r )
{
	PVPRENDER render = (PVPRENDER)r;
	if( render->redraw )
		render->redraw( render->psv_redraw, (PRENDERER)render );
	SendClientMessage( PMID_Flush_Draw, render );
}

static void CPROC VidlibProxy_MakeAbsoluteTopmost(PRENDERER r)
{
}

static void CPROC VidlibProxy_SetDisplayFade( PRENDERER r, int level )
{
}

static LOGICAL CPROC VidlibProxy_IsDisplayHidden( PRENDERER r )
{
	PVPRENDER pRender = (PVPRENDER)r;
	return pRender->flags.hidden;
}

#ifdef WIN32
static HWND CPROC VidlibProxy_GetNativeHandle( PRENDERER r )
{
}
#endif

static void CPROC VidlibProxy_GetDisplaySizeEx( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height)
{
	if( x )
		(*x) = 0;
	if( y )
		(*y) = 0;
	if( width )
		(*width) = 1024;
	if( height )
		(*height) = 768;
}

static void CPROC VidlibProxy_LockRenderer( PRENDERER render )
{
}

static void CPROC VidlibProxy_UnlockRenderer( PRENDERER render )
{
}

static void CPROC VidlibProxy_IssueUpdateLayeredEx( PRENDERER r, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS )
{
}


#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@PTRSZVAL>
		 
		 \ \																									  */
static void CPROC VidlibProxy_SetTouchHandler  ( PRENDERER r, TouchCallback c, PTRSZVAL p )
{
}
#endif

static void CPROC VidlibProxy_MarkDisplayUpdated( PRENDERER r  )
{
}

static void CPROC VidlibProxy_SetHideHandler		( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_SetRestoreHandler  ( PRENDERER r, HideAndRestoreCallback c, PTRSZVAL p )
{
}

static void CPROC VidlibProxy_RestoreDisplayEx ( PRENDERER r DBG_PASS )
{
	((PVPRENDER)r)->redraw( ((PVPRENDER)r)->psv_redraw, r );
}

static LOGICAL CPROC SACK_Vidlib_VidlibRenderAllowsCopy( void )
{
	return FALSE;
}

// android extension
PUBLIC( void, SACK_Vidlib_ShowInputDevice )( void )
{
}

PUBLIC( void, SACK_Vidlib_HideInputDevice )( void )
{
}


static RENDER_INTERFACE ProxyInterface = {
	VidlibProxy_InitDisplay
													  , VidlibProxy_SetApplicationTitle
													  , VidlibProxy_SetApplicationIcon
													  , VidlibProxy_GetDisplaySize
													  , VidlibProxy_SetDisplaySize
													  , VidlibProxy_OpenDisplaySizedAt
													  , VidlibProxy_OpenDisplayAboveSizedAt
													  , VidlibProxy_CloseDisplay
													  , VidlibProxy_UpdateDisplayPortionEx
													  , VidlibProxy_UpdateDisplayEx
													  , VidlibProxy_GetDisplayPosition
													  , VidlibProxy_MoveDisplay
													  , VidlibProxy_MoveDisplayRel
													  , VidlibProxy_SizeDisplay
													  , VidlibProxy_SizeDisplayRel
													  , VidlibProxy_MoveSizeDisplayRel
													  , VidlibProxy_PutDisplayAbove
													  , VidlibProxy_GetDisplayImage
													  , VidlibProxy_SetCloseHandler
													  , VidlibProxy_SetMouseHandler
													  , VidlibProxy_SetRedrawHandler
													  , VidlibProxy_SetKeyboardHandler
	 /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
		 
		 \ \																												 */
													  , VidlibProxy_SetLoseFocusHandler
			 ,  0  //POINTER junk1;
													  , VidlibProxy_GetMousePosition
													  , VidlibProxy_SetMousePosition
													  , VidlibProxy_HasFocus

													  , VidlibProxy_GetKeyText
													  , VidlibProxy_IsKeyDown
													  , VidlibProxy_KeyDown
													  , VidlibProxy_DisplayIsValid
													  , VidlibProxy_OwnMouseEx
													  , VidlibProxy_BeginCalibration
													  , VidlibProxy_SyncRender

													  , VidlibProxy_MoveSizeDisplay
													  , VidlibProxy_MakeTopmost
													  , VidlibProxy_HideDisplay
													  , VidlibProxy_RestoreDisplay
													  , VidlibProxy_ForceDisplayFocus
													  , VidlibProxy_ForceDisplayFront
													  , VidlibProxy_ForceDisplayBack
													  , VidlibProxy_BindEventToKey
													  , VidlibProxy_UnbindKey
													  , VidlibProxy_IsTopmost
													  , VidlibProxy_OkaySyncRender
													  , VidlibProxy_IsTouchDisplay
													  , VidlibProxy_GetMouseState
													  , VidlibProxy_EnableSpriteMethod
													  , VidlibProxy_WinShell_AcceptDroppedFiles
													  , VidlibProxy_PutDisplayIn
													  , NULL // make renderer from native handle (junk4)
													  , VidlibProxy_SetRendererTitle
													  , VidlibProxy_DisableMouseOnIdle
													  , VidlibProxy_OpenDisplayAboveUnderSizedAt
													  , VidlibProxy_SetDisplayNoMouse
													  , VidlibProxy_Redraw
													  , VidlibProxy_MakeAbsoluteTopmost
													  , VidlibProxy_SetDisplayFade
													  , VidlibProxy_IsDisplayHidden
#ifdef WIN32
													, NULL // get native handle from renderer
#endif
													  , VidlibProxy_GetDisplaySizeEx

													  , VidlibProxy_LockRenderer
													  , VidlibProxy_UnlockRenderer
													  , VidlibProxy_IssueUpdateLayeredEx
													  , VidlibProxy_RequiresDrawAll
#ifndef NO_TOUCH
													  , VidlibProxy_SetTouchHandler
#endif
													  , VidlibProxy_MarkDisplayUpdated
													  , VidlibProxy_SetHideHandler
													  , VidlibProxy_SetRestoreHandler
													  , VidlibProxy_RestoreDisplayEx
												, SACK_Vidlib_ShowInputDevice
												, SACK_Vidlib_HideInputDevice
												, VidlibProxy_AllowsAnyThreadToUpdate
												, NULL/* SetFullscreen */
												, NULL /* syspend system sleep */
												 , SACK_Vidlib_RenderIsInstanced
												 , SACK_Vidlib_VidlibRenderAllowsCopy
};

static void InitProxyInterface( void )
{
	// used to have to do this...
	//ProxyInterface._RequiresDrawAll = VidlibProxy_RequiresDrawAll;
}



static RENDER3D_INTERFACE Proxy3dInterface = {
	NULL
};

static void CPROC VidlibProxy_SetStringBehavior( Image pImage, _32 behavior )
{

}
static void CPROC VidlibProxy_SetBlotMethod	  ( _32 method )
{

}

static Image CPROC VidlibProxy_BuildImageFileEx ( PCOLOR pc, _32 width, _32 height DBG_PASS)
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	//lprintf( "CRITICAL; BuildImageFile is not possible" );
	image->render_id = INVALID_INDEX;
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_BuildImageFileEx( pc, width, height DBG_RELAY );
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	//SendClientMessage( PMID_MakeImageFile, image );
	AddLink( &ThreadNetworkState.app->application_instance->images, image );
	image->id = FindLink( &ThreadNetworkState.app->application_instance->images, image );
	SendClientMessage( PMID_MakeImage, image, INVALID_INDEX, FALSE );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make built proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
	return (Image)image;
}

static Image CPROC Internal_MakeSubImage( PVPImage pImage, S_32 x, S_32 y, _32 width, _32 height, INDEX _id DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->x = x;
	image->y = y;
	image->w = width;
	image->h = height;
	image->render_id = ((PVPImage)pImage)->render_id;
	if( ((PVPImage)pImage)->image )
	{
		image->image = l.real_interface->_MakeSubImageEx( ((PVPImage)pImage)->image, x, y, width, height DBG_RELAY );
		image->image->reverse_interface = &InstanceProxyImageInterface;
		image->image->reverse_interface_instance = image;
	}
	image->parent = (PVPImage)pImage;
	if( image->next = ((PVPImage)pImage)->child )
		image->next->prior = image;
	((PVPImage)pImage)->child = image;

	if( _id == INVALID_INDEX )
	{
		AddLink( &ThreadNetworkState.app->application_instance->images, image );
		image->id = FindLink( &ThreadNetworkState.app->application_instance->images, image );
	}
	else
	{
		image->id = _id;
		SetLink( &ThreadNetworkState.app->application_instance->images, _id, image );
	}
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "Make sub proxy image %p %d(%d,%d)  on %p %d", image, image->id, image->w, image->w, ((PVPImage)pImage), ((PVPImage)pImage)->id  );
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	SendClientMessage( PMID_MakeSubImage, image );

	return (Image)image;
}

static Image CPROC VidlibProxy_MakeSubImageEx  ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
	return Internal_MakeSubImage( (PVPImage)pImage, x, y, width, height, INVALID_INDEX DBG_RELAY );
}
static Image CPROC VidlibProxy_RemakeImageEx	 ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS)
{
	PVPImage image;
	if( !(image = (PVPImage)pImage ) )
	{
		image = New( struct vidlib_proxy_image );
		MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
		image->render_id = INVALID_INDEX;
		AddLink( &ThreadNetworkState.app->application_instance->images, image );
	}
	//lprintf( "CRITICAL; RemakeImageFile is not possible" );
	image->w = width;
	image->h = height;
	image->image = l.real_interface->_RemakeImageEx( image->image, pc, width, height DBG_RELAY );
	image->image->reverse_interface = &InstanceProxyImageInterface;
	image->image->reverse_interface_instance = image;
	image->id = FindLink( &ThreadNetworkState.app->application_instance->images, image );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	return (Image)image;
}

static Image CPROC VidlibProxy_LoadImageFileFromGroupEx( INDEX group, CTEXTSTR filename DBG_PASS )
{
	PVPImage image = New( struct vidlib_proxy_image );
	MemSet( image, 0, sizeof( struct vidlib_proxy_image ) );
	image->filegroup = group;
	image->filename = StrDup( filename );
	image->image = l.real_interface->_LoadImageFileFromGroupEx( group, filename DBG_RELAY );
	if( image->image )
	{
		image->w = image->image->width;
		image->h = image->image->height;
	}
	image->render_id = INVALID_INDEX;
	// don't really need to make this; if it needs to be updated to the client it will be handled later
	AddLink( &ThreadNetworkState.app->application_instance->images, image );
	image->id = FindLink( &ThreadNetworkState.app->application_instance->images, image );

	SendClientMessage( PMID_MakeImage, image, INVALID_INDEX, TRUE );
	//lprintf( "%p(%p) is %d", image, image->image, image->id );
	//lprintf( "loaded proxy image %p %d(%d,%d)", image, image->id, image->w, image->w );
	return (Image)image;
}

static Image CPROC VidlibProxy_LoadImageFileEx( CTEXTSTR filename DBG_PASS )
{
	return VidlibProxy_LoadImageFileFromGroupEx( GetFileGroup( WIDE("Images"), WIDE("./images") ), filename DBG_RELAY );
}



static void CPROC VidlibProxy_ResizeImageEx	  ( Image pImage, S_32 width, S_32 height DBG_PASS)
{
	PVPImage image = (PVPImage)pImage;
	image->w = width;
	image->h = height;
	l.real_interface->_ResizeImageEx( image->image, width, height DBG_RELAY );
	SendClientMessage( PMID_Size_Image, image );
}

static void CPROC VidlibProxy_MoveImage			( Image pImage, S_32 x, S_32 y )
{
	PVPImage image = (PVPImage)pImage;
	if( image->x != x || image->y != y )
	{
		image->x = x;
		image->y = y;
		l.real_interface->_MoveImage( image->image, x, y );

		SendClientMessage( PMID_Move_Image, image );
	}
}

P_8 GetMessageBuf( PVPImage image, size_t size )
{
	P_8 resultbuf;
	if( ( image->buf_avail ) <= ( size + image->sendlen ) )
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
	((_32*)resultbuf)[0] = size - 4;
	image->sendlen += size;

	return resultbuf + 4;
}

static void AppendJSON( PVPImage image, TEXTSTR msg, POINTER outmsg, size_t sendlen, LOGICAL queue )
{
	size_t size = StrLen( msg );
	if( (image->websock_buf_avail ) < (size * sizeof( TEXTCHAR ) + image->websock_sendlen + 10 ) )
	{
		P_8 newbuf;
		image->websock_buf_avail += size * sizeof( TEXTCHAR ) + 256;
		lprintf( WIDE("make new array for %p %d"), image, image->websock_buf_avail );
		newbuf = NewArray( _8, image->websock_buf_avail );
		if( image->websock_buffer )
		{
			MemCpy( newbuf, image->websock_buffer, image->websock_sendlen );
			Release( image->websock_buffer );
		}
		image->websock_buffer = newbuf;
	}
	if( image->websock_sendlen == 0 )
	{
		image->websock_buffer[0] = '[';
		image->websock_sendlen++;
#ifdef _UNICODE
		image->websock_buffer[image->websock_sendlen] = 0;
		image->websock_sendlen++;
#endif
	}
	else
	{
		image->websock_buffer[image->websock_sendlen] = ',';
		image->websock_sendlen++;
#ifdef _UNICODE
		image->websock_buffer[image->websock_sendlen] = 0;
		image->websock_sendlen++;
#endif
	}
	MemCpy( image->websock_buffer + image->websock_sendlen, msg, size * sizeof( TEXTCHAR ) );
	image->websock_sendlen += size * sizeof( TEXTCHAR );

	if( 0 )
	{
		struct server_socket_state *state = ThreadNetworkState.app;
		if( state->websock )
		{
			if( queue )
				EnqueLink( &ThreadNetworkState.app->pending_operations, msg );
			else
				WebSocketSendText( state->pc, msg, StrLen( msg ) );
		}
		else
			SendTCP( state->pc, outmsg, sendlen );
	}
}

static LOGICAL FixArea( IMAGE_RECTANGLE *result, IMAGE_RECTANGLE *source, PVPImage pifDest )
{
	IMAGE_RECTANGLE r2;
	// build rectangle of what we want to show
	// build rectangle which is presently visible on image
	r2.x		= 0;
	r2.y		= 0;
	r2.height = pifDest->h;
	r2.width  = pifDest->w;
	return l.real_interface->_IntersectRectangle( result, source, &r2 );
}

static void ClearImageBuffers( PVPImage image, LOGICAL image_only )
{
	//while( image )
	{
		image->websock_sendlen = 0;
		image->sendlen = 0;
		if( !image_only && image->child )
		{
			PVPImage child;
			for( child = image->child; child; child = child->next )
				ClearImageBuffers( child, 0 );
		}
	}
}

static void CPROC VidlibProxy_BlatColor	  ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	if( ((PVPImage)pifDest)->render_id != INVALID_INDEX )
	{
		struct json_context_object *cto;
		PVPImage image = (PVPImage)pifDest;
		struct common_message *outmsg;
		size_t sendlen;
		IMAGE_RECTANGLE r, r1;
 		// build rectangle of what we want to show
		r1.x		= x;
		r1.width  = w;
		r1.y		= y;
		r1.height = h;
		if( !FixArea( &r, &r1, image ) )
		{
			return;
		}
		EnterCriticalSec( &l.message_formatter );
		if( w == image->w && h == image->h )
		{
			ClearImageBuffers( image, 0 );
		}
		cto = (struct json_context_object *)GetLink( &l.messages, PMID_BlatColor );
		if( !cto )
			cto = WebSockInitJson( PMID_BlatColor );

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen=( 4 + 1 + sizeof( struct blatcolor_data ) ) );
		outmsg->message_id = PMID_BlatColor;
		outmsg->data.blatcolor.x = r.x;
		outmsg->data.blatcolor.y = r.y;
		outmsg->data.blatcolor.w = r.width;
		outmsg->data.blatcolor.h = r.height;
		outmsg->data.blatcolor.color = color;
		outmsg->data.blatcolor.server_image_id = image->id;
		/*
		if( w == image->w && h == image->h )
		{
			//lprintf( "clear buffer; full color : %08x", color );
		}
		else
		{
			//lprintf( "w:%d w:%d h:%d h:%d full color : %08x", w, image->w, h, image->h, color );
		}
		*/
		{
			TEXTSTR json_msg = json_build_message( cto, outmsg );
			AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
			Release( json_msg );
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		l.real_interface->_BlatColor( ((PVPImage)pifDest)->image, x, y, w, h, color );
	}
	((PVPImage)pifDest)->image->flags |= IF_FLAG_UPDATED;

}

static void CPROC VidlibProxy_BlatColorAlpha( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	if( ((PVPImage)pifDest)->render_id != INVALID_INDEX )
	{
		IMAGE_RECTANGLE r, r1;
		struct json_context_object *cto;
		PVPImage image = (PVPImage)pifDest;
		struct common_message *outmsg;
		size_t sendlen;
		cto = (struct json_context_object *)GetLink( &l.messages, PMID_BlatColorAlpha );
		if( !cto )
			cto = WebSockInitJson( PMID_BlatColor );

		// build rectangle of what we want to show
		r1.x		= x;
		r1.width  = w;
		r1.y		= y;
		r1.height = h;
		// build rectangle which is presently visible on image
		if( !FixArea( &r, &r1, image ) )
		{
			return;
		}
		EnterCriticalSec( &l.message_formatter );
		if( w == image->w && h == image->h && l.real_interface->_GetAlphaValue( color ) == 255 )
		{
			ClearImageBuffers( image, 0 );
		}

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct blatcolor_data ) ) );
		outmsg->message_id = PMID_BlatColorAlpha;
		outmsg->data.blatcolor.x = r.x;
		outmsg->data.blatcolor.y = r.y;
		outmsg->data.blatcolor.w = r.width;
		outmsg->data.blatcolor.h = r.height;
		outmsg->data.blatcolor.color = color;
		outmsg->data.blatcolor.server_image_id = image->id;
		if( w == image->w && h == image->h )
		{
			//lprintf( "clear buffer; full color : %08x", color );
		}
		else
		{
			//lprintf( "w:%d w:%d h:%d h:%d full color : %08x", w, image->w, h, image->h, color );
		}
		{
			TEXTSTR json_msg = json_build_message( cto, outmsg );
			AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
			Release( json_msg );
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		l.real_interface->_BlatColorAlpha( ((PVPImage)pifDest)->image, x, y, w, h, color );
	}
	((PVPImage)pifDest)->image->flags |= IF_FLAG_UPDATED;
}

static void SetImageUsed( PVPImage image )
{
	while( image  )
	{
		image->flags.bUsed = 1;
		image = image->parent;
	}
}

static void CPROC VidlibProxy_BlotImageSizedEx( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... )
{
	PVPImage image = (PVPImage)pDest;
	PVPImage parent_src = (PVPImage)pIF;
	if( !((PVPImage)pIF)->image )
		return;
	while( parent_src && parent_src->parent )
	{
		parent_src = parent_src->parent;
	}
	if( parent_src->render_id != INVALID_INDEX )
	{
		lprintf( WIDE( "cannot blot from display to client (yet, need soft emulation *memory*)") );
		return;
	}
	lprintf( WIDE( "blot image %p to %d,%d from %d,%d  %dx%d"), x, y, xs, ys, wd, ht );
	if( ((PVPImage)pDest)->render_id != INVALID_INDEX )
	{
		IMAGE_RECTANGLE r;
		struct json_context_object *cto;
		struct common_message *outmsg;
		size_t sendlen;
		EnterCriticalSec( &l.message_formatter );
		cto = (struct json_context_object *)GetLink( &l.messages, PMID_BlotImageSizedTo );
		if( !cto )
			cto = WebSockInitJson( PMID_BlotImageSizedTo );

		{
			IMAGE_RECTANGLE r1;
			// build rectangle of what we want to show
			r1.x		= x;
			r1.width  = wd;
			r1.y		= y;
			r1.height = ht;
			// build rectangle which is presently visible on image
			if( !FixArea( &r, &r1, image ) )
			{
				LeaveCriticalSec( &l.message_formatter );
				return;
			}
		}
		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct blot_image_data ) ) );
		outmsg->message_id = PMID_BlotImageSizedTo;
		if( x != r.x )
		{
			xs += (r.x-x);
		}
		if( y != r.y )
		{
			ys += (r.y-y);
		}
		outmsg->data.blot_image.x = r.x;
		outmsg->data.blot_image.y = r.y;
		outmsg->data.blot_image.w = r.width;
		outmsg->data.blot_image.h = r.height;
		switch( method & 3 )
		{
		case BLOT_COPY:
			// sending this clears the flag.
			if( ((PVPImage)pIF)->image->flags & IF_FLAG_UPDATED )
				SendClientMessage( PMID_ImageData, pIF );
			SetImageUsed( (PVPImage)pIF );
			outmsg->data.blot_image.image_id = ((PVPImage)pIF)->id;
			break;
		case BLOT_SHADED:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );
				while( ((PVPImage)pIF)->parent )
				{
					xs += ((PVPImage)pIF)->x;
					ys += ((PVPImage)pIF)->y;
					pIF = (Image)(((PVPImage)pIF)->parent);
				}
				((PVPImage)pIF)->image->reverse_interface = &InstanceProxyImageInterface;
				((PVPImage)pIF)->image->reverse_interface_instance = (POINTER)pIF;
				shaded_image = l.real_interface->_GetTintedImage( ((PVPImage)pIF)->image, va_arg( args, CDATA ) );
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
					if( shaded_image->flags & IF_FLAG_UPDATED )
						SendClientMessage( PMID_ImageData, actual_image );
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_image.image_id = actual_image->id;
			}
			break;
		case BLOT_MULTISHADE:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );

				while( ((PVPImage)pIF)->parent )
				{
					xs += ((PVPImage)pIF)->x;
					ys += ((PVPImage)pIF)->y;
					pIF = (Image)(((PVPImage)pIF)->parent);
				}

				{
					CDATA a = va_arg( args, CDATA );
					CDATA b = va_arg( args, CDATA );
					CDATA c = va_arg( args, CDATA );
					shaded_image = l.real_interface->_GetShadedImage( ((PVPImage)pIF)->image, a, b, c );
				}
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
					if( shaded_image->flags & IF_FLAG_UPDATED )
						SendClientMessage( PMID_ImageData, actual_image );
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_image.image_id = actual_image->id;
			}
			break;
		}
		outmsg->data.blot_image.xs = xs;
		outmsg->data.blot_image.ys = ys;
		outmsg->data.blot_image.server_image_id = image->id;
		if( ((PVPImage)pIF)->image->flags & IF_FLAG_UPDATED )
				SendClientMessage( PMID_ImageData, ((PVPImage)pIF) );
		{
			TEXTSTR json_msg = json_build_message( cto, outmsg );
			if( image->render_id != INVALID_INDEX )
			{
				PVPRENDER r = GetLink( &ThreadNetworkState.app->application_instance->renderers, image->render_id );
				if( !r || !r->flags.open )
					AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, TRUE );
				else
				{
					AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
					Release( json_msg );
				}
			}
			else
			{
				AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
				Release( json_msg );
			}
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		va_list args;
		va_start( args, method );
		l.real_interface->_BlotImageEx( image->image, ((PVPImage)pIF)->image, x, y, xs, ys, wd, ht
					, nTransparent
					, method
					, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) );
	}
	((PVPImage)image)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_BlotImageEx	  ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... )
{
	va_list args;
	va_start( args, method );
	VidlibProxy_BlotImageSizedEx( pDest, pIF, x, y, 0, 0
					, ((PVPImage)pIF)->w, ((PVPImage)pIF)->h
					, nTransparent
					, method, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) ); 
}



static void CPROC VidlibProxy_BlotScaledImageSizedEx( Image pifDest, Image pifSrc
											  , S_32 xd, S_32 yd
											  , _32 wd, _32 hd
											  , S_32 xs, S_32 ys
											  , _32 ws, _32 hs
											  , _32 nTransparent
											  , _32 method, ... )
{
	PVPImage image = (PVPImage)pifDest;
	if( image->render_id != INVALID_INDEX )
	{
		struct json_context_object *cto;
		struct common_message *outmsg;
		size_t sendlen;
		_32 dhd, dwd, dhs, dws;
		int errx, erry;
		if( !((PVPImage)pifSrc)->image )
			return;
		if( ( xd > ( image->x + image->w ) )
			|| ( yd > ( image->y + image->h ) )
			|| ( ( xd + (signed)wd ) < 0/*pifDest->x*/ )
			|| ( ( yd + (signed)hd ) < 0/*pifDest->y*/ ) )
		{
			return;
		}
		dhd = hd;
		dhs = hs;
		dwd = wd;
		dws = ws;

		// ok - how to figure out how to do this
		// need to update the position and width to be within the 
		// the bounds of pifDest....
		//	lprintf(" begin scaled output..." );
		errx = -(signed)dwd;
		erry = -(signed)dhd;

		if( ( xd < 0 ) || ( (xd+(S_32)(wd&0x7FFFFFFF)) > image->w ) )
		{
			int x = xd;
			int w = 0;
			while( x < 0 )
			{
				errx += (signed)dws;
				while( errx >= 0 )
				{
					errx -= (signed)dwd;
					ws--;
					xs++;
				}
				wd--;
				x++;
			}
			xd = x;

			while( x < image->w && SUS_GT( w, int, wd, _32 ) )
			{
				errx += (signed)dws;
				while( errx >= 0 )
				{
					errx -= (signed)dwd;
					w++;
				}
				x++;
			}
			wd = w;
		}
		//Log8( WIDE("Blot scaled params: %d %d %d %d / %d %d %d %d "), 
		//		 xs, ys, ws, hs, xd, yd, wd, hd );
		if( ( yd < 0 ) || ( yd +(S_32)(hd&0x7FFFFFFF) ) > image->h )
		{
			int y = yd;
			int h = 0;
			while( yd < 0 )
			{
				erry += (signed)dhs;
				while( erry >= 0 )
				{
					erry -= (signed)dhd;
					hs--;
					ys++;
				}
				hd--;
				yd++;
			}
			y = yd;
			while( y < image->h && h < (int)hd )
			{
				erry += (signed)dhs;
				while( erry >= 0 )
				{
					erry -= (signed)dhd;
					h++;
				}
				y++;
			}
			hd = h;
		}

		if( !wd || !hd )
			return;
		EnterCriticalSec( &l.message_formatter );
		cto = (struct json_context_object *)GetLink( &l.messages, PMID_BlotScaledImageSizedTo );
		if( !cto )
			cto = WebSockInitJson( PMID_BlotScaledImageSizedTo );

		// sending this clears the flag.
		//  different blot methods send different image(data)
		//if( ((PVPImage)pifSrc)->image->flags & IF_FLAG_UPDATED )
		//	SendClientMessage( PMID_ImageData, pifSrc );

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct blot_scaled_image_data ) ) );
		outmsg->message_id = PMID_BlotScaledImageSizedTo;

		switch( method & 3 )
		{
		case BLOT_COPY:
			// sending this clears the flag.
			if( ((PVPImage)pifSrc)->image->flags & IF_FLAG_UPDATED )
				SendClientMessage( PMID_ImageData, pifSrc );
			SetImageUsed( (PVPImage)pifSrc );

			outmsg->data.blot_scaled_image.image_id = ((PVPImage)pifSrc)->id;
			//lprintf( WIDE("copy source image ID : %d"), ((PVPImage)pifSrc)->id );
			break;
		case BLOT_SHADED:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );
				while( ((PVPImage)pifSrc)->parent )
				{
					xs += ((PVPImage)pifSrc)->x;
					ys += ((PVPImage)pifSrc)->y;
					pifSrc = (Image)(((PVPImage)pifSrc)->parent);
				}
				((PVPImage)pifSrc)->image->reverse_interface = &InstanceProxyImageInterface;
				((PVPImage)pifSrc)->image->reverse_interface_instance = (POINTER)pifSrc;
				shaded_image = l.real_interface->_GetTintedImage( ((PVPImage)pifSrc)->image, va_arg( args, CDATA ) );
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					lprintf( WIDE("wrap shaded image for %p %d"), ((PVPImage)pifSrc), ((PVPImage)pifSrc)->id );
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
					if( shaded_image->flags & IF_FLAG_UPDATED )
						SendClientMessage( PMID_ImageData, actual_image );
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_scaled_image.image_id = actual_image->id;
			}
			break;
		case BLOT_MULTISHADE:
			{
				va_list args;
				Image shaded_image;
				PVPImage actual_image;
				va_start( args, method );

				while( ((PVPImage)pifSrc)->parent )
				{
					xs += ((PVPImage)pifSrc)->x;
					ys += ((PVPImage)pifSrc)->y;
					pifSrc = (Image)(((PVPImage)pifSrc)->parent);
				}
				((PVPImage)pifSrc)->image->reverse_interface = &InstanceProxyImageInterface;
				((PVPImage)pifSrc)->image->reverse_interface_instance = (POINTER)pifSrc;
				{
					CDATA a = va_arg( args, CDATA );
					CDATA b = va_arg( args, CDATA );
					CDATA c = va_arg( args, CDATA );
					shaded_image = l.real_interface->_GetShadedImage( ((PVPImage)pifSrc)->image, a, b, c );
				}
				if( !shaded_image->reverse_interface )
				{
					// new image, and we need to reverse track it....
					actual_image = WrapImageFile( shaded_image );
				}
				else
				{
					actual_image = (PVPImage)shaded_image->reverse_interface_instance;
					if( shaded_image->flags & IF_FLAG_UPDATED )
						SendClientMessage( PMID_ImageData, actual_image );
				}
				SetImageUsed( actual_image );
				outmsg->data.blot_scaled_image.image_id = actual_image->id;
			}
			break;
		}
		outmsg->data.blot_scaled_image.x = xd;
		outmsg->data.blot_scaled_image.y = yd;
		outmsg->data.blot_scaled_image.w = wd;
		outmsg->data.blot_scaled_image.h = hd;
		outmsg->data.blot_scaled_image.xs = xs;
		outmsg->data.blot_scaled_image.ys = ys;
		outmsg->data.blot_scaled_image.ws = ws;
		outmsg->data.blot_scaled_image.hs = hs;
		//outmsg->data.blot_scaled_image.image_id = ((PVPImage)pifSrc)->id;
		outmsg->data.blot_scaled_image.server_image_id = image->id;
		{
			TEXTSTR json_msg = json_build_message( cto, outmsg );
			if( image->render_id != INVALID_INDEX )
			{
				PVPRENDER r = GetLink( &ThreadNetworkState.app->application_instance->renderers, image->render_id );
				if( !r || !r->flags.open )
				{
					AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, TRUE );
				}
				else
				{
					AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
					Release( json_msg );
				}
			}
			else
			{
				AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
				Release( json_msg );
			}
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		va_list args;
		va_start( args, method );
		l.real_interface->_BlotScaledImageSizedEx( image->image, ((PVPImage)pifSrc)->image, xd, yd, wd, hd, xs, ys, ws, hs
					, nTransparent
					, method
					, va_arg( args, CDATA ), va_arg( args, CDATA ), va_arg( args, CDATA ) );
	}
	((PVPImage)image)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_MarkImageDirty ( Image pImage )
{
	// this library tracks the IF_FLAG_UPDATED which is set by native routine;native routine marks child and all parents.
	//lprintf( "Mark image %p dirty", pImage );
	l.real_interface->_MarkImageDirty( ((PVPImage)pImage)->image );
	if( 0 )
	{
		size_t outlen;
		P_8 encoded_image;
	
		if( ((PVPImage)pImage)->parent )
			encoded_image = EncodeImage( ((PVPImage)pImage)->id, ((PVPImage)pImage)->parent->image, FALSE, &outlen );
		else
			encoded_image = EncodeImage( ((PVPImage)pImage)->id, ((PVPImage)pImage)->image, FALSE, &outlen );
		Release( encoded_image );
	}
}

#define DIMAGE_DATA_PROC(type,name,args)  static type (CPROC VidlibProxy2_##name)args;static type (CPROC* VidlibProxy_##name)args = VidlibProxy2_##name; type (CPROC VidlibProxy2_##name)args

DIMAGE_DATA_PROC( void,plot,		( Image pi, S_32 x, S_32 y, CDATA c ))
{
	if( ((PVPImage)pi)->render_id != INVALID_INDEX )
	{
	}
	else
	{
		l.real_interface->_plot[0]( ((PVPImage)pi)->image, x, y, c );
		VidlibProxy_MarkImageDirty( pi );
	}
}

DIMAGE_DATA_PROC( void,plotalpha, ( Image pi, S_32 x, S_32 y, CDATA c ))
{
	if( ((PVPImage)pi)->render_id != INVALID_INDEX )
	{
	}
	else
	{
		l.real_interface->_plot[0]( ((PVPImage)pi)->image, x, y, c );
	}
}

DIMAGE_DATA_PROC( CDATA,getpixel, ( Image pi, S_32 x, S_32 y ))
{
	if( ((PVPImage)pi)->render_id != INVALID_INDEX )
	{
	}
	else
	{
		PVPImage my_image = (PVPImage)pi;
		if( my_image )
		{
			return (*l.real_interface->_getpixel)( my_image->image, x, y );
		}
	}
	return 0;
}

DIMAGE_DATA_PROC( void,do_line,	  ( Image pifDest, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color ))
{
	PVPImage image = (PVPImage)pifDest;
	if( image->render_id != INVALID_INDEX )
	{
		struct json_context_object *cto;
		struct common_message *outmsg;
		size_t sendlen;
		EnterCriticalSec( &l.message_formatter );
		cto = (struct json_context_object *)GetLink( &l.messages, PMID_DrawLine );
		if( !cto )
			cto = WebSockInitJson( PMID_DrawLine );

		outmsg = (struct common_message*)GetMessageBuf( image, sendlen = ( 4 + 1 + sizeof( struct line_data ) ) );
		outmsg->message_id = PMID_DrawLine;
		outmsg->data.line.server_image_id = image->id;
		outmsg->data.line.x1 = x;
		outmsg->data.line.y1 = y;
		outmsg->data.line.x2 = xto;
		outmsg->data.line.y2 = yto;
		outmsg->data.line.color = color;
		{
			TEXTSTR json_msg = json_build_message( cto, outmsg );
			if( image->render_id != INVALID_INDEX )
			{
				PVPRENDER r = GetLink( &ThreadNetworkState.app->application_instance->renderers, image->render_id );
				if( !r || !r->flags.open )
				{
					AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, TRUE );
				}
				else
				{
					AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
					Release( json_msg );
				}
			}
			else
			{
				AppendJSON( image, json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
				Release( json_msg );
			}
		}
		LeaveCriticalSec( &l.message_formatter );
	}
	else
	{
		l.real_interface->_do_line[0]( image->image, x, y, xto, yto, color );
	}
	((PVPImage)image)->image->flags |= IF_FLAG_UPDATED;
}

DIMAGE_DATA_PROC( void,do_lineAlpha,( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color))
{
	VidlibProxy2_do_line( pBuffer, x, y, xto, yto, color );
}


DIMAGE_DATA_PROC( void,do_hline,	  ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, xfrom, y, xto, y, color );
}

DIMAGE_DATA_PROC( void,do_vline,	  ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, x, yfrom, x, yto, color );
}

DIMAGE_DATA_PROC( void,do_hlineAlpha,( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, xfrom, y, xto, y, color );
}

DIMAGE_DATA_PROC( void,do_vlineAlpha,( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color ))
{
	VidlibProxy2_do_line( pImage, x, yfrom, x, yto, color );
}

static SFTFont CPROC VidlibProxy_GetDefaultFont ( void )
{
	return l.real_interface->_GetDefaultFont( );
}

static _32 CPROC VidlibProxy_GetFontHeight  ( SFTFont font )
{
	return l.real_interface->_GetFontHeight( font );
}

static _32 CPROC VidlibProxy_GetStringSizeFontEx( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont )
{
	return l.real_interface->_GetStringSizeFontEx( pString, len, width, height, UseFont );
}

static void CPROC VidlibProxy_PutCharacterFont		  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutCharacterVerticalFont( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterVerticalFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutCharacterInvertFont  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterInvertFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutCharacterVerticalInvertFont( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font )
{
	l.real_interface->_PutCharacterVerticalInvertFont( ((PVPImage)pImage)->image, x, y, color, background, c, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutStringFontExx  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background
												, CTEXTSTR pc, size_t nLen, SFTFont font, int justification, _32 right )
{
	l.real_interface->_PutStringFontExx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font, justification, right );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutStringFontEx  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background
												, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutStringVerticalFontEx		( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringVerticalFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutStringInvertFontEx		  ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringInvertFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static void CPROC VidlibProxy_PutStringInvertVerticalFontEx( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
	l.real_interface->_PutStringInvertVerticalFontEx( ((PVPImage)pImage)->image, x, y, color, background, pc, nLen, font );
	((PVPImage)pImage)->image->flags |= IF_FLAG_UPDATED;
}

static _32 CPROC VidlibProxy_GetMaxStringLengthFont( _32 width, SFTFont UseFont )
{
	return l.real_interface->_GetMaxStringLengthFont( width, UseFont );
}

static void CPROC VidlibProxy_GetImageSize ( Image pImage, _32 *width, _32 *height )
{
	if( width )
		(*width) = ((PVPImage)pImage)->w;
	if( height )
		(*height) = ((PVPImage)pImage)->h;
}

static SFTFont CPROC VidlibProxy_LoadFont ( SFTFont font )
{
}
			/* <combine sack::image::UnloadFont@SFTFont>
				
				\ \												*/
			IMAGE_PROC_PTR( void, UnloadFont )					  ( SFTFont font );

/* Internal
	Interface index 44
	
	This is used by internal methods to transfer image and font
	data to the render agent.											  */	IMAGE_PROC_PTR( DataState, BeginTransferData )	 ( _32 total_size, _32 segsize, CDATA data );
/* Internal
	Interface index 45
	
	Used internally to transfer data to render agent. */	IMAGE_PROC_PTR( void, ContinueTransferData )		( DataState state, _32 segsize, CDATA data );
/* Internal
	Interface index 46
	
	Command issues at end of data transfer to decode the data
	into an image.														  */	IMAGE_PROC_PTR( Image, DecodeTransferredImage )	 ( DataState state );
/* After a data transfer decode the information as a font.
	Internal
	Interface index 47												  */	IMAGE_PROC_PTR( SFTFont, AcceptTransferredFont )	  ( DataState state );

DIMAGE_DATA_PROC( CDATA, ColorAverage,( CDATA c1, CDATA c2
													, int d, int max ))
{
	return c1;
}
/* <combine sack::image::SyncImage>
	
	Internal
	Interface index 49					*/	IMAGE_PROC_PTR( void, SyncImage )					  ( void );

static PCDATA CPROC VidlibProxy_GetImageSurface 		 ( Image pImage )
{
	if( pImage )
	{
		if( ((PVPImage)pImage)->render_id == INVALID_INDEX )
			return l.real_interface->_GetImageSurface( ((PVPImage)pImage)->image );
	}
	return NULL;
}

/* <combine sack::image::IntersectRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
				
				\ \																															*/
			IMAGE_PROC_PTR( int, IntersectRectangle )		( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
	/* <combine sack::image::MergeRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
		
		\ \																													  */
	IMAGE_PROC_PTR( int, MergeRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
	/* <combine sack::image::GetImageAuxRect@Image@P_IMAGE_RECTANGLE>
		
		\ \																				*/
	IMAGE_PROC_PTR( void, GetImageAuxRect )	( Image pImage, P_IMAGE_RECTANGLE pRect );
	/* <combine sack::image::SetImageAuxRect@Image@P_IMAGE_RECTANGLE>
		
		\ \																				*/
	IMAGE_PROC_PTR( void, SetImageAuxRect )	( Image pImage, P_IMAGE_RECTANGLE pRect );

static void CPROC VidlibProxy_OrphanSubImage ( Image pImage )
{
	PVPImage image = (PVPImage)pImage;
	if( image )
	{
		//if( !image->parent
		//	|| ( pImage->flags & IF_FLAG_OWN_DATA ) )
		//	return;
		if( image->prior )
			image->prior->next = image->next;
		else
			image->parent->child = image->next;

		if( image->next )
			image->next->prior = image->prior;

		image->parent = NULL;
		image->next = NULL; 
		image->prior = NULL; 
		
		if( image->image )
			l.real_interface->_OrphanSubImage( image->image );
	}
}

static void SmearRenderFlag( PVPImage image )
{
	for( ; image; image = image->next )
	{
		if( image->image && ( ( image->render_id = image->parent->render_id ) != INVALID_INDEX ) )
			image->image->flags |= IF_FLAG_FINAL_RENDER;
		image->image->reverse_interface = &InstanceProxyImageInterface;
		image->image->reverse_interface_instance = image;
		SmearRenderFlag( image->child );
	}
}


static void CPROC VidlibProxy_AdoptSubImage ( Image pFoster, Image pOrphan )
{
	PVPImage foster = (PVPImage)pFoster;
	PVPImage orphan = (PVPImage)pOrphan;
	if( foster->id == 1 )
	{
		int a =3 ;
	}
	if( foster && orphan )
	{
		if( ( orphan->next = foster->child ) )
			orphan->next->prior = orphan;
		orphan->parent = foster;
		foster->child = orphan;
		orphan->prior = NULL; // otherwise would be undefined
		SmearRenderFlag( orphan );

		if( foster->image && orphan->image )
			l.real_interface->_AdoptSubImage( foster->image, orphan->image );
	}
}

static void CPROC VidlibProxy_TransferSubImages( Image pImageTo, Image pImageFrom )
{
	PVPImage tmp;
	while( tmp = ((PVPImage)pImageFrom)->child )
	{
		// moving a child allows it to keep all of it's children too?
		// I think this is broken in that case; Orphan removes from the family entirely?
		VidlibProxy_OrphanSubImage( (Image)tmp );
		VidlibProxy_AdoptSubImage( (Image)pImageTo, (Image)tmp );
	}
	{
		struct json_context_object *cto;
		struct common_message *outmsg;
		size_t sendlen;
		EnterCriticalSec( &l.message_formatter );
		cto = (struct json_context_object *)GetLink( &l.messages, PMID_TransferSubImages );
		if( !cto )
			cto = WebSockInitJson( PMID_TransferSubImages );

		outmsg = (struct common_message*)GetMessageBuf( (PVPImage)pImageTo, sendlen = ( 4 + 1 + sizeof( struct transfer_sub_image_data ) ) );
		outmsg->message_id = PMID_TransferSubImages;
		outmsg->data.transfer_sub_image.image_from_id = ((PVPImage)pImageFrom)->id;
		outmsg->data.transfer_sub_image.image_to_id = ((PVPImage)pImageTo)->id;
		{
			TEXTSTR json_msg = json_build_message( cto, outmsg );
			{
				struct server_socket_state *state = ThreadNetworkState.app;
				if( state->websock )
					WebSocketSendText( state->pc, json_msg, StrLen( json_msg ) );
				else
					SendTCP( state->pc, outmsg, sendlen );
			}
			//AppendJSON( ((PVPImage)pImageTo), json_msg, ((P_8)outmsg)-4, sendlen, FALSE );
			Release( json_msg );
		}
		LeaveCriticalSec( &l.message_formatter );
	}
}

	/* <combine sack::image::MakeSpriteImageFileEx@CTEXTSTR fname>
		
		\ \																			*/
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS );
	/* <combine sack::image::MakeSpriteImageEx@Image image>
		
		\ \																  */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageEx )( Image image DBG_PASS );
	/* <combine sack::image::rotate_scaled_sprite@Image@PSPRITE@fixed@fixed@fixed>
		
		\ \																								 */
	IMAGE_PROC_PTR( void	, rotate_scaled_sprite )(Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height);
	/* <combine sack::image::rotate_sprite@Image@PSPRITE@fixed>
		
		\ \																		*/
	IMAGE_PROC_PTR( void	, rotate_sprite )(Image bmp, PSPRITE sprite, fixed angle);
 /* <combine sack::image::BlotSprite@Image@PSPRITE>
																	  
	 Internal
	Interface index 61															 */
		IMAGE_PROC_PTR( void	, BlotSprite )( Image pdest, PSPRITE ps );
	 /* <combine sack::image::DecodeMemoryToImage@P_8@_32>
		 
		 \ \																*/
static Image CPROC VidlibProxy_DecodeMemoryToImage ( P_8 buf, _32 size )
{
	Image real_image = l.real_interface->_DecodeMemoryToImage( buf, size );
	return (Image)WrapImageFile( real_image );
}

/* <combine sack::image::GetFontRenderData@SFTFont@POINTER *@_32 *>
	
	\ \																			  */
IMAGE_PROC_PTR( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, S_32 x, S_32 y );
/* <combine sack::image::SetSpritePosition@PSPRITE@S_32@S_32>
	
	\ \																		  */
IMAGE_PROC_PTR( PSPRITE, SetSpritePosition )( PSPRITE sprite, S_32 x, S_32 y );
	/* <combine sack::image::UnmakeImageFileEx@Image pif>
		
		\ \																*/
	IMAGE_PROC_PTR( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
/* <combine sack::image::GetGlobalFonts>
	
	\ \											  */

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@_32 *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@_32 *@SFTFont>
	
	\ \																																																							*/
IMAGE_PROC_PTR( _32, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );

IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );


static Image CPROC VidlibProxy_GetNativeImage( Image pImage )
{
	return ((PVPImage)pImage)->image;
}

IMAGE_PROC_PTR( void, DumpFontCache )( void );
IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

IMAGE_PROC_PTR( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
IMAGE_PROC_PTR( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
IMAGE_PROC_PTR( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, VECTOR o, LOGICAL render_pixel_scaled );


static IMAGE_INTERFACE InstanceProxyImageInterface = {
	VidlibProxy_SetStringBehavior,
		VidlibProxy_SetBlotMethod,
		VidlibProxy_BuildImageFileEx,
		VidlibProxy_MakeImageFileEx,
		VidlibProxy_MakeSubImageEx,
		VidlibProxy_RemakeImageEx,
		VidlibProxy_LoadImageFileEx,
		VidlibProxy_UnmakeImageFileEx,
		VidlibProxy_ResizeImageEx,
		VidlibProxy_MoveImage,
		VidlibProxy_BlatColor
		, VidlibProxy_BlatColorAlpha
		, VidlibProxy_BlotImageEx
		, VidlibProxy_BlotImageSizedEx
		, VidlibProxy_BlotScaledImageSizedEx
		, &VidlibProxy_plot
		, &VidlibProxy_plotalpha
		, &VidlibProxy_getpixel
		, &VidlibProxy_do_line
		, &VidlibProxy_do_lineAlpha
		, &VidlibProxy_do_hline
		, &VidlibProxy_do_vline
		, &VidlibProxy_do_hlineAlpha
		, &VidlibProxy_do_vlineAlpha
		, VidlibProxy_GetDefaultFont
		, VidlibProxy_GetFontHeight
		, VidlibProxy_GetStringSizeFontEx
		, VidlibProxy_PutCharacterFont
		, VidlibProxy_PutCharacterVerticalFont
		, VidlibProxy_PutCharacterInvertFont
		, VidlibProxy_PutCharacterVerticalInvertFont
		, VidlibProxy_PutStringFontEx
		, VidlibProxy_PutStringVerticalFontEx
		, VidlibProxy_PutStringInvertFontEx
		, VidlibProxy_PutStringInvertVerticalFontEx
		, VidlibProxy_GetMaxStringLengthFont
		, VidlibProxy_GetImageSize

		, NULL//VidlibProxy_LoadFont
		, NULL//VidlibProxy_UnloadFont
		, NULL//VidlibProxy_BeginTransferData
		, NULL//VidlibProxy_ContinueTransferData
		, NULL//VidlibProxy_DecodeTransferredImage
		, NULL//VidlibProxy_AcceptTransferredFont
		, &VidlibProxy_ColorAverage
		, NULL//VidlibProxy_SyncImage
		, VidlibProxy_GetImageSurface
		, NULL//VidlibProxy_IntersectRectangle
		, NULL//VidlibProxy_MergeRectangle
		, NULL// ** VidlibProxy_GetImageAuxRect
		, NULL// ** VidlibProxy_SetImageAuxRect
		, VidlibProxy_OrphanSubImage
		, VidlibProxy_AdoptSubImage
		, NULL // *****	VidlibProxy_MakeSpriteImageFileEx
		, NULL // *****	VidlibProxy_MakeSpriteImageEx
		, NULL // *****	VidlibProxy_rotate_scaled_sprite
		, NULL // *****	VidlibProxy_rotate_sprite
		, NULL // *****	VidlibProxy_BlotSprite
		, VidlibProxy_DecodeMemoryToImage
		, NULL//VidlibProxy_InternalRenderFontFile
		, NULL//VidlibProxy_InternalRenderFont
		, NULL//VidlibProxy_RenderScaledFontData
		, NULL//VidlibProxy_RenderFontFileScaledEx
		, NULL//VidlibProxy_DestroyFont
		, NULL
		, NULL//VidlibProxy_GetFontRenderData
		, NULL//VidlibProxy_SetFontRendererData
		, NULL //VidlibProxy_SetSpriteHotspot
		, NULL //VidlibProxy_SetSpritePosition
		, NULL //VidlibProxy_UnmakeSprite
		, NULL //VidlibProxy_struct font_global_tag *, GetGlobalFonts)( void );

, NULL //IMAGE_PROC_PTR( _32, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

, NULL //IMAGE_PROC_PTR( Image, LoadImageFileFromGroupEx )( INDEX group, CTEXTSTR filename DBG_PASS );

, NULL //IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
, NULL //IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );

, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetRedValue )( CDATA color ) ;
, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetGreenValue )( CDATA color );
, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetBlueValue )( CDATA color );
, NULL //IMAGE_PROC_PTR( COLOR_CHANNEL, GetAlphaValue )( CDATA color );
, NULL //IMAGE_PROC_PTR( CDATA, SetRedValue )( CDATA color, COLOR_CHANNEL r ) ;
, NULL //IMAGE_PROC_PTR( CDATA, SetGreenValue )( CDATA color, COLOR_CHANNEL green );
, NULL //IMAGE_PROC_PTR( CDATA, SetBlueValue )( CDATA color, COLOR_CHANNEL b );
, NULL //IMAGE_PROC_PTR( CDATA, SetAlphaValue )( CDATA color, COLOR_CHANNEL a );
, NULL //IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
, NULL //IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );

, NULL //IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
, NULL //IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
, NULL //IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );
, NULL //IMAGE_PROC_PTR( void, MarkImageDirty )( Image pImage );

, NULL //IMAGE_PROC_PTR( void, DumpFontCache )( void );
, NULL //IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
, NULL //IMAGE_PROC_PTR( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
, NULL //IMAGE_PROC_PTR( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
, NULL //IMAGE_PROC_PTR( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

, NULL //IMAGE_PROC_PTR( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
, NULL //IMAGE_PROC_PTR( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
, NULL // IMAGE_PROC_PTR( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
, NULL //IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, VECTOR o, LOGICAL render_pixel_scaled );

};

static CDATA CPROC VidlibProxy_SetRedValue( CDATA color, COLOR_CHANNEL r )
{
	return ( ((color)&0xFFFFFF00) | ( ((r)&0xFF)<<0 ) );
}
static CDATA CPROC VidlibProxy_SetGreenValue( CDATA color, COLOR_CHANNEL green )
{
	return ( ((color)&0xFFFF00FF) | ( ((green)&0xFF)<<8 ) );
}
static CDATA CPROC VidlibProxy_SetBlueValue( CDATA color, COLOR_CHANNEL b )
{
	return ( ((color)&0xFF00FFFF) | ( ((b)&0xFF)<<16 ) );
}
static CDATA CPROC VidlibProxy_SetAlphaValue( CDATA color, COLOR_CHANNEL a )
{
	return ( ((color)&0xFFFFFF) | ( (a)<<24 ) );
}

static COLOR_CHANNEL CPROC VidlibProxy_GetRedValue( CDATA color )
{
	return (color & 0xFF ) >> 0;
}

static COLOR_CHANNEL CPROC VidlibProxy_GetGreenValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0xFF00 ) >> 8);
}

static COLOR_CHANNEL CPROC VidlibProxy_GetBlueValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0x00FF0000 ) >> 16);
}

static COLOR_CHANNEL CPROC VidlibProxy_GetAlphaValue( CDATA color )
{
	return (COLOR_CHANNEL)((color & 0xFF000000 ) >> 24);
}

static CDATA CPROC VidlibProxy_MakeAlphaColor( COLOR_CHANNEL r, COLOR_CHANNEL grn, COLOR_CHANNEL b, COLOR_CHANNEL alpha )
{
#  ifdef _WIN64
#	 define _AND_FF &0xFF
#  else
/* This is a macro to cure a 64bit warning in visual studio. */
#	 define _AND_FF
#  endif
#define _AColor( r,g,b,a ) (((_32)( ((_8)((b)_AND_FF))|((_16)((_8)((g))_AND_FF)<<8))|(((_32)((_8)((r))_AND_FF)<<16)))|(((a)_AND_FF)<<24))
	return _AColor( r, grn, b, alpha );
}

static CDATA CPROC VidlibProxy_MakeColor( COLOR_CHANNEL r, COLOR_CHANNEL grn, COLOR_CHANNEL b )
{
	return VidlibProxy_MakeAlphaColor( r,grn,b, 0xFF );
}

static LOGICAL CPROC VidlibProxy_IsImageTargetFinal( Image image )
{
	if( image )
		return ( ((PVPImage)image)->render_id != INVALID_INDEX );
	return 0;
}

// new image instance, which is the same as the old...
// need to hold image memory
static Image CPROC VidlibProxy_ReuseImage( Image pif )
{
	PVPImage image = (PVPImage)pif;
	if( !image )
		return pif;

	if( !ThreadNetworkState.flags.during_connect )
	{
		lprintf( WIDE("Cannot reuse images outside of on display connect") );
		return pif;
	}

	if( !image->parent )
	{
		image->image->flags |= IF_FLAG_UPDATED;
		SetLink( &ThreadNetworkState.app->application_instance->images, image->id, image );
		SendClientMessage( PMID_MakeImage, image, image->render_id, (image->render_id==INVALID_INDEX)?TRUE:FALSE );
		return pif;
	}
	else
	{
		PVPImage id_parent = (PVPImage)GetLink( &ThreadNetworkState.app->application_instance->images, image->parent->id ) ;
		if( !id_parent )
		{
			id_parent = (PVPImage)VidlibProxy_ReuseImage( (Image)image->parent );
		}
		SetLink( &ThreadNetworkState.app->application_instance->images, image->id, image );
		SendClientMessage( PMID_MakeSubImage, image );
		return pif;
	}
}

static void InitImageInterface( void )
{
	InstanceProxyImageInterface._GetRedValue = VidlibProxy_GetRedValue;
	InstanceProxyImageInterface._GetGreenValue = VidlibProxy_GetGreenValue;
	InstanceProxyImageInterface._GetBlueValue = VidlibProxy_GetBlueValue;
	InstanceProxyImageInterface._GetAlphaValue = VidlibProxy_GetAlphaValue;
	InstanceProxyImageInterface._SetRedValue = VidlibProxy_SetRedValue;
	InstanceProxyImageInterface._SetGreenValue = VidlibProxy_SetGreenValue;
	InstanceProxyImageInterface._SetBlueValue = VidlibProxy_SetBlueValue;
	InstanceProxyImageInterface._SetAlphaValue = VidlibProxy_SetAlphaValue;
	InstanceProxyImageInterface._MakeColor = VidlibProxy_MakeColor;
	InstanceProxyImageInterface._MakeAlphaColor = VidlibProxy_MakeAlphaColor;
	InstanceProxyImageInterface._LoadImageFileFromGroupEx = VidlibProxy_LoadImageFileFromGroupEx;
	InstanceProxyImageInterface._GetStringSizeFontEx = VidlibProxy_GetStringSizeFontEx;
	InstanceProxyImageInterface._GetFontHeight = VidlibProxy_GetFontHeight;
	InstanceProxyImageInterface._OrphanSubImage = VidlibProxy_OrphanSubImage;
	InstanceProxyImageInterface._AdoptSubImage = VidlibProxy_AdoptSubImage;
	InstanceProxyImageInterface._TransferSubImages = VidlibProxy_TransferSubImages;
	InstanceProxyImageInterface._MarkImageDirty = VidlibProxy_MarkImageDirty;
	InstanceProxyImageInterface._GetNativeImage = VidlibProxy_GetNativeImage;
	InstanceProxyImageInterface._IsImageTargetFinal = VidlibProxy_IsImageTargetFinal;

	// ============= FONT Support ============================
	// these should go through real_interface
	InstanceProxyImageInterface._RenderFontFileScaledEx = l.real_interface->_RenderFontFileScaledEx;
	InstanceProxyImageInterface._RenderScaledFont = l.real_interface->_RenderScaledFont;
	InstanceProxyImageInterface._RenderScaledFontData = l.real_interface->_RenderScaledFontData;
	InstanceProxyImageInterface._RerenderFont = l.real_interface->_RerenderFont;
	InstanceProxyImageInterface._RenderScaledFontEx = l.real_interface->_RenderScaledFontEx;
	InstanceProxyImageInterface._DumpFontCache = l.real_interface->_DumpFontCache;
	InstanceProxyImageInterface._DumpFontFile = l.real_interface->_DumpFontFile;

	InstanceProxyImageInterface._GetFontHeight = l.real_interface->_GetFontHeight;

	InstanceProxyImageInterface._GetFontRenderData = l.real_interface->_GetFontRenderData;
	InstanceProxyImageInterface._GetStringRenderSizeFontEx = l.real_interface->_GetStringRenderSizeFontEx;
	InstanceProxyImageInterface._GetStringSizeFontEx = l.real_interface->_GetStringSizeFontEx;

	// this is part of the old interface; and isn't used anymore
	//InstanceProxyImageInterface._UnloadFont = l.real_interface->_UnloadFont;
	InstanceProxyImageInterface._DestroyFont = l.real_interface->_DestroyFont;
	InstanceProxyImageInterface._global_font_data = l.real_interface->_global_font_data;
	InstanceProxyImageInterface._GetGlobalFonts = l.real_interface->_GetGlobalFonts;
	InstanceProxyImageInterface._InternalRenderFontFile = l.real_interface->_InternalRenderFontFile;
	InstanceProxyImageInterface._InternalRenderFont = l.real_interface->_InternalRenderFont;
	InstanceProxyImageInterface._ReuseImage = VidlibProxy_ReuseImage;
	InstanceProxyImageInterface._PutStringFontExx = VidlibProxy_PutStringFontExx;
	InstanceProxyImageInterface._ResetImageBuffers = (void(*)(Image,LOGICAL))ClearImageBuffers;
}

static IMAGE_3D_INTERFACE Proxy3dImageInterface = {
	NULL
};

static POINTER CPROC GetProxyDisplayInterface( void )
{
	// open server socket
	return &ProxyInterface;
}
static void CPROC DropProxyDisplayInterface( POINTER i )
{
	// close connections
}
static POINTER CPROC Get3dProxyDisplayInterface( void )
{
	// open server socket
	return &Proxy3dInterface;
}
static void CPROC Drop3dProxyDisplayInterface( POINTER i )
{
	// close connections
}

static POINTER CPROC GetInstanceProxyImageInterface( void )
{
	// open server socket
	return &InstanceProxyImageInterface;
}
static void CPROC DropInstanceProxyImageInterface( POINTER i )
{
	// close connections
}
static POINTER CPROC Get3dInstanceProxyImageInterface( void )
{
	// open server socket
	return &Proxy3dImageInterface;
}
static void CPROC Drop3dInstanceProxyImageInterface( POINTER i )
{
	// close connections
}

PRIORITY_PRELOAD( RegisterProxyInterface, VIDLIB_PRELOAD_PRIORITY )
{
	InitializeCriticalSec( &l.message_formatter );
	RegisterInterface( WIDE( "sack.image.proxy.instance.server" ), GetInstanceProxyImageInterface, DropInstanceProxyImageInterface );
	RegisterInterface( WIDE( "sack.image.3d.proxy.instance.server" ), Get3dInstanceProxyImageInterface, Drop3dInstanceProxyImageInterface );
	RegisterInterface( WIDE( "sack.render.proxy.instance.server" ), GetProxyDisplayInterface, DropProxyDisplayInterface );
	RegisterInterface( WIDE( "sack.render.3d.proxy.instance.server" ), Get3dProxyDisplayInterface, Drop3dProxyDisplayInterface );
#ifdef _WIN32
	LoadFunction( WIDE("bag.image.dll"), NULL );
#endif
	l.real_interface = (PIMAGE_INTERFACE)GetInterface( WIDE( "sack.image" ) );

	//InitProxyInterface();
	// needs sack.image loaded before; fonts are passed to this
	InitImageInterface();

	// wanted to delay-init; until a renderer is actually open..
	InitService();

	// have to init all of the reply message formats;
	// sends will be initialized on-demand
	//WebSockInitReplyJson( PMID_Reply_OpenDisplayAboveUnderSizedAt );
	WebSockInitReplyJson( PMID_ClientSessionId );
	WebSockInitReplyJson( PMID_Event_Mouse );
	WebSockInitReplyJson( PMID_Event_Flush_Finished );
	WebSockInitReplyJson( PMID_Event_Key );
}


