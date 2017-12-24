#ifndef FORCE_NO_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#endif
//#define DEBUG_TIME_FORMATTING 1
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define CHAT_CONTROL_MAIN_SOURCE
#define CHAT_CONTROL_SOURCE
#include <stdhdrs.h>
#include <controls.h>
#include <psi.h>
#include <sqlgetoption.h>
#include <translation.h>
#include <psi/console.h> // text formatter

#include "../include/buttons.h"

#include "../../intershell_registry.h"
#include "../../intershell_export.h"

struct chat_widget_message;

// include public defs before internals...
#include "chat_control.h"
#include "chat_control_internal.h" 

#define CONTROL_NAME WIDE("Scrollable Message List")

#define INTERSHELL_CONTROL_NAME WIDE("Intershell/test/Scrollable Message List")

/*
typedef struct chat_time_tag
{
	uint8_t hr,mn,sc;
	uint8_t mo,dy;
	uint16_t year;
} CHAT_TIME;
typedef struct chat_time_tag *PCHAT_TIME;
*/



typedef struct chat_widget_message
{
	CHAT_TIME received_time; // the time the message was received
	CHAT_TIME sent_time; // the time the message was sent
	CHAT_TIME seen_time; // when the message was actually seen...
	LOGICAL seen; // logical whether to set seen_time or not
	uintptr_t psvSeen; // application handle of message
	TEXTSTR text;
	Image image;
	Image thumb_image;
	uint32_t thumb_image_left;
	TEXTSTR formatted_text;
	size_t formatted_text_len;
	int _formatted_height;
	int _message_y;
	LOGICAL _sent; // if not sent, is received message - determine justification and decoration
	TEXTCHAR formatted_time[256];
	int time_height;
	LOGICAL deleted;
} CHAT_MESSAGE;
typedef struct chat_widget_message *PCHAT_MESSAGE;

typedef struct chat_context_tag
{
	LOGICAL sent;
	Image sender_image;
	TEXTSTR sender;
	TEXTSTR formatted_sender;
	int sender_height;
	int sender_width;
	int formatted_height;  // has to include all message text and all tmiestamps and all image heights
	uint32_t max_width; // how wide the formatted message was
	uint32_t extra_width;
	//int message_y;   // a single value for this makes no sense.
	PLINKQUEUE messages; // queue of PCHAT_MESSAGE (search forward and backward ability)
	LOGICAL deleted;
} CHAT_CONTEXT;
typedef CHAT_CONTEXT *PCHAT_CONTEXT;

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

static const int month_len[]   = { 31, 28, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31 };
static const int month_total[] = {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

static int IsLeap( int year )
{
	return (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
}

static int GetDays( int year, int month )
{
	int leap = (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
	if( leap && month == 2 )
		return month_len[month-1] + 1;
	return month_len[month-1];
}

static int J2G( int year, int day_of_year )
{
	static const int month_len[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int leap = (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
	int day_of_month = day_of_year;
	int month;
	for (month = 0; month < 12; month ++) {
		int mlen = month_len[month];
		if (leap && month == 1)
			mlen ++;
		if (day_of_month <= mlen)
			break;
		day_of_month -= mlen;
	}
}

static int G2J( int year, int month, int day )
{
	int leap = (year % 4 == 0) && (year % 100 != 0 || year % 400 == 0);
	int day_of_year = 0;
	int extra = 0;
	int day_of_month = 0;
	if (leap && month > 2)
		extra = 1;
	return month_total[month-1] + day + extra;
}

static int64_t AbsoluteSeconds( PCHAT_TIME message_time )
{
	int now_day = G2J( message_time->yr, message_time->mo, message_time->dy );
	int yr;
	int msg_tick = message_time->hr * ( 60 * 60)
		 + message_time->mn * ( 60 )
		 + message_time->sc
		 - ( message_time->zhr * 60*60 + message_time->zmn * 60 )
		 ;
	for( yr = 2000; yr < message_time->yr; yr++ )
		now_day += IsLeap( yr )?366:365;

	return ((int64_t)now_day *24*60*60) + msg_tick;
}

// can track up to 4 timestamp deltas ....
static CTEXTSTR FormatMessageTime( CTEXTSTR time_type, PCHAT_TIME now, PCHAT_TIME message_time )
{
	static TEXTCHAR _timebuf[4][64];
	static int current_timebuf;
	TEXTCHAR *timebuf;
	int64_t absolute_day = AbsoluteSeconds( now );
	int64_t absolute_msg_day = AbsoluteSeconds( message_time );
	int64_t delta = absolute_day - absolute_msg_day;
	LOGICAL neg = (delta < 0 )? 1 : 0;
	if( neg )
		delta = -delta;
#if DEBUG_TIME_FORMATTING
	lprintf( "now is %" _64fs " msg is %" _64fs " delta %" _64fs 
				, absolute_day, absolute_msg_day, delta );
	lprintf( "now is %04d/%02d/%02d %02d:%02d:%02d %02d  %02d"
		, now->yr, now->mo, now->dy, now->hr, now->mn, now->sc, now->zhr, now->zmn );
	lprintf( "%s is %04d/%02d/%02d %02d:%02d:%02d %02d %02d"
		, time_type
		, message_time->yr, message_time->mo, message_time->dy
		, message_time->hr, message_time->mn, message_time->sc
		, message_time->zhr, message_time->zmn );
#endif
	timebuf = _timebuf[current_timebuf++];
	current_timebuf &= 3;

	if( delta >= ( 60*60*24 ) )
	{
		int part = delta / ( 60 * 60 * 24 );
		snprintf( timebuf, 64, "%s%d %s ago", neg?"-":"", part, part==1?"day":"days" );
	}
	else if( delta >= ( 60 * 60 ) )
	{
		int part = ( delta / ( 60 * 60 ) );
		snprintf( timebuf, 64, "%s%d %s ago"
			, neg?"-":""
			, part, part==1?" hour":" hours" );
	}
	else if( delta >= 60 ) {
		int part = delta / 60;
		snprintf( timebuf, 64, "%s%d%s ago", neg?"-":"", part, part==1?" minute":" minutes" );
	}	
	else if( delta )
		snprintf( timebuf, 64, "%s%d%s ago", neg?"-":"", (int)delta, delta==1?" second":" seconds" );
	else
		snprintf( timebuf, 64, "now" );
#if DEBUG_TIME_FORMATTING
	lprintf( "result is %s", timebuf );
#endif
	return timebuf;
}

static void ChopDecorations( void )
{
	{
		if( l.decoration && !l.received.BorderSegment[SEGMENT_TOP_LEFT] )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;
			uint32_t BorderWidth = l.received.div_x1 - l.received.back_x;
			uint32_t BorderWidth2 = ( l.received.back_x  + l.received.back_w ) - l.received.div_x2;
			uint32_t BorderHeight = l.received.div_y1 - l.received.back_y;

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
		else
		{
			int n;
			for( n = 0; n < 9; n++ )
				ReuseImage( l.received.BorderSegment[n] );
		}
		if( l.decoration && !l.send.BorderSegment[SEGMENT_TOP_LEFT] )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;
			uint32_t BorderWidth = l.send.div_x1 - l.send.back_x;
			uint32_t BorderWidth2 = ( l.send.back_x  + l.send.back_w ) - l.send.div_x2;
			uint32_t BorderHeight = l.send.div_y1 - l.send.back_y;

			MiddleSegmentWidth = l.send.div_x2 - l.send.div_x1;
			MiddleSegmentHeight = l.send.div_y2 - l.send.div_y1;
			l.send.BorderSegment[SEGMENT_TOP_LEFT] = MakeSubImage( l.decoration
																				  , l.send.back_x, l.send.back_y
																				  , BorderWidth, BorderHeight );
			l.send.BorderSegment[SEGMENT_TOP] = MakeSubImage( l.decoration
																			, l.send.back_x + BorderWidth, l.send.back_y
																			, MiddleSegmentWidth, BorderHeight );
			l.send.BorderSegment[SEGMENT_TOP_RIGHT] = MakeSubImage( l.decoration
																					, l.send.back_x + BorderWidth + MiddleSegmentWidth, l.send.back_y + 0
																					, BorderWidth2, BorderHeight );
			//-------------------
			// middle row segments
			l.send.BorderSegment[SEGMENT_LEFT] = MakeSubImage( l.decoration
																	  , l.send.back_x + 0, l.send.back_y + BorderHeight
																	  , BorderWidth, MiddleSegmentHeight );
			l.send.BorderSegment[SEGMENT_CENTER] = MakeSubImage( l.decoration
																		 , l.send.back_x + BorderWidth, l.send.back_y + BorderHeight
																		 , MiddleSegmentWidth, MiddleSegmentHeight );
			l.send.BorderSegment[SEGMENT_RIGHT] = MakeSubImage( l.decoration
																		, l.send.back_x + BorderWidth + MiddleSegmentWidth, l.send.back_y + BorderHeight
																		, BorderWidth2, MiddleSegmentHeight );
			//-------------------
			// lower segments
			BorderHeight = l.send.back_h - (l.send.div_y2 - l.send.back_y);
			l.send.BorderSegment[SEGMENT_BOTTOM_LEFT] = MakeSubImage( l.decoration
																				, l.send.back_x + 0, l.send.back_y + BorderHeight + MiddleSegmentHeight
																				, BorderWidth, l.send.back_h - ( BorderHeight + MiddleSegmentHeight ) );
			l.send.BorderSegment[SEGMENT_BOTTOM] = MakeSubImage( l.decoration
																		 , l.send.back_x + BorderWidth, l.send.back_y + BorderHeight + MiddleSegmentHeight
																	 , MiddleSegmentWidth, l.send.back_h - ( BorderHeight + MiddleSegmentHeight ) );
			l.send.BorderSegment[SEGMENT_BOTTOM_RIGHT] = MakeSubImage( l.decoration
																				 , l.send.back_x + BorderWidth + MiddleSegmentWidth, l.send.back_y + BorderHeight + MiddleSegmentHeight
																						, BorderWidth2, l.send.back_h - ( BorderHeight + MiddleSegmentHeight ) );
		}
		else
		{
			int n;
			for( n = 0; n < 9; n++ )
				ReuseImage( l.send.BorderSegment[n] );
		}
		if( l.decoration && !l.sent.BorderSegment[SEGMENT_TOP_LEFT] )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;
			uint32_t BorderWidth = l.sent.div_x1 - l.sent.back_x;
			uint32_t BorderWidth2 = l.sent.back_h - ( l.sent.div_y2 - l.sent.back_y );
			uint32_t BorderHeight = l.sent.div_y1 - l.sent.back_y;

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
		else
		{
			int n;
			for( n = 0; n < 9; n++ )
				ReuseImage( l.sent.BorderSegment[n] );
		}
	}
}

static uintptr_t CPROC SetBackgroundImage( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, image_name );
	l.decoration_name = StrDup( image_name );
	l.decoration = LoadImageFileFromGroup( 0, image_name );
	return psv;
}

static uintptr_t CPROC SetSentArrowArea( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	PARAM( args, int64_t, w );
	PARAM( args, int64_t, h );
	l.sent.arrow_x = (int32_t)x;
	l.sent.arrow_y = (int32_t)y;
	l.sent.arrow_w = (uint32_t)w;
	l.sent.arrow_h = (uint32_t)h;
	return psv;
}

static uintptr_t CPROC SetReceiveArrowArea( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	PARAM( args, int64_t, w );
	PARAM( args, int64_t, h );
	l.received.arrow_x = (int32_t)x;
	l.received.arrow_y = (int32_t)y;
	l.received.arrow_w = (uint32_t)w;
	l.received.arrow_h = (uint32_t)h;
	return psv;
}

static uintptr_t CPROC SetSentBackgroundArea( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	PARAM( args, int64_t, w );
	PARAM( args, int64_t, h );
	l.sent.back_x = (int32_t)x;
	l.sent.back_y = (int32_t)y;
	l.sent.back_w = (uint32_t)w;
	l.sent.back_h = (uint32_t)h;
	return psv;
}

static uintptr_t CPROC SetReceiveBackgroundArea( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	PARAM( args, int64_t, w );
	PARAM( args, int64_t, h );
	l.received.back_x = (int32_t)x;
	l.received.back_y = (int32_t)y;
	l.received.back_w = (uint32_t)w;
	l.received.back_h = (uint32_t)h;
	return psv;
}

static uintptr_t CPROC SetSentBackgroundDividers( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x1 );
	PARAM( args, int64_t, x2 );
	PARAM( args, int64_t, y1 );
	PARAM( args, int64_t, y2 );
	l.sent.div_x1 = (int32_t)x1;
	l.sent.div_x2 = (int32_t)x2;
	l.sent.div_y1 = (int32_t)y1;
	l.sent.div_y2 = (int32_t)y2;
	return psv;
}

static uintptr_t CPROC SetReceiveBackgroundDividers( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x1 );
	PARAM( args, int64_t, x2 );
	PARAM( args, int64_t, y1 );
	PARAM( args, int64_t, y2 );
	l.received.div_x1 = (int32_t)x1;
	l.received.div_x2 = (int32_t)x2;
	l.received.div_y1 = (int32_t)y1;
	l.received.div_y2 = (int32_t)y2;
	return psv;
}

static uintptr_t CPROC SetReceiveJustification( uintptr_t psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, int64_t, justify );
	l.flags.received_justification = (BIT_FIELD)justify;
	return psv;
}

