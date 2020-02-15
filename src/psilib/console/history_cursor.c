
#include <stdhdrs.h>
#include <filesys.h>
#include <logging.h>
#include <psi/console.h>
#include <sqlgetoption.h>
#include "regaccess.h"

#include "consolestruc.h" // all relavent includes
#include "history.h"
#include "histstruct.h"

PSI_CONSOLE_NAMESPACE



void SetTopOfForm( PHISTORY_LINE_CURSOR phlc );
//----------------------------------------------------------------------------

void PSI_DestroyHistoryCursor( PHISTORY_LINE_CURSOR phlc )
{
	Release( phlc );
}

//----------------------------------------------------------------------------

PHISTORY_LINE_CURSOR PSI_CreateHistoryCursor( PHISTORY_REGION phr )
{
	if( phr )
	{
		PHISTORY_LINE_CURSOR phlc = (PHISTORY_LINE_CURSOR)Allocate( sizeof( HISTORY_LINE_CURSOR ) );
		MemSet( phlc, 0, sizeof( HISTORY_LINE_CURSOR ) );

		phlc->output.DefaultColor.flags.background = SACK_GetProfileInt( "SACK/PSI/console", "background", 0 );
		phlc->output.DefaultColor.flags.foreground = SACK_GetProfileInt( "SACK/PSI/console", "foreground", 7 );

		phlc->output.PriorColor = phlc->output.DefaultColor;

		phlc->region = phr;
		AddLink( &phr->pCursors, phlc );
		return phlc;
	}
	return NULL;
}

//----------------------------------------------------------------------------

void ResolveLineColor( PHISTORY_LINE_CURSOR pdp, PTEXT pLine )
{
	FORMAT *f;
	while( pLine )
	{
		if( !(pLine->flags & TF_FORMATEX) ) // ignore extended formats
		{
			f = &pLine->format;

			if( f->flags.default_foreground )
				f->flags.foreground = pdp->output.DefaultColor.flags.foreground;
			else if( f->flags.prior_foreground )
				f->flags.foreground = pdp->output.PriorColor.flags.foreground;
			if( f->flags.highlight )
				f->flags.foreground |= 8;

			if( f->flags.default_background )
				f->flags.background = pdp->output.DefaultColor.flags.background;
			else if( f->flags.prior_background )
				f->flags.background = pdp->output.PriorColor.flags.background;

			if( f->flags.reverse )
			{
			}
			if( f->flags.blink )
				f->flags.background |= 8;
			// apply any fixup for DEFAULT_COLOR or PRIOR_COLOR
			pdp->output.PriorColor = *f;
		}
		pLine = NEXTLINE( pLine );
	}
}

//----------------------------------------------------------------------------

// get the line to which output is going...
// ncursory is measured from top of form down.
// the actual updated region may be outside of browser parameters...
PTEXTLINE GetAHistoryLine( PHISTORY_LINE_CURSOR phc, PHISTORY_BROWSER phbr, int nLine, int bCreate )
{
	if( phc )
	{
		int tmp;
		PHISTORYBLOCK phb;
		//lprintf( "Get line %d after %d", nLine, phc->output.top_of_form.line );

		tmp = nLine + phc->output.top_of_form.line;
		phb = phc->output.top_of_form.block;
		// first block is different then those after this...
		// have to remember we may be biased within a block...
		if( phb && phb->next &&tmp >= phb->nLinesUsed )
		{
			tmp -= phb->nLinesUsed - phc->output.top_of_form.line;
			phb = phb->next;
		}
		while( phb && phb->next &&
				tmp >= phb->nLinesUsed )
		{
			tmp -= phb->nLinesUsed;
			phb = phb->next;
		}
		//lprintf( "tmp is now %d", tmp );
		{
			PTEXTLINE ptl;
			while( tmp >= (phb?phb->nLinesUsed:0) )
			{
				if( !bCreate )
				{
					// fail creation of blank lines...
					return NULL;
				}
				ptl = GetNewHistoryLine( phc->region );
				if( !phc->output.top_of_form.block )
					phc->output.top_of_form.block = phc->region->pHistory.next;
				if( !phb )
					phb = phc->output.top_of_form.block;
				if( phb && phb->next )
				{
					tmp -= phb->nLinesUsed;
					phb = phb->next;
				}
			}
		}
		return phb->pLines + tmp;
	}
	else if( phbr )
	{
		int tmp;
		PHISTORYBLOCK phb;
		//lprintf( "Get line %d after %d", nLine, phbr->nLine );

		tmp = nLine + phbr->nLine;
		phb = phbr->pBlock;
		// first block is different then those after this...
		// have to remember we may be biased within a block...
		if( phb && phb->next &&tmp >= phb->nLinesUsed )
		{
			tmp -= phb->nLinesUsed - phbr->nLine;
			phb = phb->next;
		}
		while( phb && phb->next &&
				tmp >= phb->nLinesUsed )
		{
			tmp -= phb->nLinesUsed;
			phb = phb->next;
		}
		//lprintf( "tmp is now %d", tmp );
		{
			PTEXTLINE ptl;
			while( tmp >= (phb?phb->nLinesUsed:0) )
			{
				if( !bCreate )
				{
					// fail creation of blank lines...
					return NULL;
				}
				ptl = GetNewHistoryLine( phbr->region );
				if( !phbr->pBlock )
					phbr->pBlock = phbr->region->pHistory.next;
				if( !phb )
					phb = phbr->pBlock;
				if( phb->next )
				{
					tmp -= phb->nLinesUsed;
					phb = phb->next;
				}
			}
		}
		return phb->pLines + tmp;

	}
	return NULL;
}

