#define USE_IMAGE_INTERFACE GetImageInterface()
#define USE_RENDER_INTERFACE GetRenderInterface()


#include <controls.h>

static int okay = 0;

void fontPicked( uintptr_t psv, SFTFont font ) {
	PSI_CONTROL frame = (PSI_CONTROL)psv;
	PSI_CONTROL pc;

	pc = MakeCaptionedControl( frame, EDIT_FIELD, 5, 5, 410, 18, 0, "Edit Box 1" );
	pc = MakeCaptionedControl( frame, EDIT_FIELD, 5, 28, 410, 18, 0, "Edit Box 2" );
	SetEditControlPassword( pc, TRUE );
	pc = MakeCaptionedControl( frame, EDIT_FIELD, 5, 51, 410, 18, 0, "Edit Box 3" );
	AddCommonButtons( frame, NULL, &okay );
	DisplayFrame( frame );

}

SaneWinMain( argc, argv )
{
	PSI_CONTROL frame = MakeCaptionedControl( NULL, CONTROL_FRAME, 50, 50, 420, 250, 0, "Edit Control Test" );
	SetCommonFont( frame, PickFont( 0, 0, NULL, NULL, NULL/*pAbove*/, fontPicked, (uintptr_t)frame ) );
	/*
	SetCommonFont( frame, RenderFontFile( "arialbd.ttf"
								  , 40, 40
								  , 3 ) );
	*/
	while( !okay ) WakeableSleep( 10000 );
	//CommonWait( frame );
	return 0;
}
EndSaneWinMain( )

