#include <stdhdrs.h>
#include <configscript.h>

struct local_ban_scanner
{
	PCONFIG_HANDLER pch_scanner;
	uint32_t exec_timer;
	TEXTSTR command;
	TEXTSTR output;
	TEXTSTR readlog;
	TEXTSTR lastban;
} lbs;


void CPROC ExecFirewall( uintptr_t psv )
{
	system( lbs.command );
	lbs.exec_timer = 0;
}

static void AddBan( const char *IP )
{
	if( !lbs.lastban || StrCmp( IP, lbs.lastban ) ) {
		FILE *file = fopen( lbs.output, "ab" );
		printf( "add %s\n", IP, lbs.output );
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
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );

   return psv;
}

static uintptr_t CPROC failed_pass2( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );
   PARAM( args, int64_t, port );

   AddBan( ip_addr );

   return psv;
}

static uintptr_t CPROC failed_pass3( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, ip_addr );
   PARAM( args, int64_t, port );

   AddBan( ip_addr );

   return psv;
}


static uintptr_t CPROC failed_pass0( uintptr_t psv, arg_list args )
{
   PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );

   return psv;
}

static uintptr_t CPROC failed_user( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, leader );
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
	AddConfigurationMethod( lbs.pch_scanner, "%m Did not receive identification string from %w", failed_user_single );

	AddConfigurationMethod( lbs.pch_scanner, "%m Disconnected from %w port %w [preauth]", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m Did not receive identification string from %w port %i", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for invalid user %w from %w port %i ssh2", failed_pass2 );
	AddConfigurationMethod( lbs.pch_scanner, "%m Connection closed by %w port %w [preauth]", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m Unable to negotiate with %w port %w: no matching key exchange method found. Their offer: diffie-hellman-group1-sha1", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m Unable to negotiate with %w port %w: no matching key exchange method found. Their offer: diffie-hellman-group1-sha1 [preauth]", failed_pass3 );
	AddConfigurationMethod( lbs.pch_scanner, "%m Bad protocol version identification %m from  %w port %i", failed_pass );

	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for %w from %w port %i ssh2", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for %w from %w", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m Invalid user %w from %w", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m Invalid user %w from %w port %i", failed_pass );


	AddConfigurationMethod( lbs.pch_scanner, "%w - - [%m] \"GET /CherryWeb %m", failed_pass0 );
	AddConfigurationMethod( lbs.pch_scanner, "%w - - [%m] \"GET //a2billing/customer/templates/default/footer.tpl %m", failed_pass0 );
	AddConfigurationMethod( lbs.pch_scanner, "%w - - [%m] \"GET /assets/jnkp.php  %m", failed_pass0 );
	AddConfigurationMethod( lbs.pch_scanner, "%w - - [%m] \"GET /recordings%m", failed_pass0 );



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

static void ReadConfig( void )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	lbs.command = "/root/bin/iptables-setup";
	lbs.output = "/etc/firewall/banlist";
	//lbs.readlog = "/var/log/secure";

	//AddConfigurationMethod( pch, "readlog=%m", setReadPath );
	AddConfigurationMethod( pch, "command=%m", setFirewallCommand );
	AddConfigurationMethod( pch, "output=%m", setFirewallBanlist );
	ProcessConfigurationFile( pch, "linux_syslog_scanner.conf", 0 );
	DestroyConfigurationHandler( pch );

}

static uint8_t buf[4096];
int main( int argc, char **argv )
{
	int size;
	ReadConfig();
	InitBanScan();
	if( argc > 1 ) {
		FILE *in = fopen( argv[1], "rt" );
		if( in ) {
			while( fgets( buf, 4096, in ) )
			{
				ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
			}
			lprintf( "got null from file.." );
		}
	} else
		while( fgets( buf, 4096, stdin ) )
		{
		  lprintf( "Processing buffer:%s", buf );
			ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
			lprintf( "done with that buffer");
		}
			lprintf( "got null from file.." );
	return 0;
}


