#include <stdio.h>
#include <controls.h>
#include <psi.h>
#include <logging.h>

#define MNU_MINZOOM 10

	PMENU Menu; /* cheat */

EasyRegisterControl( WIDE("Click Me"), 0 );

int AddSubMenus( int base, int level, PMENU menu )
{
		PMENU child = CreatePopup();
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+10, WIDE("x128") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+9, WIDE("x64") );
		if( level < 5 )
			AddSubMenus( 10, level+1, child );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+8, WIDE("x32") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+7, WIDE("x16") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+6, WIDE("x8") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+5, WIDE("x4") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+4, WIDE("x2") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+3, WIDE("x1") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+2, WIDE("x1/2") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+1, WIDE("x1/4") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+0, WIDE("x1/8") );
		{
			TEXTCHAR buffer[256];
			snprintf( buffer, sizeof( buffer ), WIDE("%d-%d-Option"), base, level );
			AppendPopupItem( menu, MF_STRING|MF_POPUP, (PTRSZVAL)child, buffer );
		}
   return 0;
}


static int OnMouseCommon( WIDE("Click Me") )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	if( b & MK_RBUTTON )
	{
		printf( WIDE("Menu result: %d\n"), TrackPopup( Menu, pc ) );

	}
   return 0;
}
	
int main( void )
{
int i;
   PSI_CONTROL frame = CreateFrame( WIDE("Click Me"), 0, 0, 0, 0, BORDER_RESIZABLE, NULL );
Menu = CreatePopup();
DisplayFrame( frame );
MakeNamedControl( frame, WIDE("Click Me"), 0, 0, 256, 256, -1 );

	AppendPopupItem( Menu, MF_STRING, 1000, WIDE("&Properties") );
	AppendPopupItem( Menu, MF_SEPARATOR, 0, NULL );
   AddSubMenus( 10, 1, Menu );
	for( i = 0; i < 12; i++ )
	{
		TEXTCHAR buffer[256];
      PMENU child;
      snprintf( buffer, 256, WIDE("Option %d"), i );
		child = CreatePopup();
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+10, WIDE("x128") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+9, WIDE("x64") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+8, WIDE("x32") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+7, WIDE("x16") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+6, WIDE("x8") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+5, WIDE("x4") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+4, WIDE("x2") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+3, WIDE("x1") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+2, WIDE("x1/2") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+1, WIDE("x1/4") );
		AppendPopupItem( child, MF_STRING, MNU_MINZOOM+0, WIDE("x1/8") );
		AppendPopupItem( Menu, MF_STRING|MF_POPUP, (PTRSZVAL)child, buffer );
	}
  	AppendPopupItem( Menu, MF_STRING, 1002, WIDE("&Options") );
	AppendPopupItem( Menu, MF_STRING, 1004, WIDE("E&xit") );
   CommonWait( frame );
	printf( WIDE("Menu result: %d\n"), TrackPopup( Menu, NULL ) );
   return 0;
}

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
