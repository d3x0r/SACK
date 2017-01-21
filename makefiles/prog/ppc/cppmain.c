// make #if conditions spanning files a warning...
// also unbalanced #endif statements....

#ifndef __GCC__
//#include <conio.h>
#endif
#if defined( _WIN32 )
#include <windows.h> // getmodulefilename
#endif

#include <stdio.h>
#include "fileio.h"
#include "mem.h"
#include "./types.h"
#include "input.h"
#include "text.h"
#include "links.h"
#include <string.h>
#include "expr.h"
#include "define.h"

#define CPP_MAIN_SOURCE
#include "global.h"

// this module shall have provision to read a cpp.cnf
// which will indicate system include paths, and additional symbols
// which may be defined on a per-compiler basis....
	
#ifndef __GCC__
// this was good during development of infinite loops but bad in production...
// maybe ifdef _DEBUG the kbhit();
#define fprintf fprintf
#else
#define fprintf fprintf
#endif

#define ARG_UNKNOWN          0
#define ARG_INCLUDE_PATH     1
#define ARG_SYS_INCLUDE_PATH 2
#define ARG_AUTODEPEND_NAME  3
#define ARG_DEFINE_DEFINE    4
#define ARG_OUT_NAME         5
#define ARG_AUTOTARGET_NAME  6
#define ARG_INCLUDE_FILE     7
#define ARG_MACRO_FILE       8
#define ARG_GEN_STDOUT       9

/******************************
/* this portion is mainly for testing the CPP program
/* defines which are not used...
/* comments in various order
/* although these must succeed...
/******************************/

//----------------------------------------------------------------------

int KillQuotes( char *string )
{
	// this processing stage cannot be done at the pre-processor level
	// for things like "\x02" "4" which is actually 
	// character to followed by ascii 4.
	//return strlen( string );
	// okay but yes it can be done for #pramga message

	// this routine removes leading and trailing quotes.
	// and only stores that which was within the quotes.
	char quote = 0;
	char *in = string, *out = string;
	if( !string )
		return 0;
	while( *string )
	{
		if( !quote )
		{
			if( *string == '\"' || *string == '\'' )
			{
				quote = *string;
			}
		}
		else
		{
			if( *string == quote )
			{
				quote = 0;
			}
			else
			{
				*out = *string;
				out++;
			}
		}
		string++;
	}
	*out = 0;
	return out - in; // bad form - but in never changes so this is length out.
}

//----------------------------------------------------------------------

int CollapseQuotes( char *string )
{
	// this routine takes "thing" "another" and makes "thinganother"
	// however "this" and "that" is still "this" and "that"
	char quote = 0, lastquote = 0, *lastquotepos = NULL;
	char *in = string, *out = string;
	if( !string )
		return 0;
	while( *string )
	{
		if( !quote )
		{
			if( lastquote == *string )
			{
				out = lastquotepos;
				quote = *string;
			}
			else
			{
				if( *string != ' ' && *string != '\t' )
				{
					lastquote = 0;
					lastquotepos = NULL;
				}

				if( *string == '\"' || *string == '\'' )
					quote = *string;
				*out = *string;
				out++;
			}
		}
		else
		{
			if( *string == quote )
			{
				lastquote = quote;
				lastquotepos = out;
				quote = 0;
			}
			*out = *string;
			out++;
		}
		string++;
	}
	*out = 0;
	return out - in; // bad form - but in never changes so this is length out.
}

//---------------------------------------------------------------------------

void DumpSegs( PTEXT pOp )
{
	PTEXT tmp = pOp;
	//if( !g.bDebugLog )
	//	return;
	while( tmp )
	{
		if( tmp->flags & TF_INDIRECT )
		{
			DumpSegs( GetIndirect( tmp ) );
		}
		else
		{
			fprintf( stddbg, WIDE("[%d%s]"), tmp->format.spaces,GetText( tmp ) );
		}
		tmp = NEXTLINE( tmp );
	}
}


//----------------------------------------------------------------------

int ProcessSystemIncludeFile( char *name, int bAllowAbsolute, int bNext )
{
	char Workname[__MAX_PATH__];
	PTEXT pPath;
	INDEX idx;

	if( bAllowAbsolute &&
		 OpenNewInputFile( name, name, GetCurrentFileName(), GetCurrentLine(), g.bAutoDepend, bNext ) )
		return TRUE;
	{
		FORALL( g.pUserIncludePath, idx, PTEXT, pPath )
		{
			if( !idx ) // don't use auto path for 'system' includes.
				continue;
			sprintf( Workname, WIDE("%s/%s"), GetText( pPath ), name );
			if( g.bDebugLog )
			{
				fprintf( stddbg, WIDE("attempting <%s>\n") , Workname );
			}
			if( OpenNewInputFile( name, Workname, GetCurrentFileName(), GetCurrentLine(), TRUE, bNext ) )
			{
				if( idx )
					SetCurrentPath( GetText( pPath ) );
				return TRUE;
			}
		}
		FORALL( g.pSysIncludePath, idx, PTEXT, pPath )
		{
			sprintf( Workname, WIDE("%s/%s"), GetText( pPath ), name );
			if( g.bDebugLog )
			{
				fprintf( stddbg, WIDE("attempting <%s>\n") , Workname );
			}
			g.flags.doing_system_file = 1;
			if( OpenNewInputFile( name, Workname, GetCurrentFileName(), GetCurrentLine(), FALSE, bNext ) )
			{
				g.flags.doing_system_file = 0;
				SetCurrentPath( GetText( pPath ) );
				return TRUE;
			}
			g.flags.doing_system_file = 0;
		}
	}
	// at this point - offer to add another path...
	return FALSE;

}
//----------------------------------------------------------------------

