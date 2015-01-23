#include <stdhdrs.h>
//#define NO_LOGGING
#include <logging.h>
#include "sharemem.h"
#define PLUGIN_MODULE
#define DEFINES_DEKWARE_INTERFACE
#include "plugin.h"

//#define DEBUG_TRIGGER_MATCHING

int myTypeID; // supplied for uhmm... grins...
INDEX nTrigExten;


// format of trigger command
//	/trigger create name
//	/trigger list
//	/trigger <name> on ... 
//	/trigger <name> event ...
//	trigger apparnetly behaves like a macro
//	parameters can be referenced as %0 (the trigger itself)
//	  %-1 and below words before the trigger
//	  %1  and above words after the trigger
//	  if device input is 'formatted'
//		 trigger hooks into output queue of datapath...
// 
// 
// format of trigger command
//	/trigger create theft &who tried to steal
//	/send shout &0 is a bloody theif!
//	/send kill &0
//	/endmac
//
//	/trigger create autoflee
//	/trigger text autoflee < &hit &mana &move > 
//	/compare &0 under 80
//	/send flee
//	/endif
//	/endmac
//
//	/trigger opt killtrigger disable 
//	/trigger opt killtrigger enable
//
//	/trigger check %line
//	



//enum {
typedef int CPROC MyFunctionProto( PSENTIENT ps, PTEXT parameters, int bAlias );
typedef int (CPROC *MyFunction)(PSENTIENT ps,  PTEXT parameters, int bAlias );
static MyFunctionProto	HELP
	, CLEAR
	, TRIGGER_CREATE
	, DESCRIBE
	, LISTTRIGGERS
	, DESTROY
	, OPTIONS
	, TRIP
	, TRIGGER_STORE; // SAVE AS A SCRIPT FILE ALL TRIGGERS.

static _OptionHandler // options ... not really routines
	//ZERO_HOLDER // make normal result be NOT zero.
  //, 
   ANCHOR
  #define OPT_ANCHOR 1
  ,FREE  // is ~OPT_ANCHOR
  ,ONCE
  #define OPT_ONCE	2
  ,MULTI  // is ~OPT_ONCE
  ,EXACT
  #define OPT_EXACT  4
  ,SIMILAR // is ~OPT_EXACT
  ,CONSUME
  #define OPT_CONSUME 8
  ,PASS
  ,DISABLE
	#define OPT_DISABLE 0x100
  ,ENABLE
  ,ONCEOFF
  #define OPT_ONCEOFF 0x10 // counter is multi....
  ,RUN
  #define OPT_RUNLOCK 0x20 // trigger can run while moving data...
  ,LOCK
  ,HELPVAL
;


typedef struct trigger_tag {
	PTEXT pTemplate;
	PTEXT *pArgs; // static array of [n] for template storage....
					 // then we don't have to constantly alloc and free
					 // the duplicates....
	MACRO Actions; // name, arg template, and commands
	_32 used_count; // the number of times this trigger is in use in a queue
	int nOptions;
	struct trigger_tag *pNext;
	struct trigger_tag **me;
} TRIGGER, *PTRIGGER;

typedef struct triggerset_tag {
	DATAPATH common;

	// although triggers only work on DATA channel INPUT

	PTRIGGER pFirst;
	PSENTIENT pHost;

	// in case we invoke a trigger with an existing running....
	int bRunning;
	int bRunLock;
	int bAlias;  // which side (command/data) this trigger is on...
	PLINKQUEUE pTriggerList;
	PLINKQUEUE pTriggerArgs;
} TRIGGERSET, *PTRIGGERSET, *PMYDATAPATH;

#define TF_MULTIARG 0x80000000

static int DestroyTrigger( PSENTIENT ps, PTRIGGERSET pSet, PTRIGGER pTrigger, int bAlias );

//--------------------------------------------------------------------------

static PTRIGGERSET FindTriggerDatapath( PSENTIENT ps, int bAlias )
{
	if( bAlias )
	{
		//aliases are allowed to be 'hidden' since scripts
		// come and go and during the process may create an 
		// alias path - this is sooo not the right thing to do
		// but for now....
		return (PTRIGGERSET)(FindCommandDatapath( ps, myTypeID ));
	}
	else
	{
		return (PTRIGGERSET)(FindDataDatapath( ps, myTypeID ));
		/*
		if( ps->Data &&
			ps->Data->Type == myTypeID )
		{
			return (PTRIGGERSET)ps->Data;
			}
			*/
	}
	return NULL;
}

//------------------------------------------------------------------

static void EndTriggerAlias( PSENTIENT ps, PMACROSTATE pms, int bAlias )
{
	PTRIGGERSET pts;
	PTRIGGER pTrig;
	PTEXT pArgs;
	pts = FindTriggerDatapath( ps, bAlias );
	if( pts ) 
	{
		int bDeleted;
		pms->pMacro->flags.un.macro.bUsed = FALSE;
		if( pms->pMacro->flags.un.macro.bDelete )
		{
			pTrig = (PTRIGGER)pms->Data;
			if( pTrig )
				pTrig->used_count--;
			DestroyTrigger( ps, pts, pTrig, bAlias );
		}
		do
		{
			bDeleted = FALSE;
			pTrig = (PTRIGGER)DequeLink( &pts->pTriggerList );
			pArgs = (PTEXT)DequeLink( &pts->pTriggerArgs );
			if( pTrig )
				pTrig->used_count--;

			if( (int)pArgs == 1 )
				pArgs = NULL;

			if( pTrig && pTrig->Actions.flags.un.macro.bDelete )
			{
				//pTrig->Actions.flags.un.macro.bUsed = TRUE;
				DestroyTrigger( ps, pts, pTrig, bAlias );
				if( pArgs )
					LineRelease( pArgs );
				bDeleted = TRUE;
			}
		} while( bDeleted );

		if( pTrig ) 
		{
			if( pTrig->Actions.flags.un.macro.bRunLock )
				pts->bRunLock = TRUE;
			else
				pts->bRunLock = FALSE;
			pms->nCommand = 0;
			pms->pMacro = &pTrig->Actions;
			pms->pArgs = pArgs;
			{
				INDEX idx;
				PTEXT pVar;
				LIST_FORALL( pms->pVars, idx, PTEXT, pVar )
				{
					LineRelease( pVar ); // name, and indirect seg...
				}
			}
			DeleteList( &pms->pVars );
			pms->pVars = NULL;
			MemSet( &pms->state, 0, sizeof( pms->state ) );
			//pms->state.dwFlags = 0;
			pms->pInvokedBy = NULL;
			// leave this macro state on the stack, just swap out
			// the macro, the variables it has...
			// hmm the vars themselves need to be deleted...
		}
		else
		{
			pts->bRunning = FALSE;
			pts->bRunLock = FALSE;
			PopData( &ps->MacroStack ); // clear this from macro stack....
		}
  	}
	else // hmm not sure what to do in this case.....
	{
		Log( WIDE("Could not find trigger set for ending trigger?!") );
	}
}

static void EndTrigger( PSENTIENT ps, PMACROSTATE pms )
{
	EndTriggerAlias( ps, pms, FALSE );
}

