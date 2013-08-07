#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <stdhdrs.h>

#include <sharemem.h>
#include <msgclient.h>
#include <controls.h>
#include <imglib/fontstruct.h>
#include <image.h>
#include <idle.h>
#include <timers.h>
#include "sqlstruc.h"
#include <pssql.h>

//#include sqlproxy.h"
#define MSG_PACKET_SIZE 1024

typedef struct global_data_tag
{
	struct {
		unsigned int bUseSQL : 1;
		unsigned int bReady : 1;
		unsigned int bFatalError : 1;
		unsigned int bWaitingForResult : 1;
		unsigned int bShown : 1;  // No DisplayFrame
		unsigned int bHidden : 1; // HideCommon/RevealCommon
	} flags;
   _32 SQLMsgBase;
	//HWND hWndProxy
   //   , hWnd;  // handle of My window...
   //HINSTANCE hInstance;
   CRITICALSECTION cs;
   //int result_len;
	//TEXTSTR result_data;
	//int record_count;
   //char **fields;
   //char **record_result_data;

	_32 fail_timer, recover_timer;
	// rather generic information...
	// scale it down in time... but for now this
   // was the general parameters for gui.c in minicom4.
	BYTE byFontSizeInt; // last size we calculated hfn for.
	SFTFont hfn; // this is the current font we're using for text on the window.
	//HDC hdc;
   PCOMMON frame;
   //HWND hGlobalWnd;
	//RECT rc;
	_32 w, h;
   _32 x, y;
   //HBRUSH hbr;
   CDATA dwBkRGBInt;
   CDATA dwFgRGBInt;
   CDATA dwOldTextRGB;
   CDATA dwOldBkRGB;
   char szMessageInt[256];
   int Rv; // Return_value;
} LOCAL;

static LOCAL l;

// possible values of Rv
#define RV_SQL_CANCEL  -1

#define RV_SQL_DATA 10
#define RV_SQL_ERROR 11
#define RV_SQL_SUCCESS 12
#define RV_SQL_MORE 13
#define RV_SQL_RECORD 14
#define RV_SQL_NODATA 15
// need to make a window handle to receive messages....
// now - the query should take short amounts of time, and
// therefore the window should not go through the work of
// showing itself... perhaps attach a timer, and if the query
// is outstanding for more than 1 second, then show the window
// whose content is probably "Waiting for SQL response..."

//-------------------------------------------------------------------------



//Draw the window
static
  void
    FillInWindow ( BOOL   fUseParams
                 , DWORD  dwBkRGB
                 , DWORD  dwFgRGB
                 , LPSTR  szMessage
                 , BYTE   byFontSize
                 )
{

	// repeated fUseParams should be able to cause blinking
	// text, scrolling messages (on character boundries), 
	// scaling fonts (course since we're computing font size based on
	// line count, no width count.
   Image surface = GetControlSurface( l.frame );
	if (fUseParams)
	{
		BYTE byFontSizeInt; // temp var to test delta vs global.
		if( l.dwBkRGBInt != dwBkRGB )
		{
			l.dwBkRGBInt = dwBkRGB;
		}

		if( l.dwFgRGBInt != dwFgRGB )
			l.dwFgRGBInt = dwFgRGB;

		strncpy(l.szMessageInt, szMessage,sizeof(l.szMessageInt));
      l.szMessageInt[sizeof(l.szMessageInt)-1] = 0;

		byFontSizeInt = ( byFontSize > 1 ) ? byFontSize : 1;

		// if it's a new size, remake the font.
		if( l.byFontSizeInt != byFontSizeInt )
		{
			l.byFontSizeInt = byFontSizeInt;
			l.hfn = RenderFontFile( WIDE("courbd.ttf"), (l.w/5)/byFontSizeInt, (l.h*3/2)/byFontSizeInt, 2 );
		}
		// maybe we didn't create a new font this time
		// but still need to select it into context.
	}
	//Fill in "window" wih background color
	BlatColor( surface, 0, 0, l.w, l.h, l.dwBkRGBInt );
	{
		_32 w, h;
      S_32 x;
		GetStringSizeFont( l.szMessageInt, &w, &h, l.hfn );
		x = l.w - w;
		if( x < 0 )
			x = 0;

		PutStringFont( surface, ( x ) / 2, (l.h - h ) / 2, l.dwFgRGBInt, 0, l.szMessageInt, l.hfn );
	}
}


