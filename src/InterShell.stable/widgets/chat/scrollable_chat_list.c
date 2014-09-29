#ifndef FORCE_NO_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#endif
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define CHAT_CONTROL_MAIN_SOURCE
#include <stdhdrs.h>
#include <controls.h>
#include <psi.h>
#include <sqlgetoption.h>
#include <psi/console.h> // text formatter

#include "../include/buttons.h"

#include "../../intershell_registry.h"
#include "../../intershell_export.h"

#include "chat_control.h" 

#define CONTROL_NAME WIDE("Scrollable Message List")

#define INTERSHELL_CONTROL_NAME WIDE("Intershell/test/Scrollable Message List")

typedef struct chat_time_tag
{
	_8 hr,mn,sc;
	_8 mo,dy;
	_16 year;
} CHAT_TIME;
typedef struct chat_time_tag *PCHAT_TIME;


typedef struct chat_message_tag
{
	CHAT_TIME received_time; // the time the message was received
	CHAT_TIME sent_time; // the time the message was sent
	CTEXTSTR text;
	TEXTSTR formatted_text;
	size_t formatted_text_len;
	int formatted_height;
	int message_y;
	LOGICAL sent; // if not sent, is received message - determine justification and decoration
} CHAT_MESSAGE;
typedef struct chat_message_tag *PCHAT_MESSAGE;



enum {
	SEGMENT_TOP_LEFT
	  , SEGMENT_TOP
	  , SEGMENT_TOP_RIGHT
	  , SEGMENT_LEFT
	  , SEGMENT_CENTER
	  , SEGMENT_RIGHT
	  , SEGMENT_BOTTOM_LEFT
	  , SEGMENT_BOTTOM
	  , SEGMENT_BOTTOM_RIGHT
	  // border segment index's
};


EasyRegisterControlWithBorder( CONTROL_NAME, sizeof( PCHAT_LIST ), BORDER_NONE );

PRELOAD( GetInterfaces )
{
	l.pii = GetImageInterface();
	l.pdi = GetDisplayInterface();
}


