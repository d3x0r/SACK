#include <stdhdrs.h>
#include <service_hook.h>


static void CPROC Start( void )
{
}


SaneWinMain( argc, argv )
{
   // doesn't matter...
	if( argc > (1) && StrCaseCmp( argv[1], WIDE("install") ) == 0 )
	{
		ServiceInstall( GetProgramName() );
		return 0;
	}
	if( argc > (1) && StrCaseCmp( argv[1], WIDE("uninstall") ) == 0 )
	{
		ServiceUninstall( GetProgramName() );
		return 0;
	}

	if( LoadPrivateFunction( WIDE("sack.msgsvr.service.plugin"), NULL ) )
	{
		SetupService( (TEXTSTR)GetProgramName(), Start );
	}
   else
		lprintf( WIDE("Failed to load message core service.\n") );
   return 0;
}
EndSaneWinMain()
