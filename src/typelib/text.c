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

#ifdef __cplusplus
namespace sack {
namespace containers {
namespace text {
	using namespace sack::memory;
	using namespace sack::logging;
	using namespace sack::containers::queue;
#endif


typedef PTEXT (CPROC*GetTextOfProc)( PTRSZVAL, POINTER );

typedef struct text_exension_tag {
	_32 bits;
	GetTextOfProc TextOf;
	PTRSZVAL psvData;
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
	newline = (PTEXT)SegCreateFromText( WIDE("") );
	blank = (PTEXT)SegCreateFromText( WIDE(" ") );
}
//#define newline (*newline)
//#define blank   (*blank)
//#else
//__declspec( dllexport ) TEXT newline = { TF_STATIC, NULL, NULL, {1,1},{0,WIDE("")}};
//__declspec( dllexport ) TEXT blank = { TF_STATIC, NULL, NULL, {1,1},{1,WIDE(" ")}};
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

_32 GetTextFlags( PTEXT segment )
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
         HoldEx( (P_8)pt DBG_RELAY  );
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
#ifdef _UNICODE 
		TEXTSTR text_string = CharWConvertLen( text, len );
		pTemp = SegCreateEx( len DBG_RELAY );
		// include nul on copy
		MemCpy( pTemp->data.data, text_string, sizeof( TEXTCHAR ) * ( len + 1 ) );
		Deallocate( TEXTSTR, text_string );
#else
		pTemp = SegCreateEx( len DBG_RELAY );
		MemCpy( pTemp->data.data, text, sizeof( TEXTCHAR ) * ( len + 1 ) );
#endif
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
#ifdef _UNICODE 
		pTemp = SegCreateEx( nSize DBG_RELAY );
		// include nul on copy
		MemCpy( pTemp->data.data, text, sizeof( TEXTCHAR ) * ( nSize + 1 ) );
#else
		TEXTSTR text_string = WcharConvertLen( text, nSize );
		pTemp = SegCreateEx( nSize DBG_RELAY );
		// include nul on copy
		MemCpy( pTemp->data.data, text_string, sizeof( TEXTCHAR ) * ( nSize + 1 ) );
		Deallocate( TEXTSTR, text_string );
#endif
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
#ifdef _UNICODE
	pResult->data.size = snwprintf( pResult->data.data, 12, WIDE("%d"), value );
#else
	pResult->data.size = snprintf( pResult->data.data, 12, WIDE("%d"), value );
#endif
	pResult->data.data[11] = 0;
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFrom_64Ex( S_64 value DBG_PASS )
{
	PTEXT pResult;
	pResult = SegCreateEx( 32 DBG_RELAY);
#ifdef _UNICODE
	pResult->data.size = snwprintf( pResult->data.data, 32, WIDE("%")_64f, value );
#else
	pResult->data.size = snprintf( pResult->data.data, 32, WIDE("%")_64f, value );
#endif
pResult->data.data[31] = 0;
	return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateFromFloatEx( float value DBG_PASS )
{
   PTEXT pResult;
   pResult = SegCreateEx( 32 DBG_RELAY);
#ifdef _UNICODE
	pResult->data.size = snwprintf( pResult->data.data, 32, WIDE("%f"), value );
#else
	pResult->data.size = snprintf( pResult->data.data, 32, WIDE("%f"), value );
#endif
   pResult->data.data[31] = 0;
   return pResult;
}

//---------------------------------------------------------------------------

PTEXT SegCreateIndirectEx( PTEXT pText DBG_PASS )
{
   PTEXT pSeg;
   pSeg = SegCreateEx( -1 DBG_RELAY ); // no data content for indirect...
   pSeg->flags |= TF_INDIRECT;
   pSeg->data.size = (PTRSZVAL)pText;
   return pSeg;
}

//---------------------------------------------------------------------------

PTEXT SegBreak(PTEXT segment)  // remove leading segments.
{    // return leading segments!  might be ORPHANED if not handled.
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
			lprintf( WIDE("Adding %d spaces"), segment->format.position.offset.spaces );
			total += segment->format.position.offset.spaces;
		}
	}
	while( (segment->flags & TF_INDIRECT) && ( segment = GetIndirect( segment ) ) );
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
	//Log1( WIDE("SegExpand...%d"), nSize );
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

PTEXT SegConcatEx(PTEXT output,PTEXT input,S_32 offset,size_t length DBG_PASS )
{
	size_t idx=0;
	size_t len=0;
	PTEXT newseg;
	SegAppend( output, newseg = SegCreate( length ) );
	output = newseg;
	//output=SegExpandEx(output, length DBG_RELAY); /* add 1 for a null */

	GetText(output)[0]=0;

	while (input&&idx<length)
	{
//#define min(a,b) (((a)<(b))?(a):(b))
		len = min( GetTextSize( input ) - offset, length-idx );
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
	here->flags  = (*pLine)->flags;
	here->format = (*pLine)->format;
	there = SegCreateEx( (nLen - nPos) DBG_RELAY );
	there->flags  = (*pLine)->flags;
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

TEXTCHAR NextCharEx( PTEXT input, size_t idx )
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

// In this final implementation - it was decided that for a general
// library, that expressions, escapes of expressions, apostrophes
// were of no consequence, and without expressions, there is no excess
// so this simply is text stream in, text stream out.

// these are just shortcuts - these bits of code were used repeatedly....

#define SET_SPACES() do {		word->format.position.offset.spaces = (_16)spaces; \
		word->format.position.offset.tabs = (_16)tabs;                             \
		spaces = 0;                                                         \
		tabs = 0; } while(0)


//static CTEXTSTR normal_punctuation=WIDE("\'\"\\({[<>]}):@%/,;!?=*&$^~#`");
//static CTEXTSTR not_punctuation;

PTEXT TextParse( PTEXT input, CTEXTSTR punctuation, CTEXTSTR filter_space, int bTabs, int bSpaces  DBG_PASS )
// returns a TEXT list of parsed data
{
//#define DBG_OVERRIDE DBG_SRC
#define DBG_OVERRIDE DBG_RELAY
   /* takes a line of input and creates a line equivalent to it, but
      burst into its block peices.*/
   VARTEXT out;
   PTEXT outdata=(PTEXT)NULL,
         word;
	TEXTSTR tempText;
   int has_minus = -1;
	int has_plus = -1;

   _32 index;
   INDEX size;

   TEXTCHAR character;
   _32 elipses = FALSE,
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
		tempText = GetText(input);  // point to the data to process...
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
      //Log1( WIDE("Assuming %d spaces... "), spaces );
      for (index=0;(character = tempText[index]),
                   (index < size); index++) // while not at the
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
      	      //	Log( WIDE("VarTextGet Failed to result.") );
         	}
            elipses = FALSE;
         }
         else if( elipses ) // elipses and character is . - continue
         {
         	VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
				VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
				word = VarTextGetEx( &out DBG_OVERRIDE );
            	outdata = SegAppend( outdata, word );
            }
            else
            {
				VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
				if(0) {
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
						//lprintf( WIDE("Input stream has mangled spaces and tabs.") );
                     spaces = 0; // assume that the tab takes care of appropriate spacing
						}
						tabs++;
						break;
					}
				} else if(0) {
				case '\r': // a space space character...
					if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
					{
						SET_SPACES();
						outdata = SegAppend( outdata, word );
					}
					break;
				} else if(0) {
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
							VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
							break;
						}
					}
				} else if(0) {
				case '-':  // work seperations flaming-long-sword
					if( has_minus == -1 )
						if( !punctuation || StrChr( punctuation, '-' ) )
							has_minus = 1;
						else
							has_minus = 0;
					if( !has_minus )
					{
						VarTextAddCharacterEx( &out, '-' DBG_OVERRIDE );
                  break;
					}
				case '+':
				{
               int c;
					if( has_plus == -1 )
						if( !punctuation || StrChr( punctuation, '-' ) )
							has_plus = 1;
						else
							has_plus = 0;
					if( !has_plus )
					{
						VarTextAddCharacterEx( &out, '-' DBG_OVERRIDE );
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
						VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
						break;
					}
				}
//         NormalPunctuation:
            if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
            {
            	outdata = SegAppend( outdata, word );
					SET_SPACES();
               VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
					word = VarTextGetEx( &out DBG_OVERRIDE );
            	outdata = SegAppend( outdata, word );
            }
            else
            {
               VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
            VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
      burst into its block peices.*/
   VARTEXT out;
   PTEXT outdata=(PTEXT)NULL,
         word;
   TEXTSTR tempText;

   _32 index;
   size_t size;

   TEXTCHAR character;
   _32 elipses = FALSE,
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
			outdata = SegAppend( outdata, burst( GetIndirect( input ) ) );
			input = NEXTLINE( input );
			continue;
		}
		tempText = GetText(input);  // point to the data to process...
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
		//Log1( WIDE("Assuming %d spaces... "), spaces );
		for (index=0;(character = tempText[index]),
                   (index < size); index++) // while not at the
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
      	      //	Log( WIDE("VarTextGet Failed to result.") );
         	}
            elipses = FALSE;
         }
         else if( elipses ) // elipses and character is . - continue
         {
         	VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
				//lprintf( WIDE("Input stream has mangled spaces and tabs.") );
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
                  VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
						VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
//         NormalPunctuation:
            if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
            {
            	outdata = SegAppend( outdata, word );
					SET_SPACES();
               VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
					word = VarTextGetEx( &out DBG_OVERRIDE );
            	outdata = SegAppend( outdata, word );
            }
            else
            {
               VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
            VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
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
   int   TopSingle = bSingle;
   PTEXT pStack[32];
   int   nStack;
   int   skipspaces = ( PRIORLINE(pt) != NULL );
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
            //   DebugBreak();
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
//   if( length > 60000 )
//      _asm int 3;
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
// this cannot work... and it must provide multiple peices...

#undef BuildLineExx
PTEXT BuildLineExx( PTEXT pt, LOGICAL bSingle, PTEXT pEOL DBG_PASS )
{
   return BuildLineExEx( pt, bSingle, 8, pEOL DBG_RELAY );
}

PTEXT BuildLineExEx( PTEXT pt, LOGICAL bSingle, int nTabsize, PTEXT pEOL DBG_PASS )
{
	TEXTSTR buf;
	int   TopSingle = bSingle;
	PTEXT pStack[32];
	int   nStack, firstadded;
	int   skipspaces = ( PRIORLINE(pt) != NULL );
	PTEXT pOut;
	PTRSZVAL ofs;

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
				//Log( WIDE("Changing segment's color...") );
				if( ofs )
				{
					pSplit = SegSplitEx( &pOut, ofs DBG_RELAY );
					if( !pSplit )
					{
						lprintf( WIDE("Line was shorter than offset: %") _size_f WIDE(" vs %") _PTRSZVALfs WIDE(""), GetTextSize( pOut ), ofs );
					}
					pOut = NEXTLINE( pSplit );
					// new segments takes on the new attributes...
					pOut->format.flags.foreground = pt->format.flags.foreground;
					pOut->format.flags.background = pt->format.flags.background;
            		//Log2( WIDE("Split at %d result %d"), ofs, GetTextSize( pOut ) );
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
            //   DebugBreak();
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
      text->data.size = (PTRSZVAL)p;
	}
}

//----------------------------------------------------------------------------

void RegisterTextExtension( _32 flags, PTEXT(CPROC*TextOf)(PTRSZVAL,POINTER), PTRSZVAL psvData)
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

S_64 IntCreateFromText( CTEXTSTR p )
{
   //CTEXTSTR p;
	int s;
	int begin;
	S_64 num;
	//p = GetText( pText );
	if( !p )
		return FALSE;
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
			break;
		}
		else
		{
			num *= 10;
			num += *p - '0';
		}
		begin = FALSE;
		p++;
	}
	if( s )
		num *= -1;
	return num;
}

