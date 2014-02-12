#include <stdhdrs.h>
#include <stdio.h>
#include <vidlib.h>
#include <image.h>

extern FONT USEFONT;
#define MYFONT &USEFONT

int main( void )
{
	HVIDEO hVid = InitVideo( WIDE("Font Test") );
	ImageFile *surface = GetVideoImage( hVid );
	SetBlotMethod( BLOT_MMX );
	PutStringFont( surface, 0, 0, Color( 255, 255, 255 ), Color( 0, 0, 255 ), WIDE("ABC Test Font!?"), MYFONT );
	PutStringFont( surface, 0, 60, Color( 255, 255, 255 ), Color( 0, 0, 0 ), WIDE("XYZ Test Font.!"), MYFONT );
	//PutStringFont( surface, 0, 120, Color( 255, 255, 255 ), Color( 0, 0, 0 ), WIDE("A1B2C3D4E5J6K7L8M9N0OPQRSTUVXWYZ"), MYFONT );
	//PutStringFont( surface, 0, 150, Color( 255, 255, 255 ), Color( 0, 0, 0 ), WIDE("!@#$%^&*()<>?,./:\");\'{}[]_+|-=-\\", MYFONT );
	PutString( surface, 0, 120, Color( 255, 255, 255 ), Color( 0, 0, 0 ), WIDE("A1B2C3D4E5J6K7L8M9N0OPQRSTUVXWYZ") );
	PutString( surface, 0, 150, Color( 255, 255, 255 ), Color( 0, 0, 0 ), WIDE("!@#$%^&*()<>?,./:\");\'{}[]_+|-=-\\" );
	UpdateVideo( hVid );
	while( !kbhit() ) Sleep(0);
	while( kbhit() ) getch();
	return 0;
}

// $Log: test.c,v $
// Revision 1.3  2003/03/25 08:45:53  panther
// Added CVS logging tag
//
