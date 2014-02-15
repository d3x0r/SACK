#include <stdio.h>

#define DO_LOGGING
#include <stdhdrs.h>
#include <network.h>
#include <service_hook.h>
#define DEFAULT_SOCKETS 64
#define DEFAULT_TIMEOUT 5000

#define NL_PATH  0
#define NL_ROUTE 1
#define NL_OTHER 2   // PCLIENT other - the other client to send to...
#define NL_BUFFER  3 // pointer to buffer reading into....
#define NL_CONNECT_START 4 // _32 GetTickCount() when connect started.
#define NETWORK_WORDS 5

	typedef struct path_tag {
		PCLIENT in;
		PCLIENT out;
		struct route_tag *route;
		struct path_tag *next, **me;
	} PATH, *PPATH;

typedef struct route_tag {
	TEXTCHAR name[256];
	struct {
      // if set - send IP as first 4 bytes to new connection
		_32 ip_transmit : 1;
		//TODO: implement _32 hw_transmit, and wherever IP transmit is sent, makd sure MAC addr is sent as well.
		// if set - recieve first 4 bytes from connection, make
      // new connection to THAT IP, instead of the configured one.
		_32 ip_route : 1;
   } flags;
	SOCKADDR *in, *out;
	PCLIENT listen;
	PPATH paths;  // currently active connections..
	struct route_tag *next, **me;
} ROUTE, *PROUTE;

static struct local_proxy_tag
{
	struct proxy_flags{
		BIT_FIELD not_first_run : 1;
	} flags;
	PLIST pPendingList;

} local_proxy_data;
#define l local_proxy_data

PROUTE routes;
_32 dwTimeout = DEFAULT_TIMEOUT;
_16 wSockets; // default to 64 client/servers

//---------------------------------------------------------------------------

PCLIENT ConnectMate( PROUTE pRoute, PCLIENT pExisting, SOCKADDR *sa );

//---------------------------------------------------------------------------

void RemovePath( PPATH path )
{
	if( path )
	{
		xlprintf( 2100 )( WIDE("Removing path to %s %p"), path->route->name, path );
		GetMemStats( NULL, NULL, NULL, NULL );
		if( ( *path->me = path->next ) )
		{
		   xlprintf( 2100 )( WIDE("Have a next... %p"), path->me );
			GetMemStats( NULL, NULL, NULL, NULL );
			xlprintf( 2100 )( WIDE("Updating next me to my me %p"), &path->next->me );
			path->next->me = path->me;
			GetMemStats( NULL, NULL, NULL, NULL );
		}	
		GetMemStats( NULL, NULL, NULL, NULL );
		Release( path );
	}
	else
	{
		xlprintf( 2100 )( WIDE("No path defined to remove?!") );
	}
}

//---------------------------------------------------------------------------

void CPROC TCPClose( PCLIENT pc )
{
	PCLIENT other = (PCLIENT)GetNetworkLong( pc, NL_OTHER );
   xlprintf( 2100 )( WIDE("TCP Close") );
	Release( (POINTER)GetNetworkLong( pc, NL_BUFFER ) );
	if( other )
	{
		Release( (POINTER)GetNetworkLong( other, NL_BUFFER ) );
		RemoveClientEx( other, TRUE, FALSE );
	}
	RemovePath( (PPATH)GetNetworkLong( pc, NL_PATH ) );
	// hmm how to close mate...
}

//---------------------------------------------------------------------------

