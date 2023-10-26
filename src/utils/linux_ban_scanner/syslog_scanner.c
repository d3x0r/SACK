#include <stdhdrs.h>
#include <configscript.h>
#include <signal.h>
#include <pssql.h>

struct local_ban_scanner
{
	PCONFIG_HANDLER pch_scanner;
	uint32_t exec_timer;
	TEXTSTR command;
	TEXTSTR output;
	TEXTSTR readlog;
	TEXTSTR lastban;
	TEXTSTR DSN;
	PODBC db;
	uint64_t bad_pid[16];
	int next_bad_pid;
} lbs;


void CPROC ExecFirewall( uintptr_t psv )
{
	system( lbs.command );
	//System( lbs.command, NULL, 0 );
	lbs.exec_timer = 0;
}

static void AddBan( const char *IP )
{
	if( lbs.db ) {
		static char query[256];
		TEXTSTR* result = NULL;
		snprintf( query, 256, "select id from banlist where IP=`%s`", IP );
		if( SQLQuery( lbs.db, query, &result ) ) {
			if( result && result[0] ) {
				SQLEndQuery( lbs.db );
				printf( "already banned %s\n", IP );
				SQLCommandf( lbd.db, "update banlist set last_hit=now() where id=%s", result[0] );
				return; // no need to include this one.
			} else {
				SQLCommandf( lbd.db, "insert into banlist (IP) values(`%s`)", IP );
			}
		}
	}
	if( !lbs.lastban || StrCmp( IP, lbs.lastban ) ) {
		FILE *file = fopen( lbs.output, "ab" );
		printf( "add %s\n", IP );
		if( file ) {
			fprintf( file, "%s\n", IP );
			fclose( file );
			if( !lbs.exec_timer )
				lbs.exec_timer = AddTimerEx( 1000, 0, ExecFirewall, 0 );
			else
				RescheduleTimerEx( lbs.exec_timer, 1000 );
		}
		if( lbs.lastban )
			Release( lbs.lastban );
		lbs.lastban = StrDup( IP );
	}
}

static uintptr_t CPROC failed_pass( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, int64_t, pid );
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );

   return psv;
}

static uintptr_t CPROC failed_pass2( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, int64_t, pid );
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );
   PARAM( args, int64_t, port );

   AddBan( ip_addr );

   return psv;
}

static uintptr_t CPROC failed_pass3( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, int64_t, pid );
   PARAM( args, CTEXTSTR, ip_addr );
   PARAM( args, int64_t, port );
   //lprintf( "got good? %s", ip_addr );
   AddBan( ip_addr );

   return psv;
}


static uintptr_t CPROC failed_pass0( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );

   return psv;
}

static uintptr_t CPROC failed_key( uintptr_t psv, arg_list args ){
	PARAM( args, CTEXTSTR, leader );
	PARAM( args, int64_t, pid );
	lbs.bad_pid[lbs.next_bad_pid++] = pid;
	if( lbs.next_bad_pid >= (sizeof( lbs.bad_pid )/sizeof( lbs.bad_pid[0])) )
		lbs.next_bad_pid = 0;
}

static uintptr_t CPROC failed_key_close( uintptr_t psv, arg_list args ){
	PARAM( args, CTEXTSTR, leader );
	PARAM( args, int64_t, pid );
	PARAM( args, CTEXTSTR, ip_addr );
	PARAM( args, int64_t, port );
	int n;
	for( n = 0; n < (sizeof( lbs.bad_pid )/sizeof( lbs.bad_pid[0])); n++ ) {
		if( pid == lbs.bad_pid[n] ) break;
	}
	if( n < (sizeof( lbs.bad_pid )/sizeof( lbs.bad_pid[0])) ) {
		AddBan( ip_addr );
	}
}

static uintptr_t CPROC failed_user( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, leader );
	PARAM( args, int64_t, pid );
	PARAM( args, CTEXTSTR, leader2 );
	PARAM( args, CTEXTSTR, user );
	PARAM( args, CTEXTSTR, ip_addr );

	AddBan( ip_addr );
	return psv;
}

static uintptr_t CPROC failed_user_single( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, leader );
	PARAM( args, CTEXTSTR, ip_addr );

	AddBan( ip_addr );
	return psv;
}

