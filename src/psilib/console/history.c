
#include <stdhdrs.h>
#include <filesys.h>
#include <logging.h>
#include <psi/console.h>

#include "consolestruc.h" // all relavent includes
#include "history.h"

#include "histstruct.h"

PSI_CONSOLE_NAMESPACE


	//#include "interface.h"
extern int myTypeID;

void SetTopOfForm( PHISTORY_LINE_CURSOR phlc );
//----------------------------------------------------------------------------

PHISTORYBLOCK PSI_DestroyRawHistoryBlock( PHISTORYBLOCK pHistory )
{
	uint32_t i;
	PHISTORYBLOCK next;
	for( i = 0; i < pHistory->nLinesUsed; i++ )
	{
		LineRelease( pHistory->pLines[i].pLine );
	}
	if( ( next = ( (*pHistory->me) = pHistory->next ) ) )
		pHistory->next->me = pHistory->me;
	Release( pHistory );
	return next;
}

//----------------------------------------------------------------------------

void PSI_DestroyHistoryRegion( PHISTORY_REGION phr )
{
	if( phr )
	{
		while( phr->pHistory.next )
			PSI_DestroyRawHistoryBlock( phr->pHistory.next );
		DeleteList( &phr->pCursors );
		Release( phr );
	}
}
//----------------------------------------------------------------------------

void PSI_DestroyHistoryBlock( PHISTORY_REGION phr )
{
	// just destroy the first block...
	if( phr )
	{
		PSI_DestroyRawHistoryBlock( phr->pHistory.next );
	}
}


//----------------------------------------------------------------------------
void PSI_DestroyHistory( PHISTORYTRACK pht )
{
	if( pht )
	{
		PSI_DestroyHistoryRegion(pht->pRegion);
		Release( pht );
	}
}

//----------------------------------------------------------------------------

void PSI_DestroyHistoryBrowser( PHISTORY_BROWSER phbr )
{
	DeleteCriticalSec( &phbr->cs );
	DeleteDataList( &phbr->DisplayLineInfo );
	Release( phbr );
}
//----------------------------------------------------------------------------

void PSI_DestroyHistoryCursor( PHISTORY_LINE_CURSOR phlc )
{
	Release( phlc );
}
//----------------------------------------------------------------------------

PHISTORYBLOCK CreateRawHistoryBlock( void )
{
	PHISTORYBLOCK pHistory;
	pHistory = (PHISTORYBLOCK)Allocate( sizeof( HISTORYBLOCK ) );
	MemSet( pHistory, 0, sizeof( HISTORYBLOCK ) );
	return pHistory;
}

//----------------------------------------------------------------------------

void CreatePriorHistoryBlock( PHISTORY_BLOCK_LINK phbl )
{
	 // used when there aren't enough lines to go backwards
	 // to a specified control position... (create a full block.)
	PHISTORYBLOCK pHistory;
	if( !phbl->me ) // in most cases it's 'last'
	{
		lprintf( WIDE("FATAL ERROR, NODE in list is not initialized correctly.") );
		DebugBreak();
	}
	pHistory = CreateRawHistoryBlock();
	pHistory->nLinesUsed = MAX_HISTORY_LINES;
	// if passed node is root of list...
	// then the first node of the list will be the new
	// history block's next, and the list itself will
	// point at the new history block.

	// if passed node is within the list...
	// my next is the passed node's next... since
	// I will now be immediately after the passed node
	// and immediately before his next.
	pHistory->next = phbl->next;

	// now combined steps... set the reference of what is pointing
	// at to the address of phbl->next - sicne that is the thing
	// that will be pointing at me.
	// then set that pointer to be me, since it is the prior node's
	// next, which should point at the new history block.
	(*(pHistory->me = &(phbl->next))) = pHistory;
	// phbl->me is unmodified.
	// phbl->next is the new history node (by property of above asignment)
	// history->me is &phbl->next
	// history->next is phbl->next

	//phr->nHistoryBlocks++;
}

//----------------------------------------------------------------------------

PHISTORYBLOCK CreateHistoryBlock( PHISTORY_BLOCK_LINK phbl )
{
	PHISTORYBLOCK pHistory;
	// phbl needs to be valid node link thingy...
	if( !phbl->me )
	{
		lprintf( WIDE("FATAL ERROR, NODE in list is not initialized correctly.") );
		DebugBreak();
	}
	pHistory = CreateRawHistoryBlock();
	/* this is link next... (first in list)
	pHistory->next = phbl->next;
	if( phbl->me ) // in most cases it's 'last'
		(*(pHistory->me = &(phbl->me->next))) = pHistory;
		phbl->next = pHistory;
		*/
	// if this is the head of the list... then that
	// me points at the last node of the list, the
	// next of the last node is NULL, which if we
	// get the next, and set it here will be NULL.

	// if this points at an element within the list,
	// then phbl->me is the node prior to the node passed,
	// and that will be phbl itself, which will set
	// next to be phbl, and therefore will be immediately
	// before the next.
	//pHistory->next = (*phbl->me);
	pHistory->next = (*(pHistory->me = phbl->me));

	// set what is pointing at the passed node, and
	// make my reference of prior node's pointer mine,
	//pHistory->me = phbl->me;
	// and then set that pointer to ME, instead of phbl...
	(*pHistory->me) = pHistory; // set pointer pointing at me to ME
	// finally, phbl's me is now my next, since the new node points at it.
	phbl->me = &pHistory->next;

	return pHistory;
}

//----------------------------------------------------------------------------

PHISTORY_LINE_CURSOR PSI_CreateHistoryCursor( PHISTORY_REGION phr )
{
	if( phr )
	{
		PHISTORY_LINE_CURSOR phlc = (PHISTORY_LINE_CURSOR)Allocate( sizeof( HISTORY_LINE_CURSOR ) );
		MemSet( phlc, 0, sizeof( HISTORY_LINE_CURSOR ) );

#if 0 && defined( _WIN32 ) && !defined( PSICON )
		{
			int nValue;
			PTEXT name = NULL;
			if( GetRegistryInt( "Dekware\\Wincon\\Background"
									, name?GetText( name ):"default"
									, &nValue ) )
				phlc->output.DefaultColor.flags.background = nValue;
			else
#endif
				phlc->output.DefaultColor.flags.background = 0;
#if 0 && defined( _WIN32 ) && !defined( PSICON )
			if( GetRegistryInt( "Dekware\\Wincon\\Foreground"
									, name?GetText( name ):"default"
									, &nValue ) )
				phlc->output.DefaultColor.flags.foreground = nValue;
			else
#endif
				phlc->output.DefaultColor.flags.foreground = 7;
#if 0 && defined( _WIN32 ) && !defined( PSICON )
	}
#endif
		phlc->output.PriorColor = phlc->output.DefaultColor;

		phlc->region = phr;
		AddLink( &phr->pCursors, phlc );
		return phlc;
	}
	return NULL;
}