int ProcessInclude( int bNext )
{

	char Workname[__MAX_PATH__];
	char basename[__MAX_PATH__];
	int i = 0, did_subst = 0;
	PTEXT pEnd, pWord;

	if( !( pEnd = pWord = GetCurrentWord() ) )
	{
		fprintf( stderr, WIDE("%s(%d) Error: #include without name.\n"), GetCurrentFileName(), GetCurrentLine() );
		g.ErrorCount++;
		return TRUE;
	}
	g.flags.bIncludedLastFile = 0;
	do
	{
		if( GetText( pWord )[0] == '\"' )
		{
			if( bNext )
			{
				fprintf( stderr, WIDE("Hmm warning : did not implement include next for 'user' headers.\n") );
				return FALSE;
			}
			pEnd = NEXTLINE( GetCurrentWord() );
			while( pEnd && GetText( pEnd )[0] != '\"' )
			{
				i += sprintf( basename + i, WIDE("%s"), GetText( pEnd ) );
				pEnd = NEXTLINE( pEnd );
			}
			basename[i] = 0;
			if( !pEnd )
			{
				fprintf( stderr, WIDE("%s(%d) Error: Invalid name end bounding for #include \"...\n")
						, GetCurrentFileName(), GetCurrentLine() );
				g.ErrorCount++;
				return TRUE;
			}
			strcpy( Workname, basename );
			if( g.bDebugLog )
			{
				fprintf( stddbg, WIDE("attempting: \"%s\"\n"), basename );
			}
			if( g.flags.load_once )
			{
				if( AlreadyLoaded( Workname ) ) {
					SetCurrentWord( NEXTLINE( pEnd ) );
					g.flags.bIncludedLastFile = 1;
					return TRUE;
				}
			}
			if( !OpenNewInputFile( basename, Workname, GetCurrentFileName(), GetCurrentLine(), g.bAutoDepend, bNext ) )
			{
				PTEXT pPath;
				INDEX idx;
				int count;
				char *dir = pathrchr( g.pFileStack->longname );
				if( dir )
					count = (dir - g.pFileStack->longname);
				else
					count = 0;
#if( _MSC_VER && ( _MSC_VER < 1800 ) )
				_snprintf( Workname, __MAX_PATH__, WIDE( "%*.*s/%s" ), count, count, g.pFileStack->longname, basename );
#else
#if( _MSC_VER && ( _MSC_VER < 1800 ) )
				_snprintf( Workname, __MAX_PATH__, WIDE( "%*.*s/%s" ), count, count, g.pFileStack->longname, basename );
#else
				snprintf( Workname, __MAX_PATH__, WIDE( "%*.*s/%s" ), count, count, g.pFileStack->longname, basename );
#endif
#endif
				if( OpenNewInputFile( basename, Workname, GetCurrentFileName(), GetCurrentLine(), TRUE, bNext ) )
				{
					SetCurrentWord( NEXTLINE( pEnd ) );
					g.flags.bIncludedLastFile = 1;
					return TRUE;
				}
				FORALL( g.pUserIncludePath, idx, PTEXT, pPath )
				{
					sprintf( Workname, WIDE("%s/%s"), GetText( pPath ), basename );
					if( g.bDebugLog )
					{
						fprintf( stddbg, WIDE("attempting \"%s\"\n") , Workname );
					}
					/*1234*/
					if( g.flags.load_once )
					{
						if( AlreadyLoaded( Workname ) ) {
							SetCurrentWord( NEXTLINE( pEnd ) );
							g.flags.bIncludedLastFile = 1;
							return TRUE;
						}
					}
					if( OpenNewInputFile( basename, Workname, GetCurrentFileName(), GetCurrentLine(), TRUE, bNext ) )
					{
						if( idx )
							SetCurrentPath( GetText( pPath ) );
						g.flags.bIncludedLastFile = 1;
						return TRUE;
					}
				}
				fprintf( stderr, WIDE("%s(%d): Warning could not find include file \"%s\". try <%s>? I won't - but maybe...\n")
						, GetCurrentFileName()
						, GetCurrentLine()
						, basename, basename );
				return FALSE;
			}
			else
			{
				g.flags.bIncludedLastFile = 1;
				return TRUE;
			}
		}
		else if( GetText( pWord )[0] == '<' )
		{
			//PTEXT pPath;
			//INDEX idx;
			/*
			 if( GetText( GetCurrentWord() )[0] != '<' )
			 {
			 fprintf( stderr, WIDE("%s(%d) Error: Invalid name bounding for #INCLUDE %c\n")
			 , GetCurrentFileName(), GetCurrentLine()
			 , GetText( GetCurrentWord() )[0] );
			 g.ErrorCount++;
			 return TRUE;
			 }
			 */
			pEnd = NEXTLINE( pWord );
			while( pEnd && GetText( pEnd )[0] != '>' )
			{
				i += sprintf( basename + i, WIDE("%s"), GetText( pEnd ) );
				pEnd = NEXTLINE( pEnd );
			}
			basename[i] = 0;
			if( !pEnd )
			{
				fprintf( stderr, WIDE("%s(%d) Error: Invalid name end bounding for #INCLUDE <...\n")
						, GetCurrentFileName(), GetCurrentLine() );
				g.ErrorCount++;
				return TRUE;
			}
			if( g.flags.load_once )
			{
				if( AlreadyLoaded( basename ) ) {
					SetCurrentWord( NEXTLINE( pEnd ) );
					g.flags.bIncludedLastFile = 1;
					return TRUE;
				}
			}
			if( ProcessSystemIncludeFile( basename, FALSE, bNext ) ) {
				g.flags.bIncludedLastFile = 1;
				return TRUE;
			}
			break;
		}
		else
		{
			PTEXT pStart = pWord;
			EvalSubstitutions( &pWord, FALSE );
			if( pWord != pStart ) {
				SetCurrentWord( pWord );
			}
			else
				sprintf( basename, WIDE("%s"), GetText( pEnd ) );
			did_subst++;

		}
	} while( did_subst < 2 );
	if( g.flags.load_once )
	{
		AddFileDepend( g.pFileStack, basename, basename );
	}
	fprintf( stderr, WIDE("%s(%d) Warning could not process include file %c%s%c\n")
			  , GetCurrentFileName(), GetCurrentLine()
					, (pEnd!= GetCurrentWord())?GetCurrentWord()->data.data[0]:' '
					, basename
					, (pEnd != GetCurrentWord()) ? pEnd->data.data[0] : ' '
			);
	//g.ErrorCount++;

	// at this point - offer to add another path...
	return FALSE;

}

//----------------------------------------------------------------------
// values for nState in FILETRACK
//#define CONTINUE_DEFINE	0x000002
// if a ELSE or ENDIF at level 0 is found...
// then this is satisfied... #if type statement increment if levels
// and endif decrements the levels....
#define FIND_ELSE			0x000008
// FIND_ENDIF ... statements between are ignored until ENDIF
// even #else and #elseif type
// although #if within this block may also have a paired ENDIF
// #if type statements will increment the if levels and
// #endif will decrement until level is 0 and is an end...
#define FIND_ENDIF		  0x000010
//----------------------------------------------------------------------

//----------------------------------------------------------------------
static char *pFileIfStart; // mark this for on close message....
static int nLineIfStart;
static int nState;
static int nIfLevelElse;
//----------------------------------------------------------------------

void SetIfBegin( void )
{
	g.nIfLevels++;
	if( g.bDebugLog )
		fprintf( stddbg, WIDE("%s(%d): Setting IF level %d finding %s%s\n")
				, GetCurrentFileName()
				, GetCurrentLine()
				, g.nIfLevels
				, nState & FIND_ELSE?"ELSE":""
				, nState & FIND_ENDIF?"ENDING":""
				 );
	if( !pFileIfStart )
	{
		if( g.bDebugLog )
			fprintf( stddbg, WIDE("Set if starting level: %d\n"), g.nIfLevels );
		pFileIfStart = StrDup( GetCurrentFileName() );
		nLineIfStart = GetCurrentLine();
	}
}

//----------------------------------------------------------------------

void ClearIfBegin( void )
{
	if( !g.nIfLevels )
	{
		fprintf( stddbg, WIDE("%s(%d): Extra #endif without an #if\n")
				 , GetCurrentFileName()
				 , GetCurrentLine() );
		return;
	}
	if( g.bDebugLog )
		fprintf( stddbg, WIDE("%s(%d): Clearing IF %d finding %s%s\n")
					, GetCurrentFileName()
					, GetCurrentLine() 
					, g.nIfLevels
					, nState & FIND_ELSE?"ELSE":""
					, nState & FIND_ENDIF?"ENDING":""
					);
	g.nIfLevels--;
	if( !g.nIfLevels )
	{
		//fprintf( stderr, WIDE("--------------------------------------------\n") );
		if( pFileIfStart )
			Release( pFileIfStart );
		pFileIfStart = NULL;
		nLineIfStart = 0;
	}
}

