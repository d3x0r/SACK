/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   code to provide a robust text class for C
 *   Parsing, text substitution, replacment, phrase splitting
 *   options for paired parsing of almost all pairable symbols
 *   used in common language.
 *
 *
 * see also - include/typelib.h
 *
 */
#define NO_UNICODE_C
#include <stdhdrs.h>
#include <deadstart.h>
#include <stdarg.h>
#include <sack_types.h>
#include <sharemem.h>
#include <logging.h>

#include <stdio.h>
#include <wchar.h>

#ifdef _MSC_VER
// derefecing NULL pointers; the function wouldn't be called with a NULL.
// and partial expressions in lower precision
// and NULL math because never NULL.
#  pragma warning( disable:6011 26451 28182)
//Warning C26451: Arithmetic overflow: Using operator '%operator%'
// on a %size1% byte value and then casting the result to a
// %size2% byte value. Cast the value to the wider type
// before calling operator '%operator%' to avoid overflow
#  pragma warning( disable:26451 )
#endif

#ifdef __cplusplus
namespace sack {
namespace containers {
namespace text {
	using namespace sack::memory;
	using namespace sack::logging;
	using namespace sack::containers::queue;
#endif

typedef PTEXT (CPROC*GetTextOfProc)( uintptr_t, POINTER );

typedef struct text_exension_tag {
	uint32_t bits;
	GetTextOfProc TextOf;
	uintptr_t psvData;
}  TEXT_EXTENSION, *PTEXT_EXTENSION;

typedef struct vartext_tag {
	TEXTSTR collect_text;
	size_t collect_used;
	size_t collect_avail;
	size_t expand_by;
	PTEXT collect;
	PTEXT commit;
} VARTEXT;


//#ifdef __cplusplus
static PTEXT newline;
static PTEXT blank;
PRELOAD( AllocateDefaults )
{
	newline = (PTEXT)SegCreateFromText( "" );
	blank = (PTEXT)SegCreateFromText( " " );
}
//#define newline (*newline)
//#define blank	(*blank)
//#else
//__declspec( dllexport ) TEXT newline = { TF_STATIC, NULL, NULL, {1,1},{0,""}};
//__declspec( dllexport ) TEXT blank = { TF_STATIC, NULL, NULL, {1,1},{1," "}};
//#endif
static PLIST pTextExtensions;
//---------------------------------------------------------------------------

PTEXT SegCreateEx( size_t size DBG_PASS )
{
	PTEXT pTemp;
#if defined( _MSC_VER )
	//if( size > 0x8000 )
	//	_asm int 3;
#endif
	pTemp = (PTEXT)AllocateEx( sizeof(TEXT) + (size
#ifdef _MSC_VER
		+ 1
#endif
		)*sizeof(TEXTCHAR)
		DBG_RELAY ); // good thing [1] is already counted.
	MemSet( pTemp, 0, sizeof(TEXT) + (size*sizeof(TEXTCHAR)) );
	pTemp->format.flags.prior_background = 1;
	pTemp->format.flags.prior_foreground = 1;
	pTemp->data.size = size; // physical space IS one more....
	return pTemp;
}

//---------------------------------------------------------------------------

PTEXT GetIndirect(PTEXT segment )
{
	if( !segment )
		return NULL;
	if( (segment->flags&TF_APPLICATION) )
	{
		INDEX idx;
		PTEXT_EXTENSION pte;
		LIST_FORALL( pTextExtensions, idx, PTEXT_EXTENSION, pte )
		{
			if( pte->bits & segment->flags )
			{
				// size is used as a pointer...
				segment = pte->TextOf( pte->psvData, (POINTER)segment->data.size );
				break;
			}
		}
		if( !pte )
			return NULL;
		return segment;
	}
	// if it's not indirect... don't result..
	if( !(segment->flags&TF_INDIRECT) )
		return NULL;
	return (PTEXT)(segment->data.size);
}

//---------------------------------------------------------------------------

TEXTSTR GetText( PTEXT segment )
{
	while( segment )
	{
		if( segment->flags & (TF_INDIRECT|TF_APPLICATION) )
		{
			segment = GetIndirect( segment );
		}
		else
			return segment->data.data;
	}
	return NULL;
}

//---------------------------------------------------------------------------

size_t GetTextSize( PTEXT segment )
{
	while( segment )
	{
		if( segment->flags & (TF_INDIRECT|TF_APPLICATION) )
		{
			segment = GetIndirect( segment );
		}
		else
			if( !segment->data.size )
			{
				if( segment->flags & IS_DATA_FLAGS )
				{
					//lprintf( "Is Data falgs returns 2. %08x", segment->flags & IS_DATA_FLAGS );
					return segment->data.size; // is data even if is not acurate....
				}
				break;
			}
			else
				return segment->data.size;
	}
	return 0;
}

//---------------------------------------------------------------------------

uint32_t GetTextFlags( PTEXT segment )
{
	if( !segment )
		return 0;
	if( segment->flags & (TF_INDIRECT|TF_APPLICATION) )
		return GetTextFlags( GetIndirect( segment ) );
	return segment->flags;
}

//---------------------------------------------------------------------------

void SegCopyFormat( PTEXT to_this, PTEXT copy_this )
{
	if( to_this && copy_this )
	{
		if( copy_this && !( copy_this->flags & TF_FORMATPOS ) )
		{
			to_this->format.position.offset.tabs = copy_this->format.position.offset.tabs;
			to_this->format.position.offset.spaces = copy_this->format.position.offset.spaces;
		}
		else
		{
			// copy absolute positioning...
		}
	}
}

//---------------------------------------------------------------------------

PTEXT SegDuplicateEx( PTEXT pText DBG_PASS )
{
	PTEXT t;
	size_t n;
	if( pText )
	{
		if( pText->flags & TF_APPLICATION )
		{
			t = SegCreateIndirect( (PTEXT)pText->data.size );
			t->format = pText->format;
			t->flags = pText->flags;
		}
		else if( pText->flags & TF_INDIRECT )
		{
			t = SegCreateIndirectEx( SegDuplicateEx( GetIndirect( pText ) DBG_RELAY ) DBG_RELAY );
			t->format = pText->format;
			// some other mask needs to be here.. the getindirect
			// will have other flags...
			t->flags = pText->flags;
		}
		else
		{
			t = SegCreateEx( n = GetTextSize( pText ) DBG_RELAY );
			t->format = pText->format;
			MemCpy( GetText(t), GetText(pText), sizeof( TEXTCHAR ) * ( n + 1 ) );
			t->flags = pText->flags;
		}
		t->flags &= ~(TF_DEEP|TF_STATIC);
		return t;
	}
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT LineDuplicateEx( PTEXT pText DBG_PASS )
{
	PTEXT pt;
	pt = pText;
	while( pt )
	{
		if( !(pt->flags&TF_STATIC) )
			HoldEx( (uint8_t*)pt DBG_RELAY  );
		if( (pt->flags & TF_INDIRECT ) || (pt->flags&TF_APPLICATION) )
			LineDuplicateEx( GetIndirect( pt ) DBG_RELAY );
		pt = NEXTLINE( pt );
	}
	return pText;
}

//---------------------------------------------------------------------------

PTEXT TextDuplicateEx( PTEXT pText, int bSingle DBG_PASS )
{
	PTEXT pt;
	PTEXT pDup = NULL, pNew;
	pt = pText;
	while( pt )
	{
		if( (pt->flags & TF_INDIRECT ) && !(pt->flags&TF_APPLICATION) )
		{
			pNew = SegCreateIndirectEx(
			            TextDuplicateEx(
			                  GetIndirect( pt ), bSingle DBG_RELAY ) DBG_RELAY );
			pNew->format.position = pt->format.position;
			pNew->flags |= pt->flags&(IS_DATA_FLAGS);
			pNew->flags |= TF_DEEP;
		}
		else
			pNew = SegDuplicateEx( pt DBG_RELAY );
		pDup = SegAppend( pDup, pNew );
		if( bSingle )
			break;
		pt = NEXTLINE( pt );
	}
	return pDup;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromTextEx( CTEXTSTR text DBG_PASS )
{
	PTEXT pTemp;
	size_t nSize;
	if( text )
	{
		pTemp = SegCreateEx( nSize = StrLen( text ) DBG_RELAY );
		// include nul on copy
		MemCpy( pTemp->data.data, text, sizeof( TEXTCHAR ) * ( nSize + 1 ) );
		return pTemp;
	}
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromCharLenEx( const char *text, size_t len DBG_PASS )
{
	PTEXT pTemp;
	if( text )
	{
		pTemp = SegCreateEx( len DBG_RELAY );
		MemCpy( pTemp->data.data, text, sizeof( TEXTCHAR ) * ( len + 1 ) );
		return pTemp;
	}
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromCharEx( const char *text DBG_PASS )
{
	return SegCreateFromCharLenEx( text, strlen( text ) DBG_RELAY );
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromWideLenEx( const wchar_t *text, size_t nSize DBG_PASS )
{
	PTEXT pTemp;
	if( text )
	{
		TEXTSTR text_string = WcharConvertLen( text, nSize );
		int outlen;
		for( outlen = 0; text_string[outlen]; outlen++ );
		pTemp = SegCreateEx( outlen DBG_RELAY );
		// include nul on copy
		MemCpy( pTemp->data.data, text_string, sizeof( TEXTCHAR ) * ( outlen + 1 ) );
		Deallocate( TEXTSTR, text_string );
		return pTemp;
	}
	return NULL;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromWideEx( const wchar_t *text DBG_PASS )
{
	return SegCreateFromWideLenEx( text, wcslen( text ) DBG_RELAY );
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromIntEx( int value DBG_PASS )
{
	PTEXT pResult;
	pResult = SegCreateEx( 12 DBG_RELAY);
	pResult->data.size = snprintf( pResult->data.data, 12, "%d", value ); //-V512
	pResult->data.data[11] = 0;
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFrom_64Ex( int64_t value DBG_PASS )
{
	PTEXT pResult;
	pResult = SegCreateEx( 32 DBG_RELAY);
	pResult->data.size = snprintf( pResult->data.data, 32, "%" _64f, value ); //-V512
pResult->data.data[31] = 0;
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromFloatEx( double value DBG_PASS )
{
	PTEXT pResult;
	pResult = SegCreateEx( 32 DBG_RELAY);
	pResult->data.size = snprintf( pResult->data.data, 32, "%g", value ); //-V512
	pResult->data.data[31] = 0;
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateIndirectEx( PTEXT pText DBG_PASS )
{
	PTEXT pSeg;
	pSeg = SegCreateEx( -1 DBG_RELAY ); // no data content for indirect...
	pSeg->flags |= TF_INDIRECT;
	pSeg->data.size = (uintptr_t)pText;
	return pSeg;
}

//---------------------------------------------------------------------------

PTEXT SegBreak(PTEXT segment)  // remove leading segments.
{	 // return leading segments!  might be ORPHANED if not handled.
	PTEXT temp;
	if( !segment )
		return NULL;
	if((temp=PRIORLINE(segment)))
		SETNEXTLINE(temp,NULL);
	SETPRIORLINE(segment,NULL);
	return(temp);
}

INDEX  GetSegmentSpaceEx ( PTEXT segment, size_t position, int nTabs, INDEX *tabs)
{
	INDEX total = 0;
	do
	{
		if( segment && !( segment->flags & TF_FORMATPOS ) )
		{
			int n;
			for( n = 0; n < nTabs && (INDEX)position > tabs[n]; n++ );
			if( n < nTabs )
				// now position is before the first tab... such that
				for( ; n < nTabs && n < segment->format.position.offset.tabs; n++ )
				{
					total += tabs[n]-position;
					position = tabs[n];
				}
			//lprintf( "Adding %d spaces", segment->format.position.offset.spaces );
			total += segment->format.position.offset.spaces;
		}
	}
	while( segment && (segment->flags & TF_INDIRECT) && ( segment = GetIndirect( segment ) ) );
	return total;
}
//---------------------------------------------------------------------------
INDEX  GetSegmentSpace ( PTEXT segment, INDEX position, int nTabSize )
{
	INDEX total = 0;
	do
	{
		if( segment && !( segment->flags & TF_FORMATPOS ) )
		{
			int n;
			for( n = 0; n < segment->format.position.offset.tabs; n++ )
			{
				if( !total )
					// I think this is wrong.  need to validate this equation.
					total += (position % nTabSize) + 1;
				else
					total += nTabSize;
			}
			total += segment->format.position.offset.spaces;
		}
	}
	while( (segment->flags & TF_INDIRECT) && ( segment = GetIndirect( segment ) ) );
	return total;
}
//---------------------------------------------------------------------------
INDEX  GetSegmentLengthEx ( PTEXT segment, size_t position, int nTabs, INDEX *tabs )
{
	while( segment && segment->flags & TF_INDIRECT )
		segment = GetIndirect( segment );
	return GetSegmentSpaceEx( segment, position, nTabs, tabs ) + GetTextSize( segment );
}
//---------------------------------------------------------------------------
INDEX  GetSegmentLength ( PTEXT segment, size_t position, int nTabSize )
{
	while( segment && segment->flags & TF_INDIRECT )
		segment = GetIndirect( segment );
	return GetSegmentSpace( segment, position, nTabSize ) + GetTextSize( segment );
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
			SETNEXTLINE(temp,other);
			SETPRIORLINE(other,temp);
		}
	}
	else
	{
		source=other;  // nothing was before...
	}
	return(source);
}

//---------------------------------------------------------------------------

void SegReleaseEx( PTEXT seg DBG_PASS)
{
	if( seg )
		ReleaseEx( seg DBG_RELAY );
}

//---------------------------------------------------------------------------

PTEXT SegExpandEx(PTEXT source, INDEX nSize DBG_PASS)
{
	PTEXT temp;
	//Log1( "SegExpand...%d", nSize );
	temp = SegCreateEx( GetTextSize( source ) + nSize  DBG_RELAY );
	if( source )
	{
		MemCpy( temp->data.data, source->data.data, sizeof( TEXTCHAR)*(GetTextSize( source ) + 1) );
		temp->flags = source->flags;
		temp->format = source->format;
		SegSubst( temp, source );
		SegRelease( source );
	}
	return temp;
}

//---------------------------------------------------------------------------

void LineReleaseEx(PTEXT line DBG_PASS )
{
	PTEXT temp;

	if( !line )
		return;

	SetStart(line);
	while(line)
	{
		temp=NEXTLINE(line);
		if( !(line->flags&TF_STATIC) )
		{
			if( (( line->flags & (TF_INDIRECT|TF_DEEP) ) == (TF_INDIRECT|TF_DEEP) ) )
				if( !(line->flags & TF_APPLICATION) ) // if indirect, don't want to release application content
					LineReleaseEx( GetIndirect( line ) DBG_RELAY );
			ReleaseEx( line DBG_RELAY );
		}
		line=temp;
	}
}

//---------------------------------------------------------------------------

PTEXT SegConcatEx(PTEXT output,PTEXT input,int32_t offset,size_t length DBG_PASS )
{
	size_t idx=0;
	size_t len=0;
	PTEXT newseg;
	SegAppend( output, newseg = SegCreateEx( length DBG_RELAY ) );
	output = newseg;
	//output=SegExpandEx(output, length DBG_RELAY); /* add 1 for a null */

	GetText(output)[0]=0;

	while (input&&idx<length)
	{
		//#define min(a,b) (((a)<(b))?(a):(b))
		if( ( GetTextSize( input ) - offset ) < ( length-idx  ) )
			len = GetTextSize( input ) - offset;
		else
         len = length - idx;
		MemCpy( GetText(output) + idx,
				  GetText(input) + offset,
				  sizeof( TEXTCHAR ) * ( len + 1 ) );
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
		if( ( temp = PRIORLINE(segment) ) )
			SETNEXTLINE(temp,NEXTLINE(segment));
		if( ( temp = NEXTLINE(segment) ) )
			SETPRIORLINE(temp,PRIORLINE(segment));
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

PTEXT SegDelete( PTEXT segment )
{
	LineReleaseEx( SegGrab( segment ) DBG_SRC );
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
		if( ( that_start->Prior = before->Prior) )
			that_start->Prior->Next = that_start;
		if( ( that_end->Next = before ) )
			that_end->Next->Prior = that_end;
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

	if( ( that_end->Next = _this->Next ) )
		that_end->Next->Prior = that_end;

	if( ( that_start->Prior = _this->Prior) )
		that_start->Prior->Next = that_start;

	_this->Next = NULL;
	_this->Prior = NULL;
	return _this;
}

//---------------------------------------------------------------------------

PTEXT SegSplitEx( PTEXT *pLine, INDEX nPos  DBG_PASS)
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
	here->flags  = (*pLine)->flags; //-V595
	here->format = (*pLine)->format;
	there = SegCreateEx( (nLen - nPos) DBG_RELAY );
	there->flags  = (*pLine)->flags; //-V595
	there->format = (*pLine)->format;
	there->format.position.offset.spaces = 0; // was two characters presumably...
	there->format.position.offset.tabs = 0;

	MemCpy( GetText( here ), GetText( *pLine ), sizeof(TEXTCHAR)*nPos );
	GetText( here )[nPos] = 0;
	if( nLen - nPos )
	{
		MemCpy( GetText( there ), GetText( *pLine ) + nPos, sizeof(TEXTCHAR)*(nLen - nPos) );
		GetText( there )[nLen-nPos] = 0;
	}

	SETNEXTLINE( PRIORLINE( *pLine ), here );
	SETPRIORLINE( here, PRIORLINE( *pLine ) );
	SETNEXTLINE( here, there );
	SETPRIORLINE( there, here );
	SETNEXTLINE( there, NEXTLINE( *pLine ) );
	SETPRIORLINE( NEXTLINE( *pLine ), there );

	SETNEXTLINE( *pLine, NULL );
	SETPRIORLINE( *pLine, NULL );

	LineReleaseEx( *pLine DBG_RELAY );
	*pLine = here;
	return here;
}


//----------------------------------------------------------------------

TEXTRUNE NextCharEx( PTEXT input, size_t idx )
{
	if( ( ++idx ) >= input->data.size )
	{
		idx -= input->data.size;
		input = NEXTLINE( input );
	}
	if( input ) {
		return GetUtfCharIndexed( input->data.data, &idx, input->data.size );
		//return input->data.data[idx];
	}
	return 0;
}
#define NextChar() NextCharEx( input, tempText-tempText_ )
//----------------------------------------------------------------------

// In this final implementation - it was decided that for a general
// library, that expressions, escapes of expressions, apostrophes
// were of no consequence, and without expressions, there is no excess
// so this simply is text stream in, text stream out.

// these are just shortcuts - these bits of code were used repeatedly....

#define SET_SPACES() do {		word->format.position.offset.spaces = (uint16_t)spaces; \
		word->format.position.offset.tabs = (uint16_t)tabs;                             \
		spaces = 0;                                                         \
		tabs = 0; } while(0)


//static CTEXTSTR normal_punctuation="\'\"\\({[<>]}):@%/,;!?=*&$^~#`";
//static CTEXTSTR not_punctuation;

PTEXT TextParse( PTEXT input, CTEXTSTR punctuation, CTEXTSTR filter_space, int bTabs, int bSpaces  DBG_PASS )
// returns a TEXT list of parsed data
{
//#define DBG_OVERRIDE DBG_SRC
#define DBG_OVERRIDE DBG_RELAY
	/* takes a line of input and creates a line equivalent to it, but
	   burst into its block pieces.*/
	VARTEXT out;
	PTEXT outdata=(PTEXT)NULL,
	      word;
	TEXTSTR tempText, tempText_;
	int has_minus = -1;
	int has_plus = -1;

	INDEX size;

	TEXTRUNE character;
	uint32_t elipses = FALSE,
	   spaces = 0, tabs = 0;

	if (!input)        // if nothing new to process- return nothing processed.
		return((PTEXT)NULL);

	VarTextInitEx( &out DBG_OVERRIDE );

	while (input)  // while there is data to process...
	{
		if( input->flags & TF_INDIRECT )
		{

			word = VarTextGetEx( &out DBG_OVERRIDE );
			if( word )
			{
				SET_SPACES();
				outdata = SegAppend( outdata, word );
			}
			outdata = SegAppend( outdata, TextParse( GetIndirect( input ), punctuation, filter_space, bTabs, bSpaces DBG_RELAY ) );
			input = NEXTLINE( input );
			continue;
		}
		tempText_ = tempText = GetText(input);  // point to the data to process...
		size = GetTextSize(input);
		if( input->format.position.offset.spaces || input->format.position.offset.tabs )
		{
			word = VarTextGetEx( &out DBG_OVERRIDE );
			if( word )
			{
				SET_SPACES();
				outdata = SegAppend( outdata, word );
			}
		}
		spaces += input->format.position.offset.spaces;
		tabs += input->format.position.offset.tabs;
		//Log1( "Assuming %d spaces... ", spaces );
		for (;(character = GetUtfChar( (char const**)&tempText ) ),
                   ((tempText-tempText_) <= (int)size); ) // while not at the
                                         // end of the line.
		{
			if( elipses && character != '.' )
			{
				if( VarTextEndEx( &out DBG_OVERRIDE ) )
				{
					PTEXT word = VarTextGetEx( &out DBG_OVERRIDE );
					if( word )
					{
						SET_SPACES();
						outdata = SegAppend( outdata, word );
					}
					//else
					//	Log( "VarTextGet Failed to result." );
				}
				elipses = FALSE;
			}
			else if( elipses ) // elipses and character is . - continue
			{
				VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
				continue;
			}
		if( StrChr( filter_space, character ) )
		{
			goto is_a_space;
		}
		else if( StrChr( punctuation, character ) )
		{
			if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
			{
				outdata = SegAppend( outdata, word );
				SET_SPACES();
				VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
				word = VarTextGetEx( &out DBG_OVERRIDE );
				outdata = SegAppend( outdata, word );
			}
			else
			{
				VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
				word = VarTextGetEx( &out DBG_OVERRIDE );
				SET_SPACES();
				outdata = SegAppend( outdata, word );
			}
		}
		else switch(character)
		{
		case '\n':
			if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
			{
					SET_SPACES();
				outdata = SegAppend( outdata, word );
			}
			outdata = SegAppend( outdata, SegCreate( 0 ) ); // add a line-break packet
			break;
		case ' ':
		case 160 :// case '\xa0': // &nbsp;
			if( bSpaces )
			{
			is_a_space:
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				spaces++;
				break;
			}
				if(0) { //-V517
		case '\t':
					if( bTabs )
					{
						if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
						{
							SET_SPACES();
							outdata = SegAppend( outdata, word );
						}
						if( spaces )
						{
						//lprintf( "Input stream has mangled spaces and tabs." );
							spaces = 0; // assume that the tab takes care of appropriate spacing
						}
						tabs++;
						break;
					}
				} else if(0) { //-V517
		case '\r': // a space space character...
					if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
					{
						SET_SPACES();
						outdata = SegAppend( outdata, word );
					}
					break;
				} else if(0) { //-V517
		case '.': // handle multiple periods grouped (elipses)
				//goto NormalPunctuation;
				{
					TEXTCHAR c;
					if( ( !elipses &&
						  ( c = NextChar() ) &&
						  ( c == '.' ) ) )
						{
							if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
							{
								outdata = SegAppend( outdata, word );
								SET_SPACES();
							}
							VarTextAddCharacterEx( &out, '.' DBG_OVERRIDE );
							elipses = TRUE;
							break;
						}
						if( ( c = NextChar() ) &&
							( c >= '0' && c <= '9' ) )
						{
							// gather together as a floating point number...
							VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
							break;
						}
					}
				} else if(0) {
				case '-':  // work seperations flaming-long-sword
					if( has_minus == -1 ) {
						if( !punctuation || StrChr( punctuation, '-' ) )
							has_minus = 1;
						else
							has_minus = 0;
					}
					if( !has_minus )
					{
						VarTextAddCharacterEx( &out, '-' DBG_OVERRIDE );
						break;
					}
				case '+':
				{
					int c;
					if( has_plus == -1 ) {
						if( !punctuation || StrChr( punctuation, '+' ) )
							has_plus = 1;
						else
							has_plus = 0;
					}
					if( !has_plus )
					{
						VarTextAddCharacterEx( &out, '+' DBG_OVERRIDE );
						break;
					}
					if( ( c = NextChar() ) &&
						( c >= '0' && c <= '9' ) )
					{
						if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
						{
							outdata = SegAppend( outdata, word );
							SET_SPACES();
							// gather together as a sign indication on a number.
						}
						VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
						break;
					}
				}
//			NormalPunctuation:
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					outdata = SegAppend( outdata, word );
					SET_SPACES();
					VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
					word = VarTextGetEx( &out DBG_OVERRIDE );
					outdata = SegAppend( outdata, word );
				}
				else
				{
					VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
					word = VarTextGetEx( &out DBG_OVERRIDE );
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				break;
				}
			default:
				if( elipses )
				{
					if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
					{
						outdata = SegAppend( outdata, word );
						SET_SPACES();
					}
					elipses = FALSE;
				}
				VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
				break;
			}
		}
		input=NEXTLINE(input);
	}

	if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) ) // any generic outstanding data?
	{
		outdata = SegAppend( outdata, word );
		SET_SPACES();
	}

	SetStart(outdata);

	VarTextEmptyEx( &out DBG_OVERRIDE );

	return(outdata);
}



PTEXT burstEx( PTEXT input DBG_PASS )
// returns a TEXT list of parsed data
{
//#define DBG_OVERRIDE DBG_SRC
//#define DBG_OVERRIDE DBG_RELAY
	/* takes a line of input and creates a line equivalent to it, but
		burst into its block pieces.*/
	VARTEXT out;
	PTEXT outdata=(PTEXT)NULL,
			word;
	TEXTSTR tempText, tempText_;

	size_t size;

	TEXTRUNE character;
	uint32_t elipses = FALSE,
		spaces = 0, tabs = 0;

	if (!input)		  // if nothing new to process- return nothing processed.
		return((PTEXT)NULL);

	VarTextInitEx( &out DBG_OVERRIDE );

	while (input)  // while there is data to process...
	{
		if( input->flags & TF_INDIRECT )
		{

			word = VarTextGetEx( &out DBG_OVERRIDE );
			if( word )
			{
				SET_SPACES();
				outdata = SegAppend( outdata, word );
			}
			outdata = SegAppend( outdata, burst( GetIndirect( input ) ) );
			input = NEXTLINE( input );
			continue;
		}
		tempText_ = tempText = GetText(input);  // point to the data to process...
		size = GetTextSize(input);
		if( input->format.position.offset.spaces || input->format.position.offset.tabs )
		{
			word = VarTextGetEx( &out DBG_OVERRIDE );
			if( word )
			{
				SET_SPACES();
				outdata = SegAppend( outdata, word );
			}
		}
		spaces += input->format.position.offset.spaces;
		tabs += input->format.position.offset.tabs;
		//Log1( "Assuming %d spaces... ", spaces );
		for (;(character = GetUtfChar( (char const**)&tempText ) ),
		             ((tempText-tempText_) <= (int)size); ) // while not at the
		                                      // end of the line.
		{
			if( elipses && character != '.' )
			{
				if( VarTextEndEx( &out DBG_OVERRIDE ) )
				{
					PTEXT word = VarTextGetEx( &out DBG_OVERRIDE );
					if( word )
					{
						SET_SPACES();
						outdata = SegAppend( outdata, word );
					}
					//else
					//	Log( "VarTextGet Failed to result." );
				}
				elipses = FALSE;
			}
			else if( elipses ) // elipses and character is . - continue
			{
				VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
				continue;
			}

			switch(character)
			{
			case '\n':
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				outdata = SegAppend( outdata, SegCreate( 0 ) ); // add a line-break packet
				break;
			case ' ':
			case 160 :// '\xa0': // nbsp
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				spaces++;
				break;
			case '\t':
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				if( spaces )
				{
				//lprintf( "Input stream has mangled spaces and tabs." );
					spaces = 0;
				}
				tabs++;
				break;
			case '\r': // a space space character...
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				break;
			case '.': // handle multiple periods grouped (elipses)
				//goto NormalPunctuation;
				{
					TEXTCHAR c;
					if( ( !elipses &&
							( c = NextChar() ) &&
							( c == '.' ) ) )
					{
						if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
						{
							outdata = SegAppend( outdata, word );
							SET_SPACES();
						}
						VarTextAddCharacterEx( &out, '.' DBG_OVERRIDE );
						elipses = TRUE;
						break;
					}
					if( ( c = NextChar() ) &&
						 ( c >= '0' && c <= '9' ) )
					{
						// gather together as a floating point number...
						VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
						break;
					}
				}
			case '-':  // work seperations flaming-long-sword
			case '+':
				{
					int c;
					if( ( c = NextChar() ) &&
						( c >= '0' && c <= '9' ) )
					{
						if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
						{
							outdata = SegAppend( outdata, word );
							SET_SPACES();
						}
						// gather together as a sign indication on a number.
						VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
						break;
					}
				}
			case '\'': // single quote bound
			case '\"': // double quote bound
			case '\\': // escape next thingy... unusable in c processor

			case '(': // expression bounders
			case '{':
			case '[':
			case '<':

			case ')': // expression closers
			case '}':
			case ']':
			case '>':

			case ':':  // internet addresses
			case '@':  // email addresses
			case '%':
			case '/':
			case ',':
			case ';':
			case '!':
			case '?':
			case '=':
			case '*':
			case '&':
			case '$':
			case '^':
			case '~':
			case '#':
			case '`':
//			NormalPunctuation:
				if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
				{
					outdata = SegAppend( outdata, word );
					SET_SPACES();
					VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
					word = VarTextGetEx( &out DBG_OVERRIDE );
					outdata = SegAppend( outdata, word );
				}
				else
				{
					VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
					word = VarTextGetEx( &out DBG_OVERRIDE );
					SET_SPACES();
					outdata = SegAppend( outdata, word );
				}
				break;

			default:
				if( elipses )
				{
					if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
					{
						outdata = SegAppend( outdata, word );
						SET_SPACES();
					}
					elipses = FALSE;
				}
				VarTextAddRuneEx( &out, character, FALSE DBG_OVERRIDE );
				break;
			}
		}
		input=NEXTLINE(input);
	}

	if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) ) // any generic outstanding data?
	{
		outdata = SegAppend( outdata, word );
		SET_SPACES();
	}

	SetStart(outdata);

	VarTextEmptyEx( &out DBG_OVERRIDE );

	return(outdata);
}

//---------------------------------------------------------------------------
#undef LineLengthExx
size_t LineLengthExx( PTEXT pt, LOGICAL bSingle, PTEXT pEOL )
{
	return LineLengthExEx( pt, bSingle, 8, pEOL );
}

size_t LineLengthExEx( PTEXT pt, LOGICAL bSingle, int nTabsize, PTEXT pEOL )
{
	int	TopSingle = bSingle;
	PTEXT pStack[32];
	int	nStack;
	int	skipspaces = ( PRIORLINE(pt) != NULL );
	size_t length = 0;
	nStack = 0;
	while( pt )
	{
		if( pt->flags & TF_BINARY )
		{
			pt = NEXTLINE( pt );
			if( bSingle )
				break;
			continue;
		}

		if( !(pt->flags & ( IS_DATA_FLAGS | TF_INDIRECT)) &&
			 !pt->data.size
		  )
		{
			if( pEOL )
				length += pEOL->data.size;
			else
				length += 2; // full binary \r\n insertion assumed
		}
		else
		{
			if( skipspaces )
				skipspaces = FALSE;
			else
			{
				if( !(pt->flags & (TF_FORMATABS|TF_FORMATREL)) )
					length += GetSegmentSpace( pt, length, nTabsize ); // not-including NULL.
			}

			if( pt->flags&TF_INDIRECT )
			{
				bSingle = FALSE; // will be restored when we get back to top seg.
				pStack[nStack++] = pt;
				pt = GetIndirect( pt );
				//if( nStack >= 32 )
				//	DebugBreak();
				continue;
			}
			else
				length += GetTextSize( pt ); // not-including NULL.

stack_resume:
			if( pt->flags&TF_TAG )
				length += 2;
			if( pt->flags&TF_PAREN )
				length += 2;
			if( pt->flags&TF_BRACE )
				length += 2;
			if( pt->flags&TF_BRACKET )
				length += 2;
			if( pt->flags&TF_QUOTE )
				length += 2;
			if( pt->flags&TF_SQUOTE )
				length += 2;

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

#undef LineLengthEx
INDEX LineLengthEx( PTEXT pt, LOGICAL bSingle )
{
	return LineLengthExx( pt, bSingle, NULL );
}

//---------------------------------------------------------------------------

// attempts to build a solitary line segment from the text passed
// however, if there are color changes, or absolute position changes
// this cannot work... and it must provide multiple pieces...

#undef BuildLineExx
PTEXT BuildLineExx( PTEXT pt, LOGICAL bSingle, PTEXT pEOL DBG_PASS )
{
	return BuildLineExEx( pt, bSingle, 8, pEOL DBG_RELAY );
}

PTEXT BuildLineExEx( PTEXT pt, LOGICAL bSingle, int nTabsize, PTEXT pEOL DBG_PASS )
{
	TEXTSTR buf;
	int	TopSingle = bSingle;
	PTEXT pStack[32];
	int	nStack, firstadded;
	int	skipspaces = ( PRIORLINE(pt) != NULL );
	PTEXT pOut;
	uintptr_t ofs;

	{
		INDEX len;
		len = LineLengthExx( pt,bSingle,pEOL );
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
		if( pt->flags & TF_BINARY )
		{
			pt = NEXTLINE( pt );
			if( bSingle )
				break;
			continue;
		}
		// test color fields vs PRIOR_COLOR
		// if either the color IS the prior color - OR the value IS PRIOR_COLOR
		// then they can still be collapsed... DEFAULT_COLOR MAY be prior color
		// but there's no real telling... default is more like after a
		// attribute reset occurs...
		if( firstadded )
		{
			pOut->format.flags.foreground = pt->format.flags.foreground;
			pOut->format.flags.background = pt->format.flags.background;
			firstadded = FALSE;
		}
		else
		{
			if( ( !pt->format.flags.prior_foreground &&
				  !pt->format.flags.default_foreground &&
					pt->format.flags.foreground != pOut->format.flags.foreground ) ||
				 ( !pt->format.flags.prior_background &&
				  !pt->format.flags.default_background &&
					pt->format.flags.background != pOut->format.flags.background )
			  )
			{
				PTEXT pSplit;
				// ofs is the next valid character position....
				//Log( "Changing segment's color..." );
				if( ofs )
				{
					pSplit = SegSplitEx( &pOut, ofs DBG_RELAY );
					if( !pSplit )
					{
						lprintf( "Line was shorter than offset: %" _size_f " vs %" _PTRSZVALfs "", GetTextSize( pOut ), ofs );
					}
					pOut = NEXTLINE( pSplit );
					// new segments takes on the new attributes...
					pOut->format.flags.foreground = pt->format.flags.foreground;
					pOut->format.flags.background = pt->format.flags.background;
						//Log2( "Split at %d result %d", ofs, GetTextSize( pOut ) );
						buf = GetText( pOut );
					ofs = 0;
				}
				else
				{
					pOut->format.flags.foreground = pt->format.flags.foreground;
					pOut->format.flags.background = pt->format.flags.background;
				}
			}
		}

		if( !(pt->flags& (TF_INDIRECT|IS_DATA_FLAGS)) &&
			 !pt->data.size
		  )
		{
			if( pEOL )
			{
				MemCpy( buf + ofs, pEOL->data.data, sizeof( TEXTCHAR )*(pEOL->data.size + 1) );
				ofs += pEOL->data.size;
			}
			else
			{
				buf[ofs++] = '\r';
				buf[ofs++] = '\n';
			}
		}
		else
		{
			if( skipspaces )
			{
				skipspaces = FALSE;
			}
			else if( !(pt->flags & (TF_FORMATABS|TF_FORMATREL)) )
			{
				size_t spaces = GetSegmentSpace( pt, ofs, nTabsize );
				// else we cannot collapse into single line (similar to colors.)
				while( spaces-- )
				{
					buf[ofs++] = ' ';
				}
			}

			// at this point spaces before tags, and after tags
			// which used to be expression level parsed are not
			// reconstructed correctly...
			if( pt->flags&TF_TAG )
				buf[ofs++] = '<';
			if( pt->flags&TF_PAREN )
				buf[ofs++] = '(';
			if( pt->flags&TF_BRACE )
				buf[ofs++] = '{';
			if( pt->flags&TF_BRACKET )
				buf[ofs++] = '[';
			if( pt->flags&TF_QUOTE )
				buf[ofs++] = '\"';
			if( pt->flags&TF_SQUOTE )
				buf[ofs++] = '\'';

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
				MemCpy( buf+ofs, GetText( pt ), sizeof( TEXTCHAR) * (len = GetTextSize( pt ))+1 );
					ofs += len;
			}

stack_resume:
			if( pt->flags&TF_SQUOTE )
				buf[ofs++] = '\'';
			if( pt->flags&TF_QUOTE )
				buf[ofs++] = '\"';
			if( pt->flags&TF_BRACKET )
				buf[ofs++] = ']';
			if( pt->flags&TF_BRACE )
				buf[ofs++] = '}';
			if( pt->flags&TF_PAREN )
				buf[ofs++] = ')';
			if( pt->flags&TF_TAG )
				buf[ofs++] = '>';
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

#undef BuildLineEx
PTEXT BuildLineEx( PTEXT pt, int bSingle DBG_PASS )
{
	return BuildLineExx( pt, bSingle, FALSE DBG_RELAY );
}


PTEXT FlattenLine( PTEXT pLine )
{
	 PTEXT pCur, p;
	 pCur = pLine;
	 // all indirected segments get promoted to
	 // the first level...
	 while( pCur )
	 {
		  if( pCur->flags & TF_STATIC )
		  {
			  p = SegDuplicate( pCur );
			  if( p )
			  {
				  SegSubst( pCur, p );
				  if( pCur == pLine )
					  pLine = p;
				  LineReleaseEx( pCur DBG_SRC );
				  pCur = p;
			  }
			  else
			  {
					PTEXT next = NEXTLINE( pCur );
					SegGrab( pCur );
					LineRelease( pCur );
					pCur = next;
					continue;
			  }
		  }
		  if( pCur->flags & TF_INDIRECT )
		  {
				if( pCur->flags & TF_DEEP )
				{
					 p = FlattenLine( GetIndirect( pCur ) );
					 pCur->flags &= ~TF_DEEP;
				}
				else
				{
					 p = TextDuplicate( GetIndirect( pCur ), FALSE );
				}
				if( p )
				{
					SegSubst( pCur, p );
					if( pCur == pLine )
						pLine = p;
					p->flags |= pCur->flags & (~(TF_INDIRECT|TF_DEEP));
					LineReleaseEx( pCur DBG_SRC );
					pCur = p;
				}
				else
				{
					PTEXT next = NEXTLINE( pCur );
					SegGrab( pCur );
					LineRelease( pCur );
					pCur = next;
				}
				continue;

		  }
		  pCur = NEXTLINE( pCur );
	 }
	 return pLine;
}

//----------------------------------------------------------------------------

POINTER GetApplicationPointer( PTEXT text )
{
	// okay indirects up to application data are okay.
	while( ( text->flags & TF_INDIRECT ) && !(text->flags & TF_APPLICATION) )
		return GetApplicationPointer( (PTEXT)text->data.size );
	if( text->flags & TF_APPLICATION )
		return (POINTER)text->data.size;
	return NULL;
}

//----------------------------------------------------------------------------

void SetApplicationPointer( PTEXT text, POINTER p)
{
	// sets only this segment.
	if( text )
	{
		text->flags |= TF_APPLICATION;
		text->data.size = (uintptr_t)p;
	}
}

//----------------------------------------------------------------------------

void RegisterTextExtension( uint32_t flags, PTEXT(CPROC*TextOf)(uintptr_t,POINTER), uintptr_t psvData)
{
	PTEXT_EXTENSION pte = (PTEXT_EXTENSION)Allocate( sizeof( TEXT_EXTENSION ) );
	pte->bits = flags;
	pte->TextOf = TextOf;
	pte->psvData = psvData;
	AddLink( &pTextExtensions, pte );
#if 0
	if( text && ( text->flags & TF_APPLICATION ) )
	{
		INDEX idx;
		PTEXT_EXENSTION pte;
		LIST_FORALL( pTextExtension, idx, PTEXT_EXTENSION, pte )
		{
			if( pte->flags & text->flags )
			{
				text = pte->TextOf( text );
				break;
			}
		}
	}
#endif
	return;
}

//---------------------------------------------------------------------------

int TextIs( PTEXT pText, CTEXTSTR string )
{
	CTEXTSTR data = GetText( pText );
	if( data )
		return !StrCmp( data, string );
	return 0;
}

//---------------------------------------------------------------------------

int TextLike( PTEXT pText, CTEXTSTR string )
{
	CTEXTSTR data = GetText( pText );
	if( data )
		return !StrCaseCmp( data, string );
	return 0;
}

//---------------------------------------------------------------------------

int TextSimilar( PTEXT pText, CTEXTSTR string )
{
	CTEXTSTR data = GetText( pText );
	if( data )
	{
		size_t len1 = data ? StrLen( data ) : 0;
		size_t len2 = string ? StrLen( string ) : 0;
		return !StrCaseCmpEx( data, string, textmin( len1, len2 ) );
	}
	return 0;
}
//---------------------------------------------------------------------------

int SameText( PTEXT l1, PTEXT l2 )
{
	CTEXTSTR d1 = GetText( l1 );
	CTEXTSTR d2 = GetText( l2 );
	if( d1 && d2 )
		return StrCmp( d1, d2 );
	else if( d1 )
		return 1;
	else if( d2 )
		return -1;
	return 0;
}
//---------------------------------------------------------------------------

int LikeText( PTEXT l1, PTEXT l2 )
{
	CTEXTSTR d1 = GetText( l1 );
	size_t len1 = d1 ? StrLen( d1 ) : 0;
	CTEXTSTR d2 = GetText( l2 );
	size_t len2 = d2 ? StrLen( d2 ) : 0;

	if( d1 && d2 )
		return StrCaseCmpEx( d1, d2, textmin( len1, len2 ) );
	else if( d1 )
		return 1;
	else if( d2 )
		return -1;
	return 0;
}

//---------------------------------------------------------------------------

int CompareStrings( PTEXT pt1, int single1
                  , PTEXT pt2, int single2
                  , int bExact )
{
	while( pt1 && pt2 )
	{
		while( pt1 &&
				 pt1->flags && ( pt1->flags & TF_BINARY ) )
			pt1 = NEXTLINE( pt1 );
		while( pt2 &&
				 pt2->flags && ( pt2->flags & TF_BINARY ) )
			pt2 = NEXTLINE( pt2 );
		if( !pt1 && pt2 )
			return FALSE;
		if( pt1 && !pt2 )
			return FALSE;
		if( bExact )
		{
			if( SameText( pt1, pt2 ) != 0 )
				return FALSE;
		}
		else
		{
			// Like returns string compare function literal...
			if( LikeText( pt1, pt2 ) != 0 )
				return FALSE;
		}
		if( !single1 )
		{
			pt1 = NEXTLINE( pt1 );
			if( pt1 &&
				 !GetTextSize( pt1 ) && !(pt1->flags & IS_DATA_FLAGS))
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

//--------------------------------------------------------------------------

int64_t IntCreateFromTextRef( CTEXTSTR *p_ )
{
	CTEXTSTR p = p_[0];
	int s;
	int begin;
	int64_t num;
	LOGICAL altBase = FALSE;
	LOGICAL altBase2 = FALSE;
	int64_t base = 10;

	if( !p )
		return 0;
	//if( pText->flags & TF_INDIRECT )
	//   return IntCreateFromSeg( GetIndirect( pText ) );

	s = 0;
	num = 0;
	begin = TRUE;
	while( *p )
	{
		if( *p == '.' )
			break;
		else if( *p == '+' )
		{
		}
		else if( *p == '-' && begin)
		{
			s++;
		}
		else if( *p < '0' || *p > '9' )
		{
			if( !altBase2 ) {
				if( *p == 'x' ) { altBase2 = TRUE; base = 16; }
				else if( *p == 'o' ) { altBase2 = TRUE; base = 8; }
				else if( *p == 'b' ) { altBase2 = TRUE; base = 2; }
				else break;
			} else {
				if( base > 10 ) {
					if( *p >= 'a' && *p <= 'f' ) {
						num *= base;
						num += *p - 'a' + 10;
					}
					else if( *p >= 'A' && *p <= 'F' ) {
						num *= base;
						num += *p - 'A' + 10;
					}
					else break;
				}
				else break;
			}
		}
		else
		{
			if( ( !altBase ) && (*p == '0') ) { altBase = TRUE; base = 10; }
			else { if( (*p - '0') >= base ) { break; } altBase = TRUE; }
			num *= base;
			num += *p - '0';
		}
		begin = FALSE;
		p++;
	}
	p_[0] = p;
	if( s & 1 )
		num *= -1;
	return num;
}

//--------------------------------------------------------------------------

int64_t IntCreateFromText( CTEXTSTR p )
{
	return IntCreateFromTextRef( &p );
}

//--------------------------------------------------------------------------

int64_t IntCreateFromSeg( PTEXT pText )
{
	CTEXTSTR p;
	p = GetText( pText );
	if( !pText || !p )
		return FALSE;
	if( pText->flags & TF_INDIRECT )
		return IntCreateFromSeg( GetIndirect( pText ) );
	return IntCreateFromText( p );
}

//--------------------------------------------------------------------------

double FloatCreateFromText( CTEXTSTR p, CTEXTSTR *vp )
{
	return strtod( p, (char **)vp );
	int s, begin, bDec = FALSE;
	double num;
	double base = 1;
	double temp;
	if( !p )
	{
		if( vp )
			(*vp) = p;
		return 0;
	}
	s = 0;
	num = 0;
	begin = TRUE;
	while( *p )
	{
		if( *p == '-' && begin )
		{
			s++;
		}
		else if( *p < '0' || *p > '9' )
		{
			if( *p == '.' )
			{
				bDec = TRUE;
				base = 0.1;
			}
			else
				break;
		}
		else
		{
			if( bDec )
			{
				temp = *p - '0';
				num += base * temp;
				base /= 10;
			}
			else
			{
				num *= 10;
				num += *p - '0';
			}
		}
		begin = FALSE;
		p++;
	}
	if( vp )
		(*vp) = p;
	if( s )
		num *= -1;
	return num;
}

//--------------------------------------------------------------------------

double FloatCreateFromSeg( PTEXT pText )
{
	CTEXTSTR p;
	p = GetText( pText );
	if( !p )
		return FALSE;
	return FloatCreateFromText( p, NULL );
}

//--------------------------------------------------------------------------

// if bUseAll - all segments must be part of the number
// otherwise, only as many segments as are needed for the number are used...
int IsSegAnyNumberEx( PTEXT *ppText, double *fNumber, int64_t *iNumber, int *bIntNumber, int bUseAll )
{
	CTEXTSTR pCurrentCharacter = NULL;
	PTEXT pBegin;
	PTEXT pText = *ppText;
	int decimal_count, s, begin = TRUE, digits;
	// remember where we started...

	// if the first segment is indirect, collect it and only it
	// as the number... making indirects within a number what then?
	if( pText->flags & TF_INDIRECT )
	{
		int status;
		PTEXT pTemp = GetIndirect( pText );
		if( pTemp
			&& (status = IsSegAnyNumberEx( &pTemp, fNumber, iNumber, bIntNumber, TRUE )) )
		{
			// step to next token - so we toss just this
			// one indirect statement.
			if( fNumber || iNumber )
			{
				// if resulting with a number, then step the text...
				(*ppText) = NEXTLINE( pText );
			}
			return status;
		}
		// not a number....
		return FALSE;
	}
	pBegin = pText;
	decimal_count = 0;
	s = 0;
	digits = 0;
	while( pText )
	{
		// at this point... is this really valid?
		if( pText->flags & TF_INDIRECT )
		{
			lprintf( "Encountered indirect segment gathering number, stopping." );
			break;
		}

		if( !begin &&
			( pText->format.position.offset.spaces || pText->format.position.offset.tabs ) )
		{
			// had to continue with new segment, but it had spaces so stop now
			break;
		}

		pCurrentCharacter = GetText( pText );
		while( pCurrentCharacter && *pCurrentCharacter )
		{
			if( *pCurrentCharacter == '.' )
			{
				if( !decimal_count )
					decimal_count++;
				else
					break;
			}
			else if( ((*pCurrentCharacter) == '-') && begin)
			{
				s++;
			}
			else if( ((*pCurrentCharacter) < '0') || ((*pCurrentCharacter) > '9') )
			{
				if( digits && ( pCurrentCharacter == GetText( pText ) ) )
				{
					pCurrentCharacter = GetText( PRIORLINE( pText ) );
					while( pCurrentCharacter[0] )
					{
						// if the number ended in a decimal, it can qualify as an integer
						if( pCurrentCharacter[0] == '.' && !pCurrentCharacter[1] )
							decimal_count--;
						pCurrentCharacter++;
					}
					pText = NULL;
				}
				break;
			}
			else
				digits++;
			begin = FALSE;
			pCurrentCharacter++;
		}
		// invalid character - stop, we're to abort.
		if( *pCurrentCharacter )
			break;
		pText = NEXTLINE( pText );
	} //while( pText );
	if( bUseAll && pText )
		// it's not a number, cause we didn't use all segments to get one
		return FALSE;
	if( *pCurrentCharacter || ( decimal_count > 1 ) || !digits )
	{
		// didn't collect enough meaningful info to be a number..
		// or information in this state is
		return FALSE;
	}
	// yeah it was a number, update the incoming pointer...
	if( fNumber || iNumber )
	{
		// if resulting with a number, then step the text...
		(*ppText) = pText;
	}
	if( decimal_count == 1 )
	{
		if( fNumber )
			(*fNumber) = FloatCreateFromSeg( pBegin );
		if( bIntNumber )
			(*bIntNumber) = 0;
		return 2; // return specifically it's a floating point number
	}
	if( iNumber )
		(*iNumber) = IntCreateFromSeg( pBegin );
	if( bIntNumber )
		(*bIntNumber) = 1;
	return 1; // return yes, and it's an int number
}

//---------------------------------------------------------------------------

//#define VERBOSE_DEBUG_VARTEXT
//---------------------------------------------------------------------------
#define COLLECT_LEN 4096

void VarTextInitEx( PVARTEXT pvt DBG_PASS )
{
	pvt->commit = NULL;
	pvt->collect = SegCreateEx( COLLECT_LEN DBG_RELAY );
	pvt->collect_text = GetText( pvt->collect );
#ifdef VERBOSE_DEBUG_VARTEXT
	Log( "Resetting collect_used (init)" );
#endif
	pvt->collect_used = 0;
	pvt->collect_avail = COLLECT_LEN;
	pvt->expand_by = 0;
}

 PVARTEXT  VarTextCreateExEx ( uint32_t initial, uint32_t expand DBG_PASS )
{
	PVARTEXT pvt = (PVARTEXT)AllocateEx( sizeof( VARTEXT ) DBG_RELAY );

	pvt->commit = NULL;
	pvt->collect = SegCreateEx( initial DBG_RELAY );
	pvt->collect_text = GetText( pvt->collect );
	pvt->collect_used = 0;
	pvt->collect_avail = initial;
	pvt->expand_by = expand;
	return pvt;

}
//---------------------------------------------------------------------------

PVARTEXT VarTextCreateEx( DBG_VOIDPASS )
{
	PVARTEXT pvt = (PVARTEXT)AllocateEx( sizeof( VARTEXT ) DBG_RELAY );
	VarTextInitEx( pvt DBG_RELAY );
	return pvt;
}

//---------------------------------------------------------------------------

void VarTextDestroyEx( PVARTEXT *ppvt DBG_PASS )
{
	if( ppvt && *ppvt )
	{
		VarTextEmptyEx( *ppvt DBG_RELAY );
		ReleaseEx( (*ppvt) DBG_RELAY );
		*ppvt = NULL;
	}
}

//---------------------------------------------------------------------------

void VarTextEmptyEx( PVARTEXT pvt DBG_PASS )
{
	if( pvt )
	{
		size_t expand = pvt->expand_by;
		LineReleaseEx( pvt->collect DBG_RELAY );
		LineReleaseEx( pvt->commit DBG_RELAY );
		MemSet( pvt, 0, sizeof( VARTEXT ) );
		pvt->expand_by = expand;
	}
}

//---------------------------------------------------------------------------

void VarTextAddCharacterEx( PVARTEXT pvt, TEXTCHAR c DBG_PASS )
{
	if( !pvt->collect )
		VarTextInitEx( pvt DBG_RELAY );
#ifdef VERBOSE_DEBUG_VARTEXT
	Log1( "Adding character %c", c );
#endif
	if( c == '\b' )
	{
		if( pvt->collect_used )
		{
			pvt->collect_used--;
			pvt->collect_text[pvt->collect_used] = 0;
		}
	}
	else
	{
		pvt->collect_text[pvt->collect_used++] = c;
		if( pvt->collect_used >= pvt->collect_avail )
		{
			//lprintf( "Expanding segment to make sure we have room to extend...(old %d)", pvt->collect->data.size );
			pvt->collect = SegExpandEx( pvt->collect, pvt->collect_avail * 2 DBG_RELAY );
			pvt->collect_avail = pvt->collect->data.size;
			pvt->collect_text = GetText( pvt->collect );
		}
	}
}

void VarTextAddRuneEx( PVARTEXT pvt, TEXTRUNE c, LOGICAL overlong DBG_PASS )
{
	int chars;
	int n;
	char output[6];
	chars = ConvertToUTF8Ex( output, c, overlong );
	for( n = 0; n < chars; n++ )
		VarTextAddCharacterEx( pvt, output[n] DBG_RELAY );
}

//---------------------------------------------------------------------------

void VarTextAddDataEx( PVARTEXT pvt, CTEXTSTR block, size_t length DBG_PASS )
{
	if( !pvt->collect )
		VarTextInitEx( pvt DBG_RELAY );
#ifdef VERBOSE_DEBUG_VARTEXT
	Log1( "Adding character %c", c );
#endif
	{
		uint32_t n;
		for( n = 0; n < length; n++ )
		{
			if( !block[n] && ( length == VARTEXT_ADD_DATA_NULTERM ) )
				break;
			pvt->collect_text[pvt->collect_used++] = block[n];
			if( pvt->collect_used >= pvt->collect_avail )
			{
				//lprintf( "Expanding segment to make sure we have room to extend...(old %d)", pvt->collect->data.size );
				pvt->collect = SegExpandEx( pvt->collect, pvt->collect_avail * 2 + COLLECT_LEN DBG_RELAY );
				pvt->collect_avail = pvt->collect->data.size;
				pvt->collect_text = GetText( pvt->collect );
			}
		}
	}
}

//---------------------------------------------------------------------------

LOGICAL VarTextEndEx( PVARTEXT pvt DBG_PASS )
{
	if( pvt && pvt->collect_used ) // otherwise ofs will be 0...
	{
		PTEXT segs= SegSplitEx( &pvt->collect, pvt->collect_used DBG_RELAY );
		//lprintf( "End collect at %d %d", pvt->collect_used, segs?segs->data.size:pvt->collect->data.size );
		if( !segs )
		{
			segs = pvt->collect;
		}

		//Log1( "Breaking collection adding... %s", GetText( segs ) );
		// so now the remaining buffer( if any )
		// is assigned to collect into.
		// This results in...

		pvt->collect = NEXTLINE( segs );
		if( !pvt->collect ) // used all of the line...
		{
#ifdef VERBOSE_DEBUG_VARTEXT
			Log( "Starting with new buffers " );
#endif
			VarTextInitEx( pvt DBG_RELAY );
		}
		else
		{
 			//Log1( "Remaining buffer is %d", GetTextSize( pvt->collect ) );
			SegBreak( pvt->collect );
			pvt->collect_text = GetText( pvt->collect );
#ifdef VERBOSE_DEBUG_VARTEXT
			Log( "resetting collect_used after split" );
#endif
			pvt->collect_avail -= pvt->collect_used;
			pvt->collect_used = 0;
		}
		pvt->commit = SegAppend( pvt->commit, segs );
		return 1;
	}
	if( pvt && pvt->commit )
		return 1;
	return 0;
}

//---------------------------------------------------------------------------

PTEXT VarTextGetEx( PVARTEXT pvt DBG_PASS )
{
	if( !pvt )
	{
#ifdef VERBOSE_DEBUG_VARTEXT
		lprintf( DBG_FILELINEFMT "Get Text failed - no PVT." DBG_RELAY );
#endif
		return NULL;
	}
#ifdef VERBOSE_DEBUG_VARTEXT
	lprintf( DBG_FILELINEFMT "Grabbing the text from %p..." DBG_RELAY, pvt );
#endif
	if( VarTextEndEx( pvt DBG_RELAY ) )
	{
		PTEXT result = pvt->commit;
		pvt->commit = NULL;
		return result;
	}
	return NULL;
}

//---------------------------------------------------------------------------

 PTEXT  VarTextPeekEx ( PVARTEXT pvt DBG_PASS )
{
	if( !pvt )
		return NULL;
	if( pvt && pvt->collect_used ) // otherwise ofs will be 0...
	{
		SetTextSize( pvt->collect, pvt->collect_used );
		//VarTextAddCharacterEx( pvt, 0 DBG_RELAY );
		return pvt->collect;
	}
	return NULL;
}

//---------------------------------------------------------------------------

void VarTextExpandEx( PVARTEXT pvt, INDEX size DBG_PASS)
{
	pvt->collect = SegExpandEx( pvt->collect, size DBG_RELAY );
	pvt->collect_text = GetText( pvt->collect );
	pvt->collect_avail += size;
}

//---------------------------------------------------------------------------

INDEX VarTextLength( PVARTEXT pvt )
{
	//Log1( "Length is : %d", pvt->collect_used );
	if( pvt )
		return pvt->collect_used;
	return 0;
}


//---------------------------------------------------------------------------

INDEX vvtprintf( PVARTEXT pvt, CTEXTSTR format, va_list args )
{
	INDEX len;
#if ( defined( UNDER_CE ) || defined( _WIN32 ) ) && !defined( MINGW_SUX )// this might be unicode...
#  ifdef USE_UCRT
	{
		va_list tmp_args;
		va_copy( tmp_args, args );
		// len returns number of characters (not NUL)
		len = vsnprintf( NULL, 0, format
							, args
							);
		if( !len ) // nothign to add... we'll get stuck looping if this is not checked.
			return 0;
		va_end( tmp_args );
		// allocate +1 for length with NUL
		if( ((uint32_t)len+1) >= (pvt->collect_avail-pvt->collect_used) )
		{
			// expand when we need more room.
			VarTextExpand( pvt, ((len+1)<pvt->expand_by)?pvt->expand_by:(len+1+pvt->expand_by)  );
		}
#ifdef VERBOSE_DEBUG_VARTEXT
		Log3( "Print Length: %d into %d after %s", len, pvt->collect_used, pvt->collect_text );
#endif
		// include NUL in the limit of characters able to print...
		vsnprintf( pvt->collect_text + pvt->collect_used, len+1, format, args );
	}
#  else
	int tries = 0;
	while( 1 )
	{
		size_t destlen;
		if( pvt->collect_text )
		{
			len = StringCbVPrintf ( pvt->collect_text + pvt->collect_used
									, ((destlen = pvt->collect_avail - pvt->collect_used) * sizeof( TEXTCHAR ))
									, format, args );
		}
		else
			len = STRSAFE_E_INSUFFICIENT_BUFFER;
		if( len == STRSAFE_E_INSUFFICIENT_BUFFER )
		{
			tries++;
			if( tries == 100 )
			{
				lprintf( "Single buffer expanded more then %d", tries * ( (pvt->expand_by)?pvt->expand_by:(16384+pvt->expand_by) ) );
				return 0; // didn't add any
			}
			VarTextExpand( pvt, (pvt->expand_by)?pvt->expand_by:(16384)  );
			continue;
		}
		len = StrLen( pvt->collect_text + pvt->collect_used );
		pvt->collect_used += len;
		break;
	}
	return len;
#  endif
#elif defined( __GNUC__ ) && !defined( _WIN32 )
	{
		va_list tmp_args;
		va_copy( tmp_args, args );
		// len returns number of characters (not NUL)
		len = vsnprintf( NULL, 0, format
#  ifdef __GNUC__
							, tmp_args
#  else
							, args
#  endif
							);
		if( !len ) // nothign to add... we'll get stuck looping if this is not checked.
			return 0;
#  ifdef __GNUC__
		va_end( tmp_args );
#  endif
		// allocate +1 for length with NUL
		if( ((uint32_t)len+1) >= (pvt->collect_avail-pvt->collect_used) )
		{
			// expand when we need more room.
			VarTextExpand( pvt, ((len+1)<pvt->expand_by)?pvt->expand_by:(len+1+pvt->expand_by)  );
		}
#  ifdef VERBOSE_DEBUG_VARTEXT
		Log3( "Print Length: %d into %d after %s", len, pvt->collect_used, pvt->collect_text );
#  endif
		// include NUL in the limit of characters able to print...
		vsnprintf( pvt->collect_text + pvt->collect_used, len+1, format, args );
	}
#elif defined( __WATCOMC__ )
	{
		int destlen;
		va_list _args;
		_args[0] = args[0];
		do {
#  ifdef VERBOSE_DEBUG_VARTEXT
			Log2( "Print Length: ofs %d after %s"
				 , pvt->collect_used
				 , pvt->collect_text );
#  endif
			args[0] = _args[0];
			//va_start( args, format );
			len = vsnprintf( pvt->collect_text + pvt->collect_used
								, destlen = pvt->collect_avail - pvt->collect_used
								, format, args );
			if( !len ) // nothign to add... we'll get stuck looping if this is not checked.
				return 0;
#  ifdef VERBOSE_DEBUG_VARTEXT
			lprintf( "result of vsnprintf: %d(%d) \'%s\' (%s)"
					 , len, destlen
					 , pvt->collect_text
					 , format );
#  endif
			if( len >= destlen )
			{
				// vsnwprintf() for NULL and 0 length returns -1
				// so, make length be something larger than -1, and keep expanding by that much.
				if( len == -1 )
					len = 256;
				VarTextExpand( pvt, len + pvt->expand_by );
			}
		} while( len >= destlen );
	}
#else
	// uhmm not sure what state this is then...
	{
		do {
			len = vsnprintf( pvt->collect_text + pvt->collect_used
								, pvt->collect_avail - pvt->collect_used
								, format, args );
			if( len < 0 )
				VarTextExpand( pvt, pvt->expand_by?pvt->expand_by:4096 );
			//					 VarTextExpandEx( pvt, 32 DBG_SRC );
		} while( len < 0 );
		//Log1( "Print Length: %d", len );
	}
#endif
#ifdef VERBOSE_DEBUG_VARTEXT
	Log2( "used: %d plus %d", pvt->collect_used , len );
#endif
	pvt->collect_used += len;
	return len;
}
//---------------------------------------------------------------------------

INDEX vtprintfEx( PVARTEXT pvt , CTEXTSTR format, ... )
{
	va_list args;
	va_start( args, format );
	return vvtprintf( pvt, format, args );
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//
// PTEXT DumpText( PTEXT somestring )
//	 PTExT (single data segment with full description \r in text)
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static CTEXTSTR Ops[] = {
	"FORMAT_OP_CLEAR_END_OF_LINE",
	"FORMAT_OP_CLEAR_START_OF_LINE",
	"FORMAT_OP_CLEAR_LINE ",
	"FORMAT_OP_CLEAR_END_OF_PAGE",
	"FORMAT_OP_CLEAR_START_OF_PAGE",
	"FORMAT_OP_CLEAR_PAGE",
	"FORMAT_OP_CONCEAL"
	  , "FORMAT_OP_DELETE_CHARS" // background is how many to delete.
	  , "FORMAT_OP_SET_SCROLL_REGION" // format.x, y are start/end of region -1,-1 clears.
	  , "FORMAT_OP_GET_CURSOR" // this works as a transaction...
	  , "FORMAT_OP_SET_CURSOR" // responce to getcursor...

	  , "FORMAT_OP_PAGE_BREAK" // clear page, home page... result in page break...
	  , "FORMAT_OP_PARAGRAPH_BREAK" // break between paragraphs - kinda same as lines...
};

//---------------------------------------------------------------------------

static void BuildTextFlags( PVARTEXT vt, PTEXT pSeg )
{
	vtprintf( vt, "Text Flags: ");
	if( pSeg->flags & TF_STATIC )
		vtprintf( vt, "static " );
	if( pSeg->flags & TF_QUOTE )
		vtprintf( vt, "\"\" " );
	if( pSeg->flags & TF_SQUOTE )
		vtprintf( vt, "\'\' " );
	if( pSeg->flags & TF_BRACKET )
		vtprintf( vt, "[] " );
	if( pSeg->flags & TF_BRACE )
		vtprintf( vt, "{} " );
	if( pSeg->flags & TF_PAREN )
		vtprintf( vt, "() " );
	if( pSeg->flags & TF_TAG )
		vtprintf( vt, "<> " );
	if( pSeg->flags & TF_INDIRECT )
		vtprintf( vt, "Indirect " );
	/*
	if( pSeg->flags & TF_SINGLE )
	vtprintf( vt, "single " );
	*/
	if( pSeg->flags & TF_FORMATREL )
		vtprintf( vt, "format x,y(REL) " );
	if( pSeg->flags & TF_FORMATABS )
		vtprintf( vt, "format x,y " );
	else
		vtprintf( vt, "format spaces " );

	if( pSeg->flags & TF_COMPLETE )
		vtprintf( vt, "complete " );
	if( pSeg->flags & TF_BINARY )
		vtprintf( vt, "binary " );
	if( pSeg->flags & TF_DEEP )
		vtprintf( vt, "deep " );
#ifdef DEKWARE_APP_FLAGS
	if( pSeg->flags & TF_ENTITY )
		vtprintf( vt, "entity " );
	if( pSeg->flags & TF_SENTIENT )
		vtprintf( vt, "sentient " );
#endif
	if( pSeg->flags & TF_NORETURN )
		vtprintf( vt, "NoReturn " );
	if( pSeg->flags & TF_LOWER )
		vtprintf( vt, "Lower " );
	if( pSeg->flags & TF_UPPER )
		vtprintf( vt, "Upper " );
	if( pSeg->flags & TF_EQUAL )
		vtprintf( vt, "Equal " );
	if( pSeg->flags & TF_TEMP )
		vtprintf( vt, "Temp " );
#ifdef DEKWARE_APP_FLAGS
	if( pSeg->flags & TF_PROMPT )
		vtprintf( vt, "Prompt " );
	if( pSeg->flags & TF_PLUGIN )
		vtprintf( vt, "Plugin=%02x ", (uint8_t)(( pSeg->flags >> 26 ) & 0x3f ) );
#endif

	if( (pSeg->flags & TF_FORMATABS ) )
		vtprintf( vt, "Pos:%d,%d "
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else if( (pSeg->flags & TF_FORMATREL ) )
		vtprintf( vt, "Rel:%d,%d "
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else
		vtprintf( vt, "%d tabs %d spaces"
				  , pSeg->format.position.offset.tabs
				  , pSeg->format.position.offset.spaces
				  );

	if( pSeg->flags & TF_FORMATEX )
		vtprintf( vt, "format extended(%s) length:%d"
					  , Ops[ pSeg->format.flags.format_op
							 - FORMAT_OP_CLEAR_END_OF_LINE ]
					  , GetTextSize( pSeg ) );
	else
		vtprintf( vt, "Fore:%d Back:%d length:%d"
					, pSeg->format.flags.foreground
					, pSeg->format.flags.background
					, GetTextSize( pSeg ) );
}


PTEXT DumpText( PTEXT text )
{
	if( text )
	{
		PVARTEXT pvt = VarTextCreate();
		PTEXT textsave = text;
		while( text )
		{
			BuildTextFlags( pvt, text );
			vtprintf( pvt, "\n->%s\n", GetText( text ) );
			text = NEXTLINE( text );
		}
		textsave = VarTextGet( pvt );
		VarTextDestroy( &pvt );
		return textsave;
	}
	return NULL;
}

//---------------------------------------------------------------------------

/*
**  ASCII <=> EBCDIC conversion functions
*/
TEXTSTR ConvertAsciiEbdic( TEXTSTR text, INDEX length )
{
	static unsigned char a2e[256] = {
		0,  1,  2,  3, 55, 45, 46, 47, 22,  5, 37, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 60, 61, 50, 38, 24, 25, 63, 39, 28, 29, 30, 31,
		64, 79,127,123, 91,108, 80,125, 77, 93, 92, 78,107, 96, 75, 97,
		240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111,
		124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214,
		215,216,217,226,227,228,229,230,231,232,233, 74,224, 90, 95,109,
		121,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150,
		151,152,153,162,163,164,165,166,167,168,169,192,106,208,161,  7,
		32, 33, 34, 35, 36, 21,  6, 23, 40, 41, 42, 43, 44,  9, 10, 27,
		48, 49, 26, 51, 52, 53, 54,  8, 56, 57, 58, 59,  4, 20, 62,225,
		65, 66, 67, 68, 69, 70, 71, 72, 73, 81, 82, 83, 84, 85, 86, 87,
		88, 89, 98, 99,100,101,102,103,104,105,112,113,114,115,116,117,
		118,119,120,128,138,139,140,141,142,143,144,154,155,156,157,158,
		159,160,170,171,172,173,174,175,176,177,178,179,180,181,182,183,
		184,185,186,187,188,189,190,191,202,203,204,205,206,207,218,219,
		220,221,222,223,234,235,236,237,238,239,250,251,252,253,254,255
	};

	{
		INDEX n;
		for( n = 0; length?(n<length):text[n]; n++ )
		{
			text[n] = a2e[(unsigned)text[n]];
		}
	}
	return text;
}

/*
**  ASCII <=> EBCDIC conversion functions
*/
TEXTSTR ConvertEbcdicAscii( TEXTSTR text, INDEX length )
{

	static unsigned char e2a[256] = {
		0,  1,  2,  3,156,  9,134,127,151,141,142, 11, 12, 13, 14, 15,
		16, 17, 18, 19,157,133,  8,135, 24, 25,146,143, 28, 29, 30, 31,
		128,129,130,131,132, 10, 23, 27,136,137,138,139,140,  5,  6,  7,
		144,145, 22,147,148,149,150,  4,152,153,154,155, 20, 21,158, 26,
		32,160,161,162,163,164,165,166,167,168, 91, 46, 60, 40, 43, 33,
		38,169,170,171,172,173,174,175,176,177, 93, 36, 42, 41, 59, 94,
		45, 47,178,179,180,181,182,183,184,185,124, 44, 37, 95, 62, 63,
		186,187,188,189,190,191,192,193,194, 96, 58, 35, 64, 39, 61, 34,
		195, 97, 98, 99,100,101,102,103,104,105,196,197,198,199,200,201,
		202,106,107,108,109,110,111,112,113,114,203,204,205,206,207,208,
		209,126,115,116,117,118,119,120,121,122,210,211,212,213,214,215,
		216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,
		123, 65, 66, 67, 68, 69, 70, 71, 72, 73,232,233,234,235,236,237,
		125, 74, 75, 76, 77, 78, 79, 80, 81, 82,238,239,240,241,242,243,
		92,159, 83, 84, 85, 86, 87, 88, 89, 90,244,245,246,247,248,249,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57,250,251,252,253,254,255
	};


	{
		INDEX n;
		for( n = 0; length?(n<length):text[n]; n++ )
		{
			text[n] = e2a[(unsigned)text[n]];
		}
	}
	return text;
}
//---------------------------------------------------------------------------

#define NUM_RESERVED (sizeof(reserved_uri)/sizeof(reserved_uri[0]))
static TEXTCHAR reserved_uri[] = {'!','*','\'','(',')',';',':','@','&','=','+','$',',','/','?','#','[',']'
												  ,'<','>','~','.','"','{','}','|','\\','-','`','_','^','%',' '
												  , 0 };
static const TEXTCHAR *translated[] = { "%21","%2A","%27","%28","%29","%3B","%3A"
												,"%40","%26","%3D","%2B","%24","%2C","%2F"
												 ,"%3F","%23","%5B","%5D"
												 ,"%3C","%3E","%7E","%2E","%22","%7B","%7D","%7C","%5C","%2D","%60","%5F","%5E","%25","%20"
};

static int MeasureTextURI( CTEXTSTR text, INDEX length, int skip_slash )
{
	// compute how long it should be.
	INDEX i;
	int out_length = 0;
	for( i = 0; i < length && text[i]; i++ )
	{
		if( skip_slash && text[i] == '/' )
			out_length++;
		else if( StrChr( reserved_uri, text[i] ) )
			out_length += 3;
		else
			out_length++;
	}
	return out_length;
}

TEXTSTR ConvertTextURI( CTEXTSTR text, INDEX length, int skip_slash )
{
	int target_len = MeasureTextURI( text, length, skip_slash );
	TEXTSTR target = NewArray( TEXTCHAR, target_len + 1 );
	INDEX i;
	TEXTSTR out_pos = target;
	const TEXTCHAR *char_pos;
	for( i = 0; i < length && text[i]; i++ )
	{
		if( skip_slash && text[i] == '/' )
		{
			out_pos[0] = text[i];
			out_pos++;
		}
		else if( ( char_pos = StrChr( reserved_uri, text[i] ) ) )
		{
#ifdef __cplusplus
			sack::memory::
#endif
			StrCpyEx( out_pos, translated[char_pos - reserved_uri], target_len - ( out_pos - target ) );
			out_pos += 3;
		}
		else
		{
			out_pos[0] = text[i];
			out_pos++;
		}
	}
	out_pos[0] = 0;
	return target;
}

static int MeasureURIText( CTEXTSTR text, INDEX length )
{
	// compute how long it should be.
	INDEX i;
	int out_length = 0;
	for( i = 0; i < length && text[i]; i++ )
	{
		if( text[i] == '%' )
		{
			i += 2;
			out_length++;
		}
		else
			out_length++;
	}
	return out_length;
}

TEXTSTR ConvertURIText( CTEXTSTR text, INDEX length )
{
	int target_len = MeasureURIText( text, length );
	TEXTSTR target = NewArray( TEXTCHAR, target_len + 1 );
	INDEX i;
	TEXTSTR out_pos = target;
	for( i = 0; i < length && text[i]; i++ )
	{
		if( text[i] == '%' )
		{
			char char_byte;
			// A 41
			// a 61
			// 0 30
			char_byte = (((text[i+1] & 10)?(text[i+1]-0x30)
							  : (text[i+1] & 20)?(text[i+1]-'a'+10)
							  : (text[i+1]-'A'+10)) << 4 )
				| (((text[i+2] & 10)?(text[i+2]-0x30)
					 : (text[i+2] & 20)?(text[i+2]-'a'+10)
					 : (text[i+2]-'A'+10)) );
			out_pos[0] = char_byte;
			out_pos++;
		}
		else
		{
			out_pos[0] = text[i];
			out_pos++;
		}
	}
	out_pos[0] = 0;
	//out_pos++;
	return target;
}


//---------------------------------------------------------------------------

int ConvertToUTF16( wchar_t *output, TEXTRUNE rune )
{
	if( !( rune & 0xFFFF0000 ) )
	{
		if( rune < 0xD800 || rune >= 0xE000 )
		{
			output[0] = (wchar_t)rune;
			return 1;
		}
		else
			return 0; // invalid rune specified.
	}
	else
	{
		rune -= 0x10000;
		if( !( rune & 0xFFFFF ) )
		{
			output[0] = 0xD800 + (wchar_t)( ( rune & 0xFFC00 ) >> 10 );
			output[1] = 0xDC00 + (wchar_t)( ( rune & 0x003FF ) );
			return 2;
		}
	}
	return 0; // invalid rune.
}

int ConvertToUTF8( char *output, TEXTRUNE rune )
{
	int ch = 1;
	if( !( rune & 0xFFFFFF80 ) )
	{
		// 7 bits
		(*output++) = (char)rune;
		goto plus0;
	}
	else if( !( rune & 0xFFFFF800 ) )
	{
		// 11 bits
		(*output++) = 0xC0 | ( ( ( rune & 0x07C0 ) >> 6 ) & 0xFF );
		goto plus1;
	}
	else if( !( rune & 0xFFFF0000 ) )
	{
		// 16 bits
		(*output++) = 0xE0 | ( ( ( rune & 0xF000 ) >> 12 ) & 0xFF );
		goto plus2;
	}
	else if( !( rune & 0xFFE00000 ) )
	{
		// 21 bits
		(*output++) = 0xF0 | ( ( ( rune & 0x1C0000 ) >> 18 ) & 0xFF );
		goto plus3;
	}
	else if( !( rune & 0xFC000000 ) )
	{
		// 26 bits
		(*output++) = 0xF8 | ( ( ( rune & 0x3000000 ) >> 24 ) & 0xFF );
		goto plus4;
	}
	else if( !( rune & 0x80000000 ) )
	{
		// 31 bits
		(*output++) = 0xFC | ( ( ( rune & 0x40000000 ) >> 30 ) & 0xFF );
		//goto plus5;
	}
	// invalid rune (out of range)
//plus5:
	ch++; (*output++) = 0x80 | (((rune & 0x3F000000) >> 24) & 0xFF);
plus4:
	ch++; (*output++) = 0x80 | (((rune & 0x0FC0000) >> 18) & 0xFF);
plus3:
	ch++; (*output++) = 0x80 | (((rune & 0x03F000) >> 12) & 0xFF);
plus2:
	ch++; (*output++) = 0x80 | (((rune & 0x0FC0) >> 6) & 0xFF);
plus1:
	ch++; (*output++) = 0x80 | (rune & 0x3F);
plus0:
	return ch;
}


int ConvertToUTF8Ex( char *output, TEXTRUNE rune, LOGICAL overlong )
{
	int ch = 1;
	if( !overlong ) return ConvertToUTF8( output, rune );

	if( !(rune & 0xFFFFFF80) )
	{
		// 11 bits
		(*output++) = 0xC0 | (((rune & 0x07C0) >> 6) & 0xFF);
		goto plus1;
	}
	else if( !(rune & 0xFFFFF800) )
	{
		// 16 bits
		(*output++) = 0xE0 | (((rune & 0xF000) >> 12) & 0xFF);
		goto plus2;
	}
	else if( !(rune & 0xFFFF0000) )
	{
		// 21 bits
		(*output++) = 0xF0 | (((rune & 0x1C0000) >> 18) & 0xFF);
		goto plus3;
	}
	else if( !(rune & 0xFFE00000) )
	{
		// 26 bits
		(*output++) = 0xF8 | (((rune & 0x3000000) >> 24) & 0xFF);
		goto plus4;
	}
	else if( !(rune & 0xFC000000) )
	{
		// 31 bits
		(*output++) = 0xFC | (((rune & 0x40000000) >> 30) & 0xFF);
		goto plus5;
	}
	else if( !(rune & 0x80000000) ) {
		(*output++) = 0xFEU;
	}
	ch++; (*output++) = 0x80 | (((rune & 0xC0000000) >> 30) & 0xFF);
plus5:
	ch++; (*output++) = 0x80 | (((rune & 0x3F000000) >> 24) & 0xFF);
plus4:
	ch++; (*output++) = 0x80 | (((rune & 0x0FC0000) >> 18) & 0xFF);
plus3:
	ch++; (*output++) = 0x80 | (((rune & 0x03F000) >> 12) & 0xFF);
plus2:
	ch++; (*output++) = 0x80 | (((rune & 0x0FC0) >> 6) & 0xFF);
plus1:
	ch++; (*output++) = 0x80 | (rune & 0x3F);
//plus0:
	return ch;
}


char * WcharConvert_v2 ( const wchar_t *wch, size_t len, size_t *outlen DBG_PASS )
{
	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the
	// conversion functions such as:
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t  sizeInBytes;
	char  tmp[2];
	char	 *ch;
	char	 *_ch;
	const wchar_t *_wch = wch;
	sizeInBytes = 1; // start with 1 for the ending nul
	_ch = ch = tmp;
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			//lprintf( "wch = %04x", wch[0] );
			if( !( wch[0] & 0xFF80 ) )
			{
				//lprintf( "1 byte encode..." );
				sizeInBytes++;
			}
			else if( !( wch[0] & 0xF800 ) )
			{
				//lprintf( "2 byte encode..." );
				sizeInBytes += 2;
			}
			else if( (  ( ( wch[0] & 0xFC00 ) >= 0xD800 )
					   && ( ( wch[0] & 0xFC00 ) < 0xDC00 ) )
					 && ( ( ( wch[1] & 0xFC00 ) >= 0xDC00 )
					   && ( ( wch[1] & 0xFC00 ) < 0xE000 ) )
					 )
			{
				int longer_value = 0x10000 + ( ( ( wch[0] & 0x3ff ) << 10 ) | ( ( wch[1] & 0x3ff ) ) );
				//lprintf( "3 or 4 byte encode..." );
				if( !(longer_value & 0xFFFF0000 ) )
					sizeInBytes += 3;
				else if( ( longer_value >= 0xF0000 ) && ( longer_value < 0xF0800 ) ) // hack a way to encode D800-DFFF
					sizeInBytes += 2;
				else
					sizeInBytes += 4;
				wch++;
			}
			else
			{
				// just encode the 16 bits as it is.
				//lprintf( " 3 byte encode?" );
				sizeInBytes+= 3;
			}
			wch++;
		}
	}
	wch = _wch;
	_ch = ch = NewArray( char, sizeInBytes);
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			{
				if( !( wch[0] & 0xFF80 ) )
				{
					(*ch++) = ((unsigned char*)wch)[0];
				}
				else if( !( wch[0] & 0xFF00 ) )
				{
					//(*ch++) = ((unsigned char*)wch)[0];
					(*ch++) = 0xC0 | ( ( ((unsigned char*)wch)[1] & 0x7 ) << 2 ) | ( ( ((unsigned char*)wch)[0] ) >> 6 );
					(*ch++) = 0x80 | ( ((unsigned char*)wch)[0] & 0x3F );
				}
				else if( !( wch[0] & 0xF800 ) )
				{
					(*ch++) = 0xC0 | ( ( ((unsigned char*)wch)[1] & 0x7 ) << 2 ) | ( ( ((unsigned char*)wch)[0] ) >> 6 );
					(*ch++) = 0x80 | ( ((unsigned char*)wch)[0] & 0x3F );
				}
				else if( (  ( ( wch[0] & 0xFC00 ) >= 0xD800 )
							 && ( ( wch[0] & 0xFC00 ) < 0xDC00 ) )
						  && ( ( ( wch[1] & 0xFC00 ) >= 0xDC00 )
								&& ( ( wch[1] & 0xFC00 ) < 0xE000 ) )
					 )
				{
					uint32_t longer_value;
					longer_value = 0x10000 + ( ( ( wch[0] & 0x3ff ) << 10 ) | ( ( wch[1] & 0x3ff ) ) );
					if( ( longer_value >= 0xF0000 ) && ( longer_value < 0xF0800 ) ) // hack a way to encode D800-DFFF
					{
						longer_value = ( longer_value - 0xF0000 ) + 0xD800;
						sizeInBytes += 2;
					}
					wch++;
					if( !(longer_value & 0xFFFF ) )
					{
						// 16 bit encoding (shouldn't be hit
						(*ch++) = 0xE0 | (char)( ( longer_value >> 12 ) & 0x0F );
						(*ch++) = 0x80 | (char)( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | (char)( ( longer_value >> 0 ) & 0x3f );
					}
					else if( !( longer_value & 0xFFE00000 ) )
					{
						// 21 bit encoding ...
						(*ch++) = 0xF0 | (char)( ( longer_value >> 18 ) & 0x07 );
						(*ch++) = 0x80 | (char)( ( longer_value >> 12 ) & 0x3f );
						(*ch++) = 0x80 | (char)( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | (char)( ( longer_value >> 0 ) & 0x3f );
					}
					/*  ** functionally removed from spec ..... surrogates cannot be this long.
					else if( !( longer_value & 0xFC000000 ) )
					{
						(*ch++) = 0xF8 | ( longer_value >> 24 );
						(*ch++) = 0x80 | ( ( longer_value >> 18 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 12 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 0 ) & 0x3f );
					}
					else  if( !( longer_value & 0x80000000 ) )
					{
						// 31 bit encode
						(*ch++) = 0xFC | ( longer_value >> 30 );
						(*ch++) = 0x80 | ( ( longer_value >> 24 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 18 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 12 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 6 ) & 0x3f );
						(*ch++) = 0x80 | ( ( longer_value >> 0 ) & 0x3f );
					}
					*/
					else
					{
						// too long to encode.
					}
				}
				else
				{
					   //lprintf( " 3 byte encode?  16 bits" );
						(*ch++) = 0xE0 | ( ( wch[0] >> 12 ) & 0x0F ); // mask just in case of stupid compiles that tread wchar as signed?
						(*ch++) = 0x80 | ( ( wch[0] >> 6 ) & 0x3f );
						(*ch++) = 0x80 | ( ( wch[0] >> 0 ) & 0x3f );
				}
			}
			wch++;
		}
	}
	(*ch) = 0;
	if( outlen ) outlen[0] = ch - _ch;
	ch = _ch;
	return ch;
}

char * WcharConvertExx ( const wchar_t *wch, size_t len DBG_PASS )
{
	size_t outlen;
	return WcharConvert_v2( wch, len, &outlen DBG_RELAY );
}

char * WcharConvertEx ( const wchar_t *wch DBG_PASS )
{
	size_t len;
	for( len = 0; wch[len]; len++ );
	return WcharConvertExx( wch, len DBG_RELAY );
}

wchar_t * CharWConvertExx ( const char *wch, size_t len DBG_PASS )
{
	// Conversion to wchar_t* :
	// Can just convert wchar_t* to char* using one of the
	// conversion functions such as:
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t  sizeInChars;
	const char *_wch;
	wchar_t	*ch;
	wchar_t   *_ch;
	if( !wch ) return NULL;
	sizeInChars = 0;
	_wch = wch;
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			//lprintf( "first char is %d (%08x)", wch[0] );
			if( (wch[0] & 0xE0) == 0xC0 )
				wch += 2;
			else if( (wch[0] & 0xF0) == 0xE0 )
				wch += 3;
			else if( (wch[0] & 0xF0) == 0xF0 )
			{
				sizeInChars++;
				wch += 4;
			}
			else
				wch++;
			sizeInChars++;
		}
	}
	wch = _wch;
	_ch = ch = NewArray( wchar_t, sizeInChars + 1 );
	{
		size_t n;
		for( n = 0; n < len; n++ )
		{
			//lprintf( "first char is %d (%08x)", wch[0] );
			if( ( wch[0] & 0xE0 ) == 0xC0 )
			{
				ch[0] = ( ( (wchar_t)wch[0] & 0x1F ) << 6 ) | ( (wchar_t)wch[1] & 0x3f );
				wch += 2;
			}
			else if( ( wch[0] & 0xF0 ) == 0xE0 )
			{
				ch[0] = ( ( (wchar_t)wch[0] & 0xF ) << 12 )
					| ( ( (wchar_t)wch[1] & 0x3F ) << 6 )
					| ( (wchar_t)wch[2] & 0x3f );
				wch += 3;
			}
			else if( ( wch[0] & 0xF0 ) == 0xF0 )
			{
				uint32_t literal_char =  ( ( (wchar_t)wch[0] & 0x7 ) << 18 )
				                 | ( ( (wchar_t)wch[1] & 0x3F ) << 12 )
				                 | ( (wchar_t)wch[2] & 0x3f ) << 6
				                 | ( (wchar_t)wch[3] & 0x3f );
				//lprintf( "literal char is %d (%08x", literal_char, literal_char );
				ch[0] = 0xD800 + ( ( ( literal_char - 0x10000 ) & 0xFFC00 ) >> 10 );// ((wchar_t*)&literal_char)[0];
				ch[1] = 0xDC00 + ( ( literal_char - 0x10000 ) & 0x3ff );// ((wchar_t*)&literal_char)[1];
				ch++;
				wch += 4;
			}
			else
			{
				ch[0] = wch[0] & 0x7f;
				wch++;
			}
			ch++;
		}
		ch[0] = 0;
	}
	return _ch;
}

wchar_t * CharWConvertEx ( const char *ch DBG_PASS )
{
	int len;
	if( !ch ) return NULL;
	for( len = 0; ch[len]; len++ );
	return CharWConvertExx( ch, len DBG_RELAY );
}

LOGICAL ParseStringVector( CTEXTSTR data, CTEXTSTR **pData, int *nData )
{
	if( !data[0] )
	{
		*nData = 0;
		return 0;
	}
	//xlprintf(2100)( "ParseStringVector" );
	//if( StrChr( data, ',' ) )
	{
		CTEXTSTR start, end;
		int count = 0;
		end = data;
		do
		{
			count++;
			start = end;
			end = StrChr( start, ',' );
			if( end )
				end++;
		}
		while( end );

		if( (*pData) )
		{
			//lprintf( "Had old data, release and make new" );
			Release( (POINTER)(*pData) );
		}
		(*pData) = NewArray( CTEXTSTR, count );
		(*nData) = count;

		count = 0;
		end = data;
		do
		{

			size_t len;
			start = end;
			end = StrChr( start, ',' );
			if( end )
			{
				end++;
				(*pData)[count] = NewArray( TEXTCHAR, len = end - start );
			}
			else
			{
				(*pData)[count] = NewArray( TEXTCHAR, len = StrLen( start ) + 1 );
			}
			StrCpyEx( (TEXTSTR)(*pData)[count], start, len );

			count++;
		}
		while( end );

		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

TEXTRUNE GetUtfChar( const char * *from )
{
	TEXTRUNE result = (unsigned char)(*from)[0];
	//if( !result ) return result;
	if( (*from)[0] & 0x80 )
	{
		if( ( (*from)[0] & 0xE0 ) == 0xC0 )
		{
			if( ( (*from)[1] & 0xC0 ) == 0x80 )
			{
				result = ( ( (unsigned int)(*from)[0] & 0x1F ) << 6 ) | ( (unsigned int)(*from)[1] & 0x3f );
				(*from) += 2;
			}
			else
			{
				result = 0;
				//lprintf( "a 2 byte code with improper continuation encodings following it was found. %02x %02x"
				//		, (*from)[0]
				//		, (*from)[1]
				//		);
				(*from)++;
			}
		}
		else if( ( (*from)[0] & 0xF0 ) == 0xE0 )
		{
			if( ( ( (*from)[1] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[2] & 0xC0 ) == 0x80 ) )
			{
				result = ( ( (unsigned int)(*from)[0] & 0xF ) << 12 ) | ( ( (unsigned int)(*from)[1] & 0x3F ) << 6 ) | ( (unsigned int)(*from)[2] & 0x3f );
				(*from) += 3;
			}
			else
			{
				result = 0;
				//lprintf( "a 3 byte code with improper continuation encodings following it was found. %02x %02x %02x"
				//	, (*from)[0]
				//	, (*from)[1]
				//	, (*from)[2]
				//	);
				(*from)++;
			}
		}
		else if( ( (*from)[0] & 0xF8 ) == 0xF0 )
		{
			if( ( ( (*from)[1] & 0xC0 ) == 0x80 ) && ( ( (*from)[2] & 0xC0 ) == 0x80 ) && ( ( (*from)[3] & 0xC0 ) == 0x80 ) )
			{
				result =   ( ( (unsigned int)(*from)[0] & 0x7 ) << 18 )
						| ( ( (unsigned int)(*from)[1] & 0x3F ) << 12 )
						| ( ( (unsigned int)(*from)[2] & 0x3f ) << 6 )
						| ( (unsigned int)(*from)[3] & 0x3f );
				(*from) += 4;
			}
			else
			{
				result = 0;
				//lprintf( "a 4 byte code with improper continuation encodings following it was found." );
				(*from)++;
			}
		}
		else if( ( (*from)[0] & 0xFC ) == 0xF8 )
		{
			if( ( ( (*from)[1] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[2] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[3] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[4] & 0xC0 ) == 0x80 )
			  )
			{
				result =   ( ( (unsigned int)(*from)[0] & 0x3 ) << 24 )
						| ( ( (unsigned int)(*from)[1] & 0x3F ) << 18 )
						| ( ( (unsigned int)(*from)[2] & 0x3F ) << 12 )
						| ( ( (unsigned int)(*from)[3] & 0x3f ) << 6 )
						| ( (unsigned int)(*from)[4] & 0x3f );
				(*from) += 5;
			}
			else
			{
				result = 0;
				//lprintf( "a 4 byte code with improper continuation encodings following it was found." );
				(*from)++;
			}
		}
		else if( ( (*from)[0] & 0xFE ) == 0xFC )
		{
			if( ( ( (*from)[1] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[2] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[3] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[4] & 0xC0 ) == 0x80 )
			  && ( ( (*from)[5] & 0xC0 ) == 0x80 )
			  )
			{
				result =   ( ( (unsigned int)(*from)[0] & 0x1 ) << 30 )
						| ( ( (unsigned int)(*from)[1] & 0x3F ) << 24 )
						| ( ( (unsigned int)(*from)[2] & 0x3F ) << 18 )
						| ( ( (unsigned int)(*from)[3] & 0x3F ) << 12 )
						| ( ( (unsigned int)(*from)[4] & 0x3f ) << 6 )
						| ( (unsigned int)(*from)[5] & 0x3f );
				(*from) += 6;
			}
			else
			{
				result = 0;
				//lprintf( "a 4 byte code with improper continuation encodings following it was found." );
				(*from)++;
			}
		}
		else if( ( (*from)[0] & 0xC0 ) == 0x80 )
		{
			// things like 0x9F, 0x9A is OK; is a single byte character, is a unicode application escape
			//lprintf( "a continuation encoding was found." );

			//result = (unsigned char)(*from)[0];
			(*from)++;
		}
		else
		{
			//result = (unsigned char)(*from)[0];
			(*from)++;
		}
	}
	else
	{
		result = (unsigned char)(*from)[0];
		(*from)++;
	}
	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEXTRUNE GetUtfCharIndexed( const char * pc, size_t *n, size_t length )
{
	if( length )
	{
		CTEXTSTR orig = pc + n[0];
		CTEXTSTR tmp = orig;
		TEXTRUNE result = GetUtfChar( &tmp );
		if( (size_t)( tmp - orig ) <= length ) {
			n[0] += tmp - orig;
			return result;
		}
		// if illformed character was at the end... return 0
	   // cap result to length.
		( *n ) = length;
	}
	return 0;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEXTRUNE GetPriorUtfChar( const char *start, const char * *from )
{
	TEXTRUNE result;
	if( (*from) == start ) return 0;
	result = (unsigned char)(*from)[-1];
	if( !result ) return result;
	if( (*from)[-1] & 0x80 )
	{
		CTEXTSTR end;
		while( (*from > start) && ( (*from)[-1] & 0xC0 ) == 0x80 )
			(*from)--;
		if( (*from > start) ) {
			(*from)--;
			end = (*from);
			result = GetUtfChar( from );
			(*from) = end;
		}
		else
			result = 0;
	}
	else
	{
		result = (unsigned char)(*from)[-1];
		(*from)--;
	}
	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEXTRUNE GetPriorUtfCharIndexed( const char *pc, size_t *n )
{
	if( *n )
	{
		CTEXTSTR orig = pc + n[0];
		CTEXTSTR tmp = orig;
		TEXTRUNE result = GetPriorUtfChar( pc, &tmp );
		if( tmp <= orig ) {
			n[0] -= orig - tmp;
			return result;
		}
	}
	return RUNE_BEFORE_START;
}

//---------------------------------------------------------------------------

TEXTRUNE GetUtfCharW( const wchar_t * *from )
{
	TEXTRUNE result = (unsigned)(*from)[0];
	if( !result ) return result;
	if( ( ( (*from)[0] & 0xFC00 ) >= 0xD800 )
		&& ( ( (*from)[0] & 0xFC00 ) <= 0xDF00 ) )
	{
		result = 0x10000 + ( ( ( (*from)[0] & 0x3ff ) << 10 ) | ( ( (*from)[1] & 0x3ff ) ) );
		(*from) += 2;
	}
	else
	{
		(*from)++;
	}
	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEXTRUNE GetUtfCharIndexedW( const wchar_t* pc, size_t *n )
{
	const wchar_t * orig = pc + n[0];
	const wchar_t * tmp = orig;
	TEXTRUNE result = GetUtfCharW( &tmp );
	n[0] += tmp - orig;
	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEXTRUNE GetPriorUtfCharW( const wchar_t*start, const wchar_t* *from )
{
	TEXTRUNE result = (unsigned)(*from)[-1];
	if( !result ) return result;
	if( ( ( (*from)[0] & 0xFC00 ) >= 0xD800 )
		&& ( ( (*from)[0] & 0xFC00 ) <= 0xDF00 ) )
	{
		result = 0x10000 + ( ( ( (*from)[0] & 0x3ff ) << 10 ) | ( ( (*from)[1] & 0x3ff ) ) );
		(*from) += 2;
	}
	else
	{
		(*from)++;
	}
	return result;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

TEXTRUNE GetPriorUtfCharIndexedW( const wchar_t *pc, size_t *n )
{
	if( *n )
	{
		const wchar_t * orig = pc + n[0];
		const wchar_t * tmp = orig;
		TEXTRUNE result = GetPriorUtfCharW( pc, &tmp );
		n[0] += tmp - orig;
		return result;
	}
	return 0;
}

//---------------------------------------------------------------------------

// Return the integer character from the string
// using utf-8 or utf-16 decoding appropriately.  No more extended-ascii.

static int Step( CTEXTSTR *pc, size_t *nLen )
{
	CTEXTSTR _pc = (*pc);
	int ch;
	//lprintf( "Step (%s[%*.*s])", (*pc), nLen,nLen, (*pc) );
	if( nLen && !*nLen )
		return 0;

	ch = GetUtfChar( pc );
	//if( ch & 0xFFE00000 )
	//	DebugBreak();
	if( nLen )
		(*nLen) -= (*pc) - _pc;
	_pc = (*pc);

	if( ch )
	{
		while( ch == '\x9F' )
		{
			while( ch && ( ch != '\x9C' ) )
			{
				ch = GetUtfChar( pc );
				if( nLen )
					(*nLen) -= (*pc) - _pc;
				_pc = (*pc);
			}

			// if the string ended...
			if( !ch )
			{
				// this is done.  There's nothing left... command with no data is bad form, but not illegal.
				return FALSE;
			}
			else  // pc is now on the stop command, advance one....
			{
				// this is in a loop, and the next character may be another command....
				ch = GetUtfChar( pc );
				if( nLen )
					(*nLen) -= (*pc) - _pc;
				_pc = (*pc);
			}
		}
	}
	return ch;
}

size_t GetDisplayableCharacterBytes( CTEXTSTR string, size_t character_count )
{
	CTEXTSTR original = string;
	int ch;
	if( !string ) return 0;
	while( character_count &&
		( ch = Step( &string, NULL ) ) )
	{
		character_count--;
	}
	return string - original;
}

size_t GetDisplayableCharacterCount( CTEXTSTR string, size_t max_bytes )
{
	int ch;
	size_t count = 0;
	if( !string ) return 0;
	while( ( ch = Step( &string, &max_bytes ) ) )
	{
		count++;
	}
	return count;
}

CTEXTSTR GetDisplayableCharactersAtCount( CTEXTSTR string, size_t nLen )
{
	int ch;
	if( !string ) return 0;
	while( nLen > 0 &&
		 ( ch = Step( &string, NULL ) ) )
	{
		nLen--;
	}
	return string;
}


LOGICAL ParseIntVector( CTEXTSTR data, int **pData, int *nData )
{
	if( !data[0] )
	{
		*nData = 0;
		return 0;
	}
	//xlprintf(2100)( "ParseIntVector" );
	//if( StrChr( data, ',' ) )
	{
		CTEXTSTR start, end;
		int count = 0;
		end = data;
		do
		{
			count++;
			start = end;
			end = StrChr( start, ',' );
			if( end )
				end++;
		}
		while( end );

		if( (*pData) )
		{
			//lprintf( "Had old data, release and make new" );
			Release( (*pData) );
		}
		(*pData) = NewArray( int, count );
		(*nData) = count;

		count = 0;
		end = data;
		do
		{
			start = end;
			(void)sscanf( start, "%d", (*pData) + count );
			count++;
			end = StrChr( start, ',' );
			if( end )
				end++;
		}
		while( end );

		return TRUE;
	}
	return FALSE;
}

const char encodings[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789$_";
const char encodings2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static TEXTCHAR b64xor_table[256][256];
static TEXTCHAR u8xor_table[256][256];
static TEXTCHAR b64xor_table2[256][256];
static TEXTCHAR u8xor_table2[256][256];

PRELOAD( initTables ) {
	size_t n, m;
	for( n = 0; n < (sizeof( encodings )-1); n++ )
		for( m = 0; m < (sizeof( encodings )-1); m++ ) {
			b64xor_table[(uint8_t)encodings[n]][(uint8_t)encodings[m]] = encodings[n^m];
			u8xor_table[n][(uint8_t)encodings[m]] = (TEXTCHAR)(n^m);

			b64xor_table2[(uint8_t)encodings2[n]][(uint8_t)encodings2[m]] = encodings2[n^m];
			u8xor_table2[n][(uint8_t)encodings2[m]] = (TEXTCHAR)(n^m);
	}
	//LogBinary( (uint8_t*)u8xor_table[0], sizeof( u8xor_table ) );
	b64xor_table['=']['='] = '=';
}

char * b64xor( const char *a, const char *b ) {
	int n;
	char *out = NewArray( char, strlen(a) + 1);
	for( n = 0; a[n]; n++ ) {
		out[n] = b64xor_table[(uint8_t)a[n]][(uint8_t)b[n]];
	}
	out[n] = 0;
	return out;
}

char * u8xor( const char *a, size_t alen, const char *b, size_t blen, int *ofs ) {
	size_t n;
	size_t keylen = blen-5;
	int o = ofs[0];
	size_t outlen;
	char *out = NewArray( char, (outlen=alen) + 1);
	char *_out = out;
	int l = 0;
	int _mask = 0x3f;
	for( n = 0; n < alen; n++ ) {
		char v = (*a++);
		int mask;
		mask = _mask;

		if( (v & 0x80) == 0x00 ) { if( l ) lprintf( "short utf8 sequence found" ); mask = 0x3f; _mask = 0x3f; }
		else if( (v & 0xC0) == 0x80 ) { if( !l ) lprintf( "invalid utf8 sequence" ); l--; _mask = 0x3f; }
		else if( (v & 0xE0) == 0xC0 ) { if( l )
			lprintf( "short utf8 sequence found" ); l = 1; mask = 0x1; _mask = 0x3f; }  // 6 + 1 == 7 //-V640
		else if( (v & 0xF0) == 0xE0 ) { if( l )
			lprintf( "short utf8 sequence found" ); l = 2; mask = 0;  _mask = 0x1f; }  // 6 + 5 + 0 == 11 //-V640
		else if( (v & 0xF8) == 0xF0 ) { if( l )
			lprintf( "short utf8 sequence found" ); l = 3; mask = 0;  _mask = 0x0f; }  // 6(2) + 4 + 0 == 16 //-V640
		else if( (v & 0xFC) == 0xF8 ) { if( l )
			lprintf( "short utf8 sequence found" ); l = 4; mask = 0;  _mask = 0x07; }  // 6(3) + 3 + 0 == 21 //-V640
		else if( (v & 0xFE) == 0xFC ) { if( l )
			lprintf( "short utf8 sequence found" ); l = 5; mask = 0;  _mask = 0x03; }  // 6(4) + 2 + 0 == 26 //-V640

		// B is a base64 key; it would never be > 128 so char index is OK.
		char bchar = b[(n+o)%(keylen)]&0x7f;
		(*out) = (v & ~mask ) | ( u8xor_table[v & mask ][bchar] & mask );
		out++;
	}
	(*out) = 0;
	ofs[0] = (int)((ofs[0]+outlen)%keylen);
	return _out;
}

static const char * const _base642 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789$_=";
static const char * const _base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static const char * _last_base64_set;
static char _base64_r[256];

static void encodeblock( unsigned char in[3], TEXTCHAR out[4], size_t len, const char *base64 )
{
	out[0] = base64[ in[0] >> 2 ];
	out[1] = base64[ ((in[0] & 0x03) << 4) | ( ( len > 0 ) ? ((in[1] & 0xf0) >> 4) : 0 ) ];
	out[2] = (len > 1 ? base64[ ((in[1] & 0x0f) << 2) | ( ( len > 2 ) ? ((in[2] & 0xc0) >> 6) : 0 ) ] : base64[64]);
	out[3] = (len > 2 ? base64[ in[2] & 0x3f ] : base64[64]);
}

static void decodeblock( const char in[4], uint8_t out[3], size_t len, const char *base64 )
{
	int index[4];
	size_t n;
	for( n = 0; n < len; n++ )
	{
		// propagate terminator.
		if( n && ( index[n - 1] == 64 ) ) index[n] = 0;
		else index[n] = _base64_r[in[n]];
	}
	for( ; n < 4; n++ )
		index[n] = 0;

	out[0] = (char)(( index[0] ) << 2 | ( index[1] ) >> 4);
	out[1] = (char)(( index[1] ) << 4 | ( ( ( index[2] ) >> 2 ) & 0x0f ));
	out[2] = (char)(( index[2] ) << 6 | ( ( index[3] ) & 0x3F ));
	//out[] = (len > 2 ? base64[ in[2] & 0x3f ] : 0);
}

TEXTCHAR *EncodeBase64Ex( const uint8_t* buf, size_t length, size_t *outsize, const char *base64 )
{
	size_t fake_outsize;
	TEXTCHAR * real_output;
	if( !outsize ) outsize = &fake_outsize;
	if( !base64 )
		base64 = _base64;
	else if( ((uintptr_t)base64) == 1 )
		base64 = _base642;
	real_output = NewArray( TEXTCHAR, 1 + ( ( length * 4 + 2) / 3 ) + 1 + 1 + 1 );
	{
		size_t n;
		for( n = 0; n < (length+2)/3; n++ )
		{
			size_t blocklen;
			blocklen = length - n*3;
			if( blocklen > 3 )
				blocklen = 3;
			encodeblock( ((uint8_t*)buf) + n * 3, real_output + n*4, blocklen, base64 );
		}
		(*outsize) = n*4; // don't include the NUL.
		real_output[n*4] = 0;
	}
	return real_output;
}

static void setupDecodeBytes( const char *code ) {
	int n = 0;
	// default all of these, allow code to override them.
	if( _last_base64_set != code ) {
		_last_base64_set = code;
		memset( _base64_r, 0, 256 );
                // allow nul terminators (sortof)
                _base64_r[0] = 64; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r['~'] = 64; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r['='] = 64; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.

                // My JS Encoding $_ and = at the end.  allows most to be identifiers too.
                // 'standard' encoding +/
                // variants -/
                //          +,
                //          ._
                // variants -_

                _base64_r['$'] = 62; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r['+'] = 62; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r['-'] = 62; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r['.'] = 62; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.

                _base64_r['_'] = 63; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r['/'] = 63; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.
                _base64_r[','] = 63; // = ix 64 (0x40) and mask is & 0x3F dropping the upper bit.

                while( *code ) {
                    _base64_r[*code] = n++;
                    code++;
                }
        }
}
uint8_t *DecodeBase64Ex( const char* buf, size_t length, size_t *outsize, const char *base64 )
{
	static const char *useBase64;
	size_t fake_outsize;
	uint8_t * real_output;
	if( !outsize ) outsize = &fake_outsize;
	if( !base64 )
		base64 = _base64;
	else if( ((uintptr_t)base64) == 1 )
		base64 = _base642;
	if( useBase64 != base64 ) {
		useBase64 = base64;
		setupDecodeBytes( base64 );
	}
	real_output = NewArray( uint8_t, ( ( ( length + 1 ) * 3 ) / 4 ) + 1 );
	{
		size_t n;
		for( n = 0; n < (length+3)/4; n++ )
		{
			size_t blocklen;
			blocklen = length - n*4;
			if( blocklen > 4 )
				blocklen = 4;
			decodeblock( buf + n * 4, real_output + n*3, blocklen, base64 );
		}
		if( length % 4 == 1 )
			(*outsize) = (((length + 3) / 4) * 3) - 3;
		else if( length % 4 == 2 )
			(*outsize) = (((length + 3) / 4) * 3) - 2;
		else if( length % 4 == 3 )
			(*outsize) = (((length + 3) / 4) * 3) - 1;
		else if( buf[length - 1] == '=' ) {
			if( buf[length - 2] == '=' ) {
				(*outsize) = (((length + 3) / 4) * 3) - 2;
			}
			else
				(*outsize) = (((length + 3) / 4) * 3) - 1;
		}
		else
			(*outsize) = (((length + 3) / 4) * 3);
		real_output[(*outsize)] = 0;
	}
	return real_output;
}


#ifdef __cplusplus
} //namespace text {
} //namespace containers {
} // namespace sack {
#endif
#ifdef _MSC_VER
#  pragma warning( default:6011 26451 28182)
#  pragma warning( default:26451 )
#endif
