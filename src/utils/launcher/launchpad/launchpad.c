#define DO_LOGGING
#include <stdhdrs.h>
#ifdef WIN32
//#include <shellapi.h>
#endif
#include <sharemem.h>
#include <configscript.h>
#include <network.h>
#include <filesys.h>
#include <timers.h>
#define DO_LOGGING
#include <logging.h>
#include <systray.h>
#include <sqlgetoption.h>

#if defined( BUILD_SERVICE )
#include <service_hook.h>
#endif


#undef strchr
#undef strlen
#undef strncmp
#undef strcpy

// keep a list of all sequences... and expire them at some point?

typedef struct sequence_data *pSequence;
struct sequence_data {
	_32 sequence;
	_32 tick;
	/* we might end up with the same sequence for different packets...
    * happens when we launch two processes at the same time... */
	POINTER packet;
   int packet_length; // paramter to network callback is int
};

enum network_states   {
	NETWORK_STATE_GET_SIZE,
	NETWORK_STATE_GET_COMMAND,
};
struct task_output_network_state {
   enum network_states state;
	size_t toread;
	size_t bufsize;
	POINTER buffer;
};

PLIST class_names;
//CTEXTSTR class_name;
static PLIST sequences;
PCLIENT pcListen;
CTEXTSTR pInterfaceAddr;
static int bLogOutput;
int bLogPacketReceive;

typedef struct restartable_task RTASK;
typedef struct restartable_task *PRTASK;
struct restartable_task
{
	_32 fast_restart_count;
	_32 prior_tick;
	PCTEXTSTR args;
	CTEXTSTR program;
	struct {
		BIT_FIELD bLogOutput : 1;
		BIT_FIELD bHide : 1;
		BIT_FIELD bNonUser : 1;
	} flags;
};

static TEXTCHAR start_path[256];
static PLIST restarts; // these tasks restart when exited...

static void CPROC ExpireSequences( PTRSZVAL psv )
{
	pSequence pSeq = NULL;
	INDEX idx;
	_32 tick = GetTickCount();
	do
	{
		LIST_FORALL( sequences, idx, pSequence, pSeq )
		{
			if( ( pSeq->tick + 5000 ) < tick )
			{
				// moves all in list down a bit...
				SetLink( &sequences, idx, NULL );
				Release( pSeq->packet );
				Release( pSeq );
				break;
			}
		}
	} while( pSeq );
}

static void CPROC GetTaskOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	if( bLogOutput )
		lprintf( WIDE("%s"), buffer );
	{
		PCLIENT pc = (PCLIENT)psv;
		if( pc )
		{
			size_t tmpsize = size + 1;
			SendTCP( pc, &tmpsize, 4 );
			SendTCP( pc, "@", 1 );
			SendTCP( pc, buffer, size );
		}
	}
}



static void CPROC RestartTask( PTRSZVAL psv, PTASK_INFO task )
{
	PRTASK restart = (PRTASK)psv;
	_32 now = GetTickCount();
	lprintf( WIDE("restarting.") );
	if( ( now - restart->prior_tick ) < 3000 )
	{
		restart->fast_restart_count++;
	}
	else
		restart->fast_restart_count = 0;
	restart->prior_tick = now;
	if( restart->fast_restart_count < 3 )
	{
		if( restart->flags.bLogOutput)
		{
#ifdef BUILD_SERVICE
			if( restart->flags.bNonUser )
#endif
				LaunchPeerProgramExx(restart->program
										, start_path[0]?start_path:NULL
										  , (PCTEXTSTR)restart->args
										  , restart->flags.bHide?0:LPP_OPTION_DO_NOT_HIDE
										  , GetTaskOutput
										  , RestartTask
										  , (PTRSZVAL)restart
											DBG_SRC
										  );
#ifdef BUILD_SERVICE
			else
				LaunchUserProcess(restart->program, NULL
									  , (PCTEXTSTR)restart->args
									  , restart->flags.bHide?0:LPP_OPTION_DO_NOT_HIDE
									  , GetTaskOutput
									  , RestartTask
									  , (PTRSZVAL)restart
										DBG_SRC
									  );
#endif
		}
		else
		{
#ifdef BUILD_SERVICE
			if( restart->flags.bNonUser )
#endif
				LaunchProgramEx( restart->program, NULL
									, (PCTEXTSTR)restart->args
									, RestartTask
									, (PTRSZVAL)restart );

#ifdef BUILD_SERVICE
			else
				LaunchUserProcess( restart->program, NULL
									  , (PCTEXTSTR)restart->args
									  , restart->flags.bHide?0:LPP_OPTION_DO_NOT_HIDE
									  , NULL
									  , RestartTask
									  , (PTRSZVAL)restart
									  DBG_SRC );
#endif
		}
	}
	else
	{
		lprintf( WIDE("Task restart was too fast ( 3 times within 3 seconds each ). Failing task.") );
		Release( (POINTER)restart->program );
		DeleteLink( &restarts, restart );
		Release( restart );
	}
}