void CPROC TCPRead( PCLIENT pc, POINTER buffer, size_t size )
{
   //xlprintf( 2100 )( WIDE("TCP Read Enter...") );
	if( !buffer )
	{
		PROUTE route = (PROUTE)GetNetworkLong( pc, NL_ROUTE );
      PPATH path = (PPATH)GetNetworkLong( pc, NL_PATH );
		buffer = Allocate( 4096 );
		SetNetworkLong( pc, NL_BUFFER, (PTRSZVAL)buffer );
		if( route->flags.ip_route && !path )
		{
         xlprintf( 2100 )( WIDE("Was delayed connect, reading 4 bytes for address") );
			ReadTCPMsg( pc, buffer, 4 ); // MUST read 4 and only 4
         return;
		}
	}
	else
	{
		PCLIENT other;
		if( !( other = (PCLIENT)GetNetworkLong( pc, NL_OTHER ) ) )
		{
			// will NEVER have a OTHER if delayed connect
			// otherwise, we will not be here without an
			// other already associated, the other may fail later
			// but at this point it is known that there is an OTHER
			PROUTE route = (PROUTE)GetNetworkLong( pc, NL_ROUTE );
			// so - get the route of this one, see if we have to read
			// an IP...
			xlprintf( 2100 )( WIDE("Route address: %p"), route );
			if( !route )
			{
            xlprintf( 2100 )( WIDE("FATALITY! I die!") );
			}
         else if( route->flags.ip_route )
			{
				SOCKADDR saTo = *route->out;
				((_32*)&saTo)[1] = *(_32*)buffer;
				if( !ConnectMate( route, pc, &saTo ) )
				{
               xlprintf( 2100 )( WIDE("Connect mate failed... remove connection") );
					RemoveClient( pc );
					return;
				}
				else
               xlprintf( 2100 )( WIDE("Successful connection to remote (pending)") );
			}
		}
		else
		{
#ifdef _DEBUG
			//xlprintf( 2100 )( WIDE("Sending %d bytes to mate %p"), size, other );
#endif
			SendTCP( other, buffer, size );
		}
	}
	ReadTCP( pc, buffer, 4096 );
}

//---------------------------------------------------------------------------

PPATH MakeNewPath( PROUTE pRoute, PCLIENT in, PCLIENT out )
{
   xlprintf( 2100 )( WIDE("Adding path to %s"), pRoute->name );
   {
	PPATH path = (PPATH)Allocate( sizeof( PATH ) );
	path->me = &pRoute->paths;
	if( ( path->next = pRoute->paths ) )
		path->next->me = &path->next;
	pRoute->paths = path;
   path->route = pRoute;
	path->in = in;
	path->out = out;
   // link the sockets...
	SetNetworkLong( in, NL_OTHER, (PTRSZVAL)out );
	SetNetworkLong( out, NL_OTHER, (PTRSZVAL)in );
   // link the paths...
	SetNetworkLong( in, NL_PATH, (PTRSZVAL)path );
	SetNetworkLong( out, NL_PATH, (PTRSZVAL)path );
 	return path;
 	}
}

//---------------------------------------------------------------------------

void CPROC TCPConnected( PCLIENT pc, int error )
{
	// delay connect finished...
	xlprintf( 2100 )( WIDE("Connection finished...") );
	SetTCPNoDelay( pc, TRUE );
	SetClientKeepAlive( pc, TRUE );
	{
		// make sure we've set all data, and added it
		// it IS possible that this routine will be called
		// before the creator has finished initializing
		// this secondary thing...
		while( !GetNetworkLong( pc, NL_CONNECT_START ) )
			Relinquish();
		DeleteLink( &l.pPendingList, pc );
	}
	if( !error )
	{
      //xlprintf( 2100 )( WIDE("Proxied connect success!") );
		// should be okay here....
	}
	else
	{
		xlprintf( 2100 )( WIDE("Delayed connect failed... ") );
		RemoveClient( pc );
	}
}

//---------------------------------------------------------------------------

PCLIENT ConnectMate( PROUTE pRoute, PCLIENT pExisting, SOCKADDR *sa )
{
	PCLIENT out;
   xlprintf( 2100 )( WIDE("Connecting mating connection... ") );
	out = OpenTCPClientAddrExx( sa, TCPRead, TCPClose, NULL, TCPConnected );
	if( out )
	{
      _32 dwIP = (_32)GetNetworkLong( pExisting, GNL_IP );
		if( pRoute->flags.ip_transmit )
		{
			xlprintf( 2100 )( WIDE("Sending initiant's IP : %08") _32fX, dwIP );
			SendTCP( out, &dwIP, 4 );
		}
		SetNetworkLong( out, NL_ROUTE, (PTRSZVAL)pRoute );
		MakeNewPath( pRoute, pExisting, out );
		SetNetworkLong( out, NL_CONNECT_START, timeGetTime() );
		AddLink( &l.pPendingList, out );
	}
	else
	{
		// should only fail on like - no clients at all...
		xlprintf( 2100 )( WIDE("Failed to make proxy connection for: %s"), pRoute->name );
		RemoveClient( pExisting );
	}
   return out;
}

