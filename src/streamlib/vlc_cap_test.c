
#include <controls.h>


IMPORT_METHOD void link_vlc_stream( void );


int main( void )
{
	char tmp;
	PSI_CONTROL test = CreateFrame( "test vlc", 0, 0, 1024, 768, 0, NULL );
   LoadFunction( "vstreamtst.isom", NULL );
	MakeNamedControl( test, "Video Control", 20, 20, 400,300, -1 );
	DisplayFrame( test );
   CommonWait( test );
   return 0;
}
