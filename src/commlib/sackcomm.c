#define USES_OPTION_INTERFACE
#define DO_LOGGING
#ifndef SACKCOMM_SOURCE
#define SACKCOMM_SOURCE
#endif
#define _TRACE_DATA_ // show in and out data literally...
//#define _TRACE_DATA_MIN
#include <stdhdrs.h>
//#include <windows.h>
#include <string.h>
#include <stdlib.h>
#if defined( _WIN32 ) || defined( __LINUX__ )
#include <timers.h>
#endif
#if defined( __LINUX__ )
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h> // O_RDWR
#endif

#define USE_REAL_FUNCTIONS

#include <sqlgetoption.h>
#include <sackcomm.h>

int bLogDataXfer;
int gbLog;

#ifndef USE_REAL_FUNCTIONS // if using real comm functions - this is not needed.
#ifndef __LINUX__
static DCB stDcb;
#else
#endif
#endif

#ifdef __LINUX__
#ifndef B230400
#define B230400 230400
#endif
#endif


typedef struct callback_tag {
	CommReadCallback func;
	uintptr_t psvUserRead;
	int iTimer;
	struct callback_tag *next, **me;
	struct {
		BIT_FIELD skip_read : 1;
	} flags;
} CHANNEL_CALLBACK, *PCHANNEL_CALLBACK;

#define DEFAULT_READ_BUFFER 1024

typedef struct com_tracking_tag {
	TEXTCHAR  portname[24];
	int     iCommId;
	int     iUsers;
	struct {
		BIT_FIELD bUseCarrierDetect : 1;
		BIT_FIELD bHaveCarrier : 1;
		BIT_FIELD bInRead : 1;
		BIT_FIELD bOutputOnly : 1;
		BIT_FIELD bDestroy : 1;
		BIT_FIELD bInTimer : 1;
		BIT_FIELD bOwned : 1;
   } flags;

   struct {
		char mybuffer[1024];
		char *buffer;
		int len;
		uint32_t timeout;
		  // this time will be set initially when the read starts, 
		  // and will never be surpassed.
		uint32_t dwEnd; // this is the presence of info here indicator.
		int *pnCharsRead;
		uint32_t dwLastRead; // minor delay to glob reads...
		int nTotalRead; // current accumulator of total read.
	} current;
	PCHANNEL_CALLBACK callbacks;
   PCHANNEL_CALLBACK exclusive;
	char *pReadBuffer;
   int  nReadTotal;
	int  nReadLen;
#ifndef __LINUX__
	DCB     dcb;
	COMSTAT cs;
#endif
	
	struct com_tracking_tag *next;
	struct com_tracking_tag **me;
   CRITICALSECTION csOp;
} COM_TRACK, *PCOM_TRACK;

PCOM_TRACK pTracking;

static struct commlib_local_tag
{
	struct {
		BIT_FIELD bInited : 1;
	} flags;
} sack_com_local;


static void ComLocalInit( void )
{
	if( !sack_com_local.flags.bInited )
	{
#ifdef __NO_OPTIONS__
		bLogDataXfer = 0;
		gbLog = 0;
#else
		bLogDataXfer = SACK_GetPrivateProfileInt( WIDE("COM PORTS"), WIDE("Log IO"), 0, WIDE("comports.ini") );
		gbLog = SACK_GetPrivateProfileIntEx( WIDE("COM PORTS"), WIDE("allow logging"), 0, WIDE("comports.ini"), TRUE );
#endif
		sack_com_local.flags.bInited = 1;
	}
}

#if !defined( BCC16 ) && defined( _WIN32 )
//-----------------------------------------------------------------------
 int  ReadComm ( int nCom, POINTER buffer, int nSize )
{
	uint32_t nRead = 0;
	char *pBytes = (char*)buffer;
	int offset = 0;
	OVERLAPPED ovl;
	PCOM_TRACK FindComByNumber( int iCommId );
	PCOM_TRACK pct = FindComByNumber( nCom );
	if( !pct || pct->flags.bOutputOnly )
      return 0;
	ovl.Offset = 0;
	ovl.OffsetHigh = 0;
	ovl.hEvent = NULL;
   //if( gbLog )
	//	lprintf( "Reading... %d", nSize );
	while( offset < nSize &&
	       ReadFile( (HANDLE)(intptr_t)nCom, pBytes + offset, 1, (DWORD*)&nRead, NULL ) &&
	       nRead )
	{
		offset += nRead;
		nRead = 0;
	}
	return nRead + offset;
}
//-----------------------------------------------------------------------
int WriteComm( int nCom, POINTER buffer, uint32_t nSize )
{
	uint32_t nWritten;
   if( bLogDataXfer & 1 )
   {
    	int nOut = nSize;
		lprintf( WIDE("Send COM: dump buffer (%d)"), nSize );
		#if defined( _TRACE_DATA_MIN )
		   if( nOut > 16 ) nOut = 16;
		#endif
		LogBinary( (uint8_t*)buffer, nOut );
	}
	if( WriteFile( (HANDLE)(intptr_t)nCom, buffer, nSize, (DWORD*)&nWritten, NULL ) )
		return nWritten;
	return -1;
}
//-----------------------------------------------------------------------
uintptr_t OpenComm( CTEXTSTR name, int nInQueue, int nOutQueue )
{
   ComLocalInit();
	if( gbLog )
		Log1( WIDE("Going to open:%s"), name );
	{
		COMMTIMEOUTS timeout;


		HANDLE hCom = CreateFile( name, GENERIC_READ|GENERIC_WRITE
									  , FILE_SHARE_READ|FILE_SHARE_WRITE
									  , NULL
									  , OPEN_EXISTING
									  , FILE_ATTRIBUTE_NORMAL
									  , NULL );
		timeout.ReadIntervalTimeout = 
#ifndef __NO_OPTIONS__
					SACK_GetPrivateProfileInt( name, WIDE( "port timeout" ), 100, WIDE( "comports.ini" ) );
#else
					100;
#endif
		timeout.ReadTotalTimeoutMultiplier = 1;
		timeout.ReadTotalTimeoutConstant = 1;
		timeout.WriteTotalTimeoutMultiplier = 1;
		timeout.WriteTotalTimeoutConstant = 1;
		SetCommTimeouts(hCom, &timeout);
		if( gbLog )
			Log2( WIDE("Result: %p %d"), hCom, GetLastError() );
		return (uintptr_t)hCom;
	}
}
//-----------------------------------------------------------------------
int CloseComm( int nDevice )
{
	return CloseHandle( (HANDLE)(intptr_t)nDevice );
}
//-----------------------------------------------------------------------
int GetCommError( int nCom, COMSTAT *pcs )
{
	uint32_t dwErrors;
	return ClearCommError( (HANDLE)(intptr_t)nCom, (DWORD*)&dwErrors, pcs );
}
//-----------------------------------------------------------------------
int FlushComm( int nComm, int nQueues )
{
	// hmm not sure how to implement this one...
	//xlprintf(LOG_NOISE)( WIDE("Flush comm - the fake one...") );
	return 0;
}
//-----------------------------------------------------------------------
#else 
#if defined( __LINUX__ )
// Weee! Implement the messy system/core/base level functions here
// open/close/read/write...
//-----------------------------------------------------------------------
 int  ReadComm ( int nCom, POINTER buffer, int nSize )
{
	// single select?
	int n = 1024;
	if( ( ioctl( nCom, FIONREAD, &n ) ) >= 0 )
	{
		if( n )
		{
			//if( gbLog )
			//	lprintf( WIDE("Received data %d"), n );
			if( nSize < n )
				n = nSize;
			n = read( nCom, buffer, n );
			if( n < 0 && errno == EAGAIN )
			{
				if( gbLog )
 					lprintf( WIDE("wait for more data...") );
				return 0;
			}
			return n;
		}
	}
	else
	{
		lprintf( WIDE("read ioctl error: %d"), errno );
	}
	if( n == 0 )
	{
		errno = EAGAIN;
		return 0;
	}
	// else return ioctl errno...
	return -1;
}

