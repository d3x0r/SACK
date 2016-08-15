
#include "consolestruc.h"


PSI_CONSOLE_NAMESPACE

#ifdef CURSECON
#define SEPARATOR_HEIGHT 1
#else
#define SEPARATOR_HEIGHT 4
#endif


// pass the rect describing the area...
// font size, etc will have been set in the datapath already...
CORECON_PROC( void, PSI_ConsoleCalculate )( PCONSOLE_INFO pdp );
CORECON_PROC( void, PSI_RenderConsole )( PCONSOLE_INFO pdp );


// this is a short calculation, assuming that
// the size of the whole surface has not changed,
// use this to update portions within...
CORECON_PROC(void, PSI_CalculateHistory )( PCONSOLE_INFO pdp );

CORECON_PROC( PSI_Console_Phrase, PSI_WinLogicWriteEx )( PCONSOLE_INFO pdp
												, PTEXT pLine
												, int update
												);
CORECON_PROC( void, PSI_WinLogicCalculateHistory )( PCONSOLE_INFO pdp );

CORECON_PROC( void, PSI_WinLogicDoStroke )( PCONSOLE_INFO pdp, PTEXT pStroke );

// nChar could also be considered nColumn
CORECON_PROC(int, PSI_GetCharFromLine )( uint32_t cols
                         , PDISPLAYED_LINE pLine
                         , int nChar, TEXTCHAR *result );
CORECON_PROC(int, PSI_GetCharFromRowCol )( PCONSOLE_INFO pdp
                            , int row, int col
                            , char *data );

CORECON_PROC(TEXTCHAR *,PSI_GetDataFromBlock )( PCONSOLE_INFO pdp );
CORECON_PROC(int, PSI_ConvertXYToLineCol )( PCONSOLE_INFO pdp
                              , int x, int y
                              , int *line, int *col );
//CORECON_PROC(void, DoRenderHistory )( PCONSOLE_INFO pdp, int bHistoryStart );
CORECON_PROC(int, PSI_UpdateHistory )( PCONSOLE_INFO pdp );
int GetCharFromLine( PCONSOLE_INFO console, uint32_t cols
						, PDISPLAYED_LINE pLine
						, int nChar, TEXTCHAR *result );
//------------------------------------------------------------------
// reflective definitions - these need to exist in the real project...

typedef struct penging_rectangle_tag
{
	struct {
		uint32_t bHasContent : 1;
		uint32_t bTmpRect : 1;
	} flags;
   CRITICALSECTION cs;
	int32_t x, y;
   uint32_t width, height;
} PENDING_RECT, *PPENDING_RECT;


void PSI_RenderCommandLine( PCONSOLE_INFO pdp, PENDING_RECT *region );	
void CPROC PSIMeasureString( uintptr_t psvConsole, CTEXTSTR s, int nShown, uint32_t *w, uint32_t *h, SFTFont font );

PSI_CONSOLE_NAMESPACE_END


