#include <stdhdrs.h>
#include <sharemem.h>
#include <string.h>
#include <stdio.h>
#include <idle.h>

#include "global.h"

#include "chain.h"
#include "controls.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
extern int _boardx;
extern int _boardy;
extern int zoom;
extern int PeiceSize;
extern Image pGrid;
#define BOARD_X _boardx
#define BOARD_Y _boardy
//------------------------------------------------------------------------------

#ifdef TXT_STATIC
#undef TXT_STATIC
#endif
#define TXT_STATIC  0

#define BTN_OKAY    1
#define EDT_NAME 1000
#define BTN_ADD  1001
#define BTN_PLAY 1002
#define TXT_CONFIGNUM 1003
#define TXT_PLAYERS   1004
#define CHK_COMPUTER 1005
#define BTN_HELP   1006
#define BTN_QUIT   1007
#define BTN_COLOR1 1010
#define BTN_COLOR2 1011
#define BTN_COLOR3 1012
#define BTN_COLOR4 1013
#define BTN_COLOR5 1014
#define BTN_COLOR6 1015
#define BTN_COLOR7 1016
#define BTN_COLOR8 1017
#define BTN_COLOR9 1018
	
static PCOMMON pPlayerFrame;
//------------------------------------------------------------------------------

int done = FALSE;
int nPlayers = 0;
PCONTROL pcColor;
int LastColor;
extern PLAYER players[];

void CPROC AddPlayerButton( PTRSZVAL psv, PCONTROL pc )
{
	TEXTCHAR buffer[256];
	PCOMMON frame = GetFrame( pc );
	(nPlayers)++;

 	GetControlText( GetControl( frame, EDT_NAME )
 					  , buffer
 					  , sizeof(players[nPlayers].name) );
	if( strcmp( buffer, players[nPlayers].name ) )
	{
		players[nPlayers].wins = 0;
	}
	strcpy( players[nPlayers].name, buffer );
 	// also find current selected color....
	players[nPlayers].computer = GetCheckState( GetControl( frame, CHK_COMPUTER  ) );
  	players[nPlayers].color = LastColor;
  	EnableControl( pcColor, FALSE );
	if( nPlayers >= 2 )
		EnableControl( GetControl( frame, BTN_PLAY), TRUE );
	if( nPlayers >= 9 )
	{
		EnableControl( pc, FALSE ); // cannot add any more players...
		EnableControl( GetControl( frame, EDT_NAME ), FALSE );
		EnableControl( GetControl( frame, CHK_COMPUTER ), FALSE );
	}
	else
	{
		SetCheckState( GetControl( frame, CHK_COMPUTER ), players[nPlayers+1].computer );
		snprintf( buffer, 256, WIDE("Configure Player #%d"), (nPlayers)+1 );
		SetControlText( GetControl( frame, TXT_CONFIGNUM ), buffer );
		SetControlText( GetControl( frame, EDT_NAME )
						  , players[nPlayers+1].name );
		SetCommonFocus( GetControl( frame, EDT_NAME ) );
		if( IsControlEnabled( GetControl( frame, BTN_COLOR1 + players[nPlayers+1].color - 1 ) ) )
		{
	   		LastColor = players[nPlayers+1].color;
   			pcColor = GetControl( frame, BTN_COLOR1 + players[nPlayers+1].color - 1);
		}
		else
		{
			int n;
			for( n = 0; n < MAX_PLAYERS; n++ )
			{
				if( IsControlEnabled( pcColor = GetControl( frame, BTN_COLOR1 + n ) ) )
				{
		   			LastColor = n;
		   			break;
				}
			}
		}
		PressButton( pcColor, TRUE );
	}
	snprintf( buffer, 256, WIDE("Players: %d"), nPlayers );
	SetControlText( GetControl( frame, TXT_PLAYERS ), buffer );

}

void CPROC SetPlayerColor( PTRSZVAL psv, PCONTROL pc )
{
	if( LastColor >= 0 )
	{
		PressButton( pcColor, FALSE );
	}
	PressButton( pc, TRUE );
	LastColor = ( GetControlID( pc ) - BTN_COLOR1 ) + 1;
	pcColor = pc;
}

void CPROC PlayGameButton( PTRSZVAL psv, PCONTROL pc  )
{
	done = TRUE;
}

void CPROC ShowSomeHelp( PTRSZVAL psv, PCONTROL pc )
{

}

