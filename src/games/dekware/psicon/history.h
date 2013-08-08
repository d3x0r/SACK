#ifndef console_history_defined
#define console_history_defined

//#define MAX_HISTORY 25000
//#define MAX_HISTORY_LINES 100
//#define MAX_HISTORY_BLOCKS (MAX_HISTORY/MAX_HISTORY_LINES)

#ifdef __DEKWARE_PLUGIN__
#include <datapath.h>
#include <space.h>
#endif
#include "histstruct.h"

CORECON_PROC( PHISTORY_REGION, CreateHistoryRegion )( void );
CORECON_PROC( PHISTORY_LINE_CURSOR, CreateHistoryCursor )( PHISTORY_REGION );
CORECON_PROC( PHISTORY_BROWSER, CreateHistoryBrowser )( PHISTORY_REGION );
CORECON_PROC( void, DestroyHistoryRegion )( PHISTORY_REGION );
CORECON_PROC( void, DestroyHistoryCursor )( PHISTORY_LINE_CURSOR );
CORECON_PROC( void, DestroyHistoryBrowser )( PHISTORY_BROWSER );

CORECON_PROC( void, DestroyHistory )( PHISTORYTRACK pht );
CORECON_PROC( void, EnqueDisplayHistory )( PHISTORY_LINE_CURSOR phc, PTEXT pLine );
CORECON_PROC( int, GetLastLineLength )( PHISTORY_REGION pht );
CORECON_PROC( void, InitHistory )( PHISTORY_REGION pht, PTEXT name );
//void CalculateHistory( PHISTORYTRACK pht, int columns, int lines );

// standard interface functions for dekware script
#ifdef __DEKWARE_PLUGIN__
CORECON_PROC( int, ConSetColor )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
CORECON_PROC( int, HistoryStat )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
CORECON_PROC( int, SetTabs )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
CORECON_PROC( int, SetHistory )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
CORECON_PROC( int, DumpHistory )( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
#endif

CORECON_PROC( void, SetHistoryBrowserNoPageBreak )( PHISTORY_BROWSER phbr );
CORECON_PROC( void, SetHistoryBrowserOwnPageBreak )( PHISTORY_BROWSER phbr );



CORECON_PROC( void, WriteHistoryToFile )( FILE *file, PHISTORY_REGION phr );

// get a line - enumerate - start start == 0;
// this routine will auto step start++...
CORECON_PROC( PTEXT, EnumHistoryLine )( PHISTORY_BROWSER pht
							, int *offset
							, S_32 *length );

CORECON_PROC( void, SetHistoryDefaultForeground )( PHISTORY_LINE_CURSOR phc, int iColor );
CORECON_PROC( void, SetHistoryDefaultBackground )( PHISTORY_LINE_CURSOR phc, int iColor );

//int BeginHistory( PHISTORY_LINE_CURSOR cursor );
// adjust history by offset...
int AlignHistory( PHISTORY_BROWSER cursor, S_32 nOffset );
int HistoryNearEnd( PHISTORY_BROWSER cursor, int nLines );
// this takes into account typing a command, and recording a macro plus the dekware object prmopt...
int FixCommandCursor( PHISTORY_BROWSER pht, PUSER_INPUT_BUFFER CommandInfo
#ifdef __DEKWARE_PLUGIN__
						  , PMACRO pRecord
#endif
						  , int bEndOfStream );

int GetCursorLine( PHISTORY_LINE_CURSOR cursor );
// Set line position
void SetCursorLine( PHISTORY_LINE_CURSOR cursor, int nLine );
// set total number of lines...
void SetCursorLines( PHISTORY_LINE_CURSOR cursor, int nLines );
void SetBrowserLines( PHISTORY_BROWSER cursor, int nLines );

// get column position
int GetCursorColumn( PHISTORY_LINE_CURSOR cursor );
// set column position
void SetCursorColumn( PHISTORY_LINE_CURSOR cursor, int nColumn );
// set total columns available...
void SetCursorColumns( PHISTORY_LINE_CURSOR cursor, int nColumns );
void SetBrowserColumns( PHISTORY_BROWSER cursor, int nColumns );

void SetCursorNoPrompt( PHISTORY_BROWSER phbr, LOGICAL bNoPrompt );

// computation of line is computed from the cursor's columns.
// amount +/- 1 == lineup down
// amount +/- N == pageup down
int MoveHistoryCursor( PHISTORY_BROWSER phbr, int amount );

_32 ComputeNextOffset( PTEXT segment, _32 nShown );
//int ComputeToShow( _32 cols, PTEXT segment, int nOfs, int nShown );
int ComputeToShow( _32 cols, PTEXT segment, _32 nLen, int nOfs, int nShown );
int CountLinesSpanned( _32 cols, PTEXT countseg );

void BuildDisplayInfoLines( PHISTORY_BROWSER phlc );


#ifdef __DEKWARE_PLUGIN__
// this takes into account typing a command, and recording a macro plus the dekware object prmopt...
int GetCommandCursor( PHISTORY_BROWSER phbr
						  , PCOMMAND_INFO CommandInfo
						  , int bEndOfStream
                    , size_t *command_offset
						  , size_t *command_begin
                    , size_t *command_end
						  );
#endif
PDATALIST *GetDisplayInfo( PHISTORY_BROWSER phbr );
void GetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, PS_32 x, PS_32 y );
void SetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, S_32 x, S_32 y );
void SetHistoryPageLines( PHISTORY_BROWSER phbr, _32 nLines );
_32 GetBrowserDistance( PHISTORY_BROWSER phbr );
void ResetHistoryBrowser( PHISTORY_BROWSER phbr );

