#include <stdio.h>

#define DO_LOGGING
#include <stdhdrs.h>

#include <sack_types.h>
#include <network.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>

#define DEFAULT_SOCKETS 64
#define DEFAULT_TIMEOUT 5000

#define NL_ROUTE 0
#define NL_OTHER 1   // PCLIENT other - the other client to send to...
#define NL_BUFFER  2 // pointer to buffer reading into....

typedef struct route_output_tag {
	char name[256];
   SOCKADDR *socket_addr;
	PCLIENT socket;
   PLIST sendto;
} ROUTE_OUTPUT, *PROUTE_OUTPUT;

typedef struct route_tag {
	char name[256];
	SOCKADDR *listen_addr;//, *sendto;
	SOCKADDR *bind_addr;//, *sendto;
   //PLIST out_address;
	PCLIENT listen;
   PLIST outputs; // PROUTE_OUTPUT pointers...
	//struct route_tag *next, **me;
} ROUTE, *PROUTE;

PLIST outputs;
PLIST inputs;

PROUTE routes;
_16 wSockets = DEFAULT_SOCKETS; // default to 64 client/servers

static struct global_data{
   _32 packet_count;
}g;

//---------------------------------------------------------------------------

void CPROC UDPClose( PCLIENT pc )
{
	PCLIENT other = (PCLIENT)GetNetworkLong( pc, NL_OTHER );
	Release( (POINTER)GetNetworkLong( pc, NL_BUFFER ) );
	if( other )
	{
		Release( (POINTER)GetNetworkLong( other, NL_BUFFER ) );
		RemoveClientEx( other, TRUE, FALSE );
	}
	// hmm how to close mate...
}

//---------------------------------------------------------------------------

 void BuildAddr( char *addr, SOCKADDR *sa )
{
	sprintf( addr, WIDE("%d.%d.%d.%d:%d"),
    				*(((unsigned char *)sa)+4),
    				*(((unsigned char *)sa)+5),
    				*(((unsigned char *)sa)+6),
    				*(((unsigned char *)sa)+7),
			  ntohs(*(((unsigned short *)sa)+1))
			 );
}

int AddrCmp( SOCKADDR *sa1, SOCKADDR *sa2 )
{
	if( ((unsigned long*)sa1)[0] == ((unsigned long*)sa2)[0] &&
		((unsigned long*)sa1)[1] == ((unsigned long*)sa2)[1] )
      return TRUE;
	if( ((unsigned long*)sa1)[0] == ((unsigned long*)sa2)[0] &&
		(!((unsigned long*)sa1)[1] ||
		 !((unsigned long*)sa2)[1]) )
      return TRUE;
   return FALSE;
}

void CPROC UDPRead( PCLIENT pc, POINTER buffer, size_t size, SOCKADDR *saFrom )
{
   int sent = 0;
	if( !buffer )
	{
      Log( WIDE("Allocating buffer to read into...") );
		buffer = Allocate( 4096 );
		SetNetworkLong( pc, NL_BUFFER, (PTRSZVAL)buffer );
	}
	else
	{
		PCLIENT other;
		//char addr[32], addr2[32];
		PROUTE route = (PROUTE)GetNetworkLong( pc, NL_ROUTE );
		INDEX idx;
		PROUTE_OUTPUT output;
      PROUTE input;
		{
			LIST_FORALL( inputs, idx, PROUTE, input )
			{
            /*
				BuildAddr( addr, saFrom );
            BuildAddr( addr2, input->listen_addr );
				lprintf( "Packet...%d %s %s", idx, addr, addr2 );
            */
				//if( AddrCmp( saFrom, input->listen_addr ) )
				{
               INDEX idx_out;
					//lprintf( "This was sourced from me, drop the packet." );
					LIST_FORALL( outputs, idx_out, PROUTE_OUTPUT, output )
					{
						 //BuildAddr( addr, saFrom );
						 //BuildAddr( addr2, output->socket_addr );
						 //lprintf( "Packet...%d %s %s", idx, addr, addr2 );

						if( AddrCmp( saFrom, output->socket_addr ) )
						{
							//lprintf( "This was sourced from me, drop the packet." );
							break;
						}
					}
					if( !output )
					{
                  //lprintf( "no output found." );
						LIST_FORALL( route->outputs, idx_out, PROUTE_OUTPUT, output )
						{
							//BuildAddr( addr, saFrom );
							//BuildAddr( addr2, route->sendto );
							//Log3( WIDE("Received (%d bytes) from %s to %s"), size, addr, addr2 );
							while( !( other = (PCLIENT)GetNetworkLong( pc, NL_OTHER ) ) )
								Relinquish(); // not quite set yet... SOON.
							{
								INDEX idx_sendto;
								SOCKADDR *sendto;
								LIST_FORALL( output->sendto, idx_sendto, SOCKADDR*, sendto )
								{
									//BuildAddr( addr2, sendto );
									//Log3( WIDE("Received (%d bytes) from %s to %s"), size, addr, addr2 );
                           sent=1;
									SendUDPEx( output->socket
												, buffer
												, (int)size
												, sendto );
								}
							}
						}
					}
				}
			}
		}
	}
	if( sent )
      g.packet_count++;
	ReadUDP( pc, buffer, 4096 );
}

