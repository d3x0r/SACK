// Parses commands similar to bash...
//   What does that mean? 
//   " " bind parameters also! and will generate a continuation prompt
//   \" is not a quote...
//   $(...) will eventually run the command and result as the parameter(s)
//   '\ ' is a space
//   \n <newline>
//   \x## hex number storage
//   \0### octal number conversion
//   $VAR  parses variables before % substitutions take place...
//   handles and redirection don't mean much yet...
//   suppose there's much more than this but mostly I wanted
//   to provide \ escape processing, but did not want to add it as
//   the default....

#include <stdhdrs.h>

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE
#include "plugin.h"


static int myTypeID;


typedef struct mydatapath_tag {
   DATAPATH common;
   PSENTIENT ps;
   struct {
	   _32 escape : 1;      // set at '\'
	   _32 collect_hex : 1; // set at '\x'
	   _32 collect_oct : 1; // set at '\0'
	   _32 quoted : 1;      // set at "
	   _32 var    : 1;      // collecting name of a variable...
	   _32 enqued : 1;      // have enqueued something during a buffer eval
		_32 comment : 1;
		_32 no_blank_lines : 1;
	} flags;
	_32 valuebuffer;
	_32 spaces;
   FORMAT format;

	PVARTEXT vartext;

   PTEXT partial;
   PLINKQUEUE Output;
} MYDATAPATH, *PMYDATAPATH;

//---------------------------------------------------------------------------

enum {
	  OP_BACKSPACE
	, OP_DELETE
	, OP_ESCAPE
	, OP_QUOTE
	, OP_TAB
	, OP_NEWLINE
	, OP_RETURN
	, OP_CHAR
	, OP_COLLECT
	, OP_OPAREN
	, OP_CPAREN
};

//int ops[256] = { 0, 0, 0, 0, 0, 0, 0, 0
//					, OP_BACKSPACE, OP_TAB, OP_NEWLINE, 0, 0, OP_RETURN, 0, 0
//					, 0, 0, 0, 0, 0, 0, 0, 0
//					, 0, 0, 0, 0, 0, 0, 0, 0
//					, OP_SPACE, OP_CHAR, OP_QUOTE, OP_CHAR, OP_COLLECT, #$%&'
//					, OP_OPAREN, OP_CPAREN, *+,-./
//			/*48*/, 01234567
//			/*56*/, 89:;<=>?
//			/*64*/, @ABCDEFG
//			/*72*/, HIJKLMNO
//			/*80*/, PQRSTUVW
//			/*88*/, XYZ[\]^_
//			/*96*/, `abcdefg
//			/*104*/,hijklmno
//			/*112*/,pqrstuvw
//			/*120*/,xyz{|}~<del>
//			/*128*/, all above characters are what? OP_CHAR probably... 


//---------------------------------------------------------------------------