//-----------------------------------------------------------------------
int WriteComm( int nCom, POINTER buffer, uint32_t nSize )
{
   return write( nCom, buffer, nSize );
}

//-----------------------------------------------------------------------
uintptr_t OpenComm( CTEXTSTR name, int nInQueue, int nOutQueue )
{
   ComLocalInit();
	if( gbLog )
		Log1( WIDE("Going to open:%s"), name );
   return open( name, O_RDWR|O_NONBLOCK|O_NOCTTY );
}
//-----------------------------------------------------------------------
int CloseComm( int nDevice )
{
   return close( nDevice );
}
//-----------------------------------------------------------------------
int GetCommError( int nCom, COMSTAT *pcs )
{
   return errno;
}
//-----------------------------------------------------------------------
int FlushComm( int nComm, int nQueues )
{
	// hmm not sure how to implement this one...
	//xlprintf(LOG_NOISE)( WIDE("Flush comm - the fake one...") );
	return 0;
}
#endif
#endif
//-----------------------------------------------------------------------

PCOM_TRACK FindComByName( CTEXTSTR szPort )
{
	PCOM_TRACK check = pTracking;
	while( check )
	{
		if( !check->flags.bDestroy )
			if( StrCaseCmpEx( szPort, check->portname, sizeof(check->portname) ) == 0 )
				return check;
		check = check->next;
	}
	return NULL;
}

//-----------------------------------------------------------------------

PCOM_TRACK FindComByNumber( int iCommId )
{
	PCOM_TRACK check = pTracking;
	while( check )
	{
		if( check->iCommId == iCommId )
			return check;
		check = check->next;
	}
	return NULL;
}

//-----------------------------------------------------------------------

PCOM_TRACK AddComTracking( CTEXTSTR szPort, int iCommId )
{
	PCOM_TRACK pComTrack = New( COM_TRACK );
	memset( pComTrack, 0, sizeof( *pComTrack ) );
	if( ( pComTrack->next = pTracking ) )
		pTracking->me = &pComTrack->next;
	pComTrack->me = &pTracking;
	pComTrack->iUsers = 1;
	pTracking = pComTrack;
	StrCpyEx( pComTrack->portname, szPort, sizeof(pComTrack->portname)  );
	pComTrack->portname[sizeof( pComTrack->portname) - 1] = 0;
	pComTrack->iCommId = iCommId;
	pComTrack->pReadBuffer = NewArray( char, DEFAULT_READ_BUFFER );
	pComTrack->nReadTotal = DEFAULT_READ_BUFFER;
   InitializeCriticalSec( &pComTrack->csOp );
	return pComTrack;
}

//-----------------------------------------------------------------------

void RemoveComTracking( PCOM_TRACK pComTrack )
{
	if( pComTrack->flags.bInTimer )
	{
		pComTrack->flags.bDestroy = 1;
		return;
	}
	//Log2( WIDE("Unlink... %p %p"),pComTrack->me, pComTrack->next );
	if( ( (*pComTrack->me) = pComTrack->next ) )
		pComTrack->next->me = pComTrack->me;
	//xlprintf(LOG_NOISE)( WIDE("Release redbuffer...") );
	Release( pComTrack->pReadBuffer );
	//xlprintf(LOG_NOISE)( WIDE("Release comtrack..") );
	Release( pComTrack );
}

//-----------------------------------------------------------------------

