#define RELAY_SOURCE
#define DEFINE_DEFAULT_RENDER_INTERFACE
#define DO_LOGGING
#include <stdhdrs.h>
#include <time.h>
#include <netservice.h>
#include <ctype.h>
#include <stdio.h>
#include <network.h>
#include <sharemem.h>
#include <timers.h>
#include <idle.h>
#include <logging.h>
#include <filemon.h>
#include <sqlgetoption.h>
#include "account.h"
#include "relay.h"

#include "global.h"

#ifdef _WIN32
#include "resource.h"
#include "systray.h"
#endif

#define GetLastMsg ((pns)->last_message)
#define SetLastMsg(n) ((pns)->last_message = (n))

PCLIENT TCPControl;
int maxconnections;
PCONNECTION Connection;
PLINKQUEUE pdq_update_commands;
PTHREAD update_command_sender;
int bLoggedIn;
int bDone;
int bForceLowerCase;

//---------------------------------------------------------------------------

_32 MakeVersionCode( char *version )
{
    _32 accum = 0;
    _32 code;
    while( version[0] && version[0] != '.' )
    {
        accum *= 10;
        accum += version[0] - '0';
        version++;
    }
    code = accum << 16;
    if( version[0] )
        version++;
    accum = 0;
    while( version[0] && version[0] != '.' )
    {
        accum *= 10;
        accum += version[0] - '0';
        version++;
    }
    code += accum;
    return code;
}

//---------------------------------------------------------------------------

static void CPROC TCPClose( PCLIENT pc ) /*FOLD00*/
{
	PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pc, 0 );
	if( !pns )
	{
		lprintf( "partially connected client disconnected %p", pc );
		return;
	}
	lprintf( "Client Disconnected... %p", pc );
	//if( pc == TCPClient )
	pns->flags.bWantClose = 1;
	while( pns->flags.bInUse )
	{
		Relinquish();
	}
	if( pns )
	{
		{
			PACCOUNT account = pns->account;
			if( account )
			{
				account->client.TCPClient = NULL;
				Logout( account, pns->client_connection );
			}
		}

		{
			PCONNECTION mycon = pns->connection;
			if( mycon ) // wasn't open... don't bother we're done.
			{
				if( mycon->pc == pc )
				{
					PACCOUNT myacct = pns->account;
					Logout( myacct, pns->client_connection );
					mycon->pc = NULL; // is now an available connection.
				}
				else
				{
					lprintf( "Fatal: Closing socket on a connection that wasn't this one..." );
				}
			}
		}
		Release( pns->buffer );
		SetNetworkLong( pc, 0, (PTRSZVAL)NULL );
		Release( pns );
	}
	lprintf( "Client Disconnect DONE..." );
}

//---------------------------------------------------------------------------
typedef struct mytime_tag {
    unsigned char year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;
    unsigned char second;
} MYTIME, *PMYTIME;

void SendTimeEx( PCLIENT pc, int bExtended ) /*FOLD00*/
{
    _32 timebuf[4];
    PMYTIME sendtime;
    if( bExtended )
    {
        timebuf[0] = *(_32*)"TIME";
        timebuf[1] = *(_32*)" NOW";
        sendtime = (PMYTIME)(timebuf+2);
    }
    else
    {
        timebuf[0] = *(_32*)"TIME";
        sendtime = (PMYTIME)(timebuf+1);
    }

#ifdef _WIN32
    {
        SYSTEMTIME st;
        GetLocalTime( &st );
        sendtime->year = st.wYear - 2000;
        sendtime->month = (_8)st.wMonth;
        sendtime->day = (_8)st.wDay;
        sendtime->hour = (_8)st.wHour;
        sendtime->minute = (_8)st.wMinute;
        sendtime->second = (_8)st.wSecond;
    }
#else
    {
        struct tm *timething;
        struct tm timebuf;
        time_t timevalnow;
        time(&timevalnow);
        timething = gmtime_r( &timevalnow, &timebuf );
        sendtime->year = timething->tm_year - 100;
        sendtime->month = timething->tm_mon+1;
        sendtime->day = timething->tm_mday;
        sendtime->hour = timething->tm_hour;
        sendtime->minute = timething->tm_min;
        sendtime->second = timething->tm_sec;
    }
#endif
    /*
     Log6( "Send time: %02d/%02d/%02d %d:%02d:%02d"
     , sendtime->month
     , sendtime->day
     , sendtime->year
     , sendtime->hour
     , sendtime->minute
     , sendtime->second );
     */
    if( bExtended )
        SendTCP( pc, timebuf, 8 + sizeof(MYTIME) );
    else
        SendTCP( pc, timebuf, 4 + sizeof(MYTIME) );
}

//---------------------------------------------------------------------------

// returns true if within time delta
LOGICAL CompareTime( SYSTEMTIME *st1, SYSTEMTIME *st2, int seconds )
{
		FILETIME ft1;
		FILETIME ft2;
		ULARGE_INTEGER li1;
		ULARGE_INTEGER li2;

		SystemTimeToFileTime( st1, &ft1 );
		li1.u.LowPart = ft1.dwLowDateTime;
		li1.u.HighPart = ft1.dwHighDateTime;

		SystemTimeToFileTime( st2, &ft2 );
		li2.u.LowPart = ft2.dwLowDateTime;
		li2.u.HighPart = ft2.dwHighDateTime;

		if( abs( li1.QuadPart - li2.QuadPart ) > ( (_64)seconds * ( ( 10LL /* microsecond scale*/) * ( 1000LL /*millisecond scale*/ ) * ( 1000 /* second scale */ ) ) ) )
			return FALSE;
		return TRUE;
}

void SetTime( char *buffer ) /*FOLD00*/
{
	PMYTIME time = (PMYTIME)buffer;
   /*
	 lprintf( "Got time Something like: %02d/%02d/%02d %d:%02d:%02d"
			  , time->month
			  , time->day
			  , time->year
			  , time->hour
			  , time->minute
			  , time->second );
   */
#ifdef _WIN32
    {
        SYSTEMTIME st, stNow;
        memset( &st, 0, sizeof( SYSTEMTIME ) );
        st.wYear = 2000 + time->year;
        st.wMonth = time->month;
        st.wDay = time->day;
        st.wHour = time->hour;
        st.wMinute = time->minute;
        st.wSecond = time->second;

		  GetLocalTime( &stNow );
        /*
		  lprintf( "Now is Something like: %02d/%02d/%02d %d:%02d:%02d"
					, stNow.wMonth
					, stNow.wDay
					, stNow.wYear
					, stNow.wHour
					, stNow.wMinute
					, stNow.wSecond );
		  */
		  if( CompareTime( &stNow, &st, 5 ) )
		  {
			  lprintf( " --- Time was quite near already" );
			  return;
		  }

        if( !SetLocalTime( &st ) )
        {
            MessageBox( NULL, "Failed to set the clock!", "Obnoxious box!", MB_OK );
        }
    }
#else
    {
        struct tm tmnow, tmgmt, tmlocal;
        time_t utc, local;
        struct timeval tvnow;
        tmnow.tm_year = time->year + 100;
        tmnow.tm_mon = time->month-1;
        tmnow.tm_mday = time->day;
        tmnow.tm_hour = time->hour;
        tmnow.tm_min = time->minute;
        tmnow.tm_sec = time->second;
        tvnow.tv_sec = mktime( &tmnow );
        tvnow.tv_usec = 0;
        gmtime_r( &tvnow.tv_sec, &tmgmt );
        utc = mktime( &tmgmt );
        localtime_r( &tvnow.tv_sec, &tmlocal );
        local = mktime( &tmlocal );
        tvnow.tv_sec += local - utc;
        settimeofday( &tvnow, NULL );
    }
#endif
}