//---------------------------------------------------------------------------

void CPROC TCPConnect( PCLIENT pServer, PCLIENT pNew )
{
	PCLIENT out;
	PROUTE pRoute = (PROUTE)GetNetworkLong( pServer, NL_ROUTE );
   xlprintf( 2100 )( WIDE("TCP Connect received.") );
   SetTCPNoDelay( pNew, TRUE );
	SetClientKeepAlive( pNew, TRUE );
	if( pRoute->flags.ip_route )
	{
		// hmm this connection's outbound needs to be held off
		// until we get the desired destination....
      xlprintf( 2100 )( WIDE("Delayed connection to mate...") );
	}
	else
	{
		out = ConnectMate( pRoute, pNew, pRoute->out );
		if( !out )  // if not out - then pNew is already closed.
         return;
	}
	// have to set the route.
	SetNetworkLong( pNew, NL_ROUTE, (PTRSZVAL)pRoute );
	SetNetworkCloseCallback( pNew, TCPClose );
	SetNetworkReadComplete( pNew, TCPRead );
}

//---------------------------------------------------------------------------

void RemoveRoute( PROUTE route )
{
   xlprintf( 2100 )( WIDE("Remove route...") );
	if( ( *route->me = route->next ) )
		route->next->me = route->me;
	ReleaseAddress( route->in );
	ReleaseAddress( route->out );
	if( route->listen )
		RemoveClient( route->listen );
}

//---------------------------------------------------------------------------

void AddRoute( int set_ip_transmit
				 , int set_ip_route
				 , TEXTCHAR *route_name
				 , TEXTCHAR *src_name, int src_port
				 , TEXTCHAR *dest_name, int dest_port )
{
	PROUTE route = (PROUTE)Allocate( sizeof( ROUTE ) );
	xlprintf( 2100 )( WIDE("Adding Route %s: %s:%d %s:%d")
					, route_name
					, src_name?src_name:WIDE("0.0.0.0"), src_port
					, dest_name?dest_name:WIDE("0.0.0.0"), dest_port );
	if( route_name )
		StrCpyEx( route->name, route_name, sizeof( route->name ) );
	else
		StrCpyEx( route->name, WIDE("unnamed"), sizeof( route->name ) );
	route->flags.ip_transmit = set_ip_transmit;
	route->flags.ip_route = set_ip_route;
	route->in = CreateSockAddress( src_name, src_port );
	route->out = CreateSockAddress( dest_name, dest_port );
 	route->paths = NULL;
 	route->me = &routes;
 	if( ( route->next = routes ) )
 		route->next->me = &route->next;
	routes = route;
}

//---------------------------------------------------------------------------

void BeginRouting( void )
{
	PROUTE start = routes, next;
	while( start )
	{
		next = start->next;
		start->listen = OpenTCPListenerAddrEx( start->in, TCPConnect );
		if( start->listen )
			SetNetworkLong( start->listen, NL_ROUTE, (PTRSZVAL)start );
		else
		{
			xlprintf( 2100 )( WIDE("Failed to open listener for route: %s"), start->name );
			RemoveRoute( start );
		}
		start = next;
	}
}

//---------------------------------------------------------------------------