static uintptr_t CPROC SetSentJustification( uintptr_t psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, int64_t, justify );
	l.flags.sent_justification = (BIT_FIELD)justify;

	return psv;
}

static uintptr_t CPROC SetReceiveTextJustification( uintptr_t psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, int64_t, justify );
	l.flags.received_text_justification = (BIT_FIELD)justify;
	return psv;
}

static uintptr_t CPROC SetSentTextJustification( uintptr_t psv, arg_list args )
{
	// 0 = left
	// 1 = right
	// 2 = center
	PARAM( args, int64_t, justify );
	l.flags.sent_text_justification = (BIT_FIELD)justify;

	return psv;
}

static uintptr_t CPROC SetReceiveArrowOffset( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	l.received.arrow_x_offset = (int32_t)x;
	l.received.arrow_y_offset = (int32_t)y;
	return psv;
}

static uintptr_t CPROC SetSentArrowOffset( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	l.sent.arrow_x_offset = (int32_t)x;
	l.sent.arrow_y_offset = (int32_t)y;
	return psv;
}

static uintptr_t CPROC SetSidePad( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, pad );
	l.side_pad = (int)pad;
	return psv;
}

void ScrollableChatControl_AddConfigurationMethods( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, WIDE("Chat Control Background Image=%m"), SetBackgroundImage );
	AddConfigurationMethod( pch, WIDE("Chat Control Sent Arrow Area=%i,%i %i,%i"), SetSentArrowArea );
	AddConfigurationMethod( pch, WIDE("Chat Control Received Arrow Area=%i,%i %i,%i"), SetReceiveArrowArea );
	AddConfigurationMethod( pch, WIDE("Chat Control Sent Background Area=%i,%i %i,%i"), SetSentBackgroundArea );
	AddConfigurationMethod( pch, WIDE("Chat Control Received Background Area=%i,%i %i,%i"), SetReceiveBackgroundArea );
	AddConfigurationMethod( pch, WIDE("Chat Control Sent Background Dividers=%i,%i,%i,%i"), SetSentBackgroundDividers );
	AddConfigurationMethod( pch, WIDE("Chat Control Received Background Dividers=%i,%i,%i,%i"), SetReceiveBackgroundDividers );
	AddConfigurationMethod( pch, WIDE("Chat Control Sent Decoration Justification=%i"), SetSentJustification );
	AddConfigurationMethod( pch, WIDE("Chat Control Received Decoration Justification=%i"), SetReceiveJustification );
	AddConfigurationMethod( pch, WIDE("Chat Control Sent Text Justification=%i"), SetSentTextJustification );
	AddConfigurationMethod( pch, WIDE("Chat Control Received Text Justification=%i"), SetReceiveTextJustification );
	AddConfigurationMethod( pch, WIDE("Chat Control Side Pad = %i"), SetSidePad );
	AddConfigurationMethod( pch, WIDE("Chat Control Sent Arrow Offset=%i,%i"), SetSentArrowOffset );
	AddConfigurationMethod( pch, WIDE("Chat Control Received Arrow Offset=%i,%i"), SetReceiveArrowOffset );
}


static void OnLoadCommon( WIDE( "Chat Control" ) )( PCONFIG_HANDLER pch )
{
	ScrollableChatControl_AddConfigurationMethods( pch );
}

static void SetupDefaultConfig( void )
{
	if( !l.decoration && !l.decoration_name )
	{
		TEXTCHAR buf[256];
		SACK_GetProfileString( "widgets/Chat Control", "message decoration", "chat-decoration.png", buf, 256 );
		l.decoration_name = StrDup( buf );
		l.decoration = LoadImageFile( buf );
		SACK_GetProfileString( "widgets/Chat Control", "send button pressed", "send_down.png", buf, 256 );
		l.button_pressed = LoadImageFile( buf );
		SACK_GetProfileString( "widgets/Chat Control", "send button normal", "send_up.png", buf, 256 );
		l.button_normal = LoadImageFile( buf );

		l.time_pad = SACK_GetProfileInt( "widgets/Chat Control", "time padding", 1 );
		l.context_message_pad = SACK_GetProfileInt( "widgets/Chat Control", "context message padding", 3 );
		l.context_sender_pad = SACK_GetProfileInt( "widgets/Chat Control", "context sender padding", 3 );
		l.side_pad = SACK_GetProfileInt( "widgets/Chat Control", "side padding", 5 );
		//l.sent.text_color = BASE_COLOR_BLACK;
		{
			PTEXT color = SegCreate( 32 );
			PTEXT _color = color;
			SetTextSize( color
						  , SACK_GetProfileString( "widgets/Chat Control"
														 , "sent message text color"
														 , "$FF000000"
														 , GetText( color )
														 , 32 ) );
			GetColorVar( &color, &l.sent.text_color );
			LineRelease( _color );
		}

		l.send.back_x = SACK_GetProfileInt( "widgets/Chat Control", "send message background x", 0 );
		l.send.back_y = SACK_GetProfileInt( "widgets/Chat Control", "send message background y", 95 );
		l.send.back_w = SACK_GetProfileInt( "widgets/Chat Control", "send message background w", 159 );
		l.send.back_h = SACK_GetProfileInt( "widgets/Chat Control", "send message background h", 39 );
		l.send.div_x1 = SACK_GetProfileInt( "widgets/Chat Control", "send message background division inset left", 10 );
		l.send.div_x2 = SACK_GetProfileInt( "widgets/Chat Control", "send message background division inset right", 10 + 136 );
		l.send.div_y1 = SACK_GetProfileInt( "widgets/Chat Control", "send message background division inset top", 95 + 10 );
		l.send.div_y2 = SACK_GetProfileInt( "widgets/Chat Control", "send message background division inset bottom", 95 + 10 + 22 );

		l.sent.back_x = SACK_GetProfileInt( "widgets/Chat Control", "sent message background x", 0 );
		l.sent.back_y = SACK_GetProfileInt( "widgets/Chat Control", "sent message background y", 0 );
		l.sent.back_w = SACK_GetProfileInt( "widgets/Chat Control", "sent message background w", 73 );
		l.sent.back_h = SACK_GetProfileInt( "widgets/Chat Control", "sent message background h", 76 );
		l.sent.arrow_x = SACK_GetProfileInt( "widgets/Chat Control", "sent message arrow x", 0 );
		l.sent.arrow_y = SACK_GetProfileInt( "widgets/Chat Control", "sent message arrow y", 84 );
		l.sent.arrow_w = SACK_GetProfileInt( "widgets/Chat Control", "sent message arrow w", 10 );
		l.sent.arrow_h = SACK_GetProfileInt( "widgets/Chat Control", "sent message arrow h", 8 );
		l.sent.div_x1 = SACK_GetProfileInt( "widgets/Chat Control", "sent message background division inset left", 10 );
		l.sent.div_x2 = SACK_GetProfileInt( "widgets/Chat Control", "sent message background division inset right", 10 + 54 );
		l.sent.div_y1 = SACK_GetProfileInt( "widgets/Chat Control", "sent message background division inset top", 10 );
		l.sent.div_y2 = SACK_GetProfileInt( "widgets/Chat Control", "sent message background division inset bottom", 9 + 57 );
		l.sent.arrow_x_offset = SACK_GetProfileInt( "widgets/Chat Control", "sent message arrow x offset", -2 );
		l.sent.arrow_y_offset = SACK_GetProfileInt( "widgets/Chat Control", "sent message arrow y offset", -10 );
		//l.sent.font = RenderFontFileScaledEx( WIDE("msyh.ttf"), 18, 18, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
		l.flags.sent_justification = SACK_GetProfileInt( "widgets/Chat Control", "sent message justification", 0 );
		l.flags.sent_text_justification = SACK_GetProfileInt( "widgets/Chat Control", "sent message text justification", 1 );
		{
			PTEXT color = SegCreate( 32 );
			PTEXT _color = color;
			SetTextSize( color
						  , SACK_GetProfileString( "widgets/Chat Control"
														 , "received message text color"
														 , "$FF000000"
														 , GetText( color )
														 , 32 ) );
			GetColorVar( &color, &l.received.text_color );
			LineRelease( _color );
		}
		//l.received.text_color = BASE_COLOR_BLACK;
		l.received.back_x = SACK_GetProfileInt( "widgets/Chat Control", "received message background x", 83 );
		l.received.back_y = SACK_GetProfileInt( "widgets/Chat Control", "received message background y", 0 );
		l.received.back_w = SACK_GetProfileInt( "widgets/Chat Control", "received message background w", 76 );
		l.received.back_h = SACK_GetProfileInt( "widgets/Chat Control", "received message background h", 77 );
		l.received.arrow_x = SACK_GetProfileInt( "widgets/Chat Control", "received message arrow x", 147 );
		l.received.arrow_y = SACK_GetProfileInt( "widgets/Chat Control", "received message arrow y", 86 );
		l.received.arrow_w = SACK_GetProfileInt( "widgets/Chat Control", "received message arrow w", 12 );
		l.received.arrow_h = SACK_GetProfileInt( "widgets/Chat Control", "received message arrow h", 7 );
		l.received.arrow_x_offset = SACK_GetProfileInt( "widgets/Chat Control", "received message arrow x offset", +2 );
		l.received.arrow_y_offset = SACK_GetProfileInt( "widgets/Chat Control", "received message arrow y offset", -10 );
		l.received.div_x1 = SACK_GetProfileInt( "widgets/Chat Control", "received message background division inset left", 92 );
		l.received.div_x2 = SACK_GetProfileInt( "widgets/Chat Control", "received message background division inset right", 92 + 52 );
		l.received.div_y1 = SACK_GetProfileInt( "widgets/Chat Control", "received message background division inset top", 9 );
		l.received.div_y2 = SACK_GetProfileInt( "widgets/Chat Control", "received message background division inset bottom", 9 + 59 );
		//l.received.font = l.sent.font;
		l.flags.received_justification = SACK_GetProfileInt( "widgets/Chat Control", "received message justification", 1 );
		l.flags.received_text_justification = SACK_GetProfileInt( "widgets/Chat Control", "received message text justification", 0 );
	}
	else
		ReuseImage( l.decoration );
	ChopDecorations( );	
}

static void OnFinishInit( WIDE( "Chat Control" ) )( PSI_CONTROL canvas )
{
	SetupDefaultConfig();
}

static void OnSaveCommon( WIDE( "Chat Control" ) )( FILE *file )
{

	sack_fprintf( file, WIDE("%sChat Control Sent Arrow Area=%d,%d %u,%u\n")
			 , InterShell_GetSaveIndent()
			 , l.sent.arrow_x
			 , l.sent.arrow_y
			 , l.sent.arrow_w
			 , l.sent.arrow_h );
	sack_fprintf( file, WIDE("%sChat Control Sent Arrow Offset=%d,%d\n")
			 , InterShell_GetSaveIndent()
			 , l.sent.arrow_x_offset
			 , l.sent.arrow_y_offset );

	sack_fprintf( file, WIDE("%sChat Control Received Arrow Area=%d,%d %u,%u\n")
			 , InterShell_GetSaveIndent()
			 , l.received.arrow_x
			 , l.received.arrow_y
			 , l.received.arrow_w
			 , l.received.arrow_h );
	sack_fprintf( file, WIDE("%sChat Control Received Arrow Offset=%d,%d\n")
			 , InterShell_GetSaveIndent()
			 , l.received.arrow_x_offset
			 , l.received.arrow_y_offset );

	sack_fprintf( file, WIDE("%sChat Control Sent Background Area=%d,%d %u,%u\n")
			 , InterShell_GetSaveIndent()
			 , l.sent.back_x
			 , l.sent.back_y
			 , l.sent.back_w
			 , l.sent.back_h );
	sack_fprintf( file, WIDE("%sChat Control Received Background Area=%d,%d %u,%u\n")
			 , InterShell_GetSaveIndent()
			 , l.received.back_x
			 , l.received.back_y
			 , l.received.back_w
			 , l.received.back_h );
	sack_fprintf( file, WIDE("%sChat Control Sent Background Dividers=%d,%d,%d,%d\n")
			 , InterShell_GetSaveIndent()
			 , l.sent.div_x1
			 , l.sent.div_x2
			 , l.sent.div_y1
			 , l.sent.div_y2 );
	sack_fprintf( file, WIDE("%sChat Control Received Background Dividers=%d,%d,%d,%d\n")
			 , InterShell_GetSaveIndent()
			 , l.received.div_x1
			 , l.received.div_x2
			 , l.received.div_y1
			 , l.received.div_y2 );
	sack_fprintf( file, WIDE("%sChat Control Background Image=%s\n")
			 , InterShell_GetSaveIndent()
			 , l.decoration_name );
	sack_fprintf( file, WIDE("%sChat Control Side Pad=%d\n")
			 , InterShell_GetSaveIndent()
			 , l.side_pad );
	sack_fprintf( file, WIDE("%sChat Control Received Decoration Justification=%d\n")
			 , InterShell_GetSaveIndent()
			, l.flags.received_justification );
	sack_fprintf( file, WIDE("%sChat Control Sent Decoration Justification=%d\n")
			 , InterShell_GetSaveIndent()
			, l.flags.sent_justification );
	sack_fprintf( file, WIDE("%sChat Control Received Text Justification=%d\n")
			 , InterShell_GetSaveIndent()
			, l.flags.received_text_justification );
	sack_fprintf( file, WIDE("%sChat Control Sent Text Justification=%d\n")
			 , InterShell_GetSaveIndent()
			, l.flags.sent_text_justification );
}