static void EndAlias( PSENTIENT ps, PMACROSTATE pms )
{
	EndTriggerAlias( ps, pms, TRUE );
}
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

static int TestTriggerTemplate( PTEXT *ppArgs, int *nArg
							  , PTEXT pTemplate, PTEXT pLine
							  , int bExact
							  , int bAnchored )
{
	PTEXT pWord, pBegin;
	int nArgsStart = *nArg, bMultiArg, bDidMultiArg;
	PTEXT pTestWord;
	pBegin = pLine;
	while( pLine )
	{
		*nArg = nArgsStart;
	pWord = pLine;
#ifdef DEBUG_TRIGGER_MATCHING
		lprintf( WIDE("Line: %s word: %s"), GetText( pLine ), GetText( pWord ) );
#endif
		if( bAnchored )
			pLine = NULL;  // don't restart anywhere else on this line...
		else
			pLine = NEXTLINE( pLine );
		pTestWord = pTemplate; // restart template test...
		bDidMultiArg = FALSE;
		bMultiArg = FALSE;
		while( pWord && ( pTestWord || bMultiArg ) )
		{
			int data;
#ifdef DEBUG_TRIGGER_MATCHING
		lprintf( WIDE("word: %s testword %s"), GetText( pWord ), GetText( pTestWord ) );
#endif
			if( pTestWord && ( data = ( pTestWord->flags & IS_DATA_FLAGS ) ) )
			{
				if( ( pWord->flags & IS_DATA_FLAGS ) == data )
				{
					PTEXT pSubText;
					if( !( pWord->flags & TF_INDIRECT ) )
					{
						DECLTEXT(holder,WIDE("") );
						SegSubst( pWord, (PTEXT)&holder );
						pSubText = burst( pWord );
						SegSubst( (PTEXT)&holder, pWord );
						pWord->flags |= TF_DEEP | TF_INDIRECT;
						SetIndirect( pWord, pSubText );
					}
					if( TestTriggerTemplate( ppArgs, nArg
												  , GetIndirect( pTestWord )
												  , GetIndirect( pWord )
												  , bExact
												  , bAnchored ) )
					{
						pWord = NEXTLINE( pWord );
						pTestWord = NEXTLINE( pTestWord );
						continue;
					}
				}
				break;
			}
			if( pTestWord && !GetTextSize( pTestWord ) )
			{
				// store this word as a Var Value
				ppArgs[*nArg] = pWord;
				(*nArg)++;
				if( pTestWord->flags & TF_MULTIARG )
				{
#ifdef DEBUG_TRIGGER_MATCHING
					lprintf( WIDE("word: %s  added as multiarg"), GetText( pWord ) );
#endif
					pWord->flags |= TF_MULTIARG;
					bMultiArg = TRUE;
					bDidMultiArg = TRUE;
				}
				else
				{
					bMultiArg = FALSE;
#ifdef DEBUG_TRIGGER_MATCHING
					lprintf( WIDE("word: %s  added as single"), GetText( pWord ) );
#endif
				}
				pTestWord = NEXTLINE( pTestWord );
			}
			else
			{
				// compare testword to this word...
				// if match, contine, if fail, break...
				if( !GetText( pWord ) || !GetText( pTestWord ) )
				{
#ifdef DEBUG_TRIGGER_MATCHING
					lprintf( WIDE("word and testword are null..") );
#endif
					if( bMultiArg )
						pWord->flags |= TF_MULTIARG;
					pWord = NEXTLINE( pWord );
					continue;
				}
				if( bExact )
				{
					if( !strcmp( GetText( pWord ), GetText( pTestWord ) ) )
					{
#ifdef DEBUG_TRIGGER_MATCHING
						lprintf( WIDE("word and testword matched..") );
#endif

						pTestWord = NEXTLINE( pTestWord );
					}
					else
					{
						if( !bMultiArg )
							break;
						else
							pWord->flags |= TF_MULTIARG;
					}
				}
				else
				{
					if( !StrCaseCmp( GetText( pWord ), GetText( pTestWord ) ) )
					{
						pTestWord = NEXTLINE( pTestWord );
					}
					else
					{
						if( !bMultiArg )
							break;
						else
						{
							pWord->flags |= TF_MULTIARG;
#ifdef DEBUG_TRIGGER_MATCHING
							lprintf( WIDE("word: %s  added as multiarg"), GetText( pWord ) );
#endif
						}
					}
				}
			}
			pWord = NEXTLINE( pWord );
		}
		if( !pTestWord )
		{
#ifdef DEBUG_TRIGGER_MATCHING
			lprintf( WIDE("Matched!") );
#endif
			return TRUE;
		}
		// need to clear all TF_MULTIARG flags on line...
		if( bDidMultiArg )
		{
			PTEXT pSeg;
			pSeg = pBegin;
			while( pSeg )
			{
				pSeg->flags &= ~TF_MULTIARG;
				pSeg = NEXTLINE( pSeg );
			}
		}
	}

	return FALSE;
}

//--------------------------------------------------------------------------