PTEXTLINE GetHistoryLine( PHISTORY_LINE_CURSOR phc )
{
	if( phc->output.nCursorY < 0 )
		phc->output.nCursorY = 0;
	return GetAHistoryLine( phc, NULL, phc->output.nCursorY, TRUE );
}

//----------------------------------------------------------------------------

struct PSI_console_word *PutSegmentOut( PHISTORY_LINE_CURSOR phc
							  , PTEXT segment )
{
	// this puts the segment at the correct x, y
	// it also updates the curren X on the65 line...
	struct PSI_console_word *word = New( struct PSI_console_word );

	PTEXTLINE pCurrentLine;

	PTEXT text;
	int len;
	if( !(phc->region->pHistory.next) ||
		 ( phc->region->pHistory.last->nLinesUsed == MAX_HISTORY_LINES &&
			!(segment->flags&TF_NORETURN) ) )
		CreateHistoryBlock( &phc->region->pHistory.root );

	//lprintf( "Okay after much layering... need to fix some...." );
	//lprintf( "Put a segment out!" );
	// get line at nCursorY
	word->line = pCurrentLine = GetHistoryLine( phc );
	word->cursor_position_added_at = phc->output.nCursorX;
	word->segment = segment;

	if( !pCurrentLine )
	{
		lprintf( "No line result from history..." );
	}

	if( (phc->output.nCursorX) > pCurrentLine->flags.nLineLength )
	{
		// beyond the end of the line...
		PTEXT filler = SegCreate( (phc->output.nCursorX) - pCurrentLine->flags.nLineLength );
		TEXTCHAR *data = GetText( filler );
		int n;
#ifdef DEBUG_OUTPUT
		lprintf( "Cursor beyond line, creating filler segment up to X" );
#endif
		filler->format.flags.foreground = segment->format.flags.foreground;
		filler->format.flags.background = segment->format.flags.background;
		//Log1( "Make a filler segment %d charactes", phc->region->(phc->output.nCursorX) - pCurrentLine->nLineLength );
		for( n = 0; n < (phc->output.nCursorX) - pCurrentLine->flags.nLineLength; n++ )
		{
			data[n] = ' ';
		}
		pCurrentLine->flags.nLineLength = (phc->output.nCursorX);
		pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, filler );
	}


	if( (phc->output.nCursorX) < pCurrentLine->flags.nLineLength )
	{
		// very often the case...
		// need to split the segment which the cursor is on
		// unless the cursor happens to be at the start of the segment,
		// in which cast, it is spilt, segment is inserted, and
		// any data in the segment beyond 'segment' will have to be
		// split/trimmed/overwritten.

		// so - split the line, result with current segment.
		int32_t pos = 0;
#ifdef DEBUG_OUTPUT
		lprintf( "Okay insert/overwrite this segment on the display..." );
#endif
		text = pCurrentLine->pLine;
		while( pos < (phc->output.nCursorX) && text )
		{
			len = (int)GetTextSize( text );
			//Log3( "Skipping over ... pos:%d len:%d curs:%d", pos, len, phc->region->(phc->output.nCursorX) );
			if( (len + pos) > (phc->output.nCursorX) )
			{
				// 'text' is OVER pos...
				PTEXT split;
				if( text == pCurrentLine->pLine )
					split = SegSplit( &pCurrentLine->pLine, ( (phc->output.nCursorX) - pos ) );
				else
					split = SegSplit( &text, ( (phc->output.nCursorX) - pos ) );
				if( split ) // otherwise we miscalculated something.
				{
					text = NEXTLINE( split );
					len = (int)GetTextSize( text );
				}
			}
			pos += len;
			text = NEXTLINE( text );
		}
	}
	else // cursor is exactly linelength...
	{
		//Log( "Cursor is at end or after end." );
		text = NULL;
		len = 0;
	}

	// expects 'text' to be the start of the correct segment.
	if( segment->flags & TF_FORMATEX )
	{
		Log( "Extended info carried on this segment." );
		segment->flags &= ~TF_FORMATEX;
		switch( segment->format.flags.format_op )
		{
		case FORMAT_OP_CLEAR_END_OF_LINE: // these are difficult....
			//Log( "Clear end of line..." );
			if( text == pCurrentLine->pLine )
			{
				pCurrentLine->pLine = NULL;
				pCurrentLine->flags.nLineLength = 0;
			}
			SegBreak( text );
			LineRelease( text );
			text = NULL;
			len = 0;
			break;
		case FORMAT_OP_DELETE_CHARS:
			Log1( "Delete %d characters", segment->format.flags.background );
			if( text ) // otherwise there's no characters to delete?  or does it mean next line?
			{
				PTEXT split;
				Log1( "Has a current segment... %" _size_f, GetTextSize( text ) );
				if( text == pCurrentLine->pLine )
				{
					Log( "is the first segment... ");
					split = SegSplit( &pCurrentLine->pLine, segment->format.flags.background );
					text = pCurrentLine->pLine;
				}
				else
				{
					Log("Is not first - splitting segment... ");
					split = SegSplit( &text, segment->format.flags.background );
				}
				if( split )
				{
					Log( "Resulting split..." );
					text = NEXTLINE( split );
					LineRelease( SegGrab( split ) );
				}
				else
				{
					Log( "Didn't split the line - deleting the next segment." );
					LineRelease( SegGrab( text ) );
				}
			}
			else
			{
				Log( "Didn't find a next semgnet( text)" );
			}
			break;
		case FORMAT_OP_CLEAR_START_OF_LINE:
			{
				PTEXT filler = SegCreate( (phc->output.nCursorX) );
				TEXTCHAR *data = GetText( filler );
				int n;
				filler->format.flags.foreground = phc->output.PriorColor.flags.foreground;
				filler->format.flags.background = phc->output.PriorColor.flags.background;
				//Log1( "Make a filler segment %d charactes", (phc->output.nCursorX) - pCurrentLine->nLineLength );
				for( n = 0; n < (phc->output.nCursorX) - pCurrentLine->flags.nLineLength; n++ )
				{
					data[n] = ' ';
				}
				Log( "This is all so terribly bad - clear start of line is horrid idea." );
				if( text ) // insert before text... else append to line.
				{
					PTEXT fill_here = SegCreate( 1 );
					GetText( fill_here )[0] = ' '; // 1 space.
					fill_here->format.flags.foreground = segment->format.flags.foreground;
					fill_here->format.flags.background = segment->format.flags.background;
					if( text == pCurrentLine->pLine )
					{
						SegSplit( &pCurrentLine->pLine, 1 );
						text = pCurrentLine->pLine;
						SegSubst( pCurrentLine->pLine, fill_here );
						if( GetTextSize( filler ) )
							Log( "Size expected, zero size filler computed" );
						LineRelease( filler ); // should be zero sized...
					}
					else
					{
						SegSplit( &text, 1 ); // need to eat 1 character.
						SegSubst( text, fill_here );
						text = fill_here;
						SegBreak( fill_here );
						LineRelease( pCurrentLine->pLine );
						pCurrentLine->pLine = SegAppend( filler, fill_here );
					}
				}
				else
				{
					// attribute of filler needs to be set to current color paramters.
					LineRelease( pCurrentLine->pLine );
					pCurrentLine->pLine = filler;
				}
			}
			break;
		case FORMAT_OP_PAGE_BREAK:
			// enque as normal...
			segment->flags |= TF_FORMATEX;
			pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, segment );
			// step to the next line, cursor 0, all zero...
			phc->output.nCursorX = 0;
			phc->output.nCursorY++;
			pCurrentLine = GetHistoryLine( phc );  // start a new line
			SetTopOfForm( phc );  // signal current line index is a new form
			break;
		case FORMAT_OP_JUSTIFY_CENTER:
		case FORMAT_OP_JUSTIFY_RIGHT:
			// this needs to pass
			// enque as normal...
			// the display layer needs to know this ex format
			segment->flags |= TF_FORMATEX;
			break;
		default:
			Log( "Invalid extended format segment. " );
			break;
		}
		if( !(segment->flags & TF_FORMATEX ) )
		{
			if( segment->format.flags.prior_foreground )
				segment->format.flags.foreground = phc->output.PriorColor.flags.foreground;
			else
				segment->format.flags.foreground = phc->output.DefaultColor.flags.foreground;
			if( segment->format.flags.prior_background )
				segment->format.flags.background = phc->output.PriorColor.flags.background;
			else
				segment->format.flags.background = phc->output.DefaultColor.flags.background;
		}
	}
	//else
	//	Log( "No extended info carried on this segment." );


	if( GetTextSize( segment ) || ( segment->flags & TF_FORMATEX ) )
	{
		if( text )
		{
			//Log( "Inserting new segment..." );
			SegInsert( segment, text );
			if( text == pCurrentLine->pLine )
				pCurrentLine->pLine = segment;
		}
		else
		{
			//Log( "Appending segment." );
			pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, segment );
		}

		len = (int)GetTextSize( segment );
		(phc->output.nCursorX) += len;

		if( !phc->region->flags.bInsert && text )
		{
			// len characters from 'text' need to be removed.
			// okay that should be fun...
			int32_t deletelen = len;
			//Log1( "Remove %d characters!", len );
			while( deletelen && text )
			{
				int32_t thislen = (int)GetTextSize( text );
				PTEXT next = NEXTLINE( text );
				if( thislen <= deletelen )
				{
					LineRelease( SegGrab( text ) );
					deletelen -= thislen;
					text = next;
				}
				else // thislen > deletelen
				{
					if( text == pCurrentLine->pLine )
						SegSplit( &pCurrentLine->pLine, deletelen );
					else
						SegSplit( &text, deletelen );
				}
			}
		}
		else
			pCurrentLine->flags.nLineLength += len;
	}
	else if( segment->flags & TF_FORMATEX )
	{
		// the only one that gets here is the set top of form??
		//switch( segment->format.flags.format_op )

		pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, segment );
		SetTopOfForm( phc );
	}
	else
	{
		pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, segment );
		//Log( "no data in segment." );
	}
	return word;
}