//---------------------------------------------------------------------------

static void CPROC LogOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
   lprintf( "%s", buffer );
}

//---------------------------------------------------------------------------

static void DoVerifyCommands( PNETWORK_STATE pns, PACCOUNT account )
{
	INDEX idx;
	CTEXTSTR update_cmd;
	PCLIENT pc = pns->client_connection->pc;
	DWORD dwIP;
	dwIP = (_32)GetNetworkLong( pc, GNL_IP );
	LIST_FORALL( account->verify_commands, idx, CTEXTSTR, update_cmd )
	{
		TEXTCHAR cmd[4096];
		snprintf( cmd, sizeof( cmd ), "launchcmd -l -s %s -- %s"
				  , (char*)inet_ntoa( *(struct in_addr*)&dwIP )
				  , update_cmd );
		System( cmd, LogOutput, 0 );
	}
}

//---------------------------------------------------------------------------

static void DoUpdateCommands( PNETWORK_STATE pns, PACCOUNT account )
{
	INDEX idx;
	CTEXTSTR update_cmd;
	PCLIENT pc = pns->client_connection->pc;
	DWORD dwIP;
	dwIP = (_32)GetNetworkLong( pc, GNL_IP );
	LIST_FORALL( account->update_commands, idx, CTEXTSTR, update_cmd )
	{
		TEXTCHAR cmd[4096];
		snprintf( cmd, sizeof( cmd ), "launchcmd -l -s %s -- %s"
				  , (char*)inet_ntoa( *(struct in_addr*)&dwIP )
				  , update_cmd );
		System( cmd, LogOutput, 0 );
	}
}

//---------------------------------------------------------------------------

static PTRSZVAL CPROC UpdateCommandThread( PTHREAD thread )
{
	do
	{
		PNETWORK_STATE pns;
		if( pns = (PNETWORK_STATE)DequeLink( &pdq_update_commands ) )
		{
			if( pns->flags.want_update_commands )
				DoUpdateCommands( pns, pns->account );
			else
				DoVerifyCommands( pns, pns->account );
			if( !pns->flags.bWantClose )
			{
				PCLIENT pc = pns->client_connection->pc;
				lprintf( "result done after update commands. to %p", pc );
				SendTCP( pc, "DONE", 4 ); // tell him we're done so he can close files
			}
			else
				lprintf( "connection closed as a result of commands issued." );
			pns->flags.bInUse = 0;
		}
		else
			WakeableSleep( SLEEP_FOREVER );
	}
	while( 1 );
}

//---------------------------------------------------------------------------

struct manifest_args
{
	POINTER buffer;
	size_t size;
	PNETWORK_STATE pns;
};

static PTRSZVAL CPROC DoProcessManifest( PTHREAD thread )
{
	struct manifest_args *thread_args = (struct manifest_args *)GetThreadParam( thread );
	struct manifest_args args = *(thread_args);
	// set that we got a copy of the args.
	thread_args->pns = NULL;
	args.pns->flags.bInUse = 1;
	lprintf( "Begin Process manifest" );
	if( ProcessManifest( args.pns, args.pns->account, (_32*)args.buffer, args.size ) )
	{
		lprintf( "Manifest resulted in changes" );
		if( !args.pns->flags.bWantClose )
		{
			if( g.flags.bServeClean || args.pns->account->flags.bClean )
				ScanForDeletes( args.pns->account );
		}
		if( !args.pns->flags.bWantClose )
			ProcessFileChanges( args.pns->account, args.pns->client_connection );
	}
	args.pns->flags.bInUse = 0;
	lprintf( "Finished with process manifest" );
	return 0;
}

//---------------------------------------------------------------------------

static void UpdateFailureStatus( TEXTCHAR *msg )
{
	PACCOUNT account = g.AccountList;
   if( account && account->client.frame )
	{
		SetControlText( GetControl( account->client.frame, 1 ), msg );
		SetControlText( GetControl( account->client.frame, 2 ), "" );
		SetControlText( GetControl( account->client.frame, 3 ), "" );
		SetControlText( GetControl( account->client.frame, 4 ), "" );
	}
}

//---------------------------------------------------------------------------