static int TestTriggers( PTRIGGERSET pts, PTEXT pInput )
{
	PTEXT pDel = NULL, pLine
		 , pMacArgs = NULL
		 , pCheck;
	int bConsume = FALSE;
	PTRIGGER pTest;														 
	// store exact copy of input.
	pDel = NULL;
	for( pCheck = pInput; pCheck;  )
	{
		if( pCheck->flags & TF_BINARY ) 
		{
			if( !pDel )
			{
				pDel = TextDuplicate( pInput, FALSE );
				pCheck = pDel;	
				continue;
			}
			else
			{
				PTEXT pNext = NEXTLINE( pCheck );
				LineRelease( SegGrab( pCheck ) );
				if( pDel == pCheck )
					pDel = pNext;
				pCheck = pNext;
				continue;
			}
		}	
		pCheck = NEXTLINE( pCheck );
	}
	if( !pDel )
	{
		if( GetTextSize( pInput ) )
			pLine = burst( pInput );
		else
			pLine = SegDuplicate( pInput );
	}
	else
	{
		pLine = burst( pDel );
		LineRelease( pDel ); // stripped of any binary content.
	}	

  	pDel = pLine;

	pTest = pts->pFirst;
	while( pTest )
	{
		int nArg;
		if( !( pTest->nOptions & OPT_DISABLE ) ||
			  ( pTest->nOptions & OPT_ONCEOFF ) )
		{
			nArg = 0;
#ifdef DEBUG_TRIGGER_MATCHING
		lprintf( WIDE("test trigger %s"), GetText( pTest->Actions.pName ) );
#endif
			if( TestTriggerTemplate( pTest->pArgs
										  , &nArg
										  , NEXTLINE( pTest->pTemplate )
										  , pLine
										  , (pTest->nOptions & OPT_EXACT)
										  , (pTest->nOptions & OPT_ANCHOR) ) )
			{
				int n;
				// if disable at this point - must have been onceoff also
				if( pTest->nOptions & OPT_DISABLE )
				{
					pTest->nOptions &= ~(OPT_DISABLE|OPT_ONCEOFF);
				}
				else
				{
					// only set leading arg IF in face we matched.s
					pMacArgs = SegAppend( pMacArgs, SegCreateIndirect( TextDuplicate( pInput, FALSE ) ) ); 
					pMacArgs->flags |= TF_DEEP;
					for( n = 0; n < nArg; n++ )
					{
						PTEXT pArg, pNew;
						pArg = pTest->pArgs[n];
						if( pArg->flags & TF_MULTIARG )
						{
							PTEXT pInd;
							pMacArgs = SegAppend( pMacArgs, pInd = SegCreateIndirect( NULL ) );
							pInd->flags |= TF_DEEP;
							while( pArg && pArg->flags & TF_MULTIARG )
							{
								SetIndirect( pInd
											  , SegAppend( GetIndirect( pInd )
															 , SegDuplicate( pArg ) ) );
								pArg->flags &= ~TF_MULTIARG;
								pArg = NEXTLINE( pArg );
							}
							if( GetIndirect( pInd ) )
								GetIndirect( pInd )->format.position.offset.spaces = 0;
						}
						else
						{
							pMacArgs = SegAppend( pMacArgs, pNew = SegDuplicate( pArg ) );
							pNew->format.position.offset.spaces = 0;
						}
					}
					// okay to invoke the trigger here....
					if( pTest->nOptions & OPT_ONCE )
					{
						pTest->nOptions |= OPT_DISABLE;
						pTest->nOptions &= ~OPT_ONCE;
					}
					if( pTest->nOptions & OPT_CONSUME )
						bConsume = TRUE;

					pTest->Actions.flags.un.macro.bUsed = TRUE;
					if( pts->bRunning )
					{
						if( !pMacArgs )
							pMacArgs = (PTEXT)1;
						if( pTest->nOptions & OPT_RUNLOCK )
							pTest->Actions.flags.un.macro.bRunLock = TRUE;
						else
							pTest->Actions.flags.un.macro.bRunLock = FALSE;
						pTest->used_count++;
						EnqueLink( &pts->pTriggerList, pTest );
						EnqueLink( &pts->pTriggerArgs, pMacArgs );
						pMacArgs = NULL; // clear for next loop to test triggers...
					}
					else
					{
						// once upon a time I didn't have 'InvokeMacro'
						// but even if I did, I couldn't have used that cause triggers are not exactly macros.
						MACROSTATE MacState;
						MemSet( &MacState, 0, sizeof( MacState ) );
						MacState.nCommand = 0;
						MemSet( &MacState.state, 0, sizeof( MacState.state ) );
						//MacState.state.dwFlags = 0;
						MacState.pMacro = &pTest->Actions;
						MacState.pArgs = pMacArgs;
						MacState.pVars = NULL;
						MacState.pInvokedBy = NULL;
						MacState.peInvokedOn = NULL;
						if( pts->bAlias )
							MacState.MacroEnd = EndAlias;
						else
							MacState.MacroEnd = EndTrigger;
						MacState.Data = pTest;
						MacState.state.flags.data.levels = 0;
						MacState.state.flags.forced_run = 1;
						PushData( &pts->pHost->MacroStack, &MacState );
						//pts->pHost->CurrentMacro = PeekDataEx( &pts->pHost->MacroStack, 2 );
						if( pTest->nOptions & OPT_RUNLOCK )
							pts->bRunLock = TRUE;  // should be false default, and after run complete
						pts->bRunning = TRUE;
						pMacArgs = NULL;
					}
					if( bConsume )
						break;
					// continue and match all possibles....
					// perhaps - if alias, recheck this one also...
					// may do a substitute kinda thing...
					//break; // get out of NEXT TRIGGER loop...
				}
			}
			// shouldn't this always be cleared??
			// partial matches that fail, and once-off matched
			// things need this to clear...
			// if it fully matched, will all be clear and wil
			// be a simple loop here anyhow...
			//else
			{
				int n;
				PTEXT pArg;
				for( n = 0; n < nArg; n++ )
				{
					pArg = NEXTLINE( pTest->pArgs[n] );
					while( pArg && pArg->flags & TF_MULTIARG )
					{
						pArg->flags &= ~TF_MULTIARG;
						pArg = NEXTLINE( pArg );
					}
				}
			}
		}
		pTest = pTest->pNext;
	}

	if( pDel )
		LineRelease( pDel );
	return bConsume;
}

//--------------------------------------------------------------------------

static PTEXT CPROC HandleRead( PDATAPATH pdp, PTEXT pLine )
{
	//PTRIGGERSET pts = (PTRIGGERSET)pdp;
	//lprintf( WIDE("Triggers received:%s (%p)%s"), GetText(pdp->pName), pLine, GetText( pLine ) );
	if( TestTriggers( (PTRIGGERSET)pdp, pLine ) )
	{
		LineRelease( pLine );
		pLine = (PTEXT)1;
	}
	//lprintf( WIDE("Resulting with:%s %p"), GetText(pdp->pName), pLine );
	return pLine;
}

//--------------------------------------------------------------------------
static int CPROC Read( PDATAPATH pdp )
{
	return RelayInput( pdp, HandleRead );
}

//--------------------------------------------------------------------------

static int CPROC Write( PDATAPATH pdp )
{
	return RelayOutput( pdp, NULL );
}

//--------------------------------------------------------------------------

static int DestroyTrigger( PSENTIENT ps
						 , PTRIGGERSET pSet
						 , PTRIGGER pTrigger
						 , int bAlias )
{
	// in this condition though...
	// what really is the harm in destroying a macro which is used
	// who really knows about it?

	// /trigger destroy thing - while thing is running commands
	// but we are not processing a command of thing cause it's the delete
	// or a close... /close trigger for instance...
	//  it will be the current sentience running this command
	// and since the datapaths are attached to a sentience,
	// I suppose this means that it has to be an aware object, and
	// is therefore told commands and enqueued... forcing this sentience to rpocess
	// it... so - some current macro state... and maybe some other macro that this
	// has called...
	if( pTrigger->Actions.flags.un.macro.bUsed || pTrigger->used_count )
	{
		/*
		PMACROSTATE pms;
		pms = PeekStack( &ps->MacroStack );
		if( pms->pMacro == &pTrigger->Actions )
		{
			// yeah... we can lie here...
			// cmd_return is the only guy that can free this,
			// and without currentmacro set, it aborts cause
			// /return is only valid in a command
			ps->CurrentMacro = pms;
			CMD_RETURN( ps, parameters );
			// ps->CurrentMacro is reset to NULL.
			// it's the top macro, we need to do something, and we can.
			// remove this macro state... hmm waht about vars and stuff? oh.
			PopData( &ps->MacroStack );
		}
		else
		*/
		pTrigger->Actions.flags.un.macro.bDelete = TRUE;
		return FALSE;
	}
	if( !pSet )
	{
		DECLTEXT( msg, WIDE("No triggers defined...") );
		EnqueLink( &ps->Command->Output, &msg );
		return TRUE;
	}
	if( (*pTrigger->me) = pTrigger->pNext )
		pTrigger->pNext->me = pTrigger->me;

	// delete all parts of the trigger also!!!!!!
	{
		INDEX idx;
		PTEXT temp;
		LineRelease( pTrigger->Actions.pName );
		LineRelease( pTrigger->Actions.pDescription );
		LineRelease( pTrigger->Actions.pArgs );
		LineRelease( pTrigger->pTemplate );
		Release( pTrigger->pArgs );
		LIST_FORALL( pTrigger->Actions.pCommands, idx, PTEXT, temp )
		{
			LineRelease( temp );
		}
		DeleteList( &pTrigger->Actions.pCommands );
	}
	Release( pTrigger );
	return TRUE;
}