//----------------------------------------------------------------------------

void SetTopOfForm( PHISTORY_LINE_CURSOR phlc )
{
	if( phlc )
	{
		lprintf( "Set the top of form here..." );
		while( phlc->output.top_of_form.block &&
				phlc->output.top_of_form.block->next )
			phlc->output.top_of_form.block = phlc->output.top_of_form.block->next;
		if( phlc->output.top_of_form.block )
			phlc->output.top_of_form.line = phlc->output.top_of_form.block->nLinesUsed;
		phlc->output.nCursorY = 0;
	}
}

//----------------------------------------------------------------------------

PTEXT HandleExtendedFormat( PHISTORY_LINE_CURSOR phc, PTEXT pLine )
{
	PTEXT _pLine = pLine;
	PTEXTLINE pCurrentLine;
	switch( pLine->format.flags.format_op )
	{
		/*
		 case FORMAT_OP_GET_CURSOR:
		 {
		 PTEXT responce = SegCreate(0);
		 responce->flags |= TF_FORMATEX;
		 responce->format.foreground = FORMAT_OP_SET_CURSOR;
		 responce->format.position.coords.x = pht->nCursorX;
		 responce->format.position.coords.y = -pht->nCursorY;
		 EnqueLink( &pht->pdp->common.Input, responce );
		 }
		 break;
		 */
	case FORMAT_OP_PAGE_BREAK:
	case FORMAT_OP_CLEAR_PAGE: // set cursor home
		pLine->format.flags.format_op = FORMAT_OP_PAGE_BREAK;
		// and that sets the cursory to 0 for the thing...
		// but now if I hit top of form in display gather info
		// then I need to stop.
		// pages are still bottom-up safe then.
	case FORMAT_OP_CLEAR_END_OF_LINE: // these are difficult....
	case FORMAT_OP_CLEAR_START_OF_LINE:
	case FORMAT_OP_DELETE_CHARS:
	case FORMAT_OP_JUSTIFY_RIGHT:
	case FORMAT_OP_JUSTIFY_CENTER:
		// need to pass this segment through to enquesegments
		pLine->flags |= TF_FORMATEX;
		break;
	case FORMAT_OP_CLEAR_LINE:
		pLine->format = phc->output.PriorColor;
		pCurrentLine = GetHistoryLine( phc );
		phc->output.nCursorX = 0;
		pCurrentLine->flags.nLineLength = 0;
		LineRelease( pCurrentLine->pLine );
		pCurrentLine->pLine = NULL;
		break;
	case FORMAT_OP_CLEAR_END_OF_PAGE:
		pLine->format = phc->output.PriorColor;
		{
			int y;
			PTEXTLINE ptl;
			for( y = phc->output.nCursorY;
				 (ptl = GetHistoryLine( phc ));
				  y++ )
			{
				ptl->flags.nLineLength = 0;
				LineRelease( ptl->pLine );
				ptl->pLine = NULL;
			}
		}
		break;
	case FORMAT_OP_CLEAR_START_OF_PAGE:
		pLine->format = phc->output.PriorColor;
		{
			int y;
			PTEXTLINE ptl;
			for( y = 0;
				 y < phc->output.nCursorY;
				  y++ )
			{
				ptl = GetAHistoryLine( phc, NULL, y, FALSE );
				ptl->flags.nLineLength = 0;
				LineRelease( ptl->pLine );
				ptl->pLine = NULL;
			}
		}
		break;
	case FORMAT_OP_CONCEAL: // sets option to not show text at all until next color.
		pLine->format.flags.foreground = 0;
		pLine->format.flags.background = 0;
		// hmm - need to set this as a flag... but
		// for now this will be sufficient....
		pLine->flags &= ~TF_FORMATEX;
		break;
	}
	return _pLine;
}