static int 
    ParseComString ( const TEXTCHAR far *p
                   , uint32_t far *pBaud
                   , int far *pPar
                   , int far *pData
                   , int far *pStop
                   , int *piCarrier
                   , int *piRTS
                   , int *piRTSFlow
                   , const TEXTCHAR far * far *ppErr
                   )
{
  int iPar = 0, iData, iStop;
  uint32_t dwBaud;
//#if defined( _WIN32 ) || defined( BCC16 )
  dwBaud = (uint32_t)IntCreateFromText( p );
  p = strchr( p, ',' );
  if ( *p != ',' )
  {
    if ( ppErr )
      *ppErr = WIDE( "{baud}?" );
    return -5;
  }
  p++;
  if ( *p == 'e' || *p == 'E' )
  {
#ifdef __LINUX__
     iPar = PARENB;
#else
	  iPar = EVENPARITY;
#endif
  }
  else if ( *p == 'm' || *p == 'M' )
  {
#define CNSPAR 010000000000
#ifdef __LINUX__
	  iPar = CNSPAR|PARENB|PARODD;
#else
	  iPar = MARKPARITY;
#endif
  }
  else
  if ( *p == 'n' || *p == 'N' )
  {
#ifdef __LINUX__
     iPar = 0;
#else
    iPar = NOPARITY;
#endif
  }
  else
  if ( *p == 'o' || *p == 'O' )
  {
#ifdef __LINUX__
	  iPar = PARODD|PARENB;
#else
    iPar = ODDPARITY;
#endif
  }
  else
  if ( *p == 's' || *p == 'S' )
  {
#ifdef __LINUX__
	  iPar = CNSPAR|PARENB;
#else
    iPar = ODDPARITY;
#endif
  }
  else
  {
    if ( ppErr )
      *ppErr = WIDE( "{baud},?" );
    return -6;
  }
  p++;
  if ( *p != ',' )
  {
    if ( ppErr )
      *ppErr = WIDE( "{baud},{parity}?" );
    return -7;
  }
  p++;
  if ( *p < '4' || *p > '8' )
  {
    if ( ppErr )
      *ppErr = WIDE( "{baud},{parity},?" );
    return -8;
  }
#ifdef __LINUX__
  switch( *p )
  {
  case '5':
	  iData = CS5;
     break;
  case '6':
	  iData = CS6;
     break;
  case '7':
	  iData = CS7;
     break;
  case '8':
	  iData = CS8;
     break;
  }
#else
  iData = *p - '0';
#endif
  p++;
  if ( *p != ',' )
  {
    if ( ppErr )
      *ppErr = WIDE( "{baud},{parity},{data}?" );
    return -9;
  }
  p++;

  if ( p[0] == '2' && p[1] != '.' )
  {
    p++;
#ifdef __LINUX__
    iStop = CSTOPB;
#else
    iStop = TWOSTOPBITS;
#endif
  }
  else
  if ( p[0] == '1' && p[1] != '.' )
  {
    p++;
#ifdef __LINUX__
    iStop = 0;
#else
    iStop = ONESTOPBIT;
#endif
  }
  else
  if ( p[0] == '1' && p[1] == '.' && p[2] == '5' )
  {
  	 p+= 3;
#ifdef __LINUX__
    iStop = CSTOPB;
#else
	 iStop = ONE5STOPBITS;
#endif
  }
  else
  {
    if ( ppErr )
      *ppErr = WIDE( "{baud},{parity},{data},?" );
    return -10;
  }

  if( p[0] == ',' )
  {
  	  p++;
  	  if( p[0] == 'C' )
  	  {
  	  	  if( piCarrier ) *piCarrier = 1;
  	  }
  	  else
  	  	  if( piCarrier ) *piCarrier = 0;
  	  while( p[0] && p[0] != ',' ) p++;
  }

  if( p[0] == ',' )
  {
  	  p++;
  	  if( p[0] == 'R' )
  	  {
  	  	  if( piRTS ) *piRTS = 1;
  	  }
  	  else
  	  	  if( piRTS ) *piRTS = 0;
  	  while( p[0] && p[0] != ',' ) p++;
  }

  if( p[0] == ',' )
  {
  	  p++;
  	  if( p[0] == 'R' )
  	  {
  	  	  if( piRTSFlow ) *piRTSFlow = 1;
  	  }
  	  else
  	  	  if( piRTSFlow ) *piRTSFlow = 0;
  	  while( p[0] && p[0] != ',' ) p++;
  }
  if( dwBaud & 0xFFFF8000 )
  {
#ifdef __LINUX__
	  if( dwBaud == 230400ul )
	  {
        dwBaud = B230400;
	  }
	  else
#endif
  	  if( dwBaud == 256000ul )
	  {
#ifndef __LINUX__
		  dwBaud = CBR_256000;
#endif
  	  } 
  	  else if( dwBaud == 115200ul )
	  {
#ifdef __LINUX__
        dwBaud = B115200;
#else
#if defined( WIN32 )
		  dwBaud = CBR_115200;
#elif defined( BCC16 )
		  dwBaud = 0xFEFF;
#else
#error no baud defined for this compiler.
#endif
#endif
  	  } 
  	  else if( dwBaud == 128000ul )
	  {
#ifndef __LINUX__
		  dwBaud = CBR_128000;
#endif
  	  }
  	  else
  	  {
  	  	  if( (dwBaud != 57600ul) && (dwBaud != 38400ul) )
  	  	  {
      		 static TEXTCHAR buf[64];
      		 tnprintf( buf, sizeof( buf ), WIDE("Invalid Baud rate %08x"), dwBaud );
	   		 if ( ppErr )
   	   			*ppErr = buf;
			  return -15;
		  }
#ifdef __LINUX__
		  else
			  if( dwBaud == 57600ul )
				  dwBaud = B57600;
			  else if( dwBaud == 38400ul )
				  dwBaud = B38400;
#endif
     }
   }
   else
	  switch ( dwBaud )
	  {
#ifdef __LINUX__
	  case 19200ul:  dwBaud = B19200;  break;
	  case 9600ul:   dwBaud = B9600;   break;
	  case 4800ul:   dwBaud = B4800;   break;
	  case 2400ul:   dwBaud = B2400;   break;
	  case 1200ul:   dwBaud = B1200;   break;
	  case 600ul:    dwBaud = B600;    break;
	  case 300ul:    dwBaud = B300;    break;
	  case 110ul:    dwBaud = B110;    break;
#else
//  case 28000ul:  dwBaud = CBR_28000;  break;
    case 19200ul:  dwBaud = CBR_19200;  break;
    case 14400ul:  dwBaud = CBR_14400;  break;
    case 9600ul:   dwBaud = CBR_9600;   break;
//  case 9200ul:   dwBaud = CBR_9200;   break;
//  case 8400ul:   dwBaud = CBR_8400;   break;
//  case 6000ul:   dwBaud = CBR_6000;   break;
    case 4800ul:   dwBaud = CBR_4800;   break;
//  case 4400ul:   dwBaud = CBR_4400;   break;
    case 2400ul:   dwBaud = CBR_2400;   break;
    case 1200ul:   dwBaud = CBR_1200;   break;
    case 600ul:    dwBaud = CBR_600;    break;
    case 300ul:    dwBaud = CBR_300;    break;
	  case 110ul:    dwBaud = CBR_110;    break;
#endif
    default:
#ifndef HIWORD
#define HIWORD(n) (((n)>>16)&0xFFFF)
#endif
      if ( HIWORD(dwBaud) )
      {
      	static TEXTCHAR buf[64];
      	tnprintf( buf, sizeof( buf ), WIDE("Invalid Baud rate %08x"), dwBaud );
	   	 if ( ppErr )
   	   	*ppErr = buf;
          return -15;
      }
      break; 
  }
  if ( pBaud ) *pBaud = (uint32_t)dwBaud;
  if ( pPar ) *pPar = iPar;
  if ( pData ) *pData = iData;
  if ( pStop ) *pStop = iStop;
//#endif
  return 0;
}

//-----------------------------------------------------------------------