static void ChopDecorations( void )
{
	{
		if( l.decoration )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;
			_32 BorderWidth = l.received.div_x1 - l.received.back_x;
			_32 BorderWidth2 = ( l.received.back_x  + l.received.back_w ) - l.received.div_x2;
			_32 BorderHeight = l.received.div_y1 - l.received.back_y;

			MiddleSegmentWidth = l.received.div_x2 - l.received.div_x1;
			MiddleSegmentHeight = l.received.div_y2 - l.received.div_y1;
			l.received.BorderSegment[SEGMENT_TOP_LEFT] = MakeSubImage( l.decoration
																				  , l.received.back_x, l.received.back_y
																				  , BorderWidth, BorderHeight );
			l.received.BorderSegment[SEGMENT_TOP] = MakeSubImage( l.decoration
																			, l.received.back_x + BorderWidth, l.received.back_y
																			, MiddleSegmentWidth, BorderHeight );
			l.received.BorderSegment[SEGMENT_TOP_RIGHT] = MakeSubImage( l.decoration
																					, l.received.back_x + BorderWidth + MiddleSegmentWidth, l.received.back_y + 0
																					, BorderWidth2, BorderHeight );
			//-------------------
			// middle row segments
			l.received.BorderSegment[SEGMENT_LEFT] = MakeSubImage( l.decoration
																	  , l.received.back_x + 0, BorderHeight
																	  , BorderWidth, MiddleSegmentHeight );
			l.received.BorderSegment[SEGMENT_CENTER] = MakeSubImage( l.decoration
																		 , l.received.back_x + BorderWidth, BorderHeight
																		 , MiddleSegmentWidth, MiddleSegmentHeight );
			l.received.BorderSegment[SEGMENT_RIGHT] = MakeSubImage( l.decoration
																		, l.received.back_x + BorderWidth + MiddleSegmentWidth, BorderHeight
																		, BorderWidth2, MiddleSegmentHeight );
			//-------------------
			// lower segments
			BorderHeight = l.received.back_h - (l.received.div_y2 - l.received.back_y);
			l.received.BorderSegment[SEGMENT_BOTTOM_LEFT] = MakeSubImage( l.decoration
																				, l.received.back_x + 0, l.received.back_y + BorderHeight + MiddleSegmentHeight
																				, BorderWidth, BorderHeight );
			l.received.BorderSegment[SEGMENT_BOTTOM] = MakeSubImage( l.decoration
																		 , l.received.back_x + BorderWidth, l.received.back_y + BorderHeight + MiddleSegmentHeight
																	 , MiddleSegmentWidth, BorderHeight );
			l.received.BorderSegment[SEGMENT_BOTTOM_RIGHT] = MakeSubImage( l.decoration
																				 , l.received.back_x + BorderWidth + MiddleSegmentWidth, l.received.back_y + BorderHeight + MiddleSegmentHeight
																						, BorderWidth2, BorderHeight );
			l.received.arrow = MakeSubImage( l.decoration, l.received.arrow_x, l.received.arrow_y, l.received.arrow_w, l.received.arrow_h );
		}
		if( l.decoration )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;
			_32 BorderWidth = l.sent.div_x1 - l.sent.back_x;
			_32 BorderWidth2 = l.sent.back_h - ( l.sent.div_y2 - l.sent.back_y );
			_32 BorderHeight = l.sent.div_y1 - l.sent.back_y;

			MiddleSegmentWidth = l.sent.div_x2 - l.sent.div_x1;
			MiddleSegmentHeight = l.sent.div_y2 - l.sent.div_y1;
			l.sent.BorderSegment[SEGMENT_TOP_LEFT] = MakeSubImage( l.decoration
																				  , l.sent.back_x, l.sent.back_y
																				  , BorderWidth, BorderHeight );
			l.sent.BorderSegment[SEGMENT_TOP] = MakeSubImage( l.decoration
																			, l.sent.back_x + BorderWidth, l.sent.back_y
																			, MiddleSegmentWidth, BorderHeight );
			l.sent.BorderSegment[SEGMENT_TOP_RIGHT] = MakeSubImage( l.decoration
																					, l.sent.back_x + BorderWidth + MiddleSegmentWidth, l.sent.back_y + 0
																					, BorderWidth2, BorderHeight );
			//-------------------
			// middle row segments
			l.sent.BorderSegment[SEGMENT_LEFT] = MakeSubImage( l.decoration
																	  , l.sent.back_x + 0, BorderHeight
																	  , BorderWidth, MiddleSegmentHeight );
			l.sent.BorderSegment[SEGMENT_CENTER] = MakeSubImage( l.decoration
																		 , l.sent.back_x + BorderWidth, BorderHeight
																		 , MiddleSegmentWidth, MiddleSegmentHeight );
			l.sent.BorderSegment[SEGMENT_RIGHT] = MakeSubImage( l.decoration
																		, l.sent.back_x + BorderWidth + MiddleSegmentWidth, BorderHeight
																		, BorderWidth2, MiddleSegmentHeight );
			//-------------------
			// lower segments
			BorderHeight = l.sent.back_h - (l.sent.div_y2 - l.sent.back_y);
			l.sent.BorderSegment[SEGMENT_BOTTOM_LEFT] = MakeSubImage( l.decoration
																				, l.sent.back_x + 0, l.sent.back_y + BorderHeight + MiddleSegmentHeight
																				, BorderWidth, BorderHeight );
			l.sent.BorderSegment[SEGMENT_BOTTOM] = MakeSubImage( l.decoration
																		 , l.sent.back_x + BorderWidth, l.sent.back_y + BorderHeight + MiddleSegmentHeight
																	 , MiddleSegmentWidth, BorderHeight );
			l.sent.BorderSegment[SEGMENT_BOTTOM_RIGHT] = MakeSubImage( l.decoration
																				 , l.sent.back_x + BorderWidth + MiddleSegmentWidth, l.sent.back_y + BorderHeight + MiddleSegmentHeight
																				 , BorderWidth2, BorderHeight );
			l.sent.arrow = MakeSubImage( l.decoration, l.sent.arrow_x, l.sent.arrow_y, l.sent.arrow_w, l.sent.arrow_h );
		}
	}
}

static PTRSZVAL CPROC SetBackgroundImage( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, image_name );
	l.decoration_name = StrDup( image_name );
	l.decoration = LoadImageFileFromGroup( 0, image_name );
	return psv;
}

static PTRSZVAL CPROC SetSentArrowArea( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	PARAM( args, S_64, w );
	PARAM( args, S_64, h );
	l.sent.arrow_x = (S_32)x;
	l.sent.arrow_y = (S_32)y;
	l.sent.arrow_w = (_32)w;
	l.sent.arrow_h = (_32)h;
	return psv;
}

static PTRSZVAL CPROC SetReceiveArrowArea( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	PARAM( args, S_64, w );
	PARAM( args, S_64, h );
	l.received.arrow_x = (S_32)x;
	l.received.arrow_y = (S_32)y;
	l.received.arrow_w = (_32)w;
	l.received.arrow_h = (_32)h;
	return psv;
}

static PTRSZVAL CPROC SetSentBackgroundArea( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	PARAM( args, S_64, w );
	PARAM( args, S_64, h );
	l.sent.back_x = (S_32)x;
	l.sent.back_y = (S_32)y;
	l.sent.back_w = (_32)w;
	l.sent.back_h = (_32)h;
	return psv;
}

static PTRSZVAL CPROC SetReceiveBackgroundArea( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	PARAM( args, S_64, w );
	PARAM( args, S_64, h );
	l.received.back_x = (S_32)x;
	l.received.back_y = (S_32)y;
	l.received.back_w = (_32)w;
	l.received.back_h = (_32)h;
	return psv;
}

