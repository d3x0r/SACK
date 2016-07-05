//#define NO_LOGGING
//#define DEBUG_HISTORY_RENDER

#include "consolestruc.h"
//#include "interface.h"
#include "history.h"

#define WINLOGIC_SOURCE

#include "WinLogic.h"

#include "histstruct.h"
PSI_CONSOLE_NAMESPACE
//----------------------------------------------------------------------------

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
			//		, update_rect->x, update_rect->y
			//		, update_rect->width, update_rect->height
			//  	);
		}
		else
		{
			//_lprintf( DBG_AVAILABLE, WIDE("Setting (%d,%d)-(%d,%d)") DBG_RELAY
			//		, x, y
			//		, wd, ht
			//		);
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
		lprintf( WIDE("Get display line %d"), nLine );
#endif
		if( !pCurrentLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("No such line... %d"), nLine );
#endif
			return;
		}
		(*r).top = nFirstLine - pdp->nFontHeight * nLine;
		if( nFirst >= 0 )
			(*r).bottom = (*r).top + pdp->nFontHeight + 2;
		else
			(*r).bottom = (*r).top + pdp->nFontHeight;
		if( (*r).bottom <= nMinLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("bottom < minline..") );
#endif
			return;
		}
		y = (*r).top;

		(*r).left = 0;
		x = (*r).right = 0;//pdp->nXPad;
		//if( pdp->FillConsoleRect )
		//	pdp->FillConsoleRect(pdp, r, FILL_DISPLAY_BACK );

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

		if( allow_segment_coloring )
			if( pdp->SetCurrentColor )
				pdp->SetCurrentColor( pdp, COLOR_DEFAULT, NULL );
		nShown = pCurrentLine->nFirstSegOfs;
#ifdef DEBUG_HISTORY_RENDER
		if( !pText )
			lprintf( WIDE("Okay no text to show... end up filling line blank.") );
#endif
		while( pText )
		{
			size_t nLen;
			TEXTCHAR *text = GetText( pText );
#ifdef __DEKWARE_PLUGIN__
			if( !pdp->flags.bDirect && ( pText->flags & TF_PROMPT ) )
			{
				lprintf( WIDE("Segment is promtp - and we need to skip it.") );
				pText = NEXTLINE( pText );
				continue;
			}
#endif
			if( allow_segment_coloring )
				if( pdp->SetCurrentColor )
					pdp->SetCurrentColor( pdp, COLOR_SEGMENT, pText );

			nLen = GetTextSize( pText );
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("start: %d  len: %d"), nShown, nLen );
#endif
			while( nShown < nLen )
			{
#ifdef DEBUG_HISTORY_RENDER
				lprintf( WIDE("nShown < nLen... char %d len %d toshow %d"), nChar, nLen, pCurrentLine->nToShow );
#endif
				if( nChar + nLen > pCurrentLine->nToShow )
					nShow = pCurrentLine->nToShow - nChar;
				else
				{
#ifdef DEBUG_HISTORY_RENDER
					lprintf( WIDE("nShow is what's left of now to nLen from nShown... %d,%d"), nLen, nShown );
#endif
					nShow = nLen - nShown;
				}
				if( !nShow )
				{
					//lprintf( WIDE("nothing to show...") );
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
										nShow = pdp->mark_end.col - nChar;
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
									nShow = pdp->mark_start.col - nChar;
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
										nShow = pdp->mark_start.col - nChar;
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
				//lprintf( WIDE("Some stats %d %d %d"), nChar, nShow, nShown );
				if( nChar )
				{
					// not first character on line...
					x = (*r).left = (*r).right;
				}
				else
				{
					(*r).left = (*r).right;
					x = pdp->nXPad;
				}
				if( !(*r).right )
				{
					if( justify == 1 )
					{
						x = (*r).right = ( pdp->nWidth - ( pdp->nXPad + pCurrentLine->nPixelEnd ) ) + (*r).left;
					}
					else if ( justify == 2 ) 
					{
						x = (*r).right = ( ( pdp->nWidth - pCurrentLine->nPixelEnd ) / 2  ) + (*r).left;
					}
					else
						(*r).right = (*r).left + pdp->nXPad;
					if( pdp->FillConsoleRect )
						pdp->FillConsoleRect(pdp, r, FILL_DISPLAY_BACK );
				}
				(*r).right = pdp->nXPad + pCurrentLine->nPixelEnd;
				if( (*r).bottom > nMinLine )
				{
#ifdef DEBUG_HISTORY_RENDER
					lprintf( WIDE("And finally we can show some text... %s %d"), text, y );
#endif
					(*r).left = x;
					//lprintf( WIDE("putting string %s at %d,%d (left-right) %d,%d"), text, x, y, (*r).left, (*r).right );
					if( pdp->DrawString )
						pdp->DrawString( pdp, x, y, r, text, nShown, nShow );
					if( nLine == 0 )  // only keep the (last) line's end.
						pdp->nNextCharacterBegin = (*r).right;
					//DrawString( text );
					//lprintf( WIDE("putting string %s at %d,%d (left-right) %d,%d"), text, x, y, (*r).left, (*r).right );
				}
#ifdef DEBUG_HISTORY_RENDER
				else
					lprintf( WIDE("Hmm bottom < minline?") );
#endif
				// fill to the end of the line...
				//nLen -= nShow;
				nShown += nShow;
				nChar += nShow;
			}
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("nShown >= nLen...") );
#endif

			nShown -= nLen;
			pText = NEXTLINE( pText );
		}
		{
			x = (*r).left = (*r).right;
			(*r).right = pdp->nWidth;
			// if soething left to fill, blank fill it...
			if( (*r).left < (*r).right )
			{
#ifdef DEBUG_HISTORY_RENDER
				lprintf( WIDE("Fill empty to right (%d-%d)  (%d-%d)"), (*r).left, (*r).right, (*r).top, (*r).bottom );
#endif
				if( pdp->FillConsoleRect )
					pdp->FillConsoleRect( pdp, r, FILL_DISPLAY_BACK );
				//FillConsoleRect();
			}
		}
		if( nFirst >= 0 )
			nFirst = -1;
		nLine++;
	}
	//lprintf( WIDE(WIDE("(*(*r)...bottom nMin %d %d")), (*r)..bottom, nMinLine );
	if( (*r).bottom > nMinLine )
	{
		(*r).bottom = (*r).top;
		(*r).top = nMinLine;
		(*r).left = 0;
		(*r).right = pdp->nWidth;
#ifndef PSI_LIB
		//lprintf( WIDE("Would be blanking the screen here, but no, there's no reason to.") );
				 //FillEmptyScreen();
#endif
	}
}