//---------------------------------------------------------------------------

void RemoveRoute( PROUTE route )
{
	Log1( WIDE("Removeing route: %s"), route->name );
	ReleaseAddress( route->listen_addr );
	//ReleaseAddress( route->out );
   //ReleaseAddress( route->sendto );
	if( route->listen )
		RemoveClient( route->listen );
   SetLink( &inputs, FindLink( &inputs, route ), 0 );
}

//---------------------------------------------------------------------------

void RemoveOutputRoute( PROUTE input, PROUTE_OUTPUT route )
{
	Log1( WIDE("Removeing route: %s"), route->name );
	ReleaseAddress( route->socket_addr );
	//ReleaseAddress( route->out );
   //ReleaseAddress( route->sendto );
	if( route->socket )
		RemoveClient( route->socket );
	{
		SOCKADDR *sa;
		INDEX idx;
		LIST_FORALL( route->sendto, idx, SOCKADDR*, sa )
			ReleaseAddress( sa );
	}
   SetLink( &input->outputs, FindLink( &input->outputs, route ), 0 );
   SetLink( &outputs, FindLink( &outputs, route ), 0 );
}

//---------------------------------------------------------------------------

PROUTE_OUTPUT CreateOutput( char *name, SOCKADDR *addr, SOCKADDR *sendto )
{
	PROUTE_OUTPUT route;
	INDEX idx;
	LIST_FORALL( outputs, idx, PROUTE_OUTPUT, route )
	{
		if( CompareAddress( addr, route->socket_addr ) )
         break;
	}
	if( !route )
	{
		route = (PROUTE_OUTPUT)Allocate( sizeof( *route ) );
		MemSet( route, 0, sizeof( *route ) );
		snprintf( route->name, sizeof( route->name ), "%s", name?name:"unnamed" );
      /*
		{
			TEXTCHAR saFrom[32];
			BuildAddr( saFrom, addr );
			lprintf( "Output socket = %s", saFrom );
			}
         */
		route->socket_addr = addr;
      AddLink( &outputs, route );
	}
	else
		ReleaseAddress( addr ); // didn't use this... already have it.
   AddLink( &route->sendto, sendto );
   return route;
}

//---------------------------------------------------------------------------

PROUTE CreateInput( char *name, SOCKADDR *addr )
{
	PROUTE route;
	INDEX idx;
   SOCKADDR *bind_addr;
	{
		_16 port;
		GetAddressParts( addr, NULL, &port );
		bind_addr = CreateSockAddress( "0.0.0.0", port );
	}
	LIST_FORALL( inputs, idx, PROUTE, route )
	{
		if( CompareAddress( bind_addr, route->bind_addr ) )
         break;
	}
	if( !route )
	{
		route = (PROUTE)Allocate( sizeof( *route ) );
		MemSet( route, 0, sizeof( *route ) );
		snprintf( route->name, sizeof( route->name ), "%s", name?name:"unnamed" );
		route->listen_addr = addr;
		{
			_16 port;
         GetAddressParts( addr, NULL, &port );
			route->bind_addr = bind_addr;
		}
      AddLink( &inputs, route );
	}
	else
      ReleaseAddress( addr ); // didn't use this... already have it.
   return route;
}

//---------------------------------------------------------------------------

void AddRoute( char *route_name
					, char *src_name, int src_port
				 , char *dest_name, int dest_port // send from socket.
				 , char *ipto, int to_port )
{
   SOCKADDR *in = CreateSockAddress( src_name, src_port );
	PROUTE route = CreateInput( route_name, in );// = (PROUTE)Allocate( sizeof( ROUTE ) );
   //SOCKADDR *out = CreateSockAddress( dest_name, dest_port );
	PROUTE_OUTPUT output;
   /*
	{
		TEXTCHAR blah[32];
      BuildAddr( blah, out );
		lprintf( "output bind addr %s:%d= %s", dest_name, dest_port, blah );
		}
      */
	output = CreateOutput( route_name
												  , CreateSockAddress( dest_name, dest_port ) // send from socket.
												  , CreateSockAddress( ipto, to_port ) );
	AddLink( &route->outputs, output );

	lprintf( WIDE("Adding Route %s: %s:%d %s:%d sendto %s:%d")
			 , route_name
			 , src_name?src_name:"0.0.0.0", src_port
			 , dest_name?dest_name:"0.0.0.0", dest_port
			 , ipto?ipto:"BAD ADDRESS", to_port);

  // route->in = CreateSockAddress( src_name, src_port );

	//route->out = CreateSockAddress( dest_name, dest_port );
   //route->sendto = CreateSockAddress( ipto, to_port );
 	//route->me = &routes;
 	//if( ( route->next = routes ) )
 	//	route->next->me = &route->next;
	//routes = route;
}

//---------------------------------------------------------------------------

