//#define NO_LOGGING
//#define DEBUG_HISTORY_RENDER
//#define DEBUG_REGION_UPDATES
//#define DEBUG_OUTPUT_TO_CONSOLE

#include "consolestruc.h"
//#include "interface.h"
#include "history.h"

#define WINLOGIC_SOURCE

#include "WinLogic.h"

#include "histstruct.h"
PSI_CONSOLE_NAMESPACE
//----------------------------------------------------------------------------

static void AddUpdateRegion( PPENDING_RECT update_rect, int32_t x, int32_t y, uint32_t wd, uint32_t ht )
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
				update_rect->width = (wd + x) - update_rect->x;

			if( y < update_rect->y )
			{
				update_rect->height += update_rect->y - y;
				update_rect->y = y;
			}
			if( y + ht > update_rect->y + update_rect->height )
				update_rect->height = (y + ht) - update_rect->y;
#ifdef DEBUG_REGION_UPDATES
			lprintf( "result (%d,%d)-(%d,%d)"
					, update_rect->x, update_rect->y
					, update_rect->width, update_rect->height
			  	);
#endif
		}
		else
		{
#ifdef DEBUG_REGION_UPDATES
			lprintf( "Setting (%d,%d)-(%d,%d)"
					, x, y
					, wd, ht
					);
#endif
			update_rect->x = x;
			update_rect->y = y;
			update_rect->width = wd;
			update_rect->height = ht;
		}
		update_rect->flags.bHasContent = 1;
	}
#ifdef __LINUX__
	if( !update_rect->flags.bTmpRect ) {
		LeaveCriticalSec( &update_rect->cs );
	}
#endif
}

