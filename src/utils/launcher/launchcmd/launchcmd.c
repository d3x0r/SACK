#define VERSION "2.01"
#include <stdhdrs.h>
#include <system.h>
#include <sharemem.h>
#include <network.h>
#include <logging.h>

enum network_states   {
	NETWORK_STATE_GET_SIZE,
	NETWORK_STATE_GET_COMMAND,
};

struct network_read_state
{
	enum network_states state;
	size_t toread;
	size_t bufsize;
	POINTER buffer;
	PCLIENT *ppClient;
};

#define CLASSNAME "LaunchPad"
#define l local_launchpad_data
static struct local_launchpad_data_tag
{
	CTEXTSTR class_name; // only respect commands to this class... if NULL accept all
	PCLIENT pcCommand;
	PCLIENT pcTrack; // connection tracks remote...
	int track_port;
	SOCKADDR *send_to;
	PLIST client_connections;
	int clients;
	int closed_clients;
	_32 last_receive;
	int alive_probes;
	PTHREAD output_thread;
} l;

static int BeginNetwork( void )
{
	if( !NetworkStart() )
		return 0;

	return 1;
}

static int OpenSender( void )
{
	l.pcCommand = ConnectUDP( NULL, 3007, "255.255.255.255", 3006, NULL, NULL );
	if( !l.pcCommand )
		l.pcCommand = ConnectUDP( NULL, 3008, "255.255.255.255", 3006, NULL, NULL );
	if( !l.pcCommand )
		l.pcCommand = ConnectUDP( NULL, 3009, "255.255.255.255", 3006, NULL, NULL );
	if( !l.pcCommand )
		l.pcCommand = ConnectUDP( NULL, 3010, "255.255.255.255", 3006, NULL, NULL );
	if( !l.pcCommand )
		l.pcCommand = ConnectUDP( NULL, 3011, "255.255.255.255", 3006, NULL, NULL );
	if( !l.pcCommand )
		l.pcCommand = ConnectUDP( NULL, 3012, "255.255.255.255", 3006, NULL, NULL );
	if( !l.pcCommand )
	{
		srand( GetTickCount() );
		l.pcCommand = ConnectUDP( NULL, (_16)(3000 + rand() % 10000), "255.255.255.255", 3006, NULL, NULL );
	}


	if( !l.pcCommand )
	{
		printf( "Failed to bind to any port!\n" );
		return 0;
	}
	return 1;
}