//----------------------------------------------------------------------------

uint32_t ComputeNextOffset( PTEXT segment, uint32_t nShown )
{
	 uint32_t offset = 0;
	 while( segment )
	 {
		size_t nLen = GetTextSize( segment );
		TEXTCHAR *text = GetText( segment );
		while( nShown < nLen
			  && text[nShown] == ' ' )
			nShown++;
		if( nShown == nLen )
		{
			offset += nShown;
			nShown = 0;
			segment = NEXTLINE( segment );
			continue;
		}
		else
			break;
	 }
	 return nShown + offset;
}

//----------------------------------------------------------------------------

uint32_t ComputeToShow( uint32_t colsize, uint32_t *col_offset, PTEXT segment, uint32_t nLen, uint32_t nOfs, uint32_t nShown, PHISTORY_BROWSER phbr, SFTFont font )
{
	int32_t result_bias = 0;
	int32_t nShow = nLen - nShown;
	uint32_t nLenSize, nLenHeight;
	// if space left to show here is less than
	// then length to show, compute wrapping point.
	//lprintf( "Compute to show: %d (%d)%s %d %d", cols, GetTextSize( segment ), GetText( segment ), nOfs, nShown );
	phbr->measureString( phbr->psvMeasure, GetText( segment ) + nShown
		, nLen - nShown, &nLenSize, &nLenHeight, font );

	if( ( nLenSize + (*col_offset ) ) > colsize || nLenHeight > (uint32_t)phbr->nLineHeight )
	{
		uint32_t good_space_size;
		LOGICAL has_good_space = FALSE;
		uint32_t good_space = 0;
		uint32_t nSpace;
		size_t nNextSpace;
		uint32_t nSegSize, nSegHeight;
		uint32_t best_chars = 0;
		uint32_t best_char_size;
		TEXTCHAR *text = GetText( segment );
		size_t textLen = GetTextSize( segment );
		TEXTRUNE thisChar;

		for( nNextSpace = nSpace = nShown; (nSpace < nLen)?((thisChar = GetUtfCharIndexed( text, &nNextSpace, textLen )),1):0 ; nSpace = nNextSpace )
		{
			if( thisChar == '\n' )
			{
				phbr->measureString( phbr->psvMeasure, text + nShown
					, nSpace - nShown, &nSegSize, &nSegHeight, font );
				good_space_size = nSegSize;
				has_good_space = TRUE;
				good_space = nSpace; // include space in this part of the line...
				result_bias = 1; // start next line past the \n
				break;
			}
			else if( thisChar == ' ' || thisChar == '\t' )
			{
				//lprintf( "measure string until space... %s (%s)", text, text + nSpace );
				phbr->measureString( phbr->psvMeasure, text + nShown
					, nSpace - nShown, &nSegSize, &nSegHeight, font );
				if( ( (*col_offset) + nSegSize  ) < colsize )
				{
					has_good_space = TRUE;
					good_space_size = nSegSize;
					good_space = nSpace + 1; // include space in this part of the line...
				}
				else
					break;
			}
			else if( !best_chars )
			{
				phbr->measureString( phbr->psvMeasure, text + nShown
					, nSpace - nShown, &nSegSize, &nSegHeight, font );

				if( ( (*col_offset) + nSegSize  ) >= colsize )
				{
					if( (nSpace - nShown) < 2 ) {
						if( *col_offset ) return 0;
						best_char_size = nSegSize;
						best_chars = nShown + 1;  // minimum of 1 character to use.
						break;
					}
					best_chars = (nSpace - 1);
					break;
				}
				else {
					best_char_size = nSegSize;
				}
			}
		}
		if( !best_chars && !has_good_space && best_char_size && (*col_offset) == 0 )
			best_chars = nSpace - 1;

		// found a space, please show up to that.
		if( has_good_space )
		{
			//while( text[good_space] == ' ' )
			//	good_space++;
			(*col_offset) += good_space_size;
			nShow = good_space - nShown;
		}
		else if( best_chars )
		{
			(*col_offset) += best_char_size;

			nShow = best_chars - nShown;
		}
		else if( nOfs ) // if started after a line, wrap whole thing to next...
		{
			nShow = 0;
			(*col_offset) = 0;
		}
		else
		{
			// failing all of this nShow will be length, and non delimited text
			// will wrap forcably.
			// show as much fo this....
			// well.. have to figure out which character will still fit....
			for( nSpace = nShown; nSpace <= nLen; nSpace++ )
			{
				phbr->measureString( phbr->psvMeasure, text + nShown
					, nSpace - nShown, &nSegSize, &nSegHeight, font );
				if( ( (*col_offset) + nSegSize  ) < colsize )
					;
				else
				{
					nShow = ( nSpace - 1 ) - nShown;
					(*col_offset) += nSegSize;
					break;
				}
			}
			if( nSpace > nLen )
			{
				nShow = ( nSpace - 1 ) - nShown;
				(*col_offset) += nSegSize;
				// ran out of characters in segment... whole segment fits.
				// it didn't fit, but now it fits?!
				//lprintf( "This should be a segfault or something" );
			}
		}
	 }
	 else
	{
		 nShow = nLen - nShown;
		(*col_offset) += nLenSize;
	}
	if( (nShow + result_bias) < 0 )
		DebugBreak();
	//lprintf( "Show %d", nShow );
	return nShow + result_bias;
}

