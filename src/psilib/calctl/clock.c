
#define g global_calender_structure
#define USE_IMAGE_INTERFACE (g.MyImageInterface?g.MyImageInterface:(g.MyImageInterface=GetImageInterface() ))
//#define USE_RENDER_INTERFACE g.MyDisplayInterface
#include <stdhdrs.h>
#include <deadstart.h>
#include <controls.h>
#include <psi.h>
#include <timers.h>
#include <psi/clock.h>
#include <sqlgetoption.h>
#define CLOCK_CORE
#include "local.h"

PSI_CLOCK_NAMESPACE 

extern CONTROL_REGISTRATION clock_control;

//--------------------------------------------------------------------------
//cpg26dec2006 calctl\clock.c(20): Warning! W202: Symbol 'Months' has been defined, but not referenced
/*
static TEXTCHAR Months[13][10] = { WIDE("default")
                  , WIDE("January")
                  , WIDE("February")
                  , WIDE("March")
                  , WIDE("April")
                  , WIDE("May")
                  , WIDE("June")
                  , WIDE("July")
                  , WIDE("August")
                  , WIDE("September")
                  , WIDE("October")
                  , WIDE("November")
                  , WIDE("December") };
*///cpg26dec2006 calctl\clock.c(33): Warning! W202: Symbol 'Days' has been defined, but not referenced
/*static TEXTCHAR Days[7][10] = {WIDE("Sunday"), WIDE("Monday"), WIDE("Tuesday"), WIDE("Wednesday")
               , WIDE("Thursday"), WIDE("Friday"), WIDE("Saturday") };
*/
//DECLTEXTSZ( timenow, 80 );

static PTEXT GetTime( PCLOCK_CONTROL clock, int bNewline ) /*FOLD00*/
{
	static PTEXT timenow;
	if( !timenow )
      timenow = SegCreate( 80 );
//   PTEXT pTime;
#ifdef WIN32
	{
		SYSTEMTIME st;
	//   pTime = SegCreate( 38 );
		GetLocalTime( &st );
		clock->time_data.sc = (_8)st.wSecond;
		clock->time_data.mn = (_8)st.wMinute;
		clock->time_data.hr = (_8)st.wHour;
		//clock->time_data.doy = st.wDayOfWeek;
		clock->time_data.dy = (_8)st.wDay;
		clock->time_data.mo = (_8)st.wMonth;
		clock->time_data.yr = st.wYear;
		clock->time_data.ms = st.wMilliseconds;
	/*
	 n = sprintf( pTime->data.data, WIDE("%s, %s %d, %d, %02d:%02d:%02d"),
	 Days[st.wDayOfWeek], Months[st.wMonth],
	 st.wDay, st.wYear
	 , st.wHour, st.wMinute, st.wSecond );
	 */
		{
			static int last_second;
			static int prior_milli;
			if( clock->flags.bHighTime && ( last_second == st.wSecond || ( ( last_second+1)%60 == st.wSecond && prior_milli ) ) )
			{
				timenow->data.size = snprintf( timenow->data.data, 80*sizeof(TEXTCHAR), WIDE("%02d/%02d/%d%c%02d:%02d:%02d.%03d%s")
													  , st.wMonth, st.wDay, st.wYear
													  , bNewline?'\n':' '
													  , st.wHour
													  , clock->flags.bAmPm?(st.wHour == 0?12:(st.wHour > 12?st.wHour-12:st.wHour)):st.wHour
													  , st.wMinute, st.wSecond, st.wMilliseconds
													  , clock->flags.bAmPm?((st.wHour >= 12)?"P":"A"):""
													  );
				prior_milli = 1;
			}
			else
			{
				timenow->data.size = snprintf( timenow->data.data, 80*sizeof(TEXTCHAR), WIDE("%02d/%02d/%d%c%02d:%02d:%02d%s")
													  , st.wMonth, st.wDay, st.wYear
													  , bNewline?'\n':' '
													  , clock->flags.bAmPm?(st.wHour == 0?12:(st.wHour > 12?st.wHour-12:st.wHour)):st.wHour
													  , st.wMinute, st.wSecond
													  , clock->flags.bAmPm?((st.wHour >= 12)?"P":"A"):""
													  );
				prior_milli = 0;
			}
			last_second = st.wSecond;
		}
		return timenow;
	}
#else
	{
	//struct timeval tv;
		struct tm *timething;
		time_t timevalnow;
		time(&timevalnow);
		timething = localtime( &timevalnow );
      clock->time_data.sc = timething->tm_sec;
      clock->time_data.mn = timething->tm_min;
      clock->time_data.hr = timething->tm_hour;
      clock->time_data.dy = timething->tm_mday;
      clock->time_data.mo = timething->tm_mon;
      clock->time_data.yr = timething->tm_year;
      clock->time_data.ms = 0;
		strftime( timenow->data.data
				  , 80
				  , bNewline
					?"%m/%d/%Y\n%H:%M:%S"
					:"%m/%d/%Y %H:%M:%S"
				  , timething );
		return timenow;
	}
#endif
}

//------------------------------------------------------------------------

