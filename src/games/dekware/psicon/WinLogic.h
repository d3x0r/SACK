
#include "consolestruc.h"



#ifdef CURSECON
#define SEPARATOR_HEIGHT 1
#else
#define SEPARATOR_HEIGHT 4
#endif


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

CORECON_PROC(PTEXT, GetRows )( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, GetCols )( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, GetCursorX )( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, SetCursorX )( PTRSZVAL psv, struct entity_tag *pe, PTEXT newvalue );
CORECON_PROC(PTEXT, GetCursorY )( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue );
CORECON_PROC(PTEXT, SetCursorY )( PTRSZVAL psv, struct entity_tag *pe, PTEXT newvalue );


//CORECON_PROC(void, RenderCommandLine )( PCONSOLE_INFO pdp );
//void RenderSeparator( PCONSOLE_INFO pdp, int nStart );
//CORECON_PROC(void, RenderDisplay )( PCONSOLE_INFO pdp );
//CORECON_PROC(void, RenderHistory )( PCONSOLE_INFO pdp );
CORECON_PROC( void, WinLogicDoStroke )( PCONSOLE_INFO pdp, PTEXT pStroke );

// nChar could also be considered nColumn
CORECON_PROC(int, GetCharFromLine )( _32 cols
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
		_32 bHasContent : 1;
		_32 bTmpRect : 1;
	} flags;
   CRITICALSECTION cs;
	S_32 x, y;
   _32 width, height;
} PENDING_RECT, *PPENDING_RECT;

