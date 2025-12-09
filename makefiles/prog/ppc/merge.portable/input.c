
#include "input.h" // includes text.h...
#include "global.h"

#ifdef __cplusplus
namespace d3x0r {
namespace ppc {
#endif

#define Collapse( towhere ) SegConcat( towhere, begin, beginoffset, total )

//----------------------------------------------------------------------

static char PPC_NextCharEx( PTEXT input, INDEX idx ) {
	if( ( ++idx ) >= input->data.size ) {
		idx -= input->data.size;
		input = NEXTLINE( input );
	}
	if( input )
		return input->data.data[ idx ];
	return 0;
}
#define NextChar() PPC_NextCharEx( input, index )
//----------------------------------------------------------------------

static PTEXT BreakAndAddEx( char character,
                            PTEXT outdata,
                            PVARTEXT out,
                            uint32_t *spaces,
                            uint32_t *tabs ) {
	PTEXT word;

	if( ( word = VarTextGetEx( out DBG_SRC ) ) ) {
		outdata                             = SegAppend( outdata, word );
		word->format.position.offset.spaces = (uint16_t)*spaces;
		word->format.position.offset.tabs   = (uint16_t)*tabs;
		*spaces                             = 0;
		*tabs                               = 0;
		VarTextAddCharacterEx( out, character DBG_SRC );
		word    = VarTextGetEx( out DBG_SRC );
		outdata = SegAppend( outdata, word );
	} else {
		VarTextAddCharacterEx( out, character DBG_SRC );
		word                                = VarTextGetEx( out DBG_SRC );

		word->format.position.offset.spaces = (uint16_t)*spaces;
		word->format.position.offset.tabs   = (uint16_t)*tabs;
		*spaces                             = 0;
		*tabs                               = 0;
		outdata                             = SegAppend( outdata, word );
	}
	return outdata;
}
#define BreakAndAdd( c )                                                       \
	outdata = BreakAndAddEx( c, outdata, out, &spaces, &tabs )

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

// void junk(void ) <% char test <:5:>; %>

#define DBG_OVERRIDE DBG_RELAY

static PTEXT OutputDanglingCharsEx( PTEXT outdata,
                                    PVARTEXT out,
                                    uint32_t *spaces,
                                    uint32_t *tabs )
#define OutputDanglingChars()                                                  \
	outdata = OutputDanglingCharsEx( outdata, out, &spaces, &tabs )
{
	int n = 0;
	if( g.input_flags.bPercent ) {
		n++;
		outdata        = BreakAndAddEx( '%', outdata, out, spaces, tabs );
		g.input_flags.bPercent = 0;
	}
	if( g.input_flags.bColon ) {
		n++;
		outdata      = BreakAndAddEx( ':', outdata, out, spaces, tabs );
		g.input_flags.bColon = 0;
	}
	if( g.input_flags.bLesser ) {
		n++;
		outdata       = BreakAndAddEx( '<', outdata, out, spaces, tabs );
		g.input_flags.bLesser = 0;
	}
	if( g.input_flags.bGreater ) {
		n++;
		outdata        = BreakAndAddEx( '>', outdata, out, spaces, tabs );
		g.input_flags.bGreater = 0;
	}
	if( g.input_flags.bQuestion2 ) {
		n++;
		outdata          = BreakAndAddEx( '?', outdata, out, spaces, tabs );
		outdata          = BreakAndAddEx( '?', outdata, out, spaces, tabs );
		g.input_flags.bQuestion2 = 0;
		g.input_flags.bQuestion1 = 0;
	}
	if( g.input_flags.bQuestion1 ) {
		n++;
		outdata          = BreakAndAddEx( '?', outdata, out, spaces, tabs );
		g.input_flags.bQuestion1 = 0;
	}
	if( n > 1 ) {
		fprintf(
		     stderr,
		     "%s(%d): Fixed %d dangling character - perhaps misordered output\n",
		     GetCurrentFileName(), GetCurrentLine(), n );
	}
	return outdata;
}

PTEXT PPC_burstEx( PTEXT input DBG_PASS )
// returns a TEXT list of parsed data
{
	// #define DBG_OVERRIDE DBG_SRC
	/* takes a line of input and creates a line equivalent to it, but
	   burst into its block peices.*/
	PVARTEXT out  = VarTextCreate();
	PTEXT outdata = (PTEXT)NULL, word;
	char *tempText;

	uint32_t index;
	INDEX size;
	uint8_t character;
	uint32_t elipses = FALSE, spaces = 0, tabs = 0, escape = 0,
	         quote = 0; // just used for bi-graph/tri-graph stuff...
	g.input_flags.dw       = 0;
	if( !input ) // if nothing new to process- return nothing processed.
		return ( (PTEXT)NULL );

	// VarTextInitEx( &out DBG_OVERRIDE );

	while( input ) // while there is data to process...
	{
		tempText = GetText( input ); // point to the data to process...
		size     = GetTextSize( input );
		if( spaces ) {
			// Log( "Need to figure out - new word, new spaces? old word? new
			// spaces?" ); outdata = Collapse( outdata );
			// outdata->format.position.offset.spaces = spaces;
			// spaces = 0;
			// set_offset = TRUE;
		}
		if( input->format.position.offset.spaces
		    || input->format.position.offset.tabs ) {
			word = VarTextGetEx( out DBG_OVERRIDE );
			if( word ) {
				word->format.position.offset.spaces = (uint16_t)spaces;
				word->format.position.offset.tabs   = (uint16_t)tabs;
				spaces                              = 0;
				tabs                                = 0;
				outdata                             = SegAppend( outdata, word );
			}
		}
		spaces += input->format.position.offset.spaces;
		tabs += input->format.position.offset.tabs;
		// Log1( "Assuming %d spaces... ", spaces );
		for( index = 0; ( character = tempText[ index ] ),
		     ( index < size );
		     index++ ) // while not at the
		               // end of the line.
		{
			if( elipses && character != '.' ) {
				if( VarTextEndEx( out DBG_OVERRIDE ) ) {
					PTEXT word = VarTextGetEx( out DBG_OVERRIDE );
					if( word ) {
						word->format.position.offset.spaces = (uint16_t)spaces;
						word->format.position.offset.tabs   = (uint16_t)tabs;
						spaces                              = 0;
						tabs                                = 0;
						outdata = SegAppend( outdata, word );
					}
					// else
					//	Log( "VarTextGet Failed to result." );
				}
				elipses = FALSE;
			} else if( elipses ) // elipses and character is . - continue
			{
				VarTextAddCharacterEx( out, character DBG_OVERRIDE );
				continue;
			}

			if( !quote ) {
				if( character == '<' ) {
					if( g.input_flags.bLesser ) {
						BreakAndAdd( '<' );
					} else {
						OutputDanglingChars();
						g.input_flags.bLesser = 1;
					}
					continue;
				} else if( character == '>' ) {
					if( g.input_flags.bGreater ) {
						BreakAndAdd( '>' );
					} else if( g.input_flags.bColon ) {
						BreakAndAdd( ']' );
						g.input_flags.bColon = 0;
					} else if( g.input_flags.bPercent ) {
						BreakAndAdd( '}' );
						g.input_flags.bPercent = 0;
					} else {
						OutputDanglingChars();
						g.input_flags.bGreater = 1;
					}
					continue;
				} else if( character == ':' ) {
					if( g.input_flags.bLesser ) {
						BreakAndAdd( '[' );
						g.input_flags.bLesser = 0;
					} else if( g.input_flags.bPercent ) {
						BreakAndAdd( '#' );
						g.input_flags.bPercent = 0;
					} else if( g.input_flags.bColon ) {
						BreakAndAdd( ':' );
					} else {
						OutputDanglingChars();
						g.input_flags.bColon = 1;
					}
					continue;
				} else if( character == '%' ) {
					if( g.input_flags.bLesser ) {
						BreakAndAdd( '{' );
						g.input_flags.bLesser = 0;
					} else if( g.input_flags.bPercent ) {
						BreakAndAdd( '%' );
					} else {
						OutputDanglingChars();
						g.input_flags.bPercent = 1;
					}
					continue;
				} else
					OutputDanglingChars();
			} else if( g.input_flags.bQuestion2 ) {
				if( character == '<' ) {
					BreakAndAdd( '{' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
				} else if( character == '>' ) {
					BreakAndAdd( '}' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
				} else if( character == '=' ) {
					BreakAndAdd( '#' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( character == '(' ) {
					BreakAndAdd( '[' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( character == '/' ) {
					BreakAndAdd( '\\' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( g.input_flags.bQuestion2 && character == ')' ) {
					BreakAndAdd( ']' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( g.input_flags.bQuestion2 && character == '\'' ) {
					BreakAndAdd( '^' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( g.input_flags.bQuestion2 && character == '<' ) {
					BreakAndAdd( '{' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( g.input_flags.bQuestion2 && character == '!' ) {
					BreakAndAdd( '|' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( g.input_flags.bQuestion2 && character == '>' ) {
					BreakAndAdd( '}' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( g.input_flags.bQuestion2 && character == '-' ) {
					BreakAndAdd( '~' );
					g.input_flags.bQuestion2 = 0;
					g.input_flags.bQuestion1 = 0;
					continue;
				} else if( character == '?' ) {
					BreakAndAdd( '?' );
				} else {
					fprintf( stderr,
					         "%s(%d): Error unrecognized trigraph sequence!\n",
					         GetCurrentFileName(), GetCurrentLine() );
					OutputDanglingChars();
				}
			} else if( g.flags.do_trigraph && character == '?' ) {
				if( g.input_flags.bQuestion2 ) {
					BreakAndAdd( '?' );
				} else if( g.input_flags.bQuestion1 ) {
					g.input_flags.bQuestion2 = 1;
				} else
					g.input_flags.bQuestion1 = 1;
				continue;
			} else {
				OutputDanglingChars();
			}

			if( !quote ) {
				if( character == '\'' || character == '\"' )
					quote = character;
			} else {
				if( !escape && quote == character )
					quote = 0;
				else if( !escape )
					if( character == '\\' ) {
						escape = 1;
					} else
						escape = 0;
				else
					escape = 0;
			}

			switch( character ) {
			case '\n':
				if( ( word = VarTextGetEx( out DBG_OVERRIDE ) ) ) {
					word->format.position.offset.spaces = (uint16_t)spaces;
					word->format.position.offset.tabs   = (uint16_t)tabs;
					spaces  = 0; // fake a space next line...
					tabs    = 0; // fake a space next line...
					outdata = SegAppend( outdata, word );
				}
				outdata = SegAppend( outdata,
				                     SegCreate( 0 ) ); // add a line-break packet
				break;
			case ' ':
			case '\t':
				if( ( word = VarTextGetEx( out DBG_OVERRIDE ) ) ) {
					word->format.position.offset.spaces = (uint16_t)spaces;
					word->format.position.offset.tabs   = (uint16_t)tabs;
					spaces                              = 0;
					tabs                                = 0;
					outdata                             = SegAppend( outdata, word );
				}
				if( character == ' ' )
					spaces++;
				else
					tabs++;
				break;
			case '\r': // a space space character...
				if( ( word = VarTextGetEx( out DBG_OVERRIDE ) ) ) {
					word->format.position.offset.spaces = (uint16_t)spaces;
					word->format.position.offset.tabs   = (uint16_t)tabs;
					spaces                              = 0;
					tabs                                = 0;
					outdata                             = SegAppend( outdata, word );
				}
				break;

			case '.': // handle multiple periods grouped (elipses)
				// goto NormalPunctuation;
				{
					char c;
					if( ( !elipses && ( c = NextChar() ) && ( c == '.' ) ) ) {
						if( ( word = VarTextGetEx( out DBG_OVERRIDE ) ) ) {
							outdata = SegAppend( outdata, word );
							word->format.position.offset.spaces = (uint16_t)spaces;
							word->format.position.offset.tabs   = (uint16_t)tabs;
							spaces                              = 0;
							tabs                                = 0;
						}
						VarTextAddCharacterEx( out, '.' DBG_OVERRIDE );
						elipses = TRUE;
						break;
					}
					if( ( c = NextChar() ) && ( c >= '0' && c <= '9' ) ) {
						// gather together as a floating point number...
						VarTextAddCharacterEx( out, '.' DBG_OVERRIDE );
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

			case '-': // work seperations flaming-long-sword
			case '@': // email addresses
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
				if( elipses ) {
					if( ( word = VarTextGetEx( out DBG_OVERRIDE ) ) ) {
						outdata = SegAppend( outdata, word );
						word->format.position.offset.spaces = (uint16_t)spaces;
						word->format.position.offset.tabs   = (uint16_t)tabs;
						spaces                              = 0;
						tabs                                = 0;
					}
					elipses = FALSE;
				}
				VarTextAddCharacterEx( out, character DBG_OVERRIDE );
				break;
			}
		}
		input = NEXTLINE( input );
	}

	if( g.input_flags.bPercent ) {
		BreakAndAdd( '%' );
		g.input_flags.bPercent = 0;
	}
	if( g.input_flags.bColon ) {
		BreakAndAdd( ':' );
		g.input_flags.bColon = 0;
	}
	if( g.input_flags.bLesser ) {
		BreakAndAdd( '<' );
		g.input_flags.bLesser = 0;
	}
	if( g.input_flags.bGreater ) {
		BreakAndAdd( '>' );
		g.input_flags.bGreater = 0;
	}
	if( g.input_flags.bQuestion2 ) {
		BreakAndAdd( '?' );
		g.input_flags.bQuestion2 = 0;
	}
	if( g.input_flags.bQuestion1 ) {
		BreakAndAdd( '?' );
		g.input_flags.bQuestion1 = 0;
	}
	if( ( word
	      = VarTextGetEx( out DBG_OVERRIDE ) ) ) // any generic outstanding data?
	{
		outdata                             = SegAppend( outdata, word );
		word->format.position.offset.spaces = (uint16_t)spaces;
		word->format.position.offset.tabs   = (uint16_t)tabs;
		spaces                              = 0;
		tabs                                = 0;
	}

	SetStart( outdata );

	VarTextEmptyEx( out DBG_OVERRIDE );
	if( g.bDebugLog & DEBUG_READING ) {
		fprintf( stddbg, "Returning segments:" );
		DumpSegs( outdata );
		fprintf( stddbg, "\n" );
	}
	return ( outdata );
}

PTEXT get_line( FILE *source, int *line ) {
#define WORKSPACE 512 // characters for workspace
	PTEXT workline   = (PTEXT)NULL, pNew;
	uintptr_t length = 0;
	if( !source )
		return NULL;
	do {
	LineContinues:
		// create a workspace to read input from the file.
		workline = SegAppend( workline, pNew = SegCreate( WORKSPACE ) );
		// workline = pNew;
		//  SetEnd( workline );

		// read a line of input from the file.
		if( !fgets( GetText( workline ), WORKSPACE,
		            source ) ) // if no input read.
		{
			if( PRIORLINE( workline ) ) // if we've read some.
			{
				PTEXT t;
				t = PRIORLINE( workline ); // go back one.
				SegBreak( workline );
				LineRelease( workline ); // destroy the current segment.
				workline = t;
			} else {
				LineRelease( workline ); // destory only segment.
				workline = NULL;
			}
			break; // get out of the loop- there is no more to read.
		}
		{
			// this section of code shall map character trigraphs into a single
			// character... this preprocessor should/shall be unicode in nature
			// the FUTURE man not the past!
		}
		length = strlen( GetText( workline ) ); // get the length of the line.
		if( workline )
			workline->data.size = length;
	} while( GetText( workline )[ length - 1 ]
	         != '\n' ); // while not at the end of the line.
	if( workline && length > 2 ) {
		// auto drop \r from \r\n ...
		// since Linux refuses to be kind to dumb animals...
		if( GetText( workline )[ length - 2 ] == '\r' ) {
			GetText( workline )[ length - 2 ] = GetText( workline )[ length - 1 ];
			length--;
		}
	}
	( *line )++;
	if( workline && ( GetText( workline )[ length - 1 ] == '\n' ) ) {
		if( length > 1 && GetText( workline )[ length - 2 ] == '\\' ) {
			workline->data.data[ length - 2 ] = 0;
			workline->data.size               = length - 2;
			goto LineContinues;
		} else {
			// consider the possibility of ignoring whitespace between a \ \n
			// this will ease some effort on editing to kill end of line spaces...
			// but as of yet is non standard - and is non comilant to iso9899:1999
			workline->data.data[ length - 1 ] = 0;
			workline->data.size               = length - 1;
		}
	}
	if( workline )           // if I got a line, and there was some length to it.
		SetStart( workline ); // set workline to the beginning.
	return ( workline );     // return the line read from the file.
}

#ifdef __cplusplus
} // namespace ppc
} // namespace d3x0r
#endif