#ifndef console_history_defined
#define console_history_defined

//#define MAX_HISTORY 25000
//#define MAX_HISTORY_LINES 100
//#define MAX_HISTORY_BLOCKS (MAX_HISTORY/MAX_HISTORY_LINES)

//#include "histstruct.h"
#include <psi/console.h>

PSI_CONSOLE_NAMESPACE

#define STRUC_PREFIX(n) n

typedef void (CPROC *MeasureString )( uintptr_t /*PCONSOLE_INFO*/ console, CTEXTSTR s, int nShow, uint32_t *w, uint32_t *h, SFTFont font );
	
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
CORECON_PROC( PHISTORY_BROWSER, PSI_CreateHistoryBrowser )( PHISTORY_REGION, MeasureString, uintptr_t );
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
							, int32_t *length );

CORECON_PROC( void, PSI_SetHistoryDefaultForeground )( PHISTORY_LINE_CURSOR phc, int iColor );
CORECON_PROC( void, PSI_SetHistoryDefaultBackground )( PHISTORY_LINE_CURSOR phc, int iColor );

//int BeginHistory( PHISTORY_LINE_CURSOR cursor );
// adjust history by offset...
int AlignHistory( PHISTORY_BROWSER cursor, int32_t nOffset, SFTFont font );
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
						  , int *command_pixel_end
						  , int *line_offset
						  );
// Set line position
void SetCursorLine( PHISTORY_LINE_CURSOR cursor, int nLine );
// set total number of lines...
void SetCursorHeight( PHISTORY_LINE_CURSOR cursor, int nHeight );
CORECON_PROC( void, SetBrowserHeight )( PHISTORY_BROWSER cursor, int nLines );

// get column position
int GetCursorColumn( PHISTORY_LINE_CURSOR cursor );
// set column position
void SetCursorColumn( PHISTORY_LINE_CURSOR cursor, int nColumn );
// set total columns available...
void SetCursorWidth( PHISTORY_LINE_CURSOR cursor, int size );
void SetBrowserWidth( PHISTORY_BROWSER cursor, int size );
void SetBrowserFirstLine( PHISTORY_BROWSER cursor, int nLIne );

void SetCursorNoPrompt( PHISTORY_BROWSER phbr, LOGICAL bNoPrompt );

// computation of line is computed from the cursor's columns.
// amount +/- 1 == lineup down
// amount +/- N == pageup down
int MoveHistoryCursor( PHISTORY_BROWSER phbr, int amount );

uint32_t ComputeNextOffset( PTEXT segment, uint32_t nShown );
//int ComputeToShow( uint32_t cols, PTEXT segment, int nOfs, int nShown );
uint32_t ComputeToShow( uint32_t colsize, uint32_t *col_offset, PTEXT segment, uint32_t nLen, uint32_t nOfs, uint32_t nShown, PHISTORY_BROWSER phbr, SFTFont font );
int CountLinesSpanned( PHISTORY_BROWSER phb, PTEXT countseg, SFTFont font, LOGICAL count_trailing_linefeeds );

CORECON_PROC( void, BuildDisplayInfoLines )( PHISTORY_BROWSER phlc, PHISTORY_BROWSER leadin, SFTFont font );


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
void GetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, int32_t* x, int32_t* y );
void SetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, int32_t x, int32_t y );
void PSI_SetHistoryPageLines( PHISTORY_BROWSER phbr, uint32_t nLines );
int32_t GetBrowserDistance( PHISTORY_BROWSER phbr, SFTFont font );
void ResetHistoryBrowser( PHISTORY_BROWSER phbr );
CORECON_PROC( int, CountDisplayedLines) ( PHISTORY_BROWSER phbr );


void SetHistoryBackingFile( PHISTORY_REGION phr, FILE *file );


//void SetHistoryBrowserOwnPageBreak( PHISTORY_BROWSER phbr );
//void SetHistoryBrowserNoPageBreak( PHISTORY_BROWSER phbr );
PSI_CONSOLE_NAMESPACE_END


#endif

