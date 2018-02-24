
#include <controls.h>


int main( void )
{
	PSI_CONTROL frame;
	while( 1 )
	{
      int okay = 0, done = 0;
		frame = CreateFrame( WIDE("blah"), 150, 150, 0, 0, 0, NULL );
		AddCommonButtons( frame, &okay, &done );
      DisplayFrame( frame );
		CommonWait( frame );
		DestroyFrame( &frame );
	}
   return 0;
}


