#define DEFINES_INTERSHELL_INTERFACE
#define GLOBAL_SOURCE
#include "global.h"

PRIORITY_PRELOAD( InitGlobal, DEFAULT_PRELOAD_PRIORITY - 5 )
{
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

	g.blank = LoadImageFile( "blankimage.jpg");
	g.playagain=LoadImageFile( "playagain.jpg");
	g.playing  =LoadImageFile( "playing.jpg");
	g.background = LoadImageFile( "background.jpg" );
	g.strip = LoadImageFile( "slot_strip.jpg" );
	g.nReels = NUM_REELS;

}
