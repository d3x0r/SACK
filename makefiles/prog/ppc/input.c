//#include <windows.h>
#include <stdio.h> // FILE *
#include "mem.h"
#include "input.h" // includes text.h...
#include "global.h"


#define Collapse(towhere) SegConcat(towhere,begin,beginoffset,total)

//----------------------------------------------------------------------

char NextCharEx( PTEXT input, INDEX idx )
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

PTEXT BreakAndAddEx( char character, PTEXT outdata, VARTEXT *out, _32 *spaces )
{
   PTEXT tmp;
	PTEXT word;

	if( ( word = VarTextGetEx( out DBG_SRC ) ) )
	{
		outdata = SegAdd( outdata, word );
		word->format.spaces = (_16)*spaces;
		*spaces = 0;
		VarTextAddCharacterEx( out, character DBG_SRC );
		word = VarTextGetEx( out DBG_SRC );
		outdata = SegAdd( outdata, word );
	}
	else
	{
		VarTextAddCharacterEx( out, character DBG_SRC );
		word = VarTextGetEx( out DBG_SRC );

		word->format.spaces = (_16)*spaces;
		*spaces = 0;
		outdata = SegAdd( outdata, word );
	}
   return outdata;
}
#define BreakAndAdd(c) outdata = BreakAndAddEx( c, outdata, &out, &spaces )

//----------------------------------------------------------------------------
// translation trigraphs.....
/*
allll occurances of the following three character sets are
replaced without regard.
??= #  
??( [
??/ \
??) ]
??' ^
??< {
??! |
??> }
??- ~
*/

char *test = "??= #"
"??( ["
"??/\a \\"
"??) ]"
"??' ^"
"??< {"
"??! |"
"??> }"
"??- ~";

//void junk(void ) <% char test <:5:>; %>

static union {
		struct {
			_32 bLesser  : 1;
			_32 bGreater : 1;
			_32 bColon   : 1;
			_32 bPercent : 1;
			_32 bQuestion1 : 1;
			_32 bQuestion2 : 1;
		};
      _32 dw;
	} flags;

#define DBG_OVERRIDE DBG_RELAY

PTEXT OutputDanglingCharsEx( PTEXT outdata, VARTEXT *out, _32 *spaces)
#define OutputDanglingChars() outdata = OutputDanglingCharsEx( outdata, &out, &spaces )
{                               
   int n = 0;                   
	if( flags.bPercent )         
	{                            
      n++;                      
		outdata = BreakAndAddEx( '%', outdata, out, spaces );
		flags.bPercent = 0;       
	}                            
	if( flags.bColon )           
	{                            
      n++;                      
		outdata = BreakAndAddEx( ':', outdata, out, spaces  );
		flags.bColon = 0;
	}
	if( flags.bLesser )
	{
      n++;
		outdata = BreakAndAddEx( '<', outdata, out, spaces  );
		flags.bLesser = 0;
	}
	if( flags.bGreater )
	{
      n++;
		outdata = BreakAndAddEx( '>', outdata, out, spaces  );
		flags.bGreater = 0;
	}
	if( flags.bQuestion2 )
	{
      n++;
		outdata = BreakAndAddEx( '?', outdata, out, spaces  );
		outdata = BreakAndAddEx( '?', outdata, out, spaces  );
		flags.bQuestion2 = 0;
		flags.bQuestion1 = 0;
	}
	if( flags.bQuestion1 )
	{
      n++;
		outdata = BreakAndAddEx( '?', outdata, out, spaces  );
		flags.bQuestion1 = 0;     
	}                            
	if( n > 1 )                  
	{                            
		fprintf( stderr, WIDE("%s(%d): Fixed %d dangling character - perhaps misordered output\n")
				 , GetCurrentFileName(), GetCurrentLine()
				 , n
				 );
	}
   return outdata;
}