static int CPROC DrawFrame( PCOMMON pc )
{
	if( !l.SQLMsgBase )
		FillInWindow ( TRUE, Color(255,0,0), Color( 255,255,255 )
						 , WIDE("Waiting for\nSQL Proxy...")
						 , 2 );
	else if( l.flags.bWaitingForResult )
	{
		FillInWindow ( TRUE, Color(255,0,0), Color( 255,255,255 )
						 , WIDE("Waiting for\nSQL Result...")
						 , 2 );
	}
	//FillInWindow ( FALSE, 0UL, 0UL, NULL, 0 );
   return 1;
}


//-------------------------------------------------------------------------

static int InitSQLStub( void )
{
	if( l.flags.bFatalError )
      return FALSE;
	if( !l.flags.bReady )
	{
	   l.flags.bUseSQL = 1;
		if( !l.flags.bUseSQL )
		{
         l.flags.bFatalError = 1;
			return FALSE;
		}
		l.flags.bReady = 1;
	}
	return l.flags.bReady;
}

//-------------------------------------------------------------------------

static int GenerateCommand( char *command, int nSQLCommand )
{
	if( InitSQLStub() )
	{
		int first = 1;
		int len;
		if( command )
			len = strlen( command ) + 1;
		else
			len = 0;
		while( len )
		{
			if( len > MSG_PACKET_SIZE )
			{
				//Log2( WIDE("Len: %d cmd: %s"), len, command );
				//Log( WIDE("Sending a fragment...") );
				if( first )
				{
					//Log( WIDE("Sending WM_SQL_DATA_START") );
					if( !SendServerMessage( l.SQLMsgBase + WM_SQL_DATA_START
												  , command, MSG_PACKET_SIZE ) )
					{
						Log( WIDE("Failed to post message...") );
						l.flags.bReady = FALSE;
                  return FALSE;
					}
					first = 0;
				}
				else
				{
				   //Log( WIDE("Sending WM_SQL_DATA_MORE") );
					if( !SendServerMessage( l.SQLMsgBase + WM_SQL_DATA_MORE
												  , command, MSG_PACKET_SIZE ) )
					{
						Log( WIDE("Failed to post message...") );
						l.flags.bReady = FALSE;
                  return FALSE;
					}
				}
				command += MSG_PACKET_SIZE;
				len -= MSG_PACKET_SIZE;
			}
			else
				break;
		}
		l.Rv = 0;// make sure we wait for result.
		//Log( WIDE("Final bit of data going out in atom...") );
		if( !SendServerMessage( l.SQLMsgBase + nSQLCommand
									  , command, len ) )
		{
			l.flags.bReady = FALSE;
			return FALSE;
		}
	}
   return TRUE;
}

//-------------------------------------------------------------------------

void Collect( P_32 buf, _32 len )
{
	//char buf[257];
	char *newbuf;
	if( !buf || !len )
	{
      Log( WIDE("Returning without collectinl..."));
		return;
	}
	Log1( WIDE("Collecting: \"%s\""), buf );
	if( len )
	{
      //Log2( WIDE("has a length... %d %d"), len, len+l.result_len );
		newbuf = Allocate( len + l.result_len + 1 );
		if( !newbuf )
		{
			Log( WIDE("Damn we ran outa memory!") );
			(*(int*)0)=0;
		}
		if( l.result_data )
		{
         //Log( WIDE("Copying old buffer...") );
			memcpy( newbuf, l.result_data, l.result_len );
		}
      //Log4( WIDE("Copying new buffer at %d (%d) %p %p")
      //     , l.result_len, len + 1
      //     , newbuf, buf );
		memcpy( newbuf + l.result_len, buf, len + 1 );
		l.result_len += len;
		if( l.result_data )
		{
         //Log1( WIDE("Releasing old data %p"), l.result_data );
			Release( l.result_data );
		}
      //Log2( WIDE("Setting new data %p %s"), newbuf, newbuf );
		l.result_data = newbuf;
	}
	//else
   //   Log( WIDE("Data was 0 length...") );
}

//-------------------------------------------------------------------------

