
#include <image.h>
#include <vidlib.h>
#include <vectlib.h>


void CPROC UpdateImage( PTRSZVAL psv, PRENDERER hVideo )
{
	int i;
	ImageFile *pImage;                       
	pImage = GetDisplayImage( hVideo );
	for( i = 0; i < 100; i+= 10 )
	{
		do_line( pImage, i, 0, i, 100, Color( i+64, 0, 0 ) );
		do_line( pImage, 0, i, 100, i, Color( 0, i+64, 0 ) );
	}
///	UpdateDisplay( hVideo);
}


int main( void )
{
	PRENDERER hDisplay;
	SetApplicationTitle( WIDE("Train Test Application") );
	//pImage = MakeImageFile( 320, 200 );
	hDisplay = OpenDisplaySizedAt( 0, 0, 0, 250, 250 );
	SetRedrawHandler( hDisplay, UpdateImage, 0 );
   UpdateDisplay( hDisplay );
	hDisplay = OpenDisplay( 0, 0, 0, 250, 250 );
	SetRedrawHandler( hDisplay, UpdateImage, 0 );
   UpdateDisplay( hDisplay );
	hDisplay = OpenDisplay( 0, 0, 0, 250, 250 );
	SetRedrawHandler( hDisplay, UpdateImage, 0 );
   UpdateDisplay( hDisplay );
	getch();

	return 0;
}

// $Log: $