void Chat_SetMessageInputHandler( PSI_CONTROL pc, void (CPROC *Handler)( uintptr_t psv, PTEXT text ), uintptr_t psv )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	chat_control->InputData = Handler;
	chat_control->psvInputData = psv;
}


void Chat_SetPasteInputHandler( PSI_CONTROL pc, void (CPROC *Handler)( uintptr_t psv ), uintptr_t psv )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	chat_control->InputPaste = Handler;
	chat_control->psvInputPaste = psv;
}


void Chat_SetDropInputHandler( PSI_CONTROL pc, LOGICAL (CPROC *Handler)( uintptr_t psv, CTEXTSTR path, int32_t x, int32_t y ), uintptr_t psv )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	chat_control->InputDrop = Handler;
	chat_control->psvInputDrop = psv;
}

void Chat_SetSeenCallback( PSI_CONTROL pc, void (CPROC *Handler)( uintptr_t psv ), void (CPROC *DeleteHandler)( uintptr_t psv ) )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	chat_control->MessageSeen = Handler;
	chat_control->MessageDeleted = DeleteHandler;
}

void Chat_SetSeenImageCallback( PSI_CONTROL pc, void (CPROC *Handler)( uintptr_t psv, Image ), void (CPROC *DeleteHandler)( uintptr_t psv, Image ) )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	chat_control->ImageSeen = Handler;
	chat_control->ImageDeleted = DeleteHandler;
}

void Chat_ClearMessages( PSI_CONTROL pc )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	if( chat_control )
	{
		PCHAT_CONTEXT pcc;
		chat_control->control_offset = 0;
		while( pcc = (PCHAT_CONTEXT)DequeLink( &chat_control->contexts ) )
		{
			PCHAT_MESSAGE pcm;
			while( pcm = (PCHAT_MESSAGE)DequeLink( &pcc->messages ) )
			{
				Release( pcm->text );
				Release( pcm );
			}
			if( pcc->formatted_sender )
				Release( pcc->formatted_sender );
			DeleteLinkQueue( &pcc->messages );
			Release( pcc->sender );
			Release( pcc );
		}
	}
}

void Chat_ChangeMessageContent( struct chat_widget_message *msg, CTEXTSTR text ) {
	msg->text = StrDup( text );
	if( msg->formatted_text ) {
		Release( msg->formatted_text );
		msg->formatted_text = NULL;
	}
}

PCHAT_MESSAGE  Chat_EnqueMessage( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , PCHAT_TIME seen_time
							 , Image sender_icon
							 , CTEXTSTR sender
							 , CTEXTSTR text
							 , uintptr_t psvSeen )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	if( chat_control )
	{
		PCHAT_CONTEXT pcc;
		PCHAT_MESSAGE pcm;
		pcc = (PCHAT_CONTEXT)PeekQueueEx( chat_control->contexts, -1 );
		if( !pcc || ( pcc->sent != sent || StrCmp( pcc->sender, sender ) ) )
		{
			pcc = New( CHAT_CONTEXT );
			pcc->sent = sent;
			pcc->formatted_sender = NULL;
			pcc->sender = StrDup( sender );
			pcc->sender_image = sender_icon;
			pcc->max_width = 0;
			pcc->extra_width = 0;
			pcc->formatted_height = 0;
			pcc->messages = NULL;
			pcc->deleted = 0;
			EnqueLink( &chat_control->contexts, pcc );
		}
		pcm = New( CHAT_MESSAGE );
		if( received_time )
			pcm->received_time = received_time[0];
		else
			MemSet( &pcm->received_time, 0, sizeof( pcm->received_time ) );
		if( sent_time )
			pcm->sent_time = sent_time[0];
		else
			MemSet( &pcm->sent_time, 0, sizeof( pcm->sent_time ) );
		if( seen_time )
		{
			pcm->seen = seen_time[0].yr?1:0;
			pcm->seen_time = seen_time[0];
		}
		else
		{
			pcm->seen = 0;
			MemSet( &pcm->seen_time, 0, sizeof( pcm->seen_time ) );
		}
		pcm->deleted = FALSE;
		pcm->psvSeen = psvSeen;
		pcm->image = NULL;
		pcm->thumb_image = NULL;
		pcm->text = StrDup( text );
		pcm->_sent = sent;
		pcm->formatted_text = NULL;
		EnqueLink( &pcc->messages, pcm );
		return pcm;
	}
}

PCHAT_MESSAGE Chat_EnqueImage( PSI_CONTROL pc, LOGICAL sent
							 , PCHAT_TIME sent_time
							 , PCHAT_TIME received_time
							 , PCHAT_TIME seen_time
							 , Image sender_icon
							 , CTEXTSTR sender
							 , Image image
							 , uintptr_t psvSeen )
{
	PCHAT_LIST *ppList = (ControlData( PCHAT_LIST*, pc ));
	PCHAT_LIST chat_control = (*ppList);
	if( chat_control )
	{
		PCHAT_CONTEXT pcc;
		PCHAT_MESSAGE pcm;
		pcc = (PCHAT_CONTEXT)PeekQueueEx( chat_control->contexts, -1 );
		if( !pcc || ( pcc->sent != sent || pcc->sender != sender ) )
		{
			pcc = New( CHAT_CONTEXT );
			pcc->sent = sent;
			pcc->formatted_sender = NULL;
			pcc->sender = StrDup( sender );
			pcc->sender_image = sender_icon;
			pcc->max_width = 0;
			pcc->extra_width = 0;
			pcc->formatted_height = 0;
			pcc->messages = NULL;
			pcc->deleted = 0;
			EnqueLink( &chat_control->contexts, pcc );
		}
		if( pcm = (PCHAT_MESSAGE)PeekQueueEx( pcc->messages, -1 ) )
		{
			if( pcm->image == image )
				return;
		}
		
		pcm = New( CHAT_MESSAGE );
		if( received_time )
			pcm->received_time = received_time[0];
		else
			MemSet( &pcm->received_time, 0, sizeof( pcm->received_time ) );
		if( sent_time )
			pcm->sent_time = sent_time[0];
		else
			MemSet( &pcm->sent_time, 0, sizeof( pcm->sent_time ) );
		if( seen_time )
		{
			pcm->seen = seen_time[0].yr?1:0;
			pcm->seen_time = seen_time[0];
		}
		else
		{
			pcm->seen = 0;
			MemSet( &pcm->seen_time, 0, sizeof( pcm->seen_time ) );
		}
		pcm->psvSeen = psvSeen;
		pcm->text = NULL;
		pcm->image = image;
		pcm->thumb_image = MakeImageFile( 96 * image->width / image->height , 96 );
		BlotScaledImage( pcm->thumb_image, pcm->image );
		pcm->_sent = sent;
		pcm->formatted_text = NULL;
		pcm->deleted = 0;
		EnqueLink( &pcc->messages, pcm );
		return pcm;
	}
}

void MeasureFrameWidth( Image window, int32_t *left, int32_t *right, LOGICAL received, LOGICAL complete, int inset )
{
	if( received )
	{
		if( !complete )
		{
			(*left) = l.side_pad; // center? what is 2?
			(*right) = window->width - ( l.side_pad + inset );
		}
		else if( l.flags.received_justification == 0 )
		{
			(*left) = l.side_pad;
			(*right) = window->width - ( l.side_pad + l.received.arrow_w + inset ) - l.received.arrow_x_offset;
		}
		else if( l.flags.received_justification == 1 )
		{
			(*left) = l.side_pad + l.received.arrow_w - l.received.arrow_x_offset;
			(*right) = window->width - ( l.side_pad + inset );
		}
		else if( l.flags.received_justification == 2 )
		{
			(*left) = 0; // center? what is 2?
			(*right) = window->width - inset;
		}
	}
	else
	{
		if( !complete )
		{
			(*left) = l.side_pad + inset; // center? what is 2?
			(*right) = window->width - l.side_pad;
		}
		else if( l.flags.sent_justification == 0 )
		{
			(*left) = l.side_pad - l.sent.arrow_x_offset + inset;
			(*right) = window->width - ( l.side_pad + l.sent.arrow_w ) - l.sent.arrow_x_offset;
		}
		else if( l.flags.sent_justification == 1 )
		{
			(*left) = l.side_pad + l.sent.arrow_w - l.sent.arrow_x_offset + inset;
			(*right) = window->width - l.side_pad - l.sent.arrow_x_offset;
		}
		else if( l.flags.sent_justification == 2 )
		{
			(*left) =  + inset; // center? what is 2?
			(*right) = window->width;
		}
	}
}

uint32_t DrawSendTextFrame( Image window, int y, int height, int inset )
{
	int32_t x_offset_left;
	int32_t x_offset_right;
	height +=  l.send.BorderSegment[SEGMENT_TOP]->height + l.send.BorderSegment[SEGMENT_BOTTOM]->height;
	y -= l.send.BorderSegment[SEGMENT_TOP]->height + l.send.BorderSegment[SEGMENT_BOTTOM]->height;
	//lprintf( WIDE("Draw at %d   %d  bias %d") , y, height, inset );
	MeasureFrameWidth( window, &x_offset_left, &x_offset_right, TRUE, FALSE, inset );
	{
		BlotScaledImageSizedToAlpha( window, l.send.BorderSegment[SEGMENT_TOP]
											, x_offset_left + l.send.BorderSegment[SEGMENT_LEFT]->width
											, y
											, ( x_offset_right - x_offset_left ) - ( l.send.BorderSegment[SEGMENT_LEFT]->width
																	 + l.send.BorderSegment[SEGMENT_RIGHT]->width )
											, l.send.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.send.BorderSegment[SEGMENT_CENTER]
											, x_offset_left + l.send.BorderSegment[SEGMENT_LEFT]->width
											, y + l.send.BorderSegment[SEGMENT_TOP]->height
											, ( x_offset_right - x_offset_left ) - ( l.send.BorderSegment[SEGMENT_LEFT]->width
																	 + l.send.BorderSegment[SEGMENT_RIGHT]->width
																	 )
											, height - ( l.send.BorderSegment[SEGMENT_TOP]->height
															+ l.send.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.send.BorderSegment[SEGMENT_BOTTOM]
											, x_offset_left + l.send.BorderSegment[SEGMENT_LEFT]->width
											, y + height - l.send.BorderSegment[SEGMENT_BOTTOM]->height
											, ( x_offset_right - x_offset_left ) - ( l.send.BorderSegment[SEGMENT_LEFT]->width
																	 + l.send.BorderSegment[SEGMENT_RIGHT]->width)
											, l.send.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.send.BorderSegment[SEGMENT_LEFT]
											, x_offset_left
											, y + l.send.BorderSegment[SEGMENT_TOP]->height
											, l.send.BorderSegment[SEGMENT_LEFT]->width
											, height - ( l.send.BorderSegment[SEGMENT_TOP]->height + l.send.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.send.BorderSegment[SEGMENT_RIGHT]
											, (x_offset_right )-( l.send.BorderSegment[SEGMENT_RIGHT]->width )
											, y + l.send.BorderSegment[SEGMENT_TOP]->height
											, l.send.BorderSegment[SEGMENT_RIGHT]->width
											, height - ( l.send.BorderSegment[SEGMENT_TOP]->height
															+ l.send.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.send.BorderSegment[SEGMENT_TOP_LEFT]
						  , x_offset_left, y
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.send.BorderSegment[SEGMENT_TOP_RIGHT]
							  , x_offset_right - ( l.send.BorderSegment[SEGMENT_RIGHT]->width )
							  , y
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.send.BorderSegment[SEGMENT_BOTTOM_LEFT]
						  , x_offset_left , y + height - l.send.BorderSegment[SEGMENT_BOTTOM]->height
						  , ALPHA_TRANSPARENT );
		BlotImageAlpha( window, l.send.BorderSegment[SEGMENT_BOTTOM_RIGHT]
						  , x_offset_right - ( l.send.BorderSegment[SEGMENT_RIGHT]->width )
						  , y + height - l.send.BorderSegment[SEGMENT_BOTTOM]->height
						  , ALPHA_TRANSPARENT );
	}
	return height;
}

uint32_t DrawMessageFrame( Image window, int y, int height, int inset, LOGICAL received, LOGICAL complete )
{
	int32_t x_offset_left;
	int32_t x_offset_right;
	height +=  l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
	y -= l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
	//lprintf( WIDE("Draw at %d   %d  bias %d") , y, height, inset );
	MeasureFrameWidth( window, &x_offset_left, &x_offset_right, received, complete, inset );
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
																	 + l.received.BorderSegment[SEGMENT_RIGHT]->width
																	 )
											, height - ( l.received.BorderSegment[SEGMENT_TOP]->height
															+ l.received.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.received.BorderSegment[SEGMENT_BOTTOM]
											, x_offset_left + l.received.BorderSegment[SEGMENT_LEFT]->width
											, y + height - l.received.BorderSegment[SEGMENT_BOTTOM]->height
											, ( x_offset_right - x_offset_left ) - ( l.received.BorderSegment[SEGMENT_LEFT]->width
																	 + l.received.BorderSegment[SEGMENT_RIGHT]->width)
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
						  , x_offset_left, y
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
											, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width
												, y
											, ( x_offset_right - x_offset_left ) - ( l.sent.BorderSegment[SEGMENT_LEFT]->width
																	 + l.sent.BorderSegment[SEGMENT_RIGHT]->width ) , l.sent.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_BOTTOM]
											, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width
												, y + height - l.sent.BorderSegment[SEGMENT_BOTTOM]->height
											, x_offset_right - x_offset_left -( 
																	  l.sent.BorderSegment[SEGMENT_LEFT]->width
																	 + l.sent.BorderSegment[SEGMENT_RIGHT]->width)
											, l.sent.BorderSegment[SEGMENT_TOP]->height
											, ALPHA_TRANSPARENT );
		if( height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) )
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_LEFT]
											, x_offset_left
											, y + l.sent.BorderSegment[SEGMENT_TOP]->height
											, l.sent.BorderSegment[SEGMENT_LEFT]->width
											, height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		if( height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) )
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_RIGHT]
											, x_offset_right - ( l.sent.BorderSegment[SEGMENT_RIGHT]->width  )
											, y + l.sent.BorderSegment[SEGMENT_TOP]->height
											, l.sent.BorderSegment[SEGMENT_RIGHT]->width
											, height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height )
											, ALPHA_TRANSPARENT );
		if( height - ( l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) )
		BlotScaledImageSizedToAlpha( window, l.sent.BorderSegment[SEGMENT_CENTER]
											, x_offset_left + l.sent.BorderSegment[SEGMENT_LEFT]->width
											, y + l.sent.BorderSegment[SEGMENT_TOP]->height
											, ( x_offset_right - x_offset_left ) - ( l.sent.BorderSegment[SEGMENT_LEFT]->width
																	 + l.sent.BorderSegment[SEGMENT_RIGHT]->width)
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
	return height;
}

