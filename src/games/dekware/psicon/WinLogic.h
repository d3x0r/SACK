
#include "consolestruc.h"



#ifdef CURSECON
#define SEPARATOR_HEIGHT 1
#else
#define SEPARATOR_HEIGHT 4
#endif

//#define DEBUG_KEY_EVENTS
//#define DEBUG_COMMAND_INPUT_DRAW

#ifdef __DEKWARE_PLUGIN__
#ifndef WINLOGIC_SOURCE
#ifndef CORECON_SOURCE
#if 0
CORECON_PROC( volatile_variable_entry, vve_rows );
CORECON_PROC( volatile_variable_entry, vve_cols );
CORECON_PROC( volatile_variable_entry, vve_cursorx );
CORECON_PROC( volatile_variable_entry, vve_cursory );
#endif
#endif
#endif
#endif

// pass the rect describing the area...
// font size, etc will have been set in the datapath already...
CORECON_PROC(void, ChildCalculate )( PCONSOLE_INFO pdp );
CORECON_PROC( void, RenderConsole )( PCONSOLE_INFO pdp );


// this is a short calculation, assuming that
// the size of the whole surface has not changed,
// use this to update portions within...
CORECON_PROC(void, CalculateHistory )( PCONSOLE_INFO pdp );
CORECON_PROC( int, WinLogicWrite )( PDATAPATH pdp
#ifndef __DEKWARE_PLUGIN__
											 , PTEXT pLine
#endif
											 );
CORECON_PROC( void, WinLogicCalculateHistory )( PCONSOLE_INFO pdp );

CORECON_PROC(PTEXT, GetRows )( uintptr_t psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, GetCols )( uintptr_t psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, GetCursorX )( uintptr_t psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, SetCursorX )( uintptr_t psv, struct entity_tag *pe, PTEXT newvalue );
CORECON_PROC(PTEXT, GetCursorY )( uintptr_t psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, SetCursorY )( uintptr_t psv, struct entity_tag *pe, PTEXT newvalue );


//CORECON_PROC(void, RenderCommandLine )( PCONSOLE_INFO pdp );
//void RenderSeparator( PCONSOLE_INFO pdp, int nStart );
//CORECON_PROC(void, RenderDisplay )( PCONSOLE_INFO pdp );
//CORECON_PROC(void, RenderHistory )( PCONSOLE_INFO pdp );
CORECON_PROC( void, WinLogicDoStroke )( PCONSOLE_INFO pdp, PTEXT pStroke );

// nChar could also be considered nColumn
CORECON_PROC(int, GetCharFromLine )( uint32_t cols
                         , PDISPLAYED_LINE pLine
                         , int nChar, TEXTCHAR *result );
CORECON_PROC(int, GetCharFromRowCol )( PCONSOLE_INFO pdp
                            , int row, int col
                            , TEXTCHAR *data );

CORECON_PROC(TEXTCHAR *,GetDataFromBlock )( PCONSOLE_INFO pdp );
CORECON_PROC(int, ConvertXYToLineCol )( PCONSOLE_INFO pdp
                              , int x, int y
                              , int *line, int *col );
//CORECON_PROC(void, DoRenderHistory )( PCONSOLE_INFO pdp, int bHistoryStart );
CORECON_PROC(int, UpdateHistory )( PCONSOLE_INFO pdp );
//------------------------------------------------------------------
// reflective definitions - these need to exist in the real project...
#ifdef __DEKWARE_PLUGIN__
int HandleOptions( PDATAPATH pdp, PSENTIENT ps, PTEXT option );
#endif

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

