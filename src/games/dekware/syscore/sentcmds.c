#include <stdhdrs.h>
#include <stdio.h>
#include <logging.h>
#include "sharemem.h"

#include "space.h"
#include "input.h" // burst/build

//-----------------------------------------------------------------------------
// Command Pack 2 contains methods for controlling sentients. Wake, Sleep,
// resume, tell... anything about commands and command redirection
//-----------------------------------------------------------------------------

int KillWake( int bWake, PSENTIENT ps, PTEXT parameters )
{
	PVARTEXT pvt = VarTextCreate();
	while( parameters )
	{
		PENTITY pe;
		pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
		if( !pe )
		{
			vtprintf( pvt, WIDE("Kill/wake Cannot see %s."), GetText(parameters) );
			EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			parameters = NEXTLINE( parameters ); // if find failed, parameters did not update
		}
		else
		{
			if( !bWake )
			{
					if( !pe->pControlledBy )
				{
					vtprintf( pvt, WIDE("%s is not aware."), GetText(parameters) );
					EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
				}
				else if( DestroyAwareness( pe->pControlledBy ) )
				{
					if( ps->CurrentMacro )
						ps->CurrentMacro->state.flags.bSuccess = TRUE;
				}
				else
				{
					vtprintf( pvt, WIDE("%s is scheduled to be destroyed."), GetText(parameters) );
						EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
					if( ps->CurrentMacro )
						ps->CurrentMacro->state.flags.bSuccess = TRUE;
				}
			}
			else
			{
					PSENTIENT ps;
				if( ps = CreateAwareness( pe ) )
				{
					UnlockAwareness( ps );
				}
				else
					Log1( WIDE("Failed to create awareness for %p"), pe );
			}
		}
	}
	VarTextDestroy( &pvt );
	return FALSE;
}

int CPROC KILL( PSENTIENT ps, PTEXT parameters )
{
	return KillWake( FALSE, ps, parameters );
}

int CPROC WAKE( PSENTIENT ps, PTEXT parameters )
{
	return KillWake( TRUE, ps, parameters );
}

int CPROC CMD_PROCESS( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pLine;
	if( parameters )
	{
		pLine = MacroDuplicateEx( ps, parameters, FALSE, TRUE);
		EnqueLink( &ps->Command->Input, pLine );
	}
	else
	{
		EnqueLink( &ps->Command->Input, ps->pLastResult );
		ps->pLastResult = NULL;
	}
	return FALSE;
}

int CPROC CMD_RUN( PSENTIENT ps, PTEXT parameters )
{
	PMACRO match;
	PMACROSTATE pms;
	PTEXT Command = GetParam( ps, &parameters );
	if( ( match = LocateMacro( ps->Current, GetText( Command ) ) ) )
	{
		PTEXT pArgs;
		int32_t i;
		if( match->nArgs > 0 )
		{
			PVARTEXT vt;
			if( ( i = CountArguments( ps, parameters ) ) < match->nArgs )
			{
				PTEXT t;
				 vt = VarTextCreate();
				 vtprintf( vt, WIDE("Macro requires: ") );
				 t = match->pArgs;
				 while( t )
				 {
					 vtprintf( vt, WIDE("%s"), GetText( t ) );
					 t = NEXTLINE( t );
					 if( t )
						 vtprintf( vt, WIDE(", ") );
				}
				vtprintf( vt, WIDE(".") );
				EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
				VarTextDestroy( &vt );
				return FALSE; // return...
			}
			if( i > match->nArgs )
			{
				DECLTEXT( msg, WIDE("Extra parameters passed to macro. Continuing but ignoring extra.") );
				EnqueLink( &ps->Command->Output, &msg );
			}
		}

		if( match->nArgs < 0 )
		{
			// duplicate singles of negative number...
			// then add one indirect to the remainder...
			pArgs = SegCreateIndirect( MacroDuplicateEx( ps, parameters, TRUE, TRUE ) );
		}
		else
		{
			pArgs = MacroDuplicateEx( ps, parameters, TRUE, TRUE );
		}
		pms = InvokeMacro( ps, match, pArgs );
		pms->state.flags.forced_run = 1;
	}
	return 0;
}

//--------------------------------------

void Tell( PSENTIENT ps // source
			, PSENTIENT pEnt  // destination
			, PTEXT pWhat // command parameter string...
			, int bSubst ) // should we subst this line?
{
	PVARTEXT vt;
	PENTITY peEnt;
	//Log( WIDE("Tell being issued...") );
	if( !pEnt )
	{
		//lprintf( WIDE("Have to find our own entity...") );
		if( peEnt = (PENTITY)FindThing( ps, &pWhat, ps->Current, FIND_VISIBLE, NULL ) )
		{
			//lprintf( WIDE("Found an entity from parameters.... ") );
			if( peEnt &&
				 peEnt->flags.bShadow )
				peEnt = ((PSHADOW_OBJECT)peEnt)->pForm;
			pEnt = peEnt->pControlledBy;
		}
		else
		{
			vt = VarTextCreate();
			//lprintf( WIDE("Cannot see entity: %s"), GetText( pWhat ) );
			vtprintf( vt, WIDE("Tell cannot see entity: %s"), GetText( pWhat ) );
			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			VarTextDestroy( &vt );
		}
	}
	if( pEnt )
	{
		PTEXT line;
		PTEXT pFrom;

		//line = BuildLine( pWhat );
		//lprintf( WIDE("Entity was given to us...[%s]"), GetText( line ) );
		//LineRelease( line );

		ps->pLastTell = pEnt->Current;

		// perform parameter substitutions for this....
		line = MacroDuplicateEx( ps, pWhat, TRUE, bSubst ); // no subst...
		pFrom = SegCreateIndirect( (PTEXT)ps );
		pFrom->flags |= TF_SENTIENT;
		EnqueLink( &pEnt->Command->Input,
					  SegAppend( pFrom
								  , line ) );
	}
	else if( pEnt )
	{
		// directly dispatch the command using my own sentience
		// on the new object...
		PENTITY peMe = ps->Current;
		int status;
		ps->Current = peEnt;
		peEnt->pControlledBy = ps;
		status = Process_Command( ps, &pWhat );
		peEnt->pControlledBy = NULL;
		ps->Current = peMe;

		//DECLTEXT( msg, WIDE("Object is not aware.") );
		// this state should never happen anymore
		//EnqueLink( &ps->Command->Output, &msg );
	}
}


