#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <stdhdrs.h>
#include <image.h>


SaneWinMain( argc, argv )
{
	FLAGSETTYPE flags = 0;
	SystemLogTime( 0 );
	SetSyslogOptions( &flags );
	SetSystemLog( SYSLOG_FILE, stdout );
	lprintf( "if the size is not a fixed size, it is shown as -1" );
	DumpFontCache();
	SetSystemLog( SYSLOG_NONE, 0 );
	return 0;
}
EndSaneWinMain()