static int CPROC ClientEventHandlerFunction(_32 SourceID
														 , _32 MsgID
														 , _32*params
														 , _32 paramlen)
{
   lprintf( WIDE("SQL Result .... %d"), MsgID );
	switch( MsgID )
	{
	case MSG_DispatchPending:
	   // no more messages in the stream to processs
      // how can I disable dispatch_pending notification?
      return TRUE;
	case WM_SQL_DATA_START:
      Collect( params, paramlen );
		return FALSE;
	case WM_SQL_DATA_MORE:
      Collect( params, paramlen );
      return FALSE;
	case WM_SQL_RESULT_ERROR:
      Collect( params, paramlen );
		l.Rv = RV_SQL_ERROR;
		if( !l.flags.bHidden )
		{
			HideCommon( l.frame );
         l.flags.bHidden = 1;
		}
      return FALSE;
	case WM_SQL_RESULT_SUCCESS:
      Collect( params, paramlen );
      l.Rv = RV_SQL_SUCCESS;
		if( !l.flags.bHidden )
		{
			HideCommon( l.frame );
         l.flags.bHidden = 1;
		}
		return FALSE;
	case WM_SQL_RESULT_NO_DATA:
      l.Rv = RV_SQL_NODATA;
		if( !l.flags.bHidden )
		{
			HideCommon( l.frame );
         l.flags.bHidden = 1;
		}
      return FALSE;
	case WM_SQL_RESULT_DATA:
      Collect( params, paramlen );
		l.Rv = RV_SQL_DATA;
		if( !l.flags.bHidden )
		{
			HideCommon( l.frame );
         l.flags.bHidden = 1;
		}
      return FALSE;
	case WM_SQL_RESULT_RECORD:
      Collect( params, paramlen );
		l.Rv = RV_SQL_RECORD;
		if( !l.flags.bHidden )
		{
			HideCommon( l.frame );
         l.flags.bHidden = 1;
		}
      return FALSE;
	case WM_SQL_RESULT_MORE:
		Collect( params, paramlen );
		l.Rv = RV_SQL_MORE;
		if( !l.flags.bHidden )
		{
			HideCommon( l.frame );
         l.flags.bHidden = 1;
		}
      break;
	default:
      lprintf( WIDE("Unknown message %d"), MsgID );
      break;
	}

   return FALSE;
}

//-------------------------------------------------------------------------

void CPROC ConnectionTimer( PTRSZVAL psv )
{
	if( !l.SQLMsgBase )
		l.SQLMsgBase = LoadServiceEx( WIDE("SQL"), ClientEventHandlerFunction );
	if( psv == 100 && ( !l.SQLMsgBase || l.flags.bWaitingForResult ) )
	{
		if( !l.flags.bShown )
		{
         l.flags.bShown = 1;
			l.flags.bHidden = 0;
			DisplayFrame( l.frame );
		}
		else if( l.flags.bHidden )
		{
			RevealCommon( l.frame );
         l.flags.bHidden = 0;
		}
		//ShowWindow( hWnd, SW_SHOW );
		// will have to get the renderer for this to set topmost...
		//SetWindowPos( hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE );
		// transfer to a higher frequency timer...
		RemoveTimer( l.fail_timer );
		//KillTimer( hWnd, 100 );
		if( !l.SQLMsgBase )
			l.recover_timer = AddTimer( 250, ConnectionTimer, 101 );
		else
			return;
	}
	// handle proxy window find/restore
	{
		if( l.SQLMsgBase )
		{
         RemoveTimer( l.recover_timer );
			//KillTimer( hWnd, 101 );
			if( l.flags.bShown )
			{
				HideCommon( l.frame );
				l.flags.bHidden = 1;
			}
			//ShowWindow( hWnd, SW_HIDE );
         AddTimer( 1500, ConnectionTimer, 100 );
			//SetTimer( hWnd, 100, 1500, MB_OK );
			FillInWindow ( TRUE, Color(255,0,0), Color( 255,255,255 )
							 , WIDE("Waiting for\nSQL Result...")
							 , 2 );
		}
	}
}

//-------------------------------------------------------------------------