static void CPROC TaskEnded( PTRSZVAL psv, PTASK_INFO task )
{
	//lprintf( "Caught task ended - eliminates zombies?" );
	if( psv )
	{
		PCLIENT pc = (PCLIENT)psv;
		// we were sending information back to launcher...
		//lprintf( "closing client to launchcmd. %p", pc );
		SetProgramUserData( task, 0 );
		SetNetworkLong( pc, 1, 0 );
		RemoveClient( pc );
	}
}

static void CPROC RemoteClosed( PCLIENT pc )
{
	struct task_output_network_state *pns = (struct task_output_network_state*)GetNetworkLong( pc, 0 );
	PTASK_INFO task;
	if( bLogOutput )
		lprintf( WIDE("remote closed, terminating associated client") );

	if( pns )
	{
		// release TCP buffer
		Release( pns->buffer );
		Release( pns );
	}

	task = (PTASK_INFO)GetNetworkLong( pc, 1 );
	SetNetworkLong( pc, 1, 0 );

	if( task )
	{
		SetProgramUserData( task, 0 );
		//lprintf( WIDE("terminating...") );
		TerminateProgram( task );
		//lprintf( WIDE("termated.") );
	}
}

static void CPROC RemoteSent( PCLIENT pc, POINTER p, size_t size )
{
	struct task_output_network_state *pns = (struct task_output_network_state*)GetNetworkLong( pc, 0 );
	if( !p )
	{
		pns = New( struct task_output_network_state );
		MemSet( pns, 0, sizeof( struct task_output_network_state ) );
		SetNetworkLong( pc, 0, (PTRSZVAL)pns );
		pns->buffer =
			p = Allocate( 256 );
		pns->bufsize = 256;
	}
	else
	{
		switch( pns->state )
		{
		case NETWORK_STATE_GET_SIZE:
			pns->toread = *(_32*)pns->buffer;
			pns->state = NETWORK_STATE_GET_COMMAND;
			if( pns->toread > pns->bufsize )
			{
				pns->bufsize = pns->toread;
				Release( pns->buffer );
				pns->buffer = Allocate( pns->bufsize );
			}
			break;
		case NETWORK_STATE_GET_COMMAND:
			{
				P_8 buf = (P_8)p;
				// do nothing with received input...
				// it's just the other side probing connection alive... maybe... maybe there should be
				// a couple commands - one for control level
				// the other for sending to tasks's stdin channel...
				if( buf[0] == '@' )
				{
					PTASK_INFO task = (PTASK_INFO)GetNetworkLong( pc, 1 );
					buf[size] = 0;
					//lprintf( WIDE("Positing output to task:%s"), buf+1 );
					pprintf( task, WIDE("%s"), buf+1 );
				}
				if( ( buf[0] == '\0' ) && ( size == 1 ) )
				{
					SendTCP( pc, "\1\0\0\0\0", 5 );
				}
			}
			pns->state = NETWORK_STATE_GET_SIZE;
			break;
		}
	}
	if( pns->state == NETWORK_STATE_GET_SIZE )
		ReadTCPMsg( pc, pns->buffer, 4 );
	else
		ReadTCPMsg( pc, pns->buffer, pns->toread );
}

