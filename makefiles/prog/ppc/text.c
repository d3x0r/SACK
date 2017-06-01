#include <string.h>
#include <stdio.h>
#include <stdarg.h>
//#include <stdio.h>
#include "./types.h"
#include "mem.h"
#include "fileio.h"
#include "text.h"
#include "global.h"

#define min(a,b) (((a)<(b))?(a):(b))

//#define RELEASE_LOG
static PTEXT last_vartext_result;

//#define STRICT_LINKING

//---------------------------------------------------------------------------

#ifdef STRICT_LINKING
PTEXT SetPriorLineEx( PTEXT seg, PTEXT newprior DBG_PASS )
{
	if( seg == newprior )
	{
		fprintf( stderr, WIDE("ERROR! Segment is same as new prior ") DBG_FILELINEFMT "\n" DBG_RELAY );
		DebugBreak();
	}
	if( seg )
	{
		PTEXT test, start;
		seg->Prior = newprior;
		start = seg;
		test = PRIORLINE( start );
		while( test && test != start )
		{
			test = PRIORLINE( test );
		}
		if( test )
		{
			fprintf( stderr, WIDE("ERROR! Resulting link causes circularity!") DBG_FILELINEFMT "\n" DBG_RELAY );
			DebugBreak();
		}
		test = NEXTLINE( start );
		while( test && test != start )
		{
			test = NEXTLINE( test );
		}
		if( test )
		{
			fprintf( stderr, WIDE("ERROR! Resulting link causes circularity!") DBG_FILELINEFMT "\n" DBG_RELAY );
			DebugBreak();
		}
		return newprior;
	}
	else
	{
		fprintf( stderr, WIDE("ERROR! Attempt to link prior to NULL ") DBG_FILELINEFMT "\n" DBG_RELAY );
		DebugBreak();
	}
	return NULL;
}
#else
PTEXT SetPriorLineEx( PTEXT seg, PTEXT newprior DBG_PASS )
{
	if( seg )
	{
		seg->Prior = newprior;
		return newprior;
	}
	return NULL;
}
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#ifdef STRICT_LINKING
PTEXT SetNextLineEx( PTEXT seg, PTEXT newnext DBG_PASS )
{
	if( seg == newnext )
	{
		fprintf( stderr, WIDE("ERROR! Segment is same as new next ") DBG_FILELINEFMT "\n" DBG_RELAY );
	}
	if( seg )
	{
		PTEXT test, start;
		seg->Next = newnext;
		start = seg;
		test = NEXTLINE( start );
		while( test && test != start )
		{
			test = NEXTLINE( test );
		}
		if( test )
		{
			fprintf( stderr, WIDE("ERROR! Resulting link causes circularity!") DBG_FILELINEFMT "\n" DBG_RELAY );
			DebugBreak();
		}
		test = PRIORLINE( start );
		while( test && test != start )
		{
			test = PRIORLINE( test );
		}
		if( test )
		{
			fprintf( stderr, WIDE("ERROR! Resulting link causes circularity!") DBG_FILELINEFMT "\n" DBG_RELAY );
			DebugBreak();
		}
		return newnext;
	}
	else
		fprintf( stderr, WIDE("ERROR! Attempt to link prior to NULL ") DBG_FILELINEFMT "\n" DBG_RELAY );
	return NULL;
}
#else
PTEXT SetNextLineEx( PTEXT seg, PTEXT newnext DBG_PASS )
{
	if( seg )
	{
		seg->Next = newnext;
		return newnext;
	}
	return NULL;
}
#endif

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

PTEXT SegCreateEx( size_t size DBG_PASS )
{
	PTEXT pTemp;
	pTemp = AllocateEx( sizeof(TEXT) + size DBG_RELAY ); // good thing [1] is already counted.
	MemSet( pTemp, 0, sizeof(TEXT) + size );
	pTemp->data.size = size; // physical space IS one more....
	return pTemp;
}

//---------------------------------------------------------------------------

PTEXT GetIndirect(PTEXT segment )
{
	if( !segment )
		return NULL;
	if( !(segment->flags&TF_INDIRECT) )
		return 0;
	return (PTEXT)segment->data.size;
}