int InitSQLStubWindow( void )
{
	_32 w, h;
   int nw, nh;
	// window is independant - but it forces itself
	// as topmost, so that should be okay...??
	do
	{
		if( !l.SQLMsgBase )
			l.SQLMsgBase = LoadServiceEx( WIDE("SQL"), ClientEventHandlerFunction );

		if( !l.frame )
		{
			GetDisplaySize( &w, &h );
			l.Rv = 0;
			nw = w * 3 / 4;
			nh = h * 5 / 8;
			l.frame = CreateFrame( WIDE("SQL Error")
										, l.x = (w - nw) / 2, l.y = ( h - nh ) / 2
										, nw, nh
										, BORDER_NOMOVE|BORDER_NOCAPTION|BORDER_BUMP, NULL );
			if( !l.frame )
			{
				//l.flags.bFatalError = TRUE;
				lprintf( WIDE("Failed to create SQL Stub Window. - FATAL ERROR") );
				return FALSE;
			}
			l.w = nw;
			l.h = nh;
			AddCommonDraw( l.frame, DrawFrame );
			if( !l.SQLMsgBase )
			{
				DisplayFrame( l.frame );
			}
			// wait for the proxy window to open...
			if( l.Rv ) // aborted!
				return FALSE;
		}

		if( l.SQLMsgBase && l.flags.bShown )
		{
			HideCommon( l.frame );
			l.flags.bHidden = 1;
			return TRUE;
		}
		else if( !l.SQLMsgBase && !l.flags.bShown )
		{
			DisplayFrame( l.frame );
			l.flags.bShown = TRUE;
		}
		// quarter second query for service.
      if( !l.SQLMsgBase )
			WakeableSleep( 250 );
	}
	while( !l.SQLMsgBase );
	return TRUE;
}


//-------------------------------------------------------------------------

int WaitForResponce( void )
{
   // default 1 second ticker...
	_32 tick = GetTickCount() + 1500;

   l.flags.bWaitingForResult = 1;
	while( l.Rv == 0 )
	{
		if( tick < GetTickCount() )
		{
			if( l.flags.bHidden )
			{
				RevealCommon( l.frame );
            l.flags.bHidden = TRUE;
			}
			else if( !l.flags.bShown )
			{
				DisplayFrame( l.frame );
            l.flags.bShown = TRUE;
			}
         // update message from time to time...
		}
		WakeableSleep( 250 );
	}
	return (l.Rv == RV_SQL_SUCCESS) ||
		(l.Rv == RV_SQL_DATA ) ||
		(l.Rv == RV_SQL_MORE ) ||
		(l.Rv == RV_SQL_RECORD ) ?TRUE:FALSE;
}

//-------------------------------------------------------------------------
int SQLCommandEx( PODBC odbc, char *command DBG_PASS )
{
	if( !InitSQLStub() ||
	    !InitSQLStubWindow() )
		return FALSE;
	if( GenerateCommand( command, WM_SQL_COMMAND ) )
	{
		if( WaitForResponce() )
		{
         l.result_len = 0;
         l.result_data = NULL;
         return TRUE;
		}
	}
   return FALSE;
}

int DoSQLCommandEx( char *command DBG_PASS )
{
   return SQLCommandEx( NULL, command DBG_RELAY );
}


//-------------------------------------------------------------------------
// parameters to this are pairs of "name", WIDE("value")
// the last pair's name is NULL, and value does not matter.
// insert values into said table.
int DoSQLInsert( char *table, ... )
{
	if( !InitSQLStub() ||
		!InitSQLStubWindow() )
		return FALSE;
	{
		int notfirst = 0;
		int nCmdLen;
		char *command;
		int nVarLen = 2;
		char *vars;
		int nValLen = 2;
		int nValOfs;
      int quote;
		char *vals;
		int first;
      int second_half;
		char *varname;
		char *varval;
		va_list args;

      first = 1;
		va_start( args, table );
		for( varname = va_arg( args, char * );
			  varname;
			  varname = va_arg( args, char * ) )
		{
         quote = va_arg( args, int );
			varval = va_arg( args, char * );
			if( first )
			{
				first = 0;
			}
			else
			{
				nVarLen++; // ','
				nValLen++; // ','
			}
			//Log5( WIDE("Length is: %d  %d,%d %p %p"),quote, nVarLen,nValLen, varname, varval);
			nVarLen += strlen( varname );
			nValLen += strlen( varval );
			//Log3( WIDE("Length is: %d  %d,%d"),quote, nVarLen,nValLen);
			if( quote )
            nValLen+=2;
		}
		command = Allocate( 12 + strlen( table ) + nVarLen + 8 + nValLen );
      //Log2( WIDE("Command = %p (%d)"), command, 12 + strlen( table ) + nVarLen + 7 + nValLen );
		nCmdLen = sprintf( command, WIDE("Insert into %s"), table );
		second_half = nValOfs = nCmdLen + nVarLen;
		nValOfs += sprintf( command+nValOfs, WIDE(" values") );
		//Log2( WIDE("Command is now: %s %s"), command, command+second_half );
		first = 1;
		va_start( args, table );
		for( varname = va_arg( args, char * );
			  varname;
			  varname = va_arg( args, char * ) )
		{
         quote = va_arg( args, int );
			varval = va_arg( args, char *);
			nCmdLen += sprintf( command + nCmdLen, WIDE("%s%s"), first?"(":",", varname );
			nValOfs += sprintf( command + nValOfs, WIDE("%s%s%s%s")
									, first?"(":","
									, quote?"\'":""
									, varval
									, quote?"\'":""
									);
			//Log2( WIDE("Command is now: %s %s"), command, command+second_half );
			first = 0;
		}
      command[nCmdLen] = ')';
      command[nValOfs] = ')';
      command[nValOfs+1] = 0;
		//Log1( WIDE("Command is now: %s"), command );

		if( GenerateCommand( command, WM_SQL_COMMAND ) )
		{
			if( WaitForResponce() )
			{
				l.result_len = 0;
				l.result_data = NULL;
				return TRUE;
			}
		}
	}
   return FALSE;
}


