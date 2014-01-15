#include <stdhdrs.h>
#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"

int CPROC MarkTime( PSENTIENT ps, PTEXT parameters );
int CPROC ReportTime( PSENTIENT ps, PTEXT parameters );
int CPROC StopTime( PSENTIENT ps, PTEXT parameters );

// declare these with spaces so that they are
// not easily referenced from within the terminal...
DECLTEXT( mark, WIDE("mark time") );
DECLTEXT( stopmark, WIDE("stop time") );

#ifndef _WIN32
typedef struct my_sytemtime_struct {
   unsigned short wYear;
   unsigned short wMonth;
   unsigned short wDay;
   unsigned short wHour;
   unsigned short wMinute;
   unsigned short wSecond;
   unsigned short wMilliseconds;
} SYSTEMTIME;
#endif

typedef struct my_time_struct {
   int year;
   int month;
   int day;
   int hour;
   int minute;
   int second;
   int millisecond;
} MYTIME, *PMYTIME;

//-----------------------------------------------------------------------------

TEXTCHAR *months[13] = { NULL, WIDE("january"), WIDE("february"),WIDE("march")    
                     ,WIDE("april"),WIDE("may"),WIDE("june"),WIDE("july")
                     ,WIDE("august"),WIDE("september"), WIDE("october"), WIDE("november")
                     ,WIDE("december") };

void MakeMyTime( PMYTIME result
#ifdef _WIN32
				   , SYSTEMTIME *st
#else
					, struct timeval *tv
#endif
					)
{
#ifdef _WIN32
   result->year 			= st->wYear;
   result->month 		= st->wMonth;
   result->day 			= st->wDay;
   result->hour 			= st->wHour;
   result->minute 		= st->wMinute;
   result->second 		= st->wSecond;
   result->millisecond = st->wMilliseconds;
#else
	struct tm *timething;
	timething = localtime( &tv->tv_sec );

	result->year 		= timething->tm_year;
	result->month 	= timething->tm_mon;
	result->day 		= timething->tm_mday;
	result->hour 		= timething->tm_hour;
	result->minute 	= timething->tm_min;
	result->second 	= timething->tm_sec;
	result->millisecond = 	(tv->tv_usec)/1000;
#endif
}

#ifndef _WIN32
void GetLocalTime( void *timebuf )
{
	SYSTEMTIME *st = (SYSTEMTIME*)timebuf;
	struct timeval tv;
	struct tm timething;
	gettimeofday( &tv, NULL );
	localtime_r( &tv.tv_sec, &timething );
	st->wYear = timething.tm_year + 1900;
	st->wMonth = timething.tm_mon;
	st->wDay = timething.tm_mday;
	st->wHour = timething.tm_hour;
	st->wMinute = timething.tm_min;
	st->wSecond = timething.tm_sec;
	st->wMilliseconds = (tv.tv_usec)/1000;
}
#endif