//---------------------------------------------------------------------------

char *GetTextEx( PTEXT segment )
{
	if( !segment )
		return NULL;
	while( segment && segment->flags & TF_INDIRECT )
		segment = GetIndirect( segment );
	if( segment )
		return segment->data.data;
	return NULL;
}

//---------------------------------------------------------------------------

INDEX GetTextSize( PTEXT segment )
{
	int flags;
	if( !segment )
		return 0; // perhaps return -1, 0xFFFFFFFF
	if( segment->flags & TF_INDIRECT )
		return GetTextSize( GetIndirect( segment ) );
	if( !segment->data.size )
	{
		flags = *(int*)&segment->flags;
		if( flags & IS_DATA_FLAGS )
			return 2; // is data even if is not acurate....
	}
	return segment->data.size;
}

//---------------------------------------------------------------------------


int GetTextFlags( PTEXT segment )
{
	if( !segment )
		return 0;
	if( segment->flags & TF_INDIRECT )
		return GetTextFlags( GetIndirect( segment ) );
	return segment->flags;
}

//---------------------------------------------------------------------------

PTEXT SegDuplicateEx( PTEXT pText DBG_PASS )
{
	PTEXT t;
	size_t n;
	if( pText )
	{
		if( pText->flags & TF_INDIRECT )
		{
			t = SegCreateIndirect( GetIndirect( pText ) );
			t->flags = pText->flags;
		}
		else
		{
			t = SegCreateEx( n = GetTextSize( pText ) DBG_RELAY );
			t->format = pText->format;
			t->flags  = pText->flags;
			MemCpy( GetText(t), GetText(pText), n );
		}
		t->flags &= ~(TF_STATIC);
		pText = t;
	}
	return pText;
}

//---------------------------------------------------------------------------

