#include <stdhdrs.h>
#include <configscript.h>

struct local_ban_scanner
{
   PCONFIG_HANDLER pch_scanner;
} lbs;

static PTRSZVAL CPROC failed_pass( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, user );
   PARAM( args, CTEXTSTR, ip_addr );

   return psv;
}

static PTRSZVAL CPROC failed_user( PTRSZVAL psv, arg_list args )
{
   PARAM( args, CTEXTSTR, leader );
   PARAM( args, CTEXTSTR, leader2 );
   PARAM( args, CTEXTSTR, user );
	PARAM( args, CTEXTSTR, ip_addr );

   return psv;
}

void InitBanScan( void )
{
	lbs.pch_scanner = CreateConfiguratonHandler();
	AddConfigurationMethod( lbs.pch_scanner, "%m Failed password for %w from %i", failed_pass );
	//AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Received disconnect from %i", failed_pass2 );
	AddConfigurationMethod( lbs.pch_scanner, "%m sshd %m: Invalid user %w from %i", failed_user );

}

int main( void )
{


   return 0;
}