PTEXT burstEx( PTEXT input DBG_PASS )
// returns a TEXT list of parsed data
{
//#define DBG_OVERRIDE DBG_SRC
   /* takes a line of input and creates a line equivalent to it, but
      burst into its block peices.*/
   VARTEXT out;
   PTEXT outdata=(PTEXT)NULL,
         word;
   char *tempText;

   _32 index;
   INDEX size;
   _8 character;
	_32 elipses = FALSE
       , spaces = 0
       , escape = 0
	    , quote = 0; // just used for bi-graph/tri-graph stuff...
	flags.dw = 0;
   if (!input)        // if nothing new to process- return nothing processed.
      return((PTEXT)NULL);

	VarTextInitEx( &out DBG_OVERRIDE );

   while (input)  // while there is data to process...
   {
      tempText = GetText(input);  // point to the data to process...
      size = GetTextSize(input);
      if( spaces )
      {
      	//Log( WIDE("Need to figure out - new word, new spaces? old word? new spaces?") );
	      //outdata = Collapse( outdata );
         //outdata->format.spaces = spaces;
         //spaces = 0;
         //set_offset = TRUE;
      }
      if( input->format.spaces )
      {
      	word = VarTextGetEx( &out DBG_OVERRIDE );
      	if( word )
      	{
      		word->format.spaces = (_16)spaces;
      		spaces = 0;
      		outdata = SegAdd( outdata, word );
      	}
      }
      spaces += input->format.spaces;
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
	         		word->format.spaces = (_16)spaces;
   	      		spaces = 0;
      	      	outdata = SegAdd( outdata, word );
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

			if( !quote )
			{
 	 			if( character == '<' )
 	 			{
 	 				if( flags.bLesser )
 	 				{
                   BreakAndAdd( '<' );
 	 				}
 	 				else
 	 				{
                   OutputDanglingChars();
 	 					flags.bLesser = 1;
 	 				}
 	 				continue;
 	 			}
 	 			else if( character == '>' )
				{
 	 				if( flags.bGreater )
 	 				{
                   BreakAndAdd( '>' );
 	 				}
 	 				else if( flags.bColon )
 	 				{
                   BreakAndAdd( ']' );
 	 					flags.bColon = 0;
 	 				}
 	 				else if( flags.bPercent )
 	 				{
 	 					BreakAndAdd( '}' );
 	 					flags.bPercent = 0;
 	 				}
 	 				else
 	 				{
                   OutputDanglingChars();
                   flags.bGreater = 1;
 	 				}
 	 				continue;
 	 			}
 	 			else if( character == ':' )
 	 			{
 	 				if( flags.bLesser )
 	 				{
						BreakAndAdd( '[' );
						flags.bLesser = 0;
 	 				}
 	 				else if( flags.bPercent )
 	 				{
 	 					BreakAndAdd( '#' );
 	 					flags.bPercent = 0;
 	 				}
 	 				else if( flags.bColon )
 	 				{
 	 					BreakAndAdd( ':' );
 	 				}
 	 				else
 	 				{
						OutputDanglingChars();
						flags.bColon = 1;
 	 				}
 	 				continue;
 	 			}
				else if( character == '%' )
 	 			{
 	 				if( flags.bLesser )
					{
						BreakAndAdd( '{' );
						flags.bLesser = 0;
					}
 	 				else if( flags.bPercent )
 	 				{
                   BreakAndAdd( '%' );
 	 				}
 	 				else
 	 				{
                   OutputDanglingChars();
 	 					flags.bPercent = 1;
 	 				}
 	 				continue;
				}
				else
               OutputDanglingChars();
			}
			else if( flags.bQuestion2 )
			{
				if( character == '<' )
				{
					BreakAndAdd( '{' );
               flags.bQuestion2 = 0;
					flags.bQuestion1 = 0;
				}
				else if( character == '>' )
				{
					BreakAndAdd( '}' );
               flags.bQuestion2 = 0;
					flags.bQuestion1 = 0;
				}
				else if( character == '=' )
				{
 	 				BreakAndAdd( '#' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( character == '(' )
 	 			{
 	 				BreakAndAdd( '[' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( character == '/' )
 	 			{
 	 				BreakAndAdd( '\\' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( flags.bQuestion2 &&
 	 					  character == ')' )
 	 			{
 	 				BreakAndAdd( ']' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( flags.bQuestion2 &&
 	 					  character == '\'' )
 	 			{
 	 				BreakAndAdd( '^' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( flags.bQuestion2 &&
 	 					  character == '<' )
 	 			{
 	 				BreakAndAdd( '{' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( flags.bQuestion2 &&
 	 					  character == '!' )
 	 			{
 	 				BreakAndAdd( '|' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( flags.bQuestion2 &&
 	 					  character == '>' )
 	 			{
 	 				BreakAndAdd( '}' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
 	 			}
 	 			else if( flags.bQuestion2 &&
 	 					  character == '-' )
 	 			{
 	 				BreakAndAdd( '~' );
                flags.bQuestion2 = 0;
                flags.bQuestion1 = 0;
                continue;
				}
				else if( character == '?' )
				{
               BreakAndAdd( '?' );
				}
				else
				{
					fprintf( stderr, WIDE("%s(%d): Error unrecognized trigraph sequence!\n")
							, GetCurrentFileName()
							, GetCurrentLine() );
               OutputDanglingChars();
				}
			}
			else if( g.flags.do_trigraph &&
					  character == '?' )
			{
				if( flags.bQuestion2 )
				{
               BreakAndAdd( '?' );
				}
				else if( flags.bQuestion1 )
				{
               flags.bQuestion2 = 1;
				}
				else
					flags.bQuestion1 = 1;
            continue;
			}
			else
			{
				OutputDanglingChars();
			}

			if( !quote )
			{
				if( character == '\'' ||
					character == '\"' )
					quote = character;
			}
			else
			{
				if( !escape && quote == character )
					quote = 0;
				else if( !escape )
					if( character == '\\' )
					{
						escape = 1;
					}
					else
						escape = 0;
            else
					escape = 0;
			}

         switch(character)
         {
         case '\n':
            if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
            {
            	word->format.spaces = (_16)spaces;
            	spaces = 0; // fake a space next line...
            	outdata = SegAdd( outdata, word );
            }
            outdata = SegAdd( outdata, SegCreate( 0 ) ); // add a line-break packet
            break;
         case ' ':
         case '\t':
            if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
            {
            	word->format.spaces = (_16)spaces;
            	spaces = 0;
            	outdata = SegAdd( outdata, word );
            }
            spaces++;
            break;
         case '\r': // a space space character...
            if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
            {
            	word->format.spaces = (_16)spaces;
            	spaces = 0;
            	outdata = SegAdd( outdata, word );
            }
				break;

         case '.': // handle multiple periods grouped (elipses)
            //goto NormalPunctuation;
            {
               char c;
               if( ( !elipses &&
                     ( c = NextChar() ) &&
                     ( c == '.' ) ) )
               {
                  if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
                  {
                  	outdata = SegAdd( outdata, word );
                     word->format.spaces = (_16)spaces;
                     spaces = 0;
                  }
                  VarTextAddCharacterEx( &out, '.' DBG_OVERRIDE );
                  elipses = TRUE;
                  break;
               }
               if( ( c = NextChar() ) &&
                   ( c >= '0' && c <= '9' ) )
               {
                  // gather together as a floating point number...
                  VarTextAddCharacterEx( &out, '.' DBG_OVERRIDE );
                  break;
               }
				}
         case '\'': // single quote bound
			case '\"': // double quote bound
			case '\\': // escape next thingy... unusable in c processor

         case '(': // expression bounders
  			case '{':
  			case '[':
         case ')': // expression closers
         case '}':
         case ']':

         case '-':  // work seperations flaming-long-sword
         case '@':  // email addresses
         case '/':
         case ',':
         case ';':
         case '!':
         case '?':
         case '=':
         case '+':
         case '*':
         case '&':
			case '|':
         case '$':
         case '^':
         case '~':
         case '#':
  			case '`':
            BreakAndAdd( character );
            break;

         default:
            if( elipses )
            {
            	if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) )
            	{
            		outdata = SegAdd( outdata, word );
            		word->format.spaces = (_16)spaces;
                  spaces = 0;
            	}
               elipses = FALSE;
            }
            VarTextAddCharacterEx( &out, character DBG_OVERRIDE );
            break;
			}
      }
      input=NEXTLINE(input);
   }

	if( flags.bPercent )
	{
		BreakAndAdd( '%' );
		flags.bPercent = 0;
	}
	if( flags.bColon )
	{
		BreakAndAdd( ':' );
		flags.bColon = 0;
	}
	if( flags.bLesser )
	{
		BreakAndAdd( '<' );
		flags.bLesser = 0;
	}
	if( flags.bGreater )
	{
		BreakAndAdd( '>' );
		flags.bGreater = 0;
	}
	if( flags.bQuestion2 )
	{
		BreakAndAdd( '?' );
		flags.bQuestion2 = 0;
	}
	if( flags.bQuestion1 )
	{
		BreakAndAdd( '?' );
		flags.bQuestion1 = 0;
	}
   if( ( word = VarTextGetEx( &out DBG_OVERRIDE ) ) ) // any generic outstanding data?
   {
   	outdata = SegAdd( outdata, word );
      word->format.spaces = (_16)spaces;
      spaces = 0;
   }

   SetStart(outdata);

	VarTextEmptyEx( &out DBG_OVERRIDE );
	if( g.bDebugLog & DEBUG_READING )
	{
		fprintf( stddbg, WIDE("Returning segments:") );
		DumpSegs( outdata );
		fprintf( stddbg, WIDE("\n") );
	}
   return(outdata);
}



PTEXT get_line(FILE *source, int *line)
{
   #define WORKSPACE 128  // characters for workspace
   PTEXT workline=(PTEXT)NULL,pNew;
   _32 length = 0;
   if( !source )
      return NULL;
   do
   {
   LineContinues:
      // create a workspace to read input from the file.
      workline=SegAdd(workline,pNew=SegCreate(WORKSPACE));
      //workline = pNew;
      // SetEnd( workline );

      // read a line of input from the file.
      if( !fgets( GetText(workline), WORKSPACE, source) ) // if no input read.
      {
         if (PRIORLINE(workline)) // if we've read some.
         {
            PTEXT t;
            t=PRIORLINE(workline); // go back one.
            SegBreak(workline);
            LineRelease(workline);  // destroy the current segment.
            workline = t;
         }
         else
         {
            LineRelease(workline);            // destory only segment.
            workline = NULL;
         }
         break;  // get out of the loop- there is no more to read.
      }
      {
      	// this section of code shall map character trigraphs into a single
      	// character... this preprocessor should/shall be unicode in nature
      	// the FUTURE man not the past!
      }
      length = strlen(GetText(workline));  // get the length of the line.
      if( workline )
         workline->data.size = length;
   }
	while (GetText(workline)[length-1]!='\n'); //while not at the end of the line.
	if( length > 2 )
	{
		// auto drop \r from \r\n ...
      // since Linux refuses to be kind to dumb animals...
		if( GetText(workline)[length-2] == '\r' )
		{
         GetText(workline)[length-2] = GetText(workline)[length-1];
         length--;
		}
	}
   (*line)++;
   if( workline && (GetText(workline)[length-1]=='\n' ) )
   {
   	if( length > 1 && GetText(workline)[length-2] == '\\' )
		{
			workline->data.data[length-2] = 0;
	      workline->data.size = length-2;
	      goto LineContinues;
		}   		
		else
		{
			// consider the possibility of ignoring whitespace between a \ \n
			// this will ease some effort on editing to kill end of line spaces...
			// but as of yet is non standard - and is non comilant to iso9899:1999
	      workline->data.data[length-1] = 0;
   	   workline->data.size = length-1;
		}
   }
   if (workline)  // if I got a line, and there was some length to it.
      SetStart(workline);   // set workline to the beginning.
   return(workline);      // return the line read from the file.
}

