/*
 *  Crafted by James Buckeyne
 *
 *   (c) Freedom Collective 2000-2006++
 *
 *   Additional Text routines to provide more user input parsing
 *   also common editable buffers using typed text input.
 *   Handles things like backspace, saving of commands entered
 *   historically.  Could probably be remotely populated and resemble
 *   last typed search entries.  Based on PTEXT container.
 *
 * see also - include/typelib.h
 *
 */
#include <stdhdrs.h>
#include <sharemem.h>

#define PARSE_DEEP 0 // make true to recurse parse expressions

#define Collapse(towhere) SegConcat(towhere,begin,beginoffset,total)

#define IsBracket(Close)   {                                       \
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
                  total++;                                              \
               } }


//----------------------------------------------------------------------

#if 0 // used by super tokenizer below
static char NextCharEx( PTEXT input, INDEX idx )
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
#endif
//----------------------------------------------------------------------
/*
// super text tokenizer
// takes a string of input text fields,
//   bCanApostrophe disallows single quote quotations
//   bCanEscape enables using \ to modify whether a thing is a quote
//   bExpressions enables grouping of related things with "({[< >]})"
//   if expressions are enabled - excess will contain partial accumulations
// exact spacing is now retained as the default format of a text field
// TF_RALIGN and TF_LALIGN shall be phased out.
// As general as this is - it's specific to how dekware parses its data.
// probably I can generalize this... and put it into text.c which contains
// the Build LIne for reconstruction from a burst line.
PTEXT burstExx(PTEXT *excess,PTEXT input, int bCanApostrophe, int bCanEscape, int bExpressions )
// returns a TEXT list of parsed data
// and sets (*excess) to any data not put in the returned list.
{
*/
   /* takes a line of input and creates a line equivalent to it, but
      burst into its block peices.*/