static PTRSZVAL CPROC SetSentBackgroundDividers( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x1 );
	PARAM( args, S_64, x2 );
	PARAM( args, S_64, y1 );
	PARAM( args, S_64, y2 );
	l.sent.div_x1 = (S_32)x1;
	l.sent.div_x2 = (S_32)x2;
	l.sent.div_y1 = (S_32)y1;
	l.sent.div_y2 = (S_32)y2;
	return psv;
}

static PTRSZVAL CPROC SetReceiveBackgroundDividers( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x1 );
	PARAM( args, S_64, x2 );
	PARAM( args, S_64, y1 );
	PARAM( args, S_64, y2 );
	l.received.div_x1 = (S_32)x1;
	l.received.div_x2 = (S_32)x2;
	l.received.div_y1 = (S_32)y1;
	l.received.div_y2 = (S_32)y2;
	return psv;
}

static PTRSZVAL CPROC SetReceiveJustification( PTRSZVAL psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, S_64, justify );
	l.flags.received_justification = (BIT_FIELD)justify;
	return psv;
}

static PTRSZVAL CPROC SetSentJustification( PTRSZVAL psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, S_64, justify );
	l.flags.sent_justification = (BIT_FIELD)justify;

	return psv;
}

static PTRSZVAL CPROC SetReceiveTextJustification( PTRSZVAL psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, S_64, justify );
	l.flags.received_text_justification = (BIT_FIELD)justify;
	return psv;
}

static PTRSZVAL CPROC SetSentTextJustification( PTRSZVAL psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, S_64, justify );
	l.flags.sent_text_justification = (BIT_FIELD)justify;

	return psv;
}

static PTRSZVAL CPROC SetReceiveArrowOffset( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	l.received.arrow_x_offset = x;
	l.received.arrow_y_offset = y;
	return psv;
}

static PTRSZVAL CPROC SetSentArrowOffset( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	l.sent.arrow_x_offset = x;
	l.sent.arrow_y_offset = y;
	return psv;
}

static PTRSZVAL CPROC SetSidePad( PTRSZVAL psv, arg_list args )
{
	PARAM( args, S_64, pad );
	l.side_pad = pad;
	return psv;
}

void ScrollableChatControl_AddConfigurationMethods( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, "Chat Control Background Image=%m", SetBackgroundImage );
	AddConfigurationMethod( pch, "Chat Control Sent Arrow Area=%i,%i %i,%i", SetSentArrowArea );
	AddConfigurationMethod( pch, "Chat Control Received Arrow Area=%i,%i %i,%i", SetReceiveArrowArea );
	AddConfigurationMethod( pch, "Chat Control Sent Background Area=%i,%i %i,%i", SetSentBackgroundArea );
	AddConfigurationMethod( pch, "Chat Control Received Background Area=%i,%i %i,%i", SetReceiveBackgroundArea );
	AddConfigurationMethod( pch, "Chat Control Sent Background Dividers=%i,%i,%i,%i", SetSentBackgroundDividers );
	AddConfigurationMethod( pch, "Chat Control Received Background Dividers=%i,%i,%i,%i", SetReceiveBackgroundDividers );
	AddConfigurationMethod( pch, "Chat Control Sent Decoration Justification=%i", SetSentJustification );
	AddConfigurationMethod( pch, "Chat Control Received Decoration Justification=%i", SetReceiveJustification );
	AddConfigurationMethod( pch, "Chat Control Sent Text Justification=%i", SetSentTextJustification );
	AddConfigurationMethod( pch, "Chat Control Received Text Justification=%i", SetReceiveTextJustification );
	AddConfigurationMethod( pch, "Chat Control Side Pad = %i", SetSidePad );
	AddConfigurationMethod( pch, "Chat Control Sent Arrow Offset=%i,%i", SetSentArrowOffset );
	AddConfigurationMethod( pch, "Chat Control Received Arrow Offset=%i,%i", SetReceiveArrowOffset );
}


static void OnLoadCommon( WIDE( "Chat Control" ) )( PCONFIG_HANDLER pch )
{
	ScrollableChatControl_AddConfigurationMethods( pch );
}