static int BreakCollection( PMYDATAPATH pdp )
{
	PTEXT segs = VarTextGet( pdp->vartext );
	if( segs )
	{
		if( pdp->flags.var )
		{
			PTEXT saveseg = segs;
			PTEXT var = SubstToken( pdp->ps, &saveseg, TRUE, FALSE );
			if( var )
			{
				// commit is probably NULL here (in this usage)
				// since it's a monotonic token....
				var = TextDuplicate( var, FALSE );
				if( pdp->partial )
					var->format.position.offset.spaces = (_16)pdp->spaces;
			}
			pdp->partial = SegAppend( pdp->partial, var );
			pdp->flags.var = 0;
			LineRelease( segs );
		}
		else
		{
			if( pdp->partial )
				segs->format.position.offset.spaces = (_16)pdp->spaces;
			pdp->partial = SegAppend( pdp->partial, segs );
		}
		pdp->spaces = 0;
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------

static void EndCollection( PMYDATAPATH pdp )
{
	if( BreakCollection( pdp ) || 
		!pdp->flags.enqued ||
       pdp->partial ) // already collected something
	{
		//Log( "Ending the line" );
		if( !pdp->partial && // only add a newline if nothing on line.
			 !pdp->flags.no_blank_lines ) 
			pdp->partial = SegAppend( pdp->partial, SegCreate(0) ); // add end of line!

		if( pdp->partial )
		{
			//{
			//	PTEXT tmp;
			//	tmp = BuildLine( pdp->partial );
		  // 	Log1( "Line is: %s", GetText( tmp ) );
		  // 	LineRelease( tmp );
			///}

			EnqueLink( &pdp->Output, pdp->partial );
   		pdp->flags.enqued = 1;
		}
		pdp->flags.comment = 0; // out of collecting commented data.
		pdp->partial = NULL;
	}
}

//---------------------------------------------------------------------------

static void FinishCollection( PMYDATAPATH pdp )
{
	EndCollection( pdp );
	VarTextEmpty( pdp->vartext );
}

//---------------------------------------------------------------------------

static PTEXT CPROC ParseCommand( PMYDATAPATH pdp, PTEXT buffer )
{
		//	{
		//		PTEXT tmp;
		//		tmp = BuildLine(  buffer );
		//		Log1( "Read is: %s", GetText( tmp ) );
		//		LineRelease( tmp );
		//	}
    // this should really do it's own full symbolic bursting...
    // except escapes, tabs, etc are kept intact....
    // words with '\ ' in them are continuous where a ' ' actually
    // IS a seperator...
	if( buffer )
   {
      PTEXT seg;
      //Log2( "buffer: %s(%d)", GetText( buffer ), GetTextSize( buffer ) );
      //LogBinary( buffer );
      pdp->flags.enqued = 0;
      seg = buffer;
		while( seg )
		{
			TEXTCHAR character, *text;
			size_t n, len;
			len = GetTextSize( seg );
			text = GetText( seg );
			if( !(seg->flags & (TF_FORMATPOS) ) )
			{
				if( seg->format.position.offset.spaces )
				{
					BreakCollection( pdp );
					pdp->spaces += seg->format.position.offset.spaces;
				}
			}
			if( !len )
			{  // a zero length segment is still a valid terminator.
				if( !(seg->flags & TF_SENTIENT) )
					EndCollection( pdp );
			}
			for( n = 0; n < len; n++ )
			{
				character = text[n];
reevaluate_character:
				if( pdp->flags.escape )
				{
					if( pdp->flags.collect_hex )
					{
						if( character >= '0' && character <= '9' )
						{
							pdp->valuebuffer *= 16;
							pdp->valuebuffer += character - '0';
						}
						else if( character >= 'A' && character <= 'F' )
						{
							pdp->valuebuffer *= 16;
							pdp->valuebuffer += character - 'A' + 10;
						}
						else if( character >= 'a' && character <= 'f' )
						{
							pdp->valuebuffer *= 16;
							pdp->valuebuffer += character - 'a' + 10;
						}
						else
						{
							_32 value = pdp->valuebuffer;
							do
							{
								VarTextAddCharacter( pdp->vartext, (TEXTCHAR)(value % 256) );
								value /= 256;
							}while( value );
							pdp->flags.collect_hex = 0;
							pdp->flags.escape = 0;
							goto reevaluate_character;
						}
					}
					else if( pdp->flags.collect_oct )
					{
						if( character >= '0' && character <= '7' )
						{
							pdp->valuebuffer *= 8;
							pdp->valuebuffer += character - '0';
						}
						else
						{
							_32 value = pdp->valuebuffer;
							do
							{
								VarTextAddCharacter( pdp->vartext, (TEXTCHAR)(value % 256) );
								value /= 256;
							}while( value );
							pdp->flags.collect_oct = 0;
							pdp->flags.escape = 0;
							goto reevaluate_character;
						}
					}
					else switch( character )
					{
					case '#':
					case '$':
					case '\"':
					case '\'':
					case '\\':
					case ' ':
					case ';':
						VarTextAddCharacter( pdp->vartext, character );
						if( !pdp->flags.quoted )
							BreakCollection( pdp );
						pdp->flags.escape = 0;
						break;
					case 'n':
						VarTextAddCharacter( pdp->vartext, '\n' );
						if( !pdp->flags.quoted )
							BreakCollection( pdp );
						pdp->flags.escape = 0;
						break;
					case 't':
						VarTextAddCharacter( pdp->vartext, '\t' );
						if( !pdp->flags.quoted )
							BreakCollection( pdp );
						pdp->flags.escape = 0;
						break;
					case 'x':
						pdp->flags.collect_hex = 1;
						pdp->valuebuffer = 0;
						break;
					case '0':
						pdp->flags.collect_oct = 1;
						pdp->valuebuffer = 0;
						break;
					case '.': // \. is a force break (no data generated)
						if( !pdp->flags.quoted )
							BreakCollection( pdp ); // primary purpose of '\'
						pdp->flags.escape = 0;
						break;
					case '\r':
					case '\n':
                  // is okay - just ignore this and all will be well...
                  break;
					default:
						Log1( WIDE("Unknown escape passed! \\%c"), character );
						VarTextAddCharacter( pdp->vartext, '\\' );
						VarTextAddCharacter( pdp->vartext, character );
						pdp->flags.escape = 0;
						break;
					}
				}
				else if( pdp->flags.quoted )
				{
					if( character == '\"' )
					{
						BreakCollection( pdp );
						pdp->flags.quoted = 0;
					}
					else if( character == '\\' )
					{
						pdp->flags.escape = 0;
					}
					else
						VarTextAddCharacter( pdp->vartext, character );
				}
				else
				{
					switch( character )
					{
					case '.':
						if( !pdp->flags.comment )
						{
							if( !pdp->partial && !VarTextLength( pdp->vartext ) )
							{
								VarTextAddCharacter( pdp->vartext, character );
								BreakCollection( pdp );
							}
							else
								VarTextAddCharacter( pdp->vartext, character );
						}
						break;
					case '\\':
						if( !pdp->flags.comment )
							pdp->flags.escape = 1;
						if( !pdp->flags.quoted )
							BreakCollection( pdp );
						break;
					case '\"':
						if( !pdp->flags.comment )
							pdp->flags.quoted = 1;
						break;
					case '\t':
						if( !pdp->flags.comment )
						{
							BreakCollection( pdp );
							pdp->spaces++;
						}
						break;
					case ' ':
						if( !pdp->flags.comment )
						{
							BreakCollection( pdp );
							pdp->spaces++;
						}
						break;
					case '\r': // shouldn't happen but we'll just ignore it...
						break;
					case ';':
						//pdp->flags.added = 1; // fake it out... 
					case '\n':
                  if( !pdp->flags.escape )
							EndCollection( pdp );
						break;
					case '$':
						if( !pdp->flags.comment )
						{
							Log( WIDE("Beginning variable... "));
							BreakCollection( pdp );
							pdp->flags.var = 1;
						}
						break;
					case '\'': // often use apostrophies in macros... 
					case '%':
					case '/':
					case '*':
					case '+':
					case '-':
					case '^':
					case '|':
					case '=':
					case ',':
               case '&':
               case '(':
               case ')':
						if( !pdp->flags.comment )
						{
							BreakCollection( pdp );
							VarTextAddCharacter( pdp->vartext, character );
							BreakCollection( pdp );
						}
						break;
					case '#':
						if( !pdp->flags.comment )
						{
							BreakCollection( pdp );
							pdp->flags.comment = 1;
						}
						break;
					default:
						if( !pdp->flags.comment )
							VarTextAddCharacter( pdp->vartext, character );
						break;
					}
				}
			}
			seg = NEXTLINE( seg );
		}
      if( !pdp->flags.escape ) // else continue...
			FinishCollection( pdp );
		else
         pdp->flags.escape = 0; // end of line... collect new line... is not escape
		LineRelease( buffer ); // done with this old thing...
	}
   /*
	if( IsQueueEmpty( &pdp->Output ) )
		lprintf( "Bash command queue empty... resulted in no data..." );
	else
	   lprintf( "Resulting in data input... (bash parsed)" );
   */
	return (PTEXT)DequeLink( &pdp->Output );
}

//---------------------------------------------------------------------------

static int CPROC Read( PMYDATAPATH pdp )
{
	return RelayInput( (PDATAPATH)pdp, (PTEXT(CPROC *)(PDATAPATH,PTEXT))ParseCommand );
}

//---------------------------------------------------------------------------

static int CPROC Write( PMYDATAPATH pdp )
{
   //Log( "BASH Write called to relay data out..." );
	return RelayOutput( (PDATAPATH)pdp, NULL );
}

//---------------------------------------------------------------------------

static int CPROC Close( PMYDATAPATH pdp )
{
	PTEXT temp;
   pdp->common.Type = 0;
	VarTextDestroy( &pdp->vartext );
	if( pdp->partial )
		LineRelease( pdp->partial );
	RelayOutput( (PDATAPATH)pdp, NULL );
	//while( temp = DequeLink( &pdp->Output ) )
	//	LineRelease( temp );
	DeleteLinkQueue( &pdp->Output );
   return 0;
}

//---------------------------------------------------------------------------

#define OptionLike(text,string) ( StrCaseCmpEx( GetText(text), string, GetTextSize( text ) ) == 0 )

static PDATAPATH CPROC Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PMYDATAPATH pdp = NULL;
   //PTEXT option;
   // parameters
   //    none
	PTEXT pText;
   pdp = CreateDataPath( pChannel, MYDATAPATH );
	while( ( pText = GetParam( ps, &parameters ) ) )
	{
		if( OptionLike( pText, WIDE("noblank") ) )
			pdp->flags.no_blank_lines = 1;
	}
   pdp->common.Type = myTypeID;
   pdp->common.Read = (int(CPROC *)(PDATAPATH))Read;
   pdp->common.Write = (int(CPROC *)(PDATAPATH))Write;
   pdp->common.Close = (int(CPROC *)(PDATAPATH))Close;
   pdp->ps = ps;
   pdp->vartext = VarTextCreate();
   return (PDATAPATH)pdp;
}

//---------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{                           
   myTypeID = RegisterDevice( WIDE("bash"), WIDE("Parse commands bashlike"), Open );
   return DekVersion;
}