//--------------------------------------------------------------------------

S_64 IntCreateFromSeg( PTEXT pText )
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
int IsSegAnyNumberEx( PTEXT *ppText, double *fNumber, S_64 *iNumber, int *bIntNumber, int bUseAll )
{
	CTEXTSTR pCurrentCharacter;
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
			lprintf( WIDE("Encountered indirect segment gathering number, stopping.") );
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
	Log( WIDE("Resetting collect_used (init)") );
#endif
	pvt->collect_used = 0;
	pvt->collect_avail = COLLECT_LEN;
   pvt->expand_by = 32;
}

 PVARTEXT  VarTextCreateExEx ( _32 initial, _32 expand DBG_PASS )
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
	Log1( WIDE("Adding character %c"), c );
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
			//lprintf( WIDE("Expanding segment to make sure we have room to extend...(old %d)"), pvt->collect->data.size );
			pvt->collect = SegExpandEx( pvt->collect, COLLECT_LEN DBG_RELAY );
			pvt->collect_avail = pvt->collect->data.size;
			pvt->collect_text = GetText( pvt->collect );
		}
	}
}

//---------------------------------------------------------------------------

void VarTextAddDataEx( PVARTEXT pvt, CTEXTSTR block, size_t length DBG_PASS )
{
	if( !pvt->collect )
		VarTextInitEx( pvt DBG_RELAY );
#ifdef VERBOSE_DEBUG_VARTEXT
	Log1( WIDE("Adding character %c"), c );
#endif
	{
		_32 n;
		for( n = 0; n < length; n++ )
		{
			int c = block[n];
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
					//lprintf( WIDE("Expanding segment to make sure we have room to extend...(old %d)"), pvt->collect->data.size );
					pvt->collect = SegExpandEx( pvt->collect, COLLECT_LEN DBG_RELAY );
					pvt->collect_avail = pvt->collect->data.size;
					pvt->collect_text = GetText( pvt->collect );
				}
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
		//lprintf( WIDE("End collect at %d %d"), pvt->collect_used, segs?segs->data.size:pvt->collect->data.size );
		if( !segs )
		{
			segs = pvt->collect;
		}

		//Log1( WIDE("Breaking collection adding... %s"), GetText( segs ) );
		// so now the remaining buffer( if any ) 
		// is assigned to collect into.
		// This results in...

		pvt->collect = NEXTLINE( segs );
		if( !pvt->collect ) // used all of the line...
		{
#ifdef VERBOSE_DEBUG_VARTEXT
			Log( WIDE("Starting with new buffers ") );
#endif
			VarTextInitEx( pvt DBG_RELAY );
		}
		else
		{
 			//Log1( WIDE("Remaining buffer is %d"), GetTextSize( pvt->collect ) );
			SegBreak( pvt->collect );
			pvt->collect_text = GetText( pvt->collect );
#ifdef VERBOSE_DEBUG_VARTEXT
			Log( WIDE("resetting collect_used after split") );
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
	//Log1( WIDE("Length is : %d"), pvt->collect_used );
	if( pvt )
		return pvt->collect_used;
	return 0;
}


//---------------------------------------------------------------------------

INDEX vvtprintf( PVARTEXT pvt, CTEXTSTR format, va_list args )
{
	INDEX len;
	int tries = 0;
#if defined( UNDER_CE ) || defined( _MSC_VER )// this might be unicode...
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
				lprintf( WIDE( "Single buffer expanded more then %d" ), tries * ( (256<pvt->expand_by)?pvt->expand_by:(256+pvt->expand_by) ) );
				return 0; // didn't add any
			}
			VarTextExpand( pvt, (256<pvt->expand_by)?pvt->expand_by:(256+pvt->expand_by)  );
			continue;
		}
		len = StrLen( pvt->collect_text + pvt->collect_used );
		pvt->collect_used += len;
		break;
	}
	return len;

