//#define NO_LOGGING
#ifdef PSICON
#define USE_IMAGE_INTERFACE ImageInterface
#endif
#include <stdhdrs.h>
#define DEFINES_DEKWARE_INTERFACE
#include "consolestruc.h"
//#include "interface.h"
#include "history.h"

#define WINLOGIC_SOURCE

#include "WinLogic.h"

//----------------------------------------------------------------------------

static PENDING_RECT update_rect;
static S_32 mouse_x, mouse_y;
static _32  mouse_buttons, _mouse_buttons;

static void AddUpdateRegion( PPENDING_RECT update_rect, S_32 x, S_32 y, _32 wd, _32 ht )
{
#ifdef __LINUX__
	if( !update_rect->flags.bTmpRect )
	{
		if( !update_rect->flags.bHasContent )
         MemSet( &update_rect->cs, 0, sizeof( update_rect->cs ) );
		EnterCriticalSec( &update_rect->cs );
	}
#endif
	if( wd && ht )
	{
		if( update_rect->flags.bHasContent )
		{
			if( x < update_rect->x )
			{
				update_rect->width += update_rect->x - x;
				update_rect->x = x;
			}
			if( x + wd > update_rect->x + update_rect->width )
				update_rect->width = ( wd + x ) - update_rect->x;

			if( y < update_rect->y )
			{
				update_rect->height += update_rect->y - y;
				update_rect->y = y;
			}
			if( y + ht > update_rect->y + update_rect->height )
				update_rect->height = ( y + ht ) - update_rect->y;
			//lprintf( WIDE("result (%d,%d)-(%d,%d)")
          //      , update_rect->x, update_rect->y
          //      , update_rect->width, update_rect->height
			 //  	 );
		}
		else
		{
			//_lprintf( DBG_AVAILABLE, WIDE("Setting (%d,%d)-(%d,%d)") DBG_RELAY
			//		 , x, y
         //       , wd, ht
			//		 );
			update_rect->x = x;
			update_rect->y = y;
			update_rect->width = wd;
			update_rect->height = ht;
		}
		update_rect->flags.bHasContent = 1;
	}
#ifdef __LINUX__
	if( !update_rect->flags.bTmpRect )
		LeaveCriticalSec( &update_rect->cs );
#endif
}


void RenderCommandLine( PCONSOLE_INFO pdp, POINTER p )
{
	PENDING_RECT *region = (PENDING_RECT*)p;
	PENDING_RECT myrect;
	// need to render the current macro being recorded.....
	RECT upd;

	RECT r;
	size_t nMaxLen, nRecord, nShown, nShow, nCurrentCol;
	int x, y, nCursorPos;
	size_t start, end;
	int toppad = 0;
	PTEXT pStart;
	// no command line...
#ifdef __DEKWARE_PLUGIN__
	if( !pdp->common.CommandInfo )
#else
	if( !pdp->CommandInfo )
#endif
	{
      // region->bHasContent = 0?
		return;
	}
	if( !region )
	{
		region = &myrect;
		myrect.flags.bHasContent = 0;
		myrect.flags.bTmpRect = 1;
	}

	if( pdp->SetCurrentColor )
		pdp->SetCurrentColor( pdp, COLOR_COMMAND, NULL );//pdp->crCommand, pdp->crCommandBackground );

   // if in direct mode, the exisiting prompt should indicate
   // the current macro recording... well of course if there
   // is no prompt variable, then this should still be generated
   // in which case... uhmm hmm....
	nCursorPos = GetCommandCursor( pdp->pCurrentDisplay
#ifdef __DEKWARE_PLUGIN__
										  , pdp->common.CommandInfo
#else
										  , pdp->CommandInfo
#endif
                                , pdp->flags.bDirect
                                , &nCurrentCol
                                , &start
                                , &end );

	// nYpad at bottom of screen, font height up begins the top of the
	// output text line...

	// also line starts are considered from the bottom up...
   // nXPad,n__LineStart
	r.top = y = pdp->nCommandLineStart
		- ( pdp->nFontHeight > 1?pdp->nFontHeight+pdp->nYPad:0 );
	r.bottom = pdp->nHeight;
	if( !pdp->flags.bDirect )
      toppad = pdp->nCmdLinePad;
	r.top -= toppad;
/*
	lprintf( WIDE("*** Commandline %d,%d  uhh %d %d  %d and %d")
			 , r.top, r.bottom
			 , start, end
			 , nCursorPos
           , nCurrentCol
			  );
           */
   if( !nCurrentCol )
   {
      // need to blatcolor for the 5 pixels left of first TEXTCHAR...
      r.left = 0;
		r.right = pdp->nXPad;
		if( pdp->FillConsoleRect )
		{
         pdp->FillConsoleRect( pdp, &r, FILL_COMMAND_BACK );
		}
      upd.left = 0;
   }
	else
	{
      // this is an inaccurate calculation.
		//upd.left = pdp->nXPad + ( nCurrentCol * pdp->nFontWidth );
		if( pdp->flags.bDirect )
			r.right = upd.left = ( pdp->nNextCharacterBegin );
      else
			r.right = upd.left = pdp->nXPad;
	}

	// this is an inaccurate calculation.
	//r.left = x = pdp->nXPad + ( nCurrentCol * pdp->nFontWidth );
	r.left = x = r.right;
	// for now...

   upd.right = pdp->nWidth;
   upd.top = r.top;
   upd.bottom = r.bottom;

   {
      // totally set the background of the command thingy...
      // previously the putstring would have done the rect fill...
      // but now we need to just put text data over a solid backgorund...
		r.right = pdp->nWidth;
      r.top -= toppad;
		if( pdp->FillConsoleRect )
         pdp->FillConsoleRect( pdp, &r, FILL_COMMAND_BACK );
      r.top += toppad;
   }

   nRecord = 0;

#ifdef __DEKWARE_PLUGIN__
   if( pdp->common.Owner && pdp->common.Owner->pRecord )
   {
      TEXTCHAR byName[256];
      nShown = 0;
      // hmm should do something fancy with this
      // like make it externally definable like prompt is now...
      nShow = snprintf( byName, sizeof( byName ), WIDE("(%s) ")
                     , GetText( GetName( pdp->common.Owner->pRecord ) )
							 );
		if( pdp->DrawString )
			pdp->DrawString( pdp, x, y, &r, byName, nShown, nShow );
      //DrawString( byName );
      nRecord = nShow;  // total spot of end ?
      x = r.left = r.right;
	}
#endif
   // the normal prompt string will have the current
   // macroname being recorded... do not show this in
   // direct mode.....
   nMaxLen = end - start;

	nShown = start;
#ifdef __DEKWARE_PLUGIN__
	pStart = pdp->common.CommandInfo->CollectionBuffer;
#else
	pStart = pdp->CommandInfo->CollectionBuffer;
#endif
   SetStart( pStart );
   while( pStart && USS_GT( nShown, int, GetTextSize( pStart ), size_t ) )
   {
      nShown -= GetTextSize( pStart );
      pStart = NEXTLINE( pStart );
   }

   while( pStart && nCurrentCol < end )
   {
      nShow = GetTextSize( pStart ) - nShown;

      if( nCurrentCol + nShow > end )
         nShow = end - nCurrentCol;

		if( pdp->DrawString )
         pdp->DrawString( pdp, x, y, &r, GetText( pStart ), nShown, nShow );
      x = r.left = r.right;
      nShown = 0;
      nCurrentCol += nShow;
      pStart = NEXTLINE( pStart );
   }
   //if( pStart )
   //   lprintf( WIDE("Stopped because of length.") );
   r.left = r.right;
   r.right = pdp->nWidth;
   // only have to clean trail if on a direct input method...
   if( r.right > r.left )
   {
		// clear the remainder of the line...
		//lprintf( WIDE("Clearing end of line...") );
		if( pdp->FillConsoleRect )
         pdp->FillConsoleRect( pdp, &r, FILL_DISPLAY_BACK );
   }
	if( pdp->RenderCursor )
		// rect has top/bottom info, and current cursor position column
      // is passed - each client will be able to
		pdp->RenderCursor( pdp, &r, ( nRecord + nCursorPos ) ); // top/bottom are the line...

	// command line only update ? maybe add this to regions which should be updated?
	// refresh here?
	AddUpdateRegion( region, upd.left, upd.top, upd.right-upd.left,upd.bottom-upd.top );
	if( pdp->Update && region->flags.bHasContent )
	{
		RECT r;
		r.left = region->x;
		r.right = region->x + region->width;
		r.top = region->y;
      r.bottom = region->y + region->height;
		pdp->Update( pdp, &r );
      region->flags.bHasContent = 0;
	}
}