static void CPROC RemoteReverseConnected( PCLIENT pc, int error_code )
{
	//lprintf( "reverse connect complete. %d", error_code );
	if( error_code )
	{
      // didn't actually connect....
	}
}

// packet format
				// [~option][~option]...
				//   option value supported now...
//      'capture' - result to the invoking command the tasks's output
//      'path'<string>% - set start in path
//      'hide' - hide the program instead of show normal(?)
//
// [$classname]
// [^sequence]
// #command
//

struct thread_args
{
	char restart;
			
	SOCKADDR *sa;
	TEXTCHAR *inbuf;
	int bCaptureOutput;
	int bNulArgs;
	TEXTCHAR **args;
	char *seqbuf;
	int hide_process ;
	PRTASK restart_info;
	PCLIENT pc_task_reply;
};

static PTRSZVAL CPROC RemoteBackConnect( PTHREAD thread )
{
	struct thread_args *args = (struct thread_args *)GetThreadParam( thread );
	PTASK_INFO task;
	PCLIENT pc_output = args->pc_task_reply;

	if( pc_output )
	{
		SetNetworkReadComplete( pc_output, RemoteSent );
		SetNetworkCloseCallback( pc_output, RemoteClosed );
	}

	if( args->bCaptureOutput && !pc_output )
	{
      //lprintf( "Want to capture task output, so back-connect." );
		//SetAddressPort( sa, capture_port );
		//DumpAddr( "connect to", sa );
		pc_output = OpenTCPClientAddrExx( args->sa
												  , RemoteSent // no reception
												  , RemoteClosed // no close notice
												  , NULL
												  , RemoteReverseConnected
												  ); // no write notic
	}

	//lprintf( "Launch the process..." );
	task = LaunchPeerProgramExx( args->inbuf, start_path[0]?start_path:NULL
										, (PCTEXTSTR)args->args
										, args->hide_process?0:LPP_OPTION_DO_NOT_HIDE
										, GetTaskOutput
										, args->restart_info?RestartTask:TaskEnded
										, (PTRSZVAL)(args->restart_info?((PTRSZVAL)args->restart_info):((PTRSZVAL)pc_output))
										 DBG_SRC
										);
	//lprintf( "And finally hook the process to its socket.." );
	if( args->bCaptureOutput )
	{
		SetNetworkLong( pc_output, 1, (PTRSZVAL)task );
	}
	if( !task )
	{
		lprintf( WIDE("Oops, task failed, close return socket.") );
		RemoveClient( pc_output );
	}

	ReleaseAddress( args->sa );
	Release( args->inbuf );
	if( !args->restart )
	{
		// restartable tasks want to keep the args...
		Release( args->args );
	}
	Release( args );
	return 0;
}