//--------------------------------------------------------------------------

static int CPROC Close( PDATAPATH pdpX )
{
	PMYDATAPATH pdp = (PMYDATAPATH)pdpX;
	// check if bRunning
	// so we can STOP the macro....
	PTRIGGER pTrig;
	PTEXT pTemp;
	while( ( pTrig = pdp->pFirst ) )
	{
		if( !DestroyTrigger( pdp->pHost, pdp, pTrig, FALSE ) )
		{
			// if we return one, we may never get called again...
			return 1;
		}
//		DestroyTrigger( pdp->/chpHost, pTrig, bAlias );
	}

	while( DequeLink( &pdp->pTriggerList ) );
	while( ( pTemp = (PTEXT)DequeLink( &pdp->pTriggerArgs ) ) )
		LineRelease( pTemp );
	DeleteLinkQueue( &pdp->pTriggerList );
	DeleteLinkQueue( &pdp->pTriggerArgs );
	// destroy all triggers....
	// guess I need to release pdp->common.pPrior too... not sure though...
	// no... actually we'll allow ourselves to close, and not pretend to be
	// transparent to closes... that was just rude anyhow and caused
	// questionable closes...
	// return no error;
	return 0;
}

static int CPROC ANCHOR( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	return 0;
}

static PTRIGGER OptionCommon( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters );

static int CPROC FREE( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions &= ~OPT_ANCHOR;
	return 0;
}

static int CPROC ONCE( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
	{
		pThis->nOptions |= OPT_ONCE;
		pThis->nOptions &= ~(OPT_DISABLE|OPT_ONCEOFF);
	}
	return 0;
}

static int CPROC MULTI( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions &= ~(OPT_ONCE|OPT_ONCEOFF|OPT_DISABLE);

	return 0;
}

static int CPROC EXACT( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions |= OPT_EXACT;
	return 0;
}

static int CPROC SIMILAR( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions &= ~OPT_EXACT;
	return 0;
}

static int CPROC CONSUME( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions |= OPT_CONSUME;
	return 0;
}

static int CPROC PASS( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions &= ~OPT_CONSUME;
	return 0;
}

static int CPROC DISABLE( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions |= OPT_DISABLE;
	return 0;
}

static int CPROC ENABLE( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions &= ~OPT_DISABLE;
	return 0;
}

static int CPROC ONCEOFF( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
	{
		pThis->nOptions &= ~OPT_ONCE;
		pThis->nOptions |= OPT_ONCEOFF | OPT_DISABLE;
	}
	return 0;
}

static int CPROC RUN( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions &= ~OPT_RUNLOCK;
	return 0;
}

static int CPROC LOCK( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	PTRIGGER pThis = OptionCommon( pdp, ps, text );
	if( pThis )
		pThis->nOptions |= OPT_RUNLOCK;
	return 0;
}

//--------------------------------------------------------------------------

command_entry commands[]={
  {DEFTEXT(WIDE("Help")),DEFTEXT(WIDE("TRIGGER/Commands")),0,4  ,DEFTEXT(WIDE("Show this command list...")),(Function)HELP}

 ,{DEFTEXT(WIDE("Clear")),DEFTEXT(WIDE("TRIGGER/Commands")),0,5  ,DEFTEXT(WIDE("Erase all triggers")),(Function)CLEAR}
 ,{DEFTEXT(WIDE("Create")),DEFTEXT(WIDE("TRIGGER/Commands")),0,6  ,DEFTEXT(WIDE("Create a new trigger")),(Function)TRIGGER_CREATE}
// ,{DEFTEXT(WIDE("Describe")),0,8  ,DEFTEXT(WIDE("Describe a trigger for LIST")),DESCRIBE}
 ,{DEFTEXT(WIDE("List")),DEFTEXT(WIDE("TRIGGER/Commands")),0,4  ,DEFTEXT(WIDE("List present triggers... or a trigger")),(Function)LISTTRIGGERS}
 ,{DEFTEXT(WIDE("Destroy")),DEFTEXT(WIDE("TRIGGER/Commands")),0,7  ,DEFTEXT(WIDE("Destroy a trigger")),(Function)DESTROY}
 ,{DEFTEXT(WIDE("Options")),DEFTEXT(WIDE("TRIGGER/Commands")),0,7  ,DEFTEXT(WIDE("Set options for a trigger")),(Function)OPTIONS}
// ,{DEFTEXT(WIDE("Trip"))	,DEFTEXT(WIDE("TRIGGER/Commands")),0,4  ,DEFTEXT(WIDE("Invoke a trigger now")),TRIP}
 ,{DEFTEXT(WIDE("Store"))  ,DEFTEXT(WIDE("TRIGGER/Commands")),0,5  ,DEFTEXT(WIDE("Store triggers, and variables associated.")), (Function)TRIGGER_STORE }
};

#define NUM_COMMANDS (sizeof(commands)/sizeof(command_entry))
int nCommands = NUM_COMMANDS;



option_entry options[]={
  {DEFTEXT(WIDE("ANCHOR")) ,0,6  ,DEFTEXT(WIDE("Trigger must start first on line")),ANCHOR}
 ,{DEFTEXT(WIDE("FREE"))	,0,4  ,DEFTEXT(WIDE("Trigger may occur at any time")),FREE}
 ,{DEFTEXT(WIDE("ONCE"))	,0,4  ,DEFTEXT(WIDE("Once trigger occurs, it auto disables")),ONCE}
 ,{DEFTEXT(WIDE("ONCEOFF")),0,7  ,DEFTEXT(WIDE("Disable a trigger once, it auto enables")),ONCEOFF}
 ,{DEFTEXT(WIDE("MULTI"))  ,0,5  ,DEFTEXT(WIDE("Enables and clears ONCE option")),MULTI}
 ,{DEFTEXT(WIDE("CONSUME")),0,7  ,DEFTEXT(WIDE("If input line trips trigger, delete data")),CONSUME}
 ,{DEFTEXT(WIDE("PASS"))	,0,4  ,DEFTEXT(WIDE("Trigger does not delete input data")),PASS}
 ,{DEFTEXT(WIDE("ENABLE")) ,0,6  ,DEFTEXT(WIDE("Enable a trigger")),ENABLE}
 ,{DEFTEXT(WIDE("DISABLE")),0,7  ,DEFTEXT(WIDE("Disable a trigger")),DISABLE}
 ,{DEFTEXT(WIDE("EXACT"))  ,0,5  ,DEFTEXT(WIDE("Compare template case sensitive")),EXACT}
 ,{DEFTEXT(WIDE("SIMILAR")),0,7  ,DEFTEXT(WIDE("Compare template without case")),SIMILAR}
 ,{DEFTEXT(WIDE("RUN"))	 ,0,3  ,DEFTEXT(WIDE("Allow trigger to run and relay data")),RUN}
 ,{DEFTEXT(WIDE("LOCK"))	,0,4  ,DEFTEXT(WIDE("Stop relaying data while trigger runs")),LOCK}
 ,{DEFTEXT(WIDE("HELP"))	,0,4  ,DEFTEXT(WIDE("Show this list")),HELPVAL}
};