uint32_t UpdateContextExtents( Image window, PCHAT_LIST list, PCHAT_CONTEXT context )
{
	int message_idx;
	PCHAT_MESSAGE msg;

	uint32_t width;
	int32_t x_offset_left, x_offset_right;	
	int32_t frame_height;
	int32_t _x_offset_left, _x_offset_right;	
	MeasureFrameWidth( window, &x_offset_left, &x_offset_right, !context->sent, TRUE, 0 );
	_x_offset_left = x_offset_left;
	_x_offset_right = x_offset_right;
	if( context->sent )
	{
		x_offset_left += l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.arrow_w + l.sent.arrow_x_offset;
		x_offset_right -= l.sent.BorderSegment[SEGMENT_RIGHT]->width;
		width = ( x_offset_right - x_offset_left ) ;
		  //- ( l.sent.BorderSegment[SEGMENT_LEFT]->width 
		//	+ l.sent.BorderSegment[SEGMENT_RIGHT]->width ) ;
	}
	else
	{
		x_offset_left += l.received.BorderSegment[SEGMENT_LEFT]->width ;
		x_offset_right -= ( l.received.BorderSegment[SEGMENT_RIGHT]->width + l.received.arrow_w + l.received.arrow_x_offset ); 
		width = ( x_offset_right - x_offset_left ) ;
			//- ( l.received.BorderSegment[SEGMENT_LEFT]->width 
			//+ l.received.BorderSegment[SEGMENT_RIGHT]->width ) ;
	}

	context->formatted_height = 0;

	if( context->sender )
	{
		uint32_t sender_line = l.context_sender_pad;
		int max_width = width;// - ((msg->sent)?l.sent.arrow_w:l.received.arrow_w);
		int max_height = 50;
		if( !context->formatted_sender )
		{
			FormatTextToBlockEx( context->sender, &context->formatted_sender, &max_width, &max_height, list->sender_font );
			context->sender_height = max_height;
			context->sender_width = max_width;
		}
		context->formatted_height += context->sender_height + sender_line;
	}
	else
		context->sender_width = 0;

	//lprintf( WIDE("Begin formatting messages...") );
	frame_height = 0;
	for( message_idx = -1; msg = (PCHAT_MESSAGE)PeekQueueEx( context->messages, message_idx ); message_idx-- )
	{
		if( msg->deleted )
			continue;
		if( !msg->formatted_text && msg->text )
		{
			int max_width = width;// - ((msg->sent)?l.sent.arrow_w:l.received.arrow_w);
			int max_height = 9999;
			FormatTextToBlockEx( msg->text, &msg->formatted_text, &max_width, &max_height, list->sent_font );
			if( (uint32_t)max_width > ( context->max_width + context->extra_width ) )
			{
				context->extra_width = 0;
			}
			msg->formatted_text_len = StrLen( msg->formatted_text );
			msg->_formatted_height = max_height;
			if( SUS_GT( max_width, int, context->max_width, uint32_t ) )
				context->max_width = max_width;

			frame_height = msg->_formatted_height;
		}
		else
		{
			if( msg->formatted_text )
				frame_height = msg->_formatted_height;
			else
				if( msg->thumb_image )
				{
					msg->_formatted_height = msg->thumb_image->height ;
					msg->_formatted_height +=  ((message_idx<-1)?l.side_pad:0);
					msg->_message_y = + msg->_formatted_height;
					if( USS_GT( msg->thumb_image->width, int, context->max_width, uint32_t ) )
						context->max_width = msg->thumb_image->width;
					frame_height = msg->_formatted_height;
				}
		}
		frame_height += ((message_idx<-1)?l.context_message_pad:0);
		msg->_message_y = frame_height;

		if( context->sent )
		{
			CTEXTSTR timebuf;
			uint32_t w, h;
			uint32_t max_width = context->max_width;
			if( msg->sent_time.yr == 0 )
			{
				snprintf( msg->formatted_time, 256, "Sending..." );
			}
			else
			{
				timebuf = FormatMessageTime( "sent time", &l.now, &msg->sent_time ) ;
				snprintf( msg->formatted_time, 256, "Sent: %s", timebuf );
			}
			GetStringSizeFont(  msg->formatted_time, &w, &h, list->date_font );
			msg->time_height = h + l.time_pad;
			msg->_message_y += msg->time_height;
			frame_height += msg->time_height;
			if( (int32_t)w > ( x_offset_right - x_offset_left) )
				x_offset_left -= ( w ) - (x_offset_right - x_offset_left);
			if( w > context->max_width )
				context->extra_width = w - context->max_width;
		}
		else
		{
			CTEXTSTR timebuf;
			//CTEXTSTR timebuf2;
			uint32_t w2, h2;
			//CTEXTSTR timebuf3;

			timebuf = FormatMessageTime( "sent time", &l.now, &msg->sent_time ) ;
			//GetStringSizeFont( timebuf, &w, &h, list->date_font );
			//timebuf2 = FormatMessageTime( &l.now, &msg->received_time ) ;
			//GetStringSizeFont( timebuf2, &w2, &h2, list->date_font );
			//timebuf3 = FormatMessageTime( &l.now, &msg->seen_time ) ;
			//snprintf( msg->formatted_time, 256, "Sent: %s\nRcvd: %s\nSeen: %s", timebuf, timebuf2, timebuf3 );
			snprintf( msg->formatted_time, 256, "Sent: %s", timebuf);
			GetStringSizeFont( msg->formatted_time, &w2, &h2, list->date_font );
			if( w2 < (uint32_t)context->sender_width )
				w2 = (uint32_t)context->sender_width;
			msg->time_height = h2 + l.time_pad;
			msg->_message_y += msg->time_height;
			frame_height += msg->time_height;
			if( ( w2 ) > ( context->extra_width + context->max_width ) )
				context->extra_width = (w2) - context->max_width;
		}
		context->formatted_height += frame_height;
		//lprintf( "after format %d %d(%d,%d) is %s", context->formatted_height, msg->_message_y
		//	, msg->time_height, msg->_formatted_height, msg->text );
	}
	return x_offset_right - x_offset_left;
}

void DrawAMessage( Image window, PCHAT_LIST list
				  , PCHAT_CONTEXT context, PCHAT_MESSAGE msg
					, int32_t x_offset_left, int32_t x_offset_right
				  )
{
	int32_t _x_offset_left, _x_offset_right;	
	MeasureFrameWidth( window, &x_offset_left, &x_offset_right, !context->sent, TRUE, 0 );
	_x_offset_left = x_offset_left;
	_x_offset_right = x_offset_right;
	if( !msg->seen )
	{
		Chat_GetCurrentTime( &msg->seen_time );
		if( msg->image )
		{
			if( list->ImageSeen )
				list->ImageSeen( msg->psvSeen, msg->image );
		}
		else
			if( list->MessageSeen )
				list->MessageSeen( msg->psvSeen );
		msg->seen = 1;
	}
	if( context->sent )
	{
		x_offset_left += l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.arrow_w + l.sent.arrow_x_offset;
		x_offset_right -= l.sent.BorderSegment[SEGMENT_RIGHT]->width;
	}
	else
	{
		x_offset_left += l.received.BorderSegment[SEGMENT_LEFT]->width ;
		x_offset_right -= ( l.received.BorderSegment[SEGMENT_RIGHT]->width + l.received.arrow_w + l.received.arrow_x_offset ); 
	}
	//lprintf( WIDE("update next top by %d (%d+%d)"), msg->_message_y, msg->time_height, msg->_formatted_height );
	list->display.message_top -= msg->_message_y;
	if( context->sent )
	{
		CTEXTSTR timebuf;
		uint32_t  h;
		uint32_t max_width = context->extra_width + context->max_width;
		timebuf = msg->formatted_time;
		h = msg->time_height;
		PutStringFontExx( window, x_offset_left 
								+ ( ( x_offset_right - x_offset_left ) 
								- ( context->extra_width + context->max_width ) )
						, list->display.message_top// + l.received.BorderSegment[SEGMENT_TOP]->height
						, l.sent.text_color, 0
						, timebuf
						, StrLen( timebuf ), list->date_font, 0, max_width );

		if( msg->thumb_image )
			BlotImageAlpha( window, msg->thumb_image
						, msg->thumb_image_left = x_offset_left + ( ( x_offset_right - x_offset_left ) 
								- ( max_width ) )
							, h + list->display.message_top //+ l.received.BorderSegment[SEGMENT_TOP]->height 
							, ALPHA_TRANSPARENT
							);
		if( msg->formatted_text )
			PutStringFontExx( window, x_offset_left 
									+ ( ( x_offset_right - x_offset_left ) 
									- ( max_width ) )
							, h + list->display.message_top //+ l.received.BorderSegment[SEGMENT_TOP]->height
							, l.sent.text_color, 0
							, msg->formatted_text, msg->formatted_text_len, list->received_font, 0, context->max_width );
	}
	else
	{
		CTEXTSTR timebuf;
		//uint32_t w;
		uint32_t h;
		//CTEXTSTR timebuf2;
		//uint32_t w2;
		//uint32_t h2;
		uint32_t max_width = context->max_width;
		timebuf = msg->formatted_time;
		h = msg->time_height;

		PutStringFontExx( window, x_offset_left
						, list->display.message_top //+ l.received.BorderSegment[SEGMENT_TOP]->height
						, l.sent.text_color, 0
						, timebuf
						, StrLen( timebuf ), list->date_font, 0, context->max_width );

		if( msg->thumb_image )
			BlotImageAlpha( window, msg->thumb_image
							, msg->thumb_image_left = x_offset_left 
							, h + list->display.message_top //+ l.received.BorderSegment[SEGMENT_TOP]->height
							, ALPHA_TRANSPARENT
							);
		if( msg->formatted_text )
			PutStringFontExx( window, x_offset_left 
							, h + list->display.message_top //+ l.received.BorderSegment[SEGMENT_TOP]->height
							, l.received.text_color, 0
							, msg->formatted_text, msg->formatted_text_len, list->received_font, 0, max_width );
	}
}



static void ReformatMessages( PCHAT_LIST list )
{
	int context_idx;
	PCHAT_CONTEXT context;
	int message_idx;
	PCHAT_MESSAGE msg;
	for( context_idx = -1; context = (PCHAT_CONTEXT)PeekQueueEx( list->contexts, context_idx ); context_idx-- )
	{
		context->max_width = 0;
		Deallocate( TEXTSTR, context->formatted_sender );
		context->formatted_sender = NULL;
		for( message_idx = -1; msg = (PCHAT_MESSAGE)PeekQueueEx( context->messages, message_idx ); message_idx-- )
		{
			Deallocate( TEXTSTR, msg->formatted_text );
			msg->formatted_text = NULL;
		}
	}
}