static void CPROC TCPRead( PCLIENT pc, POINTER buffer, size_t size ) /*FOLD00*/
{
	int toread = 4;
	if( g.flags.log_network_read )
	{
		lprintf( "Received on %p into %p %d", pc, buffer, size );
		LogBinary( buffer, (size<1024)?size:1024 );
	}

	if( !buffer )
	{
		PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pc, 0 );
		if( pns == NULL )
		{
			pns = New( NETWORK_STATE );
			MemSet( pns, 0, sizeof( NETWORK_STATE ) );
			SetNetworkLong( pc, 0, (PTRSZVAL)pns );
		}
		pns->buffer =
			buffer = Allocate( 4096 );
		pns->last_message_time = timeGetTime();
		SetLastMsg( 0 );
		SetTCPNoDelay( pc, TRUE );
	}
	else
	{
		int LogKnown = TRUE;
		int bReleaseCrc = TRUE;
		PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pc, 0 );
		pns->last_message_time = timeGetTime();
		do
		{
			// handle receive PING/PONG
			if( !GetLastMsg && size == 4 )
			{
				// should really option verbose logging or not...
				if( g.flags.log_network_read )
					lprintf( "Got Msg: %d %4.4s", size, buffer );
				((_8*)buffer)[4] = 0;

				if( *(_32*)buffer == *(_32*)"OPTS" )
				{
					SetLastMsg( *(_32*)buffer );
					toread = 4;
				}
				else if( *(_32*)buffer == *(_32*)"OVRL" )
				{
					SetLastMsg( *(_32*)buffer );
					toread = 8;
				}
				else if( *(_32*)buffer == *(_32*)"SCAN" )
				{
					PACCOUNT account = pns->account;
					INDEX idx;
					PMONDIR pDir;
					LIST_FORALL( pns->client_connection->Monitors, idx, PMONDIR, pDir )
					{
						MonitorForgetAll( pDir->monitor );
					}
					account->DoScanTime = timeGetTime() - 1;
				}
				else if( *(_32*)buffer == *(_32*)"PONG" )
				{
					pns->pings_sent = 0;
					LogKnown = FALSE;
				}
				else if( *(_32*)buffer == *(_32*)"PING" )
				{
					SendTCP( pc, "PONG", 4 );
					LogKnown = FALSE;
				}
				else if( *(_32*)buffer == *(_32*)"MANI" )
				{
					pns->account->flags.manifest_process = 1;
					SetLastMsg( *(_32*)buffer );
					toread = 4;
				}
				else if( *(_32*)buffer == *(_32*)"FILE" ||
						  *(_32*)buffer == *(_32*)"STAT" )
				{
					// message sent when there may be a file change...
					// responce to this is SEND(from, length), NEXT, WHAT
					// lprintf( "Remote has a file to send..." );
					SetLastMsg( *(_32*)buffer );
					if( pns->version >= VER_CODE(3,2) )
					{
						toread = 36+1; // length(dword), time(qword), pathid dword and name length byte
					}
					else
						toread = 32+1; // length(dword), time(qword), pathid dword and name length byte
					LogKnown = FALSE;
				}
				else if( *(_32*)buffer == *(_32*)"FDAT" ||
						  *(_32*)buffer == *(_32*)"CDAT" )
				{
					//lprintf( "Receiving file data..." );
					LogKnown = FALSE;
					SetLastMsg( *(_32*)buffer );
					if( pns->client_connection->version >= VER_CODE( 3, 2 ) )
						toread = 16;
					else	
						toread = 8;
				}
				else if( *(_32*)buffer == *(_32*)"SEND" ||
						  *(_32*)buffer == *(_32*)"CSND"  )
				{
					// message sent indicating that the file is not current
					// and bytes (from) for (length) should be sent...
					// if from == EOF and length == 0 the file is up to date
					if( pns->client_connection->version >= VER_CODE( 3, 2 ) )
						toread = 16;
					else
						toread = 8; // get amount of data to send...

					LogKnown = FALSE;
					SetLastMsg( *(_32*)buffer );
				}
				else if( ( *(_32*)buffer == *(_32*)"FAIL" ) )
				{
					pns->flags.bInUse = 1;
					pns->flags.want_update_commands = 0;
					EnqueLink( &pdq_update_commands, pns );
					WakeThread( update_command_sender );
					//ThreadTo( VerifyCommandThread, (PTRSZVAL)pns );
					toread = 4;
				}
				else if( ( *(_32*)buffer == *(_32*)"NEXT" ) ||
						  ( *(_32*)buffer == *(_32*)"WHAT") )
				{
					if( pns->version < VER_CODE( 3, 2 ) )
					{
						INDEX idx;
						PMONDIR pDir;
						int changes = 0;
						{
							if( *(_32*)buffer == *(_32*)"WHAT")
								lprintf( "The server has no idea what to do with specified file." );
						}

						LIST_FORALL( pns->client_connection->Monitors, idx, PMONDIR, pDir )
						{
							if( pDir->flags.bStaticData )
							{
								if( changes = NextChange( pns->client_connection ) )
									break;
							}
							else
							{
								if( changes = DispatchChanges( pDir->monitor ) )
									break;
							}
						}
						if( !changes )
						{
							//lprintf( "Accouncing DONE..." );
							if( pns->client_connection->flags.bUpdated )
							{
								pns->flags.bInUse = 1;
								pns->flags.want_update_commands = 1;
								EnqueLink( &pdq_update_commands, pns );
								WakeThread( update_command_sender );
								//ThreadTo( UpdateCommandThread, (PTRSZVAL)pns );
							}
							else
							{
								lprintf( "No changes left; send done on %p", pc );
								SendTCP( pc, "DONE", 4 ); // tell him we're done so he can close files
							}
						}
					}
					else
					{
						lprintf( "Client finished manifest (and all data), send next manifest? %p", pc );
						if( pns->client_connection->flags.bUpdated )
						{
							pns->flags.bInUse = 1;
							pns->flags.want_update_commands = 1;
							EnqueLink( &pdq_update_commands, pns );
							WakeThread( update_command_sender );
							//ThreadTo( UpdateCommandThread, (PTRSZVAL)pns );
						}
						else
						{
							lprintf( "no updates, send done on %p", pc );
							SendTCP( pc, "DONE", 4 ); // tell him we're done so he can close files
						}
					}
					LogKnown = FALSE;
				}
				else if( *(_32*)buffer == *(_32*)"DONE" )
				{
					//lprintf( "Client has no further changes to report." );
					CloseCurrentFile( pns->account, pns );
					LogKnown = FALSE;
					if( g.flags.exit_when_done )
					{
						lprintf( "Done, and exiting..." );
						bDone = TRUE;
						WakeThread( g.main_thread );
						return;
						//exit(0);
					}
				}
				else if( *(_32*)buffer == *(_32*)"KILL" )
				{
					LogKnown = FALSE;
					toread = 5; // 4 bytes of pathID 1 byte name length
					SetLastMsg( *(_32*)buffer );
				}
				else if( *(_32*)buffer == *(_32*)"USER"
						  || *(_32*)buffer == *(_32*)"VERS" )
				{
					// message indicates the user identification of the
					// agent connecting.... this updates his profile to
					// select where his data gets mirrored...
					LogKnown = FALSE;
					toread = 1;
					SetLastMsg( *(_32*)buffer );
				}
				else if( *(_32*)buffer == (*(_32*)"TIME") )
				{
					toread = sizeof(MYTIME);
					SetLastMsg( *(_32*)buffer );
				}
				else if( *(_32*)buffer == *(_32*)"OKAY" )
				{
					int len;
					TEXTCHAR msg[64];
					PACCOUNT login = pns->account;
					login->flags.bLoggedIn = 1; // server read 'USER' and responded.
					/*
					 // this used to be used on ack of login to open upload from client to server also.
					 login = Login( pc, "datamirror", (_32)GetNetworkLong( pc, GNL_IP ) ); // 0 is dwIP .. get that..
					 if( login )
					 {
					 SetNetworkLong( pc, NL_ACCOUNT, (_32)login );
					 }
					 else
					 {
					 lprintf( "datamirror account is not configured!" );
					 SendTCP( pc, "QUIT", 4 );
					 RemoveClient( pc );
					 return;
					 }
					 */
					if( g.flags.bServeWindows )
					{
						lprintf( "Sending options" );
						// format of options ...
						// 4 characters (a colon followed by 3 is a good format)
						// the last option must be ':end'.
						// SO - tell the other side we're a windows system and are going to
						// give badly cased files - set forcelower, and this side will do only
						// case-insensitive comparisons on directories and names given.
						len = snprintf( msg, sizeof( msg ), "OPTS%s:end"
										  , (g.flags.bServeClean || login->flags.bClean)?":cln":""
										  , g.flags.bServeWindows?":win":"" );
						lprintf( "Sending message: %s(%d)", msg, len );
						SendTCP( pc, msg, len );
					}
				}
				else if( *(_32*)buffer == *(_32*)"EXIT" )
				{
					// Bad logins can cause this message...
					lprintf( "Login failure... instructed to exit..." );
					bDone = 1;
					WakeThread( g.main_thread );
					return;
				}
				else if( *(_32*)buffer == *(_32*)"RJCT" )
				{
					static int bad_login_count;
					TEXTCHAR msg[64];
					bad_login_count++;
					// Bad logins can cause this message...
					lprintf( "Login failure... instructed to exit... %d", bad_login_count );
					snprintf( msg, 64, "Login Failure:%d", bad_login_count );
					UpdateFailureStatus( msg );
					RemoveClient( pc ); // close this connect and retry.
					return;
				}
				else
				{
					// complain unknown message or something....
					lprintf( "%s Unknown message received size 4 %08lx %s"
						 , pns->account->unique_name
						 , *(_32*)buffer
						 , (TEXTSTR)buffer);
					TCPDrainEx( pc, 0, FALSE ); // drain any present data...
				}
				if( LogKnown )
				{
					if( g.flags.log_network_read )
						lprintf(  "Known message received size 4 %08lx %s"
								 , *(_32*)buffer
								 , (TEXTSTR)buffer);
				}
			}
			else
			{
				_32 *buf = (_32*)buffer;
				//static _32 filesize, filestart, filetime, filepath;
				if( !GetLastMsg )
				{
					lprintf( "Invalid Message recieved: %d no last message buffer:%s", size, (char*)buffer );
				}
				else if( GetLastMsg == *(_32*)"OVRL" )
				{
					SetLastMsg( 0 );
					pns->account->files.start = timeGetTime();
					pns->account->files.count = buf[0];
					pns->account->files.size = buf[1];
				}
				else if( ( GetLastMsg == (*(_32*)"OPTS" ) ) )
				{
					// toread will ahve already been set to 4 - unless some option
					// indicates more data.....
					if( *(_32*)buffer == *(_32*)":win" )
					{
						//lprintf( "Windows remote - setting force lower case..." );
						// and well - there's not much else to do about a windows thing on the other side.
						bForceLowerCase = TRUE;
					}
					else if( *(_32*)buffer == *(_32*)":end" )
					{
						//lprintf( "Received last option - clear last msg" );
						// last option - clear last message...
						SetLastMsg( 0 );
					}
					else if( *(_32*)buffer == *(_32*)":cln" )
					{
						// lprintf( "other side indicates we want to serve clean (option from server to client)" );
						// this options may be set on the client without the server knowing, and it will also clean itself.
						g.flags.bServeClean = 1;
					}
					else
					{
						lprintf( "Unknown option: %4.4s - continuing to read options", buffer );
					}
					break;
				}
				else if( GetLastMsg == *(_32*)"SEND" )
				{
					_32 *buf = (_32*)buffer;
					// 8 bytes in buffer are position and length of data to send...
					{
						PACCOUNT account = pns->account;
						PDIRECTORY pDir = ( pns->client_connection->version >= VER_CODE( 3, 2 ) )
							?(account?(PDIRECTORY)GetLink( &account->Directories, buf[2] ):NULL)
							:NULL;
						PFILE_INFO pFileInfo = pDir?((PFILE_INFO)GetLink( &pDir->files, buf[3] )):NULL;
                  if( !pFileInfo )
							lprintf( "pDir %p, index %d:%d (%d", pDir, buf[2], buf[3], pDir?pDir->files->Cnt:(-1234) );
						if(  buf[1] & 0xFF000000 )
						{
							RemoveClient( pc );
							return;
						}
						if( account )
						{
							xlprintf(2100)( "%s: Send Change[%s]( %ld, %ld )", account->unique_name, pFileInfo?pFileInfo->name:"<NULL>", buf[0], buf[1] );
						}
						else
						{
							lprintf( "<no account>: Send Change[%s]( %ld, %ld )", pFileInfo?pFileInfo->name:"<NULL>", buf[0], buf[1] );
						}
						//lprintf( "pns %p client %p last %p", pns, pns->client_connection, pns->client_connection->LastFile );
						if( pns->client_connection->version >= VER_CODE( 3, 2 ) )
						{
							if( !SendFileChange( account, pc, buf[2], buf[3], NULL, buf[0], buf[1] ) )
							{
								lprintf( "Fatality - current monitor was lost on %s", account->unique_name );
								RemoveClient( pc );
								return;
							}
						}
						else
						{
							if( !SendFileChange( account, pc, 0, 0, pns->client_connection->LastFile, buf[0], buf[1] ) )
							{
								lprintf( "Fatality - current monitor was lost on %s", account->unique_name );
								RemoveClient( pc );
								return;
							}
						}
					}
				}
				else if( GetLastMsg == *(_32*)"FDAT" )
				{
					pns->filestart = buf[0];
					pns->file_block_size = buf[1];
					if( pns->client_connection->version >= VER_CODE( 3, 2 ) )
					{
						pns->path_id = buf[2];
						pns->file_id = buf[3];
					}
					else
						pns->file_id = 0xFFFFFFFFU;
					SetLastMsg( GetLastMsg + 1 );
					if( pns->file_block_size )
					{
						//lprintf( "Starting read into buffer %d", pns->file_block_size );
						ReadTCPMsg( pc
									 , GetAccountBuffer( pns->client_connection, pns->file_block_size )
									 , pns->file_block_size );
					}
					else
					{
						lprintf( "zero byte read into buffer... (zero size file, just create it)" );
						TCPRead( pc, buffer, 0 );
					}
					return;
				}
				else if( GetLastMsg == (*(_32*)"FDAT")+1 )
				{
					//lprintf( "Writing data received into file... FPI:%ld filesize:%ld", pns->filestart, pns->filesize );
					UpdateAccountFile( pns->account
										  , pns->filestart
										  , pns->file_block_size
										  , pns
										  );
					pns->account->finished_files.count++;
					pns->account->finished_files.size += pns->file_block_size;
					if( pns->version >= VER_CODE( 3, 2 ) )
						ProcessFileChanges( pns->account, pns->client_connection );
					// reset the read buffer away from the large account buffer;
					buffer = pns->buffer;

				}
				else if( GetLastMsg == *(_32*)"MANI" )
				{
					SetLastMsg( GetLastMsg + 1 );
					buffer = pns->buffer;
					toread = buf[0];

					ReadTCPMsg( pc
							, GetAccountBuffer( pns->client_connection, toread )
							, toread );
					return;
				}
				else if( GetLastMsg == (*(_32*)"MANI")+1 )
				{
					struct manifest_args args;
					args.buffer = buffer;
					args.size = size;
					args.pns = pns;
					// verify failure returns false.
					pns->account->files.start = timeGetTime();
					ThreadTo( DoProcessManifest, (PTRSZVAL)&args );
					while( args.pns )
						Relinquish();
					// revert to normal network buffer; account buffer was larger from manifest data read
					buffer = pns->buffer;
				}
				else if( GetLastMsg == *(_32*)"FILE" ||
						  GetLastMsg == *(_32*)"STAT" )
				{
					pns->filesize = buf[0];
					pns->filetime_create.dwLowDateTime = buf[1];
					pns->filetime_create.dwHighDateTime = buf[2];
					pns->filetime_modify.dwLowDateTime = buf[3];
					pns->filetime_modify.dwHighDateTime = buf[4];
					pns->filetime_access.dwLowDateTime = buf[5];
					pns->filetime_access.dwHighDateTime = buf[6];
					pns->filepath = buf[7];
					if( pns->client_connection->version >= VER_CODE(3,2) )
					{
						pns->file_id = buf[8];
					}

					//lprintf( "size %08x time %"_64fs" path %08x", pns->filesize, *(_64*)&pns->filetime_modify, pns->filepath );
					SetLastMsg( GetLastMsg + 1 );
					toread = *(char*)(buf+8);
					break;
				}
				else if( GetLastMsg == (*(_32*)"FILE")+1 )
				{
					((char*)buffer)[size] = 0; // nul terminate string...
					if( pns->filesize && pns->filesize != (_32)-1 )
					{
						P_32 crc = (P_32)Allocate( toread = ( sizeof( _32 ) *
																 (( pns->filesize + 4095 ) / 4096 )) );
						//lprintf( "check file on account [%p %p %s] %d", pns->buffer, buffer, buffer, size );
						SetLastMsg( GetLastMsg + 1 );
						buffer = crc;
						size = toread;
						break;
					}
					else
					{
						//lprintf( "Read CRCs(0)!!!!!!" );
					}
					//lprintf( "CHANGE BUFFER FOR CRC to %p", crc );
					bReleaseCrc = FALSE;
					goto do_account;
				}
				else if( GetLastMsg == (*(_32*)"FILE")+2 )
				{
					// actually coincidentally the file buffer 'buffer' is still
					// correct for this...
				do_account:
					{
						P_8 realbuffer = (P_8)pns->buffer;
						//lprintf( "check file on account [%p %s]", realbuffer, realbuffer );
						OpenFileOnAccount( pns
											  , pns->filepath
											  , (char*)realbuffer // network buffer
											  , pns->filesize
											  , pns->filetime_create
											  , pns->filetime_modify
											  , pns->filetime_access
											  , (_32*)buffer    // CRC buffer
											  , (_32)(size / 4)  // count of CRC things
											  );

						pns->account->finished_files.count++;

						if( bReleaseCrc )
						{
							Release( buffer ); // is actually CRC buffer, real network will be used next.
							buffer = realbuffer;
						}
						// then by default to read is correct.
						//
					}
				}
				else if( GetLastMsg == (*(_32*)"STAT")+1 )
				{
					// now - should have total info to perform a stat type operation...
					// PathID comes from outgoing, filename result either triggers
					// a FILE or a KILL result.  This will have to be hung on the list
					// of pending changes....
					PACCOUNT account = pns->account;
					((char*)buffer)[size] = 0; // nul terminate string...
					Log5( "%s Stat file: %s/%s size: %d time: %lld"
						 , account->unique_name
						 , ((PDIRECTORY)GetLink( &account->Directories, pns->filepath ))->path
						 , buffer, pns->filesize
						 , pns->filetime_modify );
					// this is an outgoing monitor therefore 0 based.
					{
						PMONDIR pDir = (PMONDIR)GetLink( &pns->client_connection->Monitors, pns->filepath );
						AddMonitoredFile( pDir->pHandler, (CTEXTSTR)buffer );
						// set next scantime to 500 milliseconds - will allow
						// these guys to get all their changes...
						//lprintf( "Okay and - added that file to the monitor..." );
						// ask for the next file.
						SendTCP( pc, "NEXT", 4 );
					}
				}
				else if( GetLastMsg == *(_32*)"KILL" )
				{
					pns->filepath = buf[0];
					toread = *(_8*)(buf+1);
					SetLastMsg( GetLastMsg + 1 );
					if( toread )
					{
						break;
					}
					else
					{
						lprintf( "No filename associated with KILL" );
						SendTCP( pc, "WHAT", 4 );
					}
				}
				else if( GetLastMsg == *(_32*)"USER"
						  || GetLastMsg == *(_32*)"VERS" )
				{
					toread = *(_8*)buffer;
					SetLastMsg( GetLastMsg + 1 );
					break;
				}
				else if( GetLastMsg == (*(_32*)"VERS")+1 )
				{
					char *version = (char*)buffer;
					_32 version_code;
					version[size] = 0;
					version_code = MakeVersionCode( version );
					if( version_code < VER_CODE(3,0) )
					{
						lprintf( "Version is below acceptable number. Telling client to exit..." );
						SendTCP( pc, "EXIT", 4 );
						return;
					}
					//pns->version = version_code;
					pns->version = version_code;
				}
				else if( GetLastMsg == (*(_32*)"USER")+1 )
				{
					((char*)buffer)[size] = 0; // null terminate text...
					pns->account = Login( pc, (char*)buffer, (_32)GetNetworkLong( pc, GNL_IP ), pns->version ); // 0 is dwIP .. get that..
					if( !pns->account )
					{
						lprintf( "Login failed? Told client to exit..." );
					}
					else
					{
						//lprintf( "We accepted that client..." );
						// for all incoming directories - scan them, and report
						// for 'stat' of files.
					}
				}
				else if( GetLastMsg == (*(_32*)"KILL") + 1 )
				{
					// at this point we should have a valid filename on
					// NL_ACCOUNT relative to remove...
					char filename[256];
					PACCOUNT account = pns->account;
					((P_8)buffer)[size] = 0;
					{
						extern int bForceLowerCase;
						if( bForceLowerCase )
						{
							char *fname;
							for( fname = (char*)buffer; fname[0]; fname++ )
								fname[0] = tolower( fname[0] );
						}
					}
					sprintf( filename, "%s/%s"
							 , ((PDIRECTORY)GetLink( &account->Directories, pns->filepath ))->path
							 , buffer );
					lprintf( "%s is deleting %s", account->unique_name, filename );
					if( remove( filename ) < 0 )
						lprintf( "Failed while deleting file %s", filename );
					SendTCP( pc, "NEXT", 4 );
				}
				else if( GetLastMsg == (*(_32*)"TIME") )
				{
					SetTime( (char*)buffer );
				}
				else if( GetLastMsg == (*(_32*)"QUIT") )
				{
					lprintf( "Connection gave up and quit on me!" );
				}
				else
				{
					int tmp = GetLastMsg;
					Log4( "Unknown message %4.4s: %d bytes %08lx %s"
						 , &tmp
						 , size
						 , *(_32*)buffer
						 , (TEXTSTR)buffer );
					lprintf( "Unknown message closing..." );
					RemoveClient( pc ); // bad state
				}
				toread = 4;
				SetLastMsg( 0 );
			}
		} while(0); // do{} while(0) so we have a chance to use break to get out of inner code.... (replaces goto next_read)
	}
	// normally this would be a VERY bad thing to do...
	if( g.flags.log_network_read )
		lprintf( "read next %d into %p", toread, buffer );
	if( !toread )
		TCPRead( pc, buffer, 0 );
	ReadTCPMsg( pc, buffer, toread );
}

