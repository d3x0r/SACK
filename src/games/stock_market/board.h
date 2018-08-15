#ifndef BOARD_FUNCTIONS_DEFINED
#define BOARD_FUNCTIONS_DEFINED

void ReadBoardDefinitions( TEXTCHAR *filename );

void SetPlayerSpace( PSPACE pSpace, PPLAYER player );
PSPACE ChooseProfessionSpace( PPLAYER player );

PSPACE ChooseStartSpace( void );

void ChoosePlayerDestination( void );
void PayWageSlaves( void );
void MountPanel( PSI_CONTROL frame );
void UnmountPanel( PSI_CONTROL frame );
void AddSellToPossible( void );
void AddRollToPossible( void );

void StartFlash( void );
void StopFlash( void );

void ProcessCurrentSpace( void );

#endif
//--------------------------------------------------------------------------
// $Log: board.h,v $
// Revision 1.8  2004/12/15 18:48:44  panther
// Minor updates to latest SACK system...
//
// Revision 1.7  2004/02/12 23:19:43  panther
// Portability mods
//
// Revision 1.6  2003/11/30 08:15:49  panther
// Okay - all is well done.  Todo: slide stock prices, scale, shade columns, undo/cancel,...
//
// Revision 1.5  2003/11/29 14:49:43  panther
// Implemented player status panel...
//
// Revision 1.4  2003/11/29 04:27:18  panther
// Buy, dice dialogs done.  Fixed min sell. Left : player stat, sell
//
// Revision 1.3  2003/11/28 20:57:00  panther
// Almost done - just checkpoint in case of bad things
//
// Revision 1.2  2003/11/28 05:20:44  panther
// Invoke some motion on the board, fix stock paths, implement much - all text
//
// Revision 1.1.1.1  2002/10/09 13:21:40  panther
// Initial commit
//
//