static void DrawMessages( PCHAT_LIST list, Image window )
{
	int context_idx;
	PCHAT_CONTEXT context;
	int message_idx;
	PCHAT_MESSAGE msg;

	for( context_idx = -1; context = (PCHAT_CONTEXT)PeekQueueEx( list->contexts, context_idx ); context_idx-- )
	{
		uint32_t debug_start_top;
		uint32_t frame_size;
		int frame_pad;
		uint32_t max_width;

		int32_t x_offset_left, x_offset_right;	
		int32_t _x_offset_left, _x_offset_right;	
		if( context->deleted )
			continue;
		max_width = UpdateContextExtents( window, list, context );
		MeasureFrameWidth( window, &x_offset_left, &x_offset_right, !context->sent, TRUE, 0 );
		_x_offset_left = x_offset_left;
		_x_offset_right = x_offset_right;
		if( context->sent )
		{
			x_offset_left += l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.arrow_w + l.sent.arrow_x_offset;
			x_offset_right -= l.sent.BorderSegment[SEGMENT_RIGHT]->width;
		}
		else
		{
			x_offset_left += l.received.BorderSegment[SEGMENT_LEFT]->width ;
			x_offset_right -= ( l.received.BorderSegment[SEGMENT_RIGHT]->width + l.received.arrow_w + l.received.arrow_x_offset ); 
		}

		// between contexts...
		if( context_idx < -1 )
			list->display.message_top -= l.side_pad;

		if( context->sent )
			frame_pad = l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
		else
			frame_pad = l.received.BorderSegment[SEGMENT_BOTTOM]->height;

		if( ( ( list->display.message_top - ( frame_pad + context->formatted_height ) ) >= window->height ) )
		{
			if( context->sent )
				frame_pad += l.sent.BorderSegment[SEGMENT_TOP]->height;
			else
				frame_pad += l.received.BorderSegment[SEGMENT_TOP]->height;
			//lprintf( "Skip message is %d  (-%d", list->display.message_top, context->formatted_height );
			list->display.message_top -= context->formatted_height + frame_pad;
			continue;
		}
		//lprintf( "message start is %d  (-%d", list->display.message_top, context->formatted_height );
		frame_size = DrawMessageFrame( window
			, list->display.message_top - context->formatted_height
			, context->formatted_height
			, max_width - ( context->extra_width + context->max_width - 10 ) 
			, !context->sent, TRUE );

		debug_start_top = list->display.message_top;


		if( context->sent )
			list->display.message_top -= l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
		else
			list->display.message_top -= l.received.BorderSegment[SEGMENT_BOTTOM]->height;

		if( context->sender )
		{
			uint32_t x;
			if( context->sent )
				x = x_offset_left 
									+ ( ( x_offset_right - x_offset_left ) 
									- ( context->extra_width + context->max_width ) );
			else
				x = x_offset_left ;
			PutStringFontExx( window
							,x
							, list->display.message_top - context->formatted_height
							, l.sent.text_color, 0
							, context->formatted_sender
							, StrLen( context->formatted_sender ), list->sender_font, 0, max_width );
		}

		//lprintf( WIDE("BEgin draw messages...") );
		for( message_idx = -1; msg = (PCHAT_MESSAGE)PeekQueueEx( context->messages, message_idx ); message_idx-- )
		{
			if( msg->deleted )
				continue;
			//lprintf( "top is now %d (%d from start)", list->display.message_top, debug_start_top - list->display.message_top );
			//lprintf( "check message %d", message_idx );
			if( msg->formatted_text && 
				( ( list->display.message_top - msg->_message_y ) >= window->height ) )
			{
				//lprintf( "Skip a message %d (-%d)", list->display.message_top, msg->_message_y );
				//lprintf( WIDE("have to skip message...") );
				list->display.message_top -= msg->_message_y;
				continue;
			}
			//lprintf( "formatted : %d %d  %d", msg->_formatted_height, list->display.message_top, msg->_message_y );
			if( !msg->formatted_text || 
				 ( ( ( list->display.message_top - msg->_message_y ) < window->height )
				&& (list->display.message_top > l.side_pad ) ) )
				DrawAMessage( window, list, context, msg, x_offset_left, x_offset_right );
			if( list->display.message_top < l.side_pad )
			{
				//lprintf( WIDE("Done.") );
				break;
			}
		}

		if( context->sender )
			list->display.message_top -= context->sender_height + l.context_sender_pad;

		if( context->sent )
			list->display.message_top -= l.sent.BorderSegment[SEGMENT_TOP]->height;
		else
			list->display.message_top -= l.received.BorderSegment[SEGMENT_TOP]->height;
	}
}

static PCHAT_MESSAGE FindMessage( PCHAT_LIST list, int x, int y )
{
	int message_top = list->message_window->height + list->control_offset;
	int context_idx;
	PCHAT_CONTEXT context;
	int message_idx;
	LOGICAL found = FALSE;
	PCHAT_MESSAGE msg = NULL;
	if( y >= ( list->message_window->height ) )
		return NULL; // in command area.

	//y = list->message_window->height - y;
	//lprintf( "y to check is %d  message_top is %d", y, message_top );
	for( context_idx = -1; context = (PCHAT_CONTEXT)PeekQueueEx( list->contexts, context_idx ); context_idx-- )
	{
		//lprintf( WIDE("BEgin find messages... context %d"), context_idx );
		if( context->deleted )
			continue;
		if( context_idx < -1 )
			message_top -= l.side_pad;

		if( context->sent )
			message_top -= l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
		else
			message_top -= l.received.BorderSegment[SEGMENT_BOTTOM]->height;

		if( ( message_top - context->formatted_height ) <= y )
		{
			int check_message_top = message_top;
			for( message_idx = -1; msg = (PCHAT_MESSAGE)PeekQueueEx( context->messages, message_idx ); message_idx-- )
			{
				//lprintf( "check message %d", message_idx );
				if( msg->deleted )
					continue;
				if( msg->formatted_text )
				{
					//lprintf( WIDE("is a text skip message...") );
					check_message_top -= msg->_message_y;
					//if( ( check_message_top - msg->_message_y ) >= list->message_window->height )
					//{
						// message is below drawing area
					//	continue;
					//}
				}
				else if( msg->thumb_image )
				{
					check_message_top -= msg->thumb_image->height + msg->time_height + l.side_pad;
					//if( ( check_message_top - msg->thumb_image->height + (2*l.side_pad) ) >= list->message_window->height )
					//	continue;
				}
				if( y >= ( check_message_top ) )
				{
					found = TRUE;
					if( msg->thumb_image )
						if( SUS_LT( x, int, msg->thumb_image_left, uint32_t ) 
							|| SUS_GT( x, int, ( msg->thumb_image_left + msg->thumb_image->width ), uint32_t ) )
							msg = NULL; // didn't click ON the image, so clear return.
					break;
				}
				if( check_message_top < l.side_pad )
				{
					//lprintf( WIDE("Done.") );
					msg = NULL;
					break;
				}
			}
			if( msg || found )
				break;
		}
		else
		{
			//lprintf( "Skip based on context" );
			message_top -= context->formatted_height;
		}

		if( context->sender )
			message_top -= context->sender_height + l.context_sender_pad;

		if( context->sent )
			message_top -= l.sent.BorderSegment[SEGMENT_TOP]->height;
		else
			message_top -= l.received.BorderSegment[SEGMENT_TOP]->height;
	}
	return msg;

}

static void OnDisplayConnect( WIDE("@chat resources") )( struct display_app*app, struct display_app_local ***pppLocal )
{
	SetupDefaultConfig();	
}

int GetInputCursorIndex( PCHAT_LIST list, int x, int y )
{
	int nLine, nCursorLine;
	uint32_t cursor_pos = 0;
	uint32_t counter;
	DISPLAYED_LINE *pCurrentLine;
	PDATALIST *ppCurrentLineInfo;
	PTEXT first_seg = list->input.CommandInfo->CollectionBuffer;
	SetStart( first_seg );
	ppCurrentLineInfo = GetDisplayInfo( list->input.phb_Input );
	for( nLine = 0, counter = 0; ; nLine ++ )
	{
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
		if( !pCurrentLine
			|| ( pCurrentLine->nFirstSegOfs == 0 && pCurrentLine->start == first_seg ) )
			break;
	}
	if( y > nLine )
		return 0;

	for( nCursorLine = nLine; nCursorLine >= 0; nCursorLine-- )
	{
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nCursorLine );
		if( !pCurrentLine ) continue;
		//if( pCurrentLine && cursor_pos > pCurrentLine->nToShow )
		if( nCursorLine == y )
		{
			if( (INDEX)x < pCurrentLine->nToShow )
				cursor_pos += x;
			else
				cursor_pos += pCurrentLine->nToShow;
			break;
		}
		else
		{
			{					
				PTEXT seg = pCurrentLine->start;
				int ofs = 0; 
				int checklen = pCurrentLine->nToShow;
				int summed_len = 0;
				int first_ofs = pCurrentLine->nFirstSegOfs;
				for( ; seg; seg = NEXTLINE( seg ) )
				{
					size_t len = GetTextSize( seg );
					size_t seglen = len - first_ofs;
					if( ( summed_len + seglen ) < pCurrentLine->nToShow )
					{
						first_ofs = 0;
						checklen -= seglen;
						summed_len += seglen;
					}
					else
					{
						if( GetText( seg )[first_ofs + checklen] == '\n' )
							ofs++;
						break;
					}
				}
				cursor_pos += ofs;
			}
			cursor_pos += pCurrentLine->nToShow;
		}
	}
	return cursor_pos;
}

void GetInputCursorPos( PCHAT_LIST list, int *x, int *y )
{
	int nLine, nCursorLine;
	uint32_t cursor_pos = list->input.CommandInfo->CollectionIndex;
	uint32_t counter;
	DISPLAYED_LINE *pCurrentLine;
	PDATALIST *ppCurrentLineInfo;
	PTEXT cursor_segs;
	PTEXT first_seg = list->input.CommandInfo->CollectionBuffer;
	SetStart( first_seg );
	ppCurrentLineInfo = GetDisplayInfo( list->input.phb_Input );
	for( cursor_segs = list->input.CommandInfo->CollectionBuffer; cursor_segs; cursor_segs = cursor_segs->Prior )
	{
		if( cursor_segs == list->input.CommandInfo->CollectionBuffer )
			continue;
		cursor_pos += GetTextSize( cursor_segs );
	}

	for( nLine = 0, counter = 0; ; nLine ++ )
	{
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
		if( nLine == 0 && cursor_pos > pCurrentLine->nLineEnd )
		{
			(*y) = nLine;
			(*x) = cursor_pos - pCurrentLine->nLineStart;
			return;
		}
		if( cursor_pos >= pCurrentLine->nLineStart && cursor_pos <= pCurrentLine->nLineEnd )
		{
			(*y) = nLine;
			(*x) = cursor_pos - pCurrentLine->nLineStart;
			return;
		}
		if( !pCurrentLine
			|| ( pCurrentLine->nFirstSegOfs == 0 && pCurrentLine->start == first_seg ) )
			break;
	}

	for( nCursorLine = nLine; nCursorLine >= 0; nCursorLine-- )
	{
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nCursorLine );
		if( pCurrentLine && ( ( cursor_pos > pCurrentLine->nToShow )
					|| ( nCursorLine && cursor_pos == pCurrentLine->nToShow ) ) )
		{					
			PTEXT seg = pCurrentLine->start;
			int ofs = pCurrentLine->nToShow; 
			int checklen = ofs;
			int summed_len = 0;
			int first_ofs = pCurrentLine->nFirstSegOfs;
			int skiplen = pCurrentLine->nToShow;
			for( ; seg; seg = NEXTLINE( seg ) )
			{
				size_t len = GetTextSize( seg );
				size_t seglen = len - first_ofs;
				if( ( summed_len + seglen ) < pCurrentLine->nToShow )
				{
					first_ofs = 0;
					checklen -= seglen;
					skiplen -= seglen;
					summed_len += seglen;
				}
				else
				{
					if( GetText( seg )[first_ofs + checklen] == '\n' )
						ofs++;
					break;
				}
			}
			if( SUS_GT( ofs, int, cursor_pos, uint32_t ) )
				break;
			cursor_pos -= ofs;
		}
		else
			break;
	}
	(*x) = cursor_pos;
	(*y) = nCursorLine;
}

