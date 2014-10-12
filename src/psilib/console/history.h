#ifndef console_history_defined
#define console_history_defined

//#define MAX_HISTORY 25000
//#define MAX_HISTORY_LINES 100
//#define MAX_HISTORY_BLOCKS (MAX_HISTORY/MAX_HISTORY_LINES)

//#include "histstruct.h"
#include <psi/console.h>

PSI_CONSOLE_NAMESPACE

#ifdef __cplusplus
   //sack::psi::console::n
#define STRUC_PREFIX(n) n
#else
#define STRUC_PREFIX(n) n
#endif

typedef void (CPROC *MeasureString )( PTRSZVAL /*PCONSOLE_INFO*/ console, CTEXTSTR s, int nShow, _32 *w, _32 *h, SFTFont font );
	
typedef struct STRUC_PREFIX(history_line_tag) TEXTLINE, *PTEXTLINE;
typedef struct STRUC_PREFIX(history_block_link_tag) HISTORY_BLOCK_LINK, *PHISTORY_BLOCK_LINK;
typedef struct STRUC_PREFIX(history_block_tag) HISTORYBLOCK, *PHISTORYBLOCK;
typedef struct STRUC_PREFIX(history_region_tag)  HISTORY_REGION, *PHISTORY_REGION;
typedef struct STRUC_PREFIX(displayed_line_info_tag)  DISPLAYED_LINE, *PDISPLAYED_LINE;
typedef struct STRUC_PREFIX(history_browser_cursor_tag) HISTORY_BROWSER, *PHISTORY_BROWSER;
typedef struct STRUC_PREFIX(history_line_cursor_tag) HISTORY_LINE_CURSOR, *PHISTORY_LINE_CURSOR;
typedef struct STRUC_PREFIX(history_tracking_tag) HISTORYTRACK, *PHISTORYTRACK;
typedef struct STRUC_PREFIX(history_bios_tag) HISTORY_BIOS, *PHISTORY_BIOS;




CORECON_PROC( PHISTORY_REGION, PSI_CreateHistoryRegion )( void );
CORECON_PROC( PHISTORY_LINE_CURSOR, PSI_CreateHistoryCursor )( PHISTORY_REGION );
CORECON_PROC( PHISTORY_BROWSER, PSI_CreateHistoryBrowser )( PHISTORY_REGION, MeasureString, PTRSZVAL );
CORECON_PROC( void, PSI_DestroyHistoryRegion )( PHISTORY_REGION );
CORECON_PROC( void, PSI_DestroyHistoryCursor )( PHISTORY_LINE_CURSOR );
CORECON_PROC( void, PSI_DestroyHistoryBrowser )( PHISTORY_BROWSER );

CORECON_PROC( void, PSI_DestroyHistory )( PHISTORYTRACK pht );
CORECON_PROC( PSI_Console_Phrase, PSI_EnqueDisplayHistory )( PHISTORY_LINE_CURSOR phc, PTEXT pLine );
CORECON_PROC( int, PSI_GetLastLineLength )( PHISTORY_REGION pht );
CORECON_PROC( void, PSI_InitHistory )( PHISTORY_REGION pht, PTEXT name );
//void CalculateHistory( PHISTORYTRACK pht, int columns, int lines );

CORECON_PROC( void, PSI_SetHistoryBrowserNoPageBreak )( PHISTORY_BROWSER phbr );
CORECON_PROC( void, PSI_SetHistoryBrowserOwnPageBreak )( PHISTORY_BROWSER phbr );


// get a line - enumerate - start start == 0;
// this routine will auto step start++...
CORECON_PROC( PTEXT, PSI_EnumHistoryLine )( PHISTORY_BROWSER pht
							, int *offset
							, S_32 *length );

CORECON_PROC( void, PSI_SetHistoryDefaultForeground )( PHISTORY_LINE_CURSOR phc, int iColor );
CORECON_PROC( void, PSI_SetHistoryDefaultBackground )( PHISTORY_LINE_CURSOR phc, int iColor );

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

int GetCommandCursor( PHISTORY_BROWSER phbr
					 , SFTFont font
						  , PUSER_INPUT_BUFFER CommandInfo
						  , int bEndOfStream
						, int bWrapCommand
						  , int *command_offset
						  , int *command_begin
						  , int *command_end
						  , int *line_offset
						  );
// Set line position
void SetCursorLine( PHISTORY_LINE_CURSOR cursor, int nLine );
// set total number of lines...
void SetCursorLines( PHISTORY_LINE_CURSOR cursor, int nLines );
CORECON_PROC( void, SetBrowserLines )( PHISTORY_BROWSER cursor, int nLines );

// get column position
int GetCursorColumn( PHISTORY_LINE_CURSOR cursor );
// set column position
void SetCursorColumn( PHISTORY_LINE_CURSOR cursor, int nColumn );
// set total columns available...
void SetCursorColumns( PHISTORY_LINE_CURSOR cursor, int nColumns );
void SetBrowserColumns( PHISTORY_BROWSER cursor, int nColumns, int size );

void SetCursorNoPrompt( PHISTORY_BROWSER phbr, LOGICAL bNoPrompt );

// computation of line is computed from the cursor's columns.
// amount +/- 1 == lineup down
// amount +/- N == pageup down
int MoveHistoryCursor( PHISTORY_BROWSER phbr, int amount );

_32 ComputeNextOffset( PTEXT segment, _32 nShown );
//int ComputeToShow( _32 cols, PTEXT segment, int nOfs, int nShown );
_32 ComputeToShow( _32 colsize, _32 *col_offset, PTEXT segment, _32 nLen, _32 nOfs, _32 nShown, PHISTORY_BROWSER phbr, SFTFont font );
int CountLinesSpanned( PHISTORY_BROWSER phb, PTEXT countseg, SFTFont font );

CORECON_PROC( void, BuildDisplayInfoLines )( PHISTORY_BROWSER phlc, SFTFont font );


#ifdef __DEKWARE_PLUGIN__
// this takes into account typing a command, and recording a macro plus the dekware object prmopt...
int GetCommandCursor( PHISTORY_BROWSER phbr
						  , COMMAND_INFO *CommandInfo
						  , int bEndOfStream
                    , int *command_offset
						  , int *command_begin
                    , int *command_end
						  );
#endif
CORECON_PROC( PDATALIST *,GetDisplayInfo )( PHISTORY_BROWSER phbr );
void GetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, PS_32 x, PS_32 y );
void SetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, S_32 x, S_32 y );
void PSI_SetHistoryPageLines( PHISTORY_BROWSER phbr, _32 nLines );
_32 GetBrowserDistance( PHISTORY_BROWSER phbr, SFTFont font );
void ResetHistoryBrowser( PHISTORY_BROWSER phbr );
CORECON_PROC( int, CountDisplayedLines) ( PHISTORY_BROWSER phbr );


void SetHistoryBackingFile( PHISTORY_REGION phr, FILE *file );


//void SetHistoryBrowserOwnPageBreak( PHISTORY_BROWSER phbr );
//void SetHistoryBrowserNoPageBreak( PHISTORY_BROWSER phbr );
PSI_CONSOLE_NAMESPACE_END


#endif

