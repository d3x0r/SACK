
#include "global.h"

#define BOARD_SIZE 
#define SPACE_SIZE (BOARD_SIZE/12)

// okay confusion but let's for for the linux compat, full screen driver
// AND virtual window manager - woo



void DisplayPlayers( void )
{
}

void DisplayBoard( void )
{
    
}

void InitGameDisplay( void )
{
    //g.renderer = OpenDisplaySizedAt( 0, 800, 600, 0, 0 );
   //g.display = CreateFrameFromRenderer( "Stock Market", BORDER_NONE, g.renderer );
    g.scale = 28; // assuming 800x600 at least.

    //g.board = CreateFrame( "Game track"
    //                        , 0, 0
    //                          , 600, 600
    //                     , BORDER_INVERT|BORDER_NOMOVE
    //                            , g.display);

    // whatever the display is...
    /*
     // but this is an alpha beast and we need to find the
    // right magic to make it appropriately alpha...
    g.graph = CreateFrame( NULL
                              , 0, 0
                              , 0, 0
                              , BORDER_INVERT|BORDER_NOMOVE
                              , g.display);
    */

}