int ProcessTimeParam( PSENTIENT ps, PTEXT *parameters, PMYTIME time )
{
// valid date/times are...
// <time> 
//    hh:mm:ss.mmm
//    mm:ss.mmm
//    ss.mmm
//    :ss
//    mm
//    mm:
//    .mmm
//    :ss.mmm
//    :mm:ss.mmm
//    mm:ss
//    hh:mm:sss
// mm/dd/yyyy <time>
// month dd, yyyy <time>
// mon dd, <time>
// 
// colons and periods are going to be non parsed...
// 
   PTEXT param;
#ifdef _WIN32
   SYSTEMTIME st;
   GetLocalTime( &st );
   MakeMyTime( time, &st );
#else
	struct timeval tv;
	gettimeofday( &tv, NULL );
	MakeMyTime( time, &tv );
#endif
   while( ( param = GetParam( ps, parameters ) ) )
   {
      if( strchr( GetText( param ), ':' ) ||
          strchr( GetText( param ), '.' ) )
      {
         // this is a TIME... or should be if things are formatted well...
         TEXTCHAR *c = GetText( param );
         int accums[4], valid, last;
         accums[0] = 0;
         accums[1] = 0;
         accums[2] = 0;
         accums[3] = 0;
         valid = 0;
         last = FALSE;
         while( *c >= '0' && *c <= '9' )
         {
            accums[valid] *= 10;
            accums[valid] += *c - '0';
         }
         if( *c == ':' )
         {
            valid++;
            c++;
         }
         else if( *c == '.' )
         {
            valid++;
            last = TRUE;
            c++;
         }
         else if( *c == 0 )
         {
            
         }
         else
            return FALSE;

         while( *c >= '0' && *c <= '9' )
         {
            accums[valid] *= 10;
            accums[valid] += *c - '0';
         }
         if( !last )
         {
            if( *c == ':' )
            {
               valid++;
               c++;
            }
            else if( *c == '.' )
            {
               valid++;
               last = TRUE;
               c++;
            }
            else
               return FALSE;
         }
         else
         {
          
         }
      }
   }
   return TRUE;
}

//-----------------------------------------------------------------------------
#if 0
void ShowTime( PSENTIENT ps, SYSTEMTIME *st )
{
	PVARTEXT vt;
	PTEXT time;
	vt = VarTextCreate();
	vtprintf( vt, WIDE("Clock:%d/%d/%d %d:%d:%d.%d")
					, st->wMonth, st->wDay, st->wYear
					, st->wHour, st->wMinute, st->wSecond, st->wMilliseconds );
	EnqueLink( &ps->Command->Output, VarTextGet( vt ) );	
	VarTextDestroy( &vt );
}
#endif
//-----------------------------------------------------------------------------

static int ObjectMethod( WIDE("Timer"), WIDE("StopTimer"), WIDE("Show current time from last mark.") )( PSENTIENT ps, PTEXT params )
//int CPROC StopTime( PSENTIENT ps, PTEXT parameters )
{
   DECLTEXTSZ( systime, sizeof( SYSTEMTIME ) );
   systime.flags = 0;
   systime.Next = NULL;
   systime.Prior = NULL;
   systime.data.size = sizeof( SYSTEMTIME );

   GetLocalTime( (SYSTEMTIME*)GetText( (PTEXT)&systime ) );

   AddBinary( ps, ps->Current, (PTEXT)&stopmark, (PTEXT)&systime );
   return 1;
}

//-----------------------------------------------------------------------------

static int ObjectMethod( WIDE("Timer"), WIDE("Mark"), WIDE("Mark current time as start of elapse.") )( PSENTIENT ps, PTEXT params )
//int CPROC MarkTime( PSENTIENT ps, PTEXT parameters )
{
   DECLTEXTSZ( systime, sizeof( SYSTEMTIME ) );
   systime.flags = 0;
   systime.Next = NULL;
   systime.Prior = NULL;
   systime.data.size = sizeof( SYSTEMTIME );

   GetLocalTime( (SYSTEMTIME*)systime.data.data );
#if 0
   ShowTime( ps, (SYSTEMTIME*)systime.data.data );
#endif
   AddBinary( ps, ps->Current, (PTEXT)&mark, (PTEXT)&systime );
	AddBinary( ps, ps->Current, (PTEXT)&stopmark, NULL ); // clear stoptime
#if 0
	{
		PTEXT pIndTime;
      SYSTEMTIME *stThen;
		pIndTime = GetVariable( ps->Current->pVars, mark.data.data );
		stThen = (SYSTEMTIME*)GetText( GetIndirect( pIndTime ) );
		ShowTime( ps, stThen );
	}
#endif
   return 1;
}

//-----------------------------------------------------------------------------

int MonthDays[13] =   { 0,  31,  28,  31,  30,  31,  30
                         ,  31,  31,  30,  31,  30,  31 };
