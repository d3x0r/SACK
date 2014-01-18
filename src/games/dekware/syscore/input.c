#ifndef CORE_SOURCE
#define CORE_SOURCE
#endif
#include <stdhdrs.h>
#include "sharemem.h"
#include "input.h" // includes text.h...

#define PARSE_DEEP 0 // make true to recurse parse expressions

#define Collapse(towhere) SegConcat(towhere,begin,beginoffset,total)

#define IsBracket(Close)	{                                       \
					if (!quote&&!bracket)                                    \
					{                                                        \
						/* flush all up till this character. */               \
						if( !escape )                                         \
						{                                                     \
							bracket=Close;                                     \
							bracketlevel=1;                                    \
							if (total)                                         \
								outdata=Collapse(outdata);                      \
							begin=input;                                       \
							beginoffset=index;                                 \
							total=1;                                           \
						}                                                     \
						else                                                  \
						{                                                     \
							total++;                                           \
							escape = FALSE;                                    \
						}                                                     \
					}                                                        \
					else                                                     \
					{                                                        \
						if (bracket==Close)                                   \
							bracketlevel++;                                    \
						total++;	                                           \
					} }


//----------------------------------------------------------------------

TEXTCHAR NextCharEx( PTEXT input, INDEX idx )
{
	if( ( ++idx ) >= input->data.size ) 
	{
		idx -= input->data.size;
		input = NEXTLINE( input );
	}
	if( input )
		return input->data.data[idx];
	return 0;
}
#define NextChar() NextCharEx( input, index )
//----------------------------------------------------------------------
//----------------------------------------------------------------------
#define BUILD_LINE_OUTPUT_SIZE 256

PTEXT SplitLine( PTEXT pLine, INDEX nPos )
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

