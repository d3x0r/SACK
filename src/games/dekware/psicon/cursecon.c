#include <sack_types.h>
#include <sys/time.h>
#include <curses.h>
#include <signal.h>
#include <stdio.h> // sprintf ?
#define PLUGIN_MODULE
#include "plugin.h"
#define Sleep(n) usleep(n*1000)
//----------------------------------------------------------------------------

#include "consolestruc.h"
#include "keydefs.h"
#include "WinLogic.h"

#if !defined( WIN32 ) && !defined( __QNX__ )
#endif


// only one master copy of this is really needed...

void Lock( PCONSOLE_INFO pmdp )
{
    EnterCriticalSec( &pmdp->Lock );
}

void Unlock( PCONSOLE_INFO pmdp )
{
   LeaveCriticalSec( &pmdp->Lock );
}

//extern int myTypeID;

static int ColorMap[8]={ COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN
                , COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
static int crColorTable[] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

//----------------------------------------------------------------------------

void RenderSeparator( PCONSOLE_INFO pdp, int nStart )
{
   int c;
   if( nStart < 1 )
		return;
   lprintf( "Rendering separator...%d", nStart );
 //  asm("int $3\n");
    attrset( A_BOLD );
    color_set( 0x7, NULL );
    move( nStart-1, 0 );
    addch( '>' );
    for( c = 0; c < pdp->nWidth-2; c++ )
        addch( '-' );
    addch( '<' );
}

//----------------------------------------------------------------------------

void UnregisterWindows( void )
{    
   nocbreak();
    endwin();
   // end input thread also...
}

static int Close( PCONSOLE_INFO pdp )
{
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_rows );
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_cols );
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_cursorx );
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_cursory );
	pdp->common.Close = NULL;
   pdp->common.Write = NULL;
   pdp->common.Read = NULL;
   pdp->common.Type = 0;

	DestroyHistoryRegion( pdp->pHistory );
	DestroyHistoryCursor( pdp->pCursor );
	DestroyHistoryBrowser( pdp->pCurrentDisplay );
   DestroyHistoryBrowser( pdp->pHistoryDisplay );

   nocbreak();
   endwin();
   return 1;
}

int KeystrokePaste( PCONSOLE_INFO pdp )
{
   return 0;
}

//----------------------------------------------------------------------------

int KeyDelete( PCONSOLE_INFO pdp )
{
    DECLTEXT( rubout, "\x7f" );
    return DoStroke( pdp, (PTEXT)&rubout );
}
//----------------------------------------------------------------------------

int StatusField( PSENTIENT ps, PTEXT parameters )
{
   // /status <size> string....
   // if size is positive then it is the next field from the left
   // if size is negative it is meausred from the right.

   return 1;
}

//----------------------------------------------------------------------------

int UpdateStatusBar( PSENTIENT ps, PTEXT parameters )
{

   // no parameters... just calls render status bar - if status bar active
   return 1;
}

static void WinconPrompt( PDATAPATH pdp )
{
    RenderCommandLine( (PCONSOLE_INFO)pdp );
    prompt( ((PCONSOLE_INFO)pdp)->common.Owner );
}

//----------------------------------------------------------------------------