//---------------------------------------------------------------------------

void CPROC TCPControlClose( PCLIENT pc ) /*FOLD00*/
{
    lprintf( "Remote Control closed connection..." );
}

//---------------------------------------------------------------------------

void *DoUpdate( void *data ) /*FOLD00*/
{
    // PCONNECTION Connection = (PCONNECTION)data;
    // now we get to scan all files and do updates for this connection...
    return NULL;
}

//---------------------------------------------------------------------------

void CPROC TCPControlRead( PCLIENT pc, POINTER buffer, size_t size ) /*FOLD00*/
{
	int toread = 8;
	if( !buffer )
	{
		buffer = Allocate( 128 );
	}
	else
	{
		lprintf( "Control Message: %8.8s (%d)", buffer, size );
		((char*)buffer)[size] = 0;
		if( *(_64*)buffer == *(_64*)"DIE NOW!" )
		{
			int i;
			lprintf( "Instructed to die, and I shall do so.\n" );
			//if( TCPClient )
			//    RemoveClient( TCPClient );
			for( i = 0; i < maxconnections; i++ )
			{
				if( Connection[i].pc )
					RemoveClient( Connection[i].pc );
			}
			RemoveClient( pc );
			//while( ProcessNetworkMessages() );
			Idle(); // process net messages should never be called directly, anymore
			bDone = TRUE;
		}
		else if( *(_64*)buffer == *(_64*)"UPDATE  " )
		{
			char *user = (char*)buffer+8;
			//int len;
			//char msg[512];
			//int i;
			lprintf( "Instructed to update for %s", user[0]?user:"everyone" );
			lprintf( "Common updates are not done at the moment..." );
			SendTCP( pc, "ALL DONE", 8 );
			toread = 1;
		}
		else if( *(_64*)buffer == *(_64*)"GET TIME" )
		{
			lprintf( "Someone's asking for time... " );
			SendTimeEx( pc, TRUE );
		}
		else if( *(_64*)buffer == *(_64*)"MASTER??" )
		{
			lprintf( "Responding with Game master status... " );
			SendTCP( pc, "MESSAGE!\x1aWhich Master status?? N/A ", 35 );
			SendTCP( pc, "ALL DONE", 8 );
		}
		else if( *(_64*)buffer == *(_64*)"LISTUSER" )
		{
			char result[1024]; // INSUFFICENT for large scale!
			PACCOUNT account;
			int i;
			int ofs = 0;
			//lprintf( "Listing users... %d", maxconnections );
			SendTCP( pc, "USERLIST", 8 );
			ofs = 0;
			for( i = 0; i < maxconnections; i++ )
			{
				if( Connection[i].pc )
				{
					char version[64];
					PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( Connection[i].pc, 0 );
					_32 version_code = pns->client_connection?pns->client_connection->version:0;
					if( !version_code )
						strcpy( version, "unknown" );
					else
						sprintf( version, "%ld.%ld"
								 , version_code >> 16
								 , version_code & 0xFFF );
					account = pns->account;
					if( account )
					{
						_32 IP = (_32)GetNetworkLong( Connection[i].pc, GNL_IP );
						//lprintf( "User %s connected...", account->unique_name );
                        ofs += sprintf( result+ofs, "%s (%d) %s %s %s\n"
                                       , account->unique_name
                                       , account->logincount
                                       , version
                                       , inet_ntoa( *(struct in_addr*)&IP )
                                       , 0/*GetNetworkLong( Connection[i].pc, NL_SYSUPDATE )*/?"(U)":"" );
                    }
                    else
                    {
                        lprintf( "No account on connection!?!?!" );
                    }
                }
            }
            SendTCP( pc, &ofs, 2 );
            SendTCP( pc, result, ofs );
        }
        else if( *(_64*)buffer == *(_64*)"MEM DUMP" )
        {
            DebugDumpMemFile( "Memory.Dump" );
            SendTCP( pc, "ALL DONE", 8 );
        }
        else if( *(_64*)buffer == *(_64*)"KILLUSER" )
        {
            // need to adjust the receive here....
               char *user = (char*)buffer+8;
                int i;
					 for( i = 0; i < maxconnections; i++ )
					 {
						 if( Connection[i].pc )
						 {
							 PACCOUNT account;
							 PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( Connection[i].pc, 0 );
							 account = pns->account;
							 if( strcmp( account->unique_name, user ) == 0 )
							 {
                            RemoveClient( Connection[i].pc );
                            SendTCP( pc, "MESSAGE!\x1aUser connection terminated", 35 );
                            SendTCP( pc, "ALL DONE", 8 );
                            break;
                        }
                    }
                }
                if( i == maxconnections )
                    RemoveClient( pc );
          }
          else if( *(_64*)buffer == *(_64*)"DO SCAN!" )
             {
                 char *user;
                 int i;
                 if( size > 8 )
                     user = (char*)buffer + 8;
                 else
                     user = NULL;
                 if( user )
                     lprintf( "Instructed to do scan for %s", user );
                 for( i = 0; i < maxconnections; i++ )
                 {
                     if( Connection[i].pc )
                     {
                         PACCOUNT account;
								 PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( Connection[i].pc, 0 );
                         account = pns->account;
                         if( account )
                         {
                             if( !user || strcmp( account->unique_name, user ) == 0 )
                             {
                                 char msg[256];
                                 PMONDIR pDir;
                                 INDEX idx;
                                 int len;
                                 lprintf( "Account for %s found...", account->unique_name );
                                 SendTCP( Connection[i].pc, "SCAN", 4 );
                                 len = sprintf( msg + 9, "Client %s found, issued scan.", account->unique_name );
                                 len += sprintf( msg, "MESSAGE!%c", len );
                                 msg[9] = 'C';
                                 SendTCP( pc, msg, len );
                                 LIST_FORALL( pns->client_connection->Monitors, idx, PMONDIR, pDir )
                                 {
                                     MonitorForgetAll( pDir->monitor );
                                 }
                                 account->DoScanTime = timeGetTime() - 1;
                                 if( user )
                                 {
                                     SendTCP( pc, "ALL DONE", 8 ); // having problems closing this - messages don't get out
                                     break;
                                 }
                             }
                         }
                     }
				}
				SendTCP( pc, "ALL DONE", 8 ); // having problems closing this - messages don't get out
            }
			else if( *(_64*)buffer == *(_64*)"REBOOT!!" )
            {
				char *user = (char*)buffer+8;
                int i;
                lprintf( "Instructed to do reboot for %s", user );
            for( i = 0; i < maxconnections; i++ )
            {
               if( Connection[i].pc )
               {
					PACCOUNT account;
					PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( Connection[i].pc, 0 );
					account = pns->account;
					if( account )
					{
                        if( strcmp( account->unique_name, user ) == 0 )
                        {
                            char msg[256];
                            int len;
                            lprintf( "Account for %s found...", account->unique_name );
                            SendTCP( Connection[i].pc, "REBT", 4 );
                            len = sprintf( msg+9, "Client found, issued reboot." );
                            len += sprintf( msg, "MESSAGE!%c", len );
                            msg[9] = 'C';
                            SendTCP( pc, msg, len );
                            SendTCP( pc, "ALL DONE", 8 );
                            break;
                        }
                    }
                }
            }
            if( i == maxconnections )
                RemoveClient( pc );
        }
		else
		{
            lprintf( "Unknown message from controller: %8.8s", buffer );
			RemoveClient( pc );
			//exit(0);
		}
    }
    ReadTCP( pc, buffer, 128 ); // should be okay...
    //lprintf( "Read enabled on Control Connection!" );
}