#elif defined( __GNUC__ )
	{
#ifdef __GNUC__
		va_list tmp_args;
		va_copy( tmp_args, args );
#endif
#ifdef _UNICODE
#define vsnprintf vsnwprintf
#endif
		// len returns number of characters (not NUL)
		len = vsnprintf( NULL, 0, format
#ifdef __GNUC__
							, tmp_args
#else
							, args
#endif
							);
		if( !len ) // nothign to add... we'll get stuck looping if this is not checked.
			return 0;
#ifdef __GNUC__
      va_end( tmp_args );
#endif
		// allocate +1 for length with NUL
		if( ((_32)len+1) >= (pvt->collect_avail-pvt->collect_used) )
		{
			// expand when we need more room.
			VarTextExpand( pvt, (len+1<pvt->expand_by)?pvt->expand_by:(len+1+pvt->expand_by)  );
		}
#ifdef VERBOSE_DEBUG_VARTEXT
		Log3( WIDE("Print Length: %d into %d after %s"), len, pvt->collect_used, pvt->collect_text );
#endif
		// include NUL in the limit of characters able to print...
		vsnprintf( pvt->collect_text + pvt->collect_used, len+1, format, args );
	}
#elif defined( __WATCOMC__ )
	{
		int destlen;
		va_list _args;
		_args[0] = args[0];
		do {
#ifdef VERBOSE_DEBUG_VARTEXT
			Log2( WIDE("Print Length: ofs %d after %s")
				 , pvt->collect_used
				 , pvt->collect_text );
#endif
			args[0] = _args[0];
			//va_start( args, format );
#ifdef _UNICODE
#define vsnprintf _vsnwprintf
#endif
			len = vsnprintf( pvt->collect_text + pvt->collect_used
								, destlen = pvt->collect_avail - pvt->collect_used
								, format, args );
			if( !len ) // nothign to add... we'll get stuck looping if this is not checked.
				return 0;
#ifdef VERBOSE_DEBUG_VARTEXT
			lprintf( WIDE("result of vsnprintf: %d(%d) \'%s\' (%s)")
					 , len, destlen
					 , pvt->collect_text
					 , format );
#endif
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
			if( strcmp( format, "%s" ) == 0 && pvt->collect_used==7)
            DebugBreak();
			len = vsnprintf( pvt->collect_text + pvt->collect_used
								, pvt->collect_avail - pvt->collect_used
								, format, args );
			if( len < 0 )
				VarTextExpand( pvt, pvt->expand_by );
			//                VarTextExpandEx( pvt, 32 DBG_SRC );
		} while( len < 0 );
		//Log1( WIDE("Print Length: %d"), len );
	}
