#ifdef __cplusplus
#include <stdio.h>

#define DO_LOGGING
#include <stdhdrs.h>

#include <sack_types.h>
#include <network.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <configscript.h>

#define DEFAULT_SOCKETS 64
#define DEFAULT_TIMEOUT 5000

#define NL_PATH  0
#define NL_ROUTE 1
#define NL_OTHER 2   // PCLIENT other - the other client to send to...
#define NL_BUFFER  3 // pointer to buffer reading into....
#define NL_CONNECT_START 4 // _32 GetTickCount() when connect started.
#define NETWORK_WORDS 5


struct route_tag *routes;
_32 dwTimeout = DEFAULT_TIMEOUT;
_16 wSockets; // default to 64 client/servers

typedef struct route_tag:NETWORK {
	route_tag( int set_ip_transmit
				 , int set_ip_route
				 , char *route_name
				 , char *src_name
				, char *dest_name )
	{
		if( !wSockets )
		{
			wSockets= DEFAULT_SOCKETS;
			Log( WIDE("Starting network with default sockets.") );
			NetworkWait( 0, wSockets, NETWORK_WORDS );
		}
		lprintf( WIDE("Adding Route %s: %s %s")
				 , route_name
				 , src_name
				 , dest_name );
		if( route_name )
			strcpy( name, route_name );
		else
			strcpy( name, WIDE("unnamed") );
		flags.ip_transmit = set_ip_transmit;
		flags.ip_route = set_ip_route;
		in = CreateSockAddress( src_name, 0 );
		out = CreateSockAddress( dest_name, 0 );
		paths = NULL;
		if( Listen( in ) )
		{
			LinkThing( routes, this );
			SetLong( NL_ROUTE, (_32)this );
		}
		else
		{
			Log1( WIDE("Failed to open listener for route: %s"), name );
			delete this;
		}
	}
	~route_tag() {
      UnlinkThing( this );
		ReleaseAddress( in );
		ReleaseAddress( out );
	}
	char name[256];
	struct {
      // if set - send IP as first 4 bytes to new connection
		_32 ip_transmit : 1;
		// if set - recieve first 4 bytes from connection, make
      // new connection to THAT IP, instead of the configured one.
		_32 ip_route : 1;
   } flags;
	SOCKADDR *in, *out;
	struct path *paths;  // currently active connections..
   DeclareLink( struct route_tag );
	void ConnectComplete( PNETWORK pNew );
} ROUTE, *PROUTE;

typedef struct path {
	PNETWORK in;
	PNETWORK out;
	struct route_tag *route;
	DeclareLink( struct path );

	path( struct route_tag * pRoute, PNETWORK incli, PNETWORK outcli )
	{
		Log1( WIDE("Adding path to %s"), pRoute->name );
		{
			LinkThing( pRoute->paths, this );
			route = pRoute;
			in = incli;
			out = outcli;
			// link the sockets...
			in->SetLong( NL_OTHER, (_32)out );
			out->SetLong( NL_OTHER, (_32)in );
			// link the paths...
			in->SetLong( NL_PATH, (_32)this );
			out->SetLong( NL_PATH, (_32)this );
		}
	}
	~path() {
		UnlinkThing( this );
	}
} PATH, *PPATH;

PLIST pPendingList;

//---------------------------------------------------------------------------

//PCLIENT ConnectMate( PROUTE pRoute, PCLIENT pExisting, SOCKADDR *sa );

//---------------------------------------------------------------------------

