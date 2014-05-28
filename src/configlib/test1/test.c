#include <stdhdrs.h>
#include <configscript.h>

PTRSZVAL CPROC EventPlaying( PTRSZVAL psv, arg_list args )
{
	lprintf( " Finished loading VLC." );
	printf( " Finished loading VLC.\n" );
   return 0;
}

PTRSZVAL CPROC GenericLog( PTRSZVAL psv, CTEXTSTR line )
{
	lprintf( "%s", line );
	printf( "%s\n", line );
	return 0;
}



int main( void )
{
   PCONFIG_HANDLER pch;
		pch = CreateConfigurationHandler();
		AddConfigurationMethod( pch, "Really playing.", EventPlaying );
		AddConfigurationMethod( pch, "[%w] %m creating header for theo", EventPlaying );
		AddConfigurationMethod( pch, "[%w] %m \"globalhotkeys\"", EventPlaying );
		AddConfigurationMethod( pch, "[%w] [Media: channel2] dshow demux debug: dshow-vdev: Osprey-100e Video Device 2", EventPlaying );
		AddConfigurationMethod( pch, "[%w] [%m] dshw demux debug: dshow-vdev: Osprey-100e Video Device 2", EventPlaying );
		SetConfigurationUnhandled( pch, GenericLog );
      ProcessConfigurationFile( pch, "test.txt", 0 );
}
