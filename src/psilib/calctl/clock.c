#undef g
#define g global_calender_structure
#ifndef USE_IMAGE_INTERFACE
#  define USE_IMAGE_INTERFACE (g.MyImageInterface?g.MyImageInterface:(g.MyImageInterface=GetImageInterface() ))
#endif
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
static TEXTCHAR Months[13][10] = { "default"
                  , "January"
                  , "February"
                  , "March"
                  , "April"
                  , "May"
                  , "June"
                  , "July"
                  , "August"
                  , "September"
                  , "October"
                  , "November"
                  , "December" };
*///cpg26dec2006 calctl\clock.c(33): Warning! W202: Symbol 'Days' has been defined, but not referenced
/*static TEXTCHAR Days[7][10] = {"Sunday", "Monday", "Tuesday", "Wednesday"
               , "Thursday", "Friday", "Saturday" };
*/
//DECLTEXTSZ( timenow, 80 );

static PTEXT GetTime( PCLOCK_CONTROL clock, int bNewline ) /*FOLD00*/
{
	static PTEXT timenow;
	if( !timenow )
		timenow = SegCreate( 80 );
//	PTEXT pTime;
#ifdef WIN32
	{
		SYSTEMTIME st;
	//	pTime = SegCreate( 38 );
		GetLocalTime( &st );
		clock->time_data.sc = (uint8_t)st.wSecond;
		clock->time_data.mn = (uint8_t)st.wMinute;
		clock->time_data.hr = (uint8_t)st.wHour;
		//clock->time_data.doy = st.wDayOfWeek;
		clock->time_data.dy = (uint8_t)st.wDay;
		clock->time_data.mo = (uint8_t)st.wMonth;
		clock->time_data.yr = st.wYear;
		clock->time_data.ms = st.wMilliseconds;

	/*
	 n = sprintf( pTime->data.data, "%s, %s %d, %d, %02d:%02d:%02d",
	 Days[st.wDayOfWeek], Months[st.wMonth],
	 st.wDay, st.wYear
	 , st.wHour, st.wMinute, st.wSecond );
	 */
		{
			static int last_second;
			static int prior_milli;
			if( clock->flags.bHighTime && ( last_second == st.wSecond || ( ( last_second+1)%60 == st.wSecond && prior_milli ) ) )
			{
				timenow->data.size = tnprintf( timenow->data.data, 80*sizeof(TEXTCHAR), "%02d/%02d/%d%c%02d:%02d:%02d.%03d%s"
													  , st.wMonth, st.wDay, st.wYear
													  , bNewline?'\n':' '
													  , clock->flags.bAmPm?(st.wHour == 0?12:(st.wHour > 12?st.wHour-12:st.wHour)):st.wHour
													  , st.wMinute, st.wSecond, st.wMilliseconds
													  , clock->flags.bAmPm?((st.wHour >= 12)?"P":"A"):""
													  );
				prior_milli = 1;
			}
			else
			{
				timenow->data.size = tnprintf( timenow->data.data, 80*sizeof(TEXTCHAR), "%02d/%02d/%d%c%02d:%02d:%02d%s"
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
		struct timeval tv;
		struct tm tm;
		uint64_t tick = timeGetTime64();
		tv.tv_sec = ( tick  ) / 1000;
		tv.tv_usec =  ( ( tick ) % 1000 ) * 1000;
		localtime_r( &tv.tv_sec, &tm );
		char ftime[80];
		clock->time_data.sc = tm.tm_sec  ;//= st.sc;
		clock->time_data.mn = tm.tm_min  ;//= st.mn;
		clock->time_data.hr = tm.tm_hour ;//= st.hr;
		clock->time_data.dy = tm.tm_mday ;//= st.dy;
		clock->time_data.mo = tm.tm_mon  ;//= st.mo;
		clock->time_data.yr = tm.tm_year ;//= st.yr;
		clock->time_data.ms = tv.tv_usec/1000;

		strftime( ftime
				  , 80
				  , bNewline
					?"%m/%d/%Y\n%H:%M:%S"
					:"%m/%d/%Y %H:%M:%S"
				  , &tm );

#ifdef UNICODE
		{
			TEXTCHAR *tmp = DupCStr( ftime );
			StrCpy( timenow->data.data, tmp );
         Release( tmp );
		}
#else
		StrCpy( timenow->data.data, ftime );
#endif

		return timenow;
	}
#endif
}

//------------------------------------------------------------------------

static int CPROC psiClockDrawClock( PSI_CONTROL pc )
{
	Image surface = GetControlSurface( pc );
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		uint32_t w, h;
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
									, (SUS_GT(surface->width,int32_t,w,uint32_t)?(( surface->width - w ) / 2):0)
									, (SUS_GT(surface->height,int32_t,h,uint32_t)?(( surface->height - ( h * lines ) ) / 2):0) + ( line_count * h )
									, 0
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


static void CPROC ClockUpdate( uintptr_t psvPC )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, (PSI_CONTROL)psvPC );
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
				SmudgeCommon( (PSI_CONTROL)psvPC );

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

int CPROC InitClock( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, (PSI_CONTROL)pc );
	// 7 lines, width non-specified...
#ifndef __NO_OPTIONS__
	pClk->flags.bHighTime = SACK_GetProfileIntEx( "SACK", "PSI/Clock Control/Default to high resolution time?", 0, TRUE );
#endif
	SetCommonFont( pc
					 , RenderFontFile( NULL
										  , (GetControlSurface( pc )->width -10) / 6
										  , (GetControlSurface( pc )->height -10)/ 2
										  ,3 ) );
	SetControlUserData( pc, AddTimer( 50, ClockUpdate, (uintptr_t)pc ) );
	SetControlTransparent( pc, TRUE );
	pClk->textcolor = GetBaseColor( TEXTCOLOR );
	pClk->last_time = NULL; // make sure it's NULL
	AddLink( &g.clocks, pc );
	return TRUE;
}

void CPROC DestroyClock( PSI_CONTROL pc )
{
	RemoveTimer( (uint32_t)GetControlUserData( pc ) );
	DeleteLink( &g.clocks, pc );
}

CONTROL_REGISTRATION clock_control = { "Basic Clock Widget"
									  , { { 270, 120 }, sizeof( CLOCK_CONTROL )
										 , BORDER_FIXED|BORDER_NONE|BORDER_NOCAPTION }
									  , InitClock
									  , NULL
									  , psiClockDrawClock
									  , NULL
									  , NULL
                             , DestroyClock
};

void SetClockColor( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
      pClk->textcolor = color;
	}
}

void SetClockBackColor( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->backcolor = color;
	}
}