//----------------------------------------------------------------------------

void PSI_SetHistoryDefaultForeground( PHISTORY_LINE_CURSOR phlc, int iColor )
{
	phlc->output.DefaultColor.flags.foreground = iColor;
	phlc->output.PriorColor = phlc->output.DefaultColor;
}

//----------------------------------------------------------------------------

void PSI_SetHistoryDefaultBackground( PHISTORY_LINE_CURSOR phlc, int iColor )
{
	phlc->output.DefaultColor.flags.background = iColor;
	phlc->output.PriorColor = phlc->output.DefaultColor;
}


//----------------------------------------------------------------------------

void GetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, int32_t* x, int32_t* y )
{
	if( x ) *x = phlc->output.nCursorX;
	if( y ) *y = phlc->output.nCursorY;
}
//----------------------------------------------------------------------------
int GetCursorLine( PHISTORY_LINE_CURSOR phlc )
{
	int32_t y;
	GetHistoryCursorPos( phlc, NULL, &y );
	return y;
}
//----------------------------------------------------------------------------
int GetCursorColumn( PHISTORY_LINE_CURSOR phlc )
{
	int32_t x;
	GetHistoryCursorPos( phlc, &x, NULL );
	return x;
}
//----------------------------------------------------------------------------
void SetCursorLine( PHISTORY_LINE_CURSOR phlc, int n )
{
	//GetHistoryCursorPos( phlc, NULL, &y );
	//return y;
}
//----------------------------------------------------------------------------
void SetCursorColumn( PHISTORY_LINE_CURSOR phlc, int n )
{
	//GetHistoryCursorPos( phlc, &x, NULL );
	//return x;
}
//----------------------------------------------------------------------------
void SetCursorHeight( PHISTORY_LINE_CURSOR phlc, int n )
{
	phlc->nHeight = n;
	// recompute visible lines...
}