//void SetHistoryBrowserOwnPageBreak( PHISTORY_BROWSER phbr );
//void SetHistoryBrowserNoPageBreak( PHISTORY_BROWSER phbr );


#endif

// $Log: history.h,v $
// Revision 1.31  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.30  2005/01/20 06:10:19  d3x0r
// One down, 3 to convert... concore library should serve to encapsulate drawing logic and history code...
//
// Revision 1.29  2004/09/29 09:31:32  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.28  2004/09/27 16:06:17  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.27  2004/09/23 09:53:33  d3x0r
// Looks like it's in a usable state for windows... perhaps an alpha/beta to be released soon.
//
// Revision 1.26  2004/09/10 08:48:50  d3x0r
// progress not perfection... a grand dawning on the horizon though.
//
// Revision 1.25  2004/09/09 13:41:04  d3x0r
// works much better passing correct structures...
//
// Revision 1.24  2004/07/30 14:08:40  d3x0r
// More tinkering...
//
// Revision 1.23  2004/06/12 23:57:14  d3x0r
// Hmm some strange issues left... just need to simply code now... a lot of redunancy is left...
//
// Revision 1.22  2004/06/12 08:42:34  d3x0r
// Well it initializes, first character causes failure... all windows targets build...
//
// Revision 1.21  2004/06/12 01:24:10  d3x0r
// History cursors are being seperated into browse types and output types...
//
// Revision 1.20  2004/06/10 22:11:00  d3x0r
// more progress...
//
// Revision 1.19  2004/06/10 10:01:45  d3x0r
// Okay so cursors have input and output characteristics... history and display are run by seperate cursors.
//
// Revision 1.18  2004/06/10 00:41:49  d3x0r
// Progress..
//
// Revision 1.17  2004/06/07 15:19:54  d3x0r
// Done with history up to key-associated path (pHistoryShow)
//
// Revision 1.16  2004/06/07 08:24:43  d3x0r
// progress on weeding out structures
//
// Revision 1.15  2004/05/14 16:58:36  d3x0r
// More mangling of structures...
//
// Revision 1.14  2004/05/13 23:35:07  d3x0r
// checkpoint
//
// Revision 1.13  2004/05/13 23:18:17  d3x0r
// Seperating of history and redinrg code...
//
// Revision 1.12  2004/05/04 07:15:01  d3x0r
// Fix hinstance, implement relay segments out, clean, straighten and shape up
//
// Revision 1.11  2004/03/08 09:25:42  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.10  2004/01/19 23:42:26  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.11  2004/01/18 22:35:56  panther
// Fixed history up through cursecon..
//
// Revision 1.10  2004/01/18 22:27:04  panther
// fixcommandcursor migrated..>
//
// Revision 1.9  2003/04/20 01:20:13  panther
// Updates and fixes to window-like console.  History, window logic
//
// Revision 1.9  2003/04/17 06:40:33  panther
// More merging of wincon/psicon.  Should nearly have the renderer totally seperate now from the logic
//
// Revision 1.8  2003/04/15 06:14:53  panther
// Fixed history, fixed history recalling... generalized most logic of a display.
//
// Revision 1.7  2003/04/13 22:28:51  panther
// Seperate computation of lines and rendering... mark/copy works (wincon)
//
// Revision 1.6  2003/04/12 20:51:19  panther
// Updates for new window handling module - macro usage easer...
//
// Revision 1.6  2003/04/10 00:30:39  panther
// Implement setting the cursor remote.  Implement history header/trailer... fix to setvoltilevarible
//
// Revision 1.5  2003/04/08 06:46:40  panther
// update dekware - fix a bit of ansi handling
//
// Revision 1.5  2003/04/06 09:58:07  panther
// Handle position information - enque in right place, overwrite exisiting info
//
// Revision 1.4  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.3  2003/03/26 02:05:21  panther
// begin updating option handlers to real option handlers
//
// Revision 1.2  2003/03/25 08:59:02  panther
// Added CVS logging
//
