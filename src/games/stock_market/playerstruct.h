#ifndef PLAYER_STRUCTURE_DEFINED
#define PLAYER_STRUCTURE_DEFINED
#include <sack_types.h>
#include <colordef.h>

#include "stockstruct.h"
#include "boardstruct.h"


typedef struct archtype_tag {
	TEXTCHAR *colorname;
	CDATA color;
	// maybe info like - shape info...
	struct {
		uint32_t used: 1;

	} flags;
} ARCHTYPE, *PARCHTYPE;

typedef struct player_tag {
	struct {
		uint32_t GoingLeft : 1; // else going right.
		uint32_t bRemote : 1; // player is under remote control
	} flags;
	TEXTCHAR name[32];
	INDEX id;
	PARCHTYPE archtype;
	uint32_t Cash;
   uint32_t MinValue;
	uint32_t NetValue;
	int nHistory;
   int nHistoryAvail;
   uint32_t *History;
	PLIST portfolio; // list of PSTOCKACCOUNTS
   PSTOCKACCOUNT pMeeting;  // which stock meeting we're in
	PSPACE pCurrentSpace;
	PSI_CONTROL pPlayerToken;
   DeclareLink( struct player_tag );
} PLAYER, *PPLAYER;

#endif
//--------------------------------------------------------------------------
// $Log: playerstruct.h,v $
// Revision 1.6  2003/12/03 11:10:11  panther
// Begin remote linking stuff.  Disable dice cup, and don't disable player dialog
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
// Revision 1.1.1.1  2002/10/09 13:21:42  panther
// Initial commit
//
//