static uintptr_t CPROC Unhandled( uintptr_t psv, CTEXTSTR line )
{
	if( line )
		printf( "Unhandled: %s\n", line );
	return psv;
}

static void InitBanScan( void )
{
	lbs.pch_scanner = CreateConfigurationHandler();
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Did not receive identification string from %w", failed_user_single );

	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Disconnected from %w port %w [preauth]", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Did not receive identification string from %w port %i", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Failed password for invalid user %w from %w port %i ssh2", failed_pass2 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Connection closed by %w port %w [preauth]", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Unable to negotiate with %w port %w: no matching key exchange method found. Their offer: diffie-hellman-group1-sha1", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Unable to negotiate with %w port %w: no matching key exchange method found. Their offer: diffie-hellman-group1-sha1 [preauth]", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Bad protocol version identification %m from  %w port %i", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: fatal: Timeout before authentication for %w port %i", failed_pass3 );

	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Failed password for %w from %w port %i ssh2", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Failed password for %w from %w", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Invalid user %w from %w", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Invalid user %w from %w port %i", failed_pass );

//Sep 25 20:14:29 tower sshd[2659362]: error: kex_exchange_identification: Connection closed by remote host
//Sep 25 20:14:29 tower sshd[2659362]: Connection closed by 156.59.134.94 port 53812
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: error: kex_exchange_identification: Connection closed by remote host", failed_key );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd[%i]: Connection closed by %w port %i", failed_key_close );

//sshd[2662582]: banner exchange: Connection from 107.170.230.26 port 42576: invalid format


	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Received disconnect from %i", failed_pass2 );
	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Invalid user %w from %i", failed_user );
	SetConfigurationUnhandled( lbs.pch_scanner, Unhandled );
}

static uintptr_t CPROC setFirewallCommand( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	lbs.command = StrDup( path );
	return psv;
}

static uintptr_t CPROC setFirewallBanlist( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	lbs.output = StrDup( path );
	return psv;
}

static uintptr_t CPROC setFirewallDb( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	lbs.DSN = StrDup( path );
	return psv;
}

static void ReadConfig( void )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	lbs.command = "/root/bin/iptables-setup";
	lbs.output = "/etc/firewall/banlist";
	lbs.DSN = "maria-firewall";
	//lbs.readlog = "/var/log/secure";

	//AddConfigurationMethod( pch, "readlog=%m", setReadPath );
	AddConfigurationMethod( pch, "command=%m", setFirewallCommand );
	AddConfigurationMethod( pch, "output=%m", setFirewallBanlist );
	AddConfigurationMethod( pch, "DSN=%m", setFirewallDb );
	ProcessConfigurationFile( pch, "linux_syslog_scanner.conf", 0 );
	DestroyConfigurationHandler( pch );

}

static void OpenDb() {
	lbs.db = ConnectToDatabase( l.DSN );
	if( lbs.db ) {
		PTABLE table = GetFieldsInSQL( "create table banlist (id int AUTO INCREMENT, IP char(32), created DATETIME DEFAULT CURRENT_TIMESTAMP, last_hit DATETIME  DEFAULT CURRENT_TIMESTAMP)", FALSE );
		CheckODBCTable( lbs.db, table, CTO_MERGE );
		DestroySQLTable( table );
	}
}

static uint8_t buf[4096];
int main( int argc, char **argv )
{
	int size;
	int arg = 1;
	int lastpos = 0;
	int follow = 0;
	ReadConfig();
	OpenDb();
	InitBanScan();
	if( argc > 1 )
		while( ( arg < argc ) && argv[arg][0] == '-' ) {
			if( argv[arg][1] == 'w' ) 
				usleep( 10 * 1000 * 1000 );
			if( argv[arg][1] == 'f' )
				follow = 1;
			arg++;
		}
	if( arg < argc )
		for( ; follow; WakeableSleep( 5000 ) ) {
			FILE *in = fopen( argv[arg], "rt" );
			if( in ) {
				fseek( in, lastpos, SEEK_SET );
				while( fgets( buf, 4096, in ) )
				{
					lastpos = ftell( in );
					ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
				}
				fclose( in );
			}
		}
	else
		while( fgets( buf, 4096, stdin ) )
		{
			//lprintf( "Processing buffer:%s", buf );
			ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
			//lprintf( "done with that buffer");
		}
	//lprintf( "got null from file.." );
	return 0;
}