void SetClockBackImage( PSI_CONTROL pc, Image image )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->back_image = image;
	}

}

void SetClockHighTimeResolution( PSI_CONTROL pc, LOGICAL bEnable )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bHighTime = bEnable;
	}

}

CDATA GetClockColor( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		return pClk->textcolor;
	}
	return 0;
}

void StopClock( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 1;
	}
}

void StartClock( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 0;
	}
}

void MarkClock( PSI_CONTROL pc )
{
	ValidatedControlData( PCLOCK_CONTROL, clock_control.TypeID, pClk, pc );
	if( pClk )
	{
		pClk->flags.bStopped = 1;
	}
}

void ElapseClock( PSI_CONTROL pc )
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

/* Android support; when the app stops, stop updating timers */
static void OnDisplayPause( "PSI_Clock" TARGETNAME )( void )
{
	INDEX idx;
	PSI_CONTROL clock;
	LIST_FORALL( g.clocks, idx, PSI_CONTROL, clock )
	{
		StopClock( clock );
	}
}

/* Android support; when the app reumes, start updating timers */
static void OnDisplayResume( "PSI_Clock" TARGETNAME)( void )
{
	INDEX idx;
	PSI_CONTROL clock;
	LIST_FORALL( g.clocks, idx, PSI_CONTROL, clock )
	{
		StartClock( clock );
	}
}


//PUBLIC( uint32_t, LinkClockPlease );

PSI_CLOCK_NAMESPACE_END
#undef g