static void SetupDefaultConfig( void )
{
	if( !l.decoration && !l.decoration_name )
	{
		l.decoration_name = WIDE("chat-decoration.png");
		l.decoration = LoadImageFile( l.decoration_name );

		l.side_pad = 5;
		l.sent.back_x = 0;
		l.sent.back_y = 0;
		l.sent.back_w = 73;
		l.sent.back_h = 76;
		l.sent.arrow_x = 0;
		l.sent.arrow_y = 84;
		l.sent.arrow_w = 10;
		l.sent.arrow_h = 8;
		l.sent.div_x1 = 10;
		l.sent.div_x2 = 10 + 54;
		l.sent.div_y1 = 10;
		l.sent.div_y2 = 9 + 57;
		l.sent.arrow_x_offset = -2;
		l.sent.arrow_y_offset = -10;
		l.flags.sent_justification = 0;
		l.flags.sent_text_justification = 1;
		l.received.back_x = 83;
		l.received.back_y = 0;
		l.received.back_w = 76;
		l.received.back_h = 77;
		l.received.arrow_x = 147;
		l.received.arrow_y = 86;
		l.received.arrow_w = 12;
		l.received.arrow_h = 7;
		l.received.arrow_x_offset = +2;
		l.received.arrow_y_offset = -10;
		l.received.div_x1 = 92;
		l.received.div_x2 = 92 + 52;
		l.received.div_y1 = 9;
		l.received.div_y2 = 9 + 59;
		l.flags.received_justification = 1;
		l.flags.received_text_justification = 0;
	}
	ChopDecorations( );
}

static void OnFinishInit( WIDE( "Chat Control" ) )( PSI_CONTROL canvas )
{
	SetupDefaultConfig();
}

static void OnSaveCommon( WIDE( "Chat Control" ) )( FILE *file )
{

	fprintf( file, "%sChat Control Sent Arrow Area=%d,%d %u,%u\n"
			 , InterShell_GetSaveIndent()
			 , l.sent.arrow_x
			 , l.sent.arrow_y
			 , l.sent.arrow_w
			 , l.sent.arrow_h );
	fprintf( file, "%sChat Control Sent Arrow Offset=%d,%d\n"
			 , InterShell_GetSaveIndent()
			 , l.sent.arrow_x_offset
			 , l.sent.arrow_y_offset );

	fprintf( file, "%sChat Control Received Arrow Area=%d,%d %u,%u\n"
			 , InterShell_GetSaveIndent()
			 , l.received.arrow_x
			 , l.received.arrow_y
			 , l.received.arrow_w
			 , l.received.arrow_h );
	fprintf( file, "%sChat Control Received Arrow Offset=%d,%d\n"
			 , InterShell_GetSaveIndent()
			 , l.received.arrow_x_offset
			 , l.received.arrow_y_offset );

	fprintf( file, "%sChat Control Sent Background Area=%d,%d %u,%u\n"
			 , InterShell_GetSaveIndent()
			 , l.sent.back_x
			 , l.sent.back_y
			 , l.sent.back_w
			 , l.sent.back_h );
	fprintf( file, "%sChat Control Received Background Area=%d,%d %u,%u\n"
			 , InterShell_GetSaveIndent()
			 , l.received.back_x
			 , l.received.back_y
			 , l.received.back_w
			 , l.received.back_h );
	fprintf( file, "%sChat Control Sent Background Dividers=%d,%d,%d,%d\n"
			 , InterShell_GetSaveIndent()
			 , l.sent.div_x1
			 , l.sent.div_x2
			 , l.sent.div_y1
			 , l.sent.div_y2 );
	fprintf( file, "%sChat Control Received Background Dividers=%d,%d,%d,%d\n"
			 , InterShell_GetSaveIndent()
			 , l.received.div_x1
			 , l.received.div_x2
			 , l.received.div_y1
			 , l.received.div_y2 );
   fprintf( file, "%sChat Control Background Image=%s\n"
			 , InterShell_GetSaveIndent()
			 , l.decoration_name );
   fprintf( file, "%sChat Control Side Pad=%d\n"
			 , InterShell_GetSaveIndent()
			 , l.side_pad );
   fprintf( file, "%sChat Control Received Decoration Justification=%d\n"
			 , InterShell_GetSaveIndent()
			, l.flags.received_justification );
   fprintf( file, "%sChat Control Sent Decoration Justification=%d\n"
			 , InterShell_GetSaveIndent()
			, l.flags.sent_justification );
   fprintf( file, "%sChat Control Received Text Justification=%d\n"
			 , InterShell_GetSaveIndent()
			, l.flags.received_text_justification );
   fprintf( file, "%sChat Control Sent Text Justification=%d\n"
			 , InterShell_GetSaveIndent()
			, l.flags.sent_text_justification );
}


void Chat_EnqueMessage( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , CTEXTSTR text )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	if( chat_control )
	{
		PCHAT_MESSAGE pcm = New( CHAT_MESSAGE );
		if( received_time )
			pcm->received_time = received_time[0];
		else
			MemSet( &pcm->received_time, 0, sizeof( pcm->received_time ) );
		if( sent_time )
			pcm->sent_time = sent_time[0];
		else
			MemSet( &pcm->sent_time, 0, sizeof( pcm->sent_time ) );
		pcm->text = StrDup( text );
		pcm->sent = sent;
		pcm->formatted_text = NULL;
		EnqueLink( &chat_control->messages, pcm );
	}
}

