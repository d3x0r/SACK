#ifndef BOARD_STRUCTURE_DEFINED
#define BOARD_STRUCTURE_DEFINED
#include <sack_types.h>
#include <fractions.h>
#include "global.h"
#include "stockstruct.h"

typedef enum space_type {
	SPACE_UNKNOWN = 0
								, SPACE_PROFESSION
								, SPACE_START
								, SPACE_BROKER
								, SPACE_STOCKSELL
								, SPACE_HOLDERSMEETING
								, SPACE_STOCKBUY
								, SPACE_HOLDERSENTRANCE
								, SPACE_QUIT
								, SPACE_ROLL
								, SPACE_SELL
} SPACE_TYPE;


// for ease - the paths through stockholders meetings are mono-directional.
// therefore they are spacially equivalent, but different...

typedef struct space_tag {
   // these are indexes into g.board list.
	INDEX left, right, alternate;
	_32 ID;
	struct {
		_8 width;
		_8 height;
		_8 x;
		_8 y;
	} position;
	enum space_type type;
	struct {
      _32 bMoveLeft : 1;
      _32 bAlternateLeft : 1;
      _32 bVertical : 1;
		_32 bInvert : 1;
		_32 bFlashing : 1;
		_32 bFlashOn : 1;
	} flags;
	S_16 FixedStageAdjust;
   FRACTION Split;  // 1:1, 2:1, 3:1 7:5 (how many per one)
	union {
		struct {
			_32 cost;
			struct {
				_32 bEvenRight : 1; // odd left; otherwise odd right, even left
			} flags;
      } start;
      struct {
         FRACTION ratio;
      } split;
		struct {
			TEXTCHAR *name;
			FLAGSET( payon, 12 );
			_32 pay;
         CDATA color;
		} profession;
      struct {
         PSTOCK stock;
		} buy_sell;
		struct {
         _32 fee;
		} broker;
	} attributes;
	PCONTROL region;
   struct player_tag *pPlayers; // players on this space...
} SPACE, *PSPACE;

#endif
//--------------------------------------------------------------------------
// $Log: boardstruct.h,v $
// Revision 1.7  2005/03/14 11:29:02  panther
// Corrected Flash space... need to do the flashing int he process of updating the space.
//
// Revision 1.6  2004/02/12 23:19:43  panther
// Portability mods
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
