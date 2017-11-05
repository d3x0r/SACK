/*
 *  Crafted by James Buckeyne
 *
 *	(c) Freedom Collective 2000-2006++
 *
 *	Additional Text routines to provide more user input parsing
 *	also common editable buffers using typed text input.
 *	Handles things like backspace, saving of commands entered
 *	historically.  Could probably be remotely populated and resemble
 *	last typed search entries.  Based on PTEXT container.
 *
 * see also - include/typelib.h
 *
 */
#include <stdhdrs.h>
#include <sharemem.h>

#define PARSE_DEEP 0 // make true to recurse parse expressions

#define Collapse(towhere) SegConcat(towhere,begin,beginoffset,total)

#define IsBracket(Close)	{													\
					if (!quote&&!bracket)												\
					{																		  \
						/* flush all up till this character. */					\
						if( !escape )													  \
						{																	  \
							bracket=Close;												 \
							bracketlevel=1;												\
							if (total)													  \
								outdata=Collapse(outdata);							 \
							begin=input;													\
							beginoffset=index;											\
							total=1;														 \
						}																	  \
						else																  \
						{																	  \
							total++;														 \
							escape = FALSE;												\
						}																	  \
					}																		  \
					else																	  \
					{																		  \
						if (bracket==Close)											  \
							bracketlevel++;												\
						total++;															 \
					} }


//----------------------------------------------------------------------