//----------------------------------------------------------------------------

PHISTORY_REGION PSI_CreateHistoryRegion( void )
{
	PHISTORY_REGION phr = (PHISTORY_REGION)Allocate( sizeof( HISTORY_REGION ) );
	MemSet( phr, 0, sizeof( HISTORY_REGION ) );
	phr->pHistory.me = &phr->pHistory.next;
	CreateHistoryBlock( &phr->pHistory.root );
	return phr;
}

//----------------------------------------------------------------------------

PHISTORY_BROWSER PSI_CreateHistoryBrowser( PHISTORY_REGION region, MeasureString measureString, uintptr_t psvMeasure )
{
	PHISTORY_BROWSER phbr = (PHISTORY_BROWSER)Allocate( sizeof( HISTORY_BROWSER ) );
	MemSet( phbr, 0, sizeof( HISTORY_BROWSER ) );
	InitializeCriticalSec( &phbr->cs );
	phbr->region = region;
	phbr->DisplayLineInfo = CreateDataList( sizeof( DISPLAYED_LINE ) );
	phbr->flags.bWrapText = 1;
	phbr->measureString = measureString;
	phbr->psvMeasure = psvMeasure;
	return phbr;
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

PTEXTLINE GetSomeHistoryLineEx( PHISTORY_REGION region
									 , PHISTORYBLOCK start
									 , int line // 0 = from end of, -1 = next up...
										 DBG_PASS )
#define GetSomeHistoryLine(r,s,n) GetSomeHistoryLineEx(r,s,n DBG_SRC )
{
	// if history has never had anything...
	//_xlprintf( 0 DBG_RELAY )("Getting some history line: %d", line );
	if( !region->pHistory.next )
		return NULL;
	if( ( line > 0 ) && start )
	{
		return start->pLines + (line-1);
	}
	if( !start )
	{
		start = region->pHistory.last;
		line += start->nLinesUsed;
		//lprintf( "overriding start, using line as rel index back... now is %d", line );
	}
	// if start, and line < 0 then we'll naturally
	// step back one at a time...
	// so for a certain loop we may have a start greater than 0
	// which will rreturn immediately,
	// at less than 0 we back up a block, add that blocks size,
	// and result with that blocks, now positive index, and so forth
	// until there are no blocks, or the index becomes a positive one
	// in a block...
	{
		PHISTORYBLOCK pBlock = start;
		// correct for last block last line index...
		//line += region->pHistory.last->nLinesUsed;

		// if line is before this block, then step back
		// through the blocks until line is discovered....
		while( *(pBlock->me) && line <= 0 )
		{
			// okay we never want to pre-expand when doing this get...
			// this is really intended to be the routine to browse
			// therefore if it's not there, don't show it.

			// still have lines to go back to get to the desired line,
			// but it is past the beginning page-break... uhmm actually...
			//
			if( pBlock->nLinesUsed && pBlock->pLines[0].pLine )
				if( ( pBlock->pLines[0].pLine->flags & TF_FORMATEX ) &&
					( pBlock->pLines[0].pLine->format.flags.format_op == FORMAT_OP_PAGE_BREAK ) )
				{
					DebugBreak();
					// can't go backwards past a page-break.
					return NULL;
				}
			if( ( pBlock = pBlock->prior ) && *(pBlock->me) )
			{
				//lprintf( "Adjusting line as we step backward..." );
				line += pBlock->nLinesUsed;
			}
		}
		if( line > 0 )
		{
			if( line < MAX_HISTORY_LINES )
			{
				//lprintf( "Setting used lines in history block..." );
				//pBlock->nLinesUsed = line;
			}
			return pBlock->pLines + (line - 1);
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------

// if bExpand ...
//	this is done during normal line processing
//	and if this is the case - then Y is an apsolute
//  hmm... if not expand - then return the literal line...
//  otherwise only consider real lines?  no...
//  okay so the flags must be set and managed... but then overridden
//  if a
PTEXTLINE GetNewHistoryLine( PHISTORY_REGION region )
{
	PHISTORYBLOCK pBlock = region->pHistory.last;
	if( pBlock->nLinesUsed >= MAX_HISTORY_LINES )
	{
		pBlock = CreateHistoryBlock( &region->pHistory.root );
	}
	//lprintf( "INcrementint lines used... returning this line at %d", pBlock->nLinesUsed );
	return pBlock->pLines + pBlock->nLinesUsed++;
}

//----------------------------------------------------------------------------

// get the line to which output is going...
// ncursory is measured from top of form down.
// the actual updated region may be outside of browser parameters...
PTEXTLINE GetAHistoryLine( PHISTORY_LINE_CURSOR phc, PHISTORY_BROWSER phbr, int nLine, int bCreate )
{
	if( phc )
	{
		INDEX tmp;
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
		INDEX tmp;
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
		lprintf( WIDE("No line result from history...") );
	}

	if( (phc->output.nCursorX) > pCurrentLine->nLineLength )
	{
		// beyond the end of the line...
		PTEXT filler = SegCreate( (phc->output.nCursorX) - pCurrentLine->nLineLength );
		TEXTCHAR *data = GetText( filler );
		int n;
		lprintf( WIDE("Cursor beyond line, creating filler segment up to X") );
		filler->format.flags.foreground = segment->format.flags.foreground;
		filler->format.flags.background = segment->format.flags.background;
		//Log1( "Make a filler segment %d charactes", phc->region->(phc->output.nCursorX) - pCurrentLine->nLineLength );
		for( n = 0; n < (phc->output.nCursorX) - pCurrentLine->nLineLength; n++ )
		{
			data[n] = ' ';
		}
		pCurrentLine->nLineLength = (phc->output.nCursorX);
		pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, filler );
	}


	if( (phc->output.nCursorX) < pCurrentLine->nLineLength )
	{
		// very often the case...
		// need to split the segment which the cursor is on
		// unless the cursor happens to be at the start of the segment,
		// in which cast, it is spilt, segment is inserted, and
		// any data in the segment beyond 'segment' will have to be
		// split/trimmed/overwritten.

		// so - split the line, result with current segment.
		int32_t pos = 0;
		lprintf( WIDE("Okay insert/overwrite this segment on the display...") );
		text = pCurrentLine->pLine;
		while( pos < (phc->output.nCursorX) && text )
		{
			len = GetTextSize( text );
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
					len = GetTextSize( text );
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
		Log( WIDE("Extended info carried on this segment.") );
		segment->flags &= ~TF_FORMATEX;
		switch( segment->format.flags.format_op )
		{
		case FORMAT_OP_CLEAR_END_OF_LINE: // these are difficult....
			//Log( "Clear end of line..." );
			if( text == pCurrentLine->pLine )
			{
				pCurrentLine->pLine = NULL;
				pCurrentLine->nLineLength = 0;
			}
			SegBreak( text );
			LineRelease( text );
			text = NULL;
			len = 0;
			break;
		case FORMAT_OP_DELETE_CHARS:
			Log1( WIDE("Delete %d characters"), segment->format.flags.background );
			if( text ) // otherwise there's no characters to delete?  or does it mean next line?
			{
				PTEXT split;
				Log1( WIDE("Has a current segment... %") _size_f, GetTextSize( text ) );
				if( text == pCurrentLine->pLine )
				{
					Log( WIDE("is the first segment... "));
					split = SegSplit( &pCurrentLine->pLine, segment->format.flags.background );
					text = pCurrentLine->pLine;
				}
				else
				{
					Log(WIDE( "Is not first - splitting segment... "));
					split = SegSplit( &text, segment->format.flags.background );
				}
				if( split )
				{
					Log( WIDE("Resulting split...") );
					text = NEXTLINE( split );
					LineRelease( SegGrab( split ) );
				}
				else
				{
					Log( WIDE("Didn't split the line - deleting the next segment.") );
					LineRelease( SegGrab( text ) );
				}
			}
			else
			{
				Log( WIDE("Didn't find a next semgnet( text)") );
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
				for( n = 0; n < (phc->output.nCursorX) - pCurrentLine->nLineLength; n++ )
				{
					data[n] = ' ';
				}
				Log( WIDE("This is all so terribly bad - clear start of line is horrid idea.") );
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
							Log( WIDE("Size expected, zero size filler computed") );
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
			Log( WIDE("Invalid extended format segment. ") );
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

		len = GetTextSize( segment );
		(phc->output.nCursorX) += len;

		if( !phc->region->flags.bInsert && text )
		{
			// len characters from 'text' need to be removed.
			// okay that should be fun...
			uint32_t deletelen = len;
			//Log1( "Remove %d characters!", len );
			while( deletelen && text )
			{
				uint32_t thislen = GetTextSize( text );
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
			pCurrentLine->nLineLength += len;
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
		//Log( WIDE("no data in segment.") );
	}
	return word;
}

//----------------------------------------------------------------------------

void SetTopOfForm( PHISTORY_LINE_CURSOR phlc )
{
	if( phlc )
	{
		lprintf( WIDE("Set the top of form here...") );
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
		pCurrentLine->nLineLength = 0;
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
				ptl->nLineLength = 0;
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
				ptl->nLineLength = 0;
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
			while( ( pLine = next ) )
			{
				next = NEXTLINE( pLine );
				AddLink( &phrase->words, PutSegmentOut( phc, SegGrab( pLine ) ) );
			}
		}
		if( pCurrentLine ) // else no change...
		{
			pCurrentLine->nLineLength = LineLength( pCurrentLine->pLine );
		}
	}
	return phrase;
}

//----------------------------------------------------------------------------

void DumpBlock( PHISTORYBLOCK pBlock DBG_PASS )
{
	INDEX idx;
	_xlprintf( 0 DBG_RELAY )(WIDE("History block used lines: %")_size_f WIDE(" of %d"), pBlock->nLinesUsed, MAX_HISTORY_LINES );
	for( idx = 0; idx < pBlock->nLinesUsed; idx++ )
	{
		PTEXTLINE ptl = pBlock->pLines + idx;
		_xlprintf( 0 DBG_RELAY )(WIDE("line: %")_size_f WIDE(" = (%d,%")_size_f WIDE(",%s)"), idx, ptl->nLineLength, GetTextSize( ptl->pLine ), GetText( ptl->pLine ) );
	}
}

//----------------------------------------------------------------------------

void DumpRegion( PHISTORY_REGION region DBG_PASS )
{
	PHISTORYBLOCK blocks = region->pHistory.root.next;
	_xlprintf( 0 DBG_RELAY )(WIDE("History blocks: %d history block size: %d"), region->nHistoryBlocks
			 , region->nHistoryBlockSize
			 );
	while( blocks )
	{
		DumpBlock( blocks DBG_RELAY );
		blocks = blocks->next;
	}
}

//----------------------------------------------------------------------------

PTEXT EnumHistoryLineEx( PHISTORY_BROWSER phbr
							  , int *offset
							  , int32_t *length DBG_PASS)
#define EnumHistoryLine(hb,o,l) EnumHistoryLineEx(hb,o,l DBG_SRC )
{
	PTEXTLINE ptl;
	int line = -(*offset);
	if( !offset )
	{
		Log( WIDE("No start...") );
		return NULL;
	}
	//lprintf("Getting line: %d ", line );
	//DumpRegion( phlc->region DBG_RELAY );
	// uhmm... yeah.
	ptl = GetSomeHistoryLine( phbr->region
									, phbr->pBlock
									, (phbr->pBlock?phbr->nLine:0) + line );
	//DumpRegion( phlc->region DBG_RELAY );

	if( ptl )
	{
		(*offset)++;
		if( ptl->pLine )
		{
			if( length )
				*length = ptl->nLineLength;
			//lprintf( "Resulting: (%d)%s", ptl->nLineLength, GetText( ptl->pLine ) );
			return ptl->pLine;
		}
		else
		{
			DECLTEXT( nothing, WIDE("") );
			nothing.format.flags.foreground = 0;
			nothing.format.flags.background = 0;
			if( length )
				*length = 0;
			//lprintf( "resulting nothing. %d", line );
			return (PTEXT)&nothing;
		}
	}
	//Log( "No result" );
	return NULL;
}

//----------------------------------------------------------------------------

void SetHistoryLength( PHISTORY_REGION phr, INDEX length )
{
	Log1( WIDE("Set Length to %") _size_f WIDE(" blocks"), length/MAX_HISTORY_LINES );
	phr->nMaxHistoryBlocks = length/MAX_HISTORY_LINES;
}

//----------------------------------------------------------------------------

INDEX GetHistoryLength( PHISTORY_REGION phr )
{
	int total = 0;
	PHISTORYBLOCK block;
	for( block = phr->pHistory.root.next; block; block = block->next )
		total += block->nLinesUsed;
	return total;
}

//----------------------------------------------------------------------------

void InitHistoryRegion( PHISTORY_REGION phr )
{
	while( phr->pHistory.next )
		PSI_DestroyRawHistoryBlock( phr->pHistory.next );
	phr->tabsize = 8;
	phr->nMaxHistoryBlocks = 5000; // configurable option now...
}

//----------------------------------------------------------------------------

void WriteHistoryToFile( FILE *file, PHISTORY_REGION phr )
{
#ifndef __NO_WIN32API__
	PHISTORYBLOCK pHistory = phr->pHistory.next;
	INDEX idx;
	while( pHistory )
	{
		lprintf( WIDE("Have a history block with %")_size_f WIDE(" lines"), pHistory->nLinesUsed );
		for( idx = 0; idx < pHistory->nLinesUsed; idx++ )
		{
			size_t length;
			PTEXTLINE pLine = pHistory->pLines + idx;
			PTEXT pText = pLine->pLine;
			while( pText )
			{
				length = GetTextSize( pText );
				lprintf( WIDE("%s"), GetText(pText) );
				sack_fwrite( &length, 1,  sizeof( length ), file );
				sack_fwrite( &pText->flags, 1,  sizeof( pText->flags ), file );
				sack_fwrite( &pText->format, 1,  sizeof( pText->format ), file );
				sack_fwrite( GetText( pText ), 1, length, file );
				pText = NEXTLINE( pText );
			}
			length = 0;
			sack_fwrite( &length, 1,  sizeof( length ), file );
		}
		pHistory = pHistory->next;
	}
#else
	lprintf( WIDE("Cannot write to file, please port file access") );
#endif
}

void ReadHistoryFromFile( FILE *file, PHISTORY_REGION phr )
{
#ifndef __NO_WIN32API__
	size_t length;
	PTEXT readline = NULL;
	PTEXT rebuild;
	while( sack_fread( &length, 1,  sizeof( length ), file ) )
	{
		if( length )
		{
			rebuild = SegCreate( length );
			sack_fread( &rebuild->flags, 1,  sizeof( rebuild->flags ), file );
			sack_fread( &rebuild->format, 1,  sizeof( rebuild->format ), file );
			sack_fread( GetText( rebuild ), 1, length, file );
			readline = SegAppend( readline, rebuild );
		}
		else
		{
			PSI_EnqueDisplayHistory( (PHISTORY_LINE_CURSOR)GetLink( &phr->pCursors, 0 )
				, readline );
			readline = NULL;
		}
	}
#else
	lprintf( WIDE("Cannot write to file, please port file access") );
#endif
}


void SetHistoryBackingFile( PHISTORY_REGION phr, FILE *file )
{
	ReadHistoryFromFile( file, phr );
	phr->file_backing = file;
}

//----------------------------------------------------------------------------

uint32_t ComputeNextOffset( PTEXT segment, uint32_t nShown )
{
	 uint32_t offset = 0;
	 while( segment )
	 {
		  uint32_t nLen = GetTextSize( segment );
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
	uint32_t result_bias = 0;
	uint32_t nShow = nLen - nShown;
	uint32_t nLenSize, nLenHeight;
	// if space left to show here is less than
	// then length to show, compute wrapping point.
	//lprintf( "Compute to show: %d (%d)%s %d %d", cols, GetTextSize( segment ), GetText( segment ), nOfs, nShown );
	phbr->measureString( phbr->psvMeasure, GetText( segment ) + nShown
		, nLen - nShown, &nLenSize, &nLenHeight, font );

	if( ( nLenSize + (*col_offset ) ) > colsize || nLenHeight > phbr->nLineHeight )
	{
		uint32_t good_space_size;
		LOGICAL has_good_space = FALSE;
		uint32_t good_space = 0;
		uint32_t nSpace;
		uint32_t nSegSize, nSegHeight;
		uint32_t best_chars = 0;
		uint32_t best_char_size;
		TEXTCHAR *text = GetText( segment );

		for( nSpace = nShown; nSpace < nLen; nSpace++ )
		{
			if( text[nSpace] == '\n' )
			{
				phbr->measureString( phbr->psvMeasure, GetText( segment ) + nShown
					, nSpace - nShown, &nSegSize, &nSegHeight, font );
				good_space_size = nSegSize;
				has_good_space = TRUE;
				good_space = nSpace; // include space in this part of the line...
				result_bias = 1; // start next line past the \n
				break;
			}
			else if( text[nSpace] == ' ' || text[nSpace] == '\t' ) 
			{
				//lprintf( "measure string until space... %s (%s)", text, text + nSpace );
				phbr->measureString( phbr->psvMeasure, GetText( segment ) + nShown
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
				phbr->measureString( phbr->psvMeasure, GetText( segment ) + nShown
					, nSpace - nShown, &nSegSize, &nSegHeight, font );

				if( ( (*col_offset) + nSegSize  ) >= colsize )
				{
					if( !nSpace )
						return 0;
					best_chars = (nSpace - 1);
					break;
				}
				else 
					best_char_size = nSegSize;
			}
		}

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
				phbr->measureString( phbr->psvMeasure, GetText( segment ) + nShown
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
				//lprintf( WIDE("This should be a segfault or something") );
			}
		}
	 }
	 else
	 {
		 nShow = nLen - nShown;
		(*col_offset) += nLenSize;
	}
	 //lprintf( "Show %d", nShow );
	 return nShow + result_bias;
}

//----------------------------------------------------------------------------

int SkipSomeLines( PHISTORY_BROWSER phbr, SFTFont font, PTEXT countseg, int lines )
{
	uint32_t colsize = phbr->nWidth;
	// always spans at least one line.
	int nLines = 1;
	if( countseg && colsize )
	{
		uint32_t nShown = 0;
		uint32_t nChar = 0;
		uint32_t col_offset = 0;
		while( countseg )
		{
			uint32_t nLen = GetTextSize( countseg );
			// part of this segment has already been skipped.
			// ComputeNextOffset can do this.
			if( nShown > nLen )
			{
				nShown -= nLen;
			}
			else // otherwise nShown is within this segment...
			{
				uint32_t text_size;
				uint32_t text_lines;
				phbr->measureString( phbr->psvMeasure, GetText( countseg ), nLen, &text_size, &text_lines, NULL );

				if( ( col_offset + text_size ) > colsize )
				{
					// this is the wrapping condition...
					while( nShown < nLen )
					{
						nShown += ComputeToShow( colsize, &col_offset, countseg, nLen, nChar, nShown, phbr, NULL );
						if( nShown < nLen )
						{
							nLines++;
							nChar = 0;
						}
					}
					nShown -= nLen;
				}
				else
					nChar += nLen;
			}
			countseg = NEXTLINE( countseg );
		}
		return nShown;
	}
	return 0;
}

//----------------------------------------------------------------------------

// colsize is the size of space the line can take up
int CountLinesSpanned( PHISTORY_BROWSER phbr, PTEXT countseg, SFTFont font, LOGICAL count_trailing_linefeeds )
{
	// always spans at least one line.
	uint32_t colsize = phbr->nWidth;
	int nLines = 1;
	int used_size = 0;
	if( countseg && colsize )
	{		
		uint32_t nChar = 0;
		uint32_t col_offset = 0;  // pixel size of nShown
		uint32_t nSegShown = 0;   // how many characters of this segment have been put out
		uint32_t nShown = 0;   // how many characters have been put out
		while( countseg )
		{
			uint32_t nLen = GetTextSize( countseg );
			// part of this segment has already been skipped.
			// ComputeNextOffset can do this.
			if( !nLen )
			{
				// empty segment is linebreak....
				if( !count_trailing_linefeeds && !countseg->Next )
				{
					while( countseg->Prior && !GetTextSize( countseg->Prior ) )
					{
						nLines--;
						countseg = PRIORLINE( countseg );
					}
					return nLines;
				}
				//else
				//	nLines++;
				//if( countseg->Prior || countseg->Next )
				//	nLines++;
			}
			else
			{
				uint32_t text_size;
				uint32_t text_lines;
				phbr->measureString( phbr->psvMeasure, GetText( countseg ), nLen, &text_size, &text_lines, font );
				if( ( text_lines > phbr->nLineHeight ) 
					|| ( ( col_offset + text_size ) > colsize ) )
				{
					uint32_t _nShown = nShown;
					// this is the wrapping condition...
					while( nSegShown < nLen )
					{
						int skip_char;
						uint32_t nShow = ComputeToShow( colsize, &col_offset, countseg, nLen, nChar, nSegShown, phbr, font );
						if( nShow + nSegShown > 1 ) {
							if( GetText( countseg )[nShow + nSegShown - 1] == '\n' )
								skip_char = 1;
							else
								skip_char = 0;
						}
						else if( PRIORLINE( countseg ) ) {
							skip_char = 0;
						}
						else
							skip_char = 0;

						if( ( nShow == 0 ) && ( nSegShown < nLen ) )
							nShow++;
						nSegShown += nShow;
						nShown += nShow ;
						_nShown = nShown;
						if( nSegShown < nLen || skip_char )
						{
							nLines++;
							nChar = 0;
							col_offset = 0;
						}
					}
				}
				else
				{
					nChar += nLen;
					col_offset += text_size;
				}
			}
			nSegShown = 0;
			countseg = NEXTLINE( countseg );
		}
	}
	return nLines;
}

//----------------------------------------------------------------------------

// nOffset is number of lines to move...
int AlignHistory( PHISTORY_BROWSER phbr, int32_t nOffset, SFTFont font )
{
	int result = UPDATE_NOTHING;
	//lprintf( "--------------- ALIGN HISTORY -------------" );
	// sets the current line pointer to the current position plus/minus nOffset.

	// this routine will fix the nHistoryDisplay to be
	// within some block... consider boundry where condition
	// may cause bounce...
	// also - this will make sure that there is a minimum of
	// nHistoryLines left to be displayed...
	if( !phbr->pBlock )
	{
		PHISTORYBLOCK phb = phbr->region->pHistory.next;
		while( phb && phb->next && phb->next->nLinesUsed )
			phb = phb->next;
		phbr->nLine = phb->nLinesUsed - 1;
		phbr->pBlock = phb;
	}
	while( ( nOffset < 0 ) && phbr->pBlock )
	{
		//lprintf( "Offset is %d and is < 0", nOffset );
		nOffset++;
		if( phbr->nLine > 1 )
		{
			if( phbr->nOffset )
			{
				lprintf( WIDE("Previously had an offset... %d"), phbr->nOffset );
				phbr->nOffset--;
			}
			else
			{
				int n;
				phbr->nLine--;
				n = CountLinesSpanned( phbr
					, phbr->pBlock->pLines[phbr->nLine-1].pLine, font, FALSE );
				//lprintf( "total span is what ? %d", n );
				if( n )
					phbr->nOffset = n-1;
			}
		}
		else
		{
			if( (*phbr->pBlock->prior->me) )
			{
				lprintf( WIDE("jumping back naturally.") );
				phbr->pBlock = phbr->pBlock->prior;
				phbr->nLine = phbr->pBlock->nLinesUsed;
			}
			else
			{
				// fell off the beginning of history.
				// and nline is already 0....
				// phlc->nLine = 0;
				lprintf( WIDE("Fell off of history ... need to lock down at first line.") );
				phbr->pBlock = phbr->region->pHistory.next;
				phbr->nLine = 1; // minimum.
				nOffset = 0;
				break;
			}
			// plus full number of lines left...
			// always at least 1, but my be more from this...
			// but it is truncated result, hence it does not include the
			// 1 for the partial line (which is what the first 1 is - partial line)
			if( phbr->pBlock->pLines[phbr->nLine-1].nLineLength )
			{
				int tmp = ( phbr->pBlock->pLines[phbr->nLine-1].nLineLength - 1 ) / phbr->nColumns;
				lprintf( WIDE("Offset plus uhmm... %d"), tmp );
				nOffset += tmp;
			}
		}
	}
	{
		// fix up forward motion
		// N == number of lines the current line is...
		// the offset is the total distance of lines we wish
		// to move.
		int n;
		while( phbr->pBlock &&
				( nOffset > 0 ) )
		{
			n = CountLinesSpanned( phbr
				, phbr->pBlock->pLines[phbr->nLine-1].pLine, font, FALSE );
			//lprintf( "fixing forward motion offset: %d this spans %d", nOffset, n );
			if( n - phbr->nOffset > nOffset )
			{
				// okay then how do we find the offset of this line?
				// probably consult the uhmm gather code...
				phbr->nOffset = n - nOffset;
				nOffset = 0;
			}
			else
			{
				nOffset -= (n/*+1*/);
				//nOffset--; // subtrace one just for moving a line.
				phbr->nLine++;
				phbr->nOffset = 0;
				if( (int64_t)phbr->nLine > phbr->pBlock->nLinesUsed )
				{
					phbr->pBlock = phbr->pBlock->next;
					phbr->nLine = 1;
				}
			}
		}
		// fell off the tail of history - clear out shown history
		// and then we're all done....
		if( !phbr->pBlock )
		{
			phbr->nLine = 0;
			return result;
		}
	}
	//!!!!!!!!!!!!!!!!!!
	// this bit of code needs more smarts - to advance to
	// the line correctly located at the end of the set
	// of history lines shown and stay there....
	// fixup before all history...

	if( phbr->nLine < 0 )
	{
		nOffset = 0;
		phbr->nLine = 1;

	}

	if( !phbr->pBlock )
	{
		phbr->pBlock = phbr->region->pHistory.last;
	}

	// fixup alignment beyond the current block...
	while( (int64_t)phbr->nLine > phbr->pBlock->nLinesUsed
			&& phbr->pBlock->next )
	{
		phbr->nLine -= phbr->pBlock->nLinesUsed;
		phbr->pBlock = phbr->pBlock->next;
	}
	if( SUS_GT( phbr->nLine, int32_t, phbr->pBlock->nLinesUsed, uint32_t ) )
	{
		phbr->nLine = 0;
		phbr->pBlock = NULL;
	}
	return result;
}
//----------------------------------------------------------------------------

uint32_t GetBrowserDistance( PHISTORY_BROWSER phbr, SFTFont font )
{
	INDEX nLines = 0; // count of lines...
	PHISTORYBLOCK pHistory;
	INDEX n;
	for( n = phbr->nLine, pHistory = phbr->pBlock;
		 pHistory;
		  (pHistory = pHistory->next), n=0 )
	{
		for( ; n < pHistory->nLinesUsed; n++ )
		{
			nLines += CountLinesSpanned( phbr
				, pHistory->pLines[n].pLine, font, FALSE );
		}
	}
	lprintf( WIDE("Browser is %")_size_f WIDE(" lines from end..."), nLines );
	return nLines;
}

//----------------------------------------------------------------------------

int MoveHistoryCursor( PHISTORY_BROWSER browser, int amount )
{
	AlignHistory( browser, amount, NULL );
	return UPDATE_HISTORY;
}

//----------------------------------------------------------------------------
// return the x position of the visible cursor
int GetCommandCursor( PHISTORY_BROWSER phbr
					 , SFTFont font
						  , PUSER_INPUT_BUFFER CommandInfo
						  , int bEndOfStream
						, int bWrapCommand
						  , int *command_offset
						  , int *command_begin
						  , int *command_end
						  , int *line_offset
						  )
{
	PTEXT pCmd;
	uint32_t tmpx = 0, nLead, tmp_end;
	PDISPLAYED_LINE pdl;
	uint32_t lines;

	if( !CommandInfo )
		return 0;
	// else there is no history...
	//lprintf( "bendofstream = %d", bEndOfStream );
	if( bEndOfStream && !bWrapCommand )
	{
		(*line_offset) = 0;
		pdl = (PDISPLAYED_LINE)GetDataItem( &phbr->DisplayLineInfo, 0 );
		if( pdl )
		{
			tmpx = pdl->nToShow;

			if( tmpx > phbr->nColumns - 10 )
			{
				lprintf( WIDE("Drawing direct command prompt is confusing... and I don't like it.") );
				// I have to put the command here, so I have to adjust the visible line...
				// or go down to the next line, and if I'm on the next line, I have to result that also...
				// but then I have to know this before I even start drawing so the last line is not
				// the last line of history, but is a command prompt...
				// also need to count the prompt size (macro:line)...
			}
			nLead = tmpx;
		}
		else
			nLead = 0;
	}
	else
		nLead = 0;

	if( command_offset )
		(*command_offset) = nLead;

	pCmd = CommandInfo->CollectionBuffer;
	SetStart( pCmd );

	if( bWrapCommand )
	{
		if( !bEndOfStream )
			lines = CountLinesSpanned( phbr, pCmd, font, TRUE );
		else
			lines = CountLinesSpanned( phbr, pCmd, font, TRUE );
	}
	else
	{
		if( bEndOfStream )
			lines = 0;  // number of lines we have to push display up...
		else
			lines = 1;
	}

	while( pCmd != CommandInfo->CollectionBuffer )
	{
		tmpx += GetTextSize( pCmd );
		pCmd = NEXTLINE( pCmd );
	}
	tmp_end = tmpx;
	{
		int n;
		int used = 0;
		for( n = 0; n < phbr->nLines; n++ )
		{
			PDISPLAYED_LINE pdl;
			if( pdl = (PDISPLAYED_LINE)GetDataItem( &phbr->DisplayLineInfo, n ) )
				if( pdl->nToShow )
					used++;
		}
		return used;
	}
	while( pCmd )
	{
		uint32_t width, height;
		phbr->measureString( phbr->psvMeasure, GetText( pCmd ), GetTextSize( pCmd ), &width, &height, NULL );
		if( ( width + tmp_end ) > phbr->nWidth )
		{
			// not all of the string fits on the line....
			int a = 3;
		}
		tmp_end += GetTextSize( pCmd );
		pCmd = NEXTLINE( pCmd );
	}
	tmpx += CommandInfo->CollectionIndex;

	if( command_begin )
		(*command_begin) = 0;

	if( command_end )
	{
		if( tmp_end > phbr->nColumns )
			(*command_end) = phbr->nColumns;
		else
			(*command_end) = tmp_end;
	}
	if( tmpx > phbr->nColumns - 10 )
	{
		if( command_begin )
			(*command_begin) = ( tmpx - ( phbr->nColumns - 10 ) ) - nLead;
		tmpx = phbr->nColumns - 10;
	}

	return tmpx;
}

//----------------------------------------------------------------------------

void ResetHistoryBrowser( PHISTORY_BROWSER phbr )
{
	// now unlocked and trails the end...
	phbr->pBlock = NULL;
	phbr->nLine = 0;
}

//----------------------------------------------------------------------------

int KeyEndHst( PHISTORY_BROWSER pht )
{
	ResetHistoryBrowser( pht );
	return UPDATE_HISTORY;
}

//----------------------------------------------------------------------------

int HistoryLineUp( PHISTORY_BROWSER pb )
{
	return MoveHistoryCursor( pb, !pb->pBlock?-(int)pb->nPageLines:-1 );
}

//----------------------------------------------------------------------------

int HistoryLineDown( PHISTORY_BROWSER pb )
{
	if( pb->pBlock )
		return MoveHistoryCursor( pb, 1 );
	return 0;
}

//----------------------------------------------------------------------------

int HistoryPageUp( PHISTORY_BROWSER pb )
{
	//lprintf( "Moving history up %d", pb->nPageLines );
	return MoveHistoryCursor( pb, -(int)pb->nPageLines );
}

//----------------------------------------------------------------------------

int HistoryPageDown( PHISTORY_BROWSER pb )
{
	//lprintf( "Moving history down %d", pb->nPageLines );
	if( pb->pBlock )
		return MoveHistoryCursor( pb, pb->nPageLines );
	return 0;
}

//----------------------------------------------------------------------------

PDATALIST *GetDisplayInfo( PHISTORY_BROWSER phbr )
{
	return &phbr->DisplayLineInfo;
}


int CountDisplayedLines( PHISTORY_BROWSER phbr )
{
	int n;
	int used = 0;
	for( n = 0; n < phbr->nLines; n++ )
	{
		PDISPLAYED_LINE pdl;
		if( pdl = (PDISPLAYED_LINE)GetDataItem( &phbr->DisplayLineInfo, n ) )
			if( pdl->start )
				used++;
	}
	return used;
}

void BuildDisplayInfoLines( PHISTORY_BROWSER phbr, SFTFont font )
//void BuildDisplayInfoLines( PHISTORY_LINE_CURSOR phlc )
{
	int nLines, nLinesShown = 0;
	int nChar;  // this with col_offset is character count on line
	uint32_t nLen;
	uint32_t col_offset; // pixel position to match nChar 
	uint32_t nSegShown = 0; // characters on segment shown
	int nLineCount = phbr->nLines;
	PTEXT pText;
	PDATALIST *CurrentLineInfo = &phbr->DisplayLineInfo;
	int start;
	int firstline = 1;
	uint32_t nShown = 0; // total length of all segs shown on a line... 
	PDISPLAYED_LINE pLastSetLine = NULL;
	PTEXTLINE pLastLine = GetAHistoryLine( NULL, phbr, 0, FALSE );
	//PHISTORY phbStart;
	if( *CurrentLineInfo )
		(*CurrentLineInfo)->Cnt = 0;
	start = 0;
	EnterCriticalSec( &phbr->cs );
	//lprintf( "------------- Building Display Info Lines %d", nLineCount );
	//lprintf( "nShown starts at %d", nShown );
	if( phbr->nColumns )
		while( ( nLinesShown < nLineCount ) &&
				 ( pText = EnumHistoryLine( phbr
												  , &start
												  , NULL ) ) )
		{
			DISPLAYED_LINE dl;
			//lprintf( " Lines %d of %d", nLinesShown, nLineCount );
			// can't show past the top of form.
			// while bhilding display info lines...
			// this is history info lines also?!
			if( ( pText->flags & TF_FORMATEX ) && ( pText->format.flags.format_op == FORMAT_OP_PAGE_BREAK ) )
			{
				if( !phbr->flags.bNoPageBreak && !phbr->flags.bOwnPageBreak )
				{
					//DebugBreak();
					break; // done!
				}
				if( phbr->flags.bNoPageBreak && !phbr->flags.bOwnPageBreak )
					continue;
			}
			if( pLastLine && pText == pLastLine->pLine )
			{
				lprintf( WIDE("top of form reached, bailing early on available history.") );
				break;
			}
			if( phbr->flags.bWrapText )
			{
				int nShow;
				int nWrapped = 0;

				nLines = CountLinesSpanned( phbr, pText, font, FALSE );
				// after counting the first (last visible) line
				// figure out how much we need to overflow to show the
				// last partial line...
				// phbr->nOffset = 0->nLines-1 of wrap.  This
				// is the index of the line to show...
				// nOffset 0 == the beginning of the line
				// noffset 1 == second part of line...
				// if nOffset is small, then the extra lines
				// need to be thrown out... which means a
				// large nLines will cause a negative nLinesShown...
				//
				if( firstline )
				{
					//lprintf( "Fixup of first line by offset... %d %d"
					//		  , phbr->nOffset, nLines );
					if( 1 )
						;
					else
						nLinesShown = phbr->nOffset - (nLines-1);
					firstline = 0;
				}
				//lprintf( "Wraping text, current line is at most %d", nLines );
				nChar = 0;
				col_offset = 0;
				pLastSetLine = NULL;
				while( pText )
				{
#ifdef __DEKWARE_PLUGIN__
					if( phbr->flags.bNoPrompt
						&& ( pText->flags & TF_PROMPT )
					  )
					{
					  // lprintf( "skipping prompt segment..." );
						pText = NEXTLINE( pText );
						continue;
					}
#endif
					if( !pLastSetLine )
					{
						dl.nLine = start; // just has to be different
						dl.nFirstSegOfs = 0; // start of a new line here...
						dl.start = pText; // text in history that started this...
						dl.nToShow = 0;
						dl.nLineStart = 0;
						dl.nLineEnd = -1;
						//lprintf( "Adding line to display: %p (%d) %d", dl.start, dl.nOfs, dl.nLine );
						if( nLinesShown + nLines > 0 )
						{
							//lprintf( "Set line %d %s", nLinesShown + nLines, GetText( dl.start ) );
							pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																					 , nLinesShown + nLines -1
																					 , &dl );
						}
					}

					nLen = GetTextSize( pText );
					{
						int trim_char;
						// shown - show part of this segment... but not the whole segment...
						if( !nLen )
						{
							trim_char = 0;
							pText = NEXTLINE( pText );
							nLen = GetTextSize( pText );
							goto do_end_of_line;
						}
						else while( nSegShown < nLen )
						{
							trim_char = 0;
							// nShow is now the number of characters we can show.
							//lprintf( "Segment is %d", GetTextSize( pText ) );
							nShow = ComputeToShow( phbr->nWidth, &col_offset, pText, nLen, nChar, nSegShown, phbr, font );
							// in case we wrap 0 characters for columns less than a width of a character...
							if( nShow == 0 && nSegShown < nLen )
								nShow = 1;
							nSegShown += nShow;
							nShown += nShow;
							nChar += nShow;
							if( GetText( pText )[nSegShown-1] == '\n' )
								trim_char = 1;
							else
								trim_char = 0;
							 // wrapped on a space - word break in segment
							// or had a newline at the end which causes a wrap...
							// log the line and get a new one.
							if( ( nSegShown < nLen ) || ( trim_char ) )
							{
								//lprintf( "Wrapped line... reset nChar cause it's a new line of characters." );
								nWrapped++;
							do_end_of_line:

								//lprintf( "Setting prior toshow to nChar...%d", nChar );
								pLastSetLine->nToShow = nChar - trim_char;
								pLastSetLine->nPixelEnd = col_offset;
								pLastSetLine->nLineEnd = pLastSetLine->nLineStart + ( nChar ) - 1;
								dl.nLineEnd = pLastSetLine->nLineEnd;
								dl.nLineStart = dl.nLineEnd + 1;

								// begin a new line output
								nChar = 0;
								col_offset = 0;

								// skip next leading spaces.
								// (and can be more than the length of the first segment)
								//nShown = ComputeNextOffset( pText, nShown );
								//if( pText )
								{
									dl.nLine = nShown+start; // just has to be different
									dl.nFirstSegOfs = nSegShown; // start of a new line here...
									dl.start = pText; // text in history that started this...
									//dl.nToShow = nLen - nChar;
									//lprintf( "Adding line to display: %p (%d) %d", dl.start, dl.nOfs, dl.nLine );
									if( ( nLinesShown + nLines - nWrapped ) > 0 )
									{
										//lprintf( "Set line %d", nLinesShown + nLines - nWrapped );
										pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																		  , nLinesShown + nLines - nWrapped -1
																		  , &dl );
									}
								}
								//else
								//	pLastSetLine = NULL;
							}
						}
					}
					pText = NEXTLINE( pText );
					nSegShown = 0;
				}
				if( pLastSetLine )
				{
					//lprintf( "Fixing up last line set for number of chars to show. %d", nChar );
					pLastSetLine->nToShow += nChar;
					pLastSetLine->nLineEnd = pLastSetLine->nLineStart + ( nChar - 1 );
					pLastSetLine->nPixelEnd = col_offset;
					nLinesShown += nLines;
				}
			}
			else
			{
				// this uses the cursorX to determine the left character to show...
				// cursor Y is the bottom line to show.
				dl.nFirstSegOfs = 0;
				dl.start = NULL;
				dl.nLine = 0;
				dl.nToShow = 0;
				dl.nLineStart = dl.nLineEnd = 0;
				nLines = 1;

				nChar = 0;
				col_offset = 0;
				lprintf( WIDE("Clearing last set line...") );
				pLastSetLine = NULL;
				while( ( dl.start = pText ) )
				{
#ifdef __DEKWARE_PLUGIN__
					if( !phbr->flags.bNoPrompt && ( pText->flags & TF_PROMPT ) )
					{
						pText = NEXTLINE( pText );
						continue;
					}
#endif
					if( !pLastSetLine )
					{
						nLen = GetTextSize( pText );
						if( USS_LT( (dl.nFirstSegOfs + nLen), uint32_t, phbr->nOffset, int ) )
						{
							lprintf( WIDE("Skipping segement, it's before the offset...") );
							dl.nLineEnd += nLen;
							dl.nFirstSegOfs += nLen;
						}
						else
						{
							dl.nLine = start; // just has to be different
							//dl.start = pText; // text in history that started this...
							lprintf( WIDE("Adding line to display: %p (%")_size_f WIDE(") %")_size_f WIDE(" %d" )
									 , dl.start, dl.nFirstSegOfs, dl.nLine, nLinesShown+nLines );
							if( nLinesShown + nLines > 0 )
							{
								lprintf( WIDE("Set line %d"), nLinesShown + nLines );
								pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																						 , nLinesShown + nLines -1
																						 , &dl );
							}
							lprintf( WIDE("After set last set line is %p"), pLastSetLine );
						}
					}
					pText = NEXTLINE( pText );
				}
				if( !pLastSetLine )
				{
					// dl will be initialized as a blank line...
					lprintf( WIDE("Adding line to display: %p (%")_size_f WIDE(") %") _size_f, dl.start, dl.nFirstSegOfs, dl.nLine );
					if( nLinesShown + nLines > 0 )
					{
							lprintf( WIDE("Set line %d"), nLinesShown + nLines );
						pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																				 , nLinesShown + nLines -1
																				 , &dl );
					}
				}
				nLinesShown++;
			}
		}
	else
		lprintf( WIDE("No column defined for browser! please Fix me!") );
	//clear_remaining_lines:
	//lprintf( "Clearning lines %d to %d", nLinesShown, nLineCount );
	LeaveCriticalSec( &phbr->cs );
}

//----------------------------------------------------------------------------

void SetCursorNoPrompt( PHISTORY_BROWSER phbr, LOGICAL bNoPrompt )
{
	phbr->flags.bNoPrompt = bNoPrompt;
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
void SetCursorLines( PHISTORY_LINE_CURSOR phlc, int n )
{
	phlc->nLines = n;
	// recompute visible lines...
}

//----------------------------------------------------------------------------

void SetBrowserLines( PHISTORY_BROWSER phbr, int n )
{
	phbr->nLines = n;
	// recompute visible lines...
}
//----------------------------------------------------------------------------

void SetBrowserColumns( PHISTORY_BROWSER phbr, int n, int size )
{
	phbr->nColumns = n;
	phbr->nWidth = size;
	// recompute visible lines...
}
//----------------------------------------------------------------------------
void SetCursorColumns( PHISTORY_LINE_CURSOR phlc, int n )
{
	phlc->nColumns = n;
	// recompute visible lines...
}
//----------------------------------------------------------------------------

void SetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, int32_t x, int32_t y )
{
	phlc->output.nCursorX = (int16_t)x;
	phlc->output.nCursorY = (int16_t)y;
}

//----------------------------------------------------------------------------

void PSI_SetHistoryPageLines( PHISTORY_BROWSER phbr, uint32_t nLines )
{
	//lprintf( "Set histpry lines at %d", nLines );
	phbr->nPageLines = nLines;
}

//----------------------------------------------------------------------------

void PSI_SetHistoryBrowserNoPageBreak( PHISTORY_BROWSER phbr )
{
	phbr->flags.bNoPageBreak = 1;
}

//----------------------------------------------------------------------------

void PSI_SetHistoryBrowserOwnPageBreak( PHISTORY_BROWSER phbr )
{
	phbr->flags.bOwnPageBreak = 1;
}

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