#define NUM_OPTIONS (sizeof(options)/sizeof(command_entry))
int nOptions = NUM_OPTIONS;

//--------------------------------------------------------------------------

static int CPROC HELPVAL( PDATAPATH pdp, PSENTIENT ps, PTEXT text )
{
	//PTRIGGER pThis = OptionCommon( pdp, ps, text );
	//if( pThis )
	{
		{
			DECLTEXT( leader, WIDE(" --- Trigger Options ---") );
			EnqueLink( &ps->Command->Output, &leader );
			WriteOptionList( &ps->Command->Output, options, nOptions, NULL );
		}
	}
	return 0;
}
//--------------------------------------------------------------------------

static PTRIGGER FindTrigger( PSENTIENT ps, PTRIGGERSET pSet, PTEXT pName, int bAlias )
{
	PTRIGGER pTrigger;

	if( pSet )
		pTrigger = pSet->pFirst;
	else
		pTrigger = NULL;
 	while( pTrigger )
	{
		if( SameText( pTrigger->Actions.pName, pName ) == 0 )
		{
			break;
		}
		pTrigger = pTrigger->pNext;
	}
	return pTrigger;
}

//--------------------------------------------------------------------------
static PTRIGGER OptionCommon( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
	PTEXT pTrigger;
	PTRIGGER pThis = NULL;
	PTRIGGERSET pSet = (PTRIGGERSET)pdp;
	pTrigger = GetParam( ps, &parameters );
	if( !pTrigger )
	{
		DECLTEXT( msg, WIDE("Must specify trigger to set options for...") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}

	return FindTrigger( ps, pSet, pTrigger, pSet->bAlias );
}

//--------------------------------------------------------------------------

static PTEXT ProcessTemplate( PSENTIENT ps, PTEXT pArgs, PTEXT *ppArgs, int *pnArgs )
{
	PTEXT pTmp, pName;
	PTEXT pTemplate = NULL;
	DECLTEXT( var, WIDE("TRUELINE") );
	int bNextArg = FALSE, bMultiArg;

	// now there is a variable for triggers which is THE line
	// which was input....
	pName = SegDuplicate( (PTEXT)&var );
	*ppArgs = SegAppend(*ppArgs, pName );
	(*pnArgs)++;
	pTemplate = SegAppend( pTemplate, SegCreate(0) );

	while( ( pTmp = GetParam( ps, &pArgs ) ) )
	{
		if( pTmp->flags & IS_DATA_FLAGS )
		{
			PTEXT pData, pInd;
			DECLTEXT(holder,WIDE("") );
			SegSubst( pTmp, (PTEXT)&holder );
			pData = burst( pTmp );
			SegSubst( (PTEXT)&holder, pTmp );
			pTemplate = SegAppend( pTemplate
										, pInd = SegCreateIndirect( ProcessTemplate( ps, pData, ppArgs, pnArgs ) ) );
			pInd->flags |= pTmp->flags & IS_DATA_FLAGS;
			pInd->flags |= TF_DEEP; // release indirect on this release...
			continue;
		}

		if( GetText(pTmp)[0] == '&' )
		{
		//multiple &&'s will dissappear....
			bNextArg = TRUE;
			bMultiArg = FALSE;
		}
		else if( GetText( pTmp )[0] == '*' )
		{
			bNextArg = TRUE;
			bMultiArg = TRUE;
		}
		else
		{
			if( bNextArg )
			{
				PTEXT pName, pSeg;
				if( !GetTextSize( pTmp ) )
					continue;
				(*pnArgs)++;
				bNextArg = FALSE;
				pName = SegDuplicate( pTmp );
				*ppArgs = SegAppend(*ppArgs, pName );
				pTemplate = SegAppend( pTemplate, pSeg = SegCreate(0) );
				if( bMultiArg )
				{
					pSeg->flags |= TF_MULTIARG;
					pName->flags |= TF_MULTIARG;
				}
			}
			else
			{
				PTEXT pText;
				if( !GetTextSize( pTmp ) )
					continue;
				pText = SegDuplicate( pTmp );
				pTemplate = SegAppend( pTemplate, pText );
			}
		}
	}
	return pTemplate;
}

//--------------------------------------------------------------------------

static PDATAPATH CPROC OpenTriggerSet( PDATAPATH *pChannel, PSENTIENT ps, PTEXT params )
{
	DECLTEXT( name1, WIDE("$Triggers") );
	DECLTEXT( name2, WIDE("$Aliae") );
	PTRIGGERSET pSet;
	int bAlias;
	if( pChannel == &ps->Command )
  	{
		bAlias = TRUE;
		pSet = (PTRIGGERSET)CreateDataPath( &ps->Command, TRIGGERSET );
		pSet->common.pName = (PTEXT)&name2;
	}
	else
	{
		bAlias = FALSE;
		pSet = (PTRIGGERSET)CreateDataPath( &ps->Data, TRIGGERSET );
		pSet->common.pName = (PTEXT)&name1;
	}
	SetDatapathType( &pSet->common, myTypeID );

	pSet->common.Read  = Read;
	pSet->common.Write = Write;
	pSet->common.Close = Close;
		
	pSet->pHost = ps;
	pSet->pTriggerList = CreateLinkQueue();
	pSet->pTriggerArgs = CreateLinkQueue();
	pSet->bAlias = bAlias;
	return (PDATAPATH)pSet;
}

//--------------------------------------------------------------------------

static PTRIGGER CreateTrigger( PSENTIENT ps, PTEXT pName, PTEXT args, int bAlias )
{
	PTRIGGER pTrigger;
	PTRIGGERSET pSet;
	int nArgs;
	pSet = FindTriggerDatapath( ps, bAlias );
	if( bAlias )
	{
		// cannot log message if no input...
		//if( !ps->Command )
		//{
		//	DECLTEXT( msg, WIDE("Data path is not currently open... cannot add triggers.") );
		//	EnqueLink( &ps->Command->Output, &msg );
		//	return NULL;
		//}
	}
	else
	{
		if( !ps->Data )
		{
			DECLTEXT( msg, WIDE("Data path is not currently open... cannot add triggers.") );
			EnqueLink( &ps->Command->Output, &msg );
			return NULL;
		}
	}
	//Log( WIDE("Creating a trigger...") );
	if( ( pTrigger = FindTrigger( ps, pSet, pName, bAlias ) ) )
	{
		DestroyTrigger( ps, pSet, pTrigger, bAlias );
	}
	pTrigger = New( TRIGGER );
	MemSet( pTrigger, 0, sizeof( TRIGGER ) );
	pTrigger->Actions.pName = SegDuplicate( pName );
	pTrigger->Actions.flags.bMacro = TRUE;
	pTrigger->nOptions = OPT_RUNLOCK;
	if( bAlias )
		pTrigger->nOptions |= OPT_CONSUME|OPT_ANCHOR;
	ps->pRecord = &pTrigger->Actions;
	nArgs = 0;
	//Log( WIDE("Processing Template") );
	pTrigger->pTemplate = ProcessTemplate( ps, args
													 , &pTrigger->Actions.pArgs
													 , &nArgs );
	//Log( WIDE("Processed Template") );
	pTrigger->pArgs = NewArray( PTEXT, nArgs );
	if( !pTrigger->pTemplate )
	{
		DECLTEXT( msg, WIDE("Although allowed, it is unadvisable to define a trigger with no template") );
		EnqueLink( &ps->Command->Output, &msg );
	}
	// this works except if we have a chain of things open...
	// then well we'll have to figure out what to do...

	// Duh - this is done when we first enter...
	//Log( WIDE("finding my datapath....") );
	//if( bAlias )
	//	pSet = (PTRIGGERSET)FindCommandDatapath( ps, myTypeID );
	//else
	//	pSet = (PTRIGGERSET)FindDataDatapath( ps, myTypeID );
	
	if( !pSet ) // first trigger... update data path methods
	{
		if( bAlias )
		{
			//Log( WIDE("Could not find datapath - making a new command") );
			pSet = (PTRIGGERSET)OpenTriggerSet( &ps->Command, ps, NULL );
		}
		else
		{
			//Log( WIDE("Could not find datapath - making a new data") );
			pSet = (PTRIGGERSET)OpenTriggerSet( &ps->Data, ps, NULL );
		}
	}
	//Log( WIDE("Queuing Trigger...") );
  	pTrigger->pNext = pSet->pFirst;
	if( pSet->pFirst )
	  	pSet->pFirst->me = &pTrigger->pNext;
	pTrigger->me = &pSet->pFirst;
	pSet->pFirst = pTrigger;

	return pTrigger;
}

//--------------------------------------------------------------------------

static void StoreTriggers( FILE *pFile, PSENTIENT ps, int bAlias )
{
	PTRIGGER pTrigger;
	PTRIGGERSET pSet;
	pSet = FindTriggerDatapath( ps, bAlias );

	if( pSet )
		pTrigger = pSet->pFirst;
	else
		pTrigger = NULL;
	{
		INDEX idx;
		PLIST pVars;
		PTEXT pVar, pVal;
		pVars = ps->Current->pVars;
		fprintf( pFile, WIDE("\n## Begin Variables\n") );
		LIST_FORALL( pVars, idx, PTEXT, pVar )
		{
			fprintf( pFile, WIDE("/Declare %s "), GetText( pVar ) );
			pVal = BuildLine( GetIndirect( NEXTLINE( pVar ) ) );
			if( pVal )
				fprintf( pFile, WIDE("%s\n"), GetText( pVal ) );
			else
				fprintf( pFile, WIDE("\n") );
		}
		fprintf( pFile, WIDE("\n## Begin Triggers\n") );
	}
 	while( pTrigger )
	{
		PTEXT pParam, pCmd, pVar;
		INDEX idx;
		fprintf( pFile, WIDE("/trigger dest %s\n"), GetText( pTrigger->Actions.pName ) );
		fprintf( pFile, WIDE("/trigger create %s "), GetText( pTrigger->Actions.pName ) );
		idx = 0;
		pVar = pTrigger->Actions.pArgs;
		pParam = pTrigger->pTemplate;
		while( pParam )
		{
			if( !GetTextSize( pParam ) )
			{
				fprintf( pFile, WIDE("&%s "), GetText( pVar ) );
				pVar = NEXTLINE( pVar );
			}
			else
				fprintf( pFile, WIDE("%s "), GetText(pParam) );
			pParam = NEXTLINE( pParam );
		}
		fprintf( pFile, WIDE("\n") );
		LIST_FORALL( pTrigger->Actions.pCommands, idx, PTEXT, pCmd )
		{
			PTEXT pOut; // oh - maintain some sort of order
			pOut = BuildLine( pCmd );
			fprintf( pFile, WIDE("%s\n"), GetText( pOut ) );
			LineRelease( pOut );
		}
		fprintf( pFile, WIDE("\n") );
		pTrigger = pTrigger->pNext;
	}
}

//--------------------------------------------------------------------------

static DECLTEXTSZ(outmsg,4096);
#undef nOutput
#undef byOutput
#define nOutput outmsg.data.size
#define byOutput outmsg.data.data

//--------------------------------------------------------------------------


static int CLEAR( PSENTIENT ps, PTEXT parameters, int bAlias )
{
	PTRIGGERSET pdp;
	pdp = FindTriggerDatapath( ps, bAlias );

	if( !pdp )
	{
		DECLTEXT( msg, WIDE("Did not find trigger datapath... ") );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	// unlink myself from device chain...
	*pdp->common.ppMe = pdp->common.pPrior;
  	pdp->common.pPrior->ppMe = pdp->common.ppMe;
	// clear my links so nothing else is touched....
	pdp->common.pPrior = NULL;
	pdp->common.ppMe = NULL;
	DestroyDataPath( (PDATAPATH)pdp ); // calls my close method...

	return 0;
}

//--------------------------------------------------------------------------

static int TRIGGER_CREATE(PSENTIENT ps, PTEXT parameters, int bAlias )
{
	PTEXT pName, temp;
	pName = GetParam( ps, &parameters );
	if( !pName )
	{
		DECLTEXT( msg, WIDE("Have to specify the name of trigger to create.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	// the input to this may not be 'burst' which is 
	// the format of parsing which is done in this module...
	CreateTrigger( ps, pName, temp = burst( parameters ), bAlias );
	LineRelease( temp ); 
	return FALSE;
}

//--------------------------------------------------------------------------

static int DESTROY(PSENTIENT ps, PTEXT parameters, int bAlias )
{
	PTEXT pName;
	PTRIGGERSET pSet;
	PTRIGGER pTrigger;
	pName = GetParam( ps, &parameters );
	if( !pName )
	{
		DECLTEXT( msg, WIDE("Have to specify the name of trigger to destroy.") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	pSet = FindTriggerDatapath( ps, bAlias );
	if( ( pTrigger = FindTrigger( ps, pSet, pName, bAlias ) ) )
	{
		DestroyTrigger( ps, pSet, pTrigger, bAlias );
	}
	return FALSE;
}

//--------------------------------------------------------------------------

static int LISTTRIGGERS(PSENTIENT ps, PTEXT parameters, int bAlias )
{
	PTEXT pTriggerName;
	PLINKQUEUE *ppOutput;
	PTRIGGERSET pSet;
	PTRIGGER pTrig;
	pSet = FindTriggerDatapath( ps, bAlias );

	if( !pSet )
	{
		DECLTEXT( msg, WIDE("Data path not open, or no triggers defined") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	pTriggerName = GetParam( ps, &parameters );
	pTrig = pSet->pFirst;
	ppOutput = &ps->Command->Output;
	if( pTriggerName )
	{
		LOGICAL bFound = FALSE;
		PMACRO pm;
		nOutput = 0;
		while( pTrig )
		{
			if( SameText( pTriggerName, pTrig->Actions.pName ) == 0 )
			{
				PTEXT pt;
				INDEX idx;
				bFound = TRUE;
				pm = &pTrig->Actions;
				
				nOutput += snprintf(byOutput+nOutput,sizeof( byOutput ), WIDE("Trigger \'%s\'"), GetText( pm->pName ) );
				if( pm->pArgs )
				{
					PTEXT param;
					param = pm->pArgs;
					nOutput += snprintf(byOutput+nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR),  WIDE("(") );
					while( param )
					{
						if( PRIORLINE( param ) )
							nOutput += snprintf( byOutput+nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE(", ") );
						nOutput += snprintf( byOutput+nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE("%s"), GetText( param ) );
						param = NEXTLINE( param );
					}
					nOutput += snprintf( byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE(")") );
					EnqueLink( ppOutput, SegDuplicate((PTEXT)&outmsg) );
				}
				else
					EnqueLink( ppOutput, SegDuplicate((PTEXT)&outmsg) );
				nOutput = snprintf( byOutput, sizeof( byOutput ), WIDE("Template: ") );
				{
					PTEXT pTemplate;
					PTEXT pArg;
					pArg = pTrig->Actions.pArgs;
					pTemplate = pTrig->pTemplate;
					while( pTemplate )
					{
						if( !GetTextSize( pTemplate ) )
						{
							if( pArg->flags & TF_MULTIARG )
								nOutput += snprintf(byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE(" [%s]"), GetText( pArg ) );
							else
								nOutput += snprintf(byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE(" (%s)"), GetText( pArg ) );
							pArg = NEXTLINE( pArg );

						}
						else
						{
							nOutput += snprintf(byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE(" %s"), GetText( pTemplate ) );
						}
						pTemplate = NEXTLINE( pTemplate );
					}
					EnqueLink( ppOutput, SegDuplicate( (PTEXT)&outmsg ) );
				}
				nOutput = snprintf( byOutput,sizeof( byOutput ), WIDE("Options: %s %s %s %s %s %s"),
						 ( pTrig->nOptions & OPT_DISABLE )?WIDE("DISABLED"):WIDE("enabled")
						,( pTrig->nOptions & OPT_ONCE )?WIDE("ONCE")
									:(pTrig->nOptions & OPT_ONCEOFF )? WIDE("ONCEOFF"):WIDE("multi")
						,( pTrig->nOptions & OPT_ANCHOR )?WIDE("ANCHOR"):WIDE("free")
						,( pTrig->nOptions & OPT_EXACT )?WIDE("EXACT"):WIDE("similar")
						,( pTrig->nOptions & OPT_CONSUME )?WIDE("CONSUME"):WIDE("passdata") 
						,( pTrig->nOptions & OPT_RUNLOCK )?WIDE("lock"):WIDE("RUN")
						);
				EnqueLink( ppOutput, SegDuplicate( (PTEXT)&outmsg ) );

				LIST_FORALL( pm->pCommands, idx, PTEXT, pt )
				{
					PTEXT x;
					x = BuildLine( pt );
					nOutput = snprintf( byOutput,sizeof( byOutput ), WIDE("	 %s") ,
												GetText( x ) );
					LineRelease( x );
					EnqueLink( ppOutput, SegDuplicate((PTEXT)&outmsg) );
				}
				break;
			}
			pTrig = pTrig->pNext;
		}
		if (!bFound)
		{
			nOutput = snprintf(byOutput,sizeof( byOutput ),WIDE("Trigger %s is not defined."),GetText(pTriggerName));
			EnqueLink( &ps->Command->Output, SegDuplicate( (PTEXT)&outmsg ) );
		}
	}
	else
	{
		nOutput = snprintf( byOutput,sizeof( byOutput ), WIDE("Defined Triggers: ") );
		{
			LOGICAL didone = FALSE;
			while( pTrig )
			{
				nOutput += snprintf( byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE("%s%s"), (didone)?WIDE(", "): WIDE(""),
											GetText( pTrig->Actions.pName ) );
				didone = TRUE;
				pTrig = pTrig->pNext;
			}
			if( didone )
				nOutput += snprintf( byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE(".") );
			else
				nOutput += snprintf( byOutput + nOutput,sizeof( byOutput )-nOutput*sizeof(TEXTCHAR), WIDE("None.") );
		}
		EnqueLink( &ps->Command->Output, SegDuplicate( (PTEXT)&outmsg ) );
	}
	return FALSE;
}

//--------------------------------------------------------------------------

static int HELP(PSENTIENT ps, PTEXT parameters, int bAlias )
{
	if( bAlias )
	{
		DECLTEXT( leader, WIDE(" --- Alias Builtin Commands ---") );
			EnqueLink( &ps->Command->Output, &leader );
	}
	else
	{
		DECLTEXT( leader, WIDE(" --- Trigger Builtin Commands ---") );
			EnqueLink( &ps->Command->Output, &leader );
	}
	WriteCommandList2( &ps->Command->Output, WIDE("dekware/commands/TRIGGER/Commands"), NULL );
	return FALSE;
}

//--------------------------------------------------------------------------

static int TRIGGER_STORE(PSENTIENT ps, PTEXT parameters, int bAlias )
{ // filename...
	FILE *file;
	PTEXT pName = GetFileName( ps, &parameters );
	if( pName )
	{
		file = sack_fopen( 0, GetText( pName ), WIDE("wt") );
		if( file )
		{
			StoreTriggers( file, ps, bAlias );
			fclose( file );
			if( ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
		else if( !ps->CurrentMacro )
		{
			DECLTEXT( msg, WIDE("Could not open that file.") );
			EnqueLink( &ps->Command->Output, &msg );
		}
		LineRelease( pName );
	}
	return FALSE;
}

//--------------------------------------------------------------------------

static int OPTIONS(PSENTIENT ps, PTEXT parameters, int bAlias )
{
	PTEXT pTrigger, pOpt = NULL;
	PTRIGGER pThis = NULL;
	PTRIGGERSET pSet;
	int bSet = FALSE;
	pTrigger = GetParam( ps, &parameters );
	if( !pTrigger )
	{
		DECLTEXT( msg, WIDE("Must specify trigger to set options for...") );
		EnqueLink( &ps->Command->Output, &msg );
		return FALSE;
	}
	if( TextLike( pTrigger, WIDE("help") ) )
	{
		DECLTEXT( leader, WIDE(" --- Trigger Options ---") );
		EnqueLink( &ps->Command->Output, &leader );
		WriteOptionList( &ps->Command->Output, options, nOptions, NULL );
		return FALSE;
	}
	pSet = FindTriggerDatapath( ps, bAlias );

	pThis = FindTrigger( ps, pSet, pTrigger, bAlias );
	if( !pThis )
	{
		DECLTEXTSZ( msg, 256 );
		msg.data.size = snprintf( msg.data.data, msg.data.size, WIDE("Could not find trigger %s."), GetText( pTrigger ) );
		EnqueLink( &ps->Command->Output, SegDuplicate( (PTEXT)&msg ) );
			return FALSE;
	}

	while( pThis && ( pOpt = GetParam( ps, &parameters ) ) )
	{
		OptionHandler opt;
		bSet = TRUE;
		lprintf( "Trigger options are not tested." );
		opt = GetOptionRegistered( WIDE("trigger"), pOpt );
		if( opt )
			opt( (PDATAPATH)pSet, ps, pTrigger );
		else
		{
			DECLTEXT( msg, WIDE("Unkown option... try /trigger option help") );
			EnqueLink( &ps->Command->Output, &msg );
			return FALSE;
		}
#if 0
		else switch( (int)opt/*options[opt].function*/ )
		{
		case ONCE:
			pThis->nOptions |= OPT_ONCE;
			pThis->nOptions &= ~(OPT_DISABLE|OPT_ONCEOFF);
			break;
		case ONCEOFF:
			pThis->nOptions &= ~OPT_ONCE;
			pThis->nOptions |= OPT_ONCEOFF | OPT_DISABLE;
			break;
		case MULTI:
			pThis->nOptions &= ~(OPT_ONCE|OPT_ONCEOFF|OPT_DISABLE);
			break;
		case ANCHOR:
			pThis->nOptions |= OPT_ANCHOR;
			break;
		case FREE:
			pThis->nOptions &= ~OPT_ANCHOR;
			break;
		case EXACT:
			pThis->nOptions |= OPT_EXACT;
			break;
		case SIMILAR:
			pThis->nOptions &= ~OPT_EXACT;
			break;
		case CONSUME:
			pThis->nOptions |= OPT_CONSUME;
			break;
		case PASS:
			pThis->nOptions &= ~OPT_CONSUME;
			break;
		case DISABLE:
			pThis->nOptions |= OPT_DISABLE;
			break;
		case ENABLE:
			pThis->nOptions &= ~OPT_DISABLE;
			break;
		case RUN:
			pThis->nOptions &= ~OPT_RUNLOCK;
			break;
		case LOCK:
			pThis->nOptions |= OPT_RUNLOCK;
			break;
		case HELPVAL:
			{
				DECLTEXT( leader, WIDE(" --- Trigger Options ---") );
				EnqueLink( &ps->Command->Output, &leader );
				WriteOptionList( &ps->Command->Output, options, nOptions, NULL );
			}
			break;
		}
#endif
	}
	if( pThis && !bSet )
	{
		nOutput = snprintf( byOutput, sizeof( byOutput ), WIDE("%s options: %s %s %s %s %s %s")
				,( GetText( pTrigger ) )
				,( pThis->nOptions & OPT_DISABLE )?WIDE("DISABLED"):WIDE("enabled")
				,( pThis->nOptions & OPT_ONCE )?WIDE("ONCE")
							:(pThis->nOptions & OPT_ONCEOFF )? WIDE("ONCEOFF"):WIDE("multi")
				,( pThis->nOptions & OPT_ANCHOR )?WIDE("ANCHOR"):WIDE("free")
				,( pThis->nOptions & OPT_EXACT )?WIDE("EXACT"):WIDE("similar")
				,( pThis->nOptions & OPT_CONSUME )?WIDE("CONSUME"):WIDE("passdata") 
				,( pThis->nOptions & OPT_RUNLOCK )?WIDE("lock"):WIDE("RUN")
				);
		EnqueLink( &ps->Command->Output, SegDuplicate( (PTEXT)&outmsg ) );
	}
	return FALSE;
}

//--------------------------------------------------------------------------

static int HandleCommand(WIDE("IO"), WIDE("TRIGGER"), WIDE("Trigger macro commands - handle data filter on input") )( PSENTIENT ps, PTEXT parameters )
//static int CPROC Trigger( PSENTIENT ps, PTEXT parameters )
{
	PTEXT op;
	op = GetParam( ps, &parameters );
	if( op )
	{
		//int idx;
		MyFunction f = (MyFunction)(GetRoutineRegistered( WIDE("TRIGGER/Commands"), op ));
		//idx = GetCommandIndex( commands, NUM_COMMANDS
		//							, GetTextSize(op), GetText(op) );
		if( !f )
		{
			DECLTEXT( msg, WIDE("Trigger Operation unknown....check /trigger HELP...") );
			EnqueLink( &ps->Command->Output, &msg );
			return 0;
		}
		((MyFunction)f)( ps, parameters, FALSE );
		//((MyFunction)commands[idx].function)( ps, parameters, FALSE );
	}
	return 0;
}

//--------------------------------------------------------------------------

static int HandleCommand( WIDE("IO"), WIDE("ALIAS"), WIDE("Alias macro commands - handle command filter on input") )( PSENTIENT ps, PTEXT parameters )
//static int CPROC Alias( PSENTIENT ps, PTEXT parameters )
{
	PTEXT op;
	op = GetParam( ps, &parameters );
	if( op )
	{
		Function f = GetRoutineRegistered( WIDE("TRIGGER/Commands"), op );
 //  	int idx;
	//	idx = GetCommandIndex( commands, NUM_COMMANDS
	  //							 , GetTextSize(op), GetText(op) );
		if( !f )
		{
			DECLTEXT( msg, WIDE("Alias Operation unknown....check /alias HELP...") );
			EnqueLink( &ps->Command->Output, &msg );
			return 0;
		}
		((MyFunction)f)( ps, parameters, TRUE );
	}
	return 0;
}

//--------------------------------------------------------------------------

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
	RegisterCommands( WIDE("TRIGGER/Commands"), commands, nCommands );
	//RegisterCommands( WIDE("TRIGGER/Options"), options, nOptions );
	//RegisterDeviceOpts( WIDE("TRIGGER"), options, nOptions );
	myTypeID = RegisterDeviceOpts( WIDE("trigger"), WIDE("Fictional Trigger device layer"), OpenTriggerSet, options, nOptions  );
	//RegisterRoutine( WIDE("Trigger"), WIDE("Data trigger functions"), Trigger );
	//RegisterRoutine( WIDE("Alias"), WIDE("Data trigger functions"), Alias );
	nTrigExten = RegisterExtension( WIDE("Trigger") );
	return DekVersion;
}

//--------------------------------------------------------------------------

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	UnregisterRoutine( WIDE("Trigger") );
	UnregisterRoutine( WIDE("Alias") );
	UnregisterDevice( WIDE("trigger") );

}

// $Log: trigger.c,v $
// Revision 1.30  2005/08/08 01:05:28  d3x0r
// UPdate to set datapath ID ina  good way - since we cheated to make this path... it's important to change the type this way.
//
// Revision 1.29  2005/02/23 11:39:00  d3x0r
// Modifications/improvements to get MSVC to build.
//
// Revision 1.28  2005/02/21 12:09:04  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.27  2005/01/27 08:28:30  d3x0r
// Make triggers self contained - eliminate more possible conflicts of library linkage
//
// Revision 1.26  2005/01/18 02:47:02  d3x0r
// Mods to protect symbols from overwrites.... (declare much things static, rename others)
//
// Revision 1.25  2004/05/04 07:30:27  d3x0r
// Checkpoint for everything else.
//
// Revision 1.24  2004/03/08 09:27:57  d3x0r
// Added some logging...
//
// Revision 1.23  2003/12/10 21:59:52  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.22  2003/10/26 12:38:17  panther
// Updates for newest scheduler
//
// Revision 1.21  2003/08/15 13:19:59  panther
// Define names for data paths
//
// Revision 1.20  2003/04/20 16:19:53  panther
// Fixes for history usage... cleaned some logging messages.
//
// Revision 1.19  2003/03/25 08:59:03  panther
// Added CVS logging
//