//---------------------------------------------------------------------------

void CPROC TCPControlConnect( PCLIENT pServer, PCLIENT pNew ) /*FOLD00*/
{
   // should wait for a command here...
   // since this control port could be useful for things like 'update common'
   // update self...
   lprintf( "Control program attached!" );
   SetTCPNoDelay( pNew, TRUE );
   SetReadCallback( pNew, TCPControlRead );
}


//---------------------------------------------------------------------------

void CPROC TCPConnection( PCLIENT pServer, PCLIENT pNew ) /*FOLD00*/
{
	int i;
	lprintf( "New connection at %p", pNew );
	for( i = 0; i < maxconnections; i++ )
	{
		if( pNew == Connection[i].pc )
			lprintf( "We got a second instance of a client we KNEW about!\n" );
	}
	for( i = 0; i < maxconnections; i++ )
	{
		if( !Connection[i].pc )
		{
			PNETWORK_STATE pns;
			_32 dwIP = (_32)GetNetworkLong( pNew, GNL_IP );
			pns = New( NETWORK_STATE );
			MemSet( pns, 0, sizeof( NETWORK_STATE ) );
			SetNetworkLong( pNew, 0, (PTRSZVAL)pns );
			lprintf( "New client %d %s", i, inet_ntoa( *(struct in_addr*)&dwIP ) );
			pns->connection = Connection+i;
			Connection[i].LastCommunication = timeGetTime();
			Connection[i].pc = pNew;
			SetCloseCallback( pNew, TCPClose );
			SetReadCallback( pNew, TCPRead );
			//lprintf( "Okay... set up everything and going to continue... " );
			break;
		}
	}
	if( i == maxconnections )
	{
		lprintf( "Too many connections!" );
		RemoveClient( pNew );
	}
	lprintf( "Finished accepting new connection..." );
}

