
#include <network.h>
#include <configscript.h>
#include <sqlgetoption.h>
#include "../../intershell/widgets/include/banner.h"



int main( int argc, char **argv )
{
	PCLIENT pc;
	PBANNER banner = NULL;
   static TEXTCHAR msgbuf[1024];
   static TEXTCHAR outbuf[1024];
   static TEXTCHAR net_interface[256];
	static TEXTCHAR broadcast[256];
   int delay;
	NetworkStart();
	if( argc > 2 )
	{
      delay = atoi( argv[2] );
	}
	else
		delay = 0;

	SACK_GetProfileString( GetProgramName(), "Display Banner Text", "Service Available\\nClick to continue", msgbuf, sizeof( msgbuf ) );
   StripConfigString( outbuf, msgbuf );
	//snprintf( msgbuf, sizeof( msgbuf ), ( atoi( argv[1] ) == 1 )?"asldfjsdflksajdf;laksjdf":argv[1]);

	SACK_GetProfileStringEx( GetProgramName(), "service announce interface", "0.0.0.0:3230", net_interface, sizeof( net_interface ), TRUE );
	SACK_GetProfileStringEx( GetProgramName(), "service announce broadcast", "255.255.255.255:3233", broadcast, sizeof( broadcast ), TRUE );
	pc = ConnectUDP( net_interface, 3230, broadcast, 3233, NULL, NULL );
	if( StrCaseCmp( argv[1], "clear" ) == 0 )
	{
		char buffer[64];
      int len;
		len = snprintf( buffer, sizeof( buffer ), "%08xCLEARALL", GetTickCount() );
		SendUDP( pc, buffer, len );
      WakeableSleep( 10 );
		SendUDP( pc, buffer, len );
      WakeableSleep( 10 );
		SendUDP( pc, buffer, len );
      return 0;
	}
   if( !delay )
		CreateBannerEx( NULL, &banner, outbuf, BANNER_CLICK|BANNER_NOWAIT|BANNER_TOP, 0 );

	{
		char buffer[64];
      int len;
		len = snprintf( buffer, sizeof( buffer ), "%08xSERVING %s", GetTickCount(), argv[1] );
		SendUDP( pc, buffer, len );
      WakeableSleep( 10 );
		SendUDP( pc, buffer, len );
      WakeableSleep( 10 );
		SendUDP( pc, buffer, len );
	}

	{
		// wait for click
		if( !delay )
		{
			WaitForBanner( banner );
			RemoveBanner( banner );
		}
		else
         WakeableSleep( delay );
	}

	{
		char buffer[64];
      int len;
		len = snprintf( buffer, sizeof( buffer ), "%08xSERVED  %s", GetTickCount(), argv[1] );
		SendUDP( pc, buffer, len );
      WakeableSleep( 10 );
		SendUDP( pc, buffer, len );
      WakeableSleep( 10 );
		SendUDP( pc, buffer, len );
	}


   return 0;
}