static void DrawTextEntry( PSI_CONTROL pc, PCHAT_LIST list, LOGICAL update )
{
	if( !IsControlHidden( pc ) )
	{
		Image window = GetControlSurface( pc );
		//int lines, list->command_skip_lines;
		int region_x, region_y, region_w, region_h;
		int newfontheight;

		list->input.command_lines = CountDisplayedLines( list->input.phb_Input );

		if( list->input.command_lines <= 0 )
		{
			//list->input.command_skip_lines = 0;
			list->input.command_lines = 1;
		}
		else 
		{
			if( list->input.command_skip_lines > list->input.command_lines )
			{
				if( list->input.command_lines > 3 )
				{
					//list->input.command_skip_lines = list->input.command_lines - 3;
					list->input.command_lines = 3;
				}
				else
				{
					//list->input.command_skip_lines = 0;
				}
			}
			else if( ( list->input.command_lines - list->input.command_skip_lines ) > 3 )
			{
				//list->input.command_skip_lines = list->input.command_lines - 3;
				list->input.command_lines = 3;
			}
		}

		newfontheight = GetFontHeight( list->sent_font );
		if( !list->nFontHeight )
		{
			list->nFontHeight = newfontheight;
			list->command_height = newfontheight * list->input.command_lines;
			list->send_button_size = ( newfontheight /*list->command_height */
				//list->nFontHeight
				+ l.side_pad /** 2 /* above and below input*/ 
				+ l.sent.BorderSegment[SEGMENT_TOP]->height 
				+ l.sent.BorderSegment[SEGMENT_BOTTOM]->height );
			MoveSizeControl( list->send_button, list->message_window->width - ( ( list->send_button_width?list->send_button_width:55 ) + l.side_pad )
									+ list->send_button_x_offset
				, window->height - ( ( list->send_button_height
									?list->send_button_height
									:list->send_button_size ) + l.side_pad ) /*list->message_window->height + l.side_pad*/
					+ list->send_button_y_offset
				, list->send_button_width?list->send_button_width:55
				, list->send_button_height?list->send_button_height:(list->send_button_size ) );
		}
		else if( list->nFontHeight != newfontheight 
		       || ( list->command_height != newfontheight * list->input.command_lines )
		       )
		{
			list->nFontHeight = newfontheight;
			list->command_height = list->nFontHeight * list->input.command_lines ;
			list->send_button_size = ( list->nFontHeight /*list->command_height */
				//list->nFontHeight
				+ l.side_pad /** 2*/ /* above and below input*/ 
				+ l.sent.BorderSegment[SEGMENT_TOP]->height 
				+ l.sent.BorderSegment[SEGMENT_BOTTOM]->height );

			MoveSizeControl( list->send_button
						, list->message_window->width - ( ( list->send_button_width?list->send_button_width:55 ) + l.side_pad )
									+ list->send_button_x_offset
				, window->height - ( ( list->send_button_height
									?list->send_button_height
									:list->send_button_size ) + l.side_pad ) /*list->message_window->height + l.side_pad*/
					+ list->send_button_y_offset
				, list->send_button_width?list->send_button_width:55
				, list->send_button_height?list->send_button_height:(list->send_button_size - l.side_pad ) );
#if 0
			MoveSizeControl( list->send_button, list->message_window->width - 60
				, window->height - ( list->send_button_size ) /*list->message_window->height + l.side_pad*/
				, 55
				, list->send_button_size - l.side_pad * 2 );
#endif
		}

		region_x = 0;
		region_y = window->height - ( list->command_height + l.side_pad + l.sent.BorderSegment[SEGMENT_BOTTOM]->height  + l.sent.BorderSegment[SEGMENT_TOP]->height);
		region_w = window->width - ( list->send_button_width?list->send_button_width:55 + l.side_pad );
		region_h = list->command_height + l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height;

		if( update && list->colors.background_color )
			BlatColor( window, region_x, region_y
					, region_w, region_h, list->colors.background_color);
		else
			BlatColor( window, region_x, region_y
					, region_w, region_h, GetControlColor( pc, NORMAL ) );
		
		DrawSendTextFrame( window
			, window->height - ( list->command_height + l.side_pad ) 
			, list->command_height
			, ( list->send_button_width?list->send_button_width:55 ) + l.side_pad );

		{
			int nLine, nCursorLine, nDisplayLine;
			size_t cursor_pos = list->input.CommandInfo->CollectionIndex;
			uint32_t counter;
			DISPLAYED_LINE *pCurrentLine;
			PDATALIST *ppCurrentLineInfo;
			PTEXT cursor_segs;
			PTEXT first_seg = list->input.CommandInfo->CollectionBuffer;
			SetStart( first_seg );
			ppCurrentLineInfo = GetDisplayInfo( list->input.phb_Input );
			for( nLine = 0, counter = 0; ; nLine ++ )
			{
				pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
				if( !pCurrentLine
					|| ( pCurrentLine->nFirstSegOfs == 0 && pCurrentLine->start == first_seg ) )
					break;
			}
			for( cursor_segs = list->input.CommandInfo->CollectionBuffer; cursor_segs; cursor_segs = cursor_segs->Prior )
			{
				if( cursor_segs == list->input.CommandInfo->CollectionBuffer )
					continue;
				cursor_pos += GetTextSize( cursor_segs );
			}

			for( nCursorLine = nLine; nCursorLine >= 0; nCursorLine-- )
			{
				pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nCursorLine );
				if( !pCurrentLine )
					break;
				if( cursor_pos > pCurrentLine->nLineEnd )
					continue;
				cursor_pos -= pCurrentLine->nLineStart;
				break;
			}
			if( nCursorLine >= 0 )
				nDisplayLine = nCursorLine;
			else
			{
				nDisplayLine = 0;
				nCursorLine = 0;
			}
			if( ( nDisplayLine - list->input.command_skip_lines ) < 0 )
				list->input.command_skip_lines = nDisplayLine;
			if( ( nDisplayLine - list->input.command_skip_lines ) > 2 )
				list->input.command_skip_lines = nCursorLine - 2;
			for( nLine = list->input.command_skip_lines; 
				 nLine < (list->input.command_skip_lines+list->input.command_lines); 
				 nLine ++ )
			{
				pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
				if( IsControlFocused( list->pc ) &&
					( ( !nLine && !pCurrentLine ) || ( nLine == nCursorLine ) ) )
				{
					uint32_t w;
					uint32_t total_w = 0;
					if( pCurrentLine )
					{
						PTEXT tmp;
						uint32_t amount = cursor_pos;
						uint32_t seg_amount;
						size_t seg_start = pCurrentLine->nFirstSegOfs;
						for( tmp = pCurrentLine->start; cursor_pos && tmp; tmp = NEXTLINE( tmp ) )
						{
							seg_amount = GetTextSize( tmp ) - seg_start;
							if( cursor_pos > seg_amount )
								amount = seg_amount;
							else
								amount = cursor_pos;
							GetStringSizeFontEx( GetText( tmp ) + seg_start
								, amount, &w, NULL, list->sent_font );
							cursor_pos -= amount;
							total_w += w;
							seg_start = 0;
						}
					}
					else
						total_w = 0;
					if( list->flags.bCursorOn )
						do_vline( window, 2 + l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + total_w
						, window->height - ( l.side_pad + (l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) + list->nFontHeight * ( nLine - list->input.command_skip_lines ) )
							, window->height - ( l.side_pad + (l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) + list->nFontHeight * (( nLine - list->input.command_skip_lines )+1))
							, BASE_COLOR_BLACK );
					else
						do_vline( window, 2 + l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + total_w
							, window->height - ( l.side_pad + (l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) + list->nFontHeight * ( nLine - list->input.command_skip_lines ) )
							, window->height - ( l.side_pad + (l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) + list->nFontHeight * (( nLine - list->input.command_skip_lines )+1))
							, BASE_COLOR_WHITE );
				}
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
					//lprintf( "mark begin %d end %d", list->input.command_mark_start, list->input.command_mark_end );
					RenderTextLine( list, window, pCurrentLine, &upd
						, nLine - list->input.command_skip_lines
						, list->sent_font
						, window->height - ( l.side_pad + (l.sent.BorderSegment[SEGMENT_BOTTOM]->height ) + list->nFontHeight )
						, l.side_pad + l.sent.BorderSegment[SEGMENT_TOP]->height
						, l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width
						, list->input.command_mark_start
						, list->input.command_mark_end
						);  // cursor; to know where to draw the mark...
				}
			}
		}
		if( update )
		{
			//SmudgeCommon( pc );
			//UpdateFrame( pc, region_x, region_y, region_w, region_h );
		}
	}
}


static int OnDrawCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	Image window = GetControlSurface( pc );
	int cmd_size;

	Chat_GetCurrentTime( &l.now );

	if( !list )
	{
		ClearImageTo( window, BASE_COLOR_BLUE );
		return 1;
	}
	if( list->colors.background_color )
		ClearImageTo( window, list->colors.background_color);


	if( l.decoration )
	{
		list->command_size = ( list->command_height + l.side_pad * 2 /* above and below input*/ + l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height) ;
		ResizeImage( list->message_window, window->width, window->height - list->command_size );
		list->display.message_top = list->message_window->height + list->control_offset;
	}
	else
		cmd_size = 0;

	if( l.decoration )
	{
		DrawTextEntry( pc, list, FALSE );
	}

	if( l.decoration )
	{
		//int cmd_size = ( list->command_height + l.side_pad * 2 /* above and below input*/ + l.sent.BorderSegment[SEGMENT_TOP]->height + l.sent.BorderSegment[SEGMENT_BOTTOM]->height) ;

		//ResizeImage( list->message_window, window->width, window->height - cmd_size );
		//list->display.message_top = list->message_window->height + list->control_offset;
		MoveSizeControl( list->send_button, list->message_window->width - ( ( list->send_button_width?list->send_button_width:55 ) + l.side_pad )
								+ list->send_button_x_offset
			, window->height - ( ( list->send_button_height
								?list->send_button_height
								:list->send_button_size )  ) /*list->message_window->height + l.side_pad*/
				+ list->send_button_y_offset
			, list->send_button_width?list->send_button_width:55
			, list->send_button_height?list->send_button_height:(list->send_button_size - l.side_pad  ) );
		/*
		MoveSizeControl( list->send_button, list->message_window->width - 60
			, list->message_window->height + l.side_pad
			, 55
			, list->command_size - l.side_pad * 2 );
		*/
		DrawMessages( list, list->message_window );
	}

	{
		uint32_t mid = list->message_window->width / 2;
		if( list->display.message_top < 0 )
		{
			int arrow;
			int row;
			for( arrow = 0; arrow < 3; arrow++ )
			{
				for( row = 0; row < 6; row++ )
				{
					do_hline( window, arrow * 10 + row, 7 - row, 7 + row, BASE_COLOR_BLACK );
					do_hline( window, arrow * 10 + row, ( list->message_window->width - 7 ) - row, ( list->message_window->width - 7 ) + row, BASE_COLOR_BLACK );
				}
			}
#if 0
			//do_hline( window, 7, 5, list->message_window->width - 10, BASE_COLOR_BLACK );
			do_hline( window, 6, 5, list->message_window->width - 10, BASE_COLOR_BLACK );
			do_hline( window, 5, 5, list->message_window->width - 10, BASE_COLOR_BLACK );
			do_hline( window, 4, mid - 3, mid + 3, BASE_COLOR_BLACK );
			do_hline( window, 3, mid - 2, mid + 2, BASE_COLOR_BLACK );
			do_hline( window, 2, mid - 1, mid + 1, BASE_COLOR_BLACK );
			do_hline( window, 1, mid, mid, BASE_COLOR_BLACK );
#endif
		}
		if( list->control_offset )
		{
			int arrow;
			int row;
			int base = list->message_window->height - ( 20 );
			for( arrow = 0; arrow < 3; arrow++ )
			{
				for( row = 0; row < 6; row++ )
				{
					do_hline( window, arrow * 10 + ( base - row ), 7 - row, 7 + row, BASE_COLOR_BLACK );
					do_hline( window, arrow * 10 + ( base - row ), ( list->message_window->width - 7 ) - row, ( list->message_window->width - 7 ) + row, BASE_COLOR_BLACK );
				}
			}
#if 0
			//do_hline( window, list->message_window->height - 7, 5, list->message_window->width - 10, BASE_COLOR_BLACK );
			do_hline( window, list->message_window->height - 6, 5, list->message_window->width - 10, BASE_COLOR_BLACK );
			do_hline( window, list->message_window->height - 5, 5, list->message_window->width - 10, BASE_COLOR_BLACK );
			do_hline( window, list->message_window->height - 4, mid - 3, mid + 3, BASE_COLOR_BLACK );
			do_hline( window, list->message_window->height - 3, mid - 2, mid + 2, BASE_COLOR_BLACK );
			do_hline( window, list->message_window->height - 2, mid - 1, mid + 1, BASE_COLOR_BLACK );
			do_hline( window, list->message_window->height - 1, mid, mid, BASE_COLOR_BLACK );
#endif
		}
	}
	return 1;
}

size_t GetChatInputCursor( PCHAT_LIST list, int x, int y )
{
	PDATALIST *ppCurrentLineInfo = GetDisplayInfo( list->input.phb_Input );
	int nLine = list->input.command_skip_lines + ( list->input.command_lines - ( 1 + y / list->nFontHeight ) ); 
	{
		PDISPLAYED_LINE pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
		if( pCurrentLine )
		{
			PTEXT tmp;
			//uint32_t amount = cursor_pos;
			uint32_t line_index = 0;
			uint32_t line_length = pCurrentLine->nToShow;
			uint32_t seg_start = pCurrentLine->nFirstSegOfs;
			for( tmp = pCurrentLine->start; tmp; tmp = NEXTLINE( tmp ) )
			{
				CTEXTSTR text = GetText( tmp );
				uint32_t len = GetTextSize( tmp );
				uint32_t ch;
				uint32_t ch_width, ch_height;
				for( ch = seg_start; ch < len && line_index < line_length; ch++, line_index++ )
				{
					GetStringRenderSizeFontEx( GetText( tmp ) + ch, 1, &ch_width, &ch_height, NULL, list->sent_font );
					if( SUS_LT( x, int, ch_width / 2, uint32_t ) )
						return pCurrentLine->nLineStart + line_index;
					x -= ch_width;
				}
				seg_start = 0;
			}
			return pCurrentLine->nLineStart + (pCurrentLine->nToShow);
		}
	}
	return INVALID_INDEX;
}