//----------------------------------------------------------------------------
// 5 on left, 5 on right total 10 pixels we can't use...

CORECON_PROC( void, WinLogicCalculateHistory )( PCONSOLE_INFO pdp )
{
   // there's some other related set of values to set here....
	//lprintf( WIDE("Calculate history! %d %d"), pdp->nColumns, pdp->nLines );

   SetCursorLines( pdp->pCursor, pdp->nLines );
	SetCursorColumns( pdp->pCursor, pdp->nColumns );
	SetBrowserColumns( pdp->pHistoryDisplay, pdp->nColumns );
	SetBrowserColumns( pdp->pCurrentDisplay, pdp->nColumns );

   if( pdp->flags.bHistoryShow )
	{
      //lprintf( WIDE("Doing history... check percent and set display/history approp.") );
      switch( pdp->nHistoryPercent )
      {
      case 0:  // 25
      case 1: //50
      case 2: //75
         {
            int nWorkLines;
            nWorkLines = ( pdp->nLines * ( 3 - pdp->nHistoryPercent ) ) / 4;
				SetBrowserLines( pdp->pHistoryDisplay, (pdp->nLines - nWorkLines)+2 );
            SetHistoryPageLines( pdp->pHistoryDisplay, (pdp->nLines - nWorkLines)-3 );
				pdp->nHistoryLineStart = pdp->nDisplayLineStart - (nWorkLines)* pdp->nFontHeight;
				SetBrowserLines( pdp->pCurrentDisplay, nWorkLines );
         }
         break;
      case 3: //100
			pdp->nHistoryLineStart = pdp->nDisplayLineStart;
			SetBrowserLines( pdp->pHistoryDisplay, pdp->nLines );
         // need this to know how far close to end we can get...
			SetBrowserLines( pdp->pCurrentDisplay, 0 );
			SetHistoryPageLines( pdp->pHistoryDisplay, pdp->nLines );
         //pdp->nHistoryLines = nLines;
			//pdp->nDisplayLines = 0;
		}
   }
   else
	{
		//lprintf( WIDE("No history, all display") );
		// internally we'll need this amount to get into
      // scrollback...
		pdp->nHistoryLineStart = 0;
		{
			int nWorkLines;
			nWorkLines = ( pdp->nLines * ( 1 + pdp->nHistoryPercent ) ) / 4;
			SetHistoryPageLines( pdp->pHistoryDisplay, nWorkLines - 3 );
		}
		SetHistoryPageLines( pdp->pHistoryDisplay, pdp->nLines - 4 );
		SetBrowserLines( pdp->pHistoryDisplay, 1 );
		ResetHistoryBrowser( pdp->pHistoryDisplay );
      // 1 for the partial line at the top of the display.
		SetBrowserLines( pdp->pCurrentDisplay, pdp->nLines );
   }
	BuildDisplayInfoLines( pdp->pHistoryDisplay );
	BuildDisplayInfoLines( pdp->pCurrentDisplay );
}

//----------------------------------------------------------------------------
void DoRenderHistory( PCONSOLE_INFO pdp, int bHistoryStart, PENDING_RECT *region );