void ReadConfig( FILE *file )
{
	TEXTCHAR buffer[256];
	size_t len;
	while( fgets( buffer, 255, file ) )
	{
		int set_ip_transmit = 0;
      int set_ip_route = 0;
		TEXTCHAR *start, *end
		   , *name
		   , *addr1
		   , *addr2;
		int port1, port2;
		buffer[255] = 0;
		len = strlen( buffer );
		if( len == 255 )
		{
			xlprintf( 2100 )( WIDE("FATAL: configuration line is just toooo long.") );
			break;
		}
		buffer[len] = 0; len--; // terminate (remove '\n')

		end = strchr( buffer, '#' );
		if( end )
			*end = 0; // trim comments.

		start = buffer;
      // eat leading spaces...
		while( start[0] && (start[0] == ' ' || start[0] == '\t') )
			start++;
		end = start;
		// find the :
		while( end[0] && end[0] != ':' )
			end++;
		if( !end[0] )
		{
			//xlprintf( 2100 )( WIDE("No name field, assuming no route on line.") );
			continue;
		}
		end[0] = 0; // terminate ':'
      end++;
		// consume possible blanks after the ':'
	check_other:
		while( end[0] == ' ' || end[0] == '\t' )
		{
         end[0] = 0; // lots of nulls...
			end++;
		}

		if( !StrCaseCmpEx( end, WIDE("ip"), 2 ) )
		{
         set_ip_transmit = 1;
         end += 2;
		}
		while( end[0] == ' ' || end[0] == '\t' )
		{
         end[0] = 0;
			end++;
		}
		if( !StrCaseCmpEx( end, WIDE("switch"), 6 ) )
		{
			set_ip_route = 1;
			end += 6;
         goto check_other;
		}
      /*
      // comsume possible trailing spaces after option.
		while( end[1] == ' ' || end[1] == '\t' )
		{
			end[0] = 0;
			end++;
		}
		*/

		// fix existing bug until configuration files can be fixed.
		if( end[0] == 'a' )
		{
			end[0] = 0;
         end++;
		}


		if( !strcmp( start, WIDE("sockets") ) )
		{
			wSockets = atoi( end );
			if( !wSockets )
				wSockets = DEFAULT_SOCKETS;
			if( !NetworkWait( 0, wSockets, NETWORK_WORDS ) )
			{
				xlprintf( 2100 )( WIDE("Could not begin network!") );
			}
			xlprintf( 2100 )( WIDE("Setting socket limit to %d sockets"), wSockets );
		}
		else if( !strcmp( start, WIDE("timeout") ) )
		{
			dwTimeout = IntCreateFromText( end );
			if( !dwTimeout )
				dwTimeout = DEFAULT_TIMEOUT;
			xlprintf( 2100 )( WIDE("Setting socket connect timeout to %")_32f WIDE(" milliseconds"), dwTimeout );
		}
		else
		{
			name = start;
			addr1 = end;
			// find the next space after first address
			while( end[0] && end[0] != ' ' )
			{
				end++;
			}
			while( end[0] && end[0] == ' ' ) // kill spaces after first address
			{
				end[0] = 0;
            end++;
			}
			// now addr1 == second space delimted value...
         // if end is not a character - ran out of data...
			if( !end[0] )
			{
            // spaces followed name, but there was no name content...
				xlprintf( 2100 )( WIDE("Only found one address on the line... invalid route.") );
				continue;
			}

			addr2 = end;
			while( end[0] && end[0] != ' ' )
			{
				end++;
			}
			while( end[0] && end[0] == ' ' ) // kill spaces after second address
			{
				end[0] = 0;
            end++;
			}

			// if there was a tertiary field, then it could be read here
         // after validating that end[0] was data...

			if( ( start = strchr( addr1, ':' ) ) )
			{
				start[0] = 0;
				start++;
				port1 = atoi( start );
			}
			else
			{
				port1 = atoi( addr1 );
				addr1 = NULL;
			}
			
			if( ( start = strchr( addr2, ':' ) ) )
			{
				start[0] = 0;
				start++;
				port2 = atoi( start );
			}
			else
			{
				port2 = atoi( addr2 );
				addr2 = NULL;
			}
			AddRoute( set_ip_transmit, set_ip_route
					  , name
					  , addr1, port1
					  , addr2, port2 );
		}
	}
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC CheckPendingConnects( PTHREAD pUnused )
{
	PCLIENT pending;
	INDEX idx;
	while( 1 )
	{
		LIST_FORALL( l.pPendingList, idx, PCLIENT, pending )
		{
			PTRSZVAL dwStart = GetNetworkLong( pending, NL_CONNECT_START );

			//xlprintf( 2100 )( WIDE("Checking pending connect %ld vs %ld"),
			//     ( GetNetworkLong( pending, NL_CONNECT_START ) + dwTimeout ) , GetTickCount() );
			if( dwStart && ( ( dwStart + dwTimeout ) < timeGetTime() ) )
			{
				xlprintf( 2100 )( WIDE("Failed Connect... timeout") );
				RemoveClient( pending );
				xlprintf( 2100 )( WIDE("Done removing the pending client... "));
				SetLink( &l.pPendingList, idx, NULL ); // remove it from the list.
			}
		}
		WakeableSleep( 250 );
	}
   return 0;
}

//---------------------------------------------------------------------------

int task_done;
void CPROC MyTaskEnd( PTRSZVAL psv, PTASK_INFO task )
{
   task_done = 1;
}
void CPROC GetOutput( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, _32 length )
{
   xlprintf( 2100 )( WIDE("%s"), buffer );
}

//---------------------------------------------------------------------------
static TEXTCHAR *filename;

static void CPROC Start( void )
{
	FILE *file;
	// should clear all routes here, and reload them.
	file = sack_fopen( 0, filename, WIDE("rb") );
	xlprintf( 2100 )( WIDE("config would be [%s]"), filename );

	if( !l.flags.not_first_run )
	{
		NetworkStart();
		l.flags.not_first_run = 1;

		if( !file )
		{
			xlprintf( 2100 )( WIDE("Could not locate %s in current directory."), filename );
			return;
		}
		ReadConfig( file );
		fclose( file );

		if( !wSockets )
		{
			wSockets = DEFAULT_SOCKETS;
			if( !NetworkWait( 0, wSockets, NETWORK_WORDS ) )
			{
				xlprintf( 2100 )( WIDE("Could not begin network!") );
				return;
			}
		}

		xlprintf( 2100 )( WIDE("Pending timer is set to %") _32f, dwTimeout );
		ThreadTo( CheckPendingConnects, 0 );
		BeginRouting();
	}
	else
	{
		xlprintf( 2100 )( WIDE("Might want to re-read configuration and do something with it.") );
      fclose( file );
	}

}

//---------------------------------------------------------------------------

SaneWinMain( argc, argv )
{
   int normal = 1;
   int ofs = 0;
#ifdef BUILD_SERVICE
   normal = 0;
	if( argc > (1) && StrCaseCmp( argv[1], WIDE("install") ) == 0 )
	{
		ServiceInstall( GetProgramName() );
		return 0;
	}
	else if( argc > (1) && StrCaseCmp( argv[1], WIDE("uninstall") ) == 0 )
	{
		ServiceUninstall( GetProgramName() );
		return 0;
	}
	else if( argc > 1 && StrCaseCmp( argv[1], WIDE("normal") ) == 0 )
	{
		normal = 1;
      ofs = 1;
	}
	if( !normal )
	{
		{
			static TEXTCHAR tmp[256];
			snprintf( tmp, sizeof( tmp ), WIDE("%s/%s.conf"), GetProgramPath(), GetProgramName() );
			filename = tmp;
		}
		// for some reason service registration requires a non-const string.  pretty sure it doesn't get modified....
		SetupService( (TEXTSTR)GetProgramName(), Start );
	}
#endif
	if( normal )
	{
		if( argc < (2 + ofs ) )
		{
			static TEXTCHAR tmp[256];
			snprintf( tmp, sizeof( tmp ), WIDE("%s/%s.conf"), GetProgramPath(), GetProgramName() );
			filename = tmp;
		}
		else
			filename = argv[1 + ofs];

		Start();

		while( 1 )
			Sleep( 100000 );
	}

	return 0;
}
EndSaneWinMain()