#endif
#ifdef VERBOSE_DEBUG_VARTEXT
	Log2( WIDE("used: %d plus %d"), pvt->collect_used , len );
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
//    PTExT (single data segment with full description \r in text)
//
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static CTEXTSTR Ops[] = {
	WIDE("FORMAT_OP_CLEAR_END_OF_LINE"),
	WIDE("FORMAT_OP_CLEAR_START_OF_LINE"),
	WIDE("FORMAT_OP_CLEAR_LINE "),
	WIDE("FORMAT_OP_CLEAR_END_OF_PAGE"),
	WIDE("FORMAT_OP_CLEAR_START_OF_PAGE"),
	WIDE("FORMAT_OP_CLEAR_PAGE"),
	WIDE("FORMAT_OP_CONCEAL") 
	  , WIDE("FORMAT_OP_DELETE_CHARS") // background is how many to delete.
	  , WIDE("FORMAT_OP_SET_SCROLL_REGION") // format.x, y are start/end of region -1,-1 clears.
	  , WIDE("FORMAT_OP_GET_CURSOR") // this works as a transaction...
	  , WIDE("FORMAT_OP_SET_CURSOR") // responce to getcursor...

	  , WIDE("FORMAT_OP_PAGE_BREAK") // clear page, home page... result in page break...
	  , WIDE("FORMAT_OP_PARAGRAPH_BREAK") // break between paragraphs - kinda same as lines...
};