static uintptr_t InputThread( PTHREAD thread )
{
   PCONSOLE_INFO pdp = (PCONSOLE_INFO)thread->param;
    DECLTEXT( key, "                   " );
    struct timeval tvstart, tvnow;
    int delta, bEscape, nseq;
    char sequence[10];
    gettimeofday( &tvstart, NULL );
    key.data.size = 1;
    bEscape = FALSE;
    while( 1 )
    {
        int ch, bOutput;
        ch = getch();
        lprintf(  "Got Key: %d\n", ch );
        if( ch == 127 )
            ch = 8;
        gettimeofday( &tvnow, NULL );
        delta = ( tvnow.tv_sec - tvstart.tv_sec ) * 100000
            + ( tvnow.tv_usec - tvstart.tv_usec ) / 10;
        tvstart = tvnow;
        bOutput = FALSE;
        if( ch != ERR )
        {
            Lock( pdp );
            if( ch < 0x100 )
            {
                if( bEscape )
                {
                    if( delta < 5000 )
                    {
                        sequence[nseq++] = ch;
                        if( ch == 'A' ) // variation up arrow
                        {
                            if( nseq == 3 )
                            {
                                if( sequence[0] == 'O' &&
                                    sequence[1] == '2' )
                                {
                                    HistoryLineUp( pdp->pHistoryDisplay );
                                    bEscape = FALSE;
                                    Unlock( pdp );
                                    continue;
                                }
                                else
                                {
                                    goto clearspam;
                                }
                            }
                            else
                            {
                                goto clearspam;
                            }
                        }
                        else if( ch == 'B' ) // variation down arrow
                        {
                            if( nseq == 3 )
                            {
                                if( sequence[0] == 'O' &&
                                    sequence[1] == '2' )
                                {
                                    HistoryLineDown( pdp->pHistoryDisplay );
                                    bEscape = FALSE;
                                    Unlock( pdp );
                                    continue;
                                }
                                else goto clearspam;
                            }
                            else
                                goto clearspam;
                        }
                        Unlock( pdp );
                        continue;
                    }
                    else
                    {
                        int n;
                    clearspam:
                        bEscape = FALSE;
                        key.data.data[0] = 0x1b;
                        bOutput |= DoStroke( pdp, (PTEXT)&key );
                        for( n = 0; n < nseq; n++ )
                        {
                            key.data.data[0] = sequence[n];
                            bOutput |= DoStroke( pdp, (PTEXT)&key );
                        }
                        Unlock( pdp );
                        continue;
                    }
                }
                else if( ch == 0x1b ) // escape
                {
                    nseq = 0;
                    bEscape = TRUE;
                    Unlock( pdp );
                    continue;
                }
                key.data.data[0] = ch;
                bOutput |= DoStroke( pdp, (PTEXT)&key );
                // normal key? probably...
            bOutput |= 1;
            }
            else
            {
                lprintf( "Key: %d\n", ch );
                switch( ch )
                {
                case KEY_RESIZE:
						 pdp->rArea.left = 0;
											  // COLS comes from a define in nCurses library....
						 pdp->rArea.right = COLS;
						 pdp->rArea.top = 0;
											 // LINES comes from a define in nCurses library....
						 pdp->rArea.bottom = LINES;
                    ChildCalculate( pdp );
                    break;
            case KEY_BACKSPACE:
                    key.data.data[0] = 0x08;
                    bOutput |= DoStroke( pdp, (PTEXT)&key );
                    break;
                case KEY_UP:
                    KeyUp( pdp->common.CommandInfo );
                    break;
            case KEY_DOWN:
                    HandleKeyDown( pdp->common.CommandInfo );
                    break;
                case KEY_LEFT:
                    KeyLeft( pdp->common.CommandInfo );
                    break;
                case KEY_RIGHT:
                    KeyRight( pdp->common.CommandInfo );
                    break;
                case KEY_HOME:
                    KeyHome( pdp->common.CommandInfo );
                    break;
                case KEY_END:
                    KeyEndCmd( pdp->common.CommandInfo );
                    break;
                case KEY_PPAGE:
                    HistoryPageUp( pdp->pHistoryDisplay );
                    break;
                case KEY_NPAGE:
                    HistoryPageDown( pdp->pHistoryDisplay);
                    break;
                case KEY_DC:
                    bOutput |= KeyDelete( pdp );
                    break;
                }
            }
            if( bOutput )
                RenderCommandLine( pdp );

            Unlock( pdp );
        }
	 }
    return 0;
}


//----------------------------------------------------------------------------
int RefreshDisplay( PDATAPATH *pChannel, PSENTIENT ps, PTEXT pargs )
{
	( (PCONSOLE_INFO)*pChannel )->rArea.left = 0;
   // COLS comes from a define in nCurses library....
	( (PCONSOLE_INFO)*pChannel )->rArea.right = COLS;
	( (PCONSOLE_INFO)*pChannel )->rArea.top = 0;
   // LINES comes from a define in nCurses library....
	( (PCONSOLE_INFO)*pChannel )->rArea.bottom = LINES;
	ChildCalculate( (PCONSOLE_INFO)*pChannel );
	return 0;
}
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// methods for window logic routines to use as callbacks...
//----------------------------------------------------------------------------

static void CPROC DrawString( PCONSOLE_INFO pdp, int x, int y, RECT *r, char *s, int nShown, int nShow )
{
	uint32_t dwSize;
	lprintf( "Adding string out : %s %d %d at %d,%d", s, nShown, nShow,x,y,r->left,r->top );
	move( y-1, x );
   addnstr(s+nShown,nShow);
}

//----------------------------------------------------------------------------