int MeasureFrameWidth( Image window, S_32 *left, S_32 *right, LOGICAL received, LOGICAL complete )
{
	if( received )
	{
		if( !complete )
		{
			(*left) = l.side_pad; // center? what is 2?
			(*right) = window->width - l.side_pad;
		}
		else if( l.flags.received_justification == 0 )
		{
			(*left) = l.side_pad;
			(*right) = window->width - ( l.side_pad + l.received.arrow_w ) - l.received.arrow_x_offset;
		}
		else if( l.flags.received_justification == 1 )
		{
			(*left) = l.side_pad + l.received.arrow_w - l.received.arrow_x_offset;
			(*right) = window->width - l.side_pad;
		}
		else if( l.flags.received_justification == 2 )
		{
			(*left) = 0; // center? what is 2?
			(*right) = window->width;
		}
	}
	else
	{
		if( !complete )
		{
			(*left) = l.side_pad; // center? what is 2?
			(*right) = window->width - l.side_pad;
		}
		else if( l.flags.sent_justification == 0 )
		{
			(*left) = l.side_pad - l.sent.arrow_x_offset;
			(*right) = window->width - ( l.side_pad + l.sent.arrow_w ) - l.sent.arrow_x_offset;
		}
		else if( l.flags.sent_justification == 1 )
		{
			(*left) = l.side_pad + l.sent.arrow_w - l.sent.arrow_x_offset;
			(*right) = window->width - l.side_pad - l.sent.arrow_x_offset;
		}
		else if( l.flags.sent_justification == 2 )
		{
			(*left) = 0; // center? what is 2?
			(*right) = window->width;
		}
	}
}

