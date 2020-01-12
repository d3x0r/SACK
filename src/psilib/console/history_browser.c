
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



//----------------------------------------------------------------------------

void PSI_DestroyHistoryRegion( PHISTORY_REGION phr )
{
	if( phr )
	{
		while( phr->pHistory.next )
			PSI_DestroyRawHistoryBlock( phr->pHistory.next );
		DeleteList( &phr->pCursors );
		DeleteList( &phr->pBrowsers );
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

void PSI_DestroyHistoryBrowser( PHISTORY_BROWSER phbr )
{
	DeleteCriticalSec( &phbr->cs );
	DeleteDataList( &phbr->DisplayLineInfo );
	Release( phbr );
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
	AddLink( &region->pBrowsers, phbr );
	return phbr;
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

void DumpRegion( PHISTORY_REGION region DBG_PASS )
{
	PHISTORYBLOCK blocks = region->pHistory.root.next;
	_xlprintf( 0 DBG_RELAY )("History blocks: %d history block size: %d", region->nHistoryBlocks
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
		Log( "No start..." );
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
				*length = ptl->flags.nLineLength;
			//lprintf( "Resulting: (%d)%s", ptl->nLineLength, GetText( ptl->pLine ) );
			return ptl->pLine;
		}
		else
		{
			DECLTEXT( nothing, "" );
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

void SetHistoryLength( PHISTORY_REGION phr, int length )
{
	Log1( "Set Length to %" _size_f " blocks", length/MAX_HISTORY_LINES );
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
	int idx;
	while( pHistory )
	{
		lprintf( "Have a history block with %" _size_f " lines", pHistory->nLinesUsed );
		for( idx = 0; idx < pHistory->nLinesUsed; idx++ )
		{
			size_t length;
			PTEXTLINE pLine = pHistory->pLines + idx;
			PTEXT pText = pLine->pLine;
			while( pText )
			{
				length = GetTextSize( pText );
				lprintf( "%s", GetText(pText) );
				sack_fwrite( &length,  sizeof( length ), 1, file );
				sack_fwrite( &pText->flags,  sizeof( pText->flags ), 1, file );
				sack_fwrite( &pText->format,  sizeof( pText->format ), 1, file );
				sack_fwrite( GetText( pText ), length, 1, file );
				pText = NEXTLINE( pText );
			}
			length = 0;
			sack_fwrite( &length,  sizeof( length ), 1, file );
		}
		pHistory = pHistory->next;
	}
#else
	lprintf( "Cannot write to file, please port file access" );
#endif
}

void ReadHistoryFromFile( FILE *file, PHISTORY_REGION phr )
{
#ifndef __NO_WIN32API__
	size_t length;
	PTEXT readline = NULL;
	PTEXT rebuild;
	while( sack_fread( &length,  sizeof( length ), 1, file ) )
	{
		if( length )
		{
			rebuild = SegCreate( length );
			sack_fread( &rebuild->flags, sizeof( rebuild->flags ), 1, file );
			sack_fread( &rebuild->format, sizeof( rebuild->format ), 1, file );
			sack_fread( GetText( rebuild ), length, 1, file );
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
	lprintf( "Cannot write to file, please port file access" );
#endif
}


void SetHistoryBackingFile( PHISTORY_REGION phr, FILE *file )
{
	ReadHistoryFromFile( file, phr );
	phr->file_backing = file;
}

//----------------------------------------------------------------------------

int SkipSomeLines( PHISTORY_BROWSER phbr, SFTFont font, PTEXT countseg, int lines )
{
	uint32_t colsize = phbr->nWidth;
	// always spans at least one line.
	int nLines = 1;
	if( countseg && colsize )
	{
		int32_t nShown = 0;
		int32_t nChar = 0;
		uint32_t col_offset = 0;
		while( countseg )
		{
			int32_t nLen = (int)GetTextSize( countseg );
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
				phbr->measureString( phbr->psvMeasure, GetText( countseg ), nLen, &text_size, &text_lines, font );

				if( ( col_offset + text_size ) > colsize )
				{
					// this is the wrapping condition...
					while( nShown < nLen )
					{
						nShown += ComputeToShow( colsize, &col_offset, countseg, nLen, nChar, nShown, phbr, font );
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
int CountLinesSpannedEx( PHISTORY_BROWSER phbr, PTEXT countseg, SFTFont font, LOGICAL count_trailing_linefeeds, int leadinPad )
#define CountLinesSpanned(phbr,segs,font,lf)   CountLinesSpannedEx(phbr,segs,font,lf,0)
{
	// always spans at least one line.
	uint32_t colsize = phbr->nWidth;
	int nLines = 1;
	int used_size = 0;
	if( countseg && colsize )
	{		
		int32_t nChar = 0;
		uint32_t col_offset = leadinPad;  // pixel size of nShown
		int32_t nSegShown = 0;   // how many characters of this segment have been put out
		int32_t nShown = 0;   // how many characters have been put out
		while( countseg )
		{
			int32_t nLen = (int)GetTextSize( countseg );
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
				if( ( text_lines > (uint32_t)phbr->nLineHeight )
					|| ( ( col_offset + text_size ) > colsize ) )
				{
					int32_t _nShown = nShown;
					// this is the wrapping condition...
					while( nSegShown < nLen )
					{
						int skip_char;
						int32_t nShow = ComputeToShow( colsize, &col_offset, countseg, nLen, nChar, nSegShown, phbr, font );
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
				lprintf( "Previously had an offset... %d", phbr->nOffset );
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
				lprintf( "jumping back naturally." );
				phbr->pBlock = phbr->pBlock->prior;
				phbr->nLine = phbr->pBlock->nLinesUsed;
			}
			else
			{
				// fell off the beginning of history.
				// and nline is already 0....
				// phlc->nLine = 0;
				lprintf( "Fell off of history ... need to lock down at first line." );
				phbr->pBlock = phbr->region->pHistory.next;
				phbr->nLine = 1; // minimum.
				nOffset = 0;
				break;
			}
			// plus full number of lines left...
			// always at least 1, but my be more from this...
			// but it is truncated result, hence it does not include the
			// 1 for the partial line (which is what the first 1 is - partial line)
			if( phbr->pBlock->pLines[phbr->nLine-1].flags.nLineLength )
			{
				int tmp;
				lprintf( "This counted on an monospaced font...." );
				tmp = (phbr->pBlock->pLines[phbr->nLine - 1].flags.nLineLength - 1) / 12 /*phbr->nColumns*/;
				lprintf( "Offset plus uhmm... %d", tmp );
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
	if( phbr->nLine > phbr->pBlock->nLinesUsed )
	{
		phbr->nLine = 0;
		phbr->pBlock = NULL;
	}
	return result;
}
//----------------------------------------------------------------------------

int32_t GetBrowserDistance( PHISTORY_BROWSER phbr, SFTFont font )
{
	int32_t nLines = 0; // count of lines...
	PHISTORYBLOCK pHistory;
	int n;
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
#ifdef DEBUG_OUTPUT
	lprintf( "Browser is %" _size_f " lines from end...", nLines );
#endif
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
                    , int *command_pixel_start
                    , int *command_pixel_cursor
                    , int *line_offset
                    )
{
	PTEXT pCmd;
	int32_t tmpx = 0, nLead, tmp_end = 0;
	int32_t pixelWidth = 0;
	PDISPLAYED_LINE pdl;
	int32_t lines;

	if( !CommandInfo )
		return 0;
	// else there is no history...
	//lprintf( "bendofstream = %d", bEndOfStream );
	if( bEndOfStream )
	{
		(*line_offset) = 0;
		pdl = (PDISPLAYED_LINE)GetDataItem( &phbr->DisplayLineInfo, 0 );
		if( pdl ) {
			int32_t max;
			tmpx = pdl->nToShow;
			pixelWidth = pdl->nPixelEnd;
			if( !bWrapCommand )
				if( phbr->nWidth < 150 )
					max = (phbr->nWidth * 2) / 3;
				else
					max = phbr->nWidth - 150;
			else
				max = phbr->nWidth;
			if( pixelWidth > max ) {
				lprintf( "Drawing direct command prompt is confusing... and I don't like it." );
				// I have to put the command here, so I have to adjust the visible line...
				// or go down to the next line, and if I'm on the next line, I have to result that also...
				// but then I have to know this before I even start drawing so the last line is not
				// the last line of history, but is a command prompt...
				// also need to count the prompt size (macro:line)...
			}
			nLead = tmpx;
		}
		else {
			nLead = 0;
			pixelWidth = 0;
		}
	}
	else {
		nLead = 0;
		pixelWidth = 0;
	}

	if( command_offset )
		(*command_offset) = nLead;

	if( command_pixel_start )
		(*command_pixel_start) = pixelWidth;

	pCmd = CommandInfo->CollectionBuffer;
	SetStart( pCmd );

	if( bWrapCommand )
	{
		lines = CountLinesSpannedEx( phbr, pCmd, font, TRUE, pixelWidth );
		if( bEndOfStream )
			lines--;
	}
	else
	{
		if( bEndOfStream )
			lines = 0;  // number of lines we have to push display up...
		else
			lines = 1;
	}

	tmp_end = 0;
	while( pCmd != CommandInfo->CollectionBuffer )
	{
		uint32_t width, height;
		phbr->measureString( phbr->psvMeasure, GetText( pCmd ), (int)GetTextSize( pCmd ), &width, &height, font );
		pixelWidth += width;
		tmp_end += (int)GetTextSize( pCmd );
		pCmd = NEXTLINE( pCmd );
	}
	//tmp_end = tmpx;

	while( pCmd )
	{
		uint32_t width, height;
		if( SUS_LT(tmp_end, int32_t, CommandInfo->CollectionIndex, INDEX ) ) {
			phbr->measureString( phbr->psvMeasure, GetText( pCmd )
				, (int)CommandInfo->CollectionIndex - tmp_end /* GetTextSize( pCmd )*/, &width, &height, font );
			pixelWidth += width;
		}
		tmp_end += (int)GetTextSize( pCmd );
		pCmd = NEXTLINE( pCmd );
	}
	tmpx += (int)CommandInfo->CollectionIndex;

	if( command_begin )
		(*command_begin) = 0;

	if( command_end )
	{
		(*command_end) = tmp_end;
	}
	/*
	if( tmpx > phbr->nColumns - 10 )
	{
		if( command_begin )
			(*command_begin) = ( tmpx - ( phbr->nColumns - 10 ) ) - nLead;
		tmpx = phbr->nColumns - 10;
	}
	*/
	if( command_pixel_cursor )
		( *command_pixel_cursor) = pixelWidth;
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
	for( n = 0; ; n++ )
	{
		PDISPLAYED_LINE pdl;
		if( pdl = (PDISPLAYED_LINE)GetDataItem( &phbr->DisplayLineInfo, n ) ) {
			if( pdl->start )
				used++;
		}
		else break;
	}
	return used;
}

void BuildDisplayInfoLines( PHISTORY_BROWSER phbr, PHISTORY_BROWSER leadin, SFTFont font )
//void BuildDisplayInfoLines( PHISTORY_LINE_CURSOR phlc )
{
	if( phbr->flags.bUpdated )
	{
		int nLines, nLinesShown = 0;
		int nChar;  // this with col_offset is character count on line
		uint32_t nLen;
		uint32_t col_offset; // pixel position to match nChar 
		uint32_t nSegShown = 0; // characters on segment shown
		//int nLineCount = phbr->nLines;
		int nLineTop = phbr->nFirstLine;
		PTEXT pText;
		PDATALIST *CurrentLineInfo = &phbr->DisplayLineInfo;

		int start;
		int firstline = 1;
		uint32_t nShown = 0; // total length of all segs shown on a line... 
		PDISPLAYED_LINE pLastSetLine = NULL;
		PTEXTLINE pLastLine = GetAHistoryLine( NULL, phbr, 0, FALSE );
		PDISPLAYED_LINE pdlLeadin = (PDISPLAYED_LINE)(leadin?GetDataItem( &leadin->DisplayLineInfo, 0 ):NULL);
		//PHISTORY phbStart;
		if( *CurrentLineInfo )
			(*CurrentLineInfo)->Cnt = 0;
		start = 0;
		EnterCriticalSec( &phbr->cs );
#ifdef DEBUG_OUTPUT
			lprintf( "nShown starts at %d %d", nShown, pdlLeadin?pdlLeadin->nToShow:0 );
#endif
			while( ( nLineTop > 0 ) &&
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
					lprintf( "top of form reached, bailing early on available history." );
					break;
				}
				if( phbr->flags.bWrapText )
				{
					int nShow;
					int nWrapped = 0;

					nLines = CountLinesSpannedEx( phbr, pText, font, FALSE, pdlLeadin?pdlLeadin->nPixelEnd:0 );
#ifdef DEBUG_OUTPUT
					lprintf( "------------- Building Display Info Lines %d  offset %d", nLines, pdlLeadin ? pdlLeadin->nPixelEnd : 0 );
#endif
					if( pdlLeadin )
						col_offset = pdlLeadin->nPixelEnd;  // if there's a leadin (command prompt) then use that to start.
					else
						col_offset = 0; // new histroy line, itself should wrap.
					pdlLeadin = NULL;
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
							dl.nPixelStart = col_offset;
							// nPixelEnd has to be set later with pLastSetLine
							dl.nLineHeight = phbr->nLineHeight;
							dl.nLineTop = nLineTop - (nLines) * phbr->nLineHeight;
							nLineTop = dl.nLineTop;
							//lprintf( "Adding line to display: %p (%d) %d", dl.start, dl.nOfs, dl.nLine );
							if( nLinesShown + nLines > 0 )
							{
#ifdef DEBUG_OUTPUT
								lprintf( "Set line %d %s", nLinesShown + nLines -1, GetText( dl.start ) );
#endif
								pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																		   , nLinesShown + nLines -1
																		   , &dl );
							}
						}

						nLen = (int)GetTextSize( pText );
						{
							int trim_char;
							// shown - show part of this segment... but not the whole segment...
							if( !nLen )
							{
								// zero size segments can end up in the display because they contain 
								// position or other formatting information.
								// generally systems will filter out the newlines and convert it to 
								// a separatly queued line.
								if( 1 ) /* !pText->format.flags.prior_background ||
									!pText->format.flags.prior_background ) */ {

									pText = NEXTLINE( pText );
									nSegShown = 0;
									continue;
								}

								trim_char = 0;
								pText = NEXTLINE( pText );
								if( !pText ) break;
								nLen = (int)GetTextSize( pText );
								goto do_end_of_line;
							}
							else while( nSegShown < nLen )
							{
								trim_char = 0;
								// nShow is now the number of characters we can show.
#ifdef DEBUG_OUTPUT
								lprintf( "Segment is %d", GetTextSize( pText ) );
#endif

								nShow = ComputeToShow( phbr->nWidth, &col_offset, pText, nLen, nChar, nSegShown, phbr, font );
								// in case we wrap 0 characters for columns less than a width of a character...
								if( nShow > 0 ) {
									nSegShown += nShow;
									nShown += nShow;
									nChar += nShow;
									if( GetText( pText )[nSegShown - 1] == '\n' )
										trim_char = 1;
									else
										trim_char = 0;
									// wrapped on a space - word break in segment
									// or had a newline at the end which causes a wrap...
									// log the line and get a new one.
									pLastSetLine->nPixelEnd = col_offset;
									pLastSetLine->nToShow = nChar - trim_char;

									if( ( nSegShown < nLen ) || ( trim_char ) )
									{
										//lprintf( "Wrapped line... reset nChar cause it's a new line of characters." );
										nWrapped++;
									do_end_of_line:
										if( pLastSetLine ) {
											//lprintf( "Setting prior toshow to nChar...%d", nChar );
											if( nSegShown < nLen )
												dl.nPixelStart = col_offset = 0;
											else
												dl.nPixelStart = col_offset;
											dl.nLineEnd = pLastSetLine->nLineEnd = pLastSetLine->nLineStart + (nChar)-1;
										}
										else
											dl.nLineEnd = (nChar)-1;
										dl.nLineStart = dl.nLineEnd + 1;
									}
								}

								// begin a new line output
								if( (nSegShown < nLen) || (trim_char) ) {
									// continue with current char, but new start offset
									dl.nPixelStart = 0;
									col_offset = 0;
									dl.nLineTop += phbr->nLineHeight;
									{
										dl.nLine = nShown + start; // just has to be different
										dl.nFirstSegOfs = nSegShown; // start of a new line here...
										dl.start = pText; // text in history that started this...
										//lprintf( "Adding line to display: %p (%d) %d", dl.start, dl.nOfs, dl.nLine );
										if( (nLines - nWrapped) > 0 ) {
#ifdef DEBUG_OUTPUT
											lprintf( "Set line %d", nLinesShown + ((nLines - 1) - nWrapped) );
#endif
											pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
												, nLinesShown + (nLines-1) - nWrapped
												, &dl );
										}
									}
								}
								// skip next leading spaces.
								// (and can be more than the length of the first segment)
							}
						}
						pText = NEXTLINE( pText );
						nSegShown = 0;
					}
					nChar = 0;
					nLinesShown+=nLines;

					//lprintf( "Fixing up last line set for number of chars to show. %d", nChar );
					pLastSetLine->nToShow += nChar;
					pLastSetLine->nLineEnd = pLastSetLine->nLineStart + ( nChar - 1 );
					pLastSetLine->nPixelEnd = col_offset;
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
					lprintf( "Clearing last set line..." );
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
							nLen = (int)GetTextSize( pText );
							if( USS_LT( (dl.nFirstSegOfs + nLen), uint32_t, phbr->nOffset, int ) )
							{
								lprintf( "Skipping segement, it's before the offset..." );
								dl.nLineEnd += nLen;
								dl.nFirstSegOfs += nLen;
							}
							else
							{
								dl.nLine = start; // just has to be different
								//dl.start = pText; // text in history that started this...
								lprintf( "Adding line to display: %p (%" _size_f ") %" _size_f " %d"
										 , dl.start, dl.nFirstSegOfs, dl.nLine, nLinesShown+nLines );
								if( nLinesShown + nLines > 0 )
								{
									lprintf( "Set line %d", nLinesShown + nLines );
									pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																							 , nLinesShown + nLines -1
																							 , &dl );
								}
								lprintf( "After set last set line is %p", pLastSetLine );
							}
						}
						pText = NEXTLINE( pText );
					}
					if( !pLastSetLine )
					{
						// dl will be initialized as a blank line...
						lprintf( "Adding line to display: %p (%" _size_f ") %" _size_f, dl.start, dl.nFirstSegOfs, dl.nLine );
						if( nLinesShown + nLines > 0 )
						{
							lprintf( "Set line %d", nLinesShown + nLines -1 );
							pLastSetLine = (PDISPLAYED_LINE)SetDataItem( CurrentLineInfo
																					 , nLinesShown + nLines -1
																					 , &dl );
						}
					}
				}
			}
		if( !nLinesShown && nLineTop > 0 ) {
			uint32_t h = phbr->nLineHeight;
			DISPLAYED_LINE dl;

			dl.nFirstSegOfs = 0;
			dl.start = NULL;
			dl.nLine = 0;
			dl.nToShow = 0;
			dl.nPixelStart = 0;
			dl.nPixelEnd = 0;
			dl.nLineStart = dl.nLineEnd = 0;
			dl.nLineHeight = h;
			dl.nLineTop = nLineTop - h;
			phbr->nLineHeight = dl.nLineHeight = h;
#ifdef DEBUG_OUTPUT
			lprintf( "Set first empty dataline." );
#endif
			SetDataItem( CurrentLineInfo, 0 , &dl );
		}
		//clear_remaining_lines:
		//lprintf( "Clearning lines %d to %d", nLinesShown, nLineCount );
		phbr->flags.bUpdated = 0;
		LeaveCriticalSec( &phbr->cs );
	}
}
//----------------------------------------------------------------------------

void SetCursorNoPrompt( PHISTORY_BROWSER phbr, LOGICAL bNoPrompt )
{
	phbr->flags.bNoPrompt = bNoPrompt;
}

//----------------------------------------------------------------------------

void SetBrowserHeight( PHISTORY_BROWSER phbr, int n )
{
	phbr->nHeight = n;
	// recompute visible lines...
}
//----------------------------------------------------------------------------

void SetBrowserWidth( PHISTORY_BROWSER phbr, int size )
{
	phbr->nWidth = size;
	// recompute visible lines...
}

//----------------------------------------------------------------------------

void SetBrowserFirstLine( PHISTORY_BROWSER cursor, int nLine ) {
	cursor->nFirstLine = nLine;
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


PSI_CONSOLE_NAMESPACE_END