void PSI_RenderCommandLine( PCONSOLE_INFO pdp, PENDING_RECT *region )
{
	PENDING_RECT myrect;
	// need to render the current macro being recorded.....
	RECT upd;

	RECT r;
	int nMaxLen, nRecord, nShow, nCurrentCol, x, y, nCursorPos;
	int lines;
	int line_offset;
	INDEX nShown;
	int start, end;
	int toppad = 0;
	PTEXT pStart;
	// no command line...
	if( !pdp->CommandInfo )
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

	nCursorPos = GetCommandCursor( pdp->pCurrentDisplay, NULL
											, pdp->CommandInfo
											, pdp->flags.bDirect
											, pdp->flags.bWrapCommand
											, &nCurrentCol
											, &start
											, &end
											, &line_offset);
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
		// need to blatcolor for the 5 pixels left of first char...
		r.left = 0;
		r.right = pdp->nXPad;
		if( pdp->FillConsoleRect )
		{
			lprintf( WIDE( "draw blank to left %d-%d" ), r.left, r.right );
			pdp->FillConsoleRect( pdp, &r, FILL_COMMAND_BACK );
		}
		upd.left = 0;
	}
	else
	{
		if( pdp->flags.bDirect )
			r.right = upd.left = ( pdp->nNextCharacterBegin );
		else
			r.right = upd.left = pdp->nXPad;
	}

	//r.left = x = pdp->nXPad + ( nCurrentCol * pdp->nFontWidth );
	r.left = x = r.right;
	//lprintf( WIDE( "x/left is %d" ), x );
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
		skip_lines = 0;
		lines = CountDisplayedLines( pdp->pCommandDisplay );
		if( !lines )
			lines = 1;
		lprintf( WIDE("want to do this in %d lines"), lines );
		if( lines > 3 )
		{
			skip_lines = lines - 3;
			lines = 3;
		}
		if( pdp->flags.bDirect )
		{
			pdp->nDisplayLineStart = pdp->nCommandLineStart - ( lines * pdp->nFontHeight );
		}
		else
		{
			upd.top = 
			pdp->nDisplayLineStart = pdp->nCommandLineStart
				- ( pdp->nFontHeight * lines
					+ ( pdp->nYPad * 2 )
					+ ( pdp->nCmdLinePad )
					);
		}

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
					lprintf( WIDE("No such line... %d"), nLine );
		#endif
					break;
				}
				if( pCurrentLine->start )
					RenderTextLine( pdp, pCurrentLine, &upd
						, nLine, TRUE, y, pdp->nDisplayLineStart
						, FALSE
						, FALSE );  // cursor; to know where to draw the mark...
			}
		}
		if( pdp->RenderCursor )
			// rect has top/bottom info, and current cursor position column
			// is passed - each client will be able to
			pdp->RenderCursor( pdp, &r, ( nRecord + nCursorPos ) ); // top/bottom are the line...


	}
	else
	{

		while( pStart && nShown > GetTextSize( pStart ) )
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
		//	lprintf( WIDE("Stopped because of length.") );
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

	}
}