void DrawMessageFrame( Image window, int y, int height, LOGICAL received, LOGICAL complete )
{
	S_32 x_offset_left;
	S_32 x_offset_right;

	MeasureFrameWidth( window, &x_offset_left, &x_offset_right, received, complete );
	if( received )
	{
		BlotScaledImageSizedToAlpha( window, l.received.BorderSegment[SEGMENT_TOP]
											, x_offset_left + l.received.BorderSegment[SEGMENT_LEFT]->width
											, y
											, ( x_offset_right - x_offset_left ) - ( l.received.BorderSegment[SEGMENT_LEFT]->width
																	 + l.received.BorderSegment[SEGMENT_RIGHT]->width )
											, l.received.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.received.BorderSegment[SEGMENT_CENTER]
											, x_offset_left + l.received.BorderSegment[SEGMENT_LEFT]->width
											, y + l.received.BorderSegment[SEGMENT_TOP]->height
											, ( x_offset_right - x_offset_left ) - ( l.received.BorderSegment[SEGMENT_LEFT]->width
																	 + l.received.BorderSegment[SEGMENT_RIGHT]->width )
											, height - ( l.received.BorderSegment[SEGMENT_TOP]->height
															+ l.received.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.received.BorderSegment[SEGMENT_BOTTOM]
											, x_offset_left + l.received.BorderSegment[SEGMENT_LEFT]->width, y + height - l.received.BorderSegment[SEGMENT_BOTTOM]->height
											, ( x_offset_right - x_offset_left ) - ( l.received.BorderSegment[SEGMENT_LEFT]->width
																	 + l.received.BorderSegment[SEGMENT_RIGHT]->width )
											, l.received.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.received.BorderSegment[SEGMENT_LEFT]
											, x_offset_left 
											, y + l.received.BorderSegment[SEGMENT_TOP]->height
											, l.received.BorderSegment[SEGMENT_LEFT]->width
											, height - ( l.received.BorderSegment[SEGMENT_TOP]->height + l.received.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.received.BorderSegment[SEGMENT_RIGHT]
											, (x_offset_right )-( l.received.BorderSegment[SEGMENT_RIGHT]->width )
											, y + l.received.BorderSegment[SEGMENT_TOP]->height
											, l.received.BorderSegment[SEGMENT_RIGHT]->width
											, height - ( l.received.BorderSegment[SEGMENT_TOP]->height
															+ l.received.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.received.BorderSegment[SEGMENT_TOP_LEFT]
						  , x_offset_left , y
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.received.BorderSegment[SEGMENT_TOP_RIGHT]
							  , x_offset_right - ( l.received.BorderSegment[SEGMENT_RIGHT]->width )
							  , y
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.received.BorderSegment[SEGMENT_BOTTOM_LEFT]
						  , x_offset_left , y + height - l.received.BorderSegment[SEGMENT_BOTTOM]->height
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.received.BorderSegment[SEGMENT_BOTTOM_RIGHT]
						  , x_offset_right - ( l.received.BorderSegment[SEGMENT_RIGHT]->width )
						  , y + height - l.received.BorderSegment[SEGMENT_BOTTOM]->height
						  , ALPHA_TRANSPARENT );
		if( complete )
		{
			if( l.flags.received_justification == 0 )
				x_offset_left = window->width - l.received.arrow_w + l.received.arrow_x_offset;
			else if( l.flags.received_justification == 1 )
				x_offset_left = l.side_pad;
			else if( l.flags.received_justification == 2 )
				x_offset_left = 0; // center? what is 2?

			BlotImageAlpha( window, l.received.arrow
							  , x_offset_left
							  , l.received.arrow_y_offset + ( y + height ) - ( l.received.BorderSegment[SEGMENT_BOTTOM]->height + l.received.arrow_h )
							  , ALPHA_TRANSPARENT );
		}
	}
	else
	{
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_TOP]
											, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width, y
											, ( x_offset_right - x_offset_left ) - ( l.sent.BorderSegment[SEGMENT_LEFT]->width
																	 + l.sent.BorderSegment[SEGMENT_RIGHT]->width ) , l.sent.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_BOTTOM]
											, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width
												, y + height - l.sent.BorderSegment[SEGMENT_BOTTOM]->height
											, x_offset_right - x_offset_left -( 
																	  l.sent.BorderSegment[SEGMENT_LEFT]->width
																	 + l.sent.BorderSegment[SEGMENT_RIGHT]->width )
											, l.sent.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_LEFT]
											, x_offset_left
											, y + l.sent.BorderSegment[SEGMENT_TOP]->height
											, l.sent.BorderSegment[SEGMENT_LEFT]->width
											, height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_RIGHT]
											, x_offset_right - ( l.sent.BorderSegment[SEGMENT_RIGHT]->width  )
											, y + l.sent.BorderSegment[SEGMENT_TOP]->height
											, l.sent.BorderSegment[SEGMENT_RIGHT]->width
											, height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_CENTER]
											, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width
											, y + l.sent.BorderSegment[SEGMENT_TOP]->height
											, ( x_offset_right - x_offset_left ) - ( l.sent.BorderSegment[SEGMENT_LEFT]->width
																	 + l.sent.BorderSegment[SEGMENT_RIGHT]->width )
											, height - ( l.sent.BorderSegment[SEGMENT_TOP]->height
															+ l.sent.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.sent.BorderSegment[SEGMENT_TOP_LEFT]
						  , x_offset_left, y
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.sent.BorderSegment[SEGMENT_TOP_RIGHT]
						  , x_offset_right - ( l.sent.BorderSegment[SEGMENT_RIGHT]->width ), y
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.sent.BorderSegment[SEGMENT_BOTTOM_LEFT]
						  , x_offset_left, y + height - l.sent.BorderSegment[SEGMENT_BOTTOM]->height
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.sent.BorderSegment[SEGMENT_BOTTOM_RIGHT]
						  , x_offset_right - ( l.sent.BorderSegment[SEGMENT_RIGHT]->width )
						  , y + height - l.sent.BorderSegment[SEGMENT_BOTTOM]->height
						  , ALPHA_TRANSPARENT );
		if( complete )
		{
			if( l.flags.sent_justification == 0 )
				x_offset_left = l.sent.arrow_x_offset + window->width - ( l.side_pad + l.sent.arrow_w );
			else if( l.flags.sent_justification == 1 )
				x_offset_left = l.side_pad + l.sent.arrow_x_offset;
			else if( l.flags.sent_justification == 2 )
				x_offset_left = 0; // center? what is 2?
			BlotImageAlpha( window, l.sent.arrow
							  , x_offset_left
							  , l.sent.arrow_y_offset + ( y + height ) - ( l.sent.BorderSegment[SEGMENT_BOTTOM]->height + l.sent.arrow_h )
							  //, l.sent.arrow->width, l.sent.arrow->height
							  , ALPHA_TRANSPARENT );
		}
	}
}



void DrawAMessage( Image window, PCHAT_LIST list, PCHAT_MESSAGE msg )
{
	_32 width ;
	S_32 x_offset_left, x_offset_right;	
	S_32 frame_height;

	MeasureFrameWidth( window, &x_offset_left, &x_offset_right, !msg->sent, TRUE );
	if( msg->sent )
		width = ( x_offset_right - x_offset_left ) 
		- ( l.sent.BorderSegment[SEGMENT_LEFT]->width 
			+ l.sent.BorderSegment[SEGMENT_RIGHT]->width ) ;
	else
		width = ( x_offset_right - x_offset_left ) 
		- ( l.received.BorderSegment[SEGMENT_LEFT]->width 
			+ l.received.BorderSegment[SEGMENT_RIGHT]->width ) ;
	
	if( !msg->formatted_text )
	{
		int max_width = width;
		int max_height = 9999;
		FormatTextToBlockEx( msg->text, &msg->formatted_text, &max_width, &max_height, GetDefaultFont() );
		msg->formatted_text_len = StrLen( msg->formatted_text );
		msg->formatted_height = max_height;
		frame_height = msg->formatted_height + l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height ;
		msg->message_y = ( l.side_pad + frame_height );
	}
	else
		frame_height = msg->formatted_height + l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height ;

	list->display.message_top -= msg->message_y;

	DrawMessageFrame( window, list->display.message_top, frame_height, !msg->sent, 1 );
	if( msg->sent )
	{
		PutStringFontEx( window, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width
						, list->display.message_top + l.received.BorderSegment[SEGMENT_TOP]->height
						, BASE_COLOR_BLACK, 0
						, msg->formatted_text, msg->formatted_text_len, NULL );
	}
	else
	{
		PutStringFontEx( window, x_offset_left + l.received.BorderSegment[SEGMENT_LEFT]->width
						, list->display.message_top + l.received.BorderSegment[SEGMENT_TOP]->height
						, BASE_COLOR_BLACK, 0
						, msg->formatted_text, msg->formatted_text_len, NULL );
	}
}

