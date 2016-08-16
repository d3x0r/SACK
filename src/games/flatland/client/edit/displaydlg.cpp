
#include <psi.h>
#include <controls.h>
#include "display.h"

#define CHK_SHOWGRID 		1000
#define CHK_SHOWLINES 		1001
#define CHK_SHOWTEXTURES 	1002
#define CHK_SHOWTEXT       1003
#define CHK_GRID           1004
#define EDT_GRIDX          1005
#define EDT_GRIDY          1006
#define BTN_GRIDCOLOR      1007
#define EDT_SCALE          1008
#define CHK_GRIDTOP        1009
#define 	CLR_GRID          1010
#define CLR_BACKGROUND     1011



void CPROC ChooseDisplayColor( uintptr_t psvColor, PCONTROL pc )
{
	CDATA *color = (CDATA*)psvColor;
	PickColor( color, *color, GetFrame( pc ) );
}

void DisplayProperties( PCOMMON pc, int x, int y )
{
	// configure grid, grid colors
	// toggle display grid, display lines, display textures
	PCOMMON pf;
	TEXTCHAR val[256];
	int DisplayDone, DisplayOkay;
   PCOMMON pc_tmp;
   PDISPLAY display = ControlData( PDISPLAY, pc );
	DisplayDone = FALSE;
	DisplayOkay = FALSE;
#define FRAME_WIDTH 180
#define FRAME_HEIGHT 208
	pf = CreateFrame( WIDE("Display Properties"), 0, 0, FRAME_WIDTH, FRAME_HEIGHT, 0, pc );
  	MoveFrame( pf, x, y );

	pc_tmp = MakeCheckButton( pf, 5, 5, 150, 14, CHK_SHOWLINES, WIDE("Show Lines"), 0, NULL, 0 );
	SetCheckState( pc_tmp, display->flags.bShowLines );

	pc_tmp = MakeCheckButton( pf, 5, 22, 150, 14, CHK_SHOWTEXTURES, WIDE("Show Textures"), 0, NULL, 0 );
	SetCheckState( pc_tmp, display->flags.bShowSectorTexture );

	pc_tmp = MakeCheckButton( pf, 5, 39, 150, 14, CHK_SHOWTEXT, WIDE("Show Sector Names"), 0, NULL, 0 );
	SetCheckState( pc_tmp, display->flags.bShowSectorText );

	pc_tmp = MakeCheckButton( pf, 5, 56, 150, 14, CHK_SHOWGRID, WIDE("Show Grid"), 0, NULL, 0 );
	SetCheckState( pc_tmp, display->flags.bShowGrid );

	pc_tmp = MakeCheckButton( pf, 5, 73, 150, 14, CHK_GRID, WIDE("Use Grid"), 0, NULL, 0 );
	SetCheckState( pc_tmp, display->flags.bUseGrid );

	pc_tmp = MakeCheckButton( pf, 5, 90, 150, 14, CHK_GRIDTOP, WIDE("Grid on Top"), 0, NULL, 0 );
	SetCheckState( pc_tmp, display->flags.bGridTop );

#define LASTCHECK 90+17

	MakeTextControl( pf, 5, LASTCHECK+2, 45, 12, TXT_STATIC, WIDE("Grid X:"), TEXT_NORMAL );
	snprintf( val, 256, WIDE("%d"), display->GridXUnits );
	pc_tmp = MakeEditControl( pf, 55, LASTCHECK, 36, 16, EDT_GRIDX, val, 0 );

	MakeTextControl( pf, 5, LASTCHECK+20, 45, 12, TXT_STATIC, WIDE("Grid Y:"), TEXT_NORMAL );
	snprintf( val, 256, WIDE("%d"), display->GridYUnits );
	pc_tmp = MakeEditControl( pf, 55, LASTCHECK+18, 36, 16, EDT_GRIDY, val, 0 );

	EnableColorWellPick( MakeColorWell( pf, 5, LASTCHECK+36, 80, 16, CLR_GRID, display->GridColor ), TRUE );
	EnableColorWellPick( MakeColorWell( pf, 90, LASTCHECK+36, 80, 16, CLR_BACKGROUND, display->Background ), TRUE );

	MakeTextControl( pf, 5, LASTCHECK+36+21, 45, 12, TXT_STATIC, WIDE("Scale:"), TEXT_NORMAL );
	snprintf( val, 256, WIDE("%g"), display->scale );
	pc_tmp = MakeEditControl( pf,55, LASTCHECK+36+19, 72, 16, EDT_SCALE, val, 0 );

	AddCommonButtons( pf, &DisplayDone, &DisplayOkay );

	DisplayFrame( pf );
	CommonWait( pf );

	if( DisplayOkay )
	{
		display->GridColor = GetColorFromWell( GetControl( pf, CLR_GRID ) );
		display->Background = GetColorFromWell( GetControl( pf, CLR_BACKGROUND ) );

		display->flags.bShowSectorText = GetCheckState( GetControl( pf, CHK_SHOWTEXT ) );
		display->flags.bShowSectorTexture = GetCheckState( GetControl( pf, CHK_SHOWTEXTURES ) );
		display->flags.bShowLines = GetCheckState( GetControl( pf, CHK_SHOWLINES ) );
		display->flags.bShowGrid = GetCheckState( GetControl( pf, CHK_SHOWGRID ) );
		display->flags.bUseGrid = GetCheckState( GetControl( pf, CHK_GRID ) );
		display->flags.bGridTop = GetCheckState( GetControl( pf, CHK_GRIDTOP ) );
		GetControlText( GetControl( pf, EDT_GRIDX ), val, 256 );
		display->GridXUnits = atoi( val );
		if( display->GridXUnits < 4 )
      		display->GridXUnits = 4;
		GetControlText( GetControl( pf, EDT_GRIDY ), val, 256 );
		display->GridYUnits = atoi( val );
		if( display->GridYUnits < 4 )
	      	display->GridYUnits = 4;

		GetControlText( GetControl( pf, EDT_SCALE ), val, 256 );
		sscanf( val, WIDE("%g"), &display->scale );
		//display->scale = atof( val );
	}

	DestroyFrame( &pf );

	return;
}
