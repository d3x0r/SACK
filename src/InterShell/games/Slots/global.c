#define DEFINES_INTERSHELL_INTERFACE
#define GLOBAL_SOURCE
#include "global.h"

PRIORITY_PRELOAD( InitGlobal, DEFAULT_PRELOAD_PRIORITY - 5 )
{
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

	g.blank = LoadImageFile( WIDE("blankimage.jpg"));
	g.playagain=LoadImageFile( WIDE("playagain.jpg"));
	g.playing  =LoadImageFile( WIDE("playing.jpg"));
	g.background = LoadImageFile( WIDE("background.jpg") );
	g.strip = LoadImageFile( WIDE("slot_strip.jpg") );
	g.nReels = NUM_REELS;

}