typedef struct Connection:public NETWORK {
	Connection( ) {}
	~Connection() {}
	void CloseCallback( void ) {
		PCLIENT other = (PCLIENT)GetLong( NL_OTHER );
		Log( WIDE("TCP Close") );
		Release( (POINTER)GetLong( NL_BUFFER ) );
		if( other )
		{
			Release( (POINTER)GetNetworkLong( other, NL_BUFFER ) );
			RemoveClientEx( other, TRUE, FALSE );
		}
		delete (PPATH)GetLong( NL_PATH );
	}
	void ReadComplete( POINTER buffer, int nSize ) {

		Log( WIDE("TCP Read Enter...") );
		if( !buffer )
		{
			PROUTE route = (PROUTE)GetLong( NL_ROUTE );
			PPATH path = (PPATH)GetLong( NL_PATH );
			buffer = Allocate( 4096 );
			SetLong( NL_BUFFER, (_32)buffer );
			if( route->flags.ip_route && !path )
			{
				Log( WIDE("Was delayed connect, reading 4 bytes for address") );
				ReadBlock( buffer, 4 ); // MUST read 4 and only 4
				return;
			}
		}
		else
		{
			PCLIENT other;
			if( !( other = (PCLIENT)GetLong( NL_OTHER ) ) )
			{
				// will NEVER have a OTHER if delayed connect
				// otherwise, we will not be here without an
				// other already associated, the other may fail later
				// but at this point it is known that there is an OTHER
				PROUTE route = (PROUTE)GetLong( NL_ROUTE );
				// so - get the route of this one, see if we have to read
				// an IP...
				Log1( WIDE("Route address: %p"), route );
				if( !route )
				{
					Log( WIDE("FATALITY! I die!") );
				}
				else if( route->flags.ip_route )
				{
					SOCKADDR saTo = *route->out;
					((_32*)&saTo)[1] = *(_32*)buffer;
					if( 1 /*!ConnectMate( route, pc, &saTo )*/ )
					{
						Log( WIDE("Connect mate failed... remove connection") );
                  delete this;
						return;
					}
					else
						Log( WIDE("Successful connection to remote (pending)") );
				}
			}
			else
			{
#ifdef _DEBUG
				Log2( WIDE("Sending %d bytes to mate %p"), nSize, other );
#endif
				SendTCP( other, buffer, nSize );
			}
		}
		Read( buffer, 4096 );
	}
	void ConnectComplete( int error )
	{
		// delay connect finished...
		Log( WIDE("Connection finished...") );
		{
         /*
			PCONNECTION pending;
			INDEX idx;
			// make sure we've set all data, and added it
			// it IS possible that this routine will be called
			// before the creator has finished initializing
			// this secondary thing...
			while( !GetLong( NL_CONNECT_START ) )
				Relinquish();
			LIST_FORALL( pPendingList, idx, PCONNECTION, pending )
			{
				if( pending == this )
				{
					Log1( WIDE("Delayed connect succeeded - removing. %p"), pc );
					SetLink( &pPendingList, idx, NULL );
					Log1( WIDE("Removed.... %d"), idx );
					break;
				}
				}
            */
		}
		if( !error )
		{
			Log( WIDE("Proxied connect success!") );
			// should be okay here....
		}
		else
		{
			Log( WIDE("Delayed connect failed... ") );
			delete this;
		}
	}
} CONNECTION, *PCONNECTION;

//---------------------------------------------------------------------------


//---------------------------------------------------------------------------


PCONNECTION ConnectMate( PROUTE pRoute, PCONNECTION pExisting, SOCKADDR *sa )
{
	PCONNECTION out = new Connection;
   Log( WIDE("Connecting mating connection... ") );
	out->Connect( sa );
	if( out )
	{
      _32 dwIP = pExisting->GetLong( GNL_IP );
		if( pRoute->flags.ip_transmit )
		{
			Log1( WIDE("Sending initiant's IP : %08lX"), dwIP );
			out->Send( &dwIP, 4 );
		}
		out->SetTCPNoDelay( TRUE );
		out->SetClientKeepAlive( TRUE );
		out->SetLong( NL_ROUTE, (_32)pRoute );
      new path( pRoute, pExisting, out );
		out->SetLong( NL_CONNECT_START, GetTickCount() );
		AddLink( &pPendingList, out );
	}
	else
	{
		// should only fail on like - no clients at all...
		Log1( WIDE("Failed to make proxy connection for: %s"), pRoute->name );
      delete pExisting;
	}
   return out;
}

//---------------------------------------------------------------------------