//---------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterDevice( WIDE("bash") );
}

//---------------------------------------------------------------------------
// $Log: bash.c,v $
// Revision 1.31  2005/02/21 12:08:30  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.30  2005/01/17 08:45:42  d3x0r
// Make open method static in a bunch of these
//
// Revision 1.29  2004/04/05 21:15:49  d3x0r
// Sweeping update - macros, probably some logging enabled
//
// Revision 1.28  2004/01/21 07:25:13  d3x0r
// Remove noisy logging
//
// Revision 1.27  2004/01/19 23:42:25  d3x0r
// Misc fixes for merging cursecon with psi/win con interfaces
//
// Revision 1.26  2003/11/08 00:09:40  panther
// fixes for VarText abstraction
//
// Revision 1.25  2003/10/26 12:38:16  panther
// Updates for newest scheduler
//
// Revision 1.24  2003/10/26 11:43:53  panther
// Updated to new Relay I/O system
//
// Revision 1.23  2003/08/20 08:05:48  panther
// Fix for watcom build, file usage, stuff...
//
// Revision 1.22  2003/08/15 13:14:45  panther
// Handle more cases of common escape sequences
//
// Revision 1.21  2003/07/17 16:46:30  panther
// Remove LogBinary - moved to common syslog
//
// Revision 1.20  2003/04/20 16:19:53  panther
// Fixes for history usage... cleaned some logging messages.
//
// Revision 1.19  2003/04/06 09:59:49  panther
// Remove some logging, ignore FORMATPOS data
//
// Revision 1.18  2003/01/29 06:24:26  panther
// More leakage fixes - don't encode blank lines for TF_SENTIENTS - still think there's a hole here
//
// Revision 1.17  2003/01/29 06:05:32  panther
// Clean up datapath information on close
//
// Revision 1.16  2003/01/28 22:03:58  panther
// Add support to drop blank lines on bash
//
// Revision 1.15  2003/01/28 16:40:30  panther
// Updated documentation.  Updated features to get the active device.
// Always hook trigger closest to the object - allows multiple connection
// layering better. Version updates.
//
// Revision 1.14  2003/01/28 02:23:54  panther
// Fixes for common device scripts, some minor tweaks to commands
//
// Revision 1.13  2003/01/27 14:37:25  panther
// Updated projects from old to new, included missing projects
//
// Revision 1.12  2003/01/22 11:09:50  panther
// Cleaned up warnings issued by Visual Studio
//
// Revision 1.11  2002/09/27 07:14:57  panther
// Trailing line conditions fixed.
//
// Revision 1.10  2002/09/09 02:58:13  panther
// Conversion to new make system - includes explicit coding of exports.
//
// Revision 1.9  2002/07/28 14:11:24  panther
// Modified \ handling to stop continual consumption at end of line.
//
//