//---------------------------------------------------------------------------

int bServerRunning;

PTRSZVAL CPROC ServerTimerProc( PTHREAD unused )
{
	int bRetry = 0;
	bServerRunning = 1;
	while(1)
	{
		int did_one = 0;
		int i;
		bRetry = 0;
		for( i = 0; i < maxconnections; i++ )
		{
			// if the lock fails it's no longer active.
			PCLIENT pcping;
			if( !Connection[i].pc )
				continue;
			did_one = 1;
			pcping = NetworkLock( Connection[i].pc );
			// PCLIENT pcping = Connection[i].pc;
			if( pcping )
			{
				PNETWORK_STATE pns = (PNETWORK_STATE)GetNetworkLong( pcping, 0 );
				int version_code = pns->client_connection?pns->client_connection->version:0;

				// a slightly longer time here will prevent
				// both the client and the server reverse pinging...
				// worst case we should never expect more than
				// really 2 pings (3rd fail) but we pad an extra
				// just to be certain....

				if( pns->account )
				{
					_32 IP = (_32)GetNetworkLong( pcping, GNL_IP );
					//lprintf( "User %s connected...", account->unique_name );

					lprintf( "%p Checking connection %d %ld.%ld %s(%d) %s"
						, pcping 
						, i
							, version_code >> 16
								, version_code & 0xFFF
                            , pns->account->unique_name
                            , pns->account->logincount
                            , inet_ntoa( *(struct in_addr*)&IP )
									);
					if( ( pns->last_message_time + 13000 )
						< timeGetTime() )

					{
						if( pns->pings_sent > 4 )
						{
							lprintf( "Connection[%d] %s is not responding... closing", i,
									  ((PACCOUNT)pns->account)->unique_name );
							pns->pings_sent = 0;
							RemoveClient( pcping );
						}
						else
						{
							lprintf( "Connection is quiet, sending ping." );
							pns->last_message_time = timeGetTime();
							pns->pings_sent++;
							SendTCP( pcping, "PING", 4 );
						}
					}
                }
                else
                {
					lprintf( "%p No account on connection %d!?!?!"
						, pcping 
						, i );
					if( pns->login_fails++ > 3 )
					{
						pns->login_fails = 0;
						RemoveClient( pcping );
					}
                }
				NetworkUnlock( pcping );
			}
			else
			{
				bRetry = 1;
				lprintf( "Failed to get lock... %d", i );
			}
		}
		if( !did_one )
			WakeableSleep( 2000 );
		else if( bRetry )
			WakeableSleep( 2000 );
		else
         WakeableSleep( 2000 );
	}
	bServerRunning = 0;
	return 0;
}

