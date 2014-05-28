#include <vidlib.h>
#include <stdio.h>

unsigned char alphatable[256][256];



int main( void )
{
	int x, y, val;
	HVIDEO output;
	ImageFile *surface;
	output = InitVideoSizedEx( WIDE("Alpha Table Output"), TRUE, 270, 270 );
   surface = GetVideoImage( output );
	for( x = 0; x < 256; x++ )
		for( y = 0; y < 256; y++ )
		{
			val = y * 0x100;
			//printf( WIDE("%d "), val );
			//printf( WIDE("%d "), 0x10000 - val );
			val = val + ( ( ( 0x10000 - val ) * ( x ) ) / 0x100 );
			val /= 0x100;
			//printf( WIDE("(%d,%d)=%d\n"), x, y, val );
			alphatable[y][x] = val;
			if( val == 0x40 || 
				 val == 0x80 || 
				 val == 0xc0 || 
				 val == 0xe0 || 
				 val == 0xf0 )

				plot( surface, x, y, Color( 0, 255, 0 ) );
			else if( val == 0xF8 )
				plot( surface, x, y, Color( 255, 0, 255 ) );
			else if( val == 0xFF )
				plot( surface, x, y, Color( 0, 255, 255 ) );
			
			else if( val < 0x40 )
				plot( surface, x, y, Color( 0, 0, val*4 ) );
			else if( val < 0x80 )
				plot( surface, x, y, Color( (val-0x40)*4, 0, 255 ) );
			else if( val < 0xc0 )
				plot( surface, x, y, Color( 255, 0, 255 - ( val - 0x80 ) * 4 ) );
			else
				plot( surface, x, y, Color( 255, 255 - ( 0x100 - val )*4, 255 - ( 0x100 - val )*4 ) );
		}
	UpdateVideo( output );
	while( !kbhit() );

}

// $Log: alphatab.c,v $
// Revision 1.2  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