static int CPROC DrawClock( PCOMMON pc )
{
	Image surface = GetControlSurface( pc );
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		_32 w, h;
		int line_count = 0;
		int lines = 0;
		//PTEXT szNow = pClk->time;
      TEXTSTR line;

		if( pClk->analog_clock )
		{
			//lprintf( "Draw." );
			DrawAnalogClock( pc );
			return 1;
		}

		//else
		{
			//lprintf( "Get to draw before being analog?" );
			for( line = GetText( pClk->time ); line; line = strchr( line, '\n' ) )
			{
				lines++;
				line++;
			}
			for( line = GetText( pClk->time ); line; line = strchr( line, '\n' ) )
			{
				TEXTCHAR* trunk;
				if( line != GetText( pClk->time ) )
					line++;
				trunk = strchr( line, '\n' );
				if( trunk )
					trunk[0] = 0;
				GetStringSizeFont( line, &w, &h, GetCommonFont( pc ) );

				if( pClk->back_image )
					BlotScaledImageAlpha( surface, pClk->back_image, ALPHA_TRANSPARENT );
				else 
					BlatColorAlpha( surface, 0, 0, surface->width, surface->height, pClk->backcolor );
				//DebugBreak();
				PutStringFontEx( surface
									, (SUS_GT(surface->width,S_32,w,_32)?(( surface->width - w ) / 2):0)
									, (SUS_GT(surface->height,S_32,h,_32)?(( surface->height - ( h * lines ) ) / 2):0) + ( line_count * h )
									, pClk->textcolor, 0
									, line, strlen( line )
									, GetCommonFont( pc ) );
				if( trunk )
					trunk[0] = '\n';
				line_count++;
			}
		}
	}
	return TRUE;
}


static void CPROC Update( PTRSZVAL psvPC )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, (PCOMMON)psvPC );
	if( pClk )
	{
		if( !pClk->flags.bStopped )
		{
         int no_update = 0;
			//( pClk->time )
			// LineRelease( pClk->time );
			pClk->time = GetTime(pClk, TRUE);

			if( !pClk->analog_clock )
			{
				//static TEXTCHAR *text;
				if( pClk->last_time && StrCmp( pClk->last_time, GetText( pClk->time ) ) == 0 )
				{
               no_update = 1;
				}
				else
				{
					if( pClk->last_time )
						Release( pClk->last_time );
					pClk->last_time = StrDup( GetText( pClk->time ) );
				}
			}

         if( !no_update )
				SmudgeCommon( (PCOMMON)psvPC );

			if( pClk->analog_clock )
			{
            // +100 from now... (less than 10/sec)
				RescheduleTimer( 100 );
			}
			else
			{
            if( pClk->flags.bHighTime )
					RescheduleTimer( 40 );
				else
					RescheduleTimer( 250 );
			}
		}
	}
}

int CPROC InitClock( PCOMMON pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, (PCOMMON)pc );
	// 7 lines, width non-specified...
#ifndef __NO_OPTIONS__
	pClk->flags.bHighTime = SACK_GetProfileIntEx( WIDE( "SACK" ), WIDE( "PSI/Clock Control/Default to high resolution time?" ), 0, TRUE );
#endif
	SetCommonFont( pc
					 , RenderFontFile( NULL
										  , (GetControlSurface( pc )->width -10) / 6
										  , (GetControlSurface( pc )->height -10)/ 2
										  ,3 ) );
	SetCommonUserData( pc, AddTimer( 50, Update, (PTRSZVAL)pc ) );
	SetCommonTransparent( pc, TRUE );
	pClk->textcolor = GetBaseColor( TEXTCOLOR );
   pClk->last_time = NULL; // make sure it's NULL
	return TRUE;
}

void CPROC DestroyClock( PCOMMON pc )
{
	RemoveTimer( (_32)GetCommonUserData( pc ) );
}

CONTROL_REGISTRATION clock_control = { WIDE("Basic Clock Widget")
									  , { { 270, 120 }, sizeof( CLOCK_CONTROL )
										 , BORDER_FIXED|BORDER_NONE|BORDER_NOCAPTION }
									  , InitClock
									  , NULL
									  , DrawClock
									  , NULL
									  , NULL
                             , DestroyClock
};

void SetClockColor( PCOMMON pc, CDATA color )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->textcolor = color;
	}
}

void SetClockBackColor( PCOMMON pc, CDATA color )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->backcolor = color;
	}
}

void SetClockBackImage( PCOMMON pc, Image image )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->back_image = image;
	}

}

void SetClockHighTimeResolution( PCOMMON pc, LOGICAL bEnable )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->flags.bHighTime = bEnable;
	}

}

CDATA GetClockColor( PCOMMON pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      return pClk->textcolor;
	}
   return 0;
}

void StopClock( PCOMMON pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 1;
	}
}

void StartClock( PCOMMON pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 0;
	}
}

void MarkClock( PCOMMON pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 1;
	}
}

void ElapseClock( PCOMMON pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 1;
      // at the time it was stopped... the time needs to be marked.
	}
}

void SetClockAmPm( PSI_CONTROL pc, LOGICAL yes_no )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->flags.bAmPm = yes_no;
	}
}
void SetClockDate( PSI_CONTROL pc, LOGICAL yes_no )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->flags.bDate = yes_no;
	}
}
void SetClockDayOfWeek( PSI_CONTROL pc, LOGICAL yes_no )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->flags.bDayOfWeek = yes_no;
	}
}
void SetClockSingleLine( PSI_CONTROL pc, LOGICAL yes_no )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->flags.bSingleLine = yes_no;
	}
}

PRELOAD( DoRegisterClockControl )
{
   DoRegisterControl( &clock_control );
}

//PUBLIC( _32, LinkClockPlease );

PSI_CLOCK_NAMESPACE_END