static int OnMouseCommon( CONTROL_NAME )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	if( !list )
		return 1;

	if( MAKE_A_BUTTON( b, list->_b, MK_SCROLL_DOWN ) )
	{
		if( list->control_offset > ( list->nFontHeight * 3 ) )
			list->control_offset -= list->nFontHeight * 3;
		else
			list->control_offset = 0;
		SmudgeCommon( pc );
	}
	else if( MAKE_A_BUTTON( b, list->_b, MK_SCROLL_UP ) )
	{
		list->control_offset += list->nFontHeight * 3;
		SmudgeCommon( pc );
	}
	if( MAKE_A_BUTTON( b, list->_b, MK_LBUTTON ) )
	{
		list->flags.begin_button = 1;
		list->first_x = x; 
		list->first_y = y; 
		list->flags.checked_drag = 0;
		list->flags.long_vertical_drag = 0;
		list->flags.long_horizontal_drag = 0;
		{
			Image window = GetControlSurface( pc );

			if( y > ( window->height - ( l.sent.BorderSegment[SEGMENT_BOTTOM]->height
				+ l.sent.BorderSegment[SEGMENT_TOP]->height
				+ list->command_height + l.side_pad ) ) )
			{
				int command_y = y - ( window->height - ( l.sent.BorderSegment[SEGMENT_BOTTOM]->height
						+ list->command_height + l.side_pad ) );
				int command_x = x - ( l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width );
				int cursor = GetChatInputCursor( list, command_x, command_y );
				list->flags.in_command = 1;
				list->input.command_mark_cursor_down = cursor;
				SetUserInputPosition( list->input.CommandInfo, cursor, COMMAND_POS_SET );
				//lprintf(" below command.... : %d,%d  at %d"
				//		, command_x
				//		, command_y
				//		, cursor );
				SmudgeCommon( pc );
				//list->flags.allow_command_popup = 1;
			}
			else
			{
				list->flags.in_command = 0;
				//list->flags.allow_command_popup = 0;
			}
		}
	}
	else if( BREAK_A_BUTTON( b, list->_b, MK_LBUTTON ) )
	{
		if( !list->flags.checked_drag )
		{
			// did not find a drag motion while the button was down... this is a tap
			{
				// go through the drawn things, was it a click on a message?
				PCHAT_MESSAGE msg = FindMessage( list, list->first_x, list->first_y );
				if( msg )
				{
					if( msg->image )
					{
						PSI_CONTROL pc_viewer = ImageViewer_ShowImage( pc, msg->image, list->PopupEvent, list->psvPopup );
						ImageViewer_SetAutoCloseHandler( pc_viewer, list->OnImageAutoClose, list->psvImageAutoClose );

					}
				}
			}
		}
	}
	else if( BUTTON_STILL_DOWN( b, MK_LBUTTON ) )
	{
		if( list->flags.in_command )
		{
			Image window = GetControlSurface( pc );
			int command_y = y - ( window->height - ( l.sent.BorderSegment[SEGMENT_BOTTOM]->height
					+ list->command_height + l.side_pad ) );
			int command_x = x - ( l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width );
			int cursor = GetChatInputCursor( list, command_x, command_y );
			int start = list->input.command_mark_start;
			int end = list->input.command_mark_end;
			SetUserInputPosition( list->input.CommandInfo, cursor, COMMAND_POS_SET );
			if( cursor != list->input.command_mark_cursor_down )
			{
				if( cursor > list->input.command_mark_cursor_down )
				{
					list->input.command_mark_start = list->input.command_mark_cursor_down;
					list->input.command_mark_end = cursor + 1;
				}
				else
				{
					list->input.command_mark_end = list->input.command_mark_cursor_down;
					list->input.command_mark_start = cursor;
				}				
			}
			else
				list->input.command_mark_start = list->input.command_mark_end = 0;
			if( list->input.command_mark_start != start || list->input.command_mark_end != end )
				SmudgeCommon( pc );
		}
		else if( !list->flags.checked_drag )
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
				uint32_t original_offset = list->control_offset;
				list->control_offset += ( y - list->first_y );
				//lprintf( WIDE("adjust position by %d"), ( y - list->first_y ) );
				if( list->control_offset < 0 )
					list->control_offset = 0;
				else
				{
					int last_message_y = 0;
					int tmp_top = list->message_window->height + list->control_offset;
					int message_idx;
					int total_offset = 0;
					int context_idx;
					PCHAT_CONTEXT context;
					PCHAT_MESSAGE msg = NULL; // might have 0 contexts
					for( context_idx = -1; context = (PCHAT_CONTEXT)PeekQueueEx( list->contexts, context_idx ); context_idx-- )
					{
						uint32_t frame_pad;

						// between contexts...
						if( context_idx < -1 )
							list->display.message_top -= l.side_pad;

						if( context->sent )
						{
							frame_pad = l.sent.BorderSegment[SEGMENT_BOTTOM]->height;
							frame_pad += l.sent.BorderSegment[SEGMENT_TOP]->height;
						}
						else
						{
							frame_pad = l.received.BorderSegment[SEGMENT_BOTTOM]->height;
							frame_pad += l.received.BorderSegment[SEGMENT_TOP]->height;
						}

						for( message_idx = -1; msg = (PCHAT_MESSAGE)PeekQueueEx( context->messages, message_idx ); message_idx-- )
						{
							// never displayed the message.
							if( !msg->formatted_text )
								break;
							last_message_y = msg->_message_y;
							//tmp_top -= msg->_message_y;
							//total_offset += msg->_message_y;
						}
						if( msg )
							break;
						tmp_top -= context->formatted_height + frame_pad;
						total_offset += context->formatted_height + frame_pad;
					}

					if( !msg && ( tmp_top > ( list->message_window->height - 50/*last_message_y*/ ) ) )
					{
						list->control_offset = total_offset - 50 /*last_message_y*/;//list->message_window->height;
					}
				}
				list->first_y = y;
				if( list->control_offset != original_offset )
					SmudgeCommon( pc );
			}
		}
	}
	else // this is no buttons, and is continued no buttons (not last button either)
	{
	}

	if( MAKE_A_BUTTON( b, list->_b, MK_RBUTTON ) )
	{
		Image window = GetControlSurface( pc );

		if( y > ( window->height - ( l.sent.BorderSegment[SEGMENT_BOTTOM]->height
			+ l.sent.BorderSegment[SEGMENT_TOP]->height
			+ list->command_height + l.side_pad ) ) )
		{
			//lprintf(" below command...." );
			list->flags.allow_command_popup = 1;
		}
		else
			list->flags.allow_command_popup = 0;
	}
	else if( BREAK_A_BUTTON( b, list->_b, MK_RBUTTON ) )
	{
		if( list->flags.allow_command_popup )
		{
			int r;
			if( ( r = TrackPopup( list->popup_text_entry, pc ) ) >= 0 )
			{
				switch( r )
				{
				case 1:
					if( list->InputPaste )
						list->InputPaste( list->psvInputPaste );
					break;
				}
			}
		}
	}

	list->_b = b;
	return 1;
}

static void CPROC PSIMeasureString( uintptr_t psv, CTEXTSTR s, int nShow, uint32_t *w, uint32_t *h, SFTFont font )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	//SFTFont font = list->GetCommonFont( pc );
	list->nFontHeight = GetFontHeight( font );
	GetStringSizeFontEx( s, nShow, w, h, font );
}


static LOGICAL CPROC DropAccept( PSI_CONTROL pc, CTEXTSTR path, int32_t x, int32_t y )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	return list->InputDrop( list->psvInputDrop, path, x, y );
}

static int OnCommonFocus(CONTROL_NAME)(PSI_CONTROL control, LOGICAL bFocused )
{
	SmudgeCommon( control );
	return 1;
}

static void CPROC SendTypedMessage( uintptr_t psvUnused, PSI_CONTROL pc )
{
	PCHAT_LIST list = (PCHAT_LIST)psvUnused;
	KeyGetGatheredLine( list, list->input.CommandInfo );
	SetCommonFocus( GetCommonParent( pc ) );
	//SmudgeCommon( GetCommonParent( pc ) );
}

#ifdef _MSC_VER

static int EvalExcept( int n )
{
	switch( n )
	{
	case 		STATUS_ACCESS_VIOLATION:
			//lprintf( WIDE( "Access violation - OpenGL layer at this moment.." ) );
	return EXCEPTION_EXECUTE_HANDLER;
	default:
			//lprintf( WIDE( "Filter unknown : %08X" ), n );

		return EXCEPTION_CONTINUE_SEARCH;
	}
	// unreachable code.
	//return EXCEPTION_CONTINUE_EXECUTION;
}
#endif

static void CPROC cursorTick( uintptr_t psv )
{
	PCHAT_LIST list = (PCHAT_LIST)psv;
	list->flags.bCursorOn = !list->flags.bCursorOn;
#ifdef _MSC_VER
		__try
		{
#endif
	if( !IsControlHidden( list->pc ) )
		SmudgeCommon( list->pc );
	//DrawTextEntry( list->pc, list, TRUE );
#ifdef _MSC_VER
			}
			__except( EvalExcept( GetExceptionCode() ) )
			{
				lprintf( WIDE( "Caught exception in scrollable chat list control" ) );
				;
			}
#endif
}

static int OnCreateCommon( CONTROL_NAME )( PSI_CONTROL pc )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list;
	AddCommonAcceptDroppedFiles( pc, DropAccept );
	SetupDefaultConfig();
	(*ppList) = New( CHAT_LIST );
	MemSet( (*ppList), 0, sizeof( CHAT_LIST ) );
	list = (*ppList);
	list->pc = pc;
	list->popup_text_entry = CreatePopup();
	AppendPopupItem( list->popup_text_entry, MF_STRING, 1, "Paste" );
	list->cursor_timer = AddTimer( 500, cursorTick, (uintptr_t)list );
	{
		int w, h;
		TEXTCHAR buf[64];
		SACK_GetProfileString( "widgets/Chat Control", "message font", "msyh.ttf", buf, 64 );
		w = SACK_GetProfileInt( "widgets/Chat Control", "message font/width", 18 );
		h = SACK_GetProfileInt( "widgets/Chat Control", "message font/height", 18 );
		list->sent_font 
			= list->received_font
			= list->input_font 
			= RenderFontFileScaledEx( buf, w, h, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
		SACK_GetProfileString( "widgets/Chat Control", "date font", "msyh.ttf", buf, 64 );
		w = SACK_GetProfileInt( "widgets/Chat Control", "date font/width", 9 );
		h = SACK_GetProfileInt( "widgets/Chat Control", "date font/height", 9 );
		list->date_font 
			= RenderFontFileScaledEx( buf, w, h, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
		SACK_GetProfileString( "widgets/Chat Control", "sender font", "arialbd.ttf", buf, 64 );
		w = SACK_GetProfileInt( "widgets/Chat Control", "sender font/width", 12 );
		h = SACK_GetProfileInt( "widgets/Chat Control", "sender font/height", 12 );
		list->sender_font 
			= RenderFontFileScaledEx( buf, w, h, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
	}
	//list->colors.background_color = BASE_COLOR_WHITE;
	list->message_window = MakeSubImage( GetControlSurface( pc ), 0, 0, 1, 1 );
	list->send_button = MakeNamedCaptionedControl( pc, IMAGE_BUTTON_NAME, list->message_window->width - 35, list->message_window->height - 25, 35, 25, -1, TranslateText( "Send" ) );
	SetButtonPushMethod( list->send_button, SendTypedMessage, (uintptr_t)list );
	SetCommonBorder( list->send_button, BORDER_NONE | BORDER_FIXED | BORDER_NOCAPTION );
	SetButtonImages( list->send_button, l.button_normal, l.button_pressed );

	list->input.pHistory = PSI_CreateHistoryRegion();
	//list->phlc_Input = PSI_CreateHistoryCursor( list->pHistory );
	list->input.phb_Input = PSI_CreateHistoryBrowser( list->input.pHistory, PSIMeasureString, (uintptr_t)pc );
	list->input.CommandInfo = CreateUserInputBuffer();
	//SetBrowserLines( list->input.phb_Input, 3 );
	list->colors.crText = BASE_COLOR_BLACK;
	list->colors.crMark = BASE_COLOR_WHITE;
	list->colors.crMarkBackground = BASE_COLOR_BLUE;
	return 1;
}

static uintptr_t OnCreateControl( INTERSHELL_CONTROL_NAME )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc = MakeNamedControl( parent, CONTROL_NAME, x, y, w,h, -1 );

	return (uintptr_t)pc;
}

static PSI_CONTROL OnGetControl( INTERSHELL_CONTROL_NAME )( uintptr_t psv )
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
		{
			Image window = GetControlSurface( pc );
			// fix input
			if( l.sent.BorderSegment[SEGMENT_LEFT] )
			{
				list->input.phb_Input->nLineHeight = GetFontHeight( list->input_font );
				list->input.phb_Input->nColumns 
					= list->input.phb_Input->nWidth 
					= window->width - ( 2 * l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.BorderSegment[SEGMENT_RIGHT]->width 
							+ ( list->send_button_width?list->send_button_width:55 + l.side_pad ) );
			}
			BuildDisplayInfoLines( list->input.phb_Input, NULL, list->input_font );
			ReformatMessages( list );
		}
	}
}