static void CPROC SetCurrentColor( PCONSOLE_INFO pdp, enum current_color_type type, PTEXT segment )
{
	int f, b, colorpair;

	switch( type )
	{
	case COLOR_COMMAND:
		f=11;
      b = 0;
		break;
	case COLOR_MARK:
		f = 15;
      b = 1;
		break;
	case COLOR_DEFAULT:
		f = 7;
      b = 0;
		break;
	case COLOR_SEGMENT:
		if( segment )
		{
			f = segment->format.flags.foreground;
			b = segment->format.flags.background;
		}
      break;
	}
	if( f*8)
		attrset( A_BOLD);
	else
		attrset( A_NORMAL );
   colorpair = ( f&7 ) | ( (b&7) << 3 );
	color_set(colorpair,NULL);
}

//----------------------------------------------------------------------------

static void CPROC FillConsoleRect( PCONSOLE_INFO pdp, RECT *r, enum fill_color_type type )
{
	int c; uint32_t dwSize;
	move( r->top-1, r->left );
	//lprintf( "Filling blank line: at %d,%d  %d,%d = %d", r->top, r->left, r->left, r->right, r->right-r->left);
	switch( type )
	{
	case FILL_COMMAND_BACK:
      SetCurrentColor( pdp, COLOR_COMMAND, NULL );
      //FillRect( pdp->wincon.hDC, r, pdp->wincon.hbrBackground );
		break;
	case FILL_DISPLAY_BACK:
      SetCurrentColor( pdp, COLOR_DEFAULT, NULL );
      //FillRect( pdp->wincon.hDC, r, pdp->wincon.hbrCommandBackground );
      break;
	}
	for( c = r->left; c < r->right; c++ )
      addch( ' ' );
}

//----------------------------------------------------------------------------

static void CPROC Update( PCONSOLE_INFO pmdp )
{
   lprintf( " putting new rendered junk back on the screen..." );
	//DoRenderHistory( pmdp, FALSE );
	refresh();
}

static void CPROC RenderCursor( PCONSOLE_INFO pmdp, RECT *r, int column )
{
   if( !column )
		move( r->top-1, 1 );
   else
		move( r->top-1, column );

}
//----------------------------------------------------------------------------



void signal_ignore( int sig )
{
    
} 


PDATAPATH CPROC CreateConsole( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PCONSOLE_INFO pdp;
   static PCONSOLE_INFO pMyPath = NULL;
	if( pMyPath )
	{
      lprintf( "Can noly have one path open." );
		return NULL;
	}

    pdp = CreateDataPath( pChannel, CONSOLE_INFO );
   pMyPath = pdp;
   pdp->common.Owner = ps;
    lprintf( "Initializign curses..." );
    initscr(); // 
    cbreak();  // stop line entering - direct input
    noecho();  // do not echo the input ....
    
    nonl();
    intrflush( stdscr, FALSE );
    keypad( stdscr, TRUE );
   meta( stdscr, TRUE );
    start_color();
    signal( SIGINT, SIG_IGN );
    //scrollok( stdscr, TRUE ); think I can get by without the scroll...
    {
        // remap the colors to PC natural.
        int f, b;
        for( f = 0; f < 8; f++ )
            for( b = 0; b < 8; b++ )
            {
                init_pair( b*8+f, ColorMap[f], ColorMap[b] );
            }
    }
    
   //ps->flags.no_prompt = TRUE;
   pdp->flags.bDirect = FALSE;
   //pdp->common.Type = myTypeID;
   pdp->common.Close = (int (*)( struct datapath_tag *pdp ))Close;
//   pdp->common.Read = NULL;
	pdp->common.Write = WinLogicWrite;

   // this is destroyed when the common closes...
	pdp->common.CommandInfo = CreateCommandHistoryEx( WinconPrompt );

	pdp->pHistory = CreateHistoryRegion();
	pdp->pCursor = CreateHistoryCursor( pdp->pHistory );
	pdp->pCurrentDisplay = CreateHistoryBrowser( pdp->pHistory);
	pdp->pHistoryDisplay = CreateHistoryBrowser( pdp->pHistory );


   AddVolatileVariable( ps->Current, &vve_rows, (uintptr_t)pdp );
   AddVolatileVariable( ps->Current, &vve_cols, (uintptr_t)pdp );
   AddVolatileVariable( ps->Current, &vve_cursorx, (uintptr_t)pdp );
   AddVolatileVariable( ps->Current, &vve_cursory, (uintptr_t)pdp );

   pdp->nXPad = 0;
   pdp->nYPad = 0;
   pdp->nCmdLinePad = 0;

   pdp->nFontHeight = 1;
   pdp->nFontWidth = 1;
	pdp->cursecon.crCommand = 7;
	pdp->cursecon.crCommandBackground = 0;
	pdp->cursecon.crMark = 15;
	pdp->cursecon.crMarkBackground = 1;
   // 50% history...
	pdp->nHistoryPercent = 1;
    //InitHistory( &pdp->History, GetName( pdp->ps->Current) );

	pdp->FillConsoleRect = FillConsoleRect;
	pdp->RenderSeparator = RenderSeparator;
	pdp->DrawString = DrawString;
	pdp->KeystrokePaste = KeystrokePaste;
	pdp->SetCurrentColor = SetCurrentColor;
	pdp->RenderCursor = RenderCursor;
   pdp->Update = Update;

	pdp->rArea.left = 0;
   // COLS comes from a define in nCurses library....
	pdp->rArea.right = COLS;
	pdp->rArea.top = 0;
   // LINES comes from a define in nCurses library....
	pdp->rArea.bottom = LINES;
	ChildCalculate(pdp);

   lprintf( "Starting input thread..." );
   ThreadTo( InputThread, (uintptr_t)pdp );

   return (PDATAPATH)pdp;
}