static void ProcessPacket( PCLIENT pc_reply, POINTER buffer, size_t size, SOCKADDR *sa )
{
	TEXTCHAR restart = 0;
	TEXTCHAR *inbuf;
	int bCaptureOutput = 0;
	int bNulArgs = 0;
	TEXTCHAR **args = NULL;
	TEXTCHAR *seqbuf;
	int hide_process = 0;
	PRTASK restart_info = NULL;
	int capture_port = 3013;
#ifdef UNICODE
	inbuf = CharWConvertExx( (char*)buffer, size DBG_SRC );
#else
	inbuf = buffer;
#endif
	inbuf[size] = 0;
	start_path[0] = 0; // clear start path (default none)
	while( inbuf[0] == '~' )
	{
		if( StrCaseCmpEx( inbuf+1, WIDE("capture"), 7 ) == 0 )
		{
			int port = 0;
			char c;
			bCaptureOutput = 1;
			inbuf += 8;
			while( (c = inbuf[0]),(c >='0' && c <= '9' ) )
			{
				port *= 10;
				port += c - '0';
				inbuf++;
			}
			if( port )
				capture_port = port;
		}
		if( StrCmpEx( inbuf+1, WIDE("hide"), 4 ) == 0 )
		{
			hide_process = 1;
			inbuf += 5;
		}
		if( StrCmpEx( inbuf+1, WIDE("path"), 4 ) == 0 )
		{
			// inbuf is not const, to recast.
			TEXTCHAR *end = (TEXTCHAR*)StrChr( inbuf+5, '%' );
			if( !end )
			{
				lprintf( WIDE("Failure to get end of path.") );
			}
			else
				end[0] = 0;
			StrCpy( start_path, inbuf + 5 );
			inbuf = end + 1;
		}
	}
	if( inbuf[0] == '~' )
	{
		// some error condition - bad option?  bad end of path?
      return;
	}
	/* check to see if the message has a class assicated, if not, process as normal */
	if( inbuf[0] == '$' )
	{
		INDEX idx;
		CTEXTSTR class_name;
		if( class_names )
		{
			TEXTCHAR* tmp_sep = (TEXTCHAR*)StrChr( inbuf + 1, '^' );
			if( tmp_sep )
				tmp_sep[0] = 0;
			LIST_FORALL( class_names, idx, CTEXTSTR, class_name )
			{
				/* has a class name, do we? */
				// message has class, we have a class, check if it matches
				if( bLogPacketReceive )
					lprintf(WIDE( "Does %s == %s" ), inbuf + 1, class_name );
				if( CompareMask( inbuf + 1, class_name, 0 ) )
				{
					inbuf = tmp_sep;
					break;
				}
			}
			if( tmp_sep )
				tmp_sep[0] = '^';
			if( !class_name )
			{
				if( bLogPacketReceive )
					lprintf( WIDE("Command received, but not for my class" ) );
				return;
			}
		}
		else
		{
			if( bLogPacketReceive )
				lprintf( WIDE("Class name passed in packet, but I'm not listening to any class, so it's not for me." ) );
			return;
		}
	}
	if( !inbuf ) // malformed packet.
	{
		if( bLogPacketReceive )
		{
			lprintf( WIDE("Malformed packet..." ) );
			LogBinary( buffer, size );
		}
		return;
	}
	if( inbuf[0] == '^' )
	{
		bNulArgs = 1;
		inbuf++;
	}
	seqbuf = inbuf;
	inbuf = (TEXTCHAR*)StrChr( seqbuf, '#' );
	if( !inbuf )
	{
		inbuf = (TEXTCHAR*)StrChr( seqbuf, '@' );
		if( inbuf )
			restart = 1;
	}
	if( inbuf )
	{
		inbuf[0] = 0;
		inbuf++;
		if( StrChr( inbuf, '=' ) )
		{
			// build as a environvar...
			//also continue scanning for next part of real command
		}
		//LogBinary( (P_8)buffer, size );
		do
		{
			// keep a list of all sequences... and expire them at some point?
			_32 packet_sequence = (_32)IntCreateFromText( seqbuf );
			{
				struct sequence_data *pSeq = NULL;
				INDEX idx;
				LIST_FORALL( sequences, idx, pSequence, pSeq )
				{
					if( pSeq->sequence == packet_sequence )
					{
						if( pSeq->packet_length == size )
							if( MemCmp( pSeq->packet, buffer, size ) == 0 )
                        return;
					}
				}
				if( pSeq ) // bail out of do{}while(0)
				{
					if( bLogPacketReceive )
						lprintf( WIDE("received duplicate, ignoring.") );
               return;
				}
				{
					struct sequence_data *new_seq = New( struct sequence_data );
					new_seq->sequence = packet_sequence;
					new_seq->tick = GetTickCount();
					new_seq->packet = Allocate( size );
					new_seq->packet_length = (int)size;
					MemCpy( new_seq->packet, buffer, size );

					AddLink( &sequences, new_seq );
					if( bLogPacketReceive )
					{
						lprintf( WIDE("Received...") );
						LogBinary( (P_8)buffer, size );
					}
				}
				// need to do this so that we don't get a command prompt from "system"
				if( bNulArgs )
				{
					TEXTCHAR *arg;
					int count = 0;
					arg = inbuf;
					while( ((PTRSZVAL)arg-(PTRSZVAL)buffer) < (PTRSZVAL)size )
					{
						count++;
						arg += StrLen( arg ) + 1;
					}
					args = (TEXTCHAR**)Allocate( (count+1) * sizeof( char * ) );

					arg = inbuf;
					count = 0;
					while( ((PTRSZVAL)arg-(PTRSZVAL)buffer) < (PTRSZVAL)size )
					{
						args[count++] = arg;
						arg += StrLen( arg ) + 1;
					}
					args[count++] = NULL; // null terminate list of args..

					if( restart && !bCaptureOutput )
					{
						// capture output sends back to a host program.
						// will NOT do this reconnect for restartables...
						// therefore it is not restartable.
						restart_info = New( RTASK );
						restart_info->fast_restart_count = 0;
						restart_info->prior_tick = GetTickCount();
						restart_info->args = (PCTEXTSTR)args;
						restart_info->program = StrDup( inbuf );
						restart_info->flags.bLogOutput = bLogOutput;
						restart_info->flags.bHide = hide_process;
						AddLink( &restarts, restart_info );
					}
					if( bCaptureOutput || bLogOutput)
					{
						if( bCaptureOutput )
						{
							struct thread_args *tmp_args = New( struct thread_args );
							if( sa )
							{
								sa = DuplicateAddress( sa );
								SetAddressPort( sa, capture_port );
							}
							tmp_args->inbuf = StrDup( inbuf );
							tmp_args->args = args;
							tmp_args->hide_process = hide_process;
							tmp_args->restart_info = restart_info;
							tmp_args->sa = sa;
							tmp_args->bCaptureOutput = bCaptureOutput;
							tmp_args->restart = restart;
                     tmp_args->pc_task_reply = pc_reply;
							lprintf( WIDE("Capturing task output to send back... begin back connect.") );
							ThreadTo( RemoteBackConnect, (PTRSZVAL)tmp_args );
						}
						else
						{
							PTASK_INFO task;
							task = LaunchPeerProgramExx( inbuf
								, start_path[0]?start_path:NULL
																, (PCTEXTSTR)args
																, hide_process?0:LPP_OPTION_DO_NOT_HIDE
																, GetTaskOutput
																, restart_info?RestartTask:TaskEnded
																, (PTRSZVAL)(restart_info?((PTRSZVAL)restart_info):0)
																 DBG_SRC
																);
						}
					}
					else
					{
						LaunchProgramEx( inbuf
							, start_path[0]?start_path:NULL
											, (PCTEXTSTR)args
											, restart_info?RestartTask:TaskEnded
											, (PTRSZVAL)restart_info );
						if( !restart )
						{
							// restartable tasks want to keep the args...
							Release( args );
						}
					}
				}
				else
				{
#ifdef __cplusplus
					::
#endif
						system( (char*)buffer ); // old way...
				}
			}
		}
		while(0);
#ifdef UNICODE
		Deallocate( TEXTCHAR *, inbuf );
#endif
	}
	else
	{
		lprintf( WIDE("Running [%s]"), inbuf );
#ifdef __cplusplus
		::
#endif
			system( (char*)buffer );
	}
}