static void CPROC Received( PCLIENT pc, POINTER buffer, size_t size )
{
	struct network_read_state *pns = (struct network_read_state *)GetNetworkLong( pc, 0 );
	if( !buffer )
	{
		pns = New( struct network_read_state );
		MemSet( pns, 0, sizeof( struct network_read_state ) );
		SetNetworkLong( pc, 0, (PTRSZVAL)pns );
		pns->bufsize = 4096;
		pns->buffer = Allocate( 4096 );
	}
	else
	{
		switch( pns->state )
		{
		case NETWORK_STATE_GET_SIZE:
			pns->toread = *(_32*)(buffer);
			if( pns->toread > pns->bufsize )
			{
				Release( pns->buffer );
				pns->buffer = Allocate( pns->toread );
				pns->bufsize = pns->toread;
			}
			pns->state = NETWORK_STATE_GET_COMMAND;
			break;
		case NETWORK_STATE_GET_COMMAND:
			{
				TEXTCHAR *buf = (TEXTCHAR*)buffer;
				l.alive_probes = 0;
				l.last_receive = GetTickCount();
				if( (buf[0] == '\0') && (size==1))
				{
				}
				else if( buf[0] == '@' )
				{
					buf[size] = 0;
#if defined( CONSOLE ) || !defined(WINDOWS_MODE)
					printf( "%s", buf+1 );
#else
					lprintf( "Received %s", buf+1 );
#endif

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

static void CPROC Closed( PCLIENT pc )
{
	struct network_read_state *pns = (struct network_read_state *)GetNetworkLong( pc, 0 );
	//lprintf( "Received close..." );
	DeleteLink( &l.client_connections, pc );
	l.closed_clients++;
	if( pns->ppClient )
		(*pns->ppClient) = NULL;

}

static PTRSZVAL CPROC SendConsoleInput( PTHREAD thread )
{
	TEXTCHAR cmd[4096];
	PCLIENT pc;
	INDEX idx;
	cmd[0] = '@'; // prefix with output to send
	while( fgets( cmd+1, sizeof( cmd )-1, stdin ) )
	{
		size_t len = strlen(cmd);
		lprintf( "Sending [%s]", cmd );
		LIST_FORALL( l.client_connections, idx, PCLIENT, pc )
		{
			SendTCP( pc, &len, 4 );
			SendTCP( pc, cmd, len );
		}
	}
	return 0;
}

static PTRSZVAL CPROC MonitorConnection( PTHREAD thread )
{
	PCLIENT pc;
	INDEX idx;
	while( 1 )
	{
		LIST_FORALL( l.client_connections, idx, PCLIENT, pc )
		{
			l.last_receive = GetTickCount();
			if( l.alive_probes++ < 3 )
			{
				//lprintf( "no output from remote task, are we alive?" );
				// send one byte length and one nul
				SendTCP( pc, "\1\0\0\0\0", 5 );
			}
			else
			{
				lprintf( "lost remote..." );
				RemoveClient( pc );
			}
		}
		WakeableSleep( 2000 );
	}

	return 0;
}

static void CPROC Connected( PCLIENT pc_server, PCLIENT pc_new )
{
	struct network_read_state *pns = New( struct network_read_state );
	MemSet( pns, 0, sizeof( struct network_read_state ) );
	//lprintf( "Recieved connect..." );
	l.clients++;
	SetNetworkLong( pc_new, 0, (PTRSZVAL)pns );
	SetNetworkReadComplete( pc_new, Received );
	SetNetworkCloseCallback( pc_new, Closed );
	AddLink( &l.client_connections, pc_new );
	if( !l.output_thread )
		l.output_thread = ThreadTo( SendConsoleInput, (PTRSZVAL)pc_new );
	ThreadTo( MonitorConnection, (PTRSZVAL)pc_new );
#ifdef CONSOLE
	//lprintf( "Sending back status 'connect ok'" );
	printf( "~CONNECT OK\n" );
	fflush( stdout );
#endif
}

#if defined(WINDOWS_MODE)
int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow )
{
	int argc;
	char **argv;
	ParseIntoArgs( GetCommandLine(), &argc, &argv );
#else
int main( int argc, char **argv )
#endif
{
	if( argc == 1 )
	{
#ifdef WIN32
		MessageBox( NULL,
#else
          printf(
#endif
			  "Usage: launchcmd [-r] [-l] [-r] [-h] [[-L] -s address] [-c class]   ....remote command....\n"
					  " -L : ...\n"
					  "         when used with -s connect to remote with a TCP connection, and get task\n"
					  "         relay local input to task and task output back to local console..\n"
					  " -l : listen to remote output, stays connected, and ... (relay input to all remotes that back connect)\n"
					  " -r : auto-restart when task ends on remote side.\n"
					  " -h : launch task hidden\n"
					  " -r : auto restart the task on the remote (when it dies, start it again)\n"
					  " -S : launch task non-user(system)\n"
					  " -s : specify target address to send to (instead of local broadcast)\n"
					  " -c : specify class of launchpads to respond to commands (matches -c arguments on launchpad)\n"
					  " -d : Set start in path on remote when launching task\n"
#ifdef WIN32
								, "Usage"
								, MB_OK );
#else
                  );
#endif
      return 0;
	}
	if( BeginNetwork() )
	{
		static TEXTCHAR lpCmd[4096];
		static TEXTCHAR lpStartPath[280];
		int ofs = 0;
		int n;
		int want_restart = 0;
		int want_service = 0;
		int want_hide = 0;
		int listen_output = 0;
		int listen_output_remote_connect = 0;
		int arg_ofs;
		_32 sequence = GetTickCount(); // this should be fairly unique...
#ifdef CONSOLE
		printf( "launchcmd version %s\n", VERSION );
		fflush( stdout );
#endif
		{
			int done = 0;
			for( arg_ofs = 1; !done && arg_ofs < argc; arg_ofs++ )
			{
				if( argv[arg_ofs][0] == '-' )
				{
					switch( argv[arg_ofs][1] )
					{
					case 'd':
						if( argv[arg_ofs][2] )
						{
							StrCpy( lpStartPath, argv[arg_ofs]+2 );
						}
						else
						{
							arg_ofs++;
							StrCpy( lpStartPath, argv[arg_ofs] );
						}
						break;
					case 'L':
					case 'l':
						listen_output = 1;
						if( argv[arg_ofs][1] == 'l' ) // don't open the listener port...
						{
							int n;
							listen_output_remote_connect = 1;
							for( n = 0; n < 32; n++ )
							{
								l.pcTrack = OpenTCPListenerEx( l.track_port = 3013+n, Connected );
								if( l.pcTrack )
									break;
							}
							if( !l.pcTrack )
							{
								// failed to open TCP listener... forget this.
								lprintf( "Failed to open receiving monitor socket to listen for remote back-connect" );
#if 0
#ifdef WIN32
								MessageBox( NULL,
#else
											  printf(
#endif
														"Failed to open receiving monitor socket..."
#ifdef WIN32
													  , "Abort Launch"
													  , MB_OK );
#else
											 );
#endif
#endif

								return 0;
							}
						}

						break;
					case 'r':
						want_restart = 1;
						break;
					case 'h':
						want_hide = 1;
						break;
					case 's':
						if( argv[arg_ofs][2] )
							l.send_to = CreateSockAddress( argv[arg_ofs]+2, 3006 );
						else
						{
							arg_ofs++;
							l.send_to = CreateSockAddress( argv[arg_ofs], 3006 );
						}
						break;
					case 'S':
						want_service = 1;
						break;
					case 'c':
						if( argv[arg_ofs][2] )
							l.class_name = StrDup( argv[arg_ofs]+2 );
						else
						{
							arg_ofs++;
							l.class_name = StrDup( argv[arg_ofs] );
						}
						break;
					case '-':
						done = 1;
						break;
					}
				}
				else
					break;
			}
		}
		ofs = 0;
		if( listen_output )
			ofs += snprintf( lpCmd + ofs, sizeof( lpCmd ) - ofs * sizeof(TEXTCHAR), "~capture%d", l.track_port );

		if( want_hide )
			ofs += snprintf( lpCmd + ofs, sizeof( lpCmd ) - ofs * sizeof(TEXTCHAR), "~hide" );

		if( lpStartPath[0] )
			ofs += snprintf( lpCmd + ofs, sizeof( lpCmd ) - ofs * sizeof(TEXTCHAR), "~path%s%%", lpStartPath );

		if( l.class_name )
			ofs += snprintf( lpCmd + ofs, sizeof( lpCmd ) - ofs * sizeof(TEXTCHAR), "$%s", l.class_name );
		ofs += snprintf( lpCmd + ofs, sizeof( lpCmd ) - ofs * sizeof(TEXTCHAR), "^%ld%c", sequence, want_restart?'@':'#' );
		for( n = arg_ofs; n < argc; n++ )
		{
			ofs += snprintf( lpCmd + ofs, sizeof( lpCmd ) - ofs * sizeof(TEXTCHAR)
								, "%s"
								, argv[n] );
         ofs++; // include '\0' in buffer so we can recover exact parameters...
		}
		//lprintf( "Sending [%s]", lpCmd );
		if( listen_output && l.send_to && !listen_output_remote_connect )
		{
			PCLIENT pc_launchpad = OpenTCPClientAddrEx( l.send_to, Received, Closed, NULL );
			if( pc_launchpad )
			{
				struct network_read_state *pns = (struct network_read_state * )GetNetworkLong( pc_launchpad, 0 );
				pns->ppClient = &pc_launchpad;
				l.clients++;
				
				AddLink( &l.client_connections, pc_launchpad );

				if( !l.output_thread )
					l.output_thread = ThreadTo( SendConsoleInput, (PTRSZVAL)pc_launchpad );
				ThreadTo( MonitorConnection, (PTRSZVAL)pc_launchpad );
				SendTCP( pc_launchpad, lpCmd, ofs );
				while( pc_launchpad )
				{
					WakeableSleep( 500 );
				}
			}
			else
				lprintf( "Failed to make TCP connection to launch command (bad send address?)" );
		}
		else if( l.send_to )
		{
			if( OpenSender() )
			{
				SendUDPEx( l.pcCommand, lpCmd, ofs, l.send_to );
				Sleep( 25 );
				SendUDPEx( l.pcCommand, lpCmd, ofs, l.send_to );
				Sleep( 25 );
				SendUDPEx( l.pcCommand, lpCmd, ofs, l.send_to );
			}
		}
		else
		{
			if( OpenSender() )
			{
				SendUDP( l.pcCommand, lpCmd, ofs );
				Sleep( 25 );
				SendUDP( l.pcCommand, lpCmd, ofs );
				Sleep( 25 );
				SendUDP( l.pcCommand, lpCmd, ofs );
			}
		}

		if( listen_output_remote_connect )
		{
			_32 tick_start = GetTickCount();
			while( !l.clients && ( ( GetTickCount() - tick_start ) < 6000 ) )
			{
				//lprintf( "waiting a second for reverse connect..." );
				WakeableSleep( 100 );
			}
			if( !l.clients )
			{
				lprintf( "No remote launchpads accepted this command.  Request to monitor task output failed." );
			}
			while( (l.clients-l.closed_clients) )
			{
				//lprintf( "waiting..." );
				WakeableSleep( 500 );
			}
		}
	}
	else
		printf( "Failed to start network\n" );
	if( l.pcCommand )
		RemoveClient( l.pcCommand );
	return 0;
}
#if defined(WINDOWS_MODE)
}
#endif