int iTimerId;
#if defined _WIN32 || defined( __LINUX__ )
static void CPROC ReadTimer( uintptr_t psv )
#else
CALLBACKPROC( void, ReadTimer)( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
#endif
{
	PCOM_TRACK pct;
	//static uint32_t timer_ticks;
   static uint32_t start;
	for( pct = pTracking; pct; pct = pct->next )
	{
		if( pct->flags.bOutputOnly )
         continue;
		if( pct->callbacks )
		{
			int len;
			pct->flags.bInTimer = 1;
         EnterCriticalSec( &pct->csOp );
			len = ReadComm( pct->iCommId
							  , pct->pReadBuffer + pct->nReadLen
							  , pct->nReadTotal - pct->nReadLen);
			LeaveCriticalSec( &pct->csOp );
			if( ( len > 0 ) || ( len < 0 ) )
			{
				if( len < 0 )
				{
					if( gbLog )
						lprintf( WIDE("!!!!!!! Some sort of com error occured...%d"), errno );
					// flush this data out...
					goto issue_callbacks;
					len = -len;
				}
				else
					start = GetTickCount() + 10;
				pct->nReadLen += len;			
			}
			// if we didn't read anything, and have previosly read...
									// or if the buffer is totally full now...
         //lprintf( WIDE("len %d last %d start %d max %d"), len, pct->nReadLen, start, DEFAULT_READ_BUFFER );
			if( ( !len && pct->nReadLen && (start < GetTickCount()) ) ||
			    pct->nReadLen == pct->nReadTotal )
			{
				PCHANNEL_CALLBACK pcc, pccNext;
issue_callbacks:
				pcc = pct->callbacks;
				//lprintf( WIDE("log %d and log %d"), gbLog, bLogDataXfer );
				if( gbLog )
				{
					//Log1( WIDE("Dump information to people with callbacks...%d"), pct->nReadLen );
					if( bLogDataXfer & 2 )
					{
						lprintf( WIDE("COM Receive") );
						LogBinary( (uint8_t*)pct->pReadBuffer
								  , pct->nReadLen );
					}
				}
				while( pcc )
				{
					pccNext = pcc->next;
					// THIS pcc may go away, hopefully the NEXT won't...
					//lprintf( "pcc is %p exclus %p owned %d", pcc, pct->exclusive, pct->flags.bOwned );
					if( ( pct->flags.bOwned && ( pcc == pct->exclusive ) )
						|| !pct->flags.bOwned )
					{
						if( pcc->func )
						{
							if( !pcc->flags.skip_read )
								pcc->func( pcc->psvUserRead
											, pct->iCommId
											, pct->pReadBuffer
											, pct->nReadLen );
						}
					}
					pcc = pccNext;
				}
				// reset amount of data read.
				pct->nReadLen = 0;
			}
			pct->flags.bInTimer = 0;
			if( pct->flags.bDestroy )
			{
				RemoveComTracking( pct );
				// just get out... we probably lost the tracking in process
				// pointers are all mislinked...
				// and it's just bad.
				return;
			}
		}
	}
}

//-----------------------------------------------------------------------

uintptr_t CPROC ReadThread( PTHREAD thread )
{
   iTimerId = 1;
	while( 1 )
	{
		ReadTimer(0);
      WakeableSleep( 10 );
	}
   return 0;
}

#ifdef __LINUX__
void DumpTermios( struct termios *opts )
{
	if( !gbLog )
      return;
   lprintf( WIDE("iflag %x"), opts->c_iflag );
   lprintf( WIDE("oflag %x"), opts->c_oflag );
   lprintf( WIDE("cflag %x"), opts->c_cflag );
	lprintf( WIDE("lflag %x"), opts->c_lflag );
#ifndef __ARM__
	lprintf( WIDE("ispeed: %x"), opts->c_ispeed );
	lprintf( WIDE("ospeed: %x"), opts->c_ospeed );
#endif
}
#endif

//-----------------------------------------------------------------------

 int  SackOpenCommEx(CTEXTSTR szPort, uint32_t uiRcvQ, uint32_t uiSendQ
              , CommReadCallback func
              , uintptr_t psvRead
                  )
{
#ifdef USE_REAL_FUNCTIONS
	PCOM_TRACK pct;
   ComLocalInit();
	{
	  int iPar, iData, iStop, iCarrier, iRTS, iRTSFlow;
	  uint32_t wBaud;
	  const TEXTCHAR far *szErr;
	  TEXTCHAR szInit[64];
	  // capital letters on carrier, rts, rtsflow mean to enable - otherwise
	  // don't pay attention to those signals.
#ifndef __NO_OPTIONS__
	  SACK_GetPrivateProfileString( WIDE("COM PORTS"), szPort, WIDE("57600,N,8,1,cARRIER,RTS,rTSFLOW"), szInit, sizeof( szInit ), WIDE("comports.ini") );
#else
	  GetPrivateProfileString( WIDE("COM PORTS"), szPort, WIDE(""), szInit, sizeof( szInit ), WIDE("comports.ini") );
		if( !szPort[0] ) {
			  WritePrivateProfileString( WIDE("COM PORTS"), szPort, WIDE("57600,N,8,1,cARRIER,RTS,rTSFLOW"), WIDE("comports.ini") );

		}
#endif

#if defined(  _WIN32 ) || defined( __LINUX__ )
	  if( !iTimerId )
	  {
		  // this timer is constantly getting scheduled later than it thinks it should be.
		  ThreadTo( ReadThread, 0 );
		  //iTimerId = AddTimer( 10, ReadTimer, 0 );
	  }
#else
	  if( !iTimerId )
	      iTimerId = SetTimer( NULL, 100, 10, (TIMERPROC)ReadTimer );
#endif
	  if ( ParseComString ( szInit, &wBaud, &iPar
   	                   , &iData, &iStop
   	                   , &iCarrier
   	                   , &iRTS, &iRTSFlow
   	                   , &szErr ) )
	  {
#ifndef __LINUX__
   	 MessageBox ( (HWND)NULL, szErr, WIDE("SackOpenComm: invalid init string")
	               , MB_OK | MB_ICONHAND );
   	 MessageBox ( (HWND)NULL, szInit, WIDE("SackOpenComm: invalid init string")
						, MB_OK | MB_ICONHAND );
#endif
   	 return FALSE;
	  }
		if( ( pct = FindComByName( szPort ) ) )
		{
			if( func )
			{
				PCHANNEL_CALLBACK pcc = New( CHANNEL_CALLBACK );
				pcc->flags.skip_read = 0;
				pcc->iTimer = 0;
				pcc->func = func;
				pcc->psvUserRead = psvRead;
				if( ( pcc->next = pct->callbacks ) )
					pct->callbacks->me = &pcc->next;
				pcc->me = &pct->callbacks;
				pct->callbacks = pcc;
			}
			if( gbLog )
				Log1( WIDE("Resulting in comm already open...: %s"), szPort );
			pct->iUsers++;
			return pct->iCommId;
		}
		else
		{
			uintptr_t iCommId = OpenComm( szPort, uiRcvQ, uiSendQ );
			if( gbLog )
				lprintf( WIDE("attempted to open: %s result %d"), szPort, iCommId );
			if( (int)iCommId >= 0 )
			{
				pct = AddComTracking( szPort, iCommId );
				if( StrCaseCmpEx( szPort, WIDE("lpt"), 3 ) != 0 )
				{
					pct->flags.bOutputOnly = 0;
				   SackFlushComm( iCommId, 0 );
				   	SackFlushComm( iCommId, 1 );
#ifndef __LINUX__
#ifdef BCC16
		   	pct->dcb.Id          = iCommId;
#else
		   	pct->dcb.DCBlength   = sizeof( pct->dcb );
#endif
				pct->dcb.BaudRate    = wBaud;
				pct->dcb.ByteSize    = iData;
				pct->dcb.Parity      = iPar;
				pct->dcb.StopBits    = iStop;
				pct->dcb.fBinary     = 1; // yes! we want binary.
#ifdef BCC16				
				pct->dcb.fRtsDisable = iRTS;  /* we control the horizontal... */
				pct->dcb.fRtsflow    = iRTSFlow; /* we control the vertical... :) */
#else
				if( iRTS && iRTSFlow )
					pct->dcb.fRtsControl = RTS_CONTROL_TOGGLE;
				else if( iRTS )
					pct->dcb.fRtsControl = RTS_CONTROL_ENABLE;
				else 
					pct->dcb.fRtsControl = RTS_CONTROL_DISABLE;
#endif
            pct->dcb.fDtrControl = DTR_CONTROL_ENABLE;
				pct->flags.bUseCarrierDetect = iCarrier; // try this - remove maybe.
				lprintf( WIDE( " pct->dcb.BaudRate is %lu pct->dcb.ByteSize is %lu pct->dcb.Parity is %lu pct->dcb.fRtsControl is %lu " )
					, pct->dcb.BaudRate
					, pct->dcb.ByteSize
					, pct->dcb.Parity
					, pct->dcb.fRtsControl
					);

				//EscapeCommFunction( (HANDLE)(intptr_t)iCommId, SETDTR );
				//EscapeCommFunction( (HANDLE)(intptr_t)iCommId, SETRTS );
				//SETDTR
#ifdef BCC16
		      if ( SetCommState( &pct->dcb ) )
#else
		      if ( !SetCommState( (HANDLE)(intptr_t)iCommId, &pct->dcb ) )
#endif
		      {
#ifdef _WIN32
					lprintf( WIDE("Open: Invalid initialization string %d"), GetLastError() );
#endif
				SackCloseComm( iCommId );
				iCommId = -1;
				}
#else
				{
					struct termios opts;
               tcgetattr( iCommId, &opts );
               DumpTermios(&opts);
					//opts.c_iflag &= ~(BRKINT|PARMRK|INPCK|ISTRIP|INLCR|IXON|IXOFF|IMAXBEL);
               opts.c_iflag = 0;
					opts.c_iflag |= IGNBRK|IGNPAR|IGNCR|IXANY;

					//opts.c_oflag &= ~(OLCUC|ONLCR|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|NLDLY|TABDLY|BSDLY|VTDLY|FFDLY);
               opts.c_oflag = 0;
					opts.c_oflag |= 0; // looks like nothing special to output...

					//opts.c_cflag &= ~(CNSPAR|PARENB|PARODD|CSIZE|CSTOPB|CRTSCTS|CLOCAL);
               opts.c_cflag = 0;
					opts.c_cflag |= iPar | iData | iStop | wBaud | CREAD | CLOCAL;

					//opts.c_lflag &= ~(ISIG|ICANON|XCASE|ECHO|ECHOE|ECHOK|ECHONL|ECHOCTL...);
					opts.c_lflag = 0;
					opts.c_lflag |= 0;

					opts.c_cc[VMIN] = 1;
               opts.c_cc[VTIME] = 0;
					cfsetospeed( &opts, wBaud );
					cfsetispeed( &opts, 0 );

					DumpTermios(&opts);
					if( tcsetattr( iCommId, TCSANOW, &opts ) )
						lprintf( WIDE("tcsetattr failed: %d"), errno );
				}
            // setup the com port here (termios)
#endif
				}
				else
				{
               pct->flags.bOutputOnly = 1;
				}
			}
			//else
			//	xlprintf(LOG_NOISE)( WIDE("Failed!") );
			if( iCommId >= 0 )
			{
				if( func )
				{
					PCHANNEL_CALLBACK pcc = New( CHANNEL_CALLBACK );
					pcc->flags.skip_read = 0;
					pcc->iTimer = 0;
					pcc->func = func;
					pcc->psvUserRead = psvRead;
					if( ( pcc->next = pct->callbacks ) )
						pct->callbacks->me = &pcc->next;
					pcc->me = &pct->callbacks;
					pct->callbacks = pcc;
				}
			}
			//xlprintf(LOG_NOISE)( WIDE("Resulting to the client...") );
			return  iCommId;
		}
	}
#else
   return 42; //some non-zero comm id
#endif
}

//-----------------------------------------------------------------------

 void  SackSetBufferSize ( int iCommId
													 , int readlen )
{
	PCOM_TRACK pct;
	pct = FindComByNumber( iCommId );
	lprintf( WIDE( "updating read length to %d %p %d" ), iCommId, pct, pct->nReadTotal );
	if( pct )
	{
		if( readlen < DEFAULT_READ_BUFFER )
		{
         lprintf( WIDE( "updating read length to %d" ), pct->nReadTotal );
			pct->nReadTotal = readlen;
		}
	}
}

 void  SackSetReadCallback ( int iCommId
                                          , CommReadCallback f
              										, uintptr_t psvRead )
{
	PCOM_TRACK pct;
	pct = FindComByNumber( iCommId );
	if( pct )
	{
		PCHANNEL_CALLBACK pcc = New( CHANNEL_CALLBACK );
		pcc->flags.skip_read = 0;
		pcc->iTimer = 0;
		pcc->func = f;
		pcc->psvUserRead = psvRead;
		if( ( pcc->next = pct->callbacks ) )
			pct->callbacks->me = &pcc->next;
		pcc->me = &pct->callbacks;
		pct->callbacks = pcc;
	}
	else
	{
		Log1( WIDE("Comm ID %d is not open?"), iCommId );
	}
}
              
 int  SackClearReadCallback ( int iCommId
                                          ,  CommReadCallback func )
{
	PCOM_TRACK pct;
	pct = FindComByNumber( iCommId );
	if( pct )
	{
		PCHANNEL_CALLBACK pcc = pct->callbacks;		
		while( pcc )
		{
			if( pcc->func == func )
			{
				if( ( *(pcc->me) = pcc->next ) )
					pcc->next->me = pcc->me;
				break;
			}
			pcc = pcc->next;
		}
		if( pcc )
		{
			Release( pcc );
			Log1( WIDE("Comm ID %d had the callback, successfully removed"), iCommId );
			return TRUE;
		}
		else
		{
			Log1( WIDE("Comm ID %d did not have the specified read callback"), iCommId );
		}
	}
	else
	{
		Log1( WIDE("Comm ID %d is not open?"), iCommId );
	}
	return FALSE;
}

//-----------------------------------------------------------------------

 int  SackCloseComm(int iCommId)
{
#ifdef USE_REAL_FUNCTIONS
	 PCOM_TRACK pct = FindComByNumber( iCommId );
	 if( pct && !(--pct->iUsers ) )
	 {
		 // destroy pct here...
		 CloseComm( iCommId );
		 RemoveComTracking( pct );
	    return 1;
	 }
	 else
	 	return 0;
#else
    return 0; //0 means success
#endif
}

//-----------------------------------------------------------------------

 int  SackReadComm(int iCommId, void far *pBuf, int iChars)
{
#ifdef USE_REAL_FUNCTIONS
    return ReadComm( iCommId, pBuf, iChars );
#else
    return 0; //Always return zero characters read
#endif
}

//-----------------------------------------------------------------------

 int  SackWriteComm(int iCommId, void far *pBuf, int iChars)
{
#ifdef USE_REAL_FUNCTIONS
	return WriteComm( iCommId, pBuf, iChars );
#else
    return iChars; //All supplied characters successfully written!
#endif
}

//-----------------------------------------------------------------------
#ifndef __LINUX__
 int  SackSetCommState(DCB FAR *lpDcb)
{
#ifdef USE_REAL_FUNCTIONS
#ifdef BCC16
	 return SetCommState( lpDcb );
#else
	xlprintf(LOG_NOISE)( WIDE("SackSetCommState cannot work in 32 bit system") );
	return 0;
#endif
#else
    if (!IsBadReadPtr(lpDcb, sizeof(DCB)))  //Save DCB for possible
        memcpy(&stDcb, lpDcb, sizeof(DCB)); //Later retrieval
    return 0; //0 means success
#endif
}
#endif
//-----------------------------------------------------------------------
#ifndef __LINUX__
 int  SackGetCommState(int iCommId, DCB FAR *lpDcb)
{
#ifdef USE_REAL_FUNCTIONS
#ifdef BCC16
	return GetCommState( iCommId, lpDcb );
#else
	return GetCommState( (HANDLE)(intptr_t)iCommId, lpDcb );
#endif
#else
    if (!IsBadWritePtr(lpDcb, sizeof(DCB))) //Return passed DCB
        memcpy(lpDcb, &stDcb, sizeof(DCB)); //See! It matches!
    return 0; //0 means success
#endif
}
#endif
//-----------------------------------------------------------------------

 int  SackFlushComm(int iCommId, int iInOut)
{
#ifdef USE_REAL_FUNCTIONS
	 return FlushComm( iCommId, iInOut );
#else
    return 0; //0 means success
#endif
}

//-----------------------------------------------------------------------

 int  SackGetCommError(int iCommId, COMSTAT FAR *lpStat)
{
#ifdef USE_REAL_FUNCTIONS
	return GetCommError( iCommId, lpStat );
#else
    if (lpStat != NULL && !IsBadWritePtr(lpStat, sizeof(COMSTAT)))
    {
        lpStat->status = 0;      //
        lpStat->cbInQue = 0;     // Return all is well
        lpStat->cbOutQue = 0;    //
    }
    return 0;
#endif
}

//-----------------------------------------------------------------------

 int  SackCommReadBufferEx( int iCommId, char *buffer, int len
												 , uint32_t timeout, int *pnCharsRead 
												 DBG_PASS
												 )
{
   int iResult = SACKCOMM_ERR_NONE;
   int nCharsRead;
	PCOM_TRACK pComTrack = FindComByNumber( iCommId );

	uint32_t dwEnd = GetTickCount() + timeout;
	if( !pComTrack )
		return SACKCOMM_ERR_NOTOPEN;
	if( pComTrack->flags.bInRead )
	{
		xlprintf(LOG_NOISE)( WIDE("Result busy in comm read buffer") );
		return SACKCOMM_ERR_BUSY;
	}
  /********************************************************************\
  * ASSERT pointer(s)                                                  *
  \********************************************************************/
  if( !pnCharsRead )
  {
  	  xlprintf(LOG_NOISE)( WIDE("Failing - bad return count") );
  	  	return SACKCOMM_ERR_BUFSIZE;
  }
  if( !buffer )
  {
  	  xlprintf(LOG_NOISE)( WIDE("Failing - bad buffer") );
  	  return SACKCOMM_ERR_POINTER;
  }
   pComTrack->flags.bInRead = 1;
   if( gbLog )
		Log2( DBG_FILELINEFMT WIDE("comm read buffer len:%d timeout:%d") DBG_RELAY, len, timeout );
   if( !pComTrack->current.dwEnd )
   {
   	// save this information for later use...
   	pComTrack->current.dwEnd = dwEnd;
   	pComTrack->current.buffer = buffer;
   	pComTrack->current.len = len;
      pComTrack->current.timeout = timeout;
      pComTrack->current.pnCharsRead = pnCharsRead;
      pComTrack->current.dwLastRead = 0;
      pComTrack->current.nTotalRead = 0; // nothing read.
   }
   else
   {
   	if( buffer != pComTrack->current.buffer || 
   	    len != pComTrack->current.len ||
          timeout != pComTrack->current.timeout )
      {
         // hmm...
      	xlprintf(LOG_NOISE)( WIDE("********* Read is not the same as the exsiting read.") );
      	pComTrack->flags.bInRead = 0; 
      	return SACKCOMM_ERR_BUSY;
      }
   	if( gbLog )
			xlprintf(LOG_NOISE)( WIDE("Resuming previous read...") );
   }
  //assert_far_call ( ! IsBadWritePtr(pnCharsRead,sizeof(*pnCharsRead))
  //                , WIDE("SackCommReadBuffer: bad pnCharsRead") ); 

  /*********************************************************************\
  * As per the original code, the communications channel is checked for *
  * a pre-existing error condition.  This is done to clear the error    *
  * before any reading is done.  Otherwise, the error condition will    *
  * persist for any further communication.                              *
  * NOTE: the original code assigned the result to the static variable  *
  * nCommError inside of ReadCommandBlock.  At this stage, this served  *
  * no purpose and the result was never used, so the assignment has been*
  * removed                                                             *
  \*********************************************************************/
#ifndef __LINUX__
			SackGetCommError ( iCommId, &pComTrack->cs );
#endif

  /*********************************************************************\
  * Presumeably, this call is being made in a response to a request for *
  * data.  So, give the unit a chance to respond by waiting until either*
  * there is data to be read, until the unit has "timed out", or until  *
  * the unit is no longer connected.                                    *
  \*********************************************************************/

	if(  pComTrack->flags.bUseCarrierDetect && 
	    !pComTrack->flags.bHaveCarrier )
	{
#ifndef __LINUX__
#ifdef BCC16
		uint32_t iEvents;
	   iEvents = GetCommEventMask( iCommId, 0xFFFF );
	   if( iEvents & EV_RLSDS )
#else
		uint32_t iEvents;
		GetCommMask( (HANDLE)(intptr_t)iCommId, (DWORD*)&iEvents );
		if( !(iEvents & EV_RLSD ) )
#endif
	   {
	   	pComTrack->flags.bHaveCarrier = 1;
	   }
	   else
	   {
	   	xlprintf(LOG_NOISE)( WIDE("No Carrier - failing read. ") );
	   	*pnCharsRead = 0;
		   pComTrack->flags.bInRead = 0;
		   if( gbLog )
				xlprintf(LOG_NOISE)( WIDE("Leave comm read buffer") );
		   pComTrack->current.dwEnd = 0;
	   	return SACKCOMM_ERR_TIMEOUT;
		}
#endif
	}

   do
   {
    /*****************************************************************\
    * The order here has been carefully chosen to minimize delays.    *
    * First, read the communications port.                            *
    \*****************************************************************/
    // don't call stub version - no change - but less confusion.
    nCharsRead = ReadComm ( iCommId
                          , buffer + pComTrack->current.nTotalRead
                          , len - pComTrack->current.nTotalRead );

    /*****************************************************************\
    * If data was read in without error (the normal condition), then  *
    * go ahead and break out of the loop, we're done.                 *
    \*****************************************************************/
    if ( nCharsRead > 0 )
    {	
		 if( bLogDataXfer & 2 )
		 {
			 xlprintf(LOG_NOISE)( WIDE("Recv COM: dump buffer") );
			 //xlprintf(LOG_NOISE)( buffer + pComTrack->current.nTotalRead, nCharsRead );
		 }
		 pComTrack->current.dwLastRead = GetTickCount();
		 pComTrack->current.nTotalRead += nCharsRead;
		 if( pComTrack->current.nTotalRead == len )
		 {
			 *pnCharsRead = pComTrack->current.nTotalRead;
			 pComTrack->flags.bInRead = 0;
    		xlprintf(LOG_NOISE)( WIDE("Return buffer full.") );
			pComTrack->current.dwEnd = 0;
			return SACKCOMM_ERR_NONE;
    	}
      //Log1( WIDE("SackCommReadBuffer => SUCCESS : %d"), nCharsRead );
		continue; // try reading some more...
    }

    /*****************************************************************\
    * If the read was an error, then display the error message(s) to  *
    * the user and break with an error.                               *
    \*****************************************************************/
    if ( nCharsRead < 0 )
    { 
      TEXTCHAR cOut[128];
      int  nCommError;

#ifndef __LINUX__
			nCommError = SackGetCommError ( iCommId, &pComTrack->cs );
#endif
      tnprintf ( cOut, sizeof( cOut ), WIDE("SackCommReadBuffer: read %d chars, error=%d")
               , nCharsRead, nCommError );
      xlprintf(LOG_NOISE)( WIDE("%s"), cOut );
#ifndef __LINUX__
      lprintf ( WIDE("    cs.status=%u,0x%02X  cs.in=%u  cs.out=%u")
#ifdef BCC_16
               , pComTrack->cs.status
#else
               , *(uint32_t*)&pComTrack->cs
#endif
               , pComTrack->cs.cbInQue
					, pComTrack->cs.cbOutQue );
#endif
      iResult = SACKCOMM_ERR_COMM;
    }
    else if( GetTickCount() >= pComTrack->current.dwEnd ) // no data, check timeout
    {
    /*****************************************************************\
    * If we have exceeded the timeout limit, then set the error.      *
    \*****************************************************************/  
    	if( pComTrack->current.nTotalRead )
    	{
    		// can't get this message - the above code leaves with any data.
	      _lprintf( DBG_RELAY )( WIDE( "SackCommReadBuffer() => TIMEOUT (WITH DATA %d)" )
	             , pComTrack->current.nTotalRead );
	      iResult = SACKCOMM_ERR_NONE;
	      break; // get out of this loop.
	   }
	   else
	   {
	   	if( pComTrack->flags.bUseCarrierDetect )
			{
#ifndef __LINUX__
#ifdef BCC16
	   		uint32_t iEvents;
	   		iEvents = GetCommEventMask( iCommId, 0xFFFF );
	   		if( !(iEvents & EV_RLSDS) )
#else
	   		uint32_t iEvents;
				GetCommMask( (HANDLE)(intptr_t)iCommId, (DWORD*)&iEvents );
				if( !(iEvents & EV_RLSD ) )
#endif
	   		{
			      lprintf( WIDE("SackCommReadBuffer() => TIMEOUT(No Carrier)") );
	   			pComTrack->flags.bHaveCarrier = 0;
	   		}	
	   		else
	   		{
			      lprintf( WIDE("SackCommReadBuffer() => TIMEOUT(With carrier)") );
   			   iResult = SACKCOMM_ERR_TIMEOUT;
				}
#endif
	   	}
	   	else
	   	{
		      lprintf( WIDE("SackCommReadBuffer() => TIMEOUT(ignore carrier)") );
   		   iResult = SACKCOMM_ERR_TIMEOUT;
   		}
		}
    }
    else // not timed out, nothing read.
	 {
#ifndef __LINUX__
    	MSG msg;
    	 // check to see if we ever ready any
    	if( pComTrack->current.nTotalRead && 
    	    ( GetTickCount() > ( pComTrack->current.dwLastRead + 25 ) ) )
    	{
		   *pnCharsRead = pComTrack->current.nTotalRead;
		   pComTrack->flags.bInRead = 0;
		   //xlprintf(LOG_NOISE)( WIDE("Leave comm read buffer") );
    		iResult = SACKCOMM_ERR_NONE_MORE;
    	}
    	if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
    	{
    	   // unlock ourselves... think we can come back in and resume...
    		pComTrack->flags.bInRead = 0;
    		if( gbLog )
				Log1( WIDE("Dispatching Message.. %d"), msg.message );
    		DispatchMessage( &msg );
    		pComTrack->flags.bInRead = 1;
		}
#endif
    }
  }
  while ( iResult == SACKCOMM_ERR_NONE );
  *pnCharsRead = pComTrack->current.nTotalRead;
   pComTrack->flags.bInRead = 0;
   pComTrack->current.dwEnd = 0;
   //xlprintf(LOG_NOISE)( WIDE("Leave comm read buffer") );
  return iResult;
}

//-----------------------------------------------------------------------

 int  SackCommReadDataEx( int iCommId
												 , uint32_t timeout
												 , char **pBuffer
												 , int *pnCharsRead 
												 DBG_PASS
											  )
{
	PCOM_TRACK pComTrack = FindComByNumber( iCommId );
	int iResult =
	       SackCommReadBufferEx( iCommId
	                           , pComTrack->current.mybuffer
	                           , sizeof( pComTrack->current.mybuffer )
	                           , timeout
	                           , pnCharsRead 
	                           DBG_RELAY );
	if( pBuffer )
		*pBuffer = pComTrack->current.mybuffer;
	return iResult;
}

//-----------------------------------------------------------------------

 int  SackCommWriteBufferEx( int iCommId, char *buffer, int len
												  , uint32_t timeout DBG_PASS)
{
	PCOM_TRACK pComTrack = FindComByNumber( iCommId );
	int sendofs = 0;
	int sendlen = len;
	uint32_t dwEnd = GetTickCount() + timeout;
	// clear error...
    /*check for no error*/
#ifndef __LINUX__
	SackGetCommError ( iCommId, &pComTrack->cs );
#endif
    //nCommError = _GetCommError( gnCommID, NULL );

	/*attempt comm write*/
    //if( fnuiIsUnitConnected() || gbyTimeOut == 0 || gfDoCommAnyway)
	if( !len )
	{
		lprintf( WIDE("Sorry, no, you cannot SEND nothing.") );
      return SACKCOMM_ERR_BUFSIZE;
	}
   do
   {
   	{
    	 int thissend;
		 int dosend = sendlen;
       //xlprintf(LOG_NOISE)( WIDE("Write") );
		 thissend = SackWriteComm( iCommId, buffer + sendofs, dosend );
       //xlprintf(LOG_NOISE)( WIDE("Did Write") );
    	 if( thissend <= 0 )
    	 {
#ifdef _WIN32
			 return SACKCOMM_ERR_TIMEOUT;
#endif    	 	
		 	 Log1( WIDE("Send Error occured: %d"), GetCommError( iCommId, NULL ) );
#ifndef __LINUX__
			 SackGetCommError ( iCommId, &pComTrack->cs );
#else
       // probably need something here...
          thissend = 0;
#endif
			 thissend = -thissend;
			 if( thissend == 0 )
			 {
			 	  xlprintf(LOG_NOISE)( WIDE("Data is not going anywhere - bail out.") );
			 	  return SACKCOMM_ERR_COMM;
			 }
    	 }
    	 sendofs += thissend;
    	 sendlen -= thissend;
    	}
   }
   while( sendlen && dwEnd > GetTickCount() );
   //Log1( WIDE("SackCommWriteBuffer Leave : %d"), sendlen );
   if( sendlen )
   	return SACKCOMM_ERR_TIMEOUT;
   return SACKCOMM_ERR_NONE;
}

//-----------------------------------------------------------------------
/*****************************************************************************\
* SACKCommFlush                                                               *
*    Attempt to read from the portable until no more characters appear.       *
*    The read will last at least as long as 100ms, but no longer then 5000ms. *
\*****************************************************************************/

 void  SackCommFlush ( int nCommID )
{
	PCOM_TRACK pct = FindComByNumber( nCommID );
	if( pct )
	{
		FlushComm( pct->iCommId, 0 );
		FlushComm( pct->iCommId, 1 );
	}
	{
//#if 0
    uint32_t   dwTicks = GetTickCount();
    uint32_t   dwWaitUntilAtLeast = dwTicks + 100UL;
    uint32_t   dwWaitNoMoreThan   = dwTicks + 5000UL;
    int     nCharsRead;
    char    cBuf[100];
    PCOM_TRACK pComTrack = FindComByNumber( nCommID );

    do
    {
		  nCharsRead = ReadComm ( pComTrack->iCommId
                , cBuf
                , 100 );
        //Log1( WIDE("Read com chars: %d"), nCharsRead );
        if ( nCharsRead < 0 )
            nCharsRead = -nCharsRead;
        else if( nCharsRead > 0 )
        	   dwWaitUntilAtLeast = GetTickCount() + 100;

        if ( GetTickCount() > dwWaitNoMoreThan )
           break;
    }
    while (  ( nCharsRead > 0 )
          || ( GetTickCount() < dwWaitUntilAtLeast )
          );

   }
//#endif
}

void SetCommRTS( int nCommID, int iRTS )
{
	PCOM_TRACK pct = FindComByNumber( nCommID );
	if( pct )
	{
#ifdef WIN32
		//if( iRTS && iRTSFlow )
		//	pct->dcb.fRtsControl = RTS_CONTROL_TOGGLE;
		//	else
		if( iRTS )
			pct->dcb.fRtsControl = RTS_CONTROL_ENABLE;
		else
			pct->dcb.fRtsControl = RTS_CONTROL_DISABLE;
      EnterCriticalSec( &pct->csOp );
		if ( SetCommState( (HANDLE)(intptr_t)nCommID, &pct->dcb ) )
		{
		}
      LeaveCriticalSec( &pct->csOp );
#endif
	}
}

void SackCommOwnPort( int nCommID, CommReadCallback func, int own_flags )
{
	PCOM_TRACK pct = FindComByNumber( nCommID );
	if( pct )
	{
		PCHANNEL_CALLBACK pcc = pct->callbacks;
		while( pcc )
		{
			if( pcc->func == func )
			{
				switch( own_flags )
				{
				case COM_PORT_OWN_SHARE:
					pcc->flags.skip_read = 0;
					pct->flags.bOwned = 0;
               pct->exclusive = NULL;
					break;
				case COM_PORT_OWN_EXCLUSIVE:
					pcc->flags.skip_read = 0;
					pct->flags.bOwned = 1;
               pct->exclusive = pcc;
					break;
				case COM_PORT_IGNORE:
					pcc->flags.skip_read = 1;
					pct->flags.bOwned = 0;
					pct->exclusive = NULL;
					break;
				}
				break;
			}
			pcc = pcc->next;
		}
	}
}

//-----------------------------------------------------------------------
//PRELOAD( InitCommSack )
//{
//}

//-------------------------------------------------------------
