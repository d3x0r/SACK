#include <stdhdrs.h>
#include <stdio.h>
#include <logging.h>
#include <sharemem.h>

#include "histstruct.h"

#include "consolestruc.h" // all relavent includes

//#include "interface.h"
extern int myTypeID;

void SetTopOfForm( PHISTORY_LINE_CURSOR phlc );
//----------------------------------------------------------------------------

PHISTORYBLOCK DestroyRawHistoryBlock( PHISTORYBLOCK pHistory )
{
	_32 i;
	PHISTORYBLOCK next;
	for( i = 0; i < pHistory->nLinesUsed; i++ )
	{
		if( pHistory->pLines[i].pLine == 0xFeeefeee )
		{
			lprintf( "a deleted line is in history %d", i );
		}
		LineRelease( pHistory->pLines[i].pLine );
	}
	if( ( next = ( (*pHistory->me) = pHistory->next ) ) )
		pHistory->next->me = pHistory->me;
	Release( pHistory );
	return next;
}

//----------------------------------------------------------------------------

void DestroyHistoryRegion( PHISTORY_REGION phr )
{
   if( phr )
   {
      while( phr->pHistory.next )
         DestroyRawHistoryBlock( phr->pHistory.next );
      Release( phr );
   }
}
//----------------------------------------------------------------------------

void DestroyHistoryBlock( PHISTORY_REGION phr )
{
   // just destroy the first block...
   if( phr )
   {
      DestroyRawHistoryBlock( phr->pHistory.next );
   }
}


//----------------------------------------------------------------------------
void DestroyHistory( PHISTORYTRACK pht )
{
   if( pht )
   {
      DestroyHistoryRegion(pht->pRegion);
      Release( pht );
   }
}

//----------------------------------------------------------------------------

void DestroyHistoryBrowser( PHISTORY_BROWSER phbr )
{
   DeleteDataList( &phbr->DisplayLineInfo );
   Release( phbr );
}
//----------------------------------------------------------------------------

void DestroyHistoryCursor( PHISTORY_LINE_CURSOR phlc )
{
   Release( phlc );
}
//----------------------------------------------------------------------------