//----------------------------------------------------------------------------
// 5 on left, 5 on right total 10 pixels we can't use...

void WinLogicCalculateHistory( PCONSOLE_INFO pdp )
{
	// there's some other related set of values to set here....
		//lprintf( WIDE("Calculate history! %d %d"), pdp->nColumns, pdp->nLines );

	SetCursorLines( pdp->pCursor, pdp->nLines );
	SetCursorColumns( pdp->pCursor, pdp->nColumns );
	if( pdp->pHistoryDisplay )
		SetBrowserColumns( pdp->pHistoryDisplay, pdp->nColumns, pdp->nWidth - pdp->nXPad*2 );
	SetBrowserColumns( pdp->pCurrentDisplay, pdp->nColumns, pdp->nWidth - pdp->nXPad*2 );
	SetBrowserLines( pdp->pCurrentDisplay, pdp->nLines );
	pdp->pCurrentDisplay->nLineHeight = pdp->nHeight;
	//lprintf( WIDE("Don't forget we wanted to inset command for a button...") );
	if( pdp->pCommandDisplay )
		SetBrowserColumns( pdp->pCommandDisplay, pdp->nColumns, pdp->nWidth - pdp->nXPad*2 );

	if( pdp->flags.bHistoryShow )
	{
		//lprintf( WIDE("Doing history... check percent and set display/history approp.") );
		switch( pdp->nHistoryPercent )
		{
		case 0:  // 25
		case 1: //50
		case 2: //75
			{
				if( pdp->pHistoryDisplay )
				{
					int nWorkLines;
					nWorkLines = ( pdp->nLines * ( 3 - pdp->nHistoryPercent ) ) / 4;
					SetBrowserLines( pdp->pHistoryDisplay, (pdp->nLines - nWorkLines)+2 );
					PSI_SetHistoryPageLines( pdp->pHistoryDisplay, (pdp->nLines - nWorkLines)-3 );
					pdp->nHistoryLineStart = pdp->nDisplayLineStart - (nWorkLines)* pdp->nFontHeight;
					SetBrowserLines( pdp->pCurrentDisplay, nWorkLines );
				}
			}
			break;
		case 3: //100
			if( pdp->pHistoryDisplay )
			{
				pdp->nHistoryLineStart = pdp->nDisplayLineStart;
				SetBrowserLines( pdp->pHistoryDisplay, pdp->nLines );
				// need this to know how far close to end we can get...
				SetBrowserLines( pdp->pCurrentDisplay, 0 );
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
			//lprintf( WIDE("No history, all display") );
			// internally we'll need this amount to get into
			// scrollback...
			{
				int nWorkLines;
				nWorkLines = ( pdp->nLines * ( 1 + pdp->nHistoryPercent ) ) / 4;
				PSI_SetHistoryPageLines( pdp->pHistoryDisplay, nWorkLines - 3 );
			}
			PSI_SetHistoryPageLines( pdp->pHistoryDisplay, pdp->nLines - 4 );
			SetBrowserLines( pdp->pHistoryDisplay, 1 );
			ResetHistoryBrowser( pdp->pHistoryDisplay );
			// 1 for the partial line at the top of the display.
		}
		SetBrowserLines( pdp->pCurrentDisplay, pdp->nLines );
	}
	if( pdp->pHistoryDisplay )
		BuildDisplayInfoLines( pdp->pHistoryDisplay, NULL );
	BuildDisplayInfoLines( pdp->pCurrentDisplay, NULL );
}

//----------------------------------------------------------------------------
void DoRenderHistory( PCONSOLE_INFO pdp, int bHistoryStart, PENDING_RECT *region );

void PSI_RenderConsole( PCONSOLE_INFO pdp )
{
	PENDING_RECT upd;
	upd.flags.bHasContent = 0;
	upd.flags.bTmpRect = 1;
	MemSet( &upd.cs, 0, sizeof( upd.cs ) );
	EnterCriticalSec( &pdp->Lock );
	//lprintf( WIDE("Render Console... %d %d"), pdp->nDisplayLineStart, pdp->nHistoryLineStart );

	if( !(pdp->flags.bDirect && pdp->flags.bCharMode) )
		PSI_RenderCommandLine( pdp, &upd );

	BuildDisplayInfoLines( pdp->pCurrentDisplay, NULL );
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

void PSI_ConsoleCalculate( PCONSOLE_INFO pdp )
{
	//RECT rArea;
	int nLines;
	//lprintf( WIDE("*** DISPLAY is %d,%d by %d,%d"), pdp->rArea.top, pdp->rArea.left, pdp->rArea.right, pdp->rArea.bottom );
	if( ( ( pdp->rArea.right -  pdp->rArea.left )== 0 )
		&& ( ( pdp->rArea.bottom -  pdp->rArea.top )== 0 ) )
		return;
	if ( (pdp->rArea.right - pdp->rArea.left) <= 0 || 
			(pdp->rArea.bottom - pdp->rArea.top ) <= 0 )
	{
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

	pdp->nCommandLineStart = pdp->rArea.bottom;

	if( pdp->nFontHeight )
	{
		//lprintf( WIDE("Okay font height existsts... that's good") );
		if( pdp->flags.bDirect )
		{
			pdp->nDisplayLineStart = pdp->nCommandLineStart;
			SetCursorNoPrompt( pdp->pCurrentDisplay, FALSE );
			if( pdp->pHistoryDisplay )
				SetCursorNoPrompt( pdp->pHistoryDisplay, FALSE );
		}
		else
		{
			SetCursorNoPrompt( pdp->pCurrentDisplay, TRUE );
			if( pdp->pHistoryDisplay )
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
	PSI_RenderConsole( pdp );
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
	{
		//int flags = pLine->flags & (TF_NORETURN|TF_PROMPT);
		//lprintf( WIDE("Updated... %d"), updated );
		//updated++;

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
		result = PSI_EnqueDisplayHistory( pmdp->pCursor, pLine );
	}
	LeaveCriticalSec( &pmdp->Lock );
	return result;
}

//----------------------------------------------------------------------------

int GetCharFromLine( PCONSOLE_INFO console, _32 cols
						, PDISPLAYED_LINE pLine
						, int nChar, TEXTCHAR *result )
{
	int nLen;
	if( pLine && result )
	{
		PTEXT pText = pLine->start;
		int nOfs = 0, nSegShown = pLine->nFirstSegOfs;
		_32 seg_len;
		_32 nShown = 0; 
		_32 col_offset = 0;
		nLen = pLine->nToShow ;// ComputeToShow( cols, &col_offset, pText, GetTextSize( pText ), nOfs, nShown, console->pCurrentDisplay );
		while( pText )
		{
			// nOfs is the column position to start at...
			// nShown is the amount of the first segment shown.
			//nLen = GetTextSize( pText );
			seg_len = GetTextSize( pText );
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
	INDEX col_end = pdp->mark_end.col + 1;
	int bBlock = FALSE;
	// 2 characters to stuff in \r\n on newline.
	TEXTCHAR *result = NewArray( TEXTCHAR, ( ( line_start - line_end ) + 1 ) * (col_end + 2) );
	INDEX ofs = 0;
	int line, col;
	int first_char = TRUE;
	int first = TRUE;
	int _priorline;
	//lprintf( "allocated something crazy like %d,%d  %d %p",line_start - line_end, pdp->nColumns,  ( ( line_start - line_end ) + 1 ) * (pdp->nColumns + 2) ,  result );
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
					 (S_64)col < end_count;
					 col++ )
				{
					if( GetCharFromLine( pdp, pdp->nColumns, pdl, col, result + ofs ) )
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
	int nMinLine, nFirst = 0;
	INDEX nLine = 0;
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
#ifdef DEBUG_HISTORY_RENDER
	lprintf( WIDE("Begin Render history.") );
#endif
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

#ifdef DEBUG_HISTORY_RENDER
		lprintf( WIDE("nFirstline is %d"), nFirstLine );
#endif

		// figure out if we draw up to history or all the screen...
		if( pdp->nHistoryLineStart )
			nMinLine = pdp->nHistoryLineStart;
		else
			nMinLine = 0;
		ppCurrentLineInfo = GetDisplayInfo( pdp->pCurrentDisplay );
		//lprintf( WIDE("ppCurrentLineInfo=%p"), ppCurrentLineInfo );
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
		//lprintf( WIDE("ppCurrentLineInfo=%p"), ppCurrentLineInfo );
	}
	//lprintf( WIDE("Render history separator %d"), pdp->nHistoryLineStart );
	if( pdp->RenderSeparator )
		pdp->RenderSeparator( pdp, pdp->nHistoryLineStart );
	r.bottom = nFirstLine;
	r.top = 0;
	// left and right are relative... to the line segment only...
	// for the reason of color changes inbetween segments...
	while( 1 )
	{
		PDISPLAYED_LINE pCurrentLine;
#ifdef DEBUG_HISTORY_RENDER
		lprintf( WIDE("Get display line %d"), nLine );
#endif
		pCurrentLine = (PDISPLAYED_LINE)GetDataItem( ppCurrentLineInfo, nLine );
		if( !pCurrentLine )
		{
#ifdef DEBUG_HISTORY_RENDER
			lprintf( WIDE("No such line... %d"), nLine );
#endif
			break;
		}

		RenderTextLine( pdp, pCurrentLine, &r
			, nLine, nFirst, nFirstLine, nMinLine
			, ppCurrentLineInfo == &pdp->pCurrentDisplay->DisplayLineInfo 
			, TRUE ); 

		if( nFirst >= 0 )
			nFirst = -1;
		nLine++;
	}
	//lprintf( WIDE("r.bottom nMin %d %d"), r.bottom, nMinLine );
	if( r.bottom > nMinLine )
	{
		r.bottom = r.top;
		r.top = nMinLine;
		r.left = 0;
		r.right = pdp->nWidth;
#ifndef PSI_LIB
		//lprintf( WIDE("Would be blanking the screen here, but no, there's no reason to.") );
				 //FillEmptyScreen();
#endif
	}
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
		PSI_RenderCommandLine( pdp, region );
	LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------
void PSI_WinLogicDoStroke( PCONSOLE_INFO pdp, PTEXT stroke )
{
	EnterCriticalSec( &pdp->Lock );
	if( PSI_DoStroke( pdp, stroke ) )
	{
		if( !pdp->pCommandDisplay->pBlock )
		{
			pdp->pCommandDisplay->pBlock = pdp->pCommandDisplay->region->pHistory.root.next;
			pdp->pCommandDisplay->pBlock->nLinesUsed = 1;
			pdp->pCommandDisplay->pBlock->pLines[0].flags.deleted = 0;
			pdp->pCommandDisplay->nLine = 1;
		}
		pdp->pCommandDisplay->pBlock->pLines[0].nLineLength = LineLengthExEx( pdp->CommandInfo->CollectionBuffer, FALSE, 8, NULL );
		pdp->pCommandDisplay->pBlock->pLines[0].pLine = pdp->CommandInfo->CollectionBuffer;
		if( !pdp->flags.bDirect && pdp->flags.bWrapCommand )
			BuildDisplayInfoLines( pdp->pCommandDisplay, NULL );
	}

	LeaveCriticalSec( &pdp->Lock );
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

int PSI_UpdateHistory( PCONSOLE_INFO pdp )
{
	int bUpdate = 0;
	lprintf( WIDE("nLines = %d  percent = %d  x = %d")
			, pdp->nLines
			, pdp->nHistoryPercent
			, ( pdp->nLines * ( 3 - pdp->nHistoryPercent ) / 4 ) );
	EnterCriticalSec( &pdp->Lock );
	if( GetBrowserDistance( pdp->pHistoryDisplay, NULL ) >
		( pdp->nLines * ( 3 - pdp->nHistoryPercent ) / 4 ) )
	{
		if( !pdp->flags.bHistoryShow )
		{
			extern PSIKEYDEFINE ConsoleKeyDefs[];
			lprintf( WIDE("Key END shoudl end history..") );
			ConsoleKeyDefs[KEY_END].op[0].bFunction = HISTORYKEY;
			ConsoleKeyDefs[KEY_END].op[0].data.HistoryKey = KeyEndHst;
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
			BuildDisplayInfoLines( pdp->pHistoryDisplay, NULL );
			//lprintf( WIDE("ALready showing history?!") );
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
			extern PSIKEYDEFINE ConsoleKeyDefs[];
			lprintf( WIDE("key end command line now... please do renderings..") );
			{
				int KeyEndCmd( PTRSZVAL list, PUSER_INPUT_BUFFER pci );

				ConsoleKeyDefs[KEY_END].op[0].bFunction = COMMANDKEY;
				ConsoleKeyDefs[KEY_END].op[0].data.CommandKey = KeyEndCmd;
			}
			pdp->flags.bHistoryShow = 0;
			WinLogicCalculateHistory( pdp );
			bUpdate = 1;
		}
	}
	LeaveCriticalSec( &pdp->Lock );
	return bUpdate;
}

PSI_CONSOLE_NAMESPACE_END