void CPROC ROUTE::ConnectComplete( PNETWORK &pNew )
{
	PROUTE pRoute = (PROUTE)GetLong( NL_ROUTE );
   PCONNECTION pNewConn = pNew;
   Log( WIDE("TCP Connect received.") );
   pNew->SetNoDelay( TRUE );
	pNew->SetClientKeepAlive( TRUE );
	if( pRoute->flags.ip_route )
	{
		// hmm this connection's outbound needs to be held off
		// until we get the desired destination....
      Log( WIDE("Delayed connection to mate...") );
	}
	else
	{
		out = ConnectMate( pRoute, pNewConn, pRoute->out );
		if( !out )  // if not out - then pNew is already closed.
         return;
	}
	// have to set the route.
	pNew->SetLong( NL_ROUTE, (_32)pRoute );

	//SetNetworkCloseCallback( pNew, TCPClose );
	//SetNetworkReadComplete( pNew, TCPRead );
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetSockets( PTRSZVAL psv, va_list args )
{
   PARAM( args, _64, sockets );
	printf( WIDE("sockets: %d\n"), sockets );
   NetworkWait( 0, sockets, NETWORK_WORDS );
  	wSockets = sockets;
  	if( !wSockets )
		wSockets = DEFAULT_SOCKETS;
   return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC SetTimeout( PTRSZVAL psv, va_list args )
{
   PARAM( args, _64, timeout );
	printf( WIDE("Timeout: %d\n"), timeout );
  	if( !timeout )
		dwTimeout = DEFAULT_TIMEOUT;
   else
		dwTimeout = timeout;
   return psv;
}

//---------------------------------------------------------------------------

PTRSZVAL CPROC RouteConfig( PTRSZVAL psv, va_list args )
{
   PARAM( args, char *,name );
   PARAM( args, char *,params );
	PTEXT text = SegCreateFromText( params );
	PTEXT _segs, segs = burst( text );
	PTEXT address[3];
	int nAddress = 0;
	int set_ip_route = 0
     , set_ip_transmit = 0
     , set_next_port = 0
     , add_next_address = 0
     , n;
	_segs = segs;
	for( n = 0; n < 3; n++ )
	{
		address[n] = NULL;
	}

	LineRelease( text );
	while( segs )
	{
		lprintf( WIDE("inspect segment: %s"), GetText( segs ) );
		if( TextLike( segs, WIDE("switch") ) )
		{
			set_ip_route = 1;
		}
		else if( TextLike( segs, WIDE("ip") ) )
		{
         set_ip_transmit = 1;
		}
		else if( TextLike( segs, WIDE("pool") ) )
		{
		}
		else if( TextIs( segs, WIDE(":") ) )
		{
         set_next_port = 1;
         add_next_address = 0;
         address[nAddress] = SegAppend( address[nAddress], SegDuplicate( segs ) );
		}
		else if( TextIs( segs, WIDE(".") ) )
		{
			set_next_port = 0;
         add_next_address = 1;
         address[nAddress] = SegAppend( address[nAddress], SegDuplicate( segs ) );
		}
      else
		{
			if( set_next_port )
			{
				address[nAddress] = SegAppend( address[nAddress]
													  , SegDuplicate( segs ) );
            nAddress++;
            set_next_port = 0;
			}
			else if( add_next_address )
			{
				address[nAddress] = SegAppend( address[nAddress]
													  , SegDuplicate( segs ) );
            add_next_address = 0;
			}
			else
			{
            if( address[nAddress] )
					nAddress++;
				address[nAddress] = SegAppend( address[nAddress]
													  , SegDuplicate( segs ) );
			}
		}
      segs = NEXTLINE( segs );
	}
	for( n = 0; n < nAddress; n++ )
	{
      address[n]->format.position.spaces = 0;
		PTEXT text = BuildLine( address[n] );
		LineRelease( address[n] );
		address[n] = text;
		lprintf( WIDE("Address: %s")
				 , address[n]?GetText(address[n]):"" );
	}

	if( nAddress < 2 )
	{
		Log( WIDE("Only found one address on the line... invalid route.") );
	}


	printf( WIDE("Uhmm new route: \")%s\" \"%s\"\n", name, params );
	new ROUTE( set_ip_transmit, set_ip_route
				, name
				, GetText( address[0] )
				, GetText( address[1] ) );
   LineRelease( address[0] );
   LineRelease( address[1] );
   return psv;
}

//---------------------------------------------------------------------------

class Config:public CONFIG_READER {
public:
	Config(char *file) {
		dwTimeout = DEFAULT_TIMEOUT;

		add( WIDE("sockets:%i"), SetSockets );
		add( WIDE("timeout:%i"), (USER_CONFIG_HANDLER)SetTimeout );
		add( WIDE("%m:%m"), (USER_CONFIG_HANDLER)RouteConfig );
		go( file, 0 );
	}
	~Config() { }
};

//---------------------------------------------------------------------------

_32 CPROC CheckPendingConnects( PTHREAD pUnused )
{
	PCLIENT pending;
	INDEX idx;
	while( 1 )
	{
		LIST_FORALL( pPendingList, idx, PCLIENT, pending )
		{
			_32 dwStart = GetNetworkLong( pending, NL_CONNECT_START );

			//Log2( WIDE("Checking pending connect %ld vs %ld"),
			//     ( GetNetworkLong( pending, NL_CONNECT_START ) + dwTimeout ) , GetTickCount() );
			if( dwStart && ( ( dwStart + dwTimeout ) < GetTickCount() ) )
			{
				Log( WIDE("Failed Connect... timeout") );
				RemoveClient( pending );
				Log( WIDE("Done removing the pending client... "));
				SetLink( &pPendingList, idx, NULL ); // remove it from the list.
			}
		}
		WakeableSleep( 250 );
	}
   return 0;
}

//---------------------------------------------------------------------------

int main( int argc, char **argv )
{

	FILE *file;
	char *filename;
	if( argc < 2 )
		filename = "proxy.conf";
	else
		filename = argv[1];

	file = fopen( filename, WIDE("rb") );
   if( file )
		fclose( file );

	SetSystemLog( SYSLOG_FILE, stderr );
   SystemLogTime( SYSLOG_TIME_HIGH|SYSLOG_TIME_DELTA );
	//SetAllocateLogging( TRUE );
	// true to disable...
	SetAllocateDebug( TRUE );

	if( !file )
	{
		Log1( WIDE("Could not locate %s in current directory."), filename );
		return -1;
	}
   delete new Config( filename );

   Log1( WIDE("Pending timer is set to %ld"), dwTimeout );
	ThreadTo( CheckPendingConnects, 0 );
   //AddTimer( 250, CheckPendingConnects, 0 );
	//BeginRouting();
   // do nothing - cause everything's event based.
	while( 1 )
		Sleep( 100000 );
	return 0;
}
#else
#include <stdio.h>
int main( void )
{
	return printf( WIDE("Compiled without a C++ compiler, program cannot function.") );
}
#endif