//----------------------------------------------------------------------

void GetIfBegin( char**file, int*line)
{
	if( file )
		*file = pFileIfStart;
	if( line )
		*line = nLineIfStart;
}

//----------------------------------------------------------------------

int PreProcessLine( void )
{
	PTEXT pDirective = NULL;
	PTEXT pFirstWord;
	if( !( pFirstWord = GetCurrentWord() ) )
		return FALSE;
	if( g.bDebugLog )
	{
		fprintf( stddbg, WIDE("%s(%d): "), GetCurrentFileName(), GetCurrentLine() );
		DumpSegs( GetCurrentWord() );
		fprintf( stddbg, WIDE("\n") );
	}
	// pre-processor command processing....
	if( GetText( GetCurrentWord() )[0] == '#' )
	{
		// pre processor directive at start of line...
		pDirective = StepCurrentWord();
		if( !pDirective )
			return FALSE;
		StepCurrentWord();
//==== ENDIF =====================================================
		if( TextLike( pDirective, WIDE("endif") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}

			if( nState & FIND_ELSE )
			{
				if( g.nIfLevels == nIfLevelElse )
				{
					if( g.bDebugLog )
						fprintf( stddbg, WIDE("%s(%d): Looking for an else - found endif - correct level\n")
								 , GetCurrentFileName(), GetCurrentLine() );
					nState &= ~FIND_ELSE;
				}
				else if( g.bDebugLog )
					fprintf( stddbg, WIDE("%s(%d): Looking for an else - found endif - wrong level\n")
								 , GetCurrentFileName(), GetCurrentLine() );

			}
			if( nState & FIND_ENDIF )
			{
				if( g.nIfLevels == nIfLevelElse )
				{
					if( g.bDebugLog )
						fprintf( stddbg, WIDE("%s(%d): Looking for an endif - found endif - correct level\n")
								 , GetCurrentFileName(), GetCurrentLine() );
					nState &= ~FIND_ENDIF;
				}
				else if( g.bDebugLog )
					fprintf( stddbg, WIDE("%s(%d): Looking for an endif - found endif - wrong level\n")
								 , GetCurrentFileName(), GetCurrentLine() );
			}
			ClearIfBegin();

  			if( !g.nIfLevels )
  			{
				if( g.bDebugLog )
					fprintf( stddbg, WIDE("-------------------------------------\n") );
	  			nState &= ~(FIND_ELSE | FIND_ENDIF);
	  		}
			return FALSE;
		}
		if( nState & FIND_ENDIF )
		{
			if( TextLike( pDirective, WIDE("ifdef") ) ||
					  TextLike( pDirective, WIDE("ifndef") ) ||
					  TextLike( pDirective, WIDE("if") ) )
			{
				if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
					SetCurrentWord( pFirstWord );
					return TRUE;
				}
				SetIfBegin();
				if( g.bDebugLog )
				{
					fprintf( stddbg, WIDE("%s(%d): Another level of ifs... coming up! (%d)\n")
							 , GetCurrentFileName(), GetCurrentLine()
							 , g.nIfLevels  );
				}
			}
			return FALSE;
		}
//==== ELSE =====================================================
		else if( TextLike( pDirective, WIDE("else") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			if( GetCurrentWord() )
			{
				fprintf( stderr, WIDE("%s(%d) Warning: harmless extra tokens after #ELSE\n")
						 , GetCurrentFileName(), GetCurrentLine() );
			}
			if( nState & FIND_ENDIF )
			{
				// if only looking for endif - continue....
				if( g.bDebugLog )
					fprintf( stddbg, WIDE("Still looking for endif... skipping else\n") );
				return FALSE;
			}

			if( nState & FIND_ELSE )
			{
				if( g.nIfLevels == nIfLevelElse )
				{
					if( g.bDebugLog )
						fprintf( stddbg, WIDE("%s(%d): Found an else on the correct level - let's process...\n")
								 , GetCurrentFileName(), GetCurrentLine() );
					// is the else that we seek....
					// otherwise probalby in a sub-if and need
					// to wait for that #endif to complete and
					// get us back to the  right level to find
					// and else....
					nState &= ~FIND_ELSE;
				}
				return FALSE;
			}
			else // was in an if - and now need to find endif...
			{
					if( g.bDebugLog )
						fprintf( stddbg, WIDE("%s(%d): else termination - next to find endif this level(%d)\n")
								 , GetCurrentFileName(), GetCurrentLine()
								 , g.nIfLevels );
				// if we ever hit an else that is actively processing
				// then find the else... otherwise 
				// it would be skipped 
				//fprintf( stderr, WIDE("Finding the endif of the current if...\n") );
				nIfLevelElse = g.nIfLevels;
				nState |= FIND_ENDIF;
			}
			return FALSE;
		}
//==== ELSEIFDEF ELIFDEF =====================================================
		else if( TextLike( pDirective, WIDE("elseifdef") ) ||
					TextLike( pDirective, WIDE("elifdef") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			if( !( nState & FIND_ELSE ) )
			{
				// if was processing, an else causes find endif...
				nState = FIND_ENDIF;
				nIfLevelElse = g.nIfLevels;
				return TRUE;
			}
			return FALSE;
		}
//==== ELSEIFNDEF ELIFNDEF =====================================================
		else if( TextLike( pDirective, WIDE("elseifndef") ) ||
					TextLike( pDirective, WIDE("elifndef") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			if( !( nState & FIND_ELSE ) )
			{
				// if was processing, an else causes find endif...
				nState = FIND_ENDIF;
				nIfLevelElse = g.nIfLevels;
				return TRUE;
			}
			// results in find_else... else continues...
			// unless
			return FALSE;
		}

		// nothing else is valid if I'm still looking for an else or endif and it was not handled
		// by the prior two conditions....
		if( nState & (FIND_ELSE | FIND_ENDIF) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			if( TextLike( pDirective, WIDE("elseif") ) ||
				TextLike( pDirective, WIDE("elif") ) )
			{
				goto SubstituteAndProcess;
			}
			else if( TextLike( pDirective, WIDE("ifdef") ) ||
					  TextLike( pDirective, WIDE("ifndef") ) ||
					  TextLike( pDirective, WIDE("if") ) )
			{
				SetIfBegin();
				if( g.bDebugLog )
				{
					fprintf( stddbg, WIDE("%s(%d): Another level of ifs... coming up! (%d)\n")
							 , GetCurrentFileName(), GetCurrentLine()
							 , g.nIfLevels  );
				}
			}
			//fprintf( stderr, WIDE("Failing line...\n") );
			return FALSE;
		}

//== INCLUDE =======================================================
		if( TextLike( pDirective, WIDE("include") ) )
		{
			//fprintf( stderr, WIDE("Include segments...") );
			//DumpSegs( pDirective );
			ProcessInclude( FALSE );
			if( g.flags.keep_includes && !g.flags.bIncludedLastFile )
				SetCurrentWord( pFirstWord );

			return g.flags.keep_includes;
		}
//== INCLUDE NEXT ==================================================
		else if( TextLike( pDirective, WIDE("include_next") ) )
		{
			//fprintf( stderr, WIDE("Include segments...") );
			//DumpSegs( pDirective );
			if( g.flags.keep_includes )
				SetCurrentWord( pFirstWord );
			else {
				ProcessInclude( TRUE );
				if( g.flags.keep_includes && !g.flags.bIncludedLastFile )
					SetCurrentWord( pFirstWord );
			}
			return g.flags.keep_includes;
		}
//== DEFINE =======================================================
		else if( TextLike( pDirective, WIDE("define") ) )
		{
			if( g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			if( !NEXTLINE( pDirective ) )
			{
				fprintf( stderr, WIDE("\"#define\" keyword alone is NOT allowed...") );
				return FALSE; // can still continue....
			}
			ProcessDefine( DEFINE_FILE );
			if( g.flags.skip_logic_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			return FALSE;
		}
//== UNDEF  =======================================================
		else if( TextLike( pDirective, WIDE("undef") ) )
		{
			PDEF pDef;
			if( g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			pDef = FindDefineName( GetCurrentWord(), IGNORE_PARAMS );
			if( pDef )
			{
				DeleteDefine( &pDef );
			}
			if( g.flags.skip_logic_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			return FALSE;
		}
//== IFDEF =======================================================
		else if( TextLike( pDirective, WIDE("ifdef") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			SetIfBegin();
			if( !FindDefineName( GetCurrentWord(), IGNORE_PARAMS ) )
			{
				if( g.bDebugLog )
				{
					fprintf( stddbg, WIDE("%s(%d): ifdef %s FAILED\n")
							, GetCurrentFileName()
							  , GetCurrentLine()
							, GetText( GetCurrentWord() ));
				}
				nState |= FIND_ELSE;
			}
			else
				if( g.bDebugLog )
				{
					fprintf( stddbg, WIDE("%s(%d): ifdef %s SUCCESS\n")
							, GetCurrentFileName()
							  , GetCurrentLine()
							, GetText( GetCurrentWord() ) );
				}
			nIfLevelElse = g.nIfLevels;
			return FALSE;
		}
//== IFNDEF =======================================================
		else if( TextLike( pDirective, WIDE("ifndef") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			SetIfBegin();
			if( FindDefineName( GetCurrentWord(), IGNORE_PARAMS ) )
			{
				if( g.bDebugLog )
				{
					fprintf( stddbg, WIDE("%s(%d): ifndef %s FAILED\n")
							, GetCurrentFileName()
							  , GetCurrentLine()
							, GetText( GetCurrentWord() ));
				}
				nState |= FIND_ELSE;
			}
			else
				if( g.bDebugLog )
					fprintf( stddbg, WIDE("%s(%d): ifndef %s SUCCESS\n")
							, GetCurrentFileName()
							  , GetCurrentLine()
							, GetText( GetCurrentWord() ) );
			nIfLevelElse = g.nIfLevels;
			// otherwise we can store these statements...
			return FALSE;
		}

		// have to go through all words and check vs current
		// defines to see if we need to substitute the data or not...
	SubstituteAndProcess:
		if( !g.flags.skip_define_processing && !g.flags.skip_logic_processing )
		{
			PTEXT line = GetCurrentWord();
			EvalSubstitutions( &line, FALSE );
			if( line != GetCurrentWord() )
				SetCurrentWord( line );
		}
//=== IF ======================================================
		if( TextLike( pDirective, WIDE("if") ) )
		{
			PTEXT dbg;
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			SetIfBegin();
			if( !ProcessExpression() )
			{
				nState |= FIND_ELSE;
				nIfLevelElse = g.nIfLevels;
				if( g.bDebugLog )
				{
					dbg = BuildLine( pDirective );
					fprintf( stddbg, WIDE("%s(%d): %s FAILED\n")
									, GetCurrentFileName()
									, GetCurrentLine()
									, GetText(dbg) );
					LineRelease( dbg );
				}
			}
			else
			{
				if( g.bDebugLog )
				{
					dbg = BuildLine( pDirective );
					fprintf( stddbg, WIDE("%s(%d): %s SUCCESS\n")
									, GetCurrentFileName()
									, GetCurrentLine()
									, GetText(dbg) );
					LineRelease( dbg );
				}
			}
		}
//=== ELSEIF ELIF ======================================================
		else if( TextLike( pDirective, WIDE("elseif") ) ||
				  TextLike( pDirective, WIDE("elif") ) )
		{
			if( g.flags.skip_logic_processing || g.flags.skip_define_processing ) {
				SetCurrentWord( pFirstWord );
				return TRUE;
			}
			if( nState & FIND_ELSE )
			{
				PTEXT dbg;
				if( g.nIfLevels == nIfLevelElse &&
					 ProcessExpression() )
				{
					if( g.bDebugLog )
					{
						dbg = BuildLine( pDirective );
						fprintf( stddbg, WIDE("%s(%d): %s Success\n")
									, GetCurrentFileName()
									, GetCurrentLine()
								, GetText( dbg ) );
						LineRelease( dbg );
					}
					nState &= ~FIND_ELSE;
				}
				else if( g.bDebugLog )
				{
					dbg = BuildLine( pDirective );
					fprintf( stddbg, WIDE("%s(%d): %s Failure\n")
									, GetCurrentFileName()
									, GetCurrentLine()
							, GetText( dbg ) );
					LineRelease( dbg );
				}
			}
			else // wasn't looking for else - else found - go to endif now.
			{
				nState = FIND_ENDIF;
				nIfLevelElse = g.nIfLevels;
			}
		}
//==== PRAGMA =====================================================
		else if( TextLike( pDirective, WIDE("pragma") ) )
		{
			// pramga message seems to be a useful thing to have...
			// other pragmas need to be ignored
			// pragmas occur with all data on a single line.
			// evaluate substitutions... (already done)
			PTEXT pOp = GetCurrentWord();
			if( TextLike( pOp, WIDE("message") ) )
			{
				PTEXT pOut;
				pOut = BuildLineEx( NEXTLINE( pOp ), FALSE DBG_SRC );
				pOut->data.size = CollapseQuotes( pOut->data.data );
				pOut->data.size = KillQuotes( pOut->data.data );
				fprintf( stderr, WIDE("%s\n"), GetText( pOut ) );
				//fprintf( stdout, WIDE("%s\n"), GetText( pOut ) );
				LineRelease( pOut );
				// dump the remaining segments...
			}
			else if( TextLike( pOp, WIDE("systemincludepath") ) )
			{
				PTEXT pOut;
				pOut = BuildLineEx( NEXTLINE( pOp ), FALSE DBG_SRC );
				AddLink( g.pSysIncludePath, pOut );
			}
			else if( TextLike( pOp, WIDE("includepath") ) )
			{
				PTEXT pOut;
				pOut = BuildLineEx( NEXTLINE( pOp ), FALSE DBG_SRC );
				AddLink( g.pUserIncludePath, pOut );
			}
			else if( TextLike( pOp, WIDE("pack") ) )
			{
				if( g.bDebugLog )
				{
					PTEXT pOut;
					pOut = BuildLineEx( pOp, FALSE DBG_SRC );
					fprintf( stderr, WIDE("%s(%d): %s Unknown pragma: %s\n")
							 , GetCurrentFileName()
							 , GetCurrentLine()
							 , g.flags.bEmitUnknownPragma?"emitting":"dropping"
							 , GetText( pOut ) );
					LineRelease( pOut );
				}
				SetCurrentWord( pFirstWord );
				return g.flags.bEmitUnknownPragma;
			}
			// watcom - inline assembly junk...
			else if( TextLike( pOp, WIDE("warning") )
					 ||  TextLike( pOp, WIDE("intrinsic") )
					  || TextLike( pOp, WIDE("aux") )
					  || TextLike( pOp, WIDE("function") )
					 || TextLike( pOp, WIDE("comment") ) )
			{
				if( g.bDebugLog )
				{
					PTEXT pOut;
					pOut = BuildLineEx( pOp, FALSE DBG_SRC );
					fprintf( stderr, WIDE("%s(%d): %s Unknown pragma: %s\n")
							 , GetCurrentFileName()
							 , GetCurrentLine()
							 , g.flags.bEmitUnknownPragma?"emitting":"dropping"
							 , GetText( pOut ) );
					LineRelease( pOut );
				}
				SetCurrentWord( pFirstWord );
				return g.flags.bEmitUnknownPragma;
			}
			// watcom - dependancy generation...
			else if( TextLike( pOp, WIDE("read_only_file") ) )
			{
				// can't see any usefulness when using ppc to preprocess...
				return FALSE;
			}
			else
			{
				PTEXT pOut;
				pOut = BuildLineEx( pOp, FALSE DBG_SRC );
				fprintf( stderr, WIDE("%s(%d): Unknown pragma: %s\n")
						 , GetCurrentFileName()
						 , GetCurrentLine()
						 , GetText( pOut ) );
				LineRelease( pOut );
				// hmm - gcc processing .i files fails this.
				SetCurrentWord( pFirstWord );
				return g.flags.bEmitUnknownPragma;
				//return TRUE; // emit this line - maybe the compiler knows...
				//return FALSE;
			}
		}
		else if( TextLike( pDirective, WIDE("warning") ) )
		{
			PTEXT pOut;
			pOut = BuildLineEx( GetCurrentWord(), FALSE DBG_SRC );
			fprintf( stderr, WIDE("%s(%d): Warning %s\n")
					 , GetCurrentFileName()
					 , GetCurrentLine()
					 , GetText( pOut ) );
			LineRelease( pOut );
		}
		else if( TextLike( pDirective, WIDE("error") ) )
		{
			PTEXT pOut;
			pOut = BuildLineEx( GetCurrentWord(), FALSE DBG_SRC );
			fprintf( stderr, WIDE("%s(%d): Error %s\n")
					 , GetCurrentFileName()
					 , GetCurrentLine()
					 , GetText( pOut ) );
			LineRelease( pOut );
			g.ErrorCount++;
		}
		else
		{
			PTEXT pOut;
			pOut = BuildLineEx( pDirective, FALSE DBG_SRC );
			fprintf( stderr, WIDE("%s(%d): Unknown prepcessing directive: %s\n")
					 , GetCurrentFileName()
					 , GetCurrentLine()
					 , GetText( pOut ) );
			LineRelease( pOut );
		}
		return FALSE;
	}

	if( nState & ( FIND_ELSE|FIND_ENDIF ) )
	{
		// ignore anything on this line for output
		return FALSE;
	}
	if( pDirective )
	{
		fprintf( stderr, WIDE("ERROR: Responding true to invoking a preprocessor command to output\n") );
	}
	if( !g.flags.skip_define_processing && !g.flags.skip_logic_processing )
	{
		PTEXT *line = GetCurrentTextLine();
		//PTEXT reset = GetCurrentWord();
		if( nState )
			fprintf( stderr, WIDE("ERROR: Substituting the line...bad state %d\n"), nState );
		EvalSubstitutions( line, TRUE );
		SetCurrentWord( pFirstWord );
	}
	return TRUE;
}

//----------------------------------------------------------------------------

int ProcessStatement( void )
{

	return TRUE;


	//ProcessEnum();  // snags enumeration symbols...
	//ProcessStructUnion(); // builds union/structure space...
	//ProcessArray(); // handles gathering and possible re-emmission of arrays...
}

//----------------------------------------------------------------------------

void RunProcessFile( void )
{
	while( ReadLine( FALSE ) )
	{
		int depth = CurrentFileDepth();
		if( PreProcessLine() )
		{
			//if( g.flags.keep_includes )
			{
			//	if( depth > 1 )
			//		continue;
			}
			if( ProcessStatement() )
			{
				PTEXT pOut;
				pOut = BuildLineEx( GetCurrentWord(), FALSE DBG_SRC );
				if( pOut )
				{
					if( g.flags.bWriteLineInfo )
					{
						WriteCurrentLineInfo();
					}
					WriteLine( GetTextSize( pOut ), GetText( pOut ) );
					LineRelease( pOut );
				}
			}
		}
	}
	if( nState || g.nIfLevels )
	{
		char *file;
		int line;
		GetIfBegin( &file, &line );
		fprintf( stderr, WIDE("Missing #endif starting at %s(%d)")
					, file, line );
	}
	// at this point we have dumped an output file....
	// the standard states
//	5. Each source character set member and escape sequence in character constants and
//string literals is converted to the corresponding member of the execution character
//set; if there is no corresponding member, it is converted to an implementationdefined
//member other than the null (wide) character.7)
	// this is done above... as we are handling substitutions...
	// though this will result in an inability to handle some things like...
	// "\33" "3"
// 6. Adjacent string literal tokens are concatenated.
// 7. 8. ... and done :)
}

void ProcessFile( char *file );
void loadConfig( void ) {
	{
		char file[256];
		sprintf( file, WIDE( "%s/config.ppc" ), g.pWorkPath/*g.pExecPath*/ );
		//printf( WIDE("loading defines from %s"), file );
#if defined( __WATCOMC__ ) || defined (__LCC__)
		{
			char *includepath = getenv( WIDE( "INCLUDE" ) );
			// %WATCOM%\H\NT;%WATCOM%\H;%INCLUDE%
			char *start, *end;
			start = includepath;
			while( start[0] && (end = strchr( start, ';' )) )
			{
				PTEXT pOut;
				end[0] = 0;
				//printf( WIDE("Adding include path: %s"), start );
				pOut = SegCreateFromText( start );
				AddLink( g.pSysIncludePath, pOut );
				start = end + 1;
			}
			if( start[0] )
			{
				PTEXT pOut;
				//printf( WIDE("Adding include path: %s"), start );
				pOut = SegCreateFromText( start );
				AddLink( g.pSysIncludePath, pOut );
			}
		}
#endif
		ProcessFile( file );
		CommitDefinesToCommandLine();
		DestoyDepends();
	}

}


void ProcessFile( char *file )
{
	char newname[__MAX_PATH__];
	char *filestart = pathrchr( file );
	if( !g.flags.config_loaded ) {
		g.flags.config_loaded = 1;;
		loadConfig();
	}
	if( !filestart )
		filestart = file;
	else
		filestart++;
	if( !OpenInputFile( filestart, file ) )
	{
		return;
	}
	if( g.CurrentOutName[0] != 0xFF )
	{
		if( !g.flags.bStdout )
		{
			if( g.CurrentOutName[0] )
				strcpy( newname, g.CurrentOutName );
			else
			{
				strcpy( newname, file );
				newname[strlen(newname)-1] = 'i'; // replace last char with i...
			}
			if( !OpenOutputFile( newname ) )
			{
				CloseInputFile();
				return;
			}
		}
		else
		{
			if( !OpenStdOutputFile() )
			{
				CloseInputFile();
				return;
			}
		}
		g.CurrentOutName[0] = 0xFF;
		{
			PINCLUDE_REF pRef;
			while( pRef = PopLink( g.pIncludeList ) )
			{
				g.flags.bNoOutput = pRef->flags.bMacros;
				if( !ProcessSystemIncludeFile( pRef->name, TRUE, FALSE ) )
				{
					fprintf( stderr, WIDE("%s(%d): Warning could not process include file \'%s\'\n")
							, GetCurrentFileName(), GetCurrentLine()
							, pRef->name );
					//g.ErrorCount++;
					return;
				}
				Release( pRef );
			}
			g.flags.bNoOutput = 0;
		}
	}
	RunProcessFile(); // all files should be closed once this returns.
}


void ReleaseIncludePaths( void )
{
	INDEX idx;
	PTEXT path;
	FORALL( g.pSysIncludePath, idx, PTEXT, path )
		Release( path );
	DeleteList( g.pSysIncludePath );

	FORALL( g.pUserIncludePath, idx, PTEXT, path )
	{
		if( idx ) // don't release user include directory 0 - internal use.
			Release( path );
	}
	DeleteList( g.pUserIncludePath );
}



void usage( void )
{
	printf( WIDE("usage: %s (options) <files...>\n"), g.pExecName );
	printf( WIDE("\toptions to include\n")
			  "	------------------------------------------\n" );
	printf( WIDE("\t -[Ii]<path(s)>      add include path to default\n") );
	printf( WIDE("\t -[Ss][Ii]<path(s)>  add include path to system default\n") );
	printf( WIDE("\t -[Dd]<symbol>       define additional symbols\n") );
	printf( WIDE("\t -MF<file>           dump out auto-depend info\n") );
	printf( WIDE("\t -MT<file>           use (file) as name of target in depend file\n") );
	printf( WIDE("\t -L                  write file/line info prefixing output lines\n") );
	printf( WIDE("\t -l                  write file/line info prefixing output lines\n") );
	printf( WIDE( "	                           (without line directive)\n" ) );
	printf( WIDE("\t -K                  emit unknown pragmas into output\n") );
	printf( WIDE("\t -k                  do not emit unknown pragma (default)\n") );
	printf( WIDE("\t -c                  keep comments in output\n") );
	printf( WIDE("\t -p                  keep includes in output (don't output content of include)\n") );
	printf( WIDE("\t -f                  force / into \\ in include statements with paths\n" ) );
	printf( WIDE("\t -sd                 skip define processing (if,else,etc also skippped)\n" ) );
	printf( WIDE("\t -sl                 skip logic processing (if,else,endif skippped; process defines for include subst)\n" ) );
	printf( WIDE("\t -ssio               Skip System Include Out;try to keep #includes that are missing as #include\n" ) );
	printf( WIDE("\t -F                  force \\ into /\n") );
	printf( WIDE("\t -[Oo]<file>         specify the output filename; output filename must be set before input(s) are read\n") );
	printf( WIDE("\t                         -o output \"process this file into output.c\"\n" ) );
	printf( WIDE("\t -once               only load any include once\n") );
	printf( WIDE("\t -[Zz]#              debug info mode. where number is in ( 1, 2, 4 )\n") );
	printf( WIDE("\t @<file>              any option read from a file...\n") );
	printf( WIDE("  Any option prefixed with a - will force the option off...  --c to force not keeping comments\n") );
	printf( WIDE("  Option L is by default on. (line info with #line keyword)\n") );
	printf( WIDE("  output default is input name substituing the last character for an i...\n") );
	printf( WIDE("\t\t  test.cpp -> test.cpi  test.c -> test.i\n") );
	printf( WIDE("\t examples : \n" ) );
	printf( WIDE("\t\tppc source_file.c\n" ) );
	printf( WIDE("\t\t# the following is how to do an amalgamtion\n" ) );
	printf( WIDE("\t\tppc -sd -ssio -K -c -once -Iinclude -o file_all.c file1.c file2.c file3.c\n" ) );
	printf( WIDE("\t\tppc -sdssioKconce -Iinclude -o file_all.c file1.c file2.c file3.c\n" ) );
	printf( WIDE("\t -? for more help\n") );
	printf( WIDE(" (startup directory)/config.ppc is read first.  It is only read once.   Usage is intended to generate\n" ) );
	printf( WIDE(" one file at a time.   It IS possible to do -o file1.i file1.c -o file2.i file2.c but the config.ppc won't\n" ) );
	printf( WIDE(" be read the second time.  However, symbols meant to be kept in an amalgamtion output can be define here, and\n" ) );
   printf( WIDE(" those defines will be kept.\n" ) );
}

void longusage( void )
{
	printf( WIDE("Unimplemented yet... showing usage()\n") );
	usage();
}

int ispathchr( char c )
{
	if( c == '\\' || c == '/' )
		return TRUE;
	return FALSE;
}


char *nextchr( char *string, char *chars )
{
	char *test;
	if( !string || !chars )
		return NULL;
	while( string[0] )
	{
		test = chars;
		while( test[0] )
		{
			if( string[0] == test[0] )
				return string;
			test++;
		}
		string++;
	}
	return NULL;
}				  

int processArguments( int argc, char **argv ) {
	{
		int i=1;
		int nArgState = ARG_UNKNOWN;
		for( i = 1; i < argc; i++  )
		{
			if( argv[i][0] == '@' ) {
				FILE *file = fopen( argv[i]+1, "rt" );
				if( file ) {
					char buf[4096];
					while( fgets( buf, 4096, file ) )
					{
						int tmpargc;
						char **tmpargv;
						int len = strlen(buf);
						if( buf[len-1] == '\n' )
							buf[len-1] = 0;
						ParseIntoArgs( buf, &tmpargc, &tmpargv );
						processArguments( tmpargc+1, tmpargv-1 );
						{
							int n;
							// be nice and cleanup memory.
							for( n = 0; n < tmpargc; n++ )
								Release( tmpargv[n] );
							Release( tmpargv );
						}
					}
				}
			}
			else if( argv[i][0] == '-' ||
				 //argv[i][0] == '/' ||
				 nArgState )
			{
				int n, done = 0, negarg = 0;
				for( n = 1;
					  !done && argv[i][n];
					  n++ )
				{
					switch( nArgState )
					{
					case ARG_MACRO_FILE:
						{
							PINCLUDE_REF pRef = Allocate( sizeof( INCLUDE_REF ) );
							pRef->flags.bMacros = 1;
							pRef->name = argv[i];
							PushLink( g.pIncludeList, pRef );
						}
						nArgState = ARG_UNKNOWN;
						done = 1;
						break;
					case ARG_INCLUDE_FILE:
						{
							PINCLUDE_REF pRef = Allocate( sizeof( INCLUDE_REF ) );
							pRef->flags.bMacros = 0;
							pRef->name = argv[i];
							PushLink( g.pIncludeList, pRef );
						}
						nArgState = ARG_UNKNOWN;
						done = 1;
						break;
					case ARG_AUTOTARGET_NAME:
						strcpy( g.AutoTargetName, argv[i] );
						nArgState = ARG_UNKNOWN;
						done = 1;
						break;
					case ARG_GEN_STDOUT:
						nArgState = ARG_UNKNOWN;
						done = 1;
						break;
					case ARG_OUT_NAME:
						strcpy( g.CurrentOutName, argv[i] );
						nArgState = ARG_UNKNOWN;
						done = 1;
						break;
					case ARG_AUTODEPEND_NAME:
						g.AutoDependFile = fopen( argv[i], WIDE("wt") );
						if( !g.AutoDependFile )
						{
							fprintf( stderr, WIDE("Failed to open %s for auto depend info."), argv[i] );
						}
						nArgState = ARG_UNKNOWN;
						done = 1;
						break;
					case ARG_INCLUDE_PATH:
						{
							char *arg, *next, *tmp;
							next = arg = argv[i];
							if( argv[i][0] != '-' )
							{
								while( ( next = nextchr( arg, WIDE(";,:") ) ) )
								{
									*next = 0; // terminate new string...
									next++;
									tmp = arg + ( strlen( arg ) - 1 );

									if( ispathchr( *tmp ) )
										*tmp = 0; // terminate prior;
									AddLink( g.pUserIncludePath, SegCreateFromText( arg ) );
									arg = next;
								}
								tmp = arg + ( strlen( arg ) - 1 );

								if( ispathchr( *tmp ) )
									*tmp = 0; // terminate prior;
								AddLink( g.pUserIncludePath, SegCreateFromText( arg ) );
							}
							else
								i--;
						}
						nArgState = 0;
						done = 1;
						break;
					case ARG_SYS_INCLUDE_PATH:
						{
							char *arg, *next, *tmp;
							next = arg = argv[i];
							while( ( next = nextchr( arg, WIDE(";,") ) ) )
							{
								*next = 0; // terminate new string...
								next++;
								tmp = arg + ( strlen( arg ) - 1 );

								if( ispathchr( *tmp ) )
									*tmp = 0; // terminate prior;
								AddLink( g.pSysIncludePath, SegCreateFromText( arg ) );
								arg = next;
							}
							tmp = arg + ( strlen( arg ) - 1 );

							if( ispathchr( *tmp ) )
								*tmp = 0; // terminate prior;
							AddLink( g.pSysIncludePath, SegCreateFromText( arg ) );
						}
						nArgState = 0;
						done = 1;
						break;
					case ARG_UNKNOWN:
					default:
						// some option to turn off/on write line info...
						if( argv[i][n] == '-' )
						{
							negarg = 1;
						}
						else if( argv[i][n] == 'c' )
						{
							g.flags.keep_comments = 1;
						}
						else if( argv[i][n] == 'p' )
						{
							g.flags.keep_includes = 1;
						}
						else if( argv[i][n] == 'I' )
						{
							char *arg, *next, *tmp;
							if( !argv[i][n+1] )
							{
								nArgState = ARG_INCLUDE_PATH;
								done = 1;
								break;
							}
							next = arg = argv[i] + n + 1;

							while( ( next = nextchr( arg, WIDE(";,") ) ) )
							{
								*next = 0; // terminate new string...
								next++;
								tmp = arg + ( strlen( arg ) - 1 );

								if( ispathchr( *tmp ) )
									*tmp = 0; // terminate prior;
								AddLink( g.pUserIncludePath, SegCreateFromText( arg ) );
								arg = next;
							}
							tmp = arg + ( strlen( arg ) - 1 );

							if( ispathchr( *tmp ) )
								*tmp = 0; // terminate prior;
							AddLink( g.pUserIncludePath, SegCreateFromText( arg ) );
							done = 1;
							break;
						}
						else if( argv[i][n] == 'i' )
						{
							if( strcmp( argv[i] + n, WIDE("include") ) == 0 )
							{
								//if( argv[i][n+7]
								nArgState = ARG_INCLUDE_FILE;
								done = 1;
								break;
							}
							else if( strcmp( argv[i] + n + 1, WIDE("macro") ) == 0 )
							{
								nArgState = ARG_MACRO_FILE;
								done = 1;
								break;
							}
							else
							{
								fprintf( stderr, WIDE("Argument error: %s\n"), argv[i] );
								usage();
								return 1;
							}
						}
						else if( (argv[i][n] == 's' &&
							argv[i][n + 1] == 'o') )
						{
							g.flags.bStdout = 1;
							n += 1;
							break;
						}
						else if( argv[i][n] == 's' && argv[i][n + 1] &&
							argv[i][n + 1] == 'd' ) {
							g.flags.skip_define_processing = 1;
							n += 1;
							break;
						}
						else if( argv[i][n] == 's' && argv[i][n + 1] &&
							argv[i][n + 1] == 'l' ) {
							g.flags.skip_logic_processing = 1;
							n += 1;
							break;
						}
						else if( argv[i][n] == 's' && argv[i][n + 1] &&
							argv[i][n + 1] == 's' && argv[i][n + 2] &&
							argv[i][n + 2] == 'i' && argv[i][n + 3] &&
							argv[i][n + 3] == 'o'
							)
						{
							g.flags.bSkipSystemIncludeOut = 1;
							n+=3;
							break;
						}
						else if( ( argv[i][n] == 's' ||
							  argv[i][n] == 'S' ) &&
							( argv[i][n+1] == 'I' ||
								argv[i][n+1] == 'i' ) )
						{
							char *arg, *next, *tmp;
							if( !argv[i][n+2] )
							{
								nArgState = ARG_SYS_INCLUDE_PATH;
								done = 1;
								break;
							}
							next = arg = argv[i] + n + 2;
							while( ( next = nextchr( arg, WIDE(";,") ) ) )
							{
								*next = 0; // terminate new string...
								next++;
								tmp = arg + ( strlen( arg ) - 1 );

								if( ispathchr( *tmp ) )
									*tmp = 0; // terminate prior;
								AddLink( g.pSysIncludePath, SegCreateFromText( arg ) );
								arg = next;
							}
							tmp = arg + ( strlen( arg ) - 1 );

							if( ispathchr( *tmp ) )
								*tmp = 0; // terminate prior;
							AddLink( g.pSysIncludePath, SegCreateFromText( arg ) );
						}
						else if( strcmp( argv[i]+n, "once" ) == 0 )
						{
							g.flags.load_once = 1;
						}
						else if( argv[i][n] == 'M' )
						{
							if( argv[i][n+1] == 'F' )
							{
								if( argv[i][n+2] )
								{
									if( g.AutoDependFile )
									{
										fprintf( stderr, WIDE("Reopening auto depend file?\n") );
										fclose( g.AutoDependFile );
									}
									g.AutoDependFile = fopen( argv[i]+n+2, WIDE("wt") );
									if( !g.AutoDependFile )
									{
										fprintf( stderr, WIDE("Failed to open %s for auto depend info.\n"), argv[i]+n+2 );
									}
								}
								else
								{
									nArgState = ARG_AUTODEPEND_NAME;
								}
								g.bAutoDepend = TRUE;
							}
							else if( argv[i][n+1] == 'T' )
							{
								if( argv[i][n+2] ) strcpy( g.AutoTargetName, argv[i]+n+2 );
								else nArgState = ARG_AUTOTARGET_NAME;
							}
							else
								goto unknown_option;
						}
						else if( argv[i][n] == 'o' ||
								  argv[i][n] == 'O' )
						{
							printf( "output %s", argv[i] + 2 );
							if( negarg )
							{
								g.CurrentOutName[0] = 0xFF;
							}
							else
							{
								if( argv[i][n+1] )
								{
									strcpy( g.CurrentOutName, argv[i] + n + 1 );
									//printf( WIDE("Set output: %s\n"), argv[i] + n + 1 );
									done = 1;
								}
								else
								{
									nArgState = ARG_OUT_NAME;
								}
							}
						}
						else if( argv[i][n] == 'D' ||
								  argv[i][n] == 'd' )
						{
							if( argv[i][n+1] )
							{
								char *eq = strchr( argv[i]+n+1, '=' );
								if( eq )
								{
									*eq = 0;
									eq++;
								}
								// additional command line defines....
								DefineDefine( argv[i] + n+1, eq );
							}
							else
							{
								nArgState = ARG_DEFINE_DEFINE;
							}
						}
						else if( argv[i][n] == 'z' ||
								  argv[i][n] == 'Z' )
						{
							if( isdigit( argv[i][n+1] ) )
							{
								g.bDebugLog = atoi( argv[i] + n+1 );
								printf( WIDE("Debug set to %d\n"), g.bDebugLog );
							}
							else
								g.bDebugLog = TRUE;
						}
						else if( argv[i][n] == 'L' )
						{
							g.flags.bLineUsesLineKeyword = TRUE;
 							if( negarg )
								g.flags.bWriteLine = FALSE;
							else
							{
								g.flags.bWriteLine = TRUE;
							}
						}
						else if( argv[i][n] == 'l' )
						{
							g.flags.bLineUsesLineKeyword = FALSE;
							if( negarg )
								g.flags.bWriteLine = FALSE;
							else
								g.flags.bWriteLine = TRUE;
						}
						else if( argv[i][n] == 'f' )
						{
							if( negarg )
								g.flags.bForceBackslash = FALSE;
							else
								g.flags.bForceBackslash = TRUE;
						}
						else if( argv[i][n] == 'F' )
						{
							if( negarg )
								g.flags.bForceForeslash = FALSE;
							else
								g.flags.bForceForeslash = TRUE;
						}
						else if( argv[i][n] == 'K' )
						{
							g.flags.bEmitUnknownPragma = TRUE;
						}
						else if( argv[i][n] == 'k' )
						{
							g.flags.bEmitUnknownPragma = FALSE;
						}
						else if( ( argv[i][n] == '?' ) || ( argv[i][n] == 'h' ) )
						{
							longusage();
							return 0;
						}
						else
						{
						unknown_option:
							fprintf( stderr, "unknown option: %s (+%d)\n", argv[i], n );
							usage();
							return 0;
						}
						done = 1;
						break;
					}
				}
			}
			else
			{
				static int process_count;
				process_count++;
				if( process_count > 1 )
				{
					//fprintf( stderr, WIDE("Probable error! -include, -imacro directives are lost.\nMultiple sources on command line.\n") );
				}
				ProcessFile( argv[i] );
				if( !g.flags.skip_logic_processing )
					DeleteAllDefines( DEFINE_FILE );
				//if(  )
				//g.CurrentOutName[0] = 0; // clear name.
			}
		}
	}
	return 2;
}

int main( char argc, char **argv, char **env )
{
#ifdef __LINUX__
	{
		/* #include unistd.h, stdio.h, string.h */
		{
			char buf[256], *pb;
			int n;
			n = readlink( WIDE("/proc/self/exe"), buf, 256 );
			if( n >= 0 )
			{
				buf[n] = 0; //linux
				if( !n )
				{
					strcpy( buf, WIDE(".") );
					buf[ n = readlink( WIDE("/proc/curproc/"), buf, 256 ) ] = 0; // fbsd
				}
			}
			else
				strcpy( buf, WIDE(".") );
			pb = strrchr( buf, '/' );
			if( pb )
			{
				pb[0]=0;
				pb++;
				strcpy( g.pExecName, pb );
			}
			strcpy( g.pExecPath, buf );
		}
	}
	getcwd( g.pWorkPath, sizeof( g.pWorkPath ) );
#elif defined( _WIN32 )
	 char *laststroke;
	 GetModuleFileName( NULL, g.pExecPath, sizeof(g.pExecPath) );
	 laststroke = pathrchr( g.pExecPath );
	 if( laststroke )
		 laststroke[0] = 0;
	//printf( WIDE("path: %s\n"), g.pExecPath );
	getcwd( g.pWorkPath, sizeof( g.pWorkPath ) );
#else
	printf( WIDE("Path is not defined - probably will not work.") );
#endif
#ifdef __WATCOMC__
	SetMinAllocate( sizeof( TEXT ) + 16 );
#endif
	DisableMemoryValidate(TRUE);
	// should build this from execution path of this module
	g.flags.do_trigraph = 1;
	g.flags.bWriteLine = TRUE;
	g.flags.bLineUsesLineKeyword = TRUE;
	g.bDebugLog = FALSE;
	//g.dbgout = fopen( "debug.log", "wt" );
	g.CurrentOutName[0] = 0xFF;
	//AddLink( g.pSysIncludePath, SegCreateFromText( WIDE("m:\\lcc\\include") ) );

	AddLink( g.pUserIncludePath, (PTEXT)&g.pCurrentPath );
	SetCurrentPath( WIDE(".") );

	InitDefines(); // set current date/time macros....
	if( 0 && !g.flags.skip_define_processing )
	{
		DefineDefine( WIDE( "TRUE" ), WIDE( "1" ) );
		DefineDefine( WIDE( "FALSE" ), WIDE( "0" ) );
		DefineDefine( WIDE( "true" ), WIDE( "1" ) );
		DefineDefine( WIDE( "false" ), WIDE( "0" ) );
		DefineDefine( WIDE( "__bool_true_false_are_defined" ), WIDE( "1" ) );
		DefineDefine( WIDE( "bool" ), WIDE( "unsigned char" ) );
		DefineDefine( WIDE( "__PPCCPP__" ), WIDE( "0x100" ) );
	}
	{
		int r = processArguments( argc, argv );
		if( r == 0 )
			return 0;
		if( r == 1 )
			 return 1;
	}
	VarTextEmpty( &g.vt );
	if( g.bAutoDepend )
	{
		DumpDepends();
	}
	{
		PINCLUDE_REF pRef;
		while( pRef = PopLink( g.pIncludeList ) )
			Release( pRef );
	}
	DestoyDepends();
	DeleteAllDefines( DEFINE_ALL );
	ReleaseIncludePaths();
	DeinitDefines();
	if( g.bDebugLog )
	{
		fprintf( stderr, WIDE("Allocates: %ld(%ld) Releases: %ld\n"), g.nAllocates, g.nAllocSize, g.nReleases );
		DumpMemory();
	}
	if( g.flags.skip_define_processing )
		return 0;
	return g.ErrorCount;
}