//-------------------------------------------------------------------------
// should pass to this a &(char*) which starts as NULL for result.
// result is FALSE on error
// result is TRUE on success, and **result is updated to 
// contain the resulting data.
#undef DoSQLQuery
int DoSQLQueryEx( CTEXTSTR query, CTEXTSTR *result DBG_PASS )
{
	if( !InitSQLStub() ||
	    !InitSQLStubWindow() )
      return FALSE;
	if( GenerateCommand( query, WM_SQL_QUERY ) )
	{
		if( WaitForResponce() )
		{
         //Log( WIDE("QUery Resulting data...") );
			*result = l.result_data;
         l.result_len = 0;
         l.result_data = NULL;
         return TRUE;
		}
	}
   return FALSE;
}

//-------------------------------------------------------------------------
// should pass to this a &(char*) which starts as NULL for result.
// result is FALSE on error
// result is TRUE on success, and **result is updated to 
// contain the resulting data.
#undef GetSQLResult
int GetSQLResult( CTEXTSTR *result )
{
	if( !InitSQLStub() ||
	    !InitSQLStubWindow() )
      return FALSE;
	if( *result )
	{
		Release( *result );
		*result = NULL;
	}
	if( GenerateCommand( NULL, WM_SQL_MORE ) )
	{
		if( WaitForResponce() )
		{
			if( l.Rv == RV_SQL_NODATA )
			{

			}
			else if( l.Rv == RV_SQL_MORE )
			{
	         Log( WIDE("Resulting data...") );
				*result = l.result_data;
      	   l.result_len = 0;
         	l.result_data = NULL;
			}
			else
			{
				Log( WIDE("SEQUENCE ERROR! or no more data...") );
			}
         return TRUE;
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------
//
int GetSQLRecordResult( char ***result, int *count, char ***fields )
{
	if( !result )
		return FALSE;
	if( !InitSQLStub() ||
		!InitSQLStubWindow() )
		return FALSE;
	if( *result )
	{
		if( **result )
		{
         // free the subsequent strings...
			Release( **result );
		}
		Release( *result );
		*result = NULL;
	}
	if( GenerateCommand( NULL, WM_SQL_MORE ) )
	{
		if( WaitForResponce() )
		{
			if( l.Rv == RV_SQL_MORE )
			{
	         Log( WIDE("Resulting data...") );
				*result = l.record_result_data;
      	   l.result_len = 0;
         	l.result_data = NULL;
			}
			else
			{
				Log( WIDE("SEQUENCE ERROR! or no more data...") );
			}
         return TRUE;
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------
// should pass to this a &(char*) which starts as NULL for result.
// result is FALSE on error
// result is TRUE on success, and **result is updated to 
// contain the resulting data.
int GetSQLError( CTEXTSTR *result )
{
	if( !InitSQLStub() ||
	    !InitSQLStubWindow() )
      return FALSE;
	if( *result )
	{
		Release( *result );
		*result = NULL;
	}
	if( GenerateCommand( NULL, WM_SQL_GET_ERROR ) )
	{
		if( WaitForResponce() )
		{
			if( l.Rv == RV_SQL_MORE )
			{
	         Log( WIDE("Resulting data...") );
				*result = l.result_data;
      	   l.result_len = 0;
         	l.result_data = NULL;
			}
			else
			{
				Log( WIDE("SEQUENCE ERROR! or no more data...") );
			}
         return TRUE;
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------