static void ReformatMessages( PCHAT_LIST list )
{
	int message_idx;
	PCHAT_MESSAGE msg;
	for( message_idx = -1; msg = PeekQueueEx( list->messages, message_idx ); message_idx-- )
	{
		Deallocate( TEXTSTR, msg->formatted_text );
		msg->formatted_text = NULL;
	}
}

static void DrawMessages( PCHAT_LIST list, Image window )
{
	int message_idx;
	PCHAT_MESSAGE msg;
	for( message_idx = -1; msg = PeekQueueEx( list->messages, message_idx ); message_idx-- )
	{
		if( msg->formatted_text && 
			 ( ( list->display.message_top - msg->message_y ) > window->height ) )
		{
			list->display.message_top -= msg->message_y;
			continue;
		}
		if( !msg->formatted_text || 
			 ( ( ( list->display.message_top - msg->message_y ) < window->height )
			&& (list->display.message_top > l.side_pad ) ) )
			DrawAMessage( window, list, msg );
		if( list->display.message_top < l.side_pad )
			break;
	}
}


static int OnDrawCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	Image window = GetControlSurface( pc );
	int lines, skip_lines;

	if( !list )
	{
		ClearImageTo( window, BASE_COLOR_BLUE );
		return 1;
	}
	if( list->colors.background_color )
		ClearImageTo( window, list->colors.background_color);

	if( l.decoration )
	{
		int command_height;
		skip_lines = 0;
		lines = CountDisplayedLines( list->phb_Input );
		if( !lines )
			lines = 1;
		if( lines > 3 )
		{
			skip_lines = lines - 3;
			lines = 3;
		}
		list->nFontHeight = GetFontHeight( GetCommonFont( pc )  );
		list->command_height = list->nFontHeight * lines 
			+ l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
			
		DrawMessageFrame( window
			, window->height - ( list->command_height + l.side_pad )
			, list->command_height
			, 0
			, 0 );


		{
			int nLine;
			DISPLAYED_LINE *pCurrentLine;
			PDATALIST *ppCurrentLineInfo;
			ppCurrentLineInfo = GetDisplayInfo( list->phb_Input );
			for( nLine = 0; nLine < 3; nLine ++ )
			{
				pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
				if( !pCurrentLine )
				{
		#ifdef DEBUG_HISTORY_RENDER
					lprintf( WIDE("No such line... %d"), nLine );
		#endif
					break;
				}
				if( pCurrentLine->start )
				{
					RECT upd;
					RenderTextLine( list, window, pCurrentLine, &upd
						, nLine
						, GetCommonFont( pc )
						, window->height - ( l.side_pad + (l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) + list->nFontHeight )
						, l.side_pad + l.sent.BorderSegment[SEGMENT_TOP]->height
						, l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width
						, FALSE
						, FALSE );  // cursor; to know where to draw the mark...
				}
			}
		}
	}

	if( l.decoration )
	{

		ResizeImage( list->message_window, window->width, window->height - ( list->command_height + l.side_pad * 2 /* above and below input*/ ) );
		list->display.message_top = list->message_window->height + list->control_offset;
		DrawMessages( list, list->message_window );
	}
	return 1;
}

static int OnMouseCommon( CONTROL_NAME )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	if( !list )
		return 1;
	if( MAKE_FIRSTBUTTON( b, list->_b ) )
	{
		list->flags.begin_button = 1;
		list->first_x = x; 
		list->first_y = y; 
		list->flags.checked_drag = 0;
		list->flags.long_vertical_drag = 0;
		list->flags.long_horizontal_drag = 0;
	}
	else if( BREAK_LASTBUTTON( b, list->_b ) )
	{
		if( !list->flags.checked_drag )
		{
			// did not find a drag motion while the button was down... this is a tap
			
		}

	}
	else if( MAKE_SOMEBUTTONS( b ) )
	{
		if( !list->flags.checked_drag )
		{
			// some buttons still down  ... delta Y is more than 4 pixels
			if( ( y - list->first_y )*( y - list->first_y ) > 16 )
			{
				//begin dragging
				list->flags.long_vertical_drag = 1;
				list->flags.checked_drag = 1;
			}
			// some buttons still down  ... delta X is more than 40 pixels
			if( ( x - list->first_x )*( x - list->first_x ) > 1600 )
			{
				//begin dragging
				list->flags.long_horizontal_drag = 1;
				list->flags.checked_drag = 1;
			}
		}
		else
		{
			if( list->flags.long_vertical_drag )
			{
				list->control_offset += ( y - list->first_y );
				if( list->control_offset < 0 )
					list->control_offset = 0;
				list->first_y = y;
				SmudgeCommon( pc );
			}
		}
	}
	else // this is no buttons, and is continued no buttons (not last button either)
	{
	}
	list->_b = b;
	return 1;
}

