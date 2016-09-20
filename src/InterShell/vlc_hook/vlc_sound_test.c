#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <stdhdrs.h>
#include <idle.h>
#include <controls.h>
#include "vlcint.h"

int main( int argc, char ** argv )
{
   char *file_to_play;
	uint32_t w, h;
	PLIST names = NULL;
   int n;
	SetSystemLog( SYSLOG_FILE, stderr );
   for( n = 1; n < 75; n++ )
	{
		char file[128];
      snprintf( file, sizeof( file ), "sounds/bingo/%c%d.wav", "BINGO"[(n-1)/15], n );
		PlaySoundFile( file );
		WakeableSleep( 500 );
	}
   PlaySoundFile( "sounds/bingo/b1.wav" );

   WakeableSleep( 10000 );

   return 0;
}