static void CPROC UDPRead( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *sa )
{
	if( !buffer )
	{
		buffer = Allocate( 2048 );
	}
	else
	{
		ProcessPacket( NULL, buffer, size, sa );
	}
	// at this point, just fall out... while(0) might not continue; correctly...
	ReadUDP( pc, buffer, 2047 );
}

static void CPROC TCPReadComplete( PCLIENT pc, POINTER buffer, size_t len )
{
	struct task_output_network_state *pns = (struct task_output_network_state*)GetNetworkLong( pc, 0 );
	if( !buffer )
	{
		pns = New( struct task_output_network_state );
		MemSet( pns, 0, sizeof( struct task_output_network_state ) );
		SetNetworkLong( pc, 0, (PTRSZVAL)pns );
		pns->bufsize = 4096;
		pns->buffer = Allocate( 4096 );
	}
	else
	{
		ProcessPacket( pc, buffer, len, NULL );
	}
	ReadTCP( pc, pns->buffer, 4095 );
}

static void CPROC TCPCloseCallback( PCLIENT pc )
{
	struct task_output_network_state *pns = (struct task_output_network_state*)GetNetworkLong( pc, 0 );
	Deallocate( POINTER, pns->buffer );
}

static void CPROC ConnectionReceived( PCLIENT pcServer, PCLIENT pcNew )
{
	SetNetworkReadComplete( pcNew, TCPReadComplete );
	SetNetworkCloseCallback( pcNew, TCPCloseCallback );
}