PHISTORYBLOCK CreateRawHistoryBlock( void )
{
	PHISTORYBLOCK pHistory;
   pHistory = New( HISTORYBLOCK );
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

PHISTORY_LINE_CURSOR CreateHistoryCursor( PHISTORY_REGION phr )
{
	if( phr )
	{
		PHISTORY_LINE_CURSOR phlc = New( HISTORY_LINE_CURSOR );
		MemSet( phlc, 0, sizeof( HISTORY_LINE_CURSOR ) );

#if 0 && defined( _WIN32 ) && !defined( PSICON )
		{
			int nValue;
			PTEXT name = NULL;
			if( GetRegistryInt( WIDE("Dekware\\Wincon\\Background")
									, name?GetText( name ):WIDE("default")
									, &nValue ) )
				phlc->output.DefaultColor.background = nValue;
			else
#endif
				phlc->output.DefaultColor.flags.background = 0;
#if 0 && defined( _WIN32 ) && !defined( PSICON )
			if( GetRegistryInt( WIDE("Dekware\\Wincon\\Foreground")
									, name?GetText( name ):WIDE("default")
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

PHISTORY_REGION CreateHistoryRegion( void )
{
	PHISTORY_REGION phr = New( HISTORY_REGION );
	MemSet( phr, 0, sizeof( HISTORY_REGION ) );
	phr->pHistory.me = &phr->pHistory.next;
	CreateHistoryBlock( &phr->pHistory.root );
	return phr;
}

//----------------------------------------------------------------------------

PHISTORY_BROWSER CreateHistoryBrowser( PHISTORY_REGION region )
{
	PHISTORY_BROWSER phbr = New( HISTORY_BROWSER );
	MemSet( phbr, 0, sizeof( HISTORY_BROWSER ) );
	InitializeCriticalSec( &phbr->cs );
	phbr->region = region;
	phbr->DisplayLineInfo = CreateDataList( sizeof( DISPLAYED_LINE ) );
	phbr->flags.bWrapText = 1;
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
	//_xlprintf( 0 DBG_RELAY )(WIDE("Getting some history line: %d"), line );
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
      //lprintf( WIDE("overriding start, using line as rel index back... now is %d"), line );
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
	            //lprintf( WIDE("Adjusting line as we step backward...") );
				line += pBlock->nLinesUsed;
			}
		}
		if( line > 0 )
		{
			if( line < MAX_HISTORY_LINES )
			{
				//lprintf( WIDE("Setting used lines in history block...") );
				//pBlock->nLinesUsed = line;
			}
			return pBlock->pLines + (line - 1);
		}
   }
   return NULL;
}

//----------------------------------------------------------------------------

// if bExpand ...
//   this is done during normal line processing
//   and if this is the case - then Y is an apsolute
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
   //lprintf( WIDE("INcrementint lines used... returning this line at %d"), pBlock->nLinesUsed );
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
		size_t tmp;
		PHISTORYBLOCK phb;
		//lprintf( WIDE("Get line %d after %d"), nLine, phc->output.top_of_form.line );

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
		//lprintf( WIDE("tmp is now %d"), tmp );
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
		size_t tmp;
		PHISTORYBLOCK phb;
		//lprintf( WIDE("Get line %d after %d"), nLine, phbr->nLine );

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
		//lprintf( WIDE("tmp is now %d"), tmp );
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

PTEXTLINE PutSegmentOut( PHISTORY_LINE_CURSOR phc
							  , PTEXT segment )
{
   // this puts the segment at the correct x, y
   // it also updates the curren X on the65 line...
   PTEXTLINE pCurrentLine;
   PTEXT text;
   size_t len;
   if( !(phc->region->pHistory.next) ||
       ( phc->region->pHistory.last->nLinesUsed == MAX_HISTORY_LINES &&
         !(segment->flags&TF_NORETURN) ) )
		CreateHistoryBlock( &phc->region->pHistory.root );

	if( GetTextSize( segment ) & 0x80000000)
		lprintf( "(original input)Inverted Segment : %d", GetTextSize( segment ) );

	//lprintf( WIDE("Okay after much layering... need to fix some....") );
   //lprintf( WIDE("Put a segment out!") );
   // get line at nCursorY
	pCurrentLine = GetHistoryLine( phc );
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
      //Log1( WIDE("Make a filler segment %d charactes"), phc->region->(phc->output.nCursorX) - pCurrentLine->nLineLength );
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
		S_32 pos = 0;
      //lprintf( WIDE("Okay insert/overwrite this segment on the display...") );
      text = pCurrentLine->pLine;
      while( pos < (phc->output.nCursorX) && text )
      {
         len = GetTextSize( text );
         //Log3( WIDE("Skipping over ... pos:%d len:%d curs:%d"), pos, len, phc->region->(phc->output.nCursorX) );
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
      //Log( WIDE("Cursor is at end or after end.") );
      text = NULL;
      len = 0;
   }

			if( GetTextSize( text ) & 0x80000000)
				lprintf( "(After processing) Inverted Segment : %d", GetTextSize( text ) );
   // expects 'text' to be the start of the correct segment.
   if( segment->flags & TF_FORMATEX )
	{
      Log( WIDE("Extended info carried on this segment.") );
      segment->flags &= ~TF_FORMATEX;
      switch( segment->format.flags.format_op )
      {
      case FORMAT_OP_CLEAR_END_OF_LINE: // these are difficult....
         //Log( WIDE("Clear end of line...") );
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
            Log1( WIDE("Has a current segment... %d"), GetTextSize( text ) );
            if( text == pCurrentLine->pLine )
            {
               Log( WIDE("is the first segment... "));
               split = SegSplit( &pCurrentLine->pLine, segment->format.flags.background );
               text = pCurrentLine->pLine;
            }
            else
            {
               Log( WIDE("Is not first - splitting segment... "));
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
            //Log1( WIDE("Make a filler segment %d charactes"), (phc->output.nCursorX) - pCurrentLine->nLineLength );
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
         pCurrentLine = GetHistoryLine( phc );
			SetTopOfForm( phc );
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
   //   Log( WIDE("No extended info carried on this segment.") );


   if( GetTextSize( segment ) )
   {
      if( text )
      {
         //Log( WIDE("Inserting new segment...") );
         SegInsert( segment, text );
         if( text == pCurrentLine->pLine )
            pCurrentLine->pLine = segment;
      }
      else
      {
         //Log( WIDE("Appending segment.") );
         pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, segment );
      }

      len = GetTextSize( segment );
      (phc->output.nCursorX) += len;

      if( !phc->region->flags.bInsert && text )
      {
         // len characters from 'text' need to be removed.
         // okay that should be fun...
         _32 deletelen = len;
         //Log1( WIDE("Remove %d characters!"), len );
         while( deletelen && text )
         {
            _32 thislen = GetTextSize( text );
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
		pCurrentLine->pLine = SegAppend( pCurrentLine->pLine, segment );
      SetTopOfForm( phc );
	}
   else
      Log( WIDE("no data in segment.") );
   return pCurrentLine;
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
		 responce->format.flags.foreground = FORMAT_OP_SET_CURSOR;
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

void EnqueDisplayHistory( PHISTORY_LINE_CURSOR phc, PTEXT pLine )
{
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
      //lprintf( WIDE("Not NORETURN - therefore skipping to next line...") );
		phc->output.nCursorX = 0;
      phc->output.nCursorY++;
	}

	{
		PTEXTLINE pCurrentLine = NULL;
		//lprintf( WIDE("Flattening line...") );
		pLine = FlattenLine( pLine );
		ResolveLineColor( phc, pLine );

		// colors are processed in the stream of output - not by position
		// lprintf okay putting segments out...
		{
			PTEXT next = pLine;
			while( ( pLine = next ) )
			{
				next = NEXTLINE( pLine );
				if( GetTextSize( pLine ) // if segment has visible data...
					|| ( pLine->flags & TF_FORMATEX ) )  // or a special effect...
					pCurrentLine = PutSegmentOut( phc, SegGrab( pLine ) );
				else
				{
					LineRelease( SegGrab( pLine ) ); // won't be enqueued might as well destroy it.
				}
			}
		}
		if( pCurrentLine ) // else no change...
		{
			pCurrentLine->nLineLength = LineLength( pCurrentLine->pLine );
		}
	}
}

//----------------------------------------------------------------------------

void DumpBlock( PHISTORYBLOCK pBlock DBG_PASS )
{
   INDEX idx;
	_xlprintf( 0 DBG_RELAY )(WIDE("History block used lines: %d of %d"), pBlock->nLinesUsed, MAX_HISTORY_LINES );
	for( idx = 0; idx < pBlock->nLinesUsed; idx++ )
	{
      PTEXTLINE ptl = pBlock->pLines + idx;
		_xlprintf( 0 DBG_RELAY )(WIDE("line: %d = (%d,%d,%s)"), idx, ptl->nLineLength, GetTextSize( ptl->pLine ), GetText( ptl->pLine ) );
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
							  , S_32 *length DBG_PASS)
#define EnumHistoryLine(hb,o,l) EnumHistoryLineEx(hb,o,l DBG_SRC )
{
	PTEXTLINE ptl;
   int line = -(*offset);
	if( !offset )
	{
		Log( WIDE("No start...") );
		return NULL;
	}
	//lprintf(WIDE("Getting line: %d "), line );
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
			//lprintf( WIDE("Resulting: (%d)%s"), ptl->nLineLength, GetText( ptl->pLine ) );
			return ptl->pLine;
		}
		else
		{
			DECLTEXT( nothing, WIDE("") );
			nothing.format.flags.foreground = 0;
			nothing.format.flags.background = 0;
			if( length )
				*length = 0;
         //lprintf( WIDE("resulting nothing. %d"), line );
			return (PTEXT)&nothing;
		}
	}
	//Log( WIDE("No result") );
	return NULL;
}

//----------------------------------------------------------------------------

void SetHistoryLength( PHISTORY_REGION phr, INDEX length )
{
	Log1( WIDE("Set Length to %d blocks"), length/MAX_HISTORY_LINES );
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

void InitHistoryRegion( PHISTORY_REGION pht, PTEXT name )
{
   MemSet( pht, 0, sizeof( HISTORY_REGION ) );
   pht->tabsize = 8;
   pht->nMaxHistoryBlocks = 5000; // configurable option now...
}

//----------------------------------------------------------------------------

void WriteHistoryToFile( FILE *file, PHISTORY_REGION phr )
{
	PHISTORYBLOCK pHistory = phr->pHistory.next;
	INDEX idx;
	while( pHistory )
	{
      lprintf( WIDE("Have a history block with %d lines"), pHistory->nLinesUsed );
		for( idx = 0; idx < pHistory->nLinesUsed; idx++ )
		{
			PTEXTLINE pLine = pHistory->pLines + idx;
			PTEXT pText = pLine->pLine;
			while( pText )
			{
            lprintf( WIDE("%s"), GetText(pText) );
				fwrite( GetText( pText ), 1, GetTextSize( pText ), file );
				pText = NEXTLINE( pText );
			}
			fprintf( file, WIDE("\n") );
		}
		pHistory = pHistory->next;
	}
}
//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
int SetHistory( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
	PCONSOLE_INFO pmdp;
	PTEXT temp;
	pmdp = (PCONSOLE_INFO)pdp;
	if( !pmdp )
	{
		DECLTEXT( msg, WIDE("Datapath lacks history") );
		EnqueLink( &ps->Command->Output, &msg );
		//RemoveMethod( ps->Current, methods+SET_HISTORY_METHOD );
		return 0;
	}
	temp = GetParam( ps, &parameters );
	if( !temp )
	{
		DECLTEXT( msg, WIDE("Need to specify the length of history to be kept") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	else
	{
		if( IsNumber( temp ) )
		{
			int length = IntNumber( temp );
			Log1( WIDE("Length is: %d"), length );
			if( length > 2*MAX_HISTORY_LINES )
			{
				SetHistoryLength( pmdp->pHistory, length );
			}
			else
			{
				DECLTEXT( msg, WIDE("History length must be at least 2 * ")STRSYM(MAX_HISTORY_LINES)WIDE(".") );
				EnqueLink( &ps->Command->Output, &msg );
			}
		}
		else
		{
			DECLTEXT( msg, WIDE("Parameter to set history must be a number.") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return 0;
}
#endif
//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
int HistoryStat( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PCONSOLE_INFO pmdp;
   PTEXT stat;
   stat = SegCreate( 256 );
   pmdp = (PCONSOLE_INFO)pdp;

   if( !pmdp )
   {
      DECLTEXT( msg, WIDE("Datapath lacks history") );
      EnqueLink( &ps->Command->Output, &msg );
      return 0;
   }
	SetTextSize( stat, snprintf( GetText( stat ), 256*sizeof(TEXTCHAR), WIDE("History Length: %ld")
									  , GetHistoryLength( pmdp->pHistory ) ) );
	EnqueLink( &ps->Command->Output, stat );
   return 1;

}

#endif
//----------------------------------------------------------------------------

_32 ComputeNextOffset( PTEXT segment, _32 nShown )
{
    _32 offset = 0;
    while( segment )
    {
        _32 nLen = GetTextSize( segment );
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

int ComputeToShow( _32 cols, PTEXT segment, _32 nLen, int nOfs, int nShown )
{
    int nShow = cols - nOfs;
	 // if space left to show here is less than
	 // then length to show, compute wrapping point.
	 //lprintf( WIDE("Compute to show: %d (%d)%s %d %d"), cols, GetTextSize( segment ), GetText( segment ), nOfs, nShown );
    if( nShow < (nLen-nShown) )
    {
		int nSpace = nShow + nShown;
		TEXTCHAR *text = GetText( segment );

		  // cheap test for a space...
		  for( ; nSpace > nShown && ( text[nSpace-1] != ' ' ); nSpace-- );
		  //for( ; nSpace > nShown && ( text[nSpace-1] == ' ' ); nSpace-- );

        // found a space, please show up to that.
        if( nSpace > nShown )
            nShow = nSpace - nShown;
		  else if( nOfs ) // if started after a line, wrap whole thing to next...
			  nShow = 0;
		  // failing all of this nShow will be length, and non delimited text
        // will wrap forcably.
    }
    else
		 nShow = nLen - nShown;
    //lprintf( WIDE("Show %d"), nShow );
    return nShow;
}

//----------------------------------------------------------------------------

int CountLinesSpanned( _32 cols, PTEXT countseg )
{
   // always spans at least one line.
	int nLines = 1;
	if( countseg && cols )
	{
		_32 nShown = 0;
		_32 nChar = 0;
		while( countseg )
		{
			_32 nLen = GetTextSize( countseg );
			// part of this segment has already been skipped.
			// ComputeNextOffset can do this.
			if( nLen & 0x80000000)
				lprintf( "Inverted Segment : %d", nLen );
			else
			{
				if( nShown > nLen )
				{
					nShown -= nLen;
				}
				else // otherwise nShown is within this segment...
				{
					if( ( nChar + nLen ) > cols )
					{
						// this is the wrapping condition...
						while( nShown < nLen )
						{
							nShown += ComputeToShow( cols, countseg, nLen, nChar, nShown );
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
			}
			countseg = NEXTLINE( countseg );
		}
	}
	return nLines;
}

//----------------------------------------------------------------------------

// nOffset is number of lines to move...
int AlignHistory( PHISTORY_BROWSER phbr, S_32 nOffset )
{
	int result = UPDATE_NOTHING;
   //lprintf( WIDE("--------------- ALIGN HISTORY -------------") );
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
      //lprintf( WIDE("Offset is %d and is < 0"), nOffset );
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
				n = CountLinesSpanned( phbr->nColumns
											, phbr->pBlock->pLines[phbr->nLine-1].pLine );
				//lprintf( WIDE("total span is what ? %d"), n );
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
			n = CountLinesSpanned( phbr->nColumns
										, phbr->pBlock->pLines[phbr->nLine-1].pLine );
			//lprintf( WIDE("fixing forward motion offset: %d this spans %d"), nOffset, n );
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
				if( phbr->nLine > phbr->pBlock->nLinesUsed )
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
	while( phbr->nLine > phbr->pBlock->nLinesUsed
			&& phbr->pBlock->next )
	{
		phbr->nLine -= phbr->pBlock->nLinesUsed;
		phbr->pBlock = phbr->pBlock->next;
	}
	if( phbr->nLine > phbr->pBlock->nLinesUsed )
	{
		phbr->nLine = 0;
		phbr->pBlock = NULL;
	}
	return result;
}
//----------------------------------------------------------------------------

_32 GetBrowserDistance( PHISTORY_BROWSER phbr )
{
   INDEX nLines = 0; // count of lines...
	PHISTORYBLOCK pHistory;
	int n;
	for( n = phbr->nLine, pHistory = phbr->pBlock;
		 pHistory;
		  (pHistory = pHistory->next), n=0 )
	{
		for( ; n < pHistory->nLinesUsed; n++ )
		{
			nLines += CountLinesSpanned( phbr->nColumns
										  , pHistory->pLines[n].pLine );
		}
	}
	lprintf( WIDE("Browser is %d lines from end..."), nLines );
   return nLines;
}

//----------------------------------------------------------------------------

// result TRUE if nLine/nOffset in cursor is less than nLines from
// the end... else cursor is more than nlines from end of history.
int HistoryNearEnd( PHISTORY_BROWSER phbr, int nLinesAway )
{
	return 0;
#if 0
	{
   INDEX nLines = 0; // count of lines...
	PHISTORYBLOCK pHistory;
	int n;
	lprintf( WIDE("History near end? (within %d lines)"), nLinesAway );
   lprintf( WIDE("hmm %p is block at %d"), phbr->pBlock, phbr->nLine );
	for( n = phbr->nLine, pHistory = phbr->pBlock;
		 pHistory && nLines < nLinesAway;
		  pHistory = pHistory->next )
	{
		lprintf( WIDE("checking %d lines in block...next:%p"), pHistory->nLinesUsed, pHistory->next );
		for( ; nLines < nLinesAway && n < pHistory->nLinesUsed; n++ )
		{
			int tmp;
			tmp = CountLinesSpanned( phbr->nColumns
										  , pHistory->pLines[n].pLine );
			lprintf( WIDE("Adding %d lines..."), tmp );
			nLines += tmp;
		}
      n = 0;
      lprintf( WIDE("total history lines = %d < %d ? (is end)"), nLines, nLinesAway );
	}
	return nLines < phbr->nLines; //( nLines < nLinesAway );
	}
#endif
}

//----------------------------------------------------------------------------

int MoveHistoryCursor( PHISTORY_BROWSER browser, int amount )
{
	AlignHistory( browser, amount );
   return UPDATE_HISTORY;
}

//----------------------------------------------------------------------------
// return the x position of the visible cursor
int GetCommandCursor( PHISTORY_BROWSER phbr
#ifdef __DEKWARE_PLUGIN__
						  , COMMAND_INFO *CommandInfo
#else
						  , PCOMMAND_INFO CommandInfo
#endif
						  , int bEndOfStream
                    , size_t *command_offset
						  , size_t *command_begin
                    , size_t *command_end
						  )
{
	PTEXT pCmd;
	size_t tmpx = 0, nLead, tmp_end;
	PDISPLAYED_LINE pdl;

	if( !CommandInfo )
      return 0;
	// else there is no history...
	//lprintf( WIDE("bendofstream = %d"), bEndOfStream );
   pdl = (PDISPLAYED_LINE)GetDataItem( &phbr->DisplayLineInfo, 0 );
	if( bEndOfStream && pdl )
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

	if( command_offset )
      (*command_offset) = nLead;

	pCmd = CommandInfo->CollectionBuffer;
	SetStart( pCmd );
	while( pCmd != CommandInfo->CollectionBuffer )
	{
		tmpx += GetTextSize( pCmd );
		pCmd = NEXTLINE( pCmd );
	}
   tmp_end = tmpx;
	while( pCmd )
	{
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
	return MoveHistoryCursor( pb, !pb->pBlock?-pb->nPageLines:-1 );
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
   //lprintf( WIDE("Moving history up %d"), pb->nPageLines );
   return MoveHistoryCursor( pb, -pb->nPageLines );
}

//----------------------------------------------------------------------------

int HistoryPageDown( PHISTORY_BROWSER pb )
{
   //lprintf( WIDE("Moving history down %d"), pb->nPageLines );
	if( pb->pBlock )
		return MoveHistoryCursor( pb, pb->nPageLines );
   return 0;
}

//----------------------------------------------------------------------------

PDATALIST *GetDisplayInfo( PHISTORY_BROWSER phbr )
{
   return &phbr->DisplayLineInfo;
}

void BuildDisplayInfoLines( PHISTORY_BROWSER phbr )
//void BuildDisplayInfoLines( PHISTORY_LINE_CURSOR phlc )
{
	int nLines, nLinesShown = 0, nChar, nLen;
	int nLineCount = phbr->nLines;
	PTEXT pText;
	PDATALIST *CurrentLineInfo = &phbr->DisplayLineInfo;
	int start;
	int firstline = 1;
	_32 nShown = 0;
	PDISPLAYED_LINE pLastSetLine = NULL;
	PTEXTLINE pLastLine = GetAHistoryLine( NULL, phbr, 0, FALSE );
	//PHISTORY phbStart;
	if( *CurrentLineInfo )
		(*CurrentLineInfo)->Cnt = 0;
	start = 0;
	EnterCriticalSec( &phbr->cs );
	//lprintf( WIDE("------------- Building Display Info Lines %d"), nLineCount );
	//lprintf( WIDE("nShown starts at %d"), nShown );
	if( phbr->nColumns )
		while( ( nLinesShown < nLineCount ) &&
             ( pText = EnumHistoryLine( phbr
												  , &start
												  , NULL ) ) )
		{
			DISPLAYED_LINE dl;
			//lprintf( WIDE(" Lines %d of %d"), nLinesShown, nLineCount );
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

				nLines = CountLinesSpanned( phbr->nColumns, pText );
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
					//lprintf( WIDE("Fixup of first line by offset... %d %d")
               //        , phbr->nOffset, nLines );
					nLinesShown = phbr->nOffset - (nLines-1);
					firstline = 0;
				}
				//lprintf( WIDE("Wraping text, current line is at most %d"), nLines );
				nChar = 0;
				pLastSetLine = NULL;
				while( pText )
				{
#ifdef __DEKWARE_PLUGIN__
					if( phbr->flags.bNoPrompt
						&& ( pText->flags & TF_PROMPT )
					  )
					{
					  // lprintf( WIDE("skipping prompt segment...") );
						pText = NEXTLINE( pText );
						continue;
					}
#endif
					if( !pLastSetLine )
					{
						dl.nLine = start; // just has to be different
						dl.nOfs = 0; // start of a new line here...
						dl.start = pText; // text in history that started this...
						//lprintf( WIDE("Adding line to display: %p (%d) %d"), dl.start, dl.nOfs, dl.nLine );
						if( nLinesShown + nLines > 0 )
						{
							//lprintf( WIDE("Set line %d %s"), nLinesShown + nLines, GetText( dl.start ) );
							pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																					 , nLinesShown + nLines -1
																					 , &dl );
						}
					}

					nLen = GetTextSize( pText );

					if( nLen & 0x80000000)
						lprintf( "Inverted Segment : %d %p %p", nLen, NEXTLINE( pText ), PRIORLINE( pText ) );
					else
					{
						// shown - show part of this segment... but not the whole segment...
						while( nShown < nLen )
						{
							// nShow is now the number of characters we can show.
							//lprintf( WIDE("Segment is %d"), GetTextSize( pText ) );
							if( ( nShow = ComputeToShow( phbr->nColumns, pText, nLen, nChar, nShown ) ) )
							{
								//lprintf( WIDE("Will show %d chars..."), nShow );
								nShown += nShow;
								nChar += nShow;
							}
							else
							{
								//lprintf( WIDE("Wrapped line... reset nChar cause it's a new line of characters.") );
								nWrapped++;
								if( pLastSetLine )
								{
									//lprintf( WIDE("Setting prior toshow to nChar...%d"), nChar );
									pLastSetLine->nToShow = nChar;
								}
								else
								{
									//lprintf( WIDE("Setting this toshow to nChar...%d"), nChar );
									dl.nToShow = nChar;
								}

								nChar = 0;

								// skip next leading spaces.
								// (and can be more than the length of the first segment)
								//nShown = ComputeNextOffset( pText, nShown );
								{
									dl.nLine = start; // just has to be different
									dl.nOfs = nShown; // start of a new line here...
									dl.start = pText; // text in history that started this...
									//lprintf( WIDE("Adding line to display: %p (%d) %d"), dl.start, dl.nOfs, dl.nLine );
									if( ( nLinesShown + nLines - nWrapped ) > 0 )
									{
										//lprintf( WIDE("Set line %d"), nLinesShown + nLines - nWrapped );
										pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																		  , nLinesShown + nLines - nWrapped -1
																		  , &dl );
									}
								}
							}
						}
					}
					pText = NEXTLINE( pText );
					nShown = 0;
				}
				if( pLastSetLine )
				{
					//lprintf( WIDE("Fixing up last line set for number of chars to show. %d"), nChar );
					pLastSetLine->nToShow = nChar;
					nLinesShown += nLines;
				}
			}
			else
			{
				// this uses the cursorX to determine the left character to show...
				// cursor Y is the bottom line to show.
				dl.nOfs = 0;
				dl.start = NULL;
				dl.nLine = 0;
				dl.nToShow = 0;

				nLines = 1;

				nChar = 0;
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
						if( (dl.nOfs + nLen) < phbr->nOffset )
						{
							lprintf( WIDE("Skipping segement, it's before the offset...") );
							dl.nOfs += nLen;
						}
						else
						{
							dl.nLine = start; // just has to be different
							//dl.start = pText; // text in history that started this...
							lprintf( WIDE("Adding line to display: %p (%d) %d %d")
									 , dl.start, dl.nOfs, dl.nLine, nLinesShown+nLines );
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
					lprintf( WIDE("Adding line to display: %p (%d) %d"), dl.start, dl.nOfs, dl.nLine );
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
	//lprintf( WIDE("Clearning lines %d to %d"), nLinesShown, nLineCount );
	if( nLinesShown < nLineCount )
	{
		DISPLAYED_LINE dl;
		dl.nLine = 0; // minus 1 since it was auto incremented already
		dl.nOfs = 0; // start of a new line here...
		dl.start = NULL; // text in history that started this...
		dl.nToShow = 0;
		while( nLinesShown < nLineCount )
		{
         // going to begin a new line...
         // setup the line info for the new line...
			//lprintf( WIDE("Adding line to display: %p (%d) %d"), dl.start, dl.nOfs, dl.nLine );
         //lprintf( WIDE("Set line %d"), nLinesShown  );
         SetDataItem( CurrentLineInfo
                    , nLinesShown
                    , &dl );
         nLinesShown++;
      }
   }
   LeaveCriticalSec( &phbr->cs );
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
int DumpHistory( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PCONSOLE_INFO pmdp;
   FILE *file;
   PTEXT name;
   pmdp = (PCONSOLE_INFO)pdp;
    EnterCriticalSec( &pmdp->Lock );
    name = GetFileName( ps, &parameters );
    if( name )
    {
        file = sack_fopen( 0, GetText( name ), WIDE("wt") );      
        if( file )
        {
           // the whole region...
           WriteHistoryToFile( file, pmdp->pHistory );
           fclose( file );
        }
    }
    LeaveCriticalSec( &pmdp->Lock );
    return 0;
}
#endif
//----------------------------------------------------------------------------

void SetCursorNoPrompt( PHISTORY_BROWSER phbr, LOGICAL bNoPrompt )
{
   phbr->flags.bNoPrompt = bNoPrompt;
}

//----------------------------------------------------------------------------

void SetHistoryDefaultForeground( PHISTORY_LINE_CURSOR phlc, int iColor )
{
	phlc->output.DefaultColor.flags.foreground = iColor;
   phlc->output.PriorColor = phlc->output.DefaultColor;
}

//----------------------------------------------------------------------------

void SetHistoryDefaultBackground( PHISTORY_LINE_CURSOR phlc, int iColor )
{
	phlc->output.DefaultColor.flags.background = iColor;
   phlc->output.PriorColor = phlc->output.DefaultColor;
}


//----------------------------------------------------------------------------

void GetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, PS_32 x, PS_32 y )
{
	if( x ) *x = phlc->output.nCursorX;
   if( y ) *y = phlc->output.nCursorY;
}
//----------------------------------------------------------------------------
int GetCursorLine( PHISTORY_LINE_CURSOR phlc )
{
	S_32 y;
	GetHistoryCursorPos( phlc, NULL, &y );
   return y;
}
//----------------------------------------------------------------------------
int GetCursorColumn( PHISTORY_LINE_CURSOR phlc )
{
	S_32 x;
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

void SetBrowserColumns( PHISTORY_BROWSER phbr, int n )
{
	phbr->nColumns = n;
   // recompute visible lines...
}
//----------------------------------------------------------------------------
void SetCursorColumns( PHISTORY_LINE_CURSOR phlc, int n )
{
   phlc->nColumns = n;
   // recompute visible lines...
}
//----------------------------------------------------------------------------

void SetHistoryCursorPos( PHISTORY_LINE_CURSOR phlc, S_32 x, S_32 y )
{
	phlc->output.nCursorX = x;
	phlc->output.nCursorY = y;
}

//----------------------------------------------------------------------------

void SetHistoryPageLines( PHISTORY_BROWSER phbr, _32 nLines )
{
   //lprintf( WIDE("Set histpry lines at %d"), nLines );
   phbr->nPageLines = nLines;
}

//----------------------------------------------------------------------------

void SetHistoryBrowserNoPageBreak( PHISTORY_BROWSER phbr )
{
   phbr->flags.bNoPageBreak = 1;
}

//----------------------------------------------------------------------------

void SetHistoryBrowserOwnPageBreak( PHISTORY_BROWSER phbr )
{
   phbr->flags.bOwnPageBreak = 1;
}

//----------------------------------------------------------------------------
// $Log: history.c,v $
// Revision 1.66  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.65  2005/04/15 07:28:38  d3x0r
// Okay this all seems to work sufficicnetly - disabled logging entirely.
//
// Revision 1.64  2005/01/27 17:31:37  d3x0r
// psicon works now, added some locks to protect multiple accesses to history buffer (render/update_write)
//
// Revision 1.63  2005/01/17 11:21:46  d3x0r
// partial working status for psicon
//
// Revision 1.62  2004/12/15 05:59:35  d3x0r
// Updates for linux building happiness
//
// Revision 1.61  2004/12/14 23:40:08  d3x0r
// Minor tweaks to remove logging messages, also streamlined some redundant updates
//
// Revision 1.60  2004/09/29 09:31:32  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.59  2004/09/27 16:06:17  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.58  2004/09/23 09:53:33  d3x0r
// Looks like it's in a usable state for windows... perhaps an alpha/beta to be released soon.
//
// Revision 1.57  2004/09/20 10:00:09  d3x0r
// Okay line up, wrapped, partial line, line up, history alignment, all sorts of things work very well now.
//
// Revision 1.56  2004/09/10 08:48:50  d3x0r
// progress not perfection... a grand dawning on the horizon though.
//
// Revision 1.55  2004/09/10 00:23:24  d3x0r
// Well looks like most of the issues are done - minor things like history display, keybinds, the separator line...
//
// Revision 1.54  2004/09/09 13:41:02  d3x0r
// works much better passing correct structures...
//
// Revision 1.53  2004/08/14 00:06:09  d3x0r
// black out some changes for registry checks for defaults.
//
// Revision 1.52  2004/08/13 09:29:50  d3x0r
// checkpoint
//
// Revision 1.51  2004/07/30 14:08:40  d3x0r
// More tinkering...
//
// Revision 1.50  2004/07/29 01:24:53  d3x0r
// Minor tweaks...
//
// Revision 1.49  2004/07/28 08:08:19  d3x0r
// more fixes for history... slow but sure progress I suppose
//
// Revision 1.48  2004/07/26 11:05:48  d3x0r
// gotta straighten out these history structures...
//
// Revision 1.47  2004/07/26 08:40:40  d3x0r
// checkpoint...
//
// Revision 1.46  2004/06/14 09:07:13  d3x0r
// Mods to reduce function depth
//
// Revision 1.45  2004/06/13 01:23:42  d3x0r
// checkpoint.
//
// Revision 1.44  2004/06/12 23:57:14  d3x0r
// Hmm some strange issues left... just need to simply code now... a lot of redunancy is left...
//
// Revision 1.43  2004/06/12 08:42:34  d3x0r
// Well it initializes, first character causes failure... all windows targets build...
//
// Revision 1.42  2004/06/12 06:21:51  d3x0r
// One link error from compilation - now to revisit this and weed it some...
//
// Revision 1.41  2004/06/12 01:24:10  d3x0r
// History cursors are being seperated into browse types and output types...
//
// Revision 1.40  2004/06/10 22:11:00  d3x0r
// more progress...
//
// Revision 1.39  2004/06/10 10:01:44  d3x0r
// Okay so cursors have input and output characteristics... history and display are run by seperate cursors.
//
// Revision 1.38  2004/06/10 00:41:49  d3x0r
// Progress..
//
// Revision 1.37  2004/06/08 00:23:26  d3x0r
// Display and history combing proceeding...
//
// removed prior history - everything is obsolete, there's so much
// which has been re-worked...
//