PTEXT TextDuplicateEx( PTEXT pText DBG_PASS )
{
	PTEXT pt;
	PTEXT pDup = NULL, pNew;
	pt = pText;
	while( pt )
	{
		if( pt->flags & TF_INDIRECT )
		{
			pNew = SegCreateIndirectEx( 
							TextDuplicateEx( 
									GetIndirect( pt ) DBG_RELAY ) DBG_RELAY );
			pNew->format = pt->format;
			pNew->flags |= pt->flags&(IS_DATA_FLAGS);
			pNew->flags |= TF_DEEP; 
		}
		else
			pNew = SegDuplicateEx( pt DBG_RELAY );
		pDup = SegAppend( pDup, pNew );
		pt = NEXTLINE( pt );
	}
	SetStart( pDup );
	return pDup;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromTextEx( char *text DBG_PASS )
{
	PTEXT pTemp;
	size_t   nSize;
	if( text )
	{
		pTemp = SegCreateEx( nSize = strlen( text ) DBG_RELAY );
		MemCpy( pTemp->data.data, text, nSize );
		return pTemp;
	}
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromIntEx( int value DBG_PASS )
{
	PTEXT pResult;
	pResult = SegCreateEx( 12 DBG_RELAY);
	pResult->data.size = sprintf( pResult->data.data, WIDE("%d"), value );
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromFloatEx( float value DBG_PASS )
{
	PTEXT pResult;
	pResult = SegCreateEx( 32 DBG_RELAY);
	pResult->data.size = sprintf( pResult->data.data, WIDE("%g"), value );
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateIndirectEx( PTEXT pText DBG_PASS )
{
	PTEXT pSeg;
	pSeg = SegCreateEx( -1 DBG_RELAY ); // no data content for indirect...
	pSeg->flags |= TF_INDIRECT;
/*
	if( pText && !(pText->flags & TF_STATIC ) )
		HoldEx( pText DBG_RELAY );
*/
	pSeg->data.size = (uintptr_t)pText;
	return pSeg;
}

//---------------------------------------------------------------------------

PTEXT SegBreak(PTEXT segment)  // remove leading segments.
{    // return leading segments!  might be ORPHANED if not handled.
	PTEXT temp;
	if( !segment )
		return NULL;
	if( ( temp = PRIORLINE(segment) ) != NULL )
		SETNEXTLINE(temp, NULL );
	SETPRIORLINE( segment, NULL );
	return(temp);
}

//---------------------------------------------------------------------------

PTEXT SegAppend(PTEXT source,PTEXT other)
{
	PTEXT temp=source;

	if( temp )
	{
		if( other )
		{
			SetEnd(temp);
			SetStart( other );
			SETNEXTLINE( temp, other );
			SETPRIORLINE( other, temp );
		}
	}
	else
	{
		source=other;  // nothing was before...
	}
	return(source);
}

//---------------------------------------------------------------------------

PTEXT SegAdd(PTEXT source,PTEXT other)
{
	PTEXT temp=source;

	if( temp )
	{
		if( other )
		{
			SetEnd(temp);
			SetStart( other );
			SETNEXTLINE( temp, other );
			SETPRIORLINE( other, temp );
			return other; // return new addition...
		}
	}
	else
	{
		source=other;  // nothing was before...
	}
	return(source);
}

//---------------------------------------------------------------------------

PTEXT SegExpandEx(PTEXT source, int nSize DBG_PASS)
{
	PTEXT temp;
	temp = SegCreateEx( ( GetTextSize( source ) + nSize ) DBG_RELAY );
	if( source )
	{
		MemCpy( temp->data.data, source->data.data, GetTextSize( source ) );
		temp->flags = source->flags;
		temp->format = source->format;
		SegSubst( temp, source );
		SegRelease( source );
	}
	return temp;
}

//---------------------------------------------------------------------------

void SegReleaseEx( PTEXT seg DBG_PASS)
{
	if( seg )
		ReleaseExx( (void**)&seg DBG_RELAY );
}

//---------------------------------------------------------------------------

void LineReleaseEx(PTEXT *ppLine DBG_PASS )
{
	PTEXT line = *ppLine;
	PTEXT temp;
#ifdef RELEASE_LOG
	static int levels;
#endif
	if( !line )
		return;
#ifdef RELEASE_LOG
	levels++;
	printf( WIDE("Release...%d\n "), levels);
#endif
	SetStart(line);
	temp = line;
	while(line = temp)
	{
		temp=NEXTLINE(line);
		if( line == last_vartext_result )
			last_vartext_result = NULL;
		if( !(line->flags&TF_STATIC) )
		{
			if( (( line->flags & (TF_INDIRECT|TF_DEEP) ) == (TF_INDIRECT|TF_DEEP) ) )
			{
				LineReleaseEx( (PTEXT*)&line->data.size DBG_RELAY );
			}
			ReleaseExx( (void**)&line DBG_RELAY );
		}
		else
		{
			//Log( WIDE("Attempt to free static text...\n") );
		}
	}
#ifdef RELEASE_LOG
	levels--;
#endif
	*ppLine = NULL;
}

//---------------------------------------------------------------------------

PTEXT SegConcatEx(PTEXT output,PTEXT input,int32_t offset,int32_t length DBG_PASS )
{
	int32_t idx=0,len=0;
	output=SegExpandEx(output, length DBG_RELAY); /* add 1 for a null */

	GetText(output)[0]=0;

	while (input&&idx<length)
	{
		len = min( (int32_t)GetTextSize( input ) - offset, length-idx );
		MemCpy( GetText(output) + idx,
				  GetText(input) + offset,
				  len );
		idx += len;
		offset = 0;
		input=NEXTLINE(input);
	}
	GetText(output)[idx]=0;
	return(output);
}

//---------------------------------------------------------------------------

PTEXT SegUnlink(PTEXT segment)
{
	PTEXT temp;
	if (segment)
	{
		if( ( temp = PRIORLINE(segment) ) != NULL )
			SETNEXTLINE( temp, NEXTLINE(segment) );
		if( ( temp = NEXTLINE(segment) ) != NULL )
			SETPRIORLINE( temp, PRIORLINE(segment) );
		SETPRIORLINE(segment, NULL);
		SETNEXTLINE(segment, NULL);
	}
	return segment;
}

//---------------------------------------------------------------------------

PTEXT SegGrab( PTEXT segment )
{
	SegUnlink( segment );
	return segment;
}

//---------------------------------------------------------------------------

PTEXT SegDeleteEx( PTEXT *segment DBG_PASS )
{
	SegGrab( *segment );
	LineReleaseEx( segment DBG_RELAY );
	return NULL;
}

//---------------------------------------------------------------------------


PTEXT SegInsert( PTEXT what, PTEXT before )
{
	PTEXT that_start = what , 
			that_end= what;
	SetStart( that_start );
	SetEnd( that_end );
	if( before )
	{
		if( ( SETPRIORLINE( that_start, PRIORLINE(before) ) ) != NULL )
			SETNEXTLINE( PRIORLINE(that_start), that_start );
		if( ( SETNEXTLINE(that_end, before) ) != NULL )
			SETPRIORLINE( NEXTLINE( that_end ), that_end );
	}
	return what;
}

//---------------------------------------------------------------------------

PTEXT SegSubst( PTEXT _this, PTEXT that )
{
	PTEXT that_start = that , 
			that_end= that;
	SetStart( that_start );
	SetEnd( that_end );

	if( ( SETNEXTLINE(that_end, NEXTLINE(_this) ) ) != NULL )
		SETPRIORLINE(NEXTLINE(that_end),  that_end );

	if( ( SETPRIORLINE(that_start, PRIORLINE(_this))) != NULL )
		SETNEXTLINE(PRIORLINE(that_start), that_start );

	SETNEXTLINE(_this, NULL );
	SETPRIORLINE( _this, NULL );
	return _this;
}

//---------------------------------------------------------------------------

PTEXT SegSubstRangeEx( PTEXT *pp_this, PTEXT end, PTEXT that DBG_PASS )
{
	PTEXT _this = *pp_this;
	PTEXT after_end = NEXTLINE( end );
	if( !_this || !end )
	{
		fprintf( stderr, WIDE("%s(%d): returned early from segsubstrange:%p %p %p\n")
				, GetCurrentFileName()
				, GetCurrentLine()
				, _this, end, that );
		return NULL;
	}

	if( !that )
	{
		if( PRIORLINE( _this ) )
			SETNEXTLINE( PRIORLINE(_this), NEXTLINE(end) );
		if( NEXTLINE( end ) )
			SETPRIORLINE( NEXTLINE( end ), PRIORLINE( _this ) );

	}
	else
	{
		PTEXT that_start = that , 
				that_end= that;
	
		SetStart( that_start );
		SetEnd( that_end );
	
		if( ( SETNEXTLINE( that_end, NEXTLINE(end)) ) != NULL )
			SETPRIORLINE(NEXTLINE(that_end), that_end );
	
		if( ( SETPRIORLINE( that_start, PRIORLINE(_this))) != NULL )
			SETNEXTLINE(PRIORLINE(that_start), that_start );
	}
	SETNEXTLINE( end, NULL );
	SETPRIORLINE( _this, NULL );
	LineReleaseEx( pp_this DBG_RELAY );
	if( that )
		*pp_this = that;
	else
		*pp_this = after_end;
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT SegSplitEx( PTEXT *pLine, size_t nPos  DBG_PASS)
{
	// there includes the character at nPos - so all calculations
	// on there are +1...
	PTEXT here, there;
	size_t nLen;
	nLen = GetTextSize( *pLine );
	if( nPos > nLen )
	{
		return NULL;
	}
	if( nPos == nLen )
		return *pLine;
	here = SegCreateEx( nPos DBG_RELAY );
	here->flags  = (*pLine)->flags;
	here->format = (*pLine)->format;
	there = SegCreateEx( (nLen - nPos) DBG_RELAY );
	there->flags  = (*pLine)->flags;
	there->format = (*pLine)->format;
	there->format.spaces = 0; // pretty safe...
	there->format.tabs = 0; // pretty safe...
	if( here == there )
	{
		fprintf( stderr, WIDE("Hmm - error in segpslit\n") );
#ifdef __LINUX__
		DebugBreak();
#endif
	}
	MemCpy( GetText( here ), GetText( *pLine ), nPos );
	if( nLen - nPos )
		MemCpy( GetText( there ), GetText( *pLine ) + nPos, (nLen - nPos) );

	if( PRIORLINE( *pLine ) )
		SETNEXTLINE( PRIORLINE( *pLine ), here );
	SETPRIORLINE( here, PRIORLINE( *pLine ) );
	SETNEXTLINE( here, there );
	SETPRIORLINE( there, here );
	SETNEXTLINE( there, NEXTLINE( *pLine ) );
	if( NEXTLINE( *pLine ) )
		SETPRIORLINE( NEXTLINE( *pLine ), there );

	SETNEXTLINE( *pLine, NULL );
	SETPRIORLINE( *pLine, NULL );

	LineRelease( *pLine );
	*pLine = here;
	return here;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

size_t LineLength( PTEXT pt, int bSingle )
{
	int	TopSingle = bSingle;
	PTEXT pStack[32];
	int   nStack;
	int   skipspaces = ( PRIORLINE(pt) == NULL );
	size_t length = 0;
	nStack = 0;
	while( pt )
	{
		if( !(pt->flags & ( IS_DATA_FLAGS | TF_INDIRECT)) &&
			 !pt->data.size
		  )
			length += 2; // full binary \r\n insertion assumed
		else
		{
			if( skipspaces ) {
				skipspaces = FALSE;
				if( g.flags.keep_comments ) {
					length += pt->format.tabs; // not-including NULL.
					length += pt->format.spaces; // not-including NULL.
				}
			}
			else {
				length += pt->format.tabs; // not-including NULL.
				length += pt->format.spaces; // not-including NULL.
			}
			if( pt->flags&TF_INDIRECT )
			{
				bSingle = FALSE; // will be restored when we get back to top seg.
				pStack[nStack++] = pt;
				pt = GetIndirect( pt );
				//if( nStack >= 32 )
				//   DebugBreak();
				continue;
			}
			else
				length += GetTextSize( pt ); // not-including NULL.

stack_resume: ;

		}
		if( bSingle )
		{
			bSingle = FALSE;
			break;
		}
		pt = NEXTLINE( pt );
	}
	if( nStack )
	{
		pt = pStack[--nStack];
		if( !nStack )
			bSingle = TopSingle;
		goto stack_resume;
	}
//	if( length > 60000 )
//		_asm int 3;
	return length;
}

//---------------------------------------------------------------------------

int levels = 0;

// attempts to build a solitary line segment from the text passed
// however, if there are color changes, or absolute position changes
// this cannot work... and it must provide multiple peices...

PTEXT BuildLineEx( PTEXT pt, int bSingle DBG_PASS )
{
	char *buf;
	int   TopSingle = bSingle;
	PTEXT pStack[32];
	int   nStack, spaces = 0, tabs = 0, firstadded;
	int   skipspaces = ( PRIORLINE(pt) == NULL );
	PTEXT pOut;
	uintptr_t ofs;
	//DebugBreak();

	{
		int len;
		len = LineLength( pt, bSingle );
		if( !len )
			return NULL;
		pOut = SegCreateEx( len DBG_RELAY );
		firstadded = TRUE;
		buf = GetText( pOut );
	}

	ofs = 0;
	nStack = 0;
	while( pt )
	{

		if( !(pt->flags& (TF_INDIRECT|IS_DATA_FLAGS)) &&
			 !pt->data.size
		  )
		{
			buf[ofs++] = '\r';
			buf[ofs++] = '\n';
		}
		else
		{
			if( skipspaces )
			{
				skipspaces = FALSE;
				if( g.flags.keep_comments ) {
					spaces = pt->format.tabs;
					// else we cannot collapse into single line (similar to colors.)
					while( spaces-- )
					{
						buf[ofs++] = '\t';
					}
					spaces = pt->format.spaces;
					// else we cannot collapse into single line (similar to colors.)
					while( spaces-- )
					{
						buf[ofs++] = ' ';
					}
				}
			}
			else
			{
				spaces = pt->format.tabs;
				// else we cannot collapse into single line (similar to colors.)
				while( spaces-- )
				{
					buf[ofs++] = '\t';
				}
				spaces = pt->format.spaces;
				// else we cannot collapse into single line (similar to colors.)
				while( spaces-- )
				{
					buf[ofs++] = ' ';
				}
			}

			// at this point spaces before tags, and after tags
			// which used to be expression level parsed are not
			// reconstructed correctly...

			if( pt->flags&TF_INDIRECT )
			{
				bSingle = FALSE; // will be restored when we get back to top.
				pStack[nStack++] = pt;
				pt = GetIndirect( pt );
				//if( nStack >= 32 )
				//	DebugBreak();
				continue;
			}
			else
			{
				 size_t len;
				 MemCpy( buf+ofs, GetText( pt ), len = GetTextSize( pt ) );
				 ofs += len;
			}

stack_resume: ;

		}
		if( bSingle )
		{
			bSingle = FALSE;
			break;
		}
		pt = NEXTLINE( pt );
	}
	if( nStack )
	{
		pt = pStack[--nStack];
		if( !nStack )
			bSingle = TopSingle;
		goto stack_resume;
	}

	if( !pOut ) // have to return length instead of new text seg...
		return (PTEXT)ofs;
	SetStart( pOut ); // if formatting was inserted into the stream...
	return pOut;
}

//---------------------------------------------------------------------------

int CompareStrings( PTEXT pt1, int single1
                  , PTEXT pt2, int single2
                  , int bExact )
{
	while( pt1 && pt2 )
	{
		if( !pt1 && pt2 )
			return FALSE;
		if( pt1 && !pt2 )
			return FALSE;
		if( bExact )
		{
			if( !SameText( pt1, pt2 ) )
				return FALSE;
		}
		else
		{
			// Like returns string compare function literal...
			if( LikeText( pt1, pt2 ) )
				return FALSE;
		}
		if( !single1 )
		{
			pt1 = NEXTLINE( pt1 );
			if( pt1
			  && !GetTextSize( pt1 ) && !(pt1->flags & IS_DATA_FLAGS))
				pt1 = NULL;
		}
		else
			pt1 = NULL;
		if( !single2 )
		{
			pt2 = NEXTLINE( pt2 );
			if( pt2 &&
			    !GetTextSize( pt2 ) && 
			    !(pt2->flags & IS_DATA_FLAGS))
				pt2 = NULL;
		}
		else
			pt2 = NULL;
	}
	if( !pt1 && !pt2 )
		return TRUE;
	return FALSE;
}

//---------------------------------------------------------------------------
#define COLLECT_LEN 32

void VarTextInitEx( PVARTEXT pvt DBG_PASS )
{
	pvt->commit = NULL;
	pvt->collect = SegCreateEx( COLLECT_LEN DBG_RELAY );
	pvt->collect_text = GetText( pvt->collect );
	pvt->collect_used = 0;
}

//---------------------------------------------------------------------------

void VarTextEmptyEx( PVARTEXT pvt DBG_PASS )
{
	LineReleaseEx( &pvt->collect DBG_RELAY );
	LineReleaseEx( &pvt->commit DBG_RELAY );
	MemSet( pvt, 0, sizeof( VARTEXT ) );
}

//---------------------------------------------------------------------------

void VarTextAddCharacterEx( PVARTEXT pvt, char c DBG_PASS )
{
	if( !pvt->collect )
		VarTextInitEx( pvt DBG_RELAY );
	//fprintf( stderr, WIDE("Adding character %c\n"), c );
	pvt->collect_text[pvt->collect_used++] = c;
	if( pvt->collect_used == GetTextSize( pvt->collect ) )
	{
		//fprintf( stderr, WIDE("Expanding segment to make sure we have room to extend...\n") );
		pvt->collect = SegExpandEx( pvt->collect, COLLECT_LEN DBG_RELAY );
		pvt->collect_text = GetText( pvt->collect );
	}
}

//---------------------------------------------------------------------------

PTEXT VarTextEndEx( PVARTEXT pvt DBG_PASS )
{
	if( pvt->collect_used ) // otherwise ofs will be 0...
	{
		PTEXT segs;
		segs = SegSplitEx( &pvt->collect, pvt->collect_used DBG_RELAY );
		//fprintf( stderr, WIDE("Breaking collection adding... %s\n"), GetText( segs ) );
		// so now the remaining buffer( if any ) 
		// is assigned to collect into.
		// This results in...

		pvt->collect = NEXTLINE( pvt->collect );

		if( !pvt->collect ) // used all of the line...
		{
			//fprintf( stderr, WIDE("Starting with new buffers\n") );
			pvt->collect = SegCreateEx( COLLECT_LEN DBG_RELAY );
			pvt->collect_text = GetText( pvt->collect );
			pvt->collect_used = 0;
		}
		else
		{
			//Log1( WIDE("Remaining buffer is %d"), GetTextSize( pvt->collect ) );
			SegBreak( pvt->collect );
			pvt->collect_text = GetText( pvt->collect );
			pvt->collect_used = 0;
		}
		pvt->commit = SegAppend( pvt->commit, segs );
		//fprintf( stderr, WIDE("Resulting string: \'%s\' newly added vartextend\n"), GetText( segs ) );
		return segs;
	}
	if( pvt->commit )
	{
		//fprintf( stderr, WIDE("Resulting exsiting commit output...\n") );
		return pvt->commit;
	}
	//fprintf( stderr, WIDE("Resulting no output... var text end\n") );
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT VarTextGetEx( PVARTEXT pvt DBG_PASS )
{
	if( VarTextEndEx( pvt DBG_RELAY ) )
	{
		PTEXT result = pvt->commit;
		pvt->commit = NULL;
		if( last_vartext_result )
		{
#ifdef __LINUX__
			if( last_vartext_result == result )
				asm( WIDE("int $3;\n") );
#endif
		}
		last_vartext_result = result;
		return result;
	}
	return NULL;
}

//---------------------------------------------------------------------------

void VarTextExpandEx( PVARTEXT pvt, int size DBG_PASS)
{
	pvt->collect = SegExpandEx( pvt->collect, size DBG_RELAY );
	pvt->collect_text = GetText( pvt->collect );
}

//---------------------------------------------------------------------------

size_t VarTextLength( PVARTEXT pvt )
{
	//Log1( WIDE("Length is : %d"), pvt->collect_used );
	return pvt->collect_used;
}

//---------------------------------------------------------------------------

int vtprintfEx( PVARTEXT pvt , char *format, ... )
//int vtprintfEx( PVARTEXT pvt DBG_PASS, char *format, ... )
{
	//char valid_char;
	va_list args;
	//Log1( WIDE("vtprintf...%s"), format );
	va_start( args, format );
	{
#if defined( __LINUX__ )
		// len returns number of characters (not NUL)
		int len = vsnprintf( NULL, 0, format, args );
		// allocate +1 for length with NUL
//		VarTextExpandEx( pvt, len+1 DBG_RELAY );
		VarTextExpand( pvt, len+1  );
		//Log3( WIDE("Print Length: %d into %d after %s"), len, pvt->collect_used, pvt->collect_text );
		// include NUL in the limit of characters able to print...
		  vsnprintf( pvt->collect_text + pvt->collect_used, len+1, format, args );
#elif defined( __WATCOMC__ )
		int len, destlen;
		do {
			va_start( args, format );
			len = vsnprintf( pvt->collect_text + pvt->collect_used
			               , destlen = GetTextSize( pvt->collect ) - pvt->collect_used
			               , format, args );
			if( len > destlen )
				VarTextExpand( pvt, len - destlen + 1 );
		} while( len > destlen );
#else
#if defined( GCC ) || defined( _MSC_VER )
	#define vsnprintf _vsnprintf
#endif
		int len;
		do {
			len = vsnprintf( pvt->collect_text + pvt->collect_used
			                , GetTextSize( pvt->collect ) - pvt->collect_used
			                , format, args );
			if( len < 0 )
				VarTextExpand( pvt, 32 );
//				VarTextExpandEx( pvt, 32 DBG_SRC );
		} while( len < 0 );
		//Log1( WIDE("Print Length: %d"), len );
#endif
		pvt->collect_used += len;
		return len;
	}
}