int BeginNetwork( void )
{
	if( !NetworkWait( NULL, 48, 2 ) )
		return 0;

	pcListen = ServeUDP( pInterfaceAddr, 3006, UDPRead, NULL );
	if( !pcListen )
	{
		lprintf( WIDE("Failed to listen on 3006") );
		return 0;
	}
	UDPEnableBroadcast( pcListen, TRUE );
	lprintf( WIDE("listening on %s[:3006]"), pInterfaceAddr?pInterfaceAddr:WIDE("0.0.0.0:3006") );
	{
		SOCKADDR *host_tcp = CreateSockAddress( pInterfaceAddr, 3006 );
		pcListen = OpenTCPListenerAddrEx( host_tcp, ConnectionReceived );
		if( !pcListen )
		{
			lprintf( WIDE("Failed to listen on 3006") );
			return 0;
		}
		UDPEnableBroadcast( pcListen, TRUE );
		return 1;
	}
}

void  SetTaskLogOutput(void)
{
	bLogOutput = TRUE;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

static PTRSZVAL CPROC TaskComplete( PTRSZVAL psv, arg_list args )
{
   return psv;
}

static PTRSZVAL CPROC TaskBegin( PTRSZVAL psv, arg_list args )
{
   return psv;
}

static PTRSZVAL CPROC SetTaskProgramName( PTRSZVAL psv, arg_list args )
{
   return psv;

}

static PTRSZVAL CPROC SetTaskArguments( PTRSZVAL psv, arg_list args )
{

   return psv;
}

static PTRSZVAL CPROC SetTaskPath( PTRSZVAL psv, arg_list args )
{

   return psv;
}

static PTRSZVAL CPROC SetTaskRestart( PTRSZVAL psv, arg_list args )
{

   return psv;
}


void ProcessConfig( void )
{
	PCONFIG_HANDLER pch;
	pch = CreateConfigurationHandler();
	AddConfigurationMethod( pch, WIDE("Begin Task"), TaskBegin );
	AddConfigurationMethod( pch, WIDE("Task Done"), TaskComplete );
	AddConfigurationMethod( pch, WIDE("Program Name=%m"), SetTaskProgramName );
	AddConfigurationMethod( pch, WIDE("args=%m"), SetTaskArguments );
	AddConfigurationMethod( pch, WIDE("path=%m"), SetTaskPath );
	AddConfigurationMethod( pch, WIDE("restart=%b"), SetTaskRestart );
	ProcessConfigurationFile( pch, WIDE("launchpad.config"), 0 );
	DestroyConfigurationHandler( pch );
}

//--------------------------------------------------------------------------------

#if defined( BUILD_SERVICE )

static void CPROC Start( void )
{
	if( BeginNetwork() )
	{
		AddTimer( 2500, ExpireSequences, 0 );
	}
}

int main( int argc, char **argv )
{
	int ofs = 0;
	int did_arg = 0;
	{
		if( argc > (1) && StrCaseCmp( argv[1], WIDE("install") ) == 0 )
		{
			ServiceInstall( GetProgramName() );
			return 0;
		}
		if( argc > (1) && StrCaseCmp( argv[1], WIDE("uninstall") ) == 0 )
		{
			ServiceUninstall( GetProgramName() );
			return 0;
		}
		{
			static TEXTCHAR option[256];
			int arg_ofs;
			int arg_c;
			char **arg_v;
#ifndef __NO_OPTIONS__
			SACK_GetProfileString( GetProgramName(), WIDE("Command Line Arguments"), WIDE(""), option, sizeof( option ) );
#else
			option[0] = 0;
#endif
			ParseIntoArgs( option, &arg_c, &arg_v );
			for( arg_ofs = 0; arg_ofs < arg_c; arg_ofs++ )
			{
				if( arg_v[arg_ofs][0] == '-' )
				{
					switch( arg_v[arg_ofs][1] )
					{
					case 'c':
						if( arg_v[arg_ofs][2] )
							AddLink( &class_names, StrDup( arg_v[arg_ofs]+2 ) );
						else
						{
							arg_ofs++;
							lprintf( WIDE("Dup string %s"), arg_v[arg_ofs] );
							AddLink( &class_names, StrDup( arg_v[arg_ofs] ) );
						}
						break;
					case 'L':
						bLogPacketReceive = 1;
						if( !bLogOutput )
						{
							SetSystemLog( SYSLOG_AUTO_FILE, NULL );
							SetSystemLoggingLevel( 10000 );
							//SystemLogTime( SYSLOG_TIME_ENABLE|SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
							{
								FLAGSET( options, SYSLOG_OPT_MAX );
								int n;
								for( n = 0; n < SYSLOG_OPT_MAX; n++ )
								{
									RESETFLAG( options, n );
								}
								SETFLAG( options, SYSLOG_OPT_OPEN_BACKUP );
								SetSyslogOptions( options );
							}
						}
						break;
					case 'l':
						bLogOutput = 1;
						if( !bLogPacketReceive )
						{
							SetSystemLog( SYSLOG_AUTO_FILE, NULL );
							SetSystemLoggingLevel( 10000 );
							//SystemLogTime( SYSLOG_TIME_ENABLE|SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
							{
								FLAGSET( options, SYSLOG_OPT_MAX );
								int n;
								for( n = 0; n < SYSLOG_OPT_MAX; n++ )
								{
									RESETFLAG( options, n );
								}
								SETFLAG( options, SYSLOG_OPT_OPEN_BACKUP );
								SetSyslogOptions( options );
							}
						}
                  break;
					}
				}
			}
		}
	}
   // for some reason service registration requires a non-const string.  pretty sure it doesn't get modified....
	SetupService( (TEXTSTR)GetProgramName(), Start );
   return 0;
}

#elif defined( MILK_PLUGIN ) || defined( ISHELL_PLUGIN )


#  if defined( MILK_PLUGIN )
#    include <milk_export.h>
#    include <milk_registry.h>
#  endif

#  if defined( ISHELL_PLUGIN )
#    include "../../../InterShell/intershell_export.h"
#    include "../../../InterShell/intershell_registry.h"
#  endif

OnFinishInit( WIDE("Launchpad") )( void )
{
   lprintf( WIDE("Begin launchpad init.") );
	if( !BeginNetwork() )
	{
      xlprintf(LOG_ALWAYS)( WIDE("Fatal error starting network portion of LaunchPad") );
		return;
	}
	AddTimer( 2500, ExpireSequences, 0 );
}
#else
#  ifdef WIN32
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCmdShow )
{
	int argc;
	char **argv;
	ParseIntoArgs( GetCommandLine(), &argc, &argv );
#if 0 // debugging
	{
		int n;
		for( n = 0; n < argc; n++ )
			lprintf( WIDE("Arg %d=%s"), n, argv[n] );
	}
#endif
#    ifndef __NO_GUI__
	TerminateIcon();
	RegisterIcon( WIDE("PadIcon") );
#    endif
#  else
	int main( int argc, char **argv )
#  endif
	{
		{
			int arg_ofs;
			for( arg_ofs = 1; arg_ofs < argc; arg_ofs++ )
			{
				if( argv[arg_ofs][0] == '-' )
				{
					switch( argv[arg_ofs][1] )
					{
					case 'c':
						if( argv[arg_ofs][2] )
							AddLink( &class_names, StrDup( argv[arg_ofs]+2 ) );
						else
						{
							arg_ofs++;
							//lprintf( WIDE("Dup string %s"), argv[arg_ofs] );
							AddLink( &class_names, StrDup( argv[arg_ofs] ) );
						}
						break;
					case 'L':
						bLogPacketReceive = 1;
						if( !bLogOutput )
						{
							SetSystemLog( SYSLOG_AUTO_FILE, NULL );
							SetSystemLoggingLevel( 10000 );
							//SystemLogTime( SYSLOG_TIME_ENABLE|SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
							{
								FLAGSET( options, SYSLOG_OPT_MAX );
								int n;
								for( n = 0; n < SYSLOG_OPT_MAX; n++ )
								{
									RESETFLAG( options, n );
								}
								SETFLAG( options, SYSLOG_OPT_OPEN_BACKUP );
								SetSyslogOptions( options );
							}
						}
						break;
					case 'l':
						bLogOutput = 1;
						if( !bLogPacketReceive )
						{
							SetSystemLog( SYSLOG_AUTO_FILE, NULL );
							SetSystemLoggingLevel( 10000 );
							//SystemLogTime( SYSLOG_TIME_ENABLE|SYSLOG_TIME_LOG_DAY|SYSLOG_TIME_HIGH );
							{
								FLAGSET( options, SYSLOG_OPT_MAX );
								int n;
								for( n = 0; n < SYSLOG_OPT_MAX; n++ )
								{
									RESETFLAG( options, n );
								}
								SETFLAG( options, SYSLOG_OPT_OPEN_BACKUP );
								SetSyslogOptions( options );
							}
						}
						break;
					case 's':
						if( argv[arg_ofs][2] )
							pInterfaceAddr = StrDup( argv[arg_ofs]+2 );
						else
						{
							arg_ofs++;
							pInterfaceAddr = StrDup( argv[arg_ofs] );
						}
						break;
					}
				}
			}
		}
		lprintf( WIDE("Usage: %s\n [-c <class_name>...]  [-l]\n")
				  WIDE(" -l enables logging\n")
				  WIDE(" -L enables network packet receive logging\n")
				  WIDE(" -s socket listen address (resembles send on launchcmd)\n")
				  WIDE(" as many class names as you awnt may be added.\n")
				  WIDE("  If a class_name is specified commands for specified class will be executed.\n")
				  WIDE("  If NO class_name is specified all received commands will be executed.\n")
				  WIDE("  If no class is specified in the message, it is also launched."), argv[0] );
		if( !BeginNetwork() )
		{
			lprintf( WIDE("Fatal error starting network portion of LaunchPad") );
			return 0;
		}
#ifdef _DEBUG
		{
			INDEX idx;
			CTEXTSTR class_name;
			LIST_FORALL( class_names, idx, CTEXTSTR, class_name )
			{
				lprintf( WIDE("Listening for class %s"), class_name );
			}
		}
#endif
		AddTimer( 2500, ExpireSequences, 0 );
		while(1)
			WakeableSleep( SLEEP_FOREVER );

		return 0;
	}
#ifdef WIN32
}
#endif
#endif