int MonthBaseDays[13]={ 0,   0,  31,  59,  90, 120, 151
                         , 181, 212, 243, 273, 304, 334 };


int DayOfYear( int year, int Month, int Day )
{
   int doy;
   doy = MonthBaseDays[Month] + Day;
   if( ( year % 4 ) == 0 )
   {
      if( ( year % 100 ) == 0 )
      {
         if( ( year % 400 ) == 0 )
         {
            if( Month > 2 )
               doy++;
         }
      }
      else
      {
         if( Month > 2 )
           doy++;
      }
   }
   return doy;
}

int MonthDayofYear( int year, int doy, P_16 Month, P_16 Day )
{
   int i;

   if( doy < 0 || doy > 366 )
      return FALSE;
   if( doy == 0 )
   {
      *Month = 0;
      *Day = 0;
      return TRUE;
   }
   if( ( year % 4 ) == 0 )
   {
      if( ( year % 100 ) == 0 )
      {
         if( ( year % 400 ) == 0 )
         {
            if( *Month > 2 )
               doy--;
         }
      }
      else
      {
         if( *Month > 2 )
           doy--;
      }
   }
   for( i = 1; i < 13; i ++ )
   {
      if( MonthBaseDays[i] > doy )
         break;
   }
   *Month = i - 1;
   *Day = doy - MonthBaseDays[i-1];
   return TRUE;
}

//-----------------------------------------------------------------------------