static void RenderTextLine( 
	PCONSOLE_INFO pdp
	, PDISPLAYED_LINE pCurrentLine
	, RECT *r
	, int nLine
	, int nFirst
	, int nFirstLine
	, int nMinLine
	, LOGICAL mark_applies
	, LOGICAL allow_segment_coloring
	, int leadinOffset
	, int nBottomOffset
	, int bClearEnd
						)
{
	// left and right are relative... to the line segment only...
	// for the reason of color changes inbetween segments...
	{
		int justify = 0;
		int x, y;
		int nChar;
		PTEXT pText;
		int nShow, nShown;
#ifdef DEBUG_HISTORY_RENDER
		lprintf( "Rect is %d-%d   %d-%d", r->left, r->right, r->top, r->bottom );
		lprintf( "Get display line %d", nLine );
#endif
		if( !pCurrentLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( "No such line... %d", nLine );
#endif
			return;
		}
		(*r).top = pCurrentLine->nLineTop + nBottomOffset;
		if( nFirst >= 0 )
			(*r).bottom = (*r).top + pCurrentLine->nLineHeight + 2;
		else
			(*r).bottom = (*r).top + pCurrentLine->nLineHeight;
		if( (*r).bottom <= nMinLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( "bottom < minline.." );
#endif
			return;
		}
		y = (*r).top;
#ifdef DEBUG_HISTORY_RENDER
		lprintf( "Y STARTS AT %d", y );
#endif

		if( !pCurrentLine->nPixelStart ) {
			r->left = 0;
			r->right = pdp->nXPad;
			if( pdp->FillConsoleRect )
				pdp->FillConsoleRect(pdp, r, FILL_DISPLAY_BACK );
		}
		else {
			r->right = pdp->nXPad;
		}

		x = (*r).left = r->right + pCurrentLine->nPixelStart;
		(*r).right = (*r).left;

		//(*r).left = x;
		nChar = 0;
		pText = pCurrentLine->start;
		if( pText && ( pText->flags & TF_FORMATEX ) )
		{
			if( pText->format.flags.format_op == FORMAT_OP_JUSTIFY_RIGHT )
			{
				justify = 1;
			}
		}
		//lprintf( "Rect is %d-%d   %d-%d", r->left, r->right, r->top, r->bottom );

		if( allow_segment_coloring )
			if( pdp->SetCurrentColor )
				pdp->SetCurrentColor( pdp, COLOR_DEFAULT, NULL );
		nShown = (int)pCurrentLine->nFirstSegOfs;
#ifdef DEBUG_HISTORY_RENDER
		if( !pText )
			lprintf( "Okay no text to show... end up filling line blank." );
#endif
		while( pText )
		{
			size_t nLen;
			TEXTCHAR *text = GetText( pText );
#ifdef __DEKWARE_PLUGIN__
			if( !pdp->flags.bDirect && ( pText->flags & TF_PROMPT ) )
			{
				lprintf( "Segment is promtp - and we need to skip it." );
				pText = NEXTLINE( pText );
				continue;
			}
#endif
			if( allow_segment_coloring )
				if( pdp->SetCurrentColor )
					pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );

			nLen = GetTextSize( pText );
#ifdef DEBUG_HISTORY_RENDER
			lprintf( "start: %d  len: %d", nShown, nLen );
#endif
			while( nShown < nLen )
			{
#ifdef DEBUG_HISTORY_RENDER
				lprintf( "nShown < nLen... char %d len %d toshow %d", nChar, nLen, pCurrentLine->nToShow );
#endif
				if( USS_GT( nLen, size_t, pCurrentLine->nToShow- nChar, int ) )
					nShow = (int)(pCurrentLine->nToShow - nChar);
				else
				{
#ifdef DEBUG_HISTORY_RENDER
					lprintf( "nShow is what's left of now to nLen from nShown... %d,%d", nLen, nShown );
#endif
					nShow = (int)(nLen - nShown);
				}
				if( !nShow )
				{
					//lprintf( "nothing to show..." );
					break;
				}
				if( pdp->flags.bMarking &&
					mark_applies )
				{
					if( !pdp->flags.bMarkingBlock )
					{
						if( ( nLine ) > pdp->mark_start.row
						 ||( nLine ) < pdp->mark_end.row )
						{
							// line above or below the marked area...
							if( allow_segment_coloring )
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
										nShow = (int)(pdp->mark_end.col - nChar);
									if( allow_segment_coloring )
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
									//SetCurrentColor( pdp->crMark
									//  				, pdp->crMarkBackground );
								}
								else if( nChar >= pdp->mark_end.col )
								{
									if( allow_segment_coloring )
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );
									//SetCurrentColor( crThisText, crThisBack );
								}
								else if( nChar + nShow > pdp->mark_start.col )
								{
									nShow = (int)(pdp->mark_start.col - nChar);
								}
							}
							else
							{
								if( nLine == pdp->mark_start.row )
								{
									if( nChar >= pdp->mark_start.col )
									{
										if( allow_segment_coloring )
											if( pdp->SetCurrentColor )
												pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
										//SetCurrentColor( pdp->crMark
										//					, pdp->crMarkBackground );
									}
									else if( nChar + nShow > pdp->mark_start.col )
									{
										// current segment up to the next part...
										nShow = (int)(pdp->mark_start.col - nChar);
									}
								}
								if( ( nLine ) < pdp->mark_start.row
								 &&( nLine ) > pdp->mark_end.row )
								{
									if( allow_segment_coloring )
										if( pdp->SetCurrentColor )
											pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
									//SetCurrentColor( pdp->crMark
									//					, pdp->crMarkBackground );
								}
								if( ( nLine ) == pdp->mark_end.row )
								{
									if( nChar >= pdp->mark_end.col )
									{
										if( allow_segment_coloring )
											if( pdp->SetCurrentColor )
												pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );
										//SetCurrentColor( crThisText, crThisBack );
									}
									else if( nChar + nShow > pdp->mark_end.col )
									{
										nShow = pdp->mark_end.col - nChar;
										if( allow_segment_coloring )
											if( pdp->SetCurrentColor )
												pdp->SetCurrentColor( pdp, COLOR_MARK, pText );
										//SetCurrentColor( pdp->crMark
										//					, pdp->crMarkBackground );
									}
									else if( nChar < pdp->mark_end.col )
									{
										if( allow_segment_coloring )
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
				//lprintf( "Rect is %d-%d   %d-%d", r->left, r->right, r->top, r->bottom );
				//lprintf( "Some stats %d %d %d", nChar, nShow, nShown );
				if( nChar )
				{
					// not first character on line...
					x = (*r).left = (*r).right;
				}
				else {
					if( !(*r).right ) {
						if( justify == 1 ) {
							x = (*r).right = (pdp->nWidth - (pdp->nXPad + pCurrentLine->nPixelEnd)) + (*r).left;
						}
						else if( justify == 2 ) {
							x = (*r).right = ((pdp->nWidth - pCurrentLine->nPixelEnd) / 2) + (*r).left;
						}
						else if( !(*r).left )
							(*r).right = (*r).left + pdp->nXPad;
						else
							(*r).right = (*r).left;
						if( r->right != r->left )
							if( pdp->FillConsoleRect )
								pdp->FillConsoleRect( pdp, r, FILL_DISPLAY_BACK );
					}
				}
				//(*r).right = pdp->nXPad + pCurrentLine->nPixelEnd;
				(*r).right = (*r).left + pCurrentLine->nPixelEnd;
				if( (*r).bottom > nMinLine )
				{
					uint32_t nSegSize, nSegHeight;
#ifdef DEBUG_HISTORY_RENDER
					lprintf( "And finally we can show some text... %s %d", text, y );
#endif
					(*r).left = x;
#ifdef DEBUG_HISTORY_RENDER
					lprintf( "putting string %s at %d,%d (left-right) %d,%d", text, x, y, (*r).left, (*r).right );
#endif
					if( pdp->pHistoryDisplay->measureString )
						pdp->pHistoryDisplay->measureString( pdp->pHistoryDisplay->psvMeasure, GetText( pText ) + nShown
							, nShow, &nSegSize, &nSegHeight, GetCommonFont( pdp->psicon.frame ) );
					(*r).right = (*r).left + nSegSize;
					if( pdp->DrawString )
						//pdp->DrawString( pdp, x, y, r, text, nShown, nShow );
						pdp->DrawString( pdp, (*r).left, y, nLine?0:nSegHeight, r, text, nShown, nShow );
					(*r).left = (*r).right;
					if( nLine == 0 )  // only keep the (last) line's end.
						pdp->nNextCharacterBegin = (*r).right;
					//DrawString( text );
					//lprintf( "putting string %s at %d,%d (left-right) %d,%d", text, x, y, (*r).left, (*r).right );
				}
#ifdef DEBUG_HISTORY_RENDER
				else
					lprintf( "Hmm bottom < minline?" );
#endif
				// fill to the end of the line...
				//nLen -= nShow;
				nShown += nShow;
				nChar += nShow;
			}
#ifdef DEBUG_HISTORY_RENDER
			lprintf( "nShown >= nLen..." );
#endif

			nShown -= (int)nLen;
			pText = NEXTLINE( pText );
		}
		{
			x = (*r).left = (*r).right;
			(*r).right = pdp->nWidth;
			// if soething left to fill, blank fill it...
			if( (*r).left < (*r).right )
			{
#ifdef DEBUG_HISTORY_RENDER
				lprintf( "Fill empty to right (%d-%d)  (%d-%d)", (*r).left, (*r).right, (*r).top, (*r).bottom );
#endif
				//if( nLine || bClearEnd )
				//	if( pdp->FillConsoleRect )
				//		pdp->FillConsoleRect( pdp, r, FILL_DISPLAY_BACK );
				//FillConsoleRect();
			}
		}
		if( nFirst >= 0 )
			nFirst = -1;
		nLine++;
	}
	//lprintf( "(*(*r)...bottom nMin %d %d", (*r)..bottom, nMinLine );
	if( (*r).bottom > nMinLine )
	{
		(*r).bottom = (*r).top;
		(*r).top = nMinLine;
		(*r).left = 0;
		(*r).right = pdp->nWidth;
#ifndef PSI_LIB
		//lprintf( "Would be blanking the screen here, but no, there's no reason to." );
				 //FillEmptyScreen();
#endif
	}
}

void PSI_RenderCommandLine( PCONSOLE_INFO pdp, PENDING_RECT *region )
{
	PENDING_RECT myrect;
	// need to render the current macro being recorded.....
	RECT upd;
	SFTFont font = GetCommonFont( pdp->psicon.frame );
	RECT r;
	int nMaxLen, nShow, nCurrentCol, x, y, nCursorPos;
	int nCursorIdx;
	int lines;
	int nLeadinoffset;
	int line_offset;
	int nShown;
	int start, end;
	int toppad = 0;
	PTEXT pStart;
	// no command line...
	if( !pdp->CommandInfo )
	{
		// region->bHasContent = 0?
		return;
	}
#ifdef DEBUG_HISTORY_RENDER
	lprintf( "Begin render command line" );
#endif
	if( !region )
	{
		region = &myrect;
		myrect.flags.bHasContent = 0;
		myrect.flags.bTmpRect = 1;
	}

	if( pdp->SetCurrentColor )
		pdp->SetCurrentColor( pdp, COLOR_COMMAND, NULL );//pdp->crCommand, pdp->crCommandBackground );

	nCursorIdx = GetCommandCursor( pdp->pCurrentDisplay, font
											, pdp->CommandInfo
											, pdp->flags.bDirect
											, pdp->flags.bWrapCommand
											, &nCurrentCol
											, &start
											, &end
											, &nLeadinoffset
											, &nCursorPos
											, &line_offset);
	// nYpad at bottom of screen, font height up begins the top of the
	// output text line...

	// also line starts are considered from the bottom up...
	// nXPad,n__LineStart
	r.top = y = pdp->nCommandLineStart - pdp->nFontHeight; //****** THIS PROBABLY NEEDS AN OFFSET ********/
	r.bottom = pdp->nHeight;
	if( !pdp->flags.bDirect )
		toppad = pdp->nCmdLinePad;
	r.top -= toppad;
/*
	lprintf( "*** Commandline %d,%d  uhh %d %d  %d and %d"
			, r.top, r.bottom
			, start, end
			, nCursorPos
			 , nCurrentCol
			 );
			 */
	if( !nCurrentCol && !nLeadinoffset )
	{
		// need to blatcolor for the 5 pixels left of first char...
		r.left = 0;
		r.right = 0;
		/*
		if( pdp->FillConsoleRect )
		{
			lprintf( "draw blank to left %d-%d   %d-%d", r.left, r.right, r.top, r.bottom );
			pdp->FillConsoleRect( pdp, &r, FILL_COMMAND_BACK );
		}
		*/
		upd.left = 0;
	}
	else
	{
		if( pdp->flags.bDirect )
			r.right = nLeadinoffset;
		else
			r.right = pdp->nXPad;
	}

	//r.left = x = pdp->nXPad + ( nCurrentCol * pdp->nFontWidth );
	r.left = x = r.right;
	//lprintf( "x/left is %d", x );
	// for now...
	upd.left = x;
	upd.right = pdp->nWidth;
	upd.top = pdp->nDisplayLineStartDynamic - pdp->nFontHeight;
	upd.bottom = r.bottom;

	//lprintf( "start rect %d,%d  %d,%d, upd: %d,%d  %d,%d", r.left, r.top, r.left, r.right, upd.left, upd.top, upd.right, upd.bottom );

	{
		// totally set the background of the command thingy...
		// previously the putstring would have done the rect fill...
		// but now we need to just put text data over a solid backgorund...
		r.right = pdp->nWidth;
		//r.top -= toppad;
		if( pdp->FillConsoleRect )
			pdp->FillConsoleRect( pdp, &r, FILL_COMMAND_BACK );
		//r.top += toppad;
	}

	// the normal prompt string will have the current
	// macroname being recorded... do not show this in
	// direct mode.....
	nMaxLen = end - start;

	nShown = start;
	pStart = pdp->CommandInfo->CollectionBuffer;
	SetStart( pStart );
	if( pdp->flags.bWrapCommand )
	{
		int skip_lines;
		PDISPLAYED_LINE pdlCommand;
		skip_lines = 0;
		//lprintf( "Command line is wrapped ?" );
		lines = CountDisplayedLines( pdp->pCommandDisplay );
		if( !lines )
			lines = 1;
		if( lines > 1 )
			upd.left = 0;
#ifdef DEBUG_OUTPUT
		lprintf( "want to do this in %d lines", lines );
#endif
		if( lines > 3 )
		{
			skip_lines = lines - 3;
			lines = 3;
		}
		if( pdp->flags.bDirect )
		{
			// should already be set from last doStroke
			//pdp->nDisplayLineStartDynamic = ((PDISPLAYED_LINE)GetDataItem( &pdp->pCommandDisplay->DisplayLineInfo
			//	, pdp->pCommandDisplay->DisplayLineInfo->Cnt - 1 ))->nLineTop;
		}
		else
		{
			pdlCommand = (PDISPLAYED_LINE)GetDataItem( GetDisplayInfo( pdp->pCommandDisplay ), lines-1 );
			//lprintf( "Input parameters to setting line %d %d %d %d", pdp->nCommandLineStart
			//					, pdlCommand->nLineTop, pdp->nYPad, pdp->nCmdLinePad );
			pdp->nDisplayLineStartDynamic = pdlCommand->nLineTop - (
					+ ( pdp->nYPad ) // one at bottom, one above separator
					+ ( pdp->nCmdLinePad / 2)  // extraa width around command line
					 ) ;
			//lprintf( "(direct write command line)Setting display line to %d", pdp->nDisplayLineStartDynamic );
		}

#ifdef DEBUG_REGION_UPDATES
		lprintf( "RenderCommandLine mid" );
#endif
		AddUpdateRegion( region, upd.left, upd.top, upd.right-upd.left,upd.bottom-upd.top );

		{
			int nLine;
			DISPLAYED_LINE *pCurrentLine;
			PDATALIST *ppCurrentLineInfo;
			ppCurrentLineInfo = GetDisplayInfo( pdp->pCommandDisplay );
			for( nLine = 0; nLine < 3; nLine ++ )
			{
				pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
				if( !pCurrentLine )
				{
		#ifdef DEBUG_HISTORY_RENDER
					lprintf( "No such line... %d", nLine );
		#endif
					break;
				}
#ifdef DEBUG_HISTORY_RENDER
				lprintf( "Something else %d %d", pdp->nCommandLineStart, pCurrentLine->nLineTop );
#endif
				if( pCurrentLine->start )
					RenderTextLine( pdp, pCurrentLine, &upd
						, nLine, TRUE, y, pdp->nCommandLineStart - pCurrentLine->nLineTop
						, FALSE
						, FALSE, nLeadinoffset, 0, 1 );  // cursor; to know where to draw the mark...
				y -= pdp->nFontHeight;
				nLeadinoffset = 0;
			}
		}
		if( pdp->RenderCursor )
			// rect has top/bottom info, and current cursor position column
			// is passed - each client will be able to
			pdp->RenderCursor( pdp, &r, ( nCursorPos ) ); // top/bottom are the line...
	}
	else
	{
		lprintf( "not wrapping command line..." );
		if( pdp->flags.bDirect ) {
			y -= pdp->pCommandDisplay->nLineHeight;
			r.top = y;
		}
		while( pStart && SUS_GT( nShown, int, GetTextSize( pStart ), size_t ) )
		{
			nShown -= (int)GetTextSize( pStart );
			pStart = NEXTLINE( pStart );
		}

		while( pStart && nCurrentCol < nCursorIdx )
		{
			nShow = (int)GetTextSize( pStart ) - nShown;

			if( nCurrentCol + nShow > end )
				nShow = end - nCurrentCol;

			if( pdp->DrawString ) {
				uint32_t width, height;
#ifdef DEBUG_HISTORY_RENDER
				lprintf( "(2)putting string %s at %d,%d (left-right) %d,%d", GetText( pStart ), x, y, (r).left, (r).right );
#endif
				pdp->DrawString( pdp, x, y, 0, &r, GetText( pStart ), nShown, nShow );
				pdp->pCommandDisplay->measureString( pdp->pCommandDisplay->psvMeasure
					, GetText( pStart ) + nShown, nShow
					, &width, &height, font );
				x += width;
			}
			r.left = x;
			//x = r.left = r.right;
			nShown = 0;
			nCurrentCol += nShow;
			pStart = NEXTLINE( pStart );
		}
		//if( pStart )
		//	lprintf( "Stopped because of length." );
		r.left = r.right;
		r.right = pdp->nWidth;
		// only have to clean trail if on a direct input method...
		if( r.right > r.left )
		{
			// clear the remainder of the line...
//#ifdef DEBUG_HISTORY_RENDER
			lprintf( "Clearing end of line... %d-%d   %d-%d", r.left, r.right, r.top, r.bottom );
//#endif
			if( pdp->FillConsoleRect )
				pdp->FillConsoleRect( pdp, &r, FILL_DISPLAY_BACK );
		}
		if( pdp->RenderCursor )
			// rect has top/bottom info, and current cursor position column
			// is passed - each client will be able to
			pdp->RenderCursor( pdp, &r, ( nCursorPos ) ); // top/bottom are the line...

		// command line only update ? maybe add this to regions which should be updated?
		// refresh here?
#ifdef DEBUG_REGION_UPDATES
		lprintf( "RenderCommandLine end" );
#endif
		AddUpdateRegion( region, upd.left, upd.top, upd.right-upd.left,upd.bottom-upd.top );

	}
#ifdef DEBUG_HISTORY_RENDER
	lprintf( "done render command line" );
#endif
}


//----------------------------------------------------------------------------
// 5 on left, 5 on right total 10 pixels we can't use...

void PSI_WinLogicCalculateHistory( PCONSOLE_INFO pdp, SFTFont font )
{
	// there's some other related set of values to set here....
	//lprintf( "Calculate history! %d %d", pdp->nColumns, pdp->nLines );
	if( !pdp->nFontHeight )
		return;

	SetCursorHeight( pdp->pCursor, pdp->nHeight - pdp->nYPad*2 );
	SetCursorWidth( pdp->pCursor, pdp->nWidth - pdp->nXPad * 2 );
	if( pdp->pHistoryDisplay )
		SetBrowserWidth( pdp->pHistoryDisplay, pdp->nWidth - pdp->nXPad*2 );
	SetBrowserWidth( pdp->pCurrentDisplay, pdp->nWidth - pdp->nXPad*2 );
	SetBrowserHeight( pdp->pCurrentDisplay, pdp->nHeight );

	pdp->pCurrentDisplay->nLineHeight = pdp->nFontHeight;
	if( pdp->pHistoryDisplay )
		pdp->pHistoryDisplay->nLineHeight = pdp->nFontHeight;
	if( pdp->pCommandDisplay )
		pdp->pCommandDisplay->nLineHeight = pdp->nFontHeight;

	//lprintf( "Don't forget we wanted to inset command for a button..." );
	if( pdp->pCommandDisplay )
		SetBrowserWidth( pdp->pCommandDisplay, pdp->nWidth - pdp->nXPad*2 );
	
	if( pdp->flags.bHistoryShow )
	{
		//lprintf( "Doing history... check percent and set display/history approp." );
		switch( pdp->nHistoryPercent )
		{
		case 0:  // 25
		case 1: //50
		case 2: //75
			{
				if( pdp->pHistoryDisplay )
				{
					int nWorkLines;
					int displayHeight;
					nWorkLines = ( pdp->nHeight / pdp->nFontHeight ) * ( 3 - pdp->nHistoryPercent ) / 4;
					displayHeight = pdp->nYPad * 2 + nWorkLines * pdp->nFontHeight;
					SetBrowserHeight( pdp->pCurrentDisplay, displayHeight );
					SetBrowserHeight( pdp->pHistoryDisplay, pdp->nHeight - displayHeight );
					PSI_SetHistoryPageLines( pdp->pHistoryDisplay, (pdp->nLines - nWorkLines)-3 );
					pdp->nHistoryLineStart = pdp->nDisplayLineStartDynamic - ( pdp->nHeight * ( 3 - pdp->nHistoryPercent ) ) / 4;
				}
			}
			break;
		case 3: //100
			if( pdp->pHistoryDisplay )
			{
				pdp->nHistoryLineStart = pdp->nDisplayLineStartDynamic;
				SetBrowserHeight( pdp->pHistoryDisplay, pdp->nHeight );
				// need this to know how far close to end we can get...
				SetBrowserHeight( pdp->pCurrentDisplay, 0 );
				PSI_SetHistoryPageLines( pdp->pHistoryDisplay, pdp->nLines );
				//pdp->nHistoryLines = nLines;
				//pdp->nDisplayLines = 0;
			}
		}
	}
	else
	{
		pdp->nHistoryLineStart = 0;
		if( pdp->pHistoryDisplay )
		{
			//lprintf( "No history, all display" );
			// internally we'll need this amount to get into
			// scrollback...
			{
				int nWorkLines;
				nWorkLines = ( pdp->nLines * ( 1 + pdp->nHistoryPercent ) ) / 4;
				PSI_SetHistoryPageLines( pdp->pHistoryDisplay, nWorkLines - 3 );
			}
			PSI_SetHistoryPageLines( pdp->pHistoryDisplay, pdp->nHeight/pdp->nFontHeight - 4 );
			SetBrowserHeight( pdp->pHistoryDisplay, 0 );
			ResetHistoryBrowser( pdp->pHistoryDisplay );
			// 1 for the partial line at the top of the display.
		}
		SetBrowserHeight( pdp->pCurrentDisplay, pdp->nHeight );
	}
//#endif
	if( pdp->pHistoryDisplay ) {
		SetBrowserFirstLine( pdp->pHistoryDisplay, pdp->nHistoryLineStart );
		pdp->pHistoryDisplay->flags.bUpdated = 1;
		BuildDisplayInfoLines( pdp->pHistoryDisplay, NULL, font );
	}
	SetBrowserFirstLine( pdp->pCurrentDisplay, pdp->nDisplayLineStartDynamic - SEPARATOR_HEIGHT - pdp->nYPad );
	pdp->pCurrentDisplay->flags.bUpdated = 1;
	BuildDisplayInfoLines( pdp->pCurrentDisplay, NULL, font );
	if( pdp->pCommandDisplay ) {
#ifdef DEBUG_HISTORY_RENDER
		lprintf( "ncommand line start: %d", pdp->nCommandLineStart );
#endif
		SetBrowserFirstLine( pdp->pCommandDisplay, pdp->nCommandLineStart );
		pdp->pCommandDisplay->flags.bUpdated = 1;
		BuildDisplayInfoLines( pdp->pCommandDisplay, pdp->flags.bDirect ? pdp->pCurrentDisplay : NULL, font );
	}
}

//----------------------------------------------------------------------------
static void DoRenderHistory( PCONSOLE_INFO pdp, int bHistoryStart, int nBottomLineOffset, int nStartLineOffset, PENDING_RECT *region );

void PSI_RenderConsole( PCONSOLE_INFO pdp, SFTFont font )
{
	PENDING_RECT upd;
	upd.flags.bHasContent = 0;
	upd.flags.bTmpRect = 1;
	MemSet( &upd.cs, 0, sizeof( upd.cs ) );
	EnterCriticalSec( &pdp->Lock );
	pdp->lockCount++;
	/*
	lprintf( "Render Console... %d  %d %d  %d"
			, pdp->nDisplayLineStartDynamic, pdp->nCommandLineStart
		, pdp->nHistoryLineStart, pdp->nHeight );
	*/
#ifdef DEBUG_HISTORY_RENDER
	lprintf( "Render Console... %d %d", pdp->nDisplayLineStartDynamic, pdp->nHistoryLineStart );
#endif

	if( pdp->RenderSeparator )
	{
		if( !pdp->flags.bDirect && pdp->nDisplayLineStartDynamic != pdp->nCommandLineStart )
			pdp->nSeparatorHeight = pdp->RenderSeparator( pdp, pdp->nDisplayLineStartDynamic );
		//lprintf( "Render AGAIN the hsitory line separator" );
		if( pdp->nHistoryLineStart && pdp->nHistoryLineStart != pdp->nDisplayLineStartDynamic )
			pdp->nSeparatorHeight = pdp->RenderSeparator( pdp, pdp->nHistoryLineStart );
	}

	if( !pdp->flags.bDirect )
		PSI_RenderCommandLine( pdp, &upd );

	SetBrowserFirstLine( pdp->pCurrentDisplay, pdp->nDisplayLineStartDynamic );
	BuildDisplayInfoLines( pdp->pCurrentDisplay, NULL, font );

	if( pdp->pCommandDisplay ) {
		//if( (pdp->flags.bDirect && !pdp->flags.bCharMode) ) {
		//	BuildDisplayInfoLines( pdp->pCommandDisplay, pdp->pCurrentDisplay, font );
		//}
		PSI_RenderCommandLine( pdp, &upd );
	}

	// if direct rendering, command line doesn't change, so it doesn't need to be redrawn here.

	// if history is showing, first line is below the top (> 0 )
	if( pdp->nHistoryLineStart )
	{
		DoRenderHistory( pdp, TRUE, 0, 0, &upd );
	}
	// if there's a section of display left to render between history and command
	if( pdp->nDisplayLineStartDynamic != pdp->nHistoryLineStart )
	{
		DoRenderHistory( pdp, FALSE, 0, 0, &upd);
	}

	if( pdp->Update && upd.flags.bHasContent )
	{
		RECT r;
		r.left = upd.x;
		r.right = upd.x + upd.width;
		r.top = upd.y;
		r.bottom = upd.y + upd.height;
		//lprintf( "Update: %d %d  %d %d", r.left, r.top, upd.width, upd.height );
		pdp->Update( pdp, &r );
	}
	pdp->lockCount--;
	LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------

// called for initialization, but any time some size on the display changes (history height
// or window size.
void PSI_ConsoleCalculate( PCONSOLE_INFO pdp, SFTFont font )
{
	//RECT rArea;
	//lprintf( "*** DISPLAY is %d,%d by %d,%d", pdp->rArea.top, pdp->rArea.left, pdp->rArea.right, pdp->rArea.bottom );
	if( ( ( pdp->rArea.right -  pdp->rArea.left )== 0 )
		&& ( ( pdp->rArea.bottom -  pdp->rArea.top )== 0 ) )
		return;
	if ( (pdp->rArea.right - pdp->rArea.left) <= 0 || 
			(pdp->rArea.bottom - pdp->rArea.top ) <= 0 )
	{
		// not enough size to display anything.... disable display
		 pdp->flags.bNoDisplay = 1;
	}
	else
		 pdp->flags.bNoDisplay = 0;

	pdp->nWidth = pdp->rArea.right - pdp->rArea.left;
	pdp->nHeight = pdp->rArea.bottom - pdp->rArea.top;
	if( pdp->nWidth &0x80000000 || pdp->nHeight &0x80000000 )
	{
		pdp->nWidth = 0;
		pdp->nHeight = 0;
	}
#ifdef DEBUG_HISTORY_RENDER
	lprintf( "-------- set command line to area : %d", pdp->rArea.bottom );
#endif
	pdp->nCommandLineStart = pdp->rArea.bottom;

	//lprintf( "Okay font height existsts... that's good" );
	if( pdp->flags.bDirect )
	{
		SetCursorNoPrompt( pdp->pCurrentDisplay, FALSE );
		if( pdp->pHistoryDisplay )
			SetCursorNoPrompt( pdp->pHistoryDisplay, FALSE );
		pdp->nCommandLineStart -= pdp->nYPad;
#ifdef DEBUG_HISTORY_RENDER
		lprintf( "(direct)Setting display line to %d", pdp->nCommandLineStart );
#endif
		pdp->nDisplayLineStartDynamic = pdp->nCommandLineStart;
	}
	else
	{
		SetCursorNoPrompt( pdp->pCurrentDisplay, TRUE );
		if( pdp->pHistoryDisplay )
			SetCursorNoPrompt( pdp->pHistoryDisplay, FALSE );
		{
			PDISPLAYED_LINE pdlCommand;
			int lines = CountDisplayedLines( pdp->pCommandDisplay );

			pdlCommand = (PDISPLAYED_LINE)GetDataItem( GetDisplayInfo( pdp->pCommandDisplay ), lines - 1 );

			pdp->nCommandLineStart -= pdp->nYPad + pdp->nCmdLinePad / 2;
			pdp->nDisplayLineStartDynamic = (pdlCommand?pdlCommand->nLineTop: pdp->nCommandLineStart -pdp->nFontHeight
					- ( pdp->nYPad + ( pdp->nCmdLinePad / 2 ) ) ) ;
		}
#ifdef DEBUG_HISTORY_RENDER
		lprintf( "(linemode)Setting display line to %d", pdp->nDisplayLineStartDynamic );
#endif
	}

lprintf( "------- WINLOGIC CALCULATE -------- %p ", pdp->UpdateSize);
	PSI_WinLogicCalculateHistory( pdp, font );

	PSI_RenderConsole( pdp, font );

	if( pdp->UpdateSize ) {
		pdp->UpdateSize( pdp->psvUpdateSize, pdp->nColumns, pdp->nLines, pdp->nWidth, pdp->nHeight );
	}
}

//----------------------------------------------------------------------------

// this does assume that special formatting text packets are spoon-fed to it.
PSI_Console_Phrase PSI_WinLogicWriteEx( PCONSOLE_INFO pmdp
						, PTEXT pLine
						, int update
						)
{
	PSI_Console_Phrase result = NULL;
	EnterCriticalSec( &pmdp->Lock );
	pmdp->lockCount++;
	{
		int once = 1;
#ifdef DEBUG_OUTPUT_TO_CONSOLE
		PTEXT line = BuildLine( pLine );
		lprintf( "Console Write(pre ansi): [%s]", GetText( line ) );
		LineRelease( line );
#endif
		if( pmdp->ansi )
		{
			AnsiBurst( pmdp->ansi, pLine );
		}
		while( (pmdp->ansi && (pLine = GetPendingWrite( pmdp->ansi ))) 
		      || (!pmdp->ansi && once) ) {
			//int flags = pLine->flags & (TF_NORETURN|TF_PROMPT);
			//lprintf( "Updated... %d", updated );
			//updated++;
			once = 0;
//#ifdef DEBUG_OUTPUT_TO_CONSOLE
			{
				PTEXT line = BuildLine( pLine );
				lprintf( "Console Write(post ansi): [%s]", GetText( line ) );
				LineRelease( line );
			}
//#endif
			if( pLine->flags & TF_FORMATABS ) {
				int32_t cursorx, cursory;
				//lprintf( "absolute position format." );
				GetHistoryCursorPos( pmdp->pCursor, &cursorx, &cursory );
				if( pLine->format.position.coords.x != -16384 )
					cursorx = pLine->format.position.coords.x;
				if( pLine->format.position.coords.y != -16384 )
					cursory = /*pmdp->nLines*/ -pLine->format.position.coords.y;
				SetHistoryCursorPos( pmdp->pCursor, cursorx, cursory );
				pLine->format.position.offset.spaces = 0;
				pLine->format.position.offset.tabs = 0;
				pLine->flags &= ~TF_FORMATABS;
			}
			if( pLine->flags & TF_FORMATREL ) {
				int32_t cursorx, cursory;
				//lprintf( "relative position format" );
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
				( phc->region->flags.bForceNewline ) ) { // err new segment goes on a new line.  (even if we are in the past)
				//Log2( "Line is automatically promoting itself to the next line. %d %d"
				//  , pht->nCursorY, pht->pTrailer?pht->pTrailer->nLinesUsed:-1 );
				if( !( pLine->flags & TF_NORETURN ) || phc->region->flags.bForceNewline )
					( phc->nCursorY )++;
			}
			if( !( pLine->flags & TF_NORETURN ) || phc->region->flags.bForceNewline )
				( phc->nCursorX ) = 0;

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
			result = PSI_EnqueDisplayHistory( pmdp->pCursor, pLine );
		}
	}
	pmdp->lockCount--;
	LeaveCriticalSec( &pmdp->Lock );
	return result;
}

//----------------------------------------------------------------------------

int GetCharFromLine( PCONSOLE_INFO console, uint32_t cols
						, PDISPLAYED_LINE pLine
						, int nChar, TEXTCHAR *result )
{
	int nLen;
	if( pLine && result )
	{
		PTEXT pText = pLine->start;
		int nOfs = 0, nSegShown = pLine->nFirstSegOfs;
		int32_t seg_len;
		int32_t nShown = 0; 
		int32_t col_offset = 0;
		nLen = pLine->nToShow ;// ComputeToShow( cols, &col_offset, pText, GetTextSize( pText ), nOfs, nShown, console->pCurrentDisplay );
		while( pText )
		{
			// nOfs is the column position to start at...
			// nShown is the amount of the first segment shown.
			//nLen = GetTextSize( pText );
			seg_len = (int)GetTextSize( pText );
			if( nShown >= nLen )
				return FALSE;
			if( !seg_len && !nChar )
			{
				(*result) = '\n';
				return TRUE;
			}

			if( nChar < pText->format.position.offset.spaces )
			{
				*result = ' ';
				return TRUE;
			}
			nShown += pText->format.position.offset.spaces;
			nChar -= pText->format.position.offset.spaces;
			if( (nChar + nSegShown) < seg_len )
			{
				TEXTCHAR *text = GetText( pText );
				*result = text[nChar + nSegShown];
				return TRUE;
			}
			nChar -= seg_len - nSegShown;
			nShown += seg_len - nSegShown;
			pText = NEXTLINE( pText );
			nSegShown = 0; // have shown nothing on this segment.
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
#if 0
int GetCharFromRowCol( PCONSOLE_INFO pdp, int row, int col, char *data )
{
	PDISPLAYED_LINE pdl = GetDataItem( &pdp->pCurrentDisplay->DisplayLineInfo, row );
	return GetCharFromLine( pdp, pdp->nColumns, pdl, col, data );
}
#endif
//----------------------------------------------------------------------------

TEXTCHAR *PSI_GetDataFromBlock( PCONSOLE_INFO pdp )
{
	int line_start = pdp->mark_start.row;
	int col_start = pdp->mark_start.col;
	int line_end = pdp->mark_end.row;
	int col_end = pdp->mark_end.col + 1;
	int bBlock = FALSE;
	// 2 characters to stuff in \r\n on newline.
	TEXTCHAR *result = NewArray( TEXTCHAR, ( ( line_start - line_end ) + 1 ) * (col_end + 2) );
	int ofs = 0;
	int line, col;
	int first_char = TRUE;
	int first = TRUE;
	int _priorline;
	//lprintf( "allocated something crazy like %d,%d  %d %p",line_start - line_end, pdp->nColumns,  ( ( line_start - line_end ) + 1 ) * (pdp->nColumns + 2) ,  result );
	for( col = col_start, line = line_start
		; line >= line_end
		 ; line--, (col = bBlock) )
	{
		PDISPLAYED_LINE pdl;
		if( ( pdl = (PDISPLAYED_LINE)GetDataItem( pdp->CurrentMarkInfo, line ) ) )
		{
			if( first )
			{
				first = FALSE;
				_priorline = pdl->nLine;
			}
			else
				result[ofs++] = '\n';

			if( !pdl->nToShow || bBlock )
			{
				if( pdp->flags.bBuildDataWithCarriageReturn )
					result[ofs++] = '\r';
				result[ofs++] = '\n';
				_priorline = pdl->nLine;
			}
			{
				int end_count = bBlock?(col_end)
										 : ( line == line_end ? (col_end)
												: ( pdl->nToShow - col_start ) );
				for( ;
					 (int64_t)col < end_count;
					 col++ )
				{
					if( GetCharFromLine( pdp, 0/*pdp->nColumns*/, pdl, col, result + ofs ) )
					{
						first_char = FALSE;
						ofs++;
					}
				}
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

int PSI_ConvertXYToLineCol( PCONSOLE_INFO pdp
										, int x, int y
										, int *line, int *col )
{
	// x, y is top, left biased...
	// line is bottom biased... (also have to account for history)
#ifdef DEBUG_OUTPUT
	lprintf( "Convert XY to LineCol needs work.... it needs to iterate through the computed dipslayable lines..." );
#endif
	(*line) = 0;
	(*col) = 0;
#if 0

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
	if( y < 0 )
		return FALSE;
	*line = y / pdp->nFontHeight;
#endif
	return TRUE;
}

//----------------------------------------------------------------------------

#if 0
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
#endif

//----------------------------------------------------------------------------

void DoRenderHistory( PCONSOLE_INFO pdp, int bHistoryStart, int nBottomLineOffset, int nStartLineOffset, PENDING_RECT *region )
{
	int nMinLine, nFirst = 0;
	INDEX nLine = 0;
	RECT r;
	RECT upd;
	int nFirstLine;
	PDATALIST *ppCurrentLineInfo;
	if( pdp->flags.bNoDisplay )
	{
		lprintf( "nodisplay!" );
		return;
	}
	//lprintf( "Begin render history line" );
	EnterCriticalSec( &pdp->Lock );
	//lprintf( "Begin render history locked" );
	pdp->lockCount++;
#ifdef DEBUG_HISTORY_RENDER
	lprintf( "Begin Render history." );
#endif
	if( !bHistoryStart )
	{
		// if no display (all history?)
		// this is the command line/display line separator.
		nFirstLine = ( upd.bottom = pdp->nDisplayLineStartDynamic );
		//lprintf( "First line: %d  %d  %d", nFirstLine, upd.bottom, pdp->nDisplayLineStartDynamic );
		if( !pdp->flags.bDirect && pdp->nDisplayLineStartDynamic != pdp->nCommandLineStart )
		{
			//lprintf( "Rendering display line seperator %d (not %d %d)", pdp->nDisplayLineStartDynamic, pdp->nCommandLineStart, pdp->nCommandLineStart- pdp->nDisplayLineStartDynamic );
			if( pdp->RenderSeparator )
				pdp->RenderSeparator( pdp, pdp->nDisplayLineStartDynamic );
			nFirstLine -= pdp->nSeparatorHeight;
			// add update region...
			// but how big is the thing that just drew?!
		}

#ifdef DEBUG_HISTORY_RENDER
		lprintf( "nFirstline is %d", nFirstLine );
#endif

		// figure out if we draw up to history or all the screen...
		if( pdp->nHistoryLineStart )
			nMinLine = pdp->nHistoryLineStart;
		else
			nMinLine = 0;
		ppCurrentLineInfo = GetDisplayInfo( pdp->pCurrentDisplay );
		//lprintf( "ppCurrentLineInfo=%p", ppCurrentLineInfo );
	}
	else // do render history start...
	{
		// if no history (all display?)
		if( pdp->nHistoryLineStart == 0 )
		{
			pdp->lockCount--;
			LeaveCriticalSec( &pdp->Lock );
			return;
		}
		nFirstLine = ( upd.bottom = pdp->nHistoryLineStart ) - (pdp->nYPad);
		nMinLine = 0;
		nFirst = -1;
		// the seperator is actually rendererd OVER the top of the displayed line.
		ppCurrentLineInfo = GetDisplayInfo( pdp->pHistoryDisplay );
		//lprintf( "ppCurrentLineInfo=%p", ppCurrentLineInfo );
	}
	//lprintf( "Render history separator %d", pdp->nHistoryLineStart );
	if( pdp->RenderSeparator )
		pdp->RenderSeparator( pdp, pdp->nHistoryLineStart - nStartLineOffset );
	r.bottom = nFirstLine;
	r.top = 0;
	// left and right are relative... to the line segment only...
	// for the reason of color changes inbetween segments...
	while( 1 )
	{
		PDISPLAYED_LINE pCurrentLine;
#ifdef DEBUG_HISTORY_RENDER
		lprintf( "Get display line %d", nLine );
#endif
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
		if( !pCurrentLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( "No such line... %d", nLine );
#endif
			break;
		}
		r.left = pCurrentLine->nPixelStart;
		r.right = pCurrentLine->nPixelEnd;
		r.top = pCurrentLine->nLineTop;
		r.bottom = r.top + pCurrentLine->nLineHeight;
		//lprintf( "display line to draw: %d  %d %d %d", nLine, r.bottom, r.top, r.bottom - r.top );
		RenderTextLine( pdp, pCurrentLine, &r
			, (int)nLine, nFirst, nFirstLine - nStartLineOffset, nMinLine
			, ppCurrentLineInfo == &pdp->pCurrentDisplay->DisplayLineInfo 
			, TRUE, 0, nBottomLineOffset, 0 );

		if( nFirst >= 0 )
			nFirst = -1;
		nLine++;
	}
	//lprintf( "r.bottom nMin %d %d", r.bottom, nMinLine );
	if( ( r.bottom - nStartLineOffset ) > nMinLine )
	{
		r.bottom = r.top;
		r.top = nMinLine;
		r.left = 0;
		r.right = pdp->nWidth;
#ifndef PSI_LIB
		//lprintf( "Would be blanking the screen here, but no, there's no reason to." );
				 //FillEmptyScreen();
#endif
	}
	//RenderConsole( pdp );
	//lprintf( "Render AGAIN the display line separator" );
	upd.top = r.top;
	upd.left = 0;
	upd.right = pdp->nWidth;
#ifdef DEBUG_REGION_UPDATES
	lprintf( "DoRenderHistory end" );
#endif
	AddUpdateRegion( region
						, upd.left, upd.top
						, upd.right-upd.left, (upd.bottom - nStartLineOffset) - upd.top );
	// screen updates affect the posititon of the last line/command line
	//if( pdp->flags.bDirect && !bHistoryStart )
	//	PSI_RenderCommandLine( pdp, region );
	pdp->lockCount--;
	LeaveCriticalSec( &pdp->Lock );
	//lprintf( "Done render history line" );
}

//----------------------------------------------------------------------------
void PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke )
{
	EnterCriticalSec( &pdp->Lock );
	pdp->lockCount++;
	if( PSI_DoStroke( pdp, stroke ) )
	{
		pdp->pCommandDisplay->flags.bUpdated = 1;

		if( !pdp->pCommandDisplay->pBlock )
		{
			pdp->pCommandDisplay->pBlock = pdp->pCommandDisplay->region->pHistory.root.next;
			pdp->pCommandDisplay->pBlock->nLinesUsed = 1;
			pdp->pCommandDisplay->pBlock->pLines[0].flags.deleted = 0;
			pdp->pCommandDisplay->nLine = 1;
		}
		pdp->pCommandDisplay->pBlock->pLines[0].flags.nLineLength = (int)LineLengthExEx( pdp->CommandInfo->CollectionBuffer, FALSE, 8, NULL );
		pdp->pCommandDisplay->pBlock->pLines[0].pLine = pdp->CommandInfo->CollectionBuffer;
		if( !pdp->flags.bDirect )
			BuildDisplayInfoLines( pdp->pCommandDisplay, NULL, GetCommonFont( pdp->psicon.frame ) );
		else {
			BuildDisplayInfoLines( pdp->pCommandDisplay, pdp->pCurrentDisplay, GetCommonFont( pdp->psicon.frame ) );
			// update bias of displayed section above the last (complete)
			if( pdp->pCommandDisplay->DisplayLineInfo->Cnt > 1 )
				pdp->nDisplayLineStartDynamic = ((PDISPLAYED_LINE)GetDataItem( &pdp->pCommandDisplay->DisplayLineInfo
					, pdp->pCommandDisplay->DisplayLineInfo->Cnt - 2 ))->nLineTop;
			else {
				pdp->nDisplayLineStartDynamic = pdp->nCommandLineStart;
			}
#ifdef DEBUG_OUTPUT_TO_CONSOLE
			lprintf( "(direct stroke)Setting display line to %d", pdp->nDisplayLineStartDynamic );
#endif
		}
	}

	pdp->lockCount--;
	LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

int PSI_UpdateHistory( PCONSOLE_INFO pdp, SFTFont font )
{
	int bUpdate = 0;
	lprintf( "nLines = %d  percent = %d  x = %d"
			, 0/*pdp->nLines*/
			, pdp->nHistoryPercent
			, ( 0/*pdp->nLines*/ * ( 3 - pdp->nHistoryPercent ) / 4 ) );
	EnterCriticalSec( &pdp->Lock );
	pdp->lockCount++;
	if( GetBrowserDistance( pdp->pHistoryDisplay, NULL ) >
		( 0/*pdp->nLines*/ * ( 3 - pdp->nHistoryPercent ) / 4 ) )
	{
		if( !pdp->flags.bHistoryShow )
		{
			extern PSIKEYDEFINE ConsoleKeyDefs[];
			lprintf( "Key END shoudl end history.." );
			ConsoleKeyDefs[KEY_END].op[0].bFunction = HISTORYKEY;
			ConsoleKeyDefs[KEY_END].op[0].data.HistoryKey = KeyEndHst;
			pdp->flags.bHistoryShow = 1;
			PSI_WinLogicCalculateHistory( pdp, font ); // this builds history and real display info lines.
			bUpdate = 1;
		}
		else
		{
			PENDING_RECT upd;
			upd.flags.bHasContent = 0;
			upd.flags.bTmpRect = 0;
			MemSet( &upd.cs, 0, sizeof( upd.cs ) );
			BuildDisplayInfoLines( pdp->pHistoryDisplay, NULL, font );
			//lprintf( "ALready showing history?!" );
			DoRenderHistory(pdp, TRUE, 0, 0, &upd);

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
			extern PSIKEYDEFINE ConsoleKeyDefs[];
			lprintf( "key end command line now... please do renderings.." );
			{
				int KeyEndCmd( uintptr_t list, PUSER_INPUT_BUFFER pci );

				ConsoleKeyDefs[KEY_END].op[0].bFunction = COMMANDKEY;
				ConsoleKeyDefs[KEY_END].op[0].data.CommandKey = KeyEndCmd;
			}
			pdp->flags.bHistoryShow = 0;
			PSI_WinLogicCalculateHistory( pdp, font );
			bUpdate = 1;
		}
	}
	pdp->lockCount--;
	LeaveCriticalSec( &pdp->Lock );
	return bUpdate;
}

PSI_CONSOLE_NAMESPACE_END