static void CPROC PSIMeasureString( PTRSZVAL psv, CTEXTSTR s, int nShow, _32 *w, _32 *h )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	SFTFont font = GetCommonFont( pc );
	list->nFontHeight = GetFontHeight( font );
	GetStringSizeFontEx( s, nShow, w, h, font );
}

static int OnCreateCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list;
	SetupDefaultConfig();
	(*ppList) = New( CHAT_LIST );
	MemSet( (*ppList), 0, sizeof( CHAT_LIST ) );
	list = (*ppList);
	//list->colors.background_color = BASE_COLOR_WHITE;
	list->message_window = MakeSubImage( GetControlSurface( pc ), 0, 0, 1, 1 );
		list->pHistory = PSI_CreateHistoryRegion();
		list->phlc_Input = PSI_CreateHistoryCursor( list->pHistory );
		list->phb_Input = PSI_CreateHistoryBrowser( list->pHistory, PSIMeasureString, (PTRSZVAL)pc );
		list->CommandInfo = CreateUserInputBuffer();
		SetBrowserLines( list->phb_Input, 3 );
		list->colors.crText = BASE_COLOR_BLACK;

	Chat_EnqueMessage( pc, 0, NULL, NULL, "1) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 0, NULL, NULL, "2) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 1, NULL, NULL, "3) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 0, NULL, NULL, "4) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 1, NULL, NULL, "5) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 0, NULL, NULL, "(no67) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 1, NULL, NULL, "8) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 1, NULL, NULL, "9) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	Chat_EnqueMessage( pc, 0, NULL, NULL, "10) some test text here\nwith full support of color and\n inline fonts?\n... Need a very very long line... mary had a little lamb its fleece was white as snow..." );
	return 1;
}

static PTRSZVAL OnCreateControl( INTERSHELL_CONTROL_NAME )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc = MakeNamedControl( parent, CONTROL_NAME, x, y, w,h, -1 );

	return (PTRSZVAL)pc;
}

static PSI_CONTROL OnGetControl( INTERSHELL_CONTROL_NAME )( PTRSZVAL psv )
{
	return (PSI_CONTROL)psv;
}

static void OnSizeCommon( CONTROL_NAME )( PSI_CONTROL pc, LOGICAL begin_sizing )
{
	if( !begin_sizing )
	{
		PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
		PCHAT_LIST list = (*ppList);
		// get a size during create... and data is not inited into teh control yet.
		if( list )
			ReformatMessages( list );
	}
}



static int OnKeyCommon( CONTROL_NAME )( PSI_CONTROL pc, _32 key )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	//PCONSOLE_INFO list = (PCONSOLE_INFO)GetCommonUserData( pc );
	// this must here gather keystrokes and pass them forward into the
	// opened sentience...
	if( list )
	{
		TEXTCHAR character = GetKeyText( key );
		DECLTEXT( stroke, WIDE(" ") ); // single character ...
//cpg27dec2006 list\psicon.c(232): Warning! W202: Symbol 'bOutput' has been defined, but not referenced
//cpg27dec2006 		int bOutput = FALSE;
		//Log1( "Key: %08x", key );
//cpg27dec2006 list\psicon.c(234): Warning! W202: Symbol 'mod' has been defined, but not referenced
//cpg27dec2006		int mod = KEYMOD_NORMAL;
		if( !list || !l.decoration ) // not a valid window handle/device path
			return 0;
		//EnterCriticalSec( &list->Lock );

		// here is where we evaluate the curent keystroke....
		if( character )
		{
			stroke.data.data[0] = character;
			stroke.data.size = 1;
		}
		else
			stroke.data.size = 0;
		{
			PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
			PCHAT_LIST chat_control = (*ppList);
			Image window = GetControlSurface( pc );
			list->phb_Input->nColumns = window->width - ( 2 * l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.BorderSegment[SEGMENT_RIGHT]->width );
			list->phb_Input->nWidth = window->width - ( 2 * l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.BorderSegment[SEGMENT_RIGHT]->width );
		}

		if( key & KEY_PRESSED )
		{
			Widget_KeyPressHandler( list, (_8)(KEY_CODE(key)&0xFF), (_8)KEY_MOD(key), (PTEXT)&stroke );
			SmudgeCommon( pc );
		}
		//LeaveCriticalSec( &list->Lock );
	}
	return 1;
}