//---------------------------------------------------------------------------

static void CPROC ExitButton( PTRSZVAL psv, PSI_CONTROL pc )
{
   lprintf( "Exit button clicked, indicating done and waking main." );
	bDone = 1;
	WakeThread( g.main_thread );
}

//---------------------------------------------------------------------------

static void OpenStatusDisplay( PACCOUNT account )
{
	TEXTCHAR title[256];
	PSI_CONTROL frame;
   static int status_offset_x = 0;
   static int status_offset_y = 0;
	snprintf( title, sizeof( title ), "%s Status", account->unique_name );
	frame = account->client.frame = CreateFrame( title, 100 + status_offset_x, 0 + status_offset_y, 320, 110, BORDER_NORMAL, 0 );
	status_offset_x += 350;
	if( status_offset_x > 1024 )
	{
		status_offset_x = 0;
      status_offset_y += 130;
	}
	if( frame )
	{
		MakeTextControl( frame, 5, 5, 310, 15, 1, "", 0 );
		MakeTextControl( frame, 5, 25, 310, 15, 2, "", 0 );
		MakeTextControl( frame, 5, 45, 310, 15, 3, "", 0 );
		MakeTextControl( frame, 5, 65, 310, 15, 4, "", 0 );
		MakeButton( frame, 5, 85, 320, 20, -1, "OK", 0, ExitButton, 0 );
		DisplayFrame( frame );
		{
			PRENDERER renderer = GetFrameRenderer( frame );
			MakeTopmost( renderer );
		}
	}
}

static void UpdateStatus( PACCOUNT account )
{
	_32 now = timeGetTime();
	TEXTCHAR msg[256];
	if( account->flags.manifest_process )
	{
		snprintf( msg, sizeof( msg ), "Files(%d) %d bytes", account->manifest_files.count, account->manifest_files.size );
		SetControlText( GetControl( account->client.frame, 1 ), msg );
	}
	else
	{
		snprintf( msg, sizeof( msg ), "Blocks: %d of %d", account->finished_files.count, account->files.count );
		SetControlText( GetControl( account->client.frame, 1 ), msg );
		snprintf( msg, sizeof( msg ), "Bytes: %d of %d", account->finished_files.size, account->files.size );
		SetControlText( GetControl( account->client.frame, 2 ), msg );
		if( ( now - account->files.start ) / 1000 )
		{
			snprintf( msg, sizeof( msg ), "Blocks/sec: %d", account->finished_files.count / ( ( now - account->files.start ) / 1000 ) );
			SetControlText( GetControl( account->client.frame, 3 ), msg );
			snprintf( msg, sizeof( msg ), "Bytes/sec: %d", account->finished_files.size * 10 / ( ( now - account->files.start ) / 100 ) );
			SetControlText( GetControl( account->client.frame, 4 ), msg );
		}
	}
}

//---------------------------------------------------------------------------
int bClientRunning;

PTRSZVAL CPROC ClientTimerProc( PTHREAD thread )
{
	LOGICAL had_client;
	bClientRunning = 1;
	while( 1 )
	{
		PACCOUNT account;
		had_client = 0;
		for( account = g.AccountList; account; account = NextThing( account ) )
		{
			if( !account->flags.opened_status && account->flags.client )
			{
				OpenStatusDisplay( account );
				account->flags.opened_status = TRUE;
			}
#ifdef _WIN32
			if( bDone )
			{
				PostQuitMessage( 0 );
				break;
			}
#endif
			if( account->flags.client )
			{
				PNETWORK_STATE pns;
				had_client = 1;
				if( !account->client.TCPClient )
				{
					account->client.TCPClient = OpenTCPClientAddrEx( account->client.server
																				  , TCPRead
																				  , TCPClose
																				  , NULL );
					if( account->client.TCPClient )
					{
						// might have gotten connection, but not had read work yet.
						while( !(pns = (PNETWORK_STATE)GetNetworkLong( account->client.TCPClient, 0 ) ) )
							Relinquish();
						pns->client_connection = New( CLIENT_CONNECTION );
						memset( pns->client_connection, 0, sizeof( CLIENT_CONNECTION ) );
						pns->client_connection->version = MakeVersionCode( RELAY_VERSION );
						pns->client_connection->file = INVALID_INDEX;
						pns->client_connection->pc = account->client.TCPClient;
						pns->account = account;
					}
				}
				if( account->client.TCPClient )
				{
					pns = (PNETWORK_STATE)GetNetworkLong( account->client.TCPClient, 0 );
					if( pns )
					{
						UpdateStatus( account );
						if( !account->flags.bLoggedIn )
						{
							if( !account->flags.sent_login )
							{
								char msg[256];
								int len;
								lprintf( "need login - try lock?" );
								lprintf( "Connected, and logging in as %s", account->unique_name );
								len = sprintf( msg, "VERS%c%sUSER%c%s"
												 , strlen( RELAY_VERSION ), RELAY_VERSION
												 , strlen( account->unique_name ), account->unique_name );
								pns->version = MakeVersionCode( RELAY_VERSION );
								SendTCP( account->client.TCPClient, msg, len );
								account->flags.sent_login = 1;
							}
						}
						else
						{
							//if( !NetworkLock( account->client.TCPClient ) )
							//    return 0;
							if( ( pns->last_message_time + 10000 )
								< timeGetTime() )
							{
								if( pns->pings_sent > 10 )
								{
									lprintf( "Connection is not responding... closing" );
									pns->pings_sent = 0;
									RemoveClient( account->client.TCPClient );
									// this removes TCPClient which causes an error in the network unlock.
								}
								else
								{
									pns->last_message_time = timeGetTime();
									pns->pings_sent++;
									SendTCP( account->client.TCPClient, "PING", 4 );
									//lprintf( "Sending a ping...." );
								}
							}
						}
					}
				}
				else
				{
					lprintf( "Failed to open the socket?" );
				}
			}
		}
		if( !had_client)
			break;
		WakeableSleep( 250 );
	}
	bClientRunning = 0;
	return 0;
}