//---------------------------------------------------------------------------

static void BuildTextFlags( PVARTEXT vt, PTEXT pSeg )
{
	vtprintf( vt, WIDE( "Text Flags: " ));
	if( pSeg->flags & TF_STATIC )
		vtprintf( vt, WIDE( "static " ) );
	if( pSeg->flags & TF_QUOTE )
		vtprintf( vt, WIDE( "\"\" " ) );
	if( pSeg->flags & TF_SQUOTE )
		vtprintf( vt, WIDE( "\'\' " ) );
	if( pSeg->flags & TF_BRACKET )
		vtprintf( vt, WIDE( "[] " ) );
	if( pSeg->flags & TF_BRACE )
		vtprintf( vt, WIDE( "{} " ) );
	if( pSeg->flags & TF_PAREN )
		vtprintf( vt, WIDE( "() " ) );
	if( pSeg->flags & TF_TAG )
		vtprintf( vt, WIDE( "<> " ) );
	if( pSeg->flags & TF_INDIRECT )
		vtprintf( vt, WIDE( "Indirect " ) );
   /*
	if( pSeg->flags & TF_SINGLE )
	vtprintf( vt, WIDE( "single " ) );
   */
	if( pSeg->flags & TF_FORMATREL )
      vtprintf( vt, WIDE( "format x,y(REL) " ) );
	if( pSeg->flags & TF_FORMATABS )
      vtprintf( vt, WIDE( "format x,y " ) );
   else
		vtprintf( vt, WIDE( "format spaces " ) );

	if( pSeg->flags & TF_COMPLETE )
		vtprintf( vt, WIDE( "complete " ) );
	if( pSeg->flags & TF_BINARY )
		vtprintf( vt, WIDE( "binary " ) );
	if( pSeg->flags & TF_DEEP )
		vtprintf( vt, WIDE( "deep " ) );
#ifdef DEKWARE_APP_FLAGS
	if( pSeg->flags & TF_ENTITY )
		vtprintf( vt, WIDE( "entity " ) );
	if( pSeg->flags & TF_SENTIENT )
		vtprintf( vt, WIDE( "sentient " ) );
#endif
	if( pSeg->flags & TF_NORETURN )
		vtprintf( vt, WIDE( "NoReturn " ) );
	if( pSeg->flags & TF_LOWER )
		vtprintf( vt, WIDE( "Lower " ) );
	if( pSeg->flags & TF_UPPER )
		vtprintf( vt, WIDE( "Upper " ) );
	if( pSeg->flags & TF_EQUAL )
		vtprintf( vt, WIDE( "Equal " ) );
	if( pSeg->flags & TF_TEMP )
		vtprintf( vt, WIDE( "Temp " ) );
#ifdef DEKWARE_APP_FLAGS
	if( pSeg->flags & TF_PROMPT )
		vtprintf( vt, WIDE( "Prompt " ) );
	if( pSeg->flags & TF_PLUGIN )
		vtprintf( vt, WIDE( "Plugin=%02x " ), (_8)(( pSeg->flags >> 26 ) & 0x3f ) );
#endif
	
	if( (pSeg->flags & TF_FORMATABS ) )
		vtprintf( vt, WIDE( "Pos:%d,%d " )
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else if( (pSeg->flags & TF_FORMATREL ) )
		vtprintf( vt, WIDE( "Rel:%d,%d " )
				, pSeg->format.position.coords.x
				, pSeg->format.position.coords.y  );
	else
		vtprintf( vt, WIDE( "%d tabs %d spaces" )
				  , pSeg->format.position.offset.tabs
				  , pSeg->format.position.offset.spaces
				  );
	
	if( pSeg->flags & TF_FORMATEX )
		vtprintf( vt, WIDE( "format extended(%s) length:%d" )
		           , Ops[ pSeg->format.flags.format_op
		                - FORMAT_OP_CLEAR_END_OF_LINE ] 
		           , GetTextSize( pSeg ) );
	else
		vtprintf( vt, WIDE( "Fore:%d Back:%d length:%d" )
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
			vtprintf( pvt, WIDE( "\n->%s\n" ), GetText( text ) );
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
static const TEXTCHAR *translated[] = { WIDE( "%21" ),WIDE( "%2A" ),WIDE( "%27" ),WIDE( "%28" ),WIDE( "%29" ),WIDE( "%3B" ),WIDE( "%3A" )
												,WIDE( "%40" ),WIDE( "%26" ),WIDE( "%3D" ),WIDE( "%2B" ),WIDE( "%24" ),WIDE( "%2C" ),WIDE( "%2F" )
												 ,WIDE( "%3F" ),WIDE( "%23" ),WIDE( "%5B" ),WIDE( "%5D" )
												 ,WIDE( "%3C" ),WIDE( "%3E" ),WIDE( "%7E" ),WIDE( "%2E" ),WIDE( "%22" ),WIDE( "%7B" ),WIDE( "%7D" ),WIDE( "%7C" ),WIDE( "%5C" ),WIDE( "%2D" ),WIDE( "%60" ),WIDE( "%5F" ),WIDE( "%5E" ),WIDE( "%25" ),WIDE( "%20" )
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
		else if( char_pos = StrChr( reserved_uri, text[i] ) )
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


char * WcharConvertExx ( const wchar_t *wch, size_t len DBG_PASS )
{
	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the 
	// conversion functions such as: 
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t convertedChars = 0;
	size_t  sizeInBytes;
#if defined( _MSC_VER)
	errno_t err;
#else
	int err;
#endif
	char    *ch;
	sizeInBytes = (len + 1);
	err = 0;
	ch = NewArray( char, sizeInBytes);
#if defined( _MSC_VER )
	err = wcstombs_s(&convertedChars, 
                    ch, sizeInBytes,
						  wch, len );
#else
	convertedChars = wcstombs( ch, wch, sizeInBytes);
	err = ( convertedChars == -1 );
#endif
	if (err != 0)
		lprintf(WIDE( "wcstombs_s  failed!\n" ));

	return ch;
}

char * WcharConvertEx ( const wchar_t *wch DBG_PASS )
{
   size_t len;
	for( len = 0; wch[len]; len++ );
   return WcharConvertExx( wch, len DBG_RELAY );
}

wchar_t * CharWConvertExx ( const char *wch, size_t len DBG_PASS )
{
	// Conversion to char* :
	// Can just convert wchar_t* to char* using one of the 
	// conversion functions such as: 
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t convertedChars = 0;
	size_t  sizeInBytes;
#if defined( _MSC_VER)
	errno_t err;
#else
	int err;
#endif
	wchar_t   *ch;
	sizeInBytes = ((len + 1) * sizeof( char ) );
	err = 0;
	ch = NewArray( wchar_t, sizeInBytes);
#if defined( _MSC_VER )
	err = mbstowcs_s(&convertedChars, 
                    ch, sizeInBytes,
						  wch, sizeInBytes * sizeof( wchar_t ) );
#else
	convertedChars = mbstowcs( ch, wch, sizeInBytes);
   err = ( convertedChars == -1 );
#endif
	if (err != 0)
		lprintf(WIDE( "wcstombs_s  failed!\n" ));

	return ch;
}

wchar_t * CharWConvertEx ( const char *wch DBG_PASS )
{
	int len;
	for( len = 0; wch[len]; len++ );
   return CharWConvertExx( wch, len DBG_RELAY );
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
#ifndef _MSC_VER
#if defined( _UNICODE )
#   define sscanf     swscanf
#endif
#else
#if defined( _UNICODE )
#   undef sscanf
#   define sscanf     swscanf_s
#endif
#endif
			start = end;
			sscanf( start, WIDE("%d"), (*pData) + count );
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


#ifdef __cplusplus
}; //namespace text {
}; //namespace containers {
}; // namespace sack {
#endif