// $Log: cursecon.c,v $
// Revision 1.23  2005/08/08 15:24:11  d3x0r
// Move updated rectangle struct to common space.  Improved curses console handling....
//
// Revision 1.22  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.21  2005/04/18 23:23:01  d3x0r
// Quick hack on cursecon to make it build.
//
// Revision 1.20  2005/01/26 20:00:01  d3x0r
// Okay - need to do something about partial updates - such as command typing should only update that affected area of the screen...
//
// Revision 1.19  2005/01/23 10:09:57  d3x0r
// cursecon might even work or resemble working..
//
// Revision 1.18  2005/01/23 04:07:57  d3x0r
// Hmm somehow between display rendering stopped working.
//
// Revision 1.17  2005/01/18 02:47:02  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.16  2005/01/17 07:58:23  d3x0r
// Rename write to avoid conflict with other libraries
//
// Revision 1.15  2004/09/24 09:04:23  d3x0r
// Cleanup and polishing for linux...
//
// Revision 1.14  2004/09/10 08:48:49  d3x0r
// progress not perfection... a grand dawning on the horizon though.
//
// Revision 1.13  2004/07/30 14:08:40  d3x0r
// More tinkering...
//
// Revision 1.12  2004/06/12 23:57:14  d3x0r
// Hmm some strange issues left... just need to simply code now... a lot of redunancy is left...
//
// Revision 1.11  2004/06/12 09:46:32  d3x0r
// Linux first pass fixes to compile
//
// Revision 1.10  2004/05/04 07:15:01  d3x0r
// Fix hinstance, implement relay segments out, clean, straighten and shape up
//
// Revision 1.9  2004/04/06 01:50:31  d3x0r
// Update to standardize device options and the processing thereof.
//
// Revision 1.8  2004/04/05 05:43:04  d3x0r
// Fix command line drawing, untested in console modes, works in gui modes
//
// Revision 1.7  2004/03/24 19:24:55  d3x0r
// Progress on fixing console con...
//
// Revision 1.6  2004/03/08 09:25:42  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.5  2004/01/21 08:50:53  d3x0r
// And again it compiles
//
// Revision 1.4  2004/01/21 08:36:24  d3x0r
// Added renderseparator to console, supply fix for cursecon
//
// Revision 1.3  2004/01/21 08:30:24  d3x0r
// Fix some lost changes to cursecon
//
// Revision 1.1  2004/01/20 04:53:52  d3x0r
// Checked in?
//
// Revision 1.32  2004/01/19 15:04:01  panther
// The magic number is 255 not -1
//
// Revision 1.31  2004/01/19 14:15:53  panther
// Mods for cursecon to work.
//
// Revision 1.30  2004/01/19 02:38:51  panther
// Fix calculation of left/right relative to non graphics
//
// Revision 1.29  2004/01/19 02:15:42  panther
// Remove more usless code,comments
//
// Revision 1.5  2004/01/18 22:35:56  panther
// Fixed history up through cursecon..
//
// Revision 1.4  2004/01/18 22:27:04  panther
// fixcommandcursor migrated..>
//
// Revision 1.3  2004/01/18 22:16:41  panther
// More cuts from cursecon
//
// Revision 1.14  2003/10/26 12:38:16  panther
// Updates for newest scheduler
//
// Revision 1.13  2003/03/25 08:59:01  panther
// Added CVS logging
//
