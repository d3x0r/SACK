#include <stdio.h>
#include <stdio.h>
#include <controls.h>
#include "resources.h"

#include "local.h"

PCONTROL frame;


// list extern procs here

void CPROC GetUserButton(PTRSZVAL psv, PCONTROL pc);
void CPROC KillRelayButton(PTRSZVAL psv, PCONTROL pc);
void CPROC RebootUserButton(PTRSZVAL psv, PCONTROL pc);
void CPROC KillUserButton(PTRSZVAL psv, PCONTROL pc);
void CPROC ScanUserButton(PTRSZVAL psv, PCONTROL pc);
void CPROC UpdateUserButton(PTRSZVAL psv, PCONTROL pc);
void CPROC ScanAllButton(PTRSZVAL psv, PCONTROL pc);
void CPROC UpdateAllButton(PTRSZVAL psv, PCONTROL pc);
void CPROC GetMasterStatusButton(PTRSZVAL psv, PCONTROL pc);
void CPROC UpdateAllButton(PTRSZVAL psv, PCONTROL pc);
void CPROC RequestWinnersButton( PTRSZVAL psv, PCONTROL pc );

#define COLUMN1 338
#define COLUMN2 435

void DoController( void )
{
	int done = 0, okay = 0;
	TEXTCHAR defaultaddress[128];
	{
		FILE *file = sack_fopen( 0, WIDE( "LastConnection.data" ), WIDE( "rb" ) );
		if( file )
		{
			fgets( defaultaddress, 128, file );
   			fclose( file );
   		}
   		else
   			StrCpyEx( defaultaddress, WIDE("127.0.0.1"), sizeof( defaultaddress ) / sizeof( defaultaddress[0] ) );
   }

	frame = CreateFrame( WIDE( "Relay Control" ), 0, 0, 530, 256, 0, (0) );
	MakeListBox( frame, 5, 5, 328, 120, LST_USERS, 0 );
	MakeListBox( frame, 5, 130, 164, 120, LST_WINNERS, 0 );
	MakeButton( frame, 172, 130, 95, 23, BTN_WINNERS, WIDE( "Get Winners" ), 0, RequestWinnersButton, 0 );

	MakeEditControl( frame, COLUMN1, 5, 300, 16, EDT_SERVER, defaultaddress, 0 );
	MakeButton( frame, COLUMN1,  26, 95, 23, BTN_USERS, WIDE( "Get Users" ), 0, GetUserButton, 0 );
	EnableControl( MakeButton( frame, COLUMN1,  51, 95, 23, BTN_KILL, WIDE( "End Relay" ), 0, KillRelayButton, 0 ), FALSE );
	MakeButton( frame, COLUMN1,  76, 95, 23, BTN_SCAN, WIDE( "Scan User" ), 0, ScanUserButton, 0 );
	MakeButton( frame, COLUMN2,  76, 95, 23, BTN_SCANALL , WIDE( "Scan All" )     , 0, ScanAllButton, 0 );
	MakeButton( frame, COLUMN1, 101, 95, 23, BTN_UPDATE  , WIDE( "Update User" )  , 0, UpdateUserButton, 0 );
	MakeButton( frame, COLUMN2, 101, 95, 23, BTN_UPDATEALL, WIDE( "Update All" )  , 0, UpdateAllButton, 0 );
	MakeButton( frame, COLUMN1, 126, 95, 23, BTN_KILLUSER , WIDE( "Kill User" )   , 0, KillUserButton, 0 );
	MakeButton( frame, COLUMN1, 151, 95, 23, BTN_REBOOT   , WIDE( "Reboot User" ) , 0, RebootUserButton, 0 );
	MakeButton( frame, COLUMN1, 176, 95, 23, BTN_STATUS   , WIDE( "Get Status" )  , 0, GetMasterStatusButton, 0 );
	MakeTextControl( frame, COLUMN1, 201, 180, 16, CHK_MASTER, WIDE( "Game Master is ..." ), TEXT_NORMAL );
   
	AddCommonButtons( frame, &done, &okay );
	DisplayFrame( frame );
	CommonLoop( &done, &okay );
}
// Checked in by: $Author: jim $
// $Revision: 1.9 $
// $Log: guiremote.c,v $
// Revision 1.9  2003/05/06 00:00:45  jim
// Update for new psi control interface
//
// Revision 1.8  2003/02/19 00:29:46  jim
// Port to newest libraries and makesystem
//
// Revision 1.7  2002/05/22 18:58:54  panther
// *** empty log message ***
//
// Revision 1.6  2002/05/20 19:31:12  panther
// Modified the PSI and reversed ID and TEXT on button controls
//
// Revision 1.5  2002/05/03 15:43:11  panther
// Updates to get winners report from relay master.
//
// Revision 1.4  2002/04/29 15:07:10  panther
// Fixed game master present/absent status control to something more
// usable than that stupid check box.
//
// Revision 1.3  2002/04/23 16:45:38  panther
// *** empty log message ***
//
// Revision 1.2  2002/04/15 16:26:39  panther
// Added revision tags.
//