//----------------------------------------------------------------------------
void SetCursorWidth( PHISTORY_LINE_CURSOR phlc, int n )
{
	phlc->nWidth = n;
	// recompute visible lines...
}
//----------------------------------------------------------------------------

void SetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, int32_t x, int32_t y )
{
	phlc->output.nCursorX = (int16_t)x;
	phlc->output.nCursorY = (int16_t)y;
}


//----------------------------------------------------------------------------

PSI_Console_Phrase PSI_EnqueDisplayHistory( PHISTORY_LINE_CURSOR phc, PTEXT pLine )
{
	PSI_Console_Phrase phrase = New( struct PSI_console_phrase );

	MemSet( phrase, 0, sizeof( struct PSI_console_phrase ) );

	if( pLine->flags & TF_FORMATEX )
	{
		// clear this bit... extended format will result with bit re-set
		// if it is so required.
		pLine->flags &= ~TF_FORMATEX;
		HandleExtendedFormat( phc, pLine );
	}
	if( !(pLine->flags & TF_NORETURN) )
	{
		// auto return/newline if not noreturn
		//lprintf( "Not NORETURN - therefore skipping to next line..." );
		phc->output.nCursorX = 0;
		phc->output.nCursorY++;
	}

	{
		PTEXTLINE pCurrentLine = NULL;
		//lprintf( "Flattening line..." );
		pLine = FlattenLine( pLine );
		ResolveLineColor( phc, pLine );

		// colors are processed in the stream of output - not by position
		// lprintf okay putting segments out...
		{
			PTEXT next = pLine;
			while( (pLine = next) )
			{
				next = NEXTLINE( pLine );
				AddLink( &phrase->words, PutSegmentOut( phc, SegGrab( pLine ) ) );
			}
		}
		if( pCurrentLine ) // else no change...
		{
			pCurrentLine->flags.nLineLength = (int)LineLength( pCurrentLine->pLine );
		}
	}
	{
		INDEX idx;
		PHISTORY_BROWSER browser;
		LIST_FORALL( phc->region->pBrowsers, idx, PHISTORY_BROWSER, browser ) {
			browser->flags.bUpdated = 1;
		}
	}
	return phrase;
}

//----------------------------------------------------------------------------

void PSI_Console_SetPhraseData( PSI_Console_Phrase phrase, uintptr_t psv )
{
	if( phrase )
		phrase->data = psv;
}

uintptr_t PSI_Console_GetPhraseData( PSI_Console_Phrase phrase )
{
	if( phrase )
		return phrase->data;
	return 0;
}


PSI_CONSOLE_NAMESPACE_END
