


#include <stdhdrs.h>
#include <render.h>


int main( void )
{
	int n;
	for( n = 0; n < 1000; n++ )
	{
		PRENDERER r = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_CHILD
													|DISPLAY_ATTRIBUTE_LAYERED
													|DISPLAY_ATTRIBUTE_NO_AUTO_FOCUS
												  ,128,128,0,0);
		if( !r )
			break;
      lprintf( "open.." );
		UpdateDisplay(r );
      lprintf( "displayed..." );
		lprintf( "Created %d...", n );
	}
   printf( "Created %d...\n", n );
 

   return 0;
}