void BeginRouting( void )
{
   INDEX idx_route;
	PROUTE start;
	LIST_FORALL( inputs, idx_route, PROUTE, start )
	{
		// mark next - since THIS may dissappear with fatal error.
		start->listen = ServeUDPAddr( start->bind_addr, UDPRead, UDPClose );
		if( start->listen )
		{
			SetNetworkLong( start->listen, NL_ROUTE, (PTRSZVAL)start );
			{
            INDEX idx;
				PROUTE_OUTPUT other;
				LIST_FORALL( start->outputs, idx, PROUTE_OUTPUT, other )
				{
					if( !other->socket )
					{
                  /*
						{
							char szAddr[54];
							BuildAddr( szAddr, other->socket_addr );
                     lprintf( "serving at %s", szAddr );
							}
                     */
						other->socket = ServeUDPAddr( other->socket_addr, UDPRead, UDPClose );
						if( other->socket )
						{
							UDPEnableBroadcast( other->socket, TRUE );
							SetNetworkLong( other->socket, NL_ROUTE, (PTRSZVAL)start );
							SetNetworkLong( other->socket, NL_OTHER, (PTRSZVAL)start->listen );
							SetNetworkLong( start->listen, NL_OTHER, (PTRSZVAL)other->socket );
							Log( WIDE("Successfully mated listener with retransmitter, and vice versa") );
						}
						else
						{
							lprintf( WIDE("Failed to open retrans side of %s"), other->name );
							RemoveOutputRoute( start, other );
						}
					}
				}
			}
		}
		else
		{
			lprintf( WIDE("Failed to open listener for route %s."), start->name );
			RemoveRoute( start );
		}
	}
}

//---------------------------------------------------------------------------


void ReadConfig( FILE *file )
{
	char buffer[256];
	size_t len;
	while( fgets( buffer, 255, file ) )
	{
		char *start, *end
		   , *name
		   , *addr1
         , *addr2
         , *addr3;
		//int port1, port2, port3;
		buffer[255] = 0;
		len = strlen( buffer );
		if( len == 255 )
		{
			Log( WIDE("FATAL: configuration line is just toooo long.") );
			break;
		}
		buffer[len] = 0; len--; // terminate (remove '\n')

		end = strchr( buffer, '#' );
		if( end )
			*end = 0; // trim comments.

		start = buffer;
		while( start[0] && start[0] == ' ' )
			start++;
		end = start;
		while( end[0] && end[0] != ':' )
			end++;
		if( !end[0] )
		{
			//Log( WIDE("No name field, assuming no route on line.") );
			continue;
		}
		end[0] = 0;

		if( !strcmp( start, WIDE("sockets") ) )
		{
			wSockets = atoi( end+1 );
			if( !wSockets )
				wSockets = DEFAULT_SOCKETS;
			Log1( WIDE("Setting socket limit to %d sockets"), wSockets );
		}
      else
		{
			name = start;
			start = end+1;
			while( start[0] && start[0] == ' ' )
				start++;
			end = start;
			while( end[0] && end[0] != ' ' )
				end++;
			if( !end[0] )
			{
				Log( WIDE("Only found one address on the line... invalid route.") );
				continue;
			}
			end[0] = 0;

			addr1 = start;
			start = end+1;
			while( start[0] && start[0] == ' ' )
				start++;
			end = start;
			while( end[0] && end[0] != ' ' )
				end++;
			if( end[0] )
				end[0] = 0;
			addr2 = start;

			start = end+1;
			while( start[0] && start[0] == ' ' )
				start++;
			end = start;
			while( end[0] && end[0] != ' ' )
				end++;
			if( end[0] )
				end[0] = 0;
			addr3 = start;

			lprintf( "3 things : [%s][%s][%s]", addr1, addr2, addr3 );
			AddRoute( name, addr1, 25001, addr2, 25004, addr3, 25001 );
		}
	}
}

//---------------------------------------------------------------------------

int main( int argc, char **argv )
{

	FILE *file;
	char *filename;
	if( argc < 2 )
		filename = "forward.conf";
	else
		filename = argv[1];

   file = fopen( filename, WIDE("rb") );

	SetSystemLog( SYSLOG_FILE, stdout );
   SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
	//SetAllocateLogging( TRUE );

	if( !file )
	{
		Log1( WIDE("Could not locate %s in current directory."), filename );
		return -1;
	}
	ReadConfig( file );
	fclose( file );

	if( !NetworkWait( 0, wSockets, 5 ) )
	{
		Log( WIDE("Could not begin network!") );
		return -1;
	}

	BeginRouting();
   Log( WIDE("Ready!") );
	while( 1 )
		Sleep( 100000 );
	return 0;
}

// $Log: forward.c,v $
// Revision 1.10  2003/07/24 22:50:10  panther
// Updates to make watcom happier
//
// Revision 1.9  2003/06/04 11:39:31  panther
// Stripped carriage returns
//
// Revision 1.8  2003/04/28 00:43:18  panther
// Remove double ;; which causes out of band declarations things
//
// Revision 1.7  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
