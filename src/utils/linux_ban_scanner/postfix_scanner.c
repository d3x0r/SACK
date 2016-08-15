#include <stdhdrs.h>
#include <configscript.h>

struct local_ban_scanner
{
	PCONFIG_HANDLER pch_scanner;
	_32 exec_timer;
	TEXTSTR command;
	TEXTSTR output;
	TEXTSTR readlog;
	TEXTSTR pending_ban2;
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

static PTRSZVAL CPROC failed_user_2( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, data_ignore1 );
	PARAM( args, CTEXTSTR, ip_addr );

	if( lbs.pending_ban2 )
		Deallocate( TEXTSTR, lbs.pending_ban2 );
	lbs.pending_ban2 = StrDup( ip_addr );
	return psv;
}

static PTRSZVAL CPROC fail_user_2_ban( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, leader );
	PARAM( args, CTEXTSTR, ip_addr );
	PARAM( args, CTEXTSTR, data_ignore1 );

	AddBan( ip_addr );
	return psv;
}

static PTRSZVAL CPROC fail_prior_user2( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, leader );
	if( lbs.pending_ban2 )
		AddBan( lbs.pending_ban2 );
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
   AddConfigurationMethod( lbs.pch_scanner, "%m connect from unknown[%m]", failed_user_single );
   AddConfigurationMethod( lbs.pch_scanner, "%m connect from %m[%m]", failed_user_2 );

#define AddRule( rule, handler ) AddConfigurationMethod( lbs.pch_scanner, "%m" rule, handler )

	AddRule( " SASL: Connect to /var/run/dovecot/auth-client failed: Permission denied", fail_prior_user2 );

	AddRule( " warning: %m: hostname %m verification failed: Name or service not known", fail_user_2_ban );



   SetConfigurationUnhandled( lbs.pch_scanner, Unhandled );
}

static PTRSZVAL CPROC setFirewallCommand( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
	lbs.command = StrDup( path );
	return psv;
}

static PTRSZVAL CPROC setFirewallBanlist( PTRSZVAL psv, arg_list args )
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
	ProcessConfigurationFile( pch, "linux_postfix_scanner.conf", 0 );
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


