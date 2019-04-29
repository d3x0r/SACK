#include <stdio.h>
#include <controls.h>
#include <psi.h>
#include <logging.h>

#define MNU_MINZOOM 10

	PMENU Menu; /* cheat */

EasyRegisterControl( "Click Me", 0 );

int AddSubMenus( int base, int level, PMENU menu )
{
		PMENU child = CreatePopup();
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+10, "x128" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+9, "x64" );
		if( level < 5 )
			AddSubMenus( 10, level+1, child );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+8, "x32" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+7, "x16" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+6, "x8" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+5, "x4" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+4, "x2" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+3, "x1" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+2, "x1/2" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+1, "x1/4" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+0, "x1/8" );
		{
			TEXTCHAR buffer[256];
			snprintf( buffer, sizeof( buffer ), "%d-%d-Option", base, level );
			AppendPopupItem( menu, MF_STRING|MF_POPUP, (uintptr_t)child, buffer );
		}
   return 0;
}


static int OnMouseCommon( "Click Me" )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	if( b & MK_RBUTTON )
	{
		printf( "Menu result: %d\n", TrackPopup( Menu, pc ) );

	}
   return 0;
}
	
SaneWinMain( argc, argv )
{
	int i;
	PSI_CONTROL frame = CreateFrame( "Click Me", 0, 0, 0, 0, BORDER_RESIZABLE, NULL );
	Menu = CreatePopup();
	DisplayFrame( frame );
	MakeNamedControl( frame, "Click Me", 0, 0, 256, 256, -1 );

	AppendPopupItem( Menu, MF_STRING, 1000, "&Properties" );
	AppendPopupItem( Menu, MF_SEPARATOR, 0, NULL );
	AddSubMenus( 10, 1, Menu );
	for( i = 0; i < 12; i++ )
	{
		TEXTCHAR buffer[256];
		PMENU child;
			snprintf( buffer, 256, "Option %d", i );
		child = CreatePopup();
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+10, "x128" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+9, "x64" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+8, "x32" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+7, "x16" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+6, "x8" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+5, "x4" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+4, "x2" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+3, "x1" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+2, "x1/2" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+1, "x1/4" );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+0, "x1/8" );
		AppendPopupItem( Menu, MF_STRING|MF_POPUP, (uintptr_t)child, buffer );
	}
	AppendPopupItem( Menu, MF_STRING, 1002, "&Options" );
	AppendPopupItem( Menu, MF_STRING, 1004, "E&xit" );
	CommonWait( frame );
	printf( "Menu result: %d\n", TrackPopup( Menu, NULL ) );
	return 0;
}
EndSaneWinMain( )

// $Log: menutest.c,v $
// Revision 1.5  2003/11/04 23:59:52  panther
// Update menutest
//
// Revision 1.4  2003/03/25 09:37:58  panther
// Fix file tails mangled by CVS logging
//
// Revision 1.3  2003/03/25 08:45:57  panther
// Added CVS logging tag
//
