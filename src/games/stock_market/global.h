#ifndef GLOBAL_STRUCTURE_DEFINED
#define GLOBAL_STRUCTURE_DEFINED

#define USE_RENDER_INTERACE g.pRend
#define USE_IMAGE_INTERFACE g.pImg
#include <render.h>
#include <image.h>
#include <controls.h>

#include "stockstruct.h"
#include "boardstruct.h"
#include "playerstruct.h"

typedef struct {
	PIMAGE_INTERFACE pImg;
   PRENDER_INTERFACE pRend;
	struct {
		uint32_t bFlashOn : 1; // flash state (all spaces)
		uint32_t bFlashing : 1; // set to flash possible spaces
		uint32_t bFlashed : 1; // spaces have been flashed...
		uint32_t bChoiceNeedsEnter : 1;
		uint32_t bSelectPlayer : 1;
		uint32_t bAllowSell : 1;
		uint32_t bRandomRoll : 1;
	} flags;
	MARKET Market;
   PLIST Board;
   INDEX iCurrentPlayer;
   PLIST Players;
	PPLAYER pCurrentPlayer;
	int nCurrentRoll;
   PLIST PossibleSpaces;
   PRENDERER renderer;
	PCOMMON display;
	PCOMMON board;
	PCOMMON Player;
	PCOMMON graph;
	PCONTROL Panel; // dialog on board to mount panels
   uint32_t PanelWidth, PanelHeight;
	PCOMMON Mounted; // current dialog mounted on panel.
   PCOMMON _Mounted; // prior dialog mounted...
	PCOMMON BuyStocks; // buy stock mountable dialog
	PCOMMON SellStocks;
	PCOMMON RollDice;
	PCONTROL pStockBar;
   PLIST PossiblePlayers;
	uint32_t scale;
   uint32_t nFlashTimer;
} GLOBAL;

enum panel_ids{
	PANEL_PLAYER = 1000
				  , PANEL_BUY
				  , PANEL_SELL
				  , PANEL_ROLL
              , PANEL_GRAPH
};

#ifndef GLOBAL_STRUCTURE_DECLARTION
extern char *colors[];
extern GLOBAL g;
#endif

#endif
//--------------------------------------------------------------------------
// $Log: global.h,v $
// Revision 1.10  2004/12/15 18:48:44  panther
// Minor updates to latest SACK system...
//
// Revision 1.9  2004/02/12 23:19:43  panther
// Portability mods
//
// Revision 1.8  2003/12/02 07:14:44  panther
// Hmm problems with embedded sheets now...
//
// Revision 1.7  2003/11/30 08:15:49  panther
// Okay - all is well done.  Todo: slide stock prices, scale, shade columns, undo/cancel,...
//
// Revision 1.6  2003/11/29 14:49:43  panther
// Implemented player status panel...
//
// Revision 1.5  2003/11/29 04:27:18  panther
// Buy, dice dialogs done.  Fixed min sell. Left : player stat, sell
//
// Revision 1.4  2003/11/28 20:57:00  panther
// Almost done - just checkpoint in case of bad things
//
// Revision 1.3  2003/11/28 05:20:44  panther
// Invoke some motion on the board, fix stock paths, implement much - all text
//
// Revision 1.2  2003/06/25 16:21:09  panther
// Okay updated to newest display library... instead of meta Region library I was making
//
// Revision 1.1.1.1  2002/10/09 13:21:40  panther
// Initial commit
//
//
