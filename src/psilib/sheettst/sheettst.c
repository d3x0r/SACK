#include <logging.h>
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <controls.h>

#define SHT_SHEETS  100
#define SHT_ONE     101
#define SHT_TWO     102
#define SHT_THREE   103

int first = 1;
Image active;
Image inactive;

PCOMMON MakePages( PCOMMON frame )
{
	PCOMMON sheets;
	PCOMMON sheet[3];
	CDATA cActive = BASE_COLOR_BLACK;
	CDATA cInactive = BASE_COLOR_WHITE;
	uint32_t width, height;
	Image surface = GetControlSurface( frame );
	width = surface->width;
	height = surface->height;
	//GetImageSize( GetControlSurface( frame ), &width, &height );
	Log2( WIDE("Making sheet control %") _32f WIDE(" by %") _32f WIDE(""), width, height );
	sheets = MakeSheetControl( frame, 5, 5, width - 10, height - 10 - (first?(COMMON_BUTTON_PAD + COMMON_BUTTON_HEIGHT):0), SHT_SHEETS );
	SetTabImages( sheets, active, inactive );
	SetTabTextColors( sheets, cActive, cInactive );
	first = 0;
	GetSheetSize( sheets, &width, &height );
	Log2( WIDE("Pages are %") _32f WIDE(" by %") _32f WIDE(""), width, height );
	sheet[0] = CreateFrame( WIDE("One"), 0, 0, width, height, BORDER_FIXED|BORDER_NOCAPTION|BORDER_WITHIN|BORDER_NONE, sheets );
	SetControlID( sheet[0], SHT_ONE );
	//MakeTextControl( sheet[0], 0, 5, 5, width - 10, 15, TXT_STATIC, WIDE("Sheet One") );

	sheet[1] = CreateFrame( WIDE("Two"), 0, 0, width, height, BORDER_FIXED|BORDER_NOCAPTION|BORDER_WITHIN|BORDER_NONE, sheets );
	SetControlID( sheet[1], SHT_TWO );
	MakeTextControl( sheet[1], 5, 5, width - 10, 15, TXT_STATIC, WIDE("Sheet Two"), 0 );

	sheet[2] = CreateFrame( WIDE("Three"), 0, 0, width, height, BORDER_FIXED|BORDER_NOCAPTION|BORDER_WITHIN|BORDER_NONE, sheets );
	SetControlID( sheet[2], SHT_THREE );
	MakeTextControl( sheet[2], 5, 5, width - 10, 15, TXT_STATIC, WIDE("Sheet Three"), 0 );

	AddSheet( sheets, sheet[0] );
	AddSheet( sheets, sheet[1] );
	AddSheet( sheets, sheet[2] );
	return (PCOMMON)sheet[0];
}

SaneWinMain( argc, argv )
{
	{
		PCOMMON frame;
		active = LoadImageFile( WIDE("whitetab.png") );
		inactive = LoadImageFile( WIDE("blacktab.png") );
		frame = CreateFrame( WIDE("Sheet Test"), 0, 0, 480, 320, BORDER_NORMAL, NULL );
		if( frame )
		{
			int done = 0, okay = 0;
			if( argc > 1 )
			{
				FRACTION f = { 1, 1 };
				SetFrameFont( frame, RenderFontFile( WIDE("arialbd.ttf"), 20, 20, 3 ) );
				// reset the scaling from the font (do allow it to scale the outer frame)
				// (this gives us a bigger surface for better visibility)
				SetCommonScale( frame, &f, &f );
			}
			MakePages( MakePages( MakePages( MakePages( frame ) ) ) );
			AddCommonButtons( frame, &done, &okay );
			DisplayFrame( frame );
			CommonLoop( &done, &okay );
		}
	}
	return 0;
}
EndSaneWinMain(  )

//---------------------------------------------------------------------------
//
// $Log: sheettst.c,v $
// Revision 1.13  2005/03/04 19:07:32  panther
// Define SetItemText
//
// Revision 1.12  2005/02/01 02:20:23  panther
// Debugging added...
//
// Revision 1.11  2005/01/24 05:31:45  panther
// Update test proggy
//
// Revision 1.10  2004/12/20 19:45:15  panther
// Fixes for protected sheet control init
//
// Revision 1.9  2004/10/21 16:45:51  d3x0r
// Updaes to dialog handling... still ahve  aproblem with caption resize
//
// Revision 1.8  2004/09/04 18:49:48  d3x0r
// Changes to support scaling and font selection of dialogs
//
// Revision 1.7  2004/09/03 14:43:48  d3x0r
// flexible frame reactions to font changes...
//
// Revision 1.6  2004/03/23 22:52:58  d3x0r
// Mods to compile vs interface library
//
// Revision 1.5  2003/09/22 09:44:07  panther
// UPdate to build win32
//
// Revision 1.4  2003/09/21 20:32:41  panther
// Sheet control nearly functional.
// More logging on Orphan/Adoption path.
//
// Revision 1.3  2003/09/21 16:25:28  panther
// Removed much noisy logging, all in the interest of sheet controls.
// Fixed some linking of services.
// Fixed service close on dead client.
//
// Revision 1.2  2003/09/21 11:49:04  panther
// Fix service refernce counting for unload.  Fix Making sub images hidden.
// Fix Linking of services on server side....
// cleanup some comments...
//
// Revision 1.1  2003/09/21 00:36:49  panther
// Simple property sheet test program
//
//