void RenderConsole( PCONSOLE_INFO pdp )
{
	PENDING_RECT upd;
	upd.flags.bHasContent = 0;
	upd.flags.bTmpRect = 1;
	MemSet( &upd.cs, 0, sizeof( upd.cs ) );
	EnterCriticalSec( &pdp->Lock );
	//lprintf( WIDE("Render Console... %d %d"), pdp->nDisplayLineStart, pdp->nHistoryLineStart );
	if( pdp->nHistoryLineStart )
	{
		//lprintf( WIDE("no history rende? %d"), pdp->flags.bNoHistoryRender );
		//if( !pdp->flags.bNoHistoryRender )
		{
			//BuildDisplayInfoLines( pdp->pHistoryDisplay );
			DoRenderHistory( pdp, TRUE, &upd );
		}
	}
	if( pdp->nDisplayLineStart != pdp->nHistoryLineStart )
	{
      // should do somehting like - if it hasn't moved don't draw it...
		//BuildDisplayInfoLines( pdp->pCurrentDisplay );
		DoRenderHistory( pdp, FALSE, &upd );
	}
	if( !(pdp->flags.bDirect && pdp->flags.bCharMode) )
		RenderCommandLine( pdp, &upd );
	if( pdp->Update && upd.flags.bHasContent )
	{
		RECT r;
		r.left = upd.x;
		r.right = upd.x + upd.width;
		r.top = upd.y;
		r.bottom = upd.y + upd.height;
		pdp->Update( pdp, &r );
	}
   LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------

void ChildCalculate( PCONSOLE_INFO pdp )
{
    //RECT rArea;
	int nLines;
    //lprintf( WIDE("*** DISPLAY is %d,%d by %d,%d"), pdp->rArea.top, pdp->rArea.left, pdp->rArea.right, pdp->rArea.bottom );
    if ( (pdp->rArea.right - pdp->rArea.left) <= 0 || 
          (pdp->rArea.bottom - pdp->rArea.top ) <= 0 )
    {
        pdp->flags.bNoDisplay = 1;
    }
    else
        pdp->flags.bNoDisplay = 0;

	 pdp->nWidth = pdp->rArea.right - pdp->rArea.left;
	 pdp->nHeight = pdp->rArea.bottom - pdp->rArea.top;
	 if( pdp->nWidth <= 0 || pdp->nHeight <= 0 )
	 {
		 pdp->nWidth = 0;
		 pdp->nHeight = 0;
	 }

	 pdp->nCommandLineStart = pdp->rArea.bottom;

	 if( pdp->nFontHeight )
	 {
       //lprintf( WIDE("Okay font height existsts... that's good") );
		 if( pdp->flags.bDirect )
		 {
			 pdp->nDisplayLineStart = pdp->nCommandLineStart;
			 SetCursorNoPrompt( pdp->pCurrentDisplay, FALSE );
			 SetCursorNoPrompt( pdp->pHistoryDisplay, FALSE );
		 }
		 else
		 {
			 SetCursorNoPrompt( pdp->pCurrentDisplay, TRUE );
			 SetCursorNoPrompt( pdp->pHistoryDisplay, FALSE );
          //lprintf( WIDE("Starting display above command start") );
			 pdp->nDisplayLineStart = pdp->nCommandLineStart
				 - ( pdp->nFontHeight
					 + ( pdp->nYPad * 2 )
					 + ( pdp->nCmdLinePad )
					);
		 }
		 nLines = ( pdp->nDisplayLineStart + ( pdp->nFontHeight - 1 ) + pdp->nYPad )
				  / pdp->nFontHeight;
	 }
	 else
	 {
		 lprintf( WIDE("Font height does not exist :(") );
		 nLines = 0;
	 }

	 if( pdp->nFontWidth )
	 {
		 pdp->nColumns = pdp->nFontWidth?(( pdp->nWidth - (pdp->nXPad*2) ) / pdp->nFontWidth):0;
		 // there's some other related set of values to set here....
		 pdp->nLines = nLines;
		 WinLogicCalculateHistory( pdp );
	 }
	 // hmm this is pretty ugly - probably have to fix it oneday...
#ifdef __DEKWARE_PLUGIN__
	 /* this should be invoked as a behavior... */
    if( pdp->common.Owner &&
         LocateMacro( pdp->common.Owner->Current, WIDE("WindowResize") ) )
    {
        TEXTCHAR resize[256];
        snprintf( resize, sizeof( resize ), WIDE("/WindowResize %d %d")
                            , pdp->nColumns
                            , pdp->nLines );
        QueueCommand( pdp->common.Owner, resize );
	 }
#endif
#ifndef __cplusplus_cli
    RenderConsole( pdp );
	// otherwise we delay renderconsole until Paint();
#endif
}

//----------------------------------------------------------------------------

PTEXT CPROC GetRows( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue )
{
    PCONSOLE_INFO pmdp = (PCONSOLE_INFO)psv;
    if( *lastvalue )
        LineRelease( *lastvalue );
    *lastvalue = SegCreateFromInt( pmdp->nLines );
    return *lastvalue;
}

//----------------------------------------------------------------------------

PTEXT CPROC GetCols( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue )
{
    PCONSOLE_INFO pmdp = (PCONSOLE_INFO)psv;
    if( *lastvalue )
        LineRelease( *lastvalue );
    *lastvalue = SegCreateFromInt( pmdp->nColumns );
    return *lastvalue;
}

//----------------------------------------------------------------------------

PTEXT CPROC GetCursorX( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue )
{
    PCONSOLE_INFO pmdp = (PCONSOLE_INFO)psv;
    // cursorx ... abstract thing.... (pulled from history I guess)
    if( *lastvalue )
       LineRelease( *lastvalue );

    *lastvalue = SegCreateFromInt( GetCursorColumn( pmdp->pCursor ) + 1 );
    return *lastvalue;
}

//----------------------------------------------------------------------------

PTEXT CPROC SetCursorX( PTRSZVAL psv, struct entity_tag *pe, PTEXT newvalue )
{
    PCONSOLE_INFO pmdp = (PCONSOLE_INFO)psv;
    // cursorx ... abstract thing.... (pulled from history I guess)
    SetCursorColumn( pmdp->pCursor, atoi( GetText( newvalue ) ) - 1 );
    return NULL;
}

//----------------------------------------------------------------------------

PTEXT CPROC GetCursorY( PTRSZVAL psv, struct entity_tag *pe, PTEXT *lastvalue )
{
    PCONSOLE_INFO pmdp = (PCONSOLE_INFO)psv;
    if( *lastvalue )
        LineRelease( *lastvalue );
    *lastvalue = SegCreateFromInt( GetCursorLine( pmdp->pCursor ) + 1 );
    return *lastvalue;
}

//----------------------------------------------------------------------------

PTEXT CPROC SetCursorY( PTRSZVAL psv, struct entity_tag *pe, PTEXT newvalue )
{
    PCONSOLE_INFO pmdp = (PCONSOLE_INFO)psv;
    // cursorx ... abstract thing.... (pulled from history I guess)
    SetCursorLine( pmdp->pCursor, atoi( GetText( newvalue ) ) - 1 );
    return NULL;
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
#if 0
CORECON_EXPORT( volatile_variable_entry, vve_rows ) = { DEFTEXT( WIDE("rows") )
																		, GetRows, NULL };
CORECON_EXPORT( volatile_variable_entry, vve_cols ) = { DEFTEXT( WIDE("cols") )
																		, GetCols, NULL };
CORECON_EXPORT( volatile_variable_entry, vve_cursorx ) = { DEFTEXT( WIDE("cursorx") )
																			, GetCursorX
																			, SetCursorX };
CORECON_EXPORT( volatile_variable_entry, vve_cursory ) = { DEFTEXT( WIDE("cursory") )
																			, GetCursorY
																			, SetCursorY };
#endif
#endif

//----------------------------------------------------------------------------

int WinLogicWrite( PDATAPATH pdp
#ifndef __DEKWARE_PLUGIN__
					  , PTEXT pLine
#endif
					  )
{
	PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;
#ifdef __DEKWARE_PLUGIN__
	PTEXT pLine;
#endif
   int first_read = 1;
   static int updated;
	EnterCriticalSec( &pmdp->Lock );
#ifdef __DEKWARE_PLUGIN__
	while( ( pLine = (PTEXT)DequeLink( &pmdp->common.Output ) ) )
#endif
	{
      first_read = 0;
		//int flags = pLine->flags & (TF_NORETURN|TF_PROMPT);
      //lprintf( WIDE("Updated... %d"), updated );
      updated++;

      if( pLine->flags & TF_FORMATABS )
      {
			S_32 cursorx, cursory;
         //lprintf( WIDE("absolute position format.") );
         GetHistoryCursorPos( pmdp->pCursor, &cursorx, &cursory );
         if( pLine->format.position.coords.x != -16384 )
            cursorx = pLine->format.position.coords.x;
         if( pLine->format.position.coords.y != -16384 )
            cursory = pmdp->nLines - pLine->format.position.coords.y;
         SetHistoryCursorPos( pmdp->pCursor, cursorx, cursory );
         pLine->format.position.offset.spaces = 0;
         pLine->format.position.offset.tabs = 0;
         pLine->flags &= ~TF_FORMATABS;
      }
      if( pLine->flags & TF_FORMATREL )
      {
			S_32 cursorx, cursory;
         //lprintf( WIDE("relative position format") );
         GetHistoryCursorPos( pmdp->pCursor, &cursorx, &cursory );
         cursorx += pLine->format.position.coords.x;
         cursory += pLine->format.position.coords.y;
         SetHistoryCursorPos( pmdp->pCursor, cursorx, cursory );
         pLine->format.position.offset.spaces = 0;
         pLine->format.position.offset.tabs = 0;
         pLine->flags &= ~TF_FORMATREL;
         // this should not leave the current region....
      }
#ifdef COMMAND_LINE_ENTRY_EXTRA_NEWLINE_STUFF
      if( !( pLine->flags & TF_NORETURN ) ||
         ( pLine->flags & TF_FORMATREL ) ||
         ( phc->region->flags.bForceNewline ) )
      { // err new segment goes on a new line.  (even if we are in the past)
         //Log2( WIDE("Line is automatically promoting itself to the next line. %d %d")
         //  , pht->nCursorY, pht->pTrailer?pht->pTrailer->nLinesUsed:-1 );
         if( !( pLine->flags & TF_NORETURN ) || phc->region->flags.bForceNewline )
            (phc->nCursorY)++;
      }
      if( !( pLine->flags & TF_NORETURN ) || phc->region->flags.bForceNewline )
         (phc->nCursorX) = 0;

      pLine->flags &= ~TF_FORMATREL;
      phc->region->flags.bForceNewline = FALSE;
#endif
		// at the point, history will use the current
		// cursorx, cursory and output the line, if TF_NORETURN
		// otherwise it will reset cursorx and insert one line to history.
		// unless no line insert, then the next line will be overwritten, and one blank line added to end
		// to keep cursorY bias from end of screen the same?
		// if CursorY == 0 (last line) then one line is added, else cursor Y is adjusted... that's it.

		// history will also respect some of the format_ops... actually the display history
		// is this layer inbetween history and display that handles much of the format ops...
      EnqueDisplayHistory( pmdp->pCursor, pLine );
   }
   if( updated )
	{
		if( first_read )
		{
			PENDING_RECT upd;
			//lprintf( WIDE("Okay well there's no data, first pass to check, end of stream... flush") );
			MemSet( &upd.cs, 0, sizeof( upd.cs ) );
			upd.flags.bTmpRect = 1;
			upd.flags.bHasContent = 0;
			// need to wait for an idle condition - this
			// process is low priority...
			BuildDisplayInfoLines( pmdp->pCurrentDisplay );
			if( pmdp->psicon.frame )
				SmudgeCommon( pmdp->psicon.frame );
			else
				DoRenderHistory( pmdp, FALSE, &upd );
			if( pmdp->Update && upd.flags.bHasContent )
			{
				RECT r;
				r.left = upd.x;
				r.right = upd.x + upd.width;
				r.top = upd.y;
				r.bottom = upd.y + upd.height;
            //lprintf( WIDE("Doing update call to display...") );
				pmdp->Update( pmdp, &r );
			}
			updated = 0;
		}
	}
	LeaveCriticalSec( &pmdp->Lock );
	return updated;
}

//----------------------------------------------------------------------------

int GetCharFromLine( _32 cols
                   , PDISPLAYED_LINE pLine
                   , int nChar, TEXTCHAR *result )
{
   int nLen;
   if( pLine && result )
   {
      PTEXT pText = pLine->start;
      int nOfs = 0, nShown = pLine->nOfs;
      while( pText )
      {
         // nOfs is the column position to start at...
         // nShown is the amount of the first segment shown.
         nLen = ComputeToShow( cols, pText, GetTextSize( pText ), nOfs, nShown );
         //nLen = GetTextSize( pText );
         if( nChar < pText->format.position.offset.spaces )
         {
            *result = ' ';
            return TRUE;
         }
         nChar -= pText->format.position.offset.spaces;
         if( nChar < nLen )
         {
            TEXTCHAR *text = GetText( pText );
            *result = text[nChar + nShown];
            return TRUE;
         }
         nChar -= nLen;
         if( nLen == ( GetTextSize( pText ) + nShown ) )
            pText = NEXTLINE( pText );
         else
            pText = NULL;
         nShown = 0; // have shown nothing on this segment.
      }
   }
   return FALSE;
}

//----------------------------------------------------------------------------
#if 0
int GetCharFromRowCol( PCONSOLE_INFO pdp, int row, int col, TEXTCHAR *data )
{
    PDISPLAYED_LINE pdl = GetDataItem( &pdp->pCurrentDisplay->DisplayLineInfo, row );
    return GetCharFromLine( pdp->nColumns, pdl, col, data );
}
#endif
//----------------------------------------------------------------------------

TEXTCHAR *GetDataFromBlock( PCONSOLE_INFO pdp )
{
    int line_start = pdp->mark_start.row;
    int col_start = pdp->mark_start.col;
    int line_end = pdp->mark_end.row;
    int col_end = pdp->mark_end.col;
    int bBlock = FALSE;
    // 2 characters to stuff in \r\n on newline.
    TEXTSTR result = NewArray( TEXTCHAR, ( ( line_start - line_end ) + 1 ) * (pdp->nWidth + 2) );
    INDEX ofs = 0;
    int line, col;
    int first = TRUE;
    int _priorline;
    for( col = col_start, line = line_start
        ; line >= line_end
        ; line--, (col = bBlock)?col_start:0 )
    {
        PDISPLAYED_LINE pdl;
        if( ( pdl = (PDISPLAYED_LINE)GetDataItem( pdp->CurrentMarkInfo, line ) ) )
        {       
            if( first )
            {
                first = FALSE;
                _priorline = pdl->nLine;
            }
            else if( _priorline != pdl->nLine || bBlock )
            {
                result[ofs++] = '\r';
                result[ofs++] = '\n';
                _priorline = pdl->nLine;
            }
            for( ; 
                  col < (bBlock?(col_end)
                           : ( line == line_end ? col_end 
                                   : pdp->nColumns ));
                 col++ )
            {
                if( GetCharFromLine( pdp->nColumns, pdl, col, result + ofs ) )
                    ofs++;
            }
        }
    }
    result[ofs] = 0;
    if( ofs )
        return result;
    Release( result );
    return NULL;
}

//----------------------------------------------------------------------------

int ConvertXYToLineCol( PCONSOLE_INFO pdp
                              , int x, int y
                              , int *line, int *col )
{
    // x, y is top, left biased...
    // line is bottom biased... (also have to account for history)
    *col = ( ( ( x + ( pdp->nFontWidth / 2 ) ) - pdp->nXPad )
                     / pdp->nFontWidth );
    if( y < pdp->nHistoryLineStart )
    {
        // y is in 'history'
        // might have to bias over separator lines
        y = pdp->nHistoryLineStart - y - pdp->nYPad; // invert y;
        pdp->CurrentLineInfo = GetDisplayInfo( pdp->pHistoryDisplay );
    }
    else if( y < pdp->nDisplayLineStart )
    {
        // y is in 'display'
        y = pdp->nDisplayLineStart - pdp->nYPad - y; // invert y;
        pdp->CurrentLineInfo = GetDisplayInfo( pdp->pCurrentDisplay );
    }
    else // y is on the command line...
    {
        return FALSE;
    }
    *line = y / pdp->nFontHeight;
    return TRUE;
}

//----------------------------------------------------------------------------

int GetMaxDisplayedLine( PCONSOLE_INFO pdp, int nStart )
{
    if( nStart )
        return ( pdp->nDisplayLineStart ) 
                  / pdp->nFontHeight;
    else
        return ( pdp->nCommandLineStart 
               - pdp->nDisplayLineStart ) 
                  / pdp->nFontHeight;
}

//----------------------------------------------------------------------------

void DoRenderHistory( PCONSOLE_INFO pdp, int bHistoryStart, PENDING_RECT *region )
{
	int nLine = 0, nChar, nLen, x, y, nMinLine, nFirst = 0;
	PTEXT pText;
	RECT r;
	RECT upd;
	int nFirstLine;
	PDATALIST *ppCurrentLineInfo;
	if( pdp->flags.bNoDisplay )
	{
		lprintf( WIDE("nodisplay!") );
		return;
	}
	EnterCriticalSec( &pdp->Lock );

	if( !bHistoryStart )
	{
		// if no display (all history?)
		// this is the command line/display line separator.
		nFirstLine = ( upd.bottom = pdp->nDisplayLineStart ) - ( ( pdp->nYPad ) + pdp->nFontHeight );
		if( !pdp->flags.bDirect && pdp->nDisplayLineStart != pdp->nCommandLineStart )
		{
			//lprintf( WIDE("Rendering display line seperator %d (not %d)"), pdp->nDisplayLineStart, pdp->nCommandLineStart );
			if( pdp->RenderSeparator )
				pdp->RenderSeparator( pdp, pdp->nDisplayLineStart );
			// add update region...
			// but how big is the thing that just drew?!
		}

		//lprintf( WIDE("nFirstline is %d"), nFirstLine );

		// figure out if we draw up to history or all the screen...
		if( pdp->nHistoryLineStart )
			nMinLine = pdp->nHistoryLineStart;
		else
			nMinLine = 0;
		ppCurrentLineInfo = GetDisplayInfo( pdp->pCurrentDisplay );
	}
	else // do render history start...
	{
		// if no history (all display?)
		if( pdp->nHistoryLineStart == 0 )
		{
			LeaveCriticalSec( &pdp->Lock );
			return;
		}
		nFirstLine = ( upd.bottom = pdp->nHistoryLineStart ) - (pdp->nYPad + pdp->nFontHeight);
		nMinLine = 0;
		nFirst = -1;
		// the seperator is actually rendererd OVER the top of the displayed line.
		ppCurrentLineInfo = GetDisplayInfo( pdp->pHistoryDisplay );
	}
	//lprintf( WIDE("Render history separator %d"), pdp->nHistoryLineStart );
	if( pdp->RenderSeparator )
		pdp->RenderSeparator( pdp, pdp->nHistoryLineStart );
	r.bottom = nFirstLine;

	// left and right are relative... to the line segment only...
	// for the reason of color changes inbetween segments...
	while( 1 )
	{
		int nShow, nShown;
		PDISPLAYED_LINE pCurrentLine;
		//lprintf( WIDE("Get display line %d"), nLine );
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
		if( !pCurrentLine )
		{
			//lprintf( WIDE("No such line... %d"), nLine );
			break;
		}
		r.top = nFirstLine - pdp->nFontHeight * nLine;
		if( nFirst >= 0 )
         r.bottom = r.top + pdp->nFontHeight + 2;
      else
			r.bottom = r.top + pdp->nFontHeight;
		if( r.bottom <= nMinLine )
		{
         //lprintf( WIDE("bottom < minline..") );
			break;
		}
      y = r.top;

      r.left = 0;
		x = r.right = pdp->nXPad;
      if( pdp->FillConsoleRect )
			pdp->FillConsoleRect(pdp, &r, FILL_DISPLAY_BACK );

      r.left = x;
      nChar = 0;
      pText = pCurrentLine->start;
		if( pdp->SetCurrentColor )
         pdp->SetCurrentColor( pdp, COLOR_DEFAULT, NULL );
		nShown = pCurrentLine->nOfs;
		//if( !pText )
		//	lprintf( WIDE("Okay no text to show... end up filling line blank.") );
      while( pText )
      {
			TEXTCHAR *text = GetText( pText );
#ifdef __DEKWARE_PLUGIN__
         if( !pdp->flags.bDirect && ( pText->flags & TF_PROMPT ) )
			{
				//lprintf( WIDE("Segment is prompt - and we need to skip it.") );
            pText = NEXTLINE( pText );
            continue;
         }
#endif
			if( pdp->SetCurrentColor )
				pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );

			nLen = GetTextSize( pText );
         //lprintf( WIDE("%d %d"), nShown, nLen );
			while( nShown < nLen )
			{
            //lprintf( WIDE("nShown < nLen... TEXTCHAR %d len %d toshow %d"), nChar, nLen, pCurrentLine->nToShow );
				if( nChar + nLen > pCurrentLine->nToShow )
					nShow = pCurrentLine->nToShow - nChar;
				else
				{
               //lprintf( WIDE("nShow is what's left of now to nLen from nShown... %d,%d"), nLen, nShown );
					nShow = nLen - nShown;
				}
				if( !nShow )
				{
               //lprintf( WIDE("nothing to show...") );
					break;
				}
				if( pdp->flags.bMarking &&
					ppCurrentLineInfo == pdp->CurrentMarkInfo )
				{
					if( !pdp->flags.bMarkingBlock )
					{
						if( ( nLine ) > pdp->mark_start.row
						  ||( nLine ) < pdp->mark_end.row )
						{
						// line above or below the marked area...
							if( pdp->SetCurrentColor )
								pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );
							//SetCurrentColor( crThisText, crThisBack );
						}
						else
						{
							if( pdp->mark_start.row == pdp->mark_end.row )
							{
								if( nChar >= pdp->mark_start.col &&
									nChar < pdp->mark_end.col )
								{
									if( nChar + nShow > pdp->mark_end.col )
										nShow = pdp->mark_end.col - nChar;
									if( pdp->SetCurrentColor )
										pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
									//SetCurrentColor( pdp->crMark
									 //  				, pdp->crMarkBackground );
								}
								else if( nChar >= pdp->mark_end.col )
								{
									if( pdp->SetCurrentColor )
										pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );
									//SetCurrentColor( crThisText, crThisBack );
								}
								else if( nChar + nShow > pdp->mark_start.col )
								{
									nShow = pdp->mark_start.col - nChar;
								}
							}
							else
							{
								if( nLine == pdp->mark_start.row )
								{
									if( nChar >= pdp->mark_start.col )
									{
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
										//SetCurrentColor( pdp->crMark
										//					, pdp->crMarkBackground );
									}
									else if( nChar + nShow > pdp->mark_start.col )
									{
									// current segment up to the next part...
										nShow = pdp->mark_start.col - nChar;
									}
								}
								if( ( nLine ) < pdp->mark_start.row
								  &&( nLine ) > pdp->mark_end.row )
								{
									if( pdp->SetCurrentColor )
										pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
									//SetCurrentColor( pdp->crMark
									//					, pdp->crMarkBackground );
								}
								if( ( nLine ) == pdp->mark_end.row )
								{
									if( nChar >= pdp->mark_end.col )
									{
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );
										//SetCurrentColor( crThisText, crThisBack );
									}
									else if( nChar + nShow > pdp->mark_end.col )
									{
										nShow = pdp->mark_end.col - nChar;
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
										//SetCurrentColor( pdp->crMark
										//					, pdp->crMarkBackground );
									}
									else if( nChar < pdp->mark_end.col )
									{
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
										//SetCurrentColor( pdp->crMark
										//					, pdp->crMarkBackground );
									}
								}
							}
						}
					}
				}
				if( nChar )
				{
               // left should already equal r.right...
               x = r.left;
				}
				else
				{
					r.left = 0;
					x = pdp->nXPad;
				}
				if( r.bottom > nMinLine )
				{
					//lprintf( WIDE("And finally we can show some text... %s %d"), text, y );
					if( pdp->DrawString )
						pdp->DrawString( pdp, x, y, &r, text, nShown, nShow );
					if( nLine == 0 )  // only keep the (last) line's end.
						pdp->nNextCharacterBegin = r.right;
					//DrawString( text );
				}
				else
					lprintf( WIDE("Hmm bottom < minline?") );
						  // fill to the end of the line...
				//nLen -= nShow;
            r.left = r.right;
				nShown += nShow;
				nChar += nShow;
			}
			//lprintf( WIDE("nShown >= nLen...") );

			nShown -= nLen;
         pText = NEXTLINE( pText );
      }
      {
			x = r.left = r.right;
			r.right = pdp->nWidth;
					  // if soething left to fill, blank fill it...
			if( r.left < r.right )
			{
				//lprintf( WIDE("WRiting empty string (over top?)") );
				if( pdp->FillConsoleRect )
					pdp->FillConsoleRect( pdp, &r, FILL_DISPLAY_BACK );
			}
		}
		if( nFirst >= 0 )
			nFirst = -1;
		nLine++;
	}
	//lprintf( WIDE("r.bottom nMin %d %d"), r.bottom, nMinLine );
	if( r.bottom > nMinLine )
	{
		r.bottom = r.top;
		r.top = nMinLine;
		x = r.left = 0;
		r.right = pdp->nWidth;
#ifndef PSI_LIB
		//lprintf( WIDE("Would be blanking the screen here, but no, there's no reason to.") );
				  //FillEmptyScreen();
#endif
	}
	else
		r.top = r.bottom;
	//RenderConsole( pdp );
	//lprintf( WIDE("Render AGAIN the display line separator") );
	if( pdp->RenderSeparator )
	{
		if( pdp->nDisplayLineStart != pdp->nCommandLineStart )
			pdp->RenderSeparator( pdp, pdp->nDisplayLineStart );
		//lprintf( WIDE("Render AGAIN the hsitory line separator") );
		pdp->RenderSeparator( pdp, pdp->nHistoryLineStart );
	}
	upd.top = r.top;
	upd.left = 0;
	upd.right = pdp->nWidth;
	AddUpdateRegion( region
						, upd.left, upd.top
						, upd.right-upd.left, upd.bottom - upd.top );
	// screen updates affect the posititon of the last line/command line
	if( pdp->flags.bDirect && !bHistoryStart )
		RenderCommandLine( pdp, region );
	LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------
void WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke )
{
	PENDING_RECT upd;
	upd.flags.bHasContent = 0;
	upd.flags.bTmpRect = 1;
	EnterCriticalSec( &pdp->Lock );
	if( DoStroke( pdp, stroke ) )
	{
		RenderCommandLine( pdp, &upd );
	}

	if( pdp->Update && upd.flags.bHasContent )
	{
		RECT r;
		r.left = upd.x;
		r.right = upd.x + upd.width;
		r.top = upd.y;
		r.bottom = upd.y + upd.height;
		pdp->Update( pdp, &r );
	}
	LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
CORECON_PROC( int, SetMode )( PDATAPATH xpdp, PSENTIENT ps, PTEXT params )
{
   PTEXT temp, firstopt = NULL;
   PCONSOLE_INFO pdp = (PCONSOLE_INFO)xpdp;
   while( ( temp = GetParam( ps, &params ) ) )
   {
      PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;
      firstopt = temp;
      //if( pmdp->common.Type == myTypeID )
      {
         if( TextLike( temp, WIDE("direct") ) )
         {
            pmdp->flags.bDirect = TRUE;
            EnterCriticalSec( &pdp->Lock );
#ifdef WINCON
            SetRegistryInt( WIDE("Dekware\\Wincon\\Direct")
                          , GetText( GetName( ps->Current ) )
                          , pdp->flags.bDirect );
#endif
            ChildCalculate( pmdp );
            LeaveCriticalSec( &pdp->Lock );
         }
         else if( TextLike( temp, WIDE("line") ) )
         {
            pmdp->flags.bDirect = FALSE;
            EnterCriticalSec( &pmdp->Lock );
#ifdef WINCON
            SetRegistryInt( WIDE("Dekware\\Wincon\\Direct")
                          , GetText( GetName( ps->Current ) )
                          , pdp->flags.bDirect );
#endif
            ChildCalculate( pmdp );
            LeaveCriticalSec( &pmdp->Lock );
         }
         else if( TextLike( temp, WIDE("buffer") ) )
         {
            pmdp->flags.bCharMode = FALSE;
         }
         else if( TextLike( temp, WIDE("TEXTCHAR") ) )
         {
            pmdp->flags.bCharMode = TRUE;
         }
      }
      //else
      //{
      //  DECLTEXT( msg, WIDE("Command path is not a PSI Console") );
      //  EnqueLink( &ps->Command->Output, &msg );
      //}
   }
   if( !firstopt )
   {
      DECLTEXT( msg, WIDE("Must specify mode: Direct, Line.") );
      EnqueLink( &ps->Command->Output, &msg );
   }
   return 0;
}
#endif

//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
#ifdef WINCON
int SetFlash( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );
#endif
#endif

//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
TEXTCHAR *ColorTable[] = { WIDE("black"), WIDE("blue"), WIDE("green")
                     , WIDE("cyan"), WIDE("red"), WIDE("magenta")
                     , WIDE("brown"), WIDE("grey"), WIDE("darkgrey")
                     , WIDE("lightblue"), WIDE("lightgreen")
                     , WIDE("lightcyan"), WIDE("lightred")
                     , WIDE("lightmagenta"), WIDE("yellow")
                     , WIDE("white") };

int ConSetColor( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   // parameters are 1 or 2 words...
   PTEXT color;
   int i;
   PCONSOLE_INFO pmdp;
   pmdp = (PCONSOLE_INFO)pdp;
   color = GetParam( ps, &parameters );
   if( !color )
   {
      DECLTEXT( msg, WIDE("Parameters to SetColor are : Foreground Background") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
   for( i = 0; i < 16; i++ )
   {
      if( TextLike( color, ColorTable[i] )  )
      {
         break;
      }
   }
   if( i < 16 )
   {
      SetHistoryDefaultForeground( pmdp->pCursor, i );
   }
   else
   {
      DECLTEXT( msg, WIDE("First parameter was not a known color...") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }

   color = GetParam( ps, &parameters );
   if( color )
   {
      for( i = 0; i < 16; i++ )
      {
         if( TextLike( color, ColorTable[i] ) )
         {
            break;
         }
      }
      if( i < 16 )
      {
         SetHistoryDefaultBackground( pmdp->pCursor, i );
      }
      else
      {
         //DECLTEXT( msg, WIDE("Second parameter was not a known color...") );
         //EnqueLink( &ps->Command->Output, &msg );
         //pmdp->Display.History->pCursor->DefaultColor.background
         // = pmdp->Display.History->pCursor->PriorColor.background;
      }
   }
#if defined( WINCON )
    //SetRegistryInt( WIDE("Dekware\\Wincon\\Background")
    //                , GetText( GetName( pmdp->ps->Current ) )
    //                , pmdp->Display.History->pCursor->DefaultColor.background );
    //SetRegistryInt( WIDE("Dekware\\gggWincon\\Foreground")
    //                , GetText( GetName( pmdp->ps->Current ) )
    //                , pmdp->Display.History->pCursor->DefaultColor.foreground );
#endif
   return 1;
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
int SetTabs( PDATAPATH pdp, PSENTIENT ps, PTEXT params )
{
    PTEXT temp;
    temp = GetParam( ps, &params );
    if( temp )
    {
        PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;
        if( !pmdp )
        {
            DECLTEXT( msg, WIDE("Window datapath is not open") );
            EnqueLink( &ps->Command->Output, &msg );
            return 0;
        }
        else
		  {

            pmdp->pHistory->tabsize = (S_32)IntNumber( temp );
        }
    }
    else
    {
        PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;
        if( !pmdp )
        {
            DECLTEXT( msg, WIDE("Window datapath is not open") );
            EnqueLink( &ps->Command->Output, &msg );
            return 0;
        }
        else
        {
            DECLTEXT( msg, WIDE("Tab size not specified defaulting to 8.") );
            EnqueLink( &ps->Command->Output, &msg );
            pmdp->pHistory->tabsize = 8;
        }
    }
    return 0;
}
#endif
//----------------------------------------------------------------------------

int UpdateHistory( PCONSOLE_INFO pdp )
{
   int bUpdate = 0;
	lprintf( WIDE("nLines = %d  percent = %d  x = %d")
			 , pdp->nLines
			 , pdp->nHistoryPercent
			 , ( pdp->nLines * ( 3 - pdp->nHistoryPercent ) / 4 ) );
   EnterCriticalSec( &pdp->Lock );
	if( GetBrowserDistance( pdp->pHistoryDisplay ) >
		( pdp->nLines * ( 3 - pdp->nHistoryPercent ) / 4 ) )
	{
		if( !pdp->flags.bHistoryShow )
		{
         extern PSIKEYDEFINE KeyDefs[];
			lprintf( WIDE("Key END shoudl end history..") );
			KeyDefs[KEY_END].op[0].bFunction = HISTORYKEY;
			KeyDefs[KEY_END].op[0].data.HistoryKey = KeyEndHst;
			pdp->flags.bHistoryShow = 1;
			WinLogicCalculateHistory( pdp ); // this builds history and real display info lines.
         bUpdate = 1;
		}
		else
		{
			PENDING_RECT upd;
			upd.flags.bHasContent = 0;
			upd.flags.bTmpRect = 0;
         MemSet( &upd.cs, 0, sizeof( upd.cs ) );
			BuildDisplayInfoLines( pdp->pHistoryDisplay );
			//lprintf( WIDE("ALready showing history?!") );
			if( pdp->psicon.frame )
				SmudgeCommon( pdp->psicon.frame );
         else
				DoRenderHistory(pdp, TRUE, &upd);

			// history only changed - safe to update
         // its content on result here...
			if( pdp->Update && upd.flags.bHasContent )
			{
				RECT r;
				r.left = upd.x;
				r.right = upd.x + upd.width;
				r.top = upd.y;
				r.bottom = upd.y + upd.height;
				pdp->Update( pdp, &r );
			}
		}
	}
	else
	{
		if( pdp->flags.bHistoryShow )
		{
         extern PSIKEYDEFINE KeyDefs[];
			lprintf( WIDE("key end command line now... please do renderings..") );
			KeyDefs[KEY_END].op[0].bFunction = COMMANDKEY;
			KeyDefs[KEY_END].op[0].data.CommandKey = KeyEndCmd;
			pdp->flags.bHistoryShow = 0;
			WinLogicCalculateHistory( pdp );
         bUpdate = 1;
		}
	}
   LeaveCriticalSec( &pdp->Lock );
   return bUpdate;
}

