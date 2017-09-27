#include <stdhdrs.h>
#include <configscript.h>

struct address_minmax {
	TEXTSTR min;
	TEXTSTR max;
   TEXTSTR router;
};

struct local_ban_scanner
{
	PCONFIG_HANDLER pch_scanner;
	uint32_t exec_timer;
	TEXTSTR command;
	TEXTSTR output;
	TEXTSTR readlog;
	TEXTSTR lastban;
	int64_t used;
	PTREEROOT addressTree;
   int routers;
} lbs;



static void add_router( CTEXTSTR addr, CTEXTSTR router )
{
   struct address_minmax *router_mm;
	if( !lbs.addressTree )
		lbs.addressTree = CreateBinaryTreeEx( StrCmp, NULL );
	router_mm = FindInBinaryTree( lbs.addressTree, (uintptr_t)router );
	if( !router_mm ) {
		router_mm = New( struct address_minmax );
		router_mm->min = StrDup( addr );
		router_mm->max = StrDup( addr );
		router_mm->router = StrDup( router );
		AddBinaryNode( lbs.addressTree, router_mm, (uintptr_t)router_mm->router );
      //printf( "Added router...\n" );
		lbs.routers++;
	} else {
		if( StrCmp( router_mm->min, addr ) < 0 ) {
			//printf( "Updating router min... %s     %s     %s\n", router_mm->min, router_mm->max, addr );
			Deallocate( TEXTSTR, router_mm->min );
			router_mm->min = StrDup( addr );
		}
		if( StrCmp( router_mm->max, addr ) > 0 ) {
			//printf( "Updating router max... %s    %s     %s\n", router_mm->min, router_mm->max, addr );
			Deallocate( TEXTSTR, router_mm->max );
			router_mm->max = StrDup( addr );
		}
	}
}


static uintptr_t CPROC arp_in( uintptr_t psv, arg_list args ) {
   PARAM( args, CTEXTSTR, timstamp );
	PARAM( args, CTEXTSTR, specific );
	PARAM( args, CTEXTSTR, router );
	PARAM( args, int64_t, length );
   add_router( specific, router );
	lbs.used += length;

}

static uintptr_t CPROC Unhandled( uintptr_t psv, CTEXTSTR line )
{
	if( line )
		printf( "Unhandled: %s\n", line );
	return psv;
}

static void CPROC dumpStats( uintptr_t psv ) {
	printf( "routers: %d  bytes: %lld\n", lbs.routers, lbs.used );
}

static void CPROC dumpRoutes( uintptr_t psv ) {
	struct address_minmax *router_mm;
	for( router_mm = (struct address_minmax*)GetLeastNode( lbs.routers );
		 router_mm;
		  router_mm = (struct address_minmax*)GetGreaterNode( lbs.routers ) ) {
		printf( "Router: %16s     %16s     %16s\n", router_mm->router, router_mm->min, router_mm->max );
	}
}

static void InitArpScan( void )
{
	lbs.pch_scanner = CreateConfigurationHandler();
   //01:23:31.679127 ARP, Request who-has 70.187.123.172 tell 70.187.123.129, length 46
	AddConfigurationMethod( lbs.pch_scanner, "%m ARP, Request who-has %w tell %w, length %i", arp_in );

	SetConfigurationUnhandled( lbs.pch_scanner, Unhandled );
   AddTimer( 60000, dumpStats, 0 );
   AddTimer( 3600000, dumpRoutes, 0 );
}


static uint8_t buf[4096];
int main( void )
{
	InitArpScan();
	while( fgets( buf, 4096, stdin ) )
	{
		ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
	}
	return 0;
}