/*
   PTEXT outdata=(PTEXT)NULL,
           begin, saveinput;
   char *tempText;

   _32 index,total,beginoffset;
   INDEX size;

#define ESCALIGN_NOESCAPE 0  // don't fix prior alignment....
#define ESCALIGN_NONE 1
#define ESCALIGN_LEFT 2
#define ESCALIGN_RIGHT 3
#define ESCALIGN_MIDDLE 4

   _8 character;
   _32 quote=0,
      bracket=0,
      bracketlevel=0,
      elipses = FALSE,
      set_offset=TRUE,
      escape = FALSE,
      escape_align = 0,
      no_leftalign = FALSE,
      keep_leftalign = FALSE,
      elipses_left = FALSE,
      spaces = 0;
      

   if (!input)        // if nothing new to process- return nothing processed.
      return((PTEXT)NULL);

   if( excess && *excess )    // if there was outstanding data...
   {
      saveinput = input;
      input = SegAppend( *excess, input );  // put these together, and continue...
   }
   else
      saveinput = NULL;
   begin = input;
   beginoffset = 0;
   total = 0;
	no_leftalign = TRUE;
	keep_leftalign = TRUE;
   while (input)  // while there is data to process...
   {
      tempText = GetText(input);  // point to the data to process...
      size = GetTextSize(input);
      for (index=0;(character = tempText[index]) &&
                   (index < size); index++) // while not at the
                                         // end of the line.
      {
         if( elipses && character != '.' )
         {
            if( total )
            {
               outdata = Collapse( outdata );
               outdata->format.spaces = spaces;
               spaces = 0;
            }
            if( elipses_left )
               outdata->flags |= TF_LALIGN;
            set_offset = TRUE;
            elipses = FALSE;
         }
         else if( elipses )
         {
            total++;
            continue;
         }
         if (set_offset)
         {
            set_offset=FALSE;
            begin=input;
            beginoffset=index;
            total=0;
            if( !keep_leftalign )
               no_leftalign = FALSE;
            keep_leftalign = FALSE;
         }
         switch(character)
         {
            case '\n':
               if (quote||bracket)
               {
                  //tempText[index]=' '; // erase control character
                  // burst expression again to resolve these to control line breaks...
                  total++;   
               }
               else
               {
                  if(total)
                  {
                     outdata=Collapse(outdata); // smoosh all before this character into outbuffer
                     outdata->format.spaces = spaces;
                     spaces = 0;
                     if( escape_align )
                     {
                        switch( escape_align )
                        {
                        case ESCALIGN_LEFT:
                           outdata->flags |= TF_RALIGN;
                           break;
                        case ESCALIGN_RIGHT:
                           outdata->flags |= TF_LALIGN;
                           break;
                        case ESCALIGN_MIDDLE:
//                           outdata->flags.is_Mpunct = TRUE;
                           break;
                        }
                        escape_align = 0;
                     }
                  }
                  set_offset=TRUE;   // next character starts new pass
                  // tempText[index]=0;  // kill the character to a EOL
                  outdata = SegAppend( outdata, SegCreate( 0 ) ); // add a line-break packet
                  SetEnd( outdata ); // move to the end after append...
               }
               break;
            case '\r': // a space space character...
            case ' ':
            case '\t':
               if (!quote&&!bracket)
               {
                  if (total)
                  {
                     outdata=Collapse(outdata);
                     outdata->format.spaces = spaces;
                     if( character == ' ' )
                        spaces = 1;
                     else
                        spaces = 0;
                  }
                  else if( outdata )
                  {
                     outdata->flags &= ~TF_RALIGN;
                     spaces++;
                  }

                  //if( outdata )
                   //  if( !no_leftalign )
                    //    outdata->flags |= TF_LALIGN;
                  
                  set_offset=TRUE;
               }
               else
               {
                  if( character == '\r' )   // ignore returns - but they are whitespace...
                     tempText[index] = ' ';
                  total++;
               }
               keep_leftalign = TRUE;
               no_leftalign = TRUE;
               break;
            case '\'':
            	if( !bExpressions )
	            	goto NormalPunctuation;
               if( bCanApostrophe )
               {
                  total++;
                  break;
               }
            case '\"':
            	if( !bExpressions )
						goto NormalPunctuation;
               if (!quote&&!bracket)
               {
                  if( !escape )
                  {
                     if (total)
                     {
                        outdata=Collapse(outdata);
                     }
                     begin=input;
                     beginoffset=index;
                     total=1;
                     quote=character;
                  }
                  else
                  {
                     total++; // just account it....
                     escape = FALSE;
                  }
               }
               else
               {
                  if (quote==character)
                  {
                     // at end... flush all excluding this character
                     if (total)
                     {
                        beginoffset++;  // skip first character.
                        total--;        // skip first character.
                        outdata=Collapse(outdata);
                        outdata->format.spaces = spaces;
                        spaces = 0;
                        if( outdata->data.size && PARSE_DEEP )
                        {
                           PTEXT pInternal, pPrior, pDelete, excess;
                           excess = NULL;
                           pInternal = burstEx( &excess, outdata, bCanApostrophe, bCanEscape );
                           pDelete = outdata;
                           pPrior = SegBreak( outdata );
                           LineRelease( pDelete );
                           SegAppend( pPrior, outdata = SegCreateIndirect( pInternal ) );
                           outdata->flags |= TF_DEEP;
                           if( excess )
                              SegAppend( pPrior, excess );
                        }
                        if( character == '\"' )
                           outdata->flags |= TF_QUOTE;
                        else
                           outdata->flags |= TF_SQUOTE;
                     }
                     set_offset=TRUE;
                     quote=0;   
                  }
                  else
                     total++;
               }
               break;
            case '\\':
               // escape the next character.
               if( bCanEscape )
               {
                  if( !quote && !bracket && !escape)
                  {
                     escape = TRUE;
                     if( total )
                     {
                        outdata = Collapse( outdata );
                        outdata->format.spaces = spaces;
                        spaces = 0;
                        outdata->flags |= TF_RALIGN; // smoosh this together...
                     }
                     set_offset = TRUE;
                  }
                  else
                  {
                     escape = FALSE;
                     total++;
                  }
               }
               else
                  goto NormalPunctuation;
               break;
            case '(':
            	if( !bExpressions )
						goto NormalPunctuation;
               IsBracket(')');
               break;
            case '{':
            	if( !bExpressions )
						goto NormalPunctuation;
               IsBracket('}');
               break;
            case '[':
            	if( !bExpressions )
						goto NormalPunctuation;
               IsBracket(']');
               break;
            case '<':
            	if( !bExpressions )
						goto NormalPunctuation;
               IsBracket('>');
               break;

            case ')':
            case '}':
            case ']':
            case '>':
            	if( !bExpressions )
						goto NormalPunctuation;
               if (bracket==character)  // inversed
               {
                  bracketlevel--;
                  if (!bracketlevel)
                  {
                     beginoffset++;  // skip first character.
                     total--;        // skip first character.
                     outdata=Collapse(outdata);
                     outdata->format.spaces = spaces;
                     spaces = 0;

                     if( outdata->data.size &&
                         PARSE_DEEP )
                     {
                        PTEXT pInternal, pPrior, pDelete, excess;
                        excess = NULL;
                        pInternal = burstEx( &excess, outdata, bCanApostrophe, bCanEscape );
                        pDelete = outdata;
                        pPrior = SegBreak( outdata );
                        LineRelease( pDelete );
                        SegAppend( pPrior, outdata = SegCreateIndirect( pInternal ) );
                        outdata->flags |= TF_DEEP;
                        if( excess )
                           SegAppend( pPrior, excess );
                     }
                     switch( character )
                     {
                     case ')':
                        outdata->flags |= TF_PAREN;
                        break;
                     case '}':
                        outdata->flags |= TF_BRACE;
                        break;
                     case ']':
                        outdata->flags |= TF_BRACKET;
                        break;
                     case '>':
                        outdata->flags |= TF_TAG;
                        break;
                     }
                     set_offset=TRUE;
                     bracket=0;
                  }
                  else
                    total++;
               }
               else
                  total++;  // just include it :>
               break;
            case '.':
					{
						char c;
						if( !quote && !bracket &&
							 ( !elipses &&
								( c = NextChar() ) &&
								( c == '.' ) ) )
						{
							if( total )
							{
								outdata = Collapse( outdata );
								outdata->format.spaces = spaces;
								spaces = 0;
								elipses_left = TRUE;
							}
							else
								elipses_left = FALSE;
							beginoffset = index;
							begin = input;
							total=1;
							elipses = TRUE;
							break;
						}
            case ':':  // internet addresses
				case '-':  // work seperations flaming-long-sword
				      // these only merge as normal data if and only if
				      // there was normal data character before it (total)
				      // and the next character is not ( a space,
				      // a variable reference indicator, a variable size
				      // or an 'escape' charater ) 
				      // quote and bracket concepts are now NIL
				      // unless this routine is copied somewhere else
				      // and the () [] <> {} " " characters are unshort-circuted
						if( !quote && !bracket &&
							 total &&
							 ( c = NextChar() ) &&
							 c != ' ' &&
							 c != '\t' &&
							 c != '%' &&
							 c != '#' &&
							 c != '\\' &&
							 c != '\r' &&
							 c != '\n' )
						{
							total++;
							break;
						}
					}
				case '@':  // email addresses
#pragma message ("Beware of email addresses... not sure why I think this should matter..." )
            case '%':
            case '/':
            case ',':
            case ';':
            case '!':
            case '?':
            case '=':
            case '+':
            case '*':
            case '&':
            case '$':
            case '^':
            case '~':
            case '#':
         NormalPunctuation:
               if( bracket || quote )
               {
                  total++;
                  break;
               }
               if( total )
               {
                  outdata = Collapse( outdata );
                  outdata->format.spaces = spaces;
                  spaces = 0;
						begin = input;
                  beginoffset = index;
                  total=1;
                  outdata = Collapse( outdata );
                  outdata->format.spaces = spaces;
                  spaces = 0;
                  outdata->flags |= TF_LALIGN;
               }
               else
               {
                  total=1;
                  outdata = Collapse( outdata ); // no punct at all...
                  outdata->format.spaces = spaces;
                  spaces = 0;
               }
               set_offset = TRUE;
               if( !no_leftalign )
                  outdata->flags |= TF_LALIGN;
               outdata->flags |= TF_RALIGN;
               break;

            default:
               if( elipses )
               {
                  if( total )
                  {
                     outdata = Collapse( outdata );
                     outdata->format.spaces = spaces;
                     spaces = 0;
                  }
                  if( !no_leftalign )
                     outdata->flags |= TF_LALIGN;
                  set_offset = TRUE;
                  elipses = FALSE;
               }
               total++;
               break;
         }

      }
      input=NEXTLINE(input); 
   }

   if (set_offset)
   {
      if( outdata )
         outdata->flags |= TF_COMPLETE;
      total=0;
   }

   if (quote||bracket) // partial expression at end of input
   {
      if( excess )
      {
         *excess = SegConcat(outdata,begin,beginoffset,total);
         outdata = SegBreak( *excess );
         if( saveinput ) 
            LineRelease( SegBreak( saveinput ) );  // previous data to this is okay and will be freed by caller...
      }
      else
      {
         DebugBreak();
//         DECLTEXT( msg, WIDE("\ndid not return valid data - incomplete bounding at EOI") );
//         EnqueLink( &PLAYER->Command->Output, (PTEXT)&msg );
      }
   }
   else
   {
      if( excess )
         *excess = NULL;
      if (total) // any generic outstanding data?
      {
         outdata=Collapse(outdata);
         outdata->format.spaces = spaces;
         spaces = 0;
      }
   }
   SetStart(outdata);

   return(outdata);
}
*/
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
	if( pci->CollectionIndex )
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
	         *pOutput = SplitLine( *pOutput, *pIndex );
             output = GetText( *pOutput );
             len = *pIndex = GetTextSize( *pOutput );
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
			 /*
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
			*/
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


LOGICAL  SetUserInputPosition ( PUSER_INPUT_BUFFER pci, INDEX nPos, int whence )
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
		pci->CollectionIndex = nPos;
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