void CPROC QuitGame( PTRSZVAL psv, PCONTROL pc )
{
   DestroyFrame( &pPlayerFrame );
	exit(1);
}

// returns the number of valid players
int ConfigurePlayers( void )
{
	PCOMMON pFrame;
	PCONTROL pc;
	Image pImages[MAX_PLAYERS];
	int n;
	for( n = 1; n <= MAX_PLAYERS; n++ )
	{
		extern Image pAtom;
		pImages[n-1] = MakeImageFile( 22, 22 );
		ClearImageTo( pImages[n-1], 0 );
		BlotScaledImageSizedEx( pImages[n-1], pAtom
   						    	 , 0, 0, pImages[n-1]->width, pImages[n-1]->height
   						    	 , 0, 0, pAtom->width, pAtom->height
                         	 , TRUE, BLOT_MULTISHADE
                         	 , 0, Color( 255,255,255)
         				    	 , Colors[n]
 	   					   	 );
//		BlotScaledImageEx( pImages[n-1], pAtom, TRUE, BLOT_MULTISHADE
//                       , 0, Color( 255,255,255)
//      						, Colors[n] );
	}
	pFrame = CreateFrame( WIDE("Config Player"), 0, 0, 275, 125, BORDER_NORMAL, (PCOMMON)0 );
	pPlayerFrame = pFrame;
	MoveFrame( pFrame, 50, 50 );
	MakeButton( pFrame, 255, 5, 15, 15, BTN_QUIT, WIDE("X"), 0, QuitGame, 0 );
	MakeTextControl( pFrame, 5, 10, 150, 15, TXT_CONFIGNUM, WIDE("Configure Player #1"), 0 );
	MakeTextControl( pFrame, 5, 25, 150, 15, TXT_PLAYERS, WIDE("Players: 0 "), 0 );
	MakeTextControl( pFrame, 5, 40, 35, 11, TXT_STATIC, WIDE("Name:"), 0 );
	MakeTextControl( pFrame, 5, 58, 100, 13, TXT_STATIC, WIDE("Select Color"), 0 );
	MakeEditControl( pFrame, 43, 39, 84, 14, EDT_NAME, players[1].name, 0 );
	MakeCheckButton( pFrame, 131, 40, 80, 14, CHK_COMPUTER, WIDE("Computer"), 0, NULL, 0 );
	MakeImageButton( pFrame, 5, 71, 26, 26,   BTN_COLOR1, pImages[0], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 35, 71, 26, 26,  BTN_COLOR2, pImages[1], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 65, 71, 26, 26,  BTN_COLOR3, pImages[2], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 95, 71, 26, 26,  BTN_COLOR4, pImages[3], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 125, 71, 26, 26, BTN_COLOR5, pImages[4], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 155, 71, 26, 26, BTN_COLOR6, pImages[5], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 185, 71, 26, 26, BTN_COLOR7, pImages[6], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 215, 71, 26, 26, BTN_COLOR8, pImages[7], 0, SetPlayerColor, 0 );
	MakeImageButton( pFrame, 245, 71, 26, 26, BTN_COLOR9, pImages[8], 0, SetPlayerColor, 0 );
	MakeButton( pFrame, 202, 100, 59, 20, BTN_HELP, WIDE("Help?"), 0, ShowSomeHelp, 0 );
	MakeButton( pFrame, 108, 100, 82, 20, BTN_ADD, WIDE("Add Player"), 0, AddPlayerButton,  (PTRSZVAL)&nPlayers);
	MakeButton( pFrame, 14, 100, 82, 20, BTN_PLAY, WIDE("Play Game"), 0, PlayGameButton, (PTRSZVAL)&done );
	EnableControl( GetControl( pFrame, BTN_PLAY ), FALSE );
	SetCommonFocus( GetControl( pFrame, EDT_NAME ) );
	LastColor = players[1].color;
	pcColor = GetControl( pFrame, BTN_COLOR1 + players[1].color - 1);
	PressButton( pcColor, TRUE );
	SetCheckState( GetControl( pFrame, CHK_COMPUTER ), players[1].computer );
	DisplayFrame( pFrame );
	nPlayers = 0;
	done = FALSE;
	while( !done )
	{
		IdleFor( 250 );
	}

	DestroyFrame( &pFrame );
	for( n = 1; n <= MAX_PLAYERS; n++ )
	{
		UnmakeImageFile( pImages[n-1] );
	}

	return nPlayers;
}