static int ObjectMethod( WIDE("Timer"), WIDE("Elapse"), WIDE("Show current time from last mark.") )( PSENTIENT ps, PTEXT params )
//int CPROC ReportTime( PSENTIENT ps, PTEXT parameters )
{
   SYSTEMTIME stHolder, *stNow, *stThen, stDel;
   PTEXT pIndTime, pIndStop;
   int doyThen, doyNow, doyDel;
   pIndTime = GetVariable( ps, mark.data.data );
   if( !pIndTime )
   {
      DECLTEXT( msg, WIDE("Previous time 'mark' was not done.") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   GetLocalTime( &stHolder );
   stNow = &stHolder;
   pIndStop = GetVariable( ps, stopmark.data.data );
   if( pIndStop )
   {
      if( GetIndirect( pIndStop ) )
      {
         stNow = (SYSTEMTIME*)GetText( GetIndirect( pIndStop ) );
      }
   }
   stThen = (SYSTEMTIME*)GetText( GetIndirect( pIndTime ) );
   doyThen = DayOfYear( stThen->wYear, stThen->wMonth, stThen->wDay );
   doyNow = DayOfYear( stNow->wYear, stNow->wMonth, stNow->wDay );
   doyDel = doyNow - doyThen;
#if 0
   ShowTime( ps, stThen );
   ShowTime( ps, stNow );
#endif
#define DELOF( what )    ( stDel.what  = stNow->what - stThen->what )
   DELOF( wYear );
   DELOF( wMonth );
   DELOF( wDay );
   DELOF( wHour );
   DELOF( wMinute );
   DELOF( wSecond );
   DELOF( wMilliseconds );
#if 0
   ShowTime( ps, &stDel );
#endif
   // consider millisecond....
   if( stDel.wMilliseconds > 64000 )
   {
      stDel.wMilliseconds += 1000;
      stDel.wSecond--;
   }
   if( stDel.wSecond > 65000 )
   {
      stDel.wSecond += 60;
      stDel.wMinute--;
   }
   if( stDel.wMinute > 65000 )
   {
      stDel.wMinute += 60;
      stDel.wHour--;
   }
   if( stDel.wHour > 65000 )
   {
      stDel.wHour += 24;
      doyDel--;
   }
   if( doyDel < 0 )
   {
      doyDel += 365; // wrong!
      stDel.wYear--;
   }
   MonthDayofYear( stNow->wYear, doyDel, &stDel.wMonth, &stDel.wDay );
   if( stDel.wYear > 65000 )
   {
      DECLTEXT( msg, WIDE("Fatal error computing dates...") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   // otherwise now stDel contains the delta between last mark and now...
   {
		PVARTEXT vt;
		vt = VarTextCreate();
      if( stDel.wYear )
      {
         vtprintf( vt, WIDE("%d year%s%d month%s%d day%s%d hour%s%d minute%s and %d second%s")
                                    , stDel.wYear
                                    , ((stDel.wYear == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wMonth
                                    , ((stDel.wMonth == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wDay
                                    , ((stDel.wDay == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wHour
                                    , ((stDel.wHour == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wMinute
                                    , ((stDel.wMinute == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wSecond
                                    , ((stDel.wSecond == 1)?WIDE(""):WIDE("s"))
                                     );
      }
      else if( stDel.wMonth )
      {
         vtprintf( vt, WIDE("%d month%s%d day%s%d hour%s%d minute%sand %d second%s")
                                    , stDel.wMonth
                                    , ((stDel.wMonth == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wDay
                                    , ((stDel.wDay == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wHour
                                    , ((stDel.wHour == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wMinute
                                    , ((stDel.wMinute == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wSecond
                                    , ((stDel.wSecond == 1)?WIDE(""):WIDE("s"))
                                     );
      }
      else if( stDel.wDay )
      {
         vtprintf( vt, WIDE("%d day%s%d hour%s%d minute%sand %d second%s")
                                    , stDel.wDay
                                    , ((stDel.wDay == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wHour
                                    , ((stDel.wHour == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wMinute
                                    , ((stDel.wMinute == 1)?WIDE(" "):WIDE("s "))
                                    , stDel.wSecond
                                    , ((stDel.wSecond == 1)?WIDE(""):WIDE("s"))
                                     );
      }
      else if( stDel.wHour )
      {
         vtprintf( vt, WIDE("%d:%d:%d.%03d")
                                    , stDel.wHour
                                    , stDel.wMinute
                                    , stDel.wSecond
                                    , stDel.wMilliseconds
                                     );
      }
      else if( stDel.wMinute )
      {
         vtprintf( vt, WIDE("%d:%d.%03d")
                                    , stDel.wMinute
                                    , stDel.wSecond
                                    , stDel.wMilliseconds
                                     );
      }
      else if( stDel.wSecond )
      {
         vtprintf( vt, WIDE("%d.%03d")
                                    , stDel.wSecond
                                    , stDel.wMilliseconds
                                     );
      }
      else
         vtprintf( vt, WIDE("%d milliseconds")
                                    , stDel.wMilliseconds
                                     );

      if( ps->pLastResult )
         LineRelease( ps->pLastResult );
    	EnqueLink( &ps->Command->Output
			      , SegDuplicate( ps->pLastResult = VarTextGet( vt ) ) );
		VarTextDestroy( &vt );
   }
   return 1;
}

//-----------------------------------------------------------------------------


static int OnCreateObject( WIDE("clock"), WIDE("Your basic chronometer") )( PSENTIENT ps, PENTITY pe, PTEXT parameters )
//int CPROC InitClock( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	// this routine is passed an object already named, with the default 
	// description copied...
	// the sentience passed is the creator of the new object...
	// passed for referencig paramters from...
	PSENTIENT ps2;
	ps2 = CreateAwareness( pe );
	UnlockAwareness( ps2 );
	return 0;
}

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
   //RegisterObject( WIDE("Clock"), WIDE("Your basic chronometer..."), InitClock );
   return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterObject( WIDE("Clock") );
}



// $Log: timer.c,v $
// Revision 1.13  2005/02/21 12:09:04  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.12  2005/01/17 09:01:32  d3x0r
// checkpoint ...
//
// Revision 1.11  2004/09/27 16:06:57  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.10  2003/11/08 00:09:41  panther
// fixes for VarText abstraction
//
// Revision 1.9  2003/03/25 08:59:03  panther
// Added CVS logging
//
