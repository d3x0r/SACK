#include <stdhdrs.h>
#include <configscript.h>

struct local_ban_scanner
{
	PCONFIG_HANDLER pch_scanner;
	_32 exec_timer;
	TEXTSTR command;
	TEXTSTR output;
   TEXTSTR readlog;
} lbs;


void CPROC ExecFirewall( PTRSZVAL psv )
{
	system( lbs.command );
   lbs.exec_timer = 0;
}

static void AddBan( const char *IP )
{
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
}

static PTRSZVAL CPROC failed_pass( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );

   return psv;
}

static PTRSZVAL CPROC failed_pass2( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );
   PARAM( args, S_64, port );

   AddBan( ip_addr );

   return psv;
}

static PTRSZVAL CPROC failed_pass3( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, ip_addr );
   PARAM( args, S_64, port );

   AddBan( ip_addr );

   return psv;
}

static PTRSZVAL CPROC failed_user( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, leader2 );
   PARAM( args, CTEXTSTR, user );
	PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );
   return psv;
}

static PTRSZVAL CPROC failed_user_single( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
	PARAM( args, CTEXTSTR, ip_addr );

   AddBan( ip_addr );
   return psv;
}

static PTRSZVAL CPROC Unhandled( PTRSZVAL psv, CTEXTSTR line )
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

	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for invalid user %w from %w port %i ssh2", failed_pass2 );

	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for %w from %w port %i ssh2 ", failed_pass );
	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for %w from %w", failed_pass );
   AddConfigurationMethod( lbs.pch_scanner, "%m Invalid user %w from %w", failed_pass );

	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Received disconnect from %i", failed_pass2 );
	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Invalid user %w from %i", failed_user );
   SetConfigurationUnhandled( lbs.pch_scanner, Unhandled );
}

static PTRSZVAL CPROC setFirwallCommand( PTRSZSVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	lbs.command = StrDup( path );
   return psv;
}

static PTRSZVAL CPROC setFirwallBanlist( PTRSZSVAL psv, arg_list args )
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

static _8 buf[4096];
int main( void )
{
	int size;
   ReadConfig();
   InitBanScan();
	while( fgets( buf, 4096, stdin ) )
	{
      ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
	}
	return 0;
}