//---------------------------------------------------------------------------


#define SLD_BOARD_X 1020
#define SLD_BOARD_Y 1021
#define CHK_ANIMATE 1022
#define CHK_SPHERICAL 1023
PCONTROL pcBoard;
static int CPROC DrawMimicBoard( PCOMMON pc )
{

	int x, y, width, height;
	Image Surface = GetControlSurface( pc );
	ClearImageTo( Surface, Color(192,192,192) );
	width = BOARD_X * 3;
	height = BOARD_Y * 3;
	for( x = 0; x <= BOARD_X; x++ )
	{
		int line = x * (Surface->width-1)/BOARD_X;
		do_line( Surface, line, 0, line, Surface->height, Color(0,0,0) );
	}
	for(y = 0; y <= BOARD_Y; y++ )
	{
		int line = y * (Surface->height-1)/BOARD_Y;
		do_line( Surface, 0, line, Surface->width, line, Color(0,0,0) );
	}
   return 1;
}

void CPROC SliderUpdatedX( PTRSZVAL psv, PCONTROL pc, int val )
{
//	printf( WIDE("Value: %d\n"), val );
	BOARD_X = val;
   SmudgeCommon( pcBoard );
}

void CPROC SliderUpdatedY( PTRSZVAL psv, PCONTROL pc, int val )
{
//	printf( WIDE("Value: %d\n"), val );
	BOARD_Y = val;
   SmudgeCommon( pcBoard );
}

#include <psi.h>
CONTROL_REGISTRATION MimicBoard = { WIDE("Chain Reaction Board Sizer")
											 , { { 150, 150 }, 0, BORDER_INVERT|BORDER_THIN }
											 , NULL
											 , NULL
											 , DrawMimicBoard
};
PRELOAD( RegisterMimicBoard ){ DoRegisterControl( &MimicBoard ); }

void ConfigureBoard( int *animate, int *sphere )
{
	PCOMMON pFrame;
	PCONTROL pc;
	pFrame = CreateFrame( WIDE("Config Board"), 0, 0, 275, 275, BORDER_NORMAL, (PCOMMON)0 );
	MoveFrame( pFrame, 50, 50 );
	pcBoard = MakeControl( pFrame, MimicBoard.TypeID, 23, 23, 150, 150, TXT_STATIC );
	pc = MakeSlider( pFrame, 20, 5, 153, 15, SLD_BOARD_X, SLIDER_HORIZ, SliderUpdatedX, 0 );
	SetSliderValues( pc, 3, BOARD_X, MAX_BOARD_X );
	pc = MakeSlider( pFrame, 5, 20, 15, 153, SLD_BOARD_Y, SLIDER_VERT, SliderUpdatedY, 0 );
	SetSliderValues( pc, 4, BOARD_Y, MAX_BOARD_Y );
	MakeButton( pFrame, 14, 200, 82, 20, BTN_PLAY, WIDE("Done"), 0, PlayGameButton, (PTRSZVAL)&done );
	SetCheckState( MakeCheckButton( pFrame, 131, 200, 80, 14, CHK_ANIMATE, WIDE("Animate"), 0, NULL, 0 ), 1 );
	SetCheckState( MakeCheckButton( pFrame, 131, 220, 120, 14, CHK_SPHERICAL, WIDE("Spherical Wrap"), 0, NULL, 0 ), 1 );
	//SetControlDraw( pcBoard, DrawMimicBoard, 0 );
	//DrawMimicBoard( pcBoard );

	DisplayFrame( pFrame );
	done = FALSE;

	while( !done ) Idle();

	{
		_32 width, height;
		int z;
		GetDisplaySize( &width, &height );
		height -= 30;
		width -= 210;
		for( z = 1; z < 5; z++ )
		{
			if( ( ( width/(pGrid->width/z) ) > BOARD_X ) &&
			    ( ( height/(pGrid->height/z) ) > BOARD_Y ) )
			   break;
		}
		zoom = z;
	}
	if( animate )
		*animate = GetCheckState( GetControl( pFrame, CHK_ANIMATE ) );
	if( sphere )
      *sphere = GetCheckState( GetControl( pFrame, CHK_SPHERICAL ) );
	DestroyFrame( &pFrame );

}
// also need to ConfigureBoard( void )