ATEXIT( FileMirrorMain )
{
   lprintf( "AtExit fired; might not be the main thread, so trigger done and wake main." );
	bDone = 1;
	WakeThread( g.main_thread );
}

static void usage( void )
{
	lprintf( "relay [-cC <configname] [-xX] [-kK]" );
	lprintf( "   -k klean other files that are not part of this" );
	lprintf( "   -x exit when scan from server is done" );
	lprintf( "   -c specify the config file name" );
	printf( "relay [-cC <configname] [-xX] [-kK]\n" );
	printf( "   -k klean other files that are not part of this\n" );
	printf( "   -x exit when scan from server is done\n" );
	printf( "   -c specify the config file name\n" );
}

static void CPROC MemDump( void )
{
   DebugDumpMem();
}

#ifdef _WIN32
SaneWinMain( argc, argv )
{
#else
int main( char argc, char **argv ) /*FOLD00*/
{
#endif
	PTHREAD pServer = NULL;
	PTHREAD pClient = NULL;

	RegisterIcon( (char*)ICO_RELAY_DOWN );
	AddSystrayMenuFunction( "Memory Dump", MemDump );
#ifdef _WIN32
	g.flags.bServeWindows = 1;
#endif
	g.configname = "Accounts.Data";
	{
		int arg;
		for( arg = 1; arg < argc; arg++ )
		{
			if( argv[arg][0] == '-' )
			{
				switch( argv[arg][1] )
				{
				case 'k':
				case 'K':
					g.flags.bServeClean = 1;
					break;
				case 'c':
				case 'C':
					if( argv[arg][2] )
					{
						g.configname = StrDup( argv[arg]+2 );
					}
					else
					{
						arg++;
                  g.configname = StrDup( argv[arg] );
					}
					break;
				case 'x':
				case 'X':
               g.flags.exit_when_done = 1;
					break;
				default:
					usage();
               break;
				}
			}
		}
	}

#ifndef _WIN32
	// we're a way friendly data only transport...
	umask( 0000 );
#endif
	if( !NetworkWait( 0, 0, 0 ) )
      return 0;
	ChangeIcon( (char*)ICO_RELAY_SCAN );
	ReadAccounts( g.configname );

#ifdef VERIFY_MANIFEST
	{
		PACCOUNT account;
		for( account = g.AccountList; account; account = account->next )
		{
			INDEX idx_dir;
			INDEX idx_file;
			PDIRECTORY directory;
			PFILE_INFO pFileInfo;
			lprintf( "Verifying directories on %s", account );
			LIST_FORALL( account->Directories, idx_dir, PDIRECTORY, directory )
			{
				lprintf( "directory is %s", directory->path );
				LIST_FORALL( directory->files, idx_file, PFILE_INFO, pFileInfo )
				{
					if( pFileInfo->dwSize == 0xFFFFFFFF )
					{
						// is a directory.
						continue;
					}
					{
						TEXTCHAR full_name[MAXPATH];
						int result;
						snprintf( full_name, MAX_PATH, "%s/%s", directory->path, pFileInfo->name );
						result = ReadValidateCRCs( NULL, pFileInfo->crc, pFileInfo->crclen, full_name, pFileInfo->dwSize, pFileInfo );
						//lprintf( "Read validate of %s %d", full_name, result );
						switch( result )
						{
						case 0:
							lprintf( "Read validate of %s %d", full_name, result );
							lprintf( "Failure to open file which is in the manifest.  Mismatch." );
							// file did not exist.
							break;
						case 1:
							// file exists, and matches?
							break;
						case 2:
							lprintf( "Length error : %s", pFileInfo->full_name );
							// too much data on one side or other...
							break;
						case 3:
							lprintf( "CRC mismatch : %s", pFileInfo->full_name );
							// file CRCs mismatched
							break;
						}
					}
				}
			}
		}
	}
#else // VERIFY_MANIFEST

	// 10 minimum for a client...
	// one control service
	// one open to master server
	// one watchdog discover
	// one local watchdog
	// one pos watchdog
	// servers should have lots...
	if( !maxconnections )
	{
		lprintf( "You may want to specify 'max connections=#' in the %s file", g.configname );
		maxconnections = 256;
	}

	Connection = (PCONNECTION)Allocate( sizeof( CONNECTION ) * maxconnections );
	MemSet( Connection, 0, sizeof( CONNECTION ) * maxconnections );
	if( !NetworkWait( 0, maxconnections+3, 1 ) )
	{
		lprintf( "Network did not initialize. Bye." );
		return 0;
	}
	pdq_update_commands = CreateLinkQueue();
	update_command_sender = ThreadTo( UpdateCommandThread, 0 );

	{
		PNETBUFFER pNetBuffer = g.NetworkBuffers;
		lprintf( "First net buffer: %p", g.NetworkBuffers );
		while( pNetBuffer )
		{
			DumpAddr( "Opening server: ", pNetBuffer->sa );
			OpenTCPServerAddrEx( pNetBuffer->sa, TCPConnection );
			pNetBuffer = pNetBuffer->next;
		}
	}

	{
		int altport = 0;
		do
		{
			TCPControl = OpenTCPServerEx( 3001 + altport, TCPControlConnect );
			if( !TCPControl )
			{
				if( !altport )
					altport = 10000;
				else
					altport++;
				if( altport > 10030 )
				{
					lprintf( "Failed to open control socket server" );
					break;
				}
				//Sleep( 2000 );
			}
		} while( !TCPControl );
	}

	if( !g.NetworkBuffers && !g.AccountList )
	{
		lprintf( "No server or client mode defined. Exiting." );
		return 0;
	}

	ChangeIcon( (char*)ICO_RELAY );
	if( g.NetworkBuffers ) // had someone to listen to...
	{
		lprintf( "Starting server...." );
		pServer = ThreadTo( ServerTimerProc, 0 );
	}

	if( g.AccountList ) // had someone to login as...
	{
		lprintf( "Starting client...." );
		pClient = ThreadTo( ClientTimerProc, 0 );
	}

	g.flags.log_network_read = SACK_GetProfileInt( GetProgramName(), "Log network reads", 0 );
	g.flags.log_file_ops = SACK_GetProfileInt( GetProgramName(), "Log file operations", 0 );

#ifdef _WIN32
	g.main_thread = MakeThread();
   lprintf( "In main thread; waiting forever" );
	while( !bDone )
		WakeableSleep( 100000 );
	lprintf( "Done was set, and now we exit...." );

   // set done again, just in case, to allow other threads to end gracefully.
	bDone = 1;
	if( pServer )
		WakeThread( pServer );
	if( pClient )
		WakeThread( pClient );
	{
		_32 start = timeGetTime() + 1000;
		while( bServerRunning && ( start < timeGetTime() ) )
			Relinquish();
      if( bServerRunning )
			EndThread( pServer );
		while( bClientRunning && ( start < timeGetTime() ) )
			Relinquish();
		//if( bClientRunning )
		//	EndThread( pClient );
	}
	UnregisterIcon();
	//CloseAllAccounts();
	//NetworkQuit();
	//Release( Connection );
	//DebugDumpMemFile( "relay.dump" );
#else
	WakeableSleep( SLEEP_FOREVER );
#endif
#endif // VERIFY_MANIFEST

	return 0;
}
EndSaneWinMain( )