CORE_PROC( PTEXT, GatherLineEx )( PTEXT *pOutput, INDEX *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput )
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

	INDEX pos
       , len = 0
       , size
       , maxlen = 0;
	PTEXT pReturn = NULL;
	PTEXT pDelete = NULL;
	TEXTCHAR character;
	TEXTSTR output;

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
				*pOutput = SplitLine( *pOutput, *pIndex );
				output = GetText( *pOutput );
				len = *pIndex = GetTextSize( *pOutput );
			}
			else
			{
				maxlen = (*pOutput)->data.size;
			}
		}
	}
	else
	{
		output = GetText( *pOutput );
		len = GetTextSize( *pOutput );
	}

	while( pInput )
	{
		size = GetTextSize( pInput );
		for( pos = 0; pos < size; pos++ )
		{
			switch( character = GetText( pInput )[pos] )
			{
			case '\x1b':
				if( !bData )
				{
					SetEnd( *pOutput );
					SetStart( *pOutput )
					{
						SetTextSize( *pOutput, 0 );
					}
					SetTextSize( *pOutput, 0 );
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
							size_t len2;
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
							size_t len2;
							TEXTCHAR *data;
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
					size_t maxlen;
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
							size_t sz;
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
						SetTextSize( *pOutput, --len );
					}
					else
					{
						if( PRIORLINE( *pOutput ) )
						{
							*pOutput = PRIORLINE( *pOutput );
							len = GetTextSize( *pOutput );
						}

						if( len )
							SetTextSize( *pOutput, --len );
					}
				}
				break;
			case '\r': // ignore this character...
				if( !bSaveCR )
					break;
				// falls through .. past this and saves the return...
				if(0)
			case '\n':
				if( !pReturn )
				{
					// transfer *pOutput to pReturn....
					pReturn = *pOutput;
					SetEnd( pReturn );
					output = GetText( pReturn );
					len = GetTextSize( pReturn );
					output[len] = character;
					SetTextSize( pReturn, ++len );
					// begin next collection in case more data is in the input...
					*pOutput = SegCreate( BUILD_LINE_OUTPUT_SIZE );
					SetTextSize( *pOutput, 0 );
					output = GetText( *pOutput );
					len = 0;
					break;
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
	SetStart( pReturn )
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

CORE_PROC( void, RecallCommand )( PCOMMAND_INFO pci, int bUp )
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
		if( pci->nHistory >= (int)pci->InputHistory->Cnt )
			pci->nHistory -= pci->InputHistory->Cnt;
		if( pci->nHistory == (signed)pci->InputHistory->Top )
		{
			// now we are at the NEW entry to the list...
			// nothing yet - can leave buffer at NULL, return
			// will result in OUTPUT, ClearOuput, Show NULL...
			LineRelease( pci->CollectionBuffer );
			pci->CollectionBuffer = NULL;
			pci->CollectionIndex = 0;
			pci->nHistory = -1;
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

CORE_PROC( void, EnqueCommandHistory )( PCOMMAND_INFO pci, PTEXT pHistory )
{
	PTEXT pCommand;

	if( GetQueueLength( pci->InputHistory ) > 50 ) // keep history somewhat short...
	{
		LineRelease( (PTEXT)DequeLink( &pci->InputHistory ) );
	}

	if( pCommand = BuildLine( pHistory ) )
	{
		const PTEXT ppLast = (PTEXT)PeekQueue( pci->InputHistory );
		if( ppLast && ( SameText( pCommand, ppLast ) == 0 ) )
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

CORE_PROC( PTEXT, GatherCommand )( PCOMMAND_INFO pci, PTEXT stroke )
{
	PTEXT pLine = GatherLine( &pci->CollectionBuffer
							, &pci->CollectionIndex
							, pci->CollectionInsert
							, TRUE
							, stroke );
	if( pLine )
		EnqueCommandHistory( pci, pLine );
	return pLine;
}


//----------------------------------------------------------------------------

CORE_PROC( void, EmptyCommandHistory )( PCOMMAND_INFO pci )
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

CORE_PROC( PCOMMAND_INFO, CreateCommandHistoryEx )( PromptCallback prompt )
#define CreateCommandHistory() CreateCommandHistoryEx( NULL )
{
	PCOMMAND_INFO pci = New( COMMAND_INFO );
	pci->CollectionBufferLock = FALSE;

	pci->CollectionBuffer = NULL;
	pci->InputHistory = CreateLinkQueue();
	pci->nHistory = -1;
	pci->bRecallBegin = FALSE;

	pci->CollectionIndex = 0;
	pci->CollectionInsert = TRUE;
	pci->Prompt = prompt;

	return pci;	
}

//----------------------------------------------------------------------------

CORE_PROC( void, DestroyCommandHistory )( PCOMMAND_INFO *pci )
{
	if( pci )
	{
		EmptyCommandHistory( *pci );
		Release( *pci );
		*pci = NULL;
	}
}

//----------------------------------------------------------------------------


CORE_PROC( int, SetCommandPosition )( PCOMMAND_INFO pci, int nPos, int whence )
{
	if( whence == SEEK_SET )
	{
		if( nPos < 0 )
		{
			int nLen;
			while( NEXTLINE( pci->CollectionBuffer ) )
				pci->CollectionBuffer = NEXTLINE( pci->CollectionBuffer );
    		nLen = GetTextSize( pci->CollectionBuffer );
 			nPos++; // plus 1...
 			if( nLen > nPos )
    		{
				pci->CollectionIndex = nLen + nPos;
			}
			else
				pci->CollectionIndex = 0;
		}  	
		else // position positive is pos along line...s
		{
			if( !PRIORLINE( pci->CollectionBuffer ) && !pci->CollectionIndex )
				return FALSE;
			while( PRIORLINE( pci->CollectionBuffer ) )
				pci->CollectionBuffer = PRIORLINE( pci->CollectionBuffer );
			pci->CollectionIndex = 0;
		}
	}
	else if( whence == SEEK_CUR )
	{
		nPos += pci->CollectionIndex;
		//pci->CollectionIndex += nPos;
			
		while( nPos >= (int)GetTextSize( pci->CollectionBuffer ) )
		{
			if( NEXTLINE( pci->CollectionBuffer ) )
			{
				nPos -= GetTextSize( pci->CollectionBuffer );
				pci->CollectionBuffer = NEXTLINE( pci->CollectionBuffer );
			}
			else
			{
				// hmm well...set to last character in this buffer..
				nPos = GetTextSize( pci->CollectionBuffer );
				break;
			}
     }
     while( nPos < 0 && PRIORLINE( pci->CollectionBuffer) )
     {
          pci->CollectionBuffer = PRIORLINE( pci->CollectionBuffer );
          nPos += GetTextSize( pci->CollectionBuffer );
     }
       if( nPos < 0 )
        nPos = 0;

		pci->CollectionIndex = nPos;
		
	}
	return TRUE;
}

//----------------------------------------------------------------------------

CORE_PROC( void, SetCommandInsert )( PCOMMAND_INFO pci, int bInsert )
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