#ifdef __cplusplus
namespace sack {
	namespace containers {
	namespace text {
	using namespace sack::memory;
	using namespace sack::logging;
	using namespace sack::containers::queue;
#endif
//----------------------------------------------------------------------
#define BUILD_LINE_OUTPUT_SIZE 256

static PTEXT SplitLine( PTEXT pLine, size_t nPos )
{
	PTEXT newseg, end;
	if( !nPos )
	{
			if( PRIORLINE( pLine ) )
				return PRIORLINE( pLine ); 
			// otherwise we'll have to insert a new segment 
			// in front of this....
		newseg = SegCreate( BUILD_LINE_OUTPUT_SIZE );
		newseg->data.size = 0;
		pLine->format.position.offset.spaces = 0;
		SegAppend( newseg, pLine );
		return newseg;
	}
	else if( pLine->data.size == nPos )
	{
		// if the point to split is not in the middle of
			// this segment....
			return pLine;
	}

	newseg = SegCreate( BUILD_LINE_OUTPUT_SIZE );

	end = NEXTLINE( pLine );
	SegBreak( end );

	// fill in new segment with trailing data
	newseg->data.size = pLine->data.size - nPos;
	MemCpy( newseg->data.data, pLine->data.data + nPos, newseg->data.size );

	pLine->data.size = nPos; 
	pLine->data.data[nPos] = 0; // terminate with null also...
	SegAppend( pLine
				, SegAppend( newseg, end ) 
				);

	return pLine;
}

//----------------------------------------------------------------------

PTEXT GetUserInputLine( PUSER_INPUT_BUFFER pci )
{
	PTEXT pReturn = NULL;
	if( pci->CollectionIndex 
		|| ( pci->CollectionBuffer && GetTextSize( pci->CollectionBuffer ) ) )
	{
		PTEXT tmp;
		pReturn = pci->CollectionBuffer;
		GetText(pReturn)[GetTextSize(pReturn)] = 0;
		SetStart( pReturn );
		tmp = BuildLine( pReturn );
		LineRelease( pReturn );
		pReturn = tmp;
		// begin next collection in case more data is in the input...
		pci->CollectionBuffer = SegCreate( BUILD_LINE_OUTPUT_SIZE );
		SetTextSize( pci->CollectionBuffer, 0 );
		pci->CollectionIndex = 0;
	}
	return pReturn;
}

static PTEXT GatherLineEx( PTEXT *pOutput, INDEX *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput )
#define GatherLine( out,idx,ins,cr,in) GatherLineEx( (out),(idx),(ins),(cr),(FALSE),(in))
// if data - assume data is coming from a preformatted source
// otherwise use escape as command entry and clear buffer...
{
	// this routine should be used to process user type character
	// input into a legible line buffer....
	// results in a complete line....
	// the line returned must be used - the output buffer
	// is an accumulator and will contain any partial input buffer
	// which remaineder if an EOL sequence was found....

	// build line in buffer using standard console
	// behavior... 

	INDEX pos;
	INDEX size;
	INDEX maxlen = 0;
	size_t len = 0;
	PTEXT pReturn = NULL;
	PTEXT pDelete = NULL;
	TEXTCHAR character;
	TEXTCHAR *output;

	if( !pOutput ) // must supply a holder for partial collection...
		return NULL;

	if( !pInput ) // nothing new input - just using previous collection...
	{
		if( *pOutput )
		{
			// use prior partial as new input....
			pInput = *pOutput;
			pDelete = pInput; // this is never deleted if we use prior...
			SetStart( pInput );
			*pOutput = NULL;
		}
		else
			return NULL;
	}
	// probably first pass of gathering...
	if( !*pOutput )
	{
		*pOutput = SegCreate( BUILD_LINE_OUTPUT_SIZE );
		if( pIndex )
			*pIndex = 0;
		SetTextSize( *pOutput, 0 );
		output = GetText( *pOutput );
		len = 0;
	}
	else if( pIndex )
	{
		output = GetText( *pOutput );
		len = *pIndex;
		if( (*pOutput)->data.size != len )
		{
			if( bInsert )
			{
				// creates a new segment inbetween here.....
				*pOutput = SplitLine( *pOutput, len );
				 output = GetText( *pOutput );
			}
			else
			{
				maxlen = (INDEX)(*pOutput)->data.size;
			}
		}
	}
	else
	{
		output = GetText( *pOutput );
		len = GetTextSize( *pOutput );
	}
	if( NEXTLINE( *pOutput ) && GetTextSize( NEXTLINE( *pOutput ) ) == 0 )
	{
		PTEXT delseg = SegGrab( NEXTLINE( *pOutput ) );
		LineRelease( delseg );
	}
	if( PRIORLINE( *pOutput ) && GetTextSize( PRIORLINE( *pOutput ) ) == 0 )
	{
		PTEXT delseg = SegGrab( PRIORLINE( *pOutput ) );
		LineRelease( delseg );
	}
	while( pInput )
	{
		size = GetTextSize( pInput );
		for( pos = 0; pos < size; pos++ )
		{
			switch( character = GetText( pInput )[pos] )
			{
			case '\x1b': // escape; clear all text
				if( !bData )
				{
					SetEnd( *pOutput );  
					SetStart( *pOutput )  // no semicolon; include the following in the loop.
					{
						SetTextSize( *pOutput, 0 );
					}
					SetTextSize( *pOutput, 0 ); // and set the first segment's length.
					output = GetText( *pOutput );
					len = 0;
				}
				else
					goto defaultcase;
				break;
			case '\x7f': // handle unix type delete too ? perhaps...
				if( !bInsert )
				{
					PTEXT pNext;
					// this will slide from the middle to the end...
					// if bInsert - then prior to entrying this switch
					// the data was split and THIS segment is set to zero.
					pNext = *pOutput;
					if( len != (maxlen = GetTextSize( *pOutput )) )
					{
						MemCpy( output + len, output+len+1, maxlen - len );
						SetTextSize( *pOutput, --maxlen );
					}
					else
					{
						PTEXT pDel;
						pNext = *pOutput;
						do {
							pDel = pNext;
							pNext = NEXTLINE( pNext );
							if( pDel != *pOutput )
						  	{
								SegGrab( pDel );
								LineRelease( pDel );
							}
						} while( pNext && !GetTextSize( pNext ) );
						if( pNext )
						{
							INDEX len2;
							output = GetText( pNext );
							len2 = GetTextSize( pNext ) - 1;
							*pOutput = pNext;
							len = 0;
							MemCpy( output, output+1, len2 );
							SetTextSize( pNext, len2 );
						}
					}
				}
				else // was insert is either at end....
				{
					// I dunno perform sliding delete operation...
					// must refresh the output string....
					{
						PTEXT pNext, pDel;
						pNext = *pOutput;
						do {
							pDel = pNext;
							pNext = NEXTLINE( pNext );
							if( pDel != *pOutput )
						  	{
								SegGrab( pDel );
								LineRelease( pDel );
							}
						} while( pNext && !GetTextSize( pNext ) );
						if( pNext )
						{
							INDEX len2;
							TEXTSTR data;
							data = GetText( pNext );
							MemCpy( data, data+1, len2 = (GetTextSize( pNext ) - 1 ));
							SetTextSize( pNext, len2 );
						}
					}
				}
				break;
			case '\b':
				/* perhaps consider using split for backspace in a line...*/
				if( !bInsert )
				{
					PTEXT pNext;
				INDEX maxlen;
					// this will slide from the middle to the end...
				// if bInsert - then prior to entrying this switch
				// the data was split and THIS segment is set to zero.
				pNext = *pOutput;
				maxlen = GetTextSize( *pOutput );
				while( !maxlen && PRIORLINE( *pOutput ) )
				{
					*pOutput = PRIORLINE( *pOutput );
					len = maxlen = GetTextSize( *pOutput );
				}
				if( maxlen )
				{
					if( len != maxlen )
					{
						INDEX sz;
						sz = maxlen - len;
								MemCpy( output + len - 1, output + len, sz );
						SetTextSize( *pOutput, maxlen - 1 );
						len--;
					}
					else
					{
						SetTextSize( *pOutput, --len );
					}
				}
				}
				else // was insert is either at end....
				{
					if( len )
					{
						GetPriorUtfCharIndexed( pOutput[0]->data.data, &len );
						if( len )
							SetTextSize( *pOutput, len );
						else
						{
							if( PRIORLINE( *pOutput ) )
							{
								*pOutput = PRIORLINE( *pOutput );
								LineRelease( SegGrab( NEXTLINE( *pOutput ) ) );
								len = GetTextSize( *pOutput );
							}
							else
							{
								*pOutput = NEXTLINE( *pOutput );
								LineRelease( SegGrab( PRIORLINE( *pOutput ) ) );
								len = 0;
							}
						}
					}
					else
					{
						if( PRIORLINE( *pOutput ) )
						{
							*pOutput = PRIORLINE( *pOutput );
							len = GetTextSize( *pOutput );
						}
						else
						{
							*pOutput = NEXTLINE( *pOutput );
							LineRelease( SegGrab( PRIORLINE( *pOutput ) ) );
							len = 0;
						}

						if( len )
						{
							GetPriorUtfCharIndexed( pOutput[0]->data.data, &len );
							SetTextSize( *pOutput, len );
							//SetTextSize( *pOutput, --len );
						}
					}
				}
				break;
			case '\r': // ignore this character...
				if( !bSaveCR )
					break;
				// falls through .. past this and saves the return...
				if(0) {
			case '\n':

				if( !pReturn )
				{
				pReturn = GetGatheredLine( pOutput );
					// transfer *pOutput to pReturn....
					output = GetText( pReturn );
					len = GetTextSize( pReturn );
					output[len] = character;
					SetTextSize( pReturn, ++len );
					// begin next collection in case more data is in the input...
					output = GetText( *pOutput );
					len = 0;
					break;
				}
				}
				// store carriage return... 
			default:
	 defaultcase:
				output[len++] = character;

				if( (maxlen && len == maxlen ) ||
					 len == BUILD_LINE_OUTPUT_SIZE )
				{
					PTEXT pTemp;
					SetTextSize( *pOutput, len );
					if( !NEXTLINE( *pOutput ) )
					{
						SegAppend( *pOutput, pTemp = SegCreate( BUILD_LINE_OUTPUT_SIZE ) );
						SetTextSize( pTemp, 0 );
					}
					else
					{
						pTemp = NEXTLINE( *pOutput );
						maxlen = GetTextSize( pTemp );
					}
					*pOutput = pTemp;
					output = GetText( *pOutput );
					len = 0;
				}
				else
				{
					if( bInsert ) // insertion happens at end of segment
									  // and the segment is broken...
					{
						if( !pIndex || ( len > *pIndex ) )
							SetTextSize( *pOutput, len );
					}
					else
						if( len > GetTextSize( *pOutput ) )
						{
							SetTextSize( *pOutput, len );
						}
				}

				break;
			}
		}
		if( pIndex )
		  *pIndex = len;
		pInput = NEXTLINE( pInput );
	}
	if( pDelete )
		LineRelease( pDelete );

	SetStart( pReturn );

	if( NEXTLINE( *pOutput ) && GetTextSize( NEXTLINE( *pOutput ) ) == 0 )
	{
		PTEXT delseg = SegGrab( NEXTLINE( *pOutput ) );
		LineRelease( delseg );
	}
	if( PRIORLINE( *pOutput ) && GetTextSize( PRIORLINE( *pOutput ) ) == 0 )
	{
		PTEXT delseg = SegGrab( PRIORLINE( *pOutput ) );
		LineRelease( delseg );
	}
	if( pReturn )
	{
		if( !pReturn->data.size )
		{
			PTEXT p;
			if( !(p = NEXTLINE( pReturn )) &&
				 !(p = PRIORLINE( pReturn )) )
			{
				// can't happen that both are NULL - but JUST in case...
				LineRelease( pReturn );
				return NULL;
			}	
			SegGrab( pReturn );
			pReturn = p;
		}
	};
	return pReturn;
}

//----------------------------------------------------------------------------

void RecallUserInput( PUSER_INPUT_BUFFER pci, int bUp )
{
	PTEXT temp;

	if( !bUp && pci->nHistory == -1 ) // scroll down, no recall done...
		return;

	if( bUp && pci->bRecallBegin )
			pci->nHistory = -1; // make sure we start at end of list recalling...

	pci->bRecallBegin = FALSE;

	if( !bUp )
	{
		pci->nHistory++;
		if( pci->nHistory >= pci->InputHistory->Cnt )
				pci->nHistory -= pci->InputHistory->Cnt;
		if( pci->nHistory == pci->InputHistory->Top )
		{
			// now we are at the NEW entry to the list...
			// nothing yet - can leave buffer at NULL, return
			// will result in OUTPUT, ClearOuput, Show NULL...
			LineRelease( pci->CollectionBuffer );
				pci->CollectionBuffer = NULL;
			pci->CollectionIndex = 0;
			pci->nHistory = INVALID_INDEX;
			return;
		}
	}
	else
	{
		// if nothing has been put in the history queue...
		if( pci->InputHistory->Top == pci->InputHistory->Bottom )
		{
			return;
		}

		// if we weren't previously working on a recalled command...
		if( pci->nHistory == -1 )
			pci->nHistory = pci->InputHistory->Top - 1;
		else
		{		 
			// if already on the first entered command (last avail recall...)
			if( pci->nHistory == (signed)pci->InputHistory->Bottom )
			{
				return;
			}
			pci->nHistory--;
		}
		// wrap to a valid position....
		if( pci->nHistory < 0 )
			pci->nHistory += pci->InputHistory->Cnt;
	}

	LineRelease( pci->CollectionBuffer );
	pci->CollectionBuffer = NULL;
	pci->CollectionIndex = 0; // adding temp to output buffer fixes this...
	temp = (PTEXT)pci->InputHistory->pNode[pci->nHistory];
	temp = GatherLine( &pci->CollectionBuffer
						  , (INDEX*)&pci->CollectionIndex
						  , pci->CollectionInsert
						  , FALSE, temp );
	if( temp && !GetTextSize( pci->CollectionBuffer ) )
	{
		LineRelease( pci->CollectionBuffer );
		// result buffer will have a newline in it...
		// we need to eat that otherwise we would have just executed
		// this command!
		SetTextSize( temp, GetTextSize(temp) - 1 );
		pci->CollectionIndex = GetTextSize( temp );
		pci->CollectionBuffer = temp;
	}
	else if( temp )
	{
		lprintf( WIDE("Losing data... there was \n in the command buffer, and data after it also!") );
	}
}

//----------------------------------------------------------------------------

void EnqueUserInputHistory( PUSER_INPUT_BUFFER pci, PTEXT pHistory )
{
	PTEXT pCommand;

	if( GetQueueLength( pci->InputHistory ) > 50 ) // keep history somewhat short...
	{
		LineRelease( (PTEXT)DequeLink( &pci->InputHistory ) );
	}

	if( ( pCommand = BuildLine( pHistory ) ) )
	{
			const PTEXT *ppLast = (PTEXT*)(PeekQueue( pci->InputHistory ));
		if( ppLast && ( SameText( pCommand, *ppLast ) == 0 ) )
		{
				LineRelease( pCommand );
		  	pCommand = NULL;
		}
		if( pCommand )
	  		EnqueLink( &pci->InputHistory, pCommand );
	}
	pci->bRecallBegin = TRUE; // new enqueue - next recall MAY begin a-new
	//LineRelease( pHistory ); // not sure why this would happen....
}

//----------------------------------------------------------------------------

void DeleteUserInput( PUSER_INPUT_BUFFER pci )
{
#if 0
	PTEXT pLine = GatherLine( &pci->CollectionBuffer
							, &pci->CollectionIndex
							, pci->CollectionInsert
							, TRUE
							, stroke );
#endif
	PTEXT pNext;
	size_t cursor = pci->CollectionIndex;
	size_t maxlen;
	size_t charlen;
	size_t len = pci->CollectionIndex;
	// this will slide from the middle to the end...
	// if bInsert - then prior to entrying this switch
	// the data was split and THIS segment is set to zero.
	pNext = pci->CollectionBuffer;
	while( pNext && cursor == ( maxlen = GetTextSize( pNext ) ) )
	{
		pNext = NEXTLINE( pNext );
		len = cursor = 0;
	}
	if( pNext )
	{
		GetUtfCharIndexed( pNext->data.data, &len, maxlen );
		charlen = len - cursor;
		if( ( maxlen - charlen ) > 0 )
		{
			MemCpy( pNext->data.data + cursor
					, pNext->data.data + len
					, ( maxlen - len ) + 1 );
			SetTextSize( pNext, ( maxlen - charlen ) );
		}
		else
		{
			PTEXT remaining;
			if( pNext == pci->CollectionBuffer )
			{
				if( remaining = NEXTLINE( pNext ) )
				{
					SegUnlink( pNext );
					LineRelease( pNext );
					pci->CollectionBuffer = remaining;
					pci->CollectionIndex = 0;
				}
				else if( remaining = PRIORLINE( pNext ) )
				{
					SegUnlink( pNext );
					LineRelease( pNext );
					pci->CollectionBuffer = remaining;
					pci->CollectionIndex = GetTextSize( remaining );
				}
				else
				{
					// empty line already
				}
			}
			else
			{
				SegUnlink( pNext );
				LineRelease( pNext );
			}

		}
	}
}

PTEXT GatherUserInput( PUSER_INPUT_BUFFER pci, PTEXT stroke )
{
	PTEXT pLine = GatherLine( &pci->CollectionBuffer
							, &pci->CollectionIndex
							, pci->CollectionInsert
							, TRUE
							, stroke );
	if( pLine )
	{
		if( pci->CollectedEvent )
		{
			pci->CollectedEvent( pci->psvCollectedEvent, pLine );
		}
		EnqueUserInputHistory( pci, pLine );
	}
	return pLine;
}


//----------------------------------------------------------------------------