static int OnKeyCommon( CONTROL_NAME )( PSI_CONTROL pc, uint32_t key )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	//PCONSOLE_INFO list = (PCONSOLE_INFO)GetCommonUserData( pc );
	// this must here gather keystrokes and pass them forward into the
	// opened sentience...
	if( list )
	{
		DECLTEXT( stroke, WIDE("           ") ); // single character ...
		//Log1( "Key: %08x", key );
		if( !list || !l.decoration ) // not a valid window handle/device path
			return 0;
		//EnterCriticalSec( &list->Lock );
		if( ( key & KEY_PRESSED ) 
			&& KEY_CODE( key ) == KEY_V 
			&& ( key & KEY_CONTROL_DOWN )
			&& !( key & ( KEY_SHIFT_DOWN | KEY_ALT_DOWN ) ) )
		{
			if( list->InputPaste )
				list->InputPaste( list->psvInputPaste );
		}
		else switch( KEY_CODE( key ) )
		{
		case KEY_HOME:
			list->input.control_key_state = ( key & ( KEY_SHIFT_DOWN|KEY_CONTROL_DOWN|KEY_ALT_DOWN ) ) >> 28;
			if( key & KEY_PRESSED ) {
				Chat_KeyHome( list, list->input.CommandInfo );
				SmudgeCommon( pc );
			}
			break;
		case KEY_END:
			list->input.control_key_state = ( key & ( KEY_SHIFT_DOWN|KEY_CONTROL_DOWN|KEY_ALT_DOWN ) ) >> 28;
			if( key & KEY_PRESSED ) {
				Chat_KeyEndCmd( list, list->input.CommandInfo );
				SmudgeCommon( pc );
			}
			break;
		case KEY_LEFT:
			list->input.control_key_state = ( key & ( KEY_SHIFT_DOWN|KEY_CONTROL_DOWN|KEY_ALT_DOWN ) ) >> 28;
			if( key & KEY_PRESSED ) {
				Chat_KeyLeft( list, list->input.CommandInfo );
				SmudgeCommon( pc );
			}
			break;
		case KEY_RIGHT:
			list->input.control_key_state = ( key & ( KEY_SHIFT_DOWN|KEY_CONTROL_DOWN|KEY_ALT_DOWN ) ) >> 28;
			if( key & KEY_PRESSED ) {
				Chat_KeyRight( list, list->input.CommandInfo );
				SmudgeCommon( pc );
			}
			break;
		case KEY_UP:
			list->input.control_key_state = ( key & ( KEY_SHIFT_DOWN|KEY_CONTROL_DOWN|KEY_ALT_DOWN ) ) >> 28;
			if( key & KEY_PRESSED ) {
				Chat_CommandKeyUp( list, list->input.CommandInfo );
				SmudgeCommon( pc );
			}
			break;
		case KEY_DOWN:
			list->input.control_key_state = ( key & ( KEY_SHIFT_DOWN|KEY_CONTROL_DOWN|KEY_ALT_DOWN ) ) >> 28;
			if( key & KEY_PRESSED ) {
				Chat_HandleKeyDown( list, list->input.CommandInfo );
				SmudgeCommon( pc );
			}
			break;
		case KEY_DELETE:
			{
				DECLTEXT( KeyStrokeDelete, WIDE("\x7f") ); // DECLTEXT implies 'static'
				if( key & KEY_PRESSED ) {
					DeleteUserInput( list->input.CommandInfo );
					ReformatInput( list );
					SmudgeCommon( pc );
				}
			}
			break;
		case KEY_BACKSPACE:
			{
				DECLTEXT( KeyStrokeBackspace, WIDE("\x08") ); // DECLTEXT implies 'static'
				if( key & KEY_PRESSED ) {
					Widget_WinLogicDoStroke( list, (PTEXT)&KeyStrokeBackspace );
					SmudgeCommon( pc );
				}
			}
			break;
		case KEY_ENTER:
			if( key & ( KEY_SHIFT_DOWN | KEY_CONTROL_DOWN ) )
			{
				KeyGetGatheredLine( list, list->input.CommandInfo );
				SmudgeCommon( pc );
				break;
			}
			// else fall through.
		default:
			// here is where we evaluate the curent keystroke....
			{
				const TEXTCHAR* character = GetKeyText( key );
				if( character )
				{
					int n;
					for( n = 0; character[n]; n++ )
						stroke.data.data[n] = character[n];
					stroke.data.data[n] = character[n];
					stroke.data.size = n;
				}
				else
					stroke.data.size = 0;
				{
					Image window = GetControlSurface( pc );
					//SetBrowserColumns( list->input.phb_Input, ... );
					list->input.phb_Input->nLineHeight = GetFontHeight( list->input_font );
					list->input.phb_Input->nColumns = window->width - ( 2 * l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.BorderSegment[SEGMENT_RIGHT]->width );
					list->input.phb_Input->nWidth = window->width - ( 2 * l.side_pad + l.sent.BorderSegment[SEGMENT_LEFT]->width + l.sent.BorderSegment[SEGMENT_RIGHT]->width 
								+ ( list->send_button_width?list->send_button_width:55 + l.side_pad ) );
				}

				if( key & KEY_PRESSED )
				{
					Widget_WinLogicDoStroke( list, (PTEXT)&stroke );
					//Widget_KeyPressHandler( list, (uint8_t)(KEY_CODE(key)&0xFF), (uint8_t)KEY_MOD(key), (PTEXT)&stroke );
					SmudgeCommon( pc );
				}
			}
			break;
		}
		//LeaveCriticalSec( &list->Lock );
	}
	return 1;
}

void Chat_GetCurrentTime( PCHAT_TIME timebuf )
{
#ifdef _WIN32
	SYSTEMTIME st;
	GetLocalTime( &st );
	timebuf->yr = st.wYear;
	timebuf->mo = (uint8_t)st.wMonth;
	timebuf->dy = (uint8_t)st.wDay;
	timebuf->hr = (uint8_t)st.wHour;
	timebuf->mn = (uint8_t)st.wMinute;
	timebuf->sc = (uint8_t)st.wSecond;
	timebuf->ms = st.wMilliseconds;

	{
		// Get the local system time.

		SYSTEMTIME LocalTime = { 0 };

		SYSTEMTIME GmtTime = { 0 };

		GetSystemTime( &LocalTime );

		{
			DWORD dwType;
			DWORD dwValue;
			DWORD dwSize = sizeof( dwValue );
			HKEY hTemp;
			DWORD dwStatus;

			dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE
			                       , "SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation", 0
			                       , KEY_READ, &hTemp );

			if( (dwStatus == ERROR_SUCCESS) && hTemp )
			{
				dwSize = sizeof( dwValue );
				dwStatus = RegQueryValueEx(hTemp, "ActiveTimeBias", 0
				                          , &dwType
				                          , (PBYTE)&dwValue
				                          , &dwSize );

				RegCloseKey( hTemp );
			}
			else
				dwValue = 0;
			//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\TimeZoneInformation
			// Get the timezone info.
			//TIME_ZONE_INFORMATION TimeZoneInfo;
			//GetTimeZoneInformation( &TimeZoneInfo );

			// Convert local time to UTC.
			//TzSpecificLocalTimeToSystemTime( &TimeZoneInfo,
			//								 &LocalTime,
			//								 &GmtTime );
			// Local time expressed in terms of GMT bias.
			{
			timebuf->zhr = (int8_t)( -( (int)dwValue/60 ) ) ;
			timebuf->zmn = ( dwValue % 60 );
			}
		}
	}
	// latest information...
	// DYNAMIC_TIME_ZONE_INFORMATION  tzi;
	/* 
	typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
			  LONG       Bias;
			  WCHAR      StandardName[32];
			  SYSTEMTIME StandardDate;
			  LONG       StandardBias;
			  WCHAR      DaylightName[32];
			  SYSTEMTIME DaylightDate;
			  LONG       DaylightBias;
			  WCHAR      TimeZoneKeyName[128];
			  BOOLEAN    DynamicDaylightTimeDisabled;
			} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;
    */
	// GetDynamicTimeZoneInformation
#else
	lprintf( "No time buffer built for ______" );
#endif
}

// time in seconds
void Chat_ClearOldMessages( PSI_CONTROL pc, int delete_time )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	//PCONSOLE_INFO list = (PCONSOLE_INFO)GetCommonUserData( pc );
	// this must here gather keystrokes and pass them forward into the
	// opened sentience...
	if( list )
	{
		int deleted = 0;
		PCHAT_CONTEXT context;
		PCHAT_MESSAGE msg;
		CHAT_TIME now;
		int64_t old_limit;
		int c;
		//list->control_offset = 0;
		list->display.message_top = list->message_window->height + list->control_offset;

		Chat_GetCurrentTime( &now );
		old_limit = AbsoluteSeconds( &now ) - delete_time;
		//lprintf( "now stamp is %d/%d/%d %d:%d:%d  %d %d"
		//	, now.mo, now.dy, now.yr, now.hr, now.mn, now.sc, now.zhr, now.zmn );
		//lprintf( "limit = %d", old_limit );
		for( c = 0; context = (PCHAT_CONTEXT)PeekQueueEx( list->contexts, c ); c++ )
		{
			int n;
			//y = list->message_window->height - y;
			//lprintf( WIDE("BEgin draw messages...") );
			for( n = 0; msg = (PCHAT_MESSAGE)PeekQueueEx( context->messages, n ); n++ )
			{
				int64_t msg_time ;
				if( context->sent )
				{ 
					// if hasn't been sent yet...
					if( msg->sent_time.yr == 0 )
						continue;
					msg_time = AbsoluteSeconds( &msg->sent_time );
					//lprintf( "seen stamp is %d/%d/%d %d:%d:%d  %d %d"
					//	, msg->sent_time.mo, msg->sent_time.dy, msg->sent_time.yr
					//	, msg->sent_time.hr, msg->sent_time.mn, msg->sent_time.sc
					//	, msg->sent_time.zhr, msg->sent_time.zmn );
				}
				else
				{
					if( !msg->seen )
						break;
					msg_time =   AbsoluteSeconds( &msg->seen_time );
					//lprintf( "seen stamp is %d/%d/%d %d:%d:%d  %d %d"
					//	, msg->seen_time.mo, msg->seen_time.dy, msg->seen_time.yr
					//	, msg->seen_time.hr, msg->seen_time.mn, msg->seen_time.sc
					//	, msg->seen_time.zhr, msg->seen_time.zmn );
				}
				//lprintf( "(check old message) %lld - %lld = %lld", msg_time, old_limit, old_limit - msg_time );
				if( msg->deleted || ( msg_time < old_limit ) )
				{
					if( n == 0 )
					{
						if( msg->formatted_text )
							Release( msg->formatted_text );
						Release( msg->text );
						Release( msg );
						DequeLink( &context->messages );
						n--;
					}
					else
						msg->deleted = TRUE;
					deleted++;
				}
				//else
				//    break;
			}
			if( IsQueueEmpty( &context->messages ) )
			{
				if( c == 0 )
				{
					if( context->formatted_sender )
						Release( context->formatted_sender );
					Release( context->sender );
					Release( context );
					DequeLink( &list->contexts );
					c--;
				}
				else
					context->deleted = TRUE;
			}
			//else
			//	break;
		}
		if( deleted )
			SmudgeCommon( pc );
	}
}

void Chat_SetExpire( PSI_CONTROL pc, int delta_seconds )
{
}

void Chat_TypePastedInput( PSI_CONTROL pc, PTEXT segment )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	Widget_WinLogicDoStroke( list, segment );
	DrawTextEntry( pc, list, TRUE );
}

void Chat_SetSendButtonText( PSI_CONTROL pc, CTEXTSTR text )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	SetControlText( list->send_button, text );
}


void Chat_SetSendButtonImages( PSI_CONTROL pc, Image normal, Image pressed, Image rollover, Image focused )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	SetButtonImages( list->send_button, normal, pressed );
	SetButtonFocusedImages( list->send_button, focused, pressed );
	SetButtonRolloverImages( list->send_button, rollover, pressed );
}

void Chat_SetSendButtonSlicedImages( PSI_CONTROL pc, SlicedImage normal, SlicedImage pressed, SlicedImage rollover, SlicedImage focused )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	SetButtonSlicedImages( list->send_button, normal, pressed );
	SetButtonSlicedFocusedImages( list->send_button, focused, pressed );
	SetButtonSlicedRolloverImages( list->send_button, rollover, pressed );
}

void Chat_SetSendButtonTextOffset( PSI_CONTROL pc, int32_t x, int32_t y )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	SetButtonTextOffset( list->send_button, x, y );
}


void Chat_SetSendButtonSize( PSI_CONTROL pc, uint32_t width, uint32_t height )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	list->send_button_width = width;
	list->send_button_height = height;
}

void Chat_SetSendButtonOffset( PSI_CONTROL pc, int32_t x, int32_t y )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	list->send_button_x_offset = x;
	list->send_button_y_offset = y;
}

void Chat_SetImageViewerPopupCallback( PSI_CONTROL pc, void (CPROC*PopupEvent)( uintptr_t,LOGICAL ), uintptr_t psvEvent )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	list->PopupEvent = PopupEvent;
	list->psvPopup = psvEvent;
}

void Chat_SetImageViewerAutoCloseHandler( PSI_CONTROL pc, void (CPROC*Event)( uintptr_t ), uintptr_t psvEvent )
{
	PCHAT_LIST *ppList = ControlData( PCHAT_LIST*, pc );
	PCHAT_LIST list = (*ppList);
	list->OnImageAutoClose = Event;
	list->psvImageAutoClose = psvEvent;
}

void Chat_UpdateMessageSendTime( PSI_CONTROL pc
													, struct chat_widget_message *msg
													, PCHAT_TIME time )
{
	msg->sent_time = time[0];
	SmudgeCommon( pc );
}