int CPROC TELL( PSENTIENT ps, PTEXT parameters )
{
	Tell( ps, NULL, parameters, TRUE );
	return FALSE;
}
//--------------------------------------
int CPROC REPLY( PSENTIENT ps, PTEXT parameters )
{
	PSENTIENT psReplyTo;
	if( parameters ) // must have something to reply with...
	{
		if( !ps->CurrentMacro )
		{
			if( !( psReplyTo = ps->pToldBy ) )
			{
				DECLTEXT( msg, WIDE("Reply may only be used from a macro, or last command was not a tell."));
				EnqueLink( &ps->Command->Output, &msg );
				return FALSE;
			}
		}
		else
			psReplyTo = ps->CurrentMacro->pInvokedBy;

		if( !psReplyTo )
			psReplyTo = ps; // invoked by self...
		Tell( ps, psReplyTo, parameters, TRUE );
	}
	return FALSE;
}

int CPROC SUSPEND( PSENTIENT ps, PTEXT parameters )
{
		PMACROSTATE pms;
		pms = (PMACROSTATE)PeekData( &ps->MacroStack );
		if( pms )
		{
			if( !ps->flags.resume_run )
				pms->state.flags.macro_suspend = TRUE;
			ps->flags.resume_run = FALSE;
		}
	return FALSE;
}
//--------------------------------------
int CPROC RESUME( PSENTIENT ps, PTEXT parameters )
{
	PMACROSTATE pms;
	pms = (PMACROSTATE)PeekData( &ps->MacroStack );
	if( pms )
	{
		ps->flags.resume_run = FALSE;
		if( pms->state.flags.macro_suspend )
			pms->state.flags.macro_suspend = FALSE;
		else
		{
			ps->flags.resume_run = TRUE;
		}
		{
			if( pms->state.flags.macro_delay )
			{
				pms->state.flags.data.delay_end -= GetTickCount();
				pms->state.flags.macro_delay = FALSE;
				ps->flags.resume_run = FALSE;
			}
		}
	}

	return FALSE;
}
//--------------------------------------
int CPROC DELAY( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( !ps->CurrentMacro )
	{
		DECLTEXT( msg, WIDE("Must be in a macro to execute a /DELAY") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		unsigned int len;
		len = atoi( GetText( temp ) );
		if( !len )
		{
			DECLTEXT( msg, WIDE("Invalid delay length...") );
			EnqueLink( &ps->Command->Output, &msg );
		}
		else
		{
			if( !ps->flags.resume_run )
			{
				ps->flags.scheduled = 1;
				lprintf( WIDE("Setting wait on thread.") );
				AddTimerEx( len, 0, TimerWake, (uintptr_t)ps );
				ps->CurrentMacro->state.flags.data.delay_end = GetTickCount() + len;
				ps->CurrentMacro->state.flags.macro_delay = TRUE;
			}
			ps->flags.resume_run = FALSE;
		}
	}
	return FALSE;
}
//--------------------------------------
int CPROC GetDelay( PSENTIENT ps, PTEXT parameters )
{
	PVARTEXT vt;
	if( !ps->CurrentMacro )
		ps->pLastResult = SegCreateFromText( WIDE("0") );
	else
	{
		vt = VarTextCreate();
		vtprintf( vt, WIDE("%ld")
					, ps->CurrentMacro->state.flags.data.delay_end );
		ps->pLastResult = VarTextGet( vt );
	}
	return  FALSE;
}
//--------------------------------------
int CPROC CMD_WAIT( PSENTIENT ps, PTEXT parameters )
{
	if( ps->CurrentMacro )
		ps->CurrentMacro->state.flags.bInputWait = TRUE;
	else
	{
		DECLTEXT( msg, WIDE("Must be in a macro to /WAIT.") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_TRACE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( ps->CurrentMacro )
	{
		if( ( temp = GetParam( ps, &parameters ) ) )
		{
			if( TextIs( temp, WIDE("on") ) )
			{
				ps->CurrentMacro->state.flags.bTrace = TRUE;
			} else if( TextIs( temp, WIDE("off") ) )
			{
				ps->CurrentMacro->state.flags.bTrace = FALSE;
			}
		}
		else
			ps->CurrentMacro->state.flags.bTrace ^= 1;
	}
	else
	{
		extern int gbTrace;
		if( ( temp = GetParam( ps, &parameters ) ) )
		{
			if( TextIs( temp, WIDE("on") ) )
				gbTrace = TRUE;
			else if( TextIs( temp, WIDE("off") ) )
				gbTrace = FALSE;
			else
				gbTrace ^= 1;
		}
		else
			gbTrace ^= 1;
	}
	return FALSE;
}

int CPROC BECOME( PSENTIENT ps, PTEXT parameters )
{
	// CANNOT Become a held object...
	PENTITY pe;
	if( pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL ) )
	{
		if( !pe->pControlledBy )
		{
			DECLTEXT( msg, WIDE("You are now that object.") );
			ps->Current->pControlledBy = NULL; // is no longer controlled
			ps->Current = pe;
			EnqueLink( &ps->Command->Output, &msg );
		}
		else
		{
			DECLTEXT( msg, WIDE("Object is already aware - please use /monitor.") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	else
	{
		DECLTEXT( msg, WIDE("Object is not visible...") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	return FALSE;
}
//--------------------------------------
int CPROC MONITOR( PSENTIENT ps, PTEXT parameters )
{
	PENTITY pe;
	pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_NEAR, NULL );
	if( !pe )
		pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_IN, NULL );
	if( pe )
	{
		if( !pe->pControlledBy )
		{
			DECLTEXT( msg, WIDE("Object is not aware - please use /become.") );
			EnqueLink( &ps->Command->Output, &msg );
		}
		else
		{
			DECLTEXT( msg, WIDE("You are now act as that object.") );
			ps =
				global.PLAYER = pe->pControlledBy;
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_RESULT( PSENTIENT ps, PTEXT parameters )
{
	if( ps->pLastResult )
		LineRelease( ps->pLastResult );
	ps->pLastResult = MacroDuplicateEx( ps, parameters, TRUE, TRUE );
	return FALSE;
}
//--------------------------------------
int CPROC CMD_GETRESULT( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;

	temp = GetParam( ps, &parameters );
	if( temp )
	{
		AddVariable( ps, ps->Current, temp, ps->pLastResult );
	}
	else
	{
		DECLTEXT( msg, WIDE("Must specify a variable name to store result in...") );
		EnqueLink( &ps->Command->Output, &msg );
	}

	return FALSE;
}
//--------------------------------------
int CPROC CMD_INPUT( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pSave, temp;
	pSave = parameters;
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		if( temp == pSave )
		{
			DECLTEXT( msg, WIDE("Invalid Variable Reference to INPUT...") );
			EnqueLink( &ps->Command->Output, &msg );
			return FALSE;
		}
		ps->MacroInputVar = temp;
		LineRelease( GetIndirect( ps->MacroInputVar ) );
		SetIndirect( ps->MacroInputVar, NULL );
		ps->flags.macro_input = TRUE;
	}
	return FALSE;
}
//--------------------------------------
int CPROC PAGE( PSENTIENT ps, PTEXT parameters )
{
	DECLTEXT( page_break, WIDE("") );
	page_break.flags |= TF_FORMATEX;
	page_break.format.flags.format_op = FORMAT_OP_PAGE_BREAK;
	EnqueLink( &ps->Command->Output, (PTEXT)&page_break );
	return FALSE;
}
//--------------------------------------
int CPROC ECHO( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pLine, pOutput;
	extern int gbTrace;
	//if( gbTrace )
	//  	DebugBreak();
	pLine = MacroDuplicateEx( ps, parameters, TRUE, TRUE );
	if( pLine )
	{
		pLine->format.position.offset.spaces = 0;
		pOutput = BuildLine( pLine );
		//lprintf( WIDE("Built line from macro substitution... %s %s"), GetText( pLine ), GetText( pOutput ) );
		EnqueLink( &ps->Command->Output, pOutput );
		LineRelease( pLine );
	}
	else
	{
		 DECLTEXT( blank, WIDE(" ") );
		EnqueLink( &ps->Command->Output, &blank );
	}
	return FALSE;
}
//--------------------------------------
int CPROC FECHO( PSENTIENT ps, PTEXT parameters )
{

	PTEXT pLine, pOutput;
	pLine = MacroDuplicateEx( ps, parameters, TRUE, TRUE );
	if( pLine )
	{
		 pOutput = BuildLine( pLine );
		 LineRelease( pLine );
		 pOutput->flags |= TF_NORETURN; //mark first seg to disallow lead cr/lf
		 EnqueLink( &ps->Command->Output, pOutput );
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_MACRO( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		PENTITY pe;
		PMACRO pr;
		if( ps->pRecord )
		{
			ps->nRecord++;
			return FALSE;
		}
		// blah - to be /macro in object named <parameters...>
		if( TextLike( temp, WIDE("in") ) )
		{
			pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
			if( !pe )
			{
				DECLTEXT( msg, WIDE("Could not see the entity to define macro in...") );
				EnqueLink( &ps->Command->Output, &msg );
				return FALSE;
			}
			temp = GetParam( ps, &parameters );
			if( !temp )
			{
				DECLTEXT( msg, WIDE("Must specify a name for the macro to create.") );
				EnqueLink( &ps->Command->Output, &msg );
				return FALSE;
			}
		}
		else
			pe = ps->Current;
		pr = ps->pRecord;
		ps->pRecord = CreateMacro( pe, &ps->Command->Output, SegDuplicate(temp) );
		if( ps->pRecord ) // macro was not a duplicated name...
		{
			if( ( ps->pRecord->nArgs = CountArguments( ps, parameters) ) )
				ps->pRecord->pArgs = MacroDuplicateEx( ps, parameters, FALSE, TRUE );
			else
				ps->pRecord->pArgs = NULL;
		}
		else
		{
			DECLTEXT( msg, WIDE("Macro name conflicts with previous command or macro.") );
			EnqueLink( &ps->Command->Output, &msg );
			ps->pRecord = pr; // restore record to last...
		}
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_RETURN( PSENTIENT ps, PTEXT parameters )  // combination state...
{
	if( !ps->CurrentMacro )
	{
 		return FALSE;
	}
	{
		INDEX idx;
		PTEXT pVar;
		LIST_FORALL( ps->CurrentMacro->pVars, idx, PTEXT, pVar )
		{
			/*
			PTEXT pVal;
			 // this is just silly to do...
			 // line release releases the entire stream of text
			if( ( pVal = NEXTLINE( pVar ) ) )
			{
				LineRelease( GetIndirect( pVal ) );
				SetIndirect( pVal, NULL );
			}
			*/
			LineRelease( pVar ); // name, and indirect seg...
		}
		DeleteList( &ps->CurrentMacro->pVars );
		ps->CurrentMacro->pVars = NULL;
		if( ps->CurrentMacro->pArgs )
		{
			LineRelease( ps->CurrentMacro->pArgs ); // a single line of (DEEP indirects?)
			ps->CurrentMacro->pArgs = NULL;
		}
	}
	ps->CurrentMacro = NULL;
	return FALSE;
}
//--------------------------------------
int CPROC CMD_ENDMACRO( PSENTIENT ps, PTEXT parameters )
{
	if( ps->nRecord )
	{
		ps->nRecord--;
		return FALSE;
	}
	if( ps->pRecord )
		ps->pRecord = NULL;
	else
		CMD_RETURN(ps, parameters );
	return FALSE;
}
//--------------------------------------
void TerminateMacro( PMACROSTATE pms )
{
	if( pms )
	{
		// jump macro to /ENDMAC statement.
		pms->nCommand = pms->pMacro->nCommands - 1;
		pms->state.flags.macro_delay = FALSE;
	}
}

int CPROC CMD_STOP( PSENTIENT ps, PTEXT parameters )
{
	PMACROSTATE pms;
	pms = (PMACROSTATE)PeekData( &ps->MacroStack );
	if( pms )
	{
		TerminateMacro( pms );
	}
	return FALSE;
}

void ListMacro( PSENTIENT ps, PVARTEXT vt, PMACRO pm, TEXTCHAR *type )
{
	PTEXT pt;
	INDEX idx;
	vtprintf( vt, WIDE("%s \'%s\' ("), type, GetText( GetName(pm) ) );
	{
		PTEXT param;
		param = pm->pArgs;
		while( param )
		{
			if( PRIORLINE( param ) )
				vtprintf( vt, WIDE(", %s"), GetText( param ) );
			else
				vtprintf( vt, WIDE("%s"), GetText( param ) );
			param = NEXTLINE( param );
		}
		vtprintf( vt, WIDE(")") );
		EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
	}
	LIST_FORALL( pm->pCommands, idx, PTEXT, pt )
	{
		PTEXT x;
		x = BuildLine( pt );
		vtprintf( vt, WIDE("	 %s"), GetText( x ) );
		LineRelease( x );
		EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
	}
}


//---------------------------------------------------------------------------
int CPROC CMD_LIST( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	PVARTEXT vt;
	vt = VarTextCreate();
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		INDEX idx;
		int bFound = FALSE;
		PMACRO pm;
		if( TextLike( temp, WIDE("on") ) )
		{
			INDEX idx;
			PMACRO behavior;
			temp = GetParam( ps, &parameters );
			LIST_FORALL( ps->Current->pGlobalBehaviors, idx, PMACRO, behavior )
			{
				if( LikeText( temp, (PTEXT)GetLink( &global.behaviors, idx ) ) == 0 )
				{
					bFound = TRUE;
					ListMacro( ps, vt, behavior, WIDE("Global Behavior") );
					break;
				}
			}
			if( !bFound )
			{
				LIST_FORALL( ps->Current->pBehaviors, idx, PMACRO, behavior )
				{
					if( LikeText( temp, (PTEXT)GetLink( &ps->Current->behaviors, idx ) ) == 0 )
					{
						bFound = TRUE;
						ListMacro( ps, vt, behavior, WIDE("Object Behavior") );
						break;
					}
				}
			}
			if (!bFound)
			{
				vtprintf( vt, WIDE("Behavior %s is not defined."), GetText(temp));
			}

		}
		else
		{
			LIST_FORALL( ps->Current->pMacros, idx, PMACRO, pm )
				if( SameText( temp, GetName(pm) ) == 0 )
				{
					bFound = TRUE;
					ListMacro( ps, vt, pm, WIDE("Macro") );
					break;
				}
			if (!bFound)
			{
				vtprintf( vt, WIDE("Macro %s is not defined."), GetText(temp));
			}
		}
	}
	else
	{
			vtprintf( vt, WIDE("Defined Macros: ") );
		{
			INDEX idx;
			PMACRO pm;
			LOGICAL didone = FALSE;
			LIST_FORALL( ps->Current->pMacros, idx, PMACRO, pm )
			{
					vtprintf( vt, WIDE("%s%s"), (didone)?WIDE(", "): WIDE(""),
											GetText( GetName(pm) ));
				didone = TRUE;
			}
			if( didone )
					vtprintf( vt, WIDE(".") );
			else
					vtprintf( vt, WIDE("None.") );
			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );

		}
		vtprintf( vt, WIDE("Defined Behaviors: ") );
		{
			INDEX idx;
			PMACRO behavior;
			LOGICAL didone = FALSE;
			LIST_FORALL( ps->Current->pGlobalBehaviors, idx, PMACRO, behavior )
			{
				// although the name exists in the behavior macro
				// that name is 'on accept'... the better name to choose
				// is the related name node from global.beavhiors or current->behaviors
				vtprintf( vt, WIDE("%s%s"), (didone)?WIDE(", "):WIDE(""), GetText( (PTEXT)GetLink( &global.behaviors, idx ) ) );
				didone = TRUE;
			}
			LIST_FORALL( ps->Current->pBehaviors, idx, PMACRO, behavior )
			{
				// although the name exists in the behavior macro
				// that name is 'on accept'... the better name to choose
				// is the related name node from global.beavhiors or current->behaviors
				vtprintf( vt, WIDE("%s%s"), (didone)?WIDE(", "):WIDE(""), GetText( (PTEXT)GetLink( &ps->Current->behaviors, idx ) ) );
				didone = TRUE;
			}
			if( didone )
			{
				vtprintf( vt, WIDE(".") );
				EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			}
		}
		VarTextDestroy( &vt );
	}
	return FALSE;
}
//--------------------------------------

//--------------------------------------------------------------------------

int IsBlank( PTEXT pText )
{
	int rval;
	rval = TRUE;
	while( pText && rval )
	{
		if( pText->flags & TF_INDIRECT )
			rval = IsBlank( GetIndirect( pText ) );
		else
		{
			if( GetTextSize( pText ) )
			{
				rval = FALSE;
				break;
			}
		}
		pText = NEXTLINE( pText );
	}
	return rval;
}

//--------------------------------------------------------------------------

#if 0
int64_t IntNumber( PTEXT pText )
{
	TEXTCHAR *p;
	int s;
	int begin;
	int64_t num;
	p = GetText( pText );
	if( !pText || !p )
		return FALSE;
	if( pText->flags & TF_INDIRECT )
		return IntNumber( GetIndirect( pText ) );

	s = 0;
	num = 0;
	begin = TRUE;
	while( *p )
	{
		if( *p == '.' )
			break;
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

double FltNumber( PTEXT pText )
{
	TEXTCHAR *p;
	int s, begin, bDec = FALSE;
	double num;
	double base = 1;
	double temp;
	p = GetText( pText );
	if( !p )
		return FALSE;
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
	if( s )
		num *= -1;
	return num;
}
#endif

//--------------------------------------------------------------------------

int TestList( PTEXT word, PTEXT list, int same )
{
	//PTEXT start = word;
	while( list )
	{
		if( same )
		{
			Log2( WIDE("Testing %s vs %s\n"), GetText( word ), GetText( list ) );
			if( SameText( word, list ) == 0 )
			{
				Log( WIDE("Success\n") );
				return 1;
			}
		}
		else
		{
			Log2( WIDE("Testing %s vs %s\n"), GetText( word ), GetText( list ) );
			if( LikeText( word, list ) == 0 )
			{
				Log( WIDE("Success\n") );
				return 1;
			}
		}
		list = NEXTLINE( list );
	}
	return 0;
}

//--------------------------------------

int CPROC CMD_COMPARE( PSENTIENT ps, PTEXT parameters )
{
	if( ps->CurrentMacro )
	{
		PTEXT op, d1, d2;
		int d1IsVar;
		if( ( d1 = GetParam( ps, &parameters ) ) )
		{
			if( ( op = GetParam( ps, &parameters ) ) )
			{
				d2 = GetParam( ps, &parameters );
				if( d1->flags&TF_INDIRECT )
				{
					d1IsVar = TRUE;
					d1 = GetIndirect( d1 ); // may be more than one word
				}
				else
				{
					d1IsVar = FALSE; // must only be one word....
				}

				if( d2 &&
					 d2->flags&TF_INDIRECT )
					d2 = GetIndirect( d2 );
				// d2 may be more than one word...
				if( d1 && d2 )
				{
					if( TextIs( op, WIDE("is") ) ) {
						ps->CurrentMacro->state.flags.bSuccess =
								CompareStrings( d1, !d1IsVar
												  , d2, FALSE // (d2->flags & TF_SINGLE)
												  , TRUE );
					} else if( TextIs( op, WIDE("like") ) ) {
						lprintf( WIDE("Comparing like '%s' and '%s'"),  GetText( d1 ), GetText( d2 ) );
						if( GetTextSize( d1 ) && GetTextSize( d2 ) )
							ps->CurrentMacro->state.flags.bSuccess =
								CompareStrings( d1, !d1IsVar
												  , d2, FALSE // (d2->flags & TF_SINGLE)
												  , FALSE );
						else
							if( !GetTextSize( d1 ) && !GetTextSize( d2 ) )
								ps->CurrentMacro->state.flags.bSuccess = TRUE;

					} else if( TextIs( op, WIDE("in") ) )
					{
						if( GetTextSize( d1 ) && GetTextSize( d2 ) )
						{
							ps->CurrentMacro->state.flags.bSuccess =
								TestList( d1, d2, FALSE );
						}
						else
						{
							if( !GetTextSize( d1 ) && !GetTextSize( d2 ) )
								ps->CurrentMacro->state.flags.bSuccess = TRUE;
						}
					} else if( TextIs( op, WIDE("typeof") ) )
					{
						if( GetTextSize( d1 ) && GetTextSize( d2 ) )
						{
							ps->CurrentMacro->state.flags.bSuccess =
								IsObjectTypeOf( ps, d1, d2 );
						}
					}
					// morethan, if fail, is less or equal
					else if ( TextIs( op, WIDE("morethan") ) )
					{
						if( IsNumber( d1 ) && IsNumber( d2 ) )
						{
							double n1, n2;
							n1 = FloatCreateFromSeg( d1 );
							n2 = FloatCreateFromSeg( d2 );
							if( n1 > n2 )
								ps->CurrentMacro->state.flags.bSuccess = TRUE;
						}
						else
							if( strcmp( GetText( d1 ), GetText( d2 ) ) > 0 )
								ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					// lessthan, if fail, is more or equal
					else if( TextIs( op, WIDE("lessthan") ) )
					{
						if( IsNumber( d1 ) && IsNumber( d2 ) )
						{
							double n1, n2;
							n1 = FloatCreateFromSeg( d1 );
							n2 = FloatCreateFromSeg( d2 );
							if( n1 < n2 )
								ps->CurrentMacro->state.flags.bSuccess = TRUE;
						}
						else
							if( strcmp( GetText( d1 ), GetText( d2 ) ) < 0 )
								ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					else {
						DECLTEXT( msg, WIDE("Compare : Operand undefined value.") );
						EnqueLink( &ps->Command->Output, &msg );
					}
				}
				else
				{
					if( TextIs( op, WIDE("is_tag") ) ) {
						ps->CurrentMacro->state.flags.bSuccess = (d1->flags & TF_TAG);
					}
					else if( TextIs( op, WIDE("is_quote") ) ) {
						if( d1 && d1->flags & TF_QUOTE )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					else if( TextIs( op, WIDE("is_device") ) ) {
						if( d1 && FindDevice( d1 ) )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					else if( TextIs( op, WIDE("blank") ) ) {
						ps->CurrentMacro->state.flags.bSuccess = IsBlank( d1 );
					}
					else if( TextIs( op, WIDE("eol") ) ) {
						ps->CurrentMacro->state.flags.bSuccess = d1 && !GetTextSize( d1 ) && !(d1->flags&IS_DATA_FLAGS);
					}
					else if( TextIs( op, WIDE("is") ) ) {
						ps->CurrentMacro->state.flags.bSuccess = FALSE;
					}
					else if( TextIs( op, WIDE("like") ) ) {
						ps->CurrentMacro->state.flags.bSuccess = FALSE;
					}
					else if( TextIs( op, WIDE("binary") ) ) {
						if( d1->flags & TF_BINARY )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					else if( TextIs( op, WIDE("number") ) )
					{
						if( IsNumber( d1 ) )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					else {
						DECLTEXT( msg, WIDE("Compare : Operand undefined value.") );
						ps->CurrentMacro->state.flags.bSuccess = FALSE;
						EnqueLink( &ps->Command->Output, &msg );
					}
				}
			}
			else
			{
				if( !(d1->flags&TF_INDIRECT) ||
					 GetIndirect( d1 ) ) // might have been a null variable in indirect
				{
					if( TextIs( d1, WIDE("connected") ) )
					{
						if( ps->Data &&
							 ps->Data->Type &&
							 !ps->Data->flags.Closed)
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					// not sure about this keyword......
					else if( TextIs( d1, WIDE("active") ) )
					{
						if( ps->Command )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
					else if( TextIs( d1, WIDE("is_console") ) )
					{
						extern int IsConsole;
						if( IsConsole )
							ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
				}
			}
		}
	}
	else
	{
		DECLTEXT( msg, WIDE("Compare should be used in a macro...") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	return FALSE;
}

//--------------------------------------
int CPROC CMD_IF( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( !ps->CurrentMacro )
	{
		DECLTEXT( msg, WIDE("Must be in a macro to execute IF.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		if( TextLike( temp, WIDE("success") )
		  || TextLike( temp, WIDE("true") )
		  		  )
		{
			if( !ps->CurrentMacro->state.flags.bSuccess )
			{
				ps->CurrentMacro->state.flags.bFindElse = TRUE;
				ps->CurrentMacro->state.flags.data.levels = 0;
			}
		}
		else if( TextLike( temp, WIDE("fail") )
				  || TextLike( temp, WIDE("failure") )
				  || TextLike( temp, WIDE("false") )
				 )
		{
			if( ps->CurrentMacro->state.flags.bSuccess )
			{
				ps->CurrentMacro->state.flags.bFindElse = TRUE;
				ps->CurrentMacro->state.flags.data.levels = 0;
			}
		}
		else
		{
			PVARTEXT pvt = VarTextCreate();
			vtprintf( pvt, WIDE("Unknown expression for IF:%s"), GetText( temp ) );
			EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			VarTextDestroy( &pvt );
		}
	}
	else
	{
		DECLTEXT( msg, WIDE("Have to specify either [true/success] or [false/fail/failure] after IF.") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_ELSE( PSENTIENT ps, PTEXT parameters )
{
	if( !ps->CurrentMacro )
	{
		DECLTEXT( msg, WIDE("Must be in a macro to execute ELSE.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( !ps->CurrentMacro->state.flags.data.levels )
	{
		if( !ps->CurrentMacro->state.flags.bFindElse )
			ps->CurrentMacro->state.flags.bFindEndIf = TRUE;
		else
			ps->CurrentMacro->state.flags.bFindElse = FALSE;
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_ENDIF( PSENTIENT ps, PTEXT parameters )
{
	if( !ps->CurrentMacro )
	{
		DECLTEXT( msg, WIDE("Must be in a macro to execute ENDIF.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;

	}
	if( !ps->CurrentMacro->state.flags.data.levels )
	{
		ps->CurrentMacro->state.flags.bFindElse = FALSE;
		ps->CurrentMacro->state.flags.bFindEndIf = FALSE;
	}
	else
		ps->CurrentMacro->state.flags.data.levels--;
	return FALSE;
}
//--------------------------------------
int CPROC CMD_LABEL( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( !ps->CurrentMacro )
	{
		DECLTEXT( msg, WIDE("Must be in a macro to execute a LABEL.") );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	if( ps->CurrentMacro->state.flags.bFindLabel )
	{
		if( ( temp = GetParam( ps, &parameters ) ) )
		{
			//lprintf( WIDE("Comparing %s and %s"), GetText( temp ), GetText( ps->CurrentMacro->state.flags.data.pLabel ) );
			if( SameText( ps->CurrentMacro->state.flags.data.pLabel,
							  temp ) == 0 )
			{
				SegRelease( ps->CurrentMacro->state.flags.data.pLabel );
				ps->CurrentMacro->state.flags.data.pLabel = NULL;
				ps->CurrentMacro->state.flags.bFindLabel = FALSE;
			}
		}
	}
	return FALSE;
}
//--------------------------------------
int CPROC CMD_GOTO( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( !ps->CurrentMacro )
	{
		DECLTEXT( msg, WIDE("Must be in a macro to execute a GOTO.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( ( temp = GetParam( ps, &parameters ) ) )
	{
		ps->CurrentMacro->nCommand = 0; // start at the beginning of the macro...
		ps->CurrentMacro->state.flags.bFindLabel = TRUE;
		ps->CurrentMacro->state.flags.bFindElse = FALSE;
		ps->CurrentMacro->state.flags.bFindEndIf = FALSE;
		ps->CurrentMacro->state.flags.data.pLabel = SegDuplicate(temp); // is going to stay during macro...
	}
	return FALSE;
}
//--------------------------------------

int CPROC ForEach( PSENTIENT ps, PTEXT parameters )
{
	// build list with SegCreateIndirect( )->flags |= TF_ENTITY
	// or not...
	FOREACH_STATE newstate;
	if( !ps->CurrentMacro )
	{
		S_MSG( ps, WIDE("FOREACH may only be used in a macro.") );
		return 0;
	}
	{
		newstate.nForEachTopMacroCmd = ps->CurrentMacro->nCommand; // don't really care about the statement itself.
		newstate.list_next = newstate.list_prior = NULL;
		newstate.item = NULL;
		// /foreach item in %room
		newstate.variablename = SegDuplicate( GetParam( ps, &parameters ) );
		if( !newstate.variablename )
		{
			S_MSG( ps, WIDE("You need to specify a variable name for foreach") );
			return 0;
		}
	}
	{
		PTEXT iteration_type = GetParam( ps, &parameters );
		// switch( iternation_type ) blah...
		if( TextLike( iteration_type, WIDE("in") ) )
		{
			PLIST paramlist = NULL;
			PENTITY pe;
			INDEX idx;
			while( pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL ) )
			{
				AddLink( &paramlist, pe );
			}
			LIST_FORALL( paramlist, idx, PENTITY, pe )
			{
				PENTITY within;
				INDEX idx2;
				LIST_FORALL( pe->pContains, idx2, PENTITY, within )
				{
					PTEXT tmp;
					newstate.list_next = SegAppend( newstate.list_next, tmp = SegCreateIndirect( GetName( within ) ) );
					//tmp->flags |= TF_ENTITY;
				}
			}
			DeleteList( &paramlist );
			// this is first - step is a little different...
			newstate.item = newstate.list_next;
			newstate.list_next = NEXTLINE( newstate.list_next );
			SegGrab( newstate.item );
			AddVariableExxx( ps, ps->Current, newstate.variablename, newstate.item, FALSE, FALSE, TRUE DBG_SRC );
			//AddVariableExx( ps, ps->Current, newstate.variablename, newstate.item );
			PushData( &ps->CurrentMacro->pdsForEachState, &newstate );
		}
		else if( TextLike( iteration_type, WIDE("token") ) )
		{
		}
		else if( TextLike( iteration_type, WIDE("phrase") ) )
		{

		}
		else if( TextLike( iteration_type, WIDE("near") ) )
		{
			PLIST paramlist = NULL;
			PENTITY pe;
			INDEX idx;
			while( pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL ) )
			{
				AddLink( &paramlist, pe );
			}
			LIST_FORALL( paramlist, idx, PENTITY, pe )
			{
				PENTITY within;
				INDEX idx2;
				LIST_FORALL( pe->pContains, idx2, PENTITY, within )
				{
					PTEXT tmp;
					newstate.list_next = SegAppend( newstate.list_next, tmp = SegCreateIndirect( GetName( within ) ) );
					tmp->flags |= TF_ENTITY;
				}
			}
			DeleteList( &paramlist );
			// this is first - step is a little different...
			newstate.item = newstate.list_next;
			newstate.list_next = NEXTLINE( newstate.list_next );
			SegGrab( newstate.item );
			AddVariableExxx( ps, ps->Current, newstate.variablename, newstate.item, FALSE, FALSE, TRUE DBG_SRC );
			PushData( &ps->CurrentMacro->pdsForEachState, &newstate );

		}
		else if( TextLike( iteration_type, WIDE("around") ) )
		{

		}
		else if( TextLike( iteration_type, WIDE("exit") ) ||
				  TextLike( iteration_type, WIDE("attached") ) ||
				  TextLike( iteration_type, WIDE("on") ) )
		{

		}
	}
	return 0;
}

int CPROC StepEach( PSENTIENT ps, PTEXT parameters )
{
	if( !ps->CurrentMacro )
	{
		S_MSG( ps, WIDE("STEP may only be used in a macro.") );
		return 0;
	}
	// consider using an alternate parameter to skip to a higher top level
	// goto used within a foreach/step construct will cause errors.
	// this stack will probably need to be blown away ... but then
	// jumping to within a higher level foreach will cause errors when
	// the final /step is done...
	// heh - maybe we should just be like smart of something
	// and look ahead to see if we'll blow up - yeah right.
	{
		PFOREACH_STATE pfes = (PFOREACH_STATE)PeekData( &ps->CurrentMacro->pdsForEachState );
		if( !pfes )
		{
			S_MSG( ps, WIDE("STEP may only be used after a FOREACH.") );
			return 0;
		}
		{
			PTEXT direction = GetParam( ps, &parameters );
			if( !direction
				|| TextLike( direction, WIDE("next") ) )
			{
				pfes->list_prior = SegInsert( pfes->item, pfes->list_prior );
				pfes->item = pfes->list_next;
				if( !pfes->item )
				{
					// done.
					PopData( &ps->CurrentMacro->pdsForEachState );
					LineRelease( pfes->list_prior );
					// next and item are already emtpy (NULL);
					LineRelease( pfes->variablename );
					return 0;
				}
				pfes->list_next = NEXTLINE( pfes->list_next );
				SegGrab( pfes->item );
				AddVariableExxx( ps, ps->Current, pfes->variablename, pfes->item, FALSE, FALSE, TRUE DBG_SRC );
				// wonder how CMD_GOTO handles this...
				ps->CurrentMacro->nCommand = pfes->nForEachTopMacroCmd;
			}
			else if( TextLike( direction, WIDE("prior") ) )
			{
			}
			else if( TextLike( direction, WIDE("first") ) )
			{
			}
			else if( TextLike( direction, WIDE("last") ) )
			{
			}
		}
	}
	return 1;
}

//--------------------------------------
int CPROC CMD_WHILE( PSENTIENT ps, PTEXT parameters )
{
	return FALSE;
}


//--------------------------------------
int CPROC PROMPT( PSENTIENT ps, PTEXT parameters )
{
	prompt( ps );
	return FALSE;
}

//--------------------------------------------------------------------------
// behaviors are called, run in/by the object they are defined for
// a useful variable is then WIDE("%commander") which is the object
// which caused the invokation of the behavior.
#if 0
#define DEF_BEHAVIOR( ps, name, member )			  if( ps->pRecord ) \
			{							\
				ps->nRecord++;	  \
			}							\
			else						\
			{							\
				PMACRO pMacro;											 \
				pMacro = Allocate( sizeof( MACRO ) );			  \
				MemSet( pMacro, 0, sizeof( MACRO ) );			  \
				pMacro->flags.bMacro = TRUE;						  \
				pMacro->nArgs = 0;									  \
				pMacro->pArgs = NULL;								  \
				pMacro->pName = (PTEXT)&name;						\
				pMacro->pDescription = NULL;						 \
				pMacro->pCommands = NULL;							 \
				ps->pRecord = pMacro;								  \
				AssignBehavior( ps->Current, #member, pMacro );

//				if( !ps->Current->pBehaviors )					  \
//				{															 \
//					ps->Current->pBehaviors = Allocate( sizeof( BEHAVIOR ) );\
//					MemSet( ps->Current->pBehaviors, 0, sizeof( BEHAVIOR ) );\
//	}															 \


//				if( ps->Current->pBehaviors->member )			\
//					DestroyMacro( NULL, ps->Current->pBehaviors->member ); \
//				ps->Current->pBehaviors->member = pMacro;			  \
//			}
#endif

int CPROC DefineOnBehavior( PSENTIENT ps, PTEXT parameters )
{
	// /on <enter/leave/attach/grab/store/OnEnterNear>
	//	  <entrance/exit>
	// ... commands ...
	// /endmac
	INDEX idx;
	PTEXT pTest;
	PLIST *ppBehaviors;
	PTEXT pAction;
	pAction = GetParam( ps, &parameters );
	if( pAction )
	{
		LIST_FORALL( ps->Current->behaviors, idx, PTEXT, pTest )
		{
			if( TextLike( pAction, GetText( pTest ) ) )
			{
				ppBehaviors = &ps->Current->pBehaviors;
				break;
			}
		}
		if( !pTest )
		{
			LIST_FORALL( global.behaviors, idx, PTEXT, pTest )
			{
				if( TextLike( pAction, GetText( pTest ) ) )
				{
					ppBehaviors = &ps->Current->pGlobalBehaviors;
					break;
				}
			}
		}
		if( pTest )
		{
			if( ps->pRecord )
			{
				ps->nRecord++;
			}
			else
			{
				PMACRO pMacro;
				PTEXT pMacroName;
				//TEXTCHAR name[64];

				//snprintf( name, sizeof( name ), WIDE("%s"), GetText( pTest ) );
				pMacroName = SegCreateFromText( GetText( pTest ) );
				pMacro = New( MACRO );
				MemSet( pMacro, 0, sizeof( MACRO ) );
				pMacro->flags.bMacro = TRUE;
				pMacro->Used = 1;
				pMacro->nArgs = 0;
				pMacro->pArgs = NULL;
				pMacro->pName = pMacroName;
				pMacro->pDescription = NULL;
				pMacro->pCommands = NULL;
				ps->pRecord = pMacro;
				{
					PMACRO pOld = (PMACRO)GetLink( ppBehaviors, idx );
					if( pOld )
						DestroyMacro( NULL, pOld );
				}
				SetLink( ppBehaviors, idx, pMacro );
			}
		}
		else
		{
			S_MSG( ps, WIDE("Could not find behavior called \'%s\'"), GetText( pAction ) );
		}
	}
	else
	{
		if( !ps->CurrentMacro )
		{
			PVARTEXT pvt = VarTextCreate();
			PTEXT name;
			INDEX idx;
			vtprintf( pvt, WIDE(" --- Available behaviors --- ") );
			EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			LIST_FORALL( global.behaviors, idx, PTEXT, name )
			{
				vtprintf( pvt, WIDE("%-15s %s"), GetText( name ), GetText( NEXTLINE( name ) ) );
				EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			}
			LIST_FORALL( ps->Current->behaviors, idx, PTEXT, name )
			{
				vtprintf( pvt, WIDE("%-15s %s"), GetText( name ), GetText( NEXTLINE( name ) ) );
				EnqueLink( &ps->Command->Output, VarTextGet( pvt ) );
			}
		}
	}
	return 0;
}

// $Log: sentcmds.c,v $
// Revision 1.46  2005/08/08 09:30:24  d3x0r
// Fixed listing and handling of ON behaviors.  Protected against some common null spots.  Fixed databath destruction a bit....
//
// Revision 1.45  2005/05/30 11:51:42  d3x0r
// Remove many messages...
//
// Revision 1.44  2005/04/24 10:00:20  d3x0r
// Many fixes to finding things, etc.  Also fixed resuming on /wait command, previously fell back to the 5 second poll on sentients, instead of when data became available re-evaluating for a command.
//
// Revision 1.43  2005/04/20 06:20:25  d3x0r
//
// Revision 1.42  2005/03/02 19:13:59  d3x0r
// Add local behaviors to correct list.
//
// Revision 1.41  2005/02/26 23:26:33  d3x0r
// Show behaviors which have been defined on the /list command.  Show list of available behaviors on /on with no parameters
//
// Revision 1.40  2005/02/24 03:10:51  d3x0r
// Fix behaviors to work better... now need to register terminal close and a couple other behaviors...
//
// Revision 1.39  2005/02/23 11:38:59  d3x0r
// Modifications/improvements to get MSVC to build.
//
// Revision 1.38  2005/02/21 12:08:58  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.37  2005/01/17 09:01:32  d3x0r
// checkpoint ...
//
// Revision 1.36  2004/09/29 09:31:33  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.35  2004/09/27 16:06:57  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.34  2004/05/06 08:10:04  d3x0r
// Cleaned all warning from core code...
//
// Revision 1.33  2004/04/06 17:40:57  d3x0r
// Minor cleanup on unchecked NULL situations
//
// Revision 1.32  2004/03/07 17:50:49  d3x0r
// Fixed lost event when data passing through trigger during read invokes a macro
//
// Revision 1.31  2004/01/20 07:38:06  d3x0r
// Extended dumped flags
//
// Revision 1.30  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.29  2003/11/07 23:45:28  panther
// Port to new vartext abstraction
//
// Revision 1.28  2003/10/26 12:37:21  panther
// Rework event scheduling/handling based on newest Heavy Sleep Mechanism
//
// Revision 1.27  2003/08/04 07:17:03  panther
// Updates to cards engine
//
// Revision 1.26  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.25  2003/08/01 23:53:13  panther
// Updates for msvc build
//
// Revision 1.24  2003/08/01 02:36:18  panther
// Updates for watcom...
//
// Revision 1.23  2003/07/28 08:45:16  panther
// Cleanup exports, and use cproc type for threadding
//
// Revision 1.22  2003/04/12 20:51:19  panther
// Updates for new window handling module - macro usage easer...
//
// Revision 1.21  2003/03/25 08:59:03  panther
// Added CVS logging
//