 void  EmptyUserInputHistory ( PUSER_INPUT_BUFFER pci )
{
	PTEXT pHistory;
	while( ( pHistory = (PTEXT)DequeLink( &pci->InputHistory ) ) )
		LineRelease( pHistory );
	DeleteLinkQueue( &pci->InputHistory );
	LineRelease( pci->CollectionBuffer );
	pci->InputHistory = NULL;
	pci->nHistory = -1; // shouldn't matter what this gets set to...	
}

//----------------------------------------------------------------------------

 PUSER_INPUT_BUFFER  CreateUserInputBuffer ( void )
{
	PUSER_INPUT_BUFFER pci = New( USER_INPUT_BUFFER );
	pci->CollectionBufferLock = FALSE;
	pci->CollectedEvent = NULL;

	pci->CollectionBuffer = NULL;
	pci->InputHistory = CreateLinkQueue();
	pci->nHistory = -1;
	pci->bRecallBegin = FALSE;

	pci->CollectionIndex = 0;
	pci->CollectionInsert = TRUE;
	return pci;	
}

//----------------------------------------------------------------------------

 void  DestroyUserInputBuffer ( PUSER_INPUT_BUFFER *pci )
{
	if( pci )
	{
		EmptyUserInputHistory( *pci );
		Release( *pci );
		*pci = NULL;
	}
}

//----------------------------------------------------------------------------


LOGICAL  SetUserInputPosition ( PUSER_INPUT_BUFFER pci, int nPos, int whence )
{
	if( whence == SEEK_SET )
	{
		if( nPos == 0 )
		{
			if( !PRIORLINE( pci->CollectionBuffer ) && !pci->CollectionIndex )
				return FALSE;
			while( PRIORLINE( pci->CollectionBuffer ) )
				pci->CollectionBuffer = PRIORLINE( pci->CollectionBuffer );
			pci->CollectionIndex = 0;
		}
		if( nPos == -1 )
		{
			SetEnd( pci->CollectionBuffer );
			pci->CollectionIndex = GetTextSize( pci->CollectionBuffer );
		}
		else
		{
			int totalpos = 0;
			size_t tmpstart = 0;
			PTEXT start = pci->CollectionBuffer;
			PTEXT lastseg = start;
			SetStart( start );
			while( start )
			{
				nPos -= (int)( tmpstart = GetTextSize( lastseg  = start ) );
				if( nPos < 0 )
				{
					pci->CollectionBuffer = start;
					pci->CollectionIndex = nPos + tmpstart;
					break;
				}
				start = NEXTLINE( start );
			}
			if( !start )
			{
				pci->CollectionBuffer = lastseg;
				pci->CollectionIndex = GetTextSize( lastseg );
			}
		}
	}
	else if( whence == SEEK_CUR )
	{
		int totalpos = 0;
		size_t tmpstart = 0;
		PTEXT start = pci->CollectionBuffer;
		PTEXT lastseg = start;
		SetStart( start );
		while( start != pci->CollectionBuffer )
		{
			totalpos += (int)GetTextSize( start );
			start = NEXTLINE( start );
		}

		totalpos += (int)pci->CollectionIndex;
		SetStart( start );
		if( nPos > 0 )
		{
			size_t tmp;
			size_t total = 0;
			size_t start;
			size_t cursor = pci->CollectionIndex;
			PTEXT curseg = pci->CollectionBuffer;
			TEXTRUNE ch;
			start = cursor;
			if( cursor == curseg->data.size ) 
			{
				curseg = NEXTLINE( curseg );
				if( !curseg )
					return FALSE;
				start = cursor = 0;
			}
			for( tmp = 0; tmp < (size_t)nPos; tmp++ )
			{
				if( cursor < curseg->data.size )
					ch = GetUtfCharIndexed( curseg->data.data, &cursor, curseg->data.size );
				else
					ch = 0;
				if( !ch || ( cursor == curseg->data.size ) )
				{
					total += cursor - start;
					curseg = NEXTLINE( curseg );
					start = cursor = 0;
					if( !ch )  // didn't get a character, need to try this one again...
						tmp--;
				}
			}
			nPos = (int)total + (int)( cursor - start );
		}
		else
		{
			int tmp;
			int total = 0;
			int start;
			int cursor = (int)pci->CollectionIndex;
			PTEXT curseg = pci->CollectionBuffer;
			TEXTRUNE ch;
			start = cursor;
			for( tmp = 0; tmp > nPos; tmp-- )
			{
				if( cursor == 0 )
				{
					curseg = PRIORLINE( curseg );
					if( curseg )
						start = cursor = (int)curseg->data.size;
				}
				if( !curseg )
					break;
				ch = GetPriorUtfCharIndexed( curseg->data.data, (size_t*)&cursor );
				if( !cursor )
				{
					total += cursor - start;
					curseg = PRIORLINE( curseg );
					if( curseg )
						start = cursor = (int)curseg->data.size;
					else 
						break;
				}
			}
			nPos = total + ( cursor - start );

		}

		if( ( totalpos + nPos ) > 0 )
		{
			totalpos += nPos;
			while( start && SUS_GT( totalpos, int, ( tmpstart = GetTextSize( lastseg = start ) ), size_t ) )
			{
				totalpos -= (int)tmpstart;
				start = NEXTLINE( start );
			}
			if( !start )
			{
				pci->CollectionIndex = tmpstart;
				pci->CollectionBuffer = lastseg;
			}
			else
			{
				pci->CollectionIndex = totalpos;
				pci->CollectionBuffer = start;
			}
		}
		else
		{
			pci->CollectionBuffer = start;
			pci->CollectionIndex = 0;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------

 void  SetUserInputInsert ( PUSER_INPUT_BUFFER pci, int bInsert )
{
	if( bInsert < 0 )
		pci->CollectionInsert = !pci->CollectionInsert;
	else if( bInsert )
		pci->CollectionInsert = 1;
	else
		pci->CollectionInsert = 0;

}


/*
	while( LockedExchange( &pci->CollectionBufferLock, 1 ) )
		Sleep(0);

	pci->CollectionBufferLock = 0;
*/
#ifdef __cplusplus
	}; // namespace text {
}; //namespace containers {
}; // namespace sack {
#endif

//--------------------------------------------------------------------
// $Log: input.c,v $
// Revision 1.11  2004/09/23 09:53:33  d3x0r
// Looks like it's in a usable state for windows... perhaps an alpha/beta to be released soon.
//
// Revision 1.10  2004/06/12 08:43:16  d3x0r
// Overall fixes since some uninitialized values demonstrated unhandled error paths.
//
// Revision 1.9  2004/05/14 18:24:34  d3x0r
// Extend command input module to provide full command queue functionality through methods.
//
// Revision 1.8  2004/05/06 08:10:04  d3x0r
// Cleaned all warning from core code...
//
// Revision 1.7  2003/07/28 08:45:16  panther
// Cleanup exports, and use cproc type for threadding
//
// Revision 1.6  2003/03/25 08:59:03  panther
// Added CVS logging
//
