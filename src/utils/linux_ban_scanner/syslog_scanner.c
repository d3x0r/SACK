#include <stdhdrs.h>
#include <configscript.h>

struct local_ban_scanner
{
	PCONFIG_HANDLER pch_scanner;
   _32 exec_timer;
} lbs;


void CPROC ExecFirewall( PTRSZVAL psv )
{
	system( "/etc/systemd/network/firewall/firewall.sh" );
   lbs.exec_timer = 0;
}

static void AddBan( const char *IP )
{
	FILE *file = fopen( "/etc/systemd/network/firewall/banlist.auto", "ab" );
   printf( "add %s\n", IP );
   if( file )
		fprintf( file, "%s\n", IP );
	fclose( file );
	if( !lbs.exec_timer )
		lbs.exec_timer = AddTimerEx( 1000, 0, ExecFirewall, 0 );
	else
      RescheduleTimerEx( lbs.exec_timer, 1000 );
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

void InitBanScan( void )
{
	lbs.pch_scanner = CreateConfigurationEvaluator();
   AddConfigurationMethod( lbs.pch_scanner, "%m Did not receive identification string from %w", failed_user_single );

   AddConfigurationMethod( lbs.pch_scanner, "%m Disconnected from %w port %w [preauth]", failed_pass3 );

   AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for invalid user %w from %w port %i ssh2", failed_pass2 );

	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for %w from %w", failed_pass );
	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Received disconnect from %i", failed_pass2 );
	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Invalid user %w from %i", failed_user );
   SetConfigurationUnhandled( lbs.pch_scanner, Unhandled );
}

static _8 buf[4096];
int main( void )
{
	int size;
   InitBanScan();
	while( fgets( buf, 4096, stdin ) )
	{
      ProcessConfigurationInput( lbs.pch_scanner, buf, strlen(buf), 0 );
	}
	return 0;
}


