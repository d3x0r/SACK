#ifndef CORE_SOURCE
#define CORE_SOURCE
#endif
#include <stdhdrs.h>
#include <deadstart.h>
#include <sqlgetoption.h>
#include <string.h>
#include <procreg.h>
#include <deadstart.h>
	 //#define NO_LOGGING
#define DO_LOGGING
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <filesys.h>
#define MASTER_CODE
#include "story.h"
#include "input.h" // requires burst...

#include "space.h"
#ifndef WIN32
#include <pthread.h>
#include <errno.h>
int ConsoleWrite( PDATAPATH pdp );
#endif
#include "plugin.h"

//--------------------------------------------------------------------------
// global variables... ???
LOGICAL gbExitNow;
PTHREAD pMainThread;
//LIBMAIN() { DebugBreak(); return 1; } LIBEXIT() { return 1; } LIBMAIN_END();
//--------------------------------------------------------------------------

CORE_PROC( PTEXT, GetName )( void *p )
{
	DECLTEXT( other, "No name" );
	PENTITY pe = (PENTITY)p;
	if( pe )
	{
		if( pe->flags.bShadow )
			return GetName( ((PSHADOW_OBJECT)pe)->pForm );
		else if ( pe->flags.bMacro )
			return ((PMACRO)p)->pName;
		return pe->pName;
	}
	//DebugBreak();
	lprintf( "Resulting with 'no name', Null entity" );
	return (PTEXT)&other;
}

//--------------------------------------------------------------------------

CORE_PROC( PTEXT, GetDescription )( void *p )
{
	DECLTEXT( other, "Nothing Special." );
	PENTITY pe = (PENTITY)p;
	if( pe )
	{
		if( pe->flags.bShadow )
			return GetDescription( ((PSHADOW_OBJECT)pe)->pForm );
		else if( pe->flags.bMacro )
		{
			if( ((PMACRO)p)->pDescription )
				return ((PMACRO)p)->pDescription;
		}
		else if( pe->pDescription )
			return pe->pDescription;
	}
	return (PTEXT)&other;
}

CORE_PROC( void, SetDescription )( void *p, PTEXT desc )
{
	PENTITY pe = (PENTITY)p;
	if( pe )
	{
		if( pe->flags.bShadow )
			return; // don't allow setting of shadow's descriptions?
		else if( pe->flags.bMacro )
		{
			LineRelease(((PMACRO)p)->pDescription );
			((PMACRO)p)->pDescription = desc;
		}
		else
		{
			LineRelease( pe->pDescription );
			pe->pDescription = desc;
		}
	}
}

//--------------------------------------------------------------------------

PENTITY putin(PENTITY that,PENTITY _this)
{
	_this->pWithin = that;
	if( that )
	{
		AddLink( &that->pContains, _this );
	}
	return that;
}

//--------------------------------------------------------------------------

PENTITY pullout(PENTITY that,PENTITY _this)
{
	if( that )
	{
		DeleteLink( &that->pContains, _this );
		_this->pWithin = NULL;
	}
	return _this;
}

//--------------------------------------------------------------------------

PLIST BuildAttachedListEx( PENTITY also_ignore, PLIST *ppList, PENTITY source, int max_levels )
{
	INDEX idx, idx2;
	PLIST pAttached = NULL;
	PENTITY pCurrent, pTest;
	int bPresent;
	if( !ppList )
		ppList = &pAttached;
	if( max_levels == 1 ) // 1 level has already been added, and that's all we can do...
		return NULL;
	LIST_FORALL( source->pAttached, idx, PENTITY, pCurrent )
	{
		bPresent = FALSE;
		// if the attached list is lengthy...
		// such as a road attached to an intersection, etc
		// may want to consider a level Limitation.
		if( also_ignore == pCurrent )
			continue;
		pAttached = *ppList;
		LIST_FORALL( pAttached, idx2, PENTITY, pTest )
			if( pTest == pCurrent )
			{
				bPresent = TRUE;
				break;
			}
		if( !bPresent )
		{
			AddLink( ppList, pCurrent );
			BuildAttachedListEx( also_ignore, ppList, pCurrent, max_levels?max_levels--:0 );
		}
	}
	return *ppList;
}

//--------------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "core object", "contents", "Contents of the current object" )( PENTITY pe, PTEXT *last )
//PTEXT CPROC GetContents( uintptr_t psv, PENTITY pe, PTEXT *last )
{
	PTEXT pList = NULL;
	if( !(*last) )
		(*last) = SegCreateIndirect( NULL );
	pList = WriteList( pe->pContains, NULL, NULL );
	if( *last )
		LineRelease( GetIndirect( *last ) );
	SetIndirect( (*last), pList );
	return (*last);
}

//--------------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "core object", "scriptpath", "Contents of the current object" )( PENTITY pe, PTEXT *last )
{
	static PTEXT path;
	if( !path )
	{
		PVARTEXT pvtTmp = VarTextCreate();
		vtprintf( pvtTmp, "%s/share/SACK/dekware/scripts", GetInstallPath() );
		path = VarTextGet( pvtTmp );
lprintf( "Script path:", GetText( path ));
		VarTextDestroy( &pvtTmp );
	}
	return path;
}

//--------------------------------------------------------------------------

PTEXT _GetContents( uintptr_t psv, PENTITY pe, PTEXT *last )
{
	PTEXT pList = NULL;
	if( !(*last) )
		(*last) = SegCreateIndirect( NULL );
	pList = WriteList( pe->pContains, NULL, NULL );
	if( *last )
		LineRelease( GetIndirect( *last ) );
	SetIndirect( (*last), pList );
	return (*last);
}

//--------------------------------------------------------------------------

PENTITY FindContainerEx( PENTITY source
							  , PENTITY *mount_point )
{
	PENTITY pCurrent, pResult;
	PLIST pAttached;
	INDEX idx;
	if( ( pResult = source->pWithin ) )
	{
		if( mount_point )
			*mount_point = source;
		return pResult;
	}
	pAttached = BuildAttachedList( source );
	LIST_FORALL( pAttached, idx, PENTITY, pCurrent )
	{
		if( ( pResult = pCurrent->pWithin ) )
		{
			if( mount_point )
				*mount_point = pCurrent;
			break;
		}
	}
	DeleteList( &pAttached );
	return pResult;
}

//--------------------------------------------------------------------------

PENTITY attach(PENTITY obj1,PENTITY obj2)
{
	// if and only if obj1 and obj2 are not already attached...
	if( obj1 && obj2 )
	{
		AddLink( &obj1->pAttached, obj2 );
		AddLink( &obj2->pAttached, obj1 );
	}
	return obj1;
}

//--------------------------------------------------------------------------

PENTITY detach(PENTITY obj1,PENTITY obj2)
{
	//if and only if obj1 and obj2 are attached...
	if( obj1 && obj2 )
	{
		DeleteLink( &obj1->pAttached, obj2 );
		DeleteLink( &obj2->pAttached, obj1 );
	}
	return obj2;
}

//--------------------------------------------------------------------------


PENTITY findbynameEx( PLIST list, size_t *count, TEXTCHAR *name, PENTITY exclude_self )
{
	INDEX idx;
	INDEX cnt;
	PENTITY workobject;
	if( count )
		cnt = *count;
	else
		cnt = 0;

	LIST_FORALL(list,idx,PENTITY, workobject)
	{
		if( workobject == exclude_self )
			continue;
		//lprintf( "Is %s==%s?", GetText( GetName( workobject ) ), name );
		if( NameIs( workobject, name) )
		{
			if( cnt )
				cnt--;
			if( !cnt )
				return(workobject);
		}
	}
	if( count )
		*count = cnt;
	return((PENTITY)NULL);
}

//--------------------------------------------------------------------------

POINTER DoFindThing( PENTITY Around, enum FindWhere type, enum FindWhere *foundtype, size_t *count, TEXTCHAR *t )
{
	PENTITY pContainer;
	POINTER p;
	static int level;
	enum FindWhere junk;
	if( !foundtype )
		foundtype = &junk;
	p = NULL;
	//lprintf( "Set found type to %d", type );
	*foundtype = type; // ugly, and confusing location for _this...
							 // because of recursive nature, last test which
	// succeeds leaves _this value set...
	level++;
	if( Around )
	{
		switch( type )
		{
		case FIND_VISIBLE:
			//lprintf( "(%d)Find visible...", level );
			if( !p ) p = DoFindThing( Around, FIND_GRABBABLE, foundtype, count, t );
			if( !p ) p = DoFindThing( Around, FIND_AROUND, foundtype, count, t );
			if( !p ) p = DoFindThing( Around, FIND_ANYTHING_NEAR, foundtype, count, t );
			break;
		case FIND_GRABBABLE:
			//lprintf( "(%d)Find grabbable...", level );
			if( !p ) p = DoFindThing( Around, FIND_NEAR, foundtype, count, t );
			if( !p ) p = DoFindThing( Around, FIND_WITHIN, foundtype, count, t );
			if( !p ) p = DoFindThing( Around, FIND_ON, foundtype, count, t );
			break;
		case FIND_IN:
			{
				//lprintf( "(%d)Find in...", level );
				p = findbynameEx(Around->pContains,count,t, NULL);
			}
			break;
		case FIND_WITHIN:
			{
				PLIST pList = NULL;
				INDEX idx;
				PENTITY pe;
				//lprintf( "(%d)Find within...", level );
				LIST_FORALL( Around->pContains, idx, PENTITY, pe )
				{
					//lprintf( "adding a link to the list..." );
					AddLink( &pList, pe );
					BuildAttachedListEx( NULL, &pList, pe, 0 );
				}
				p = findbynameEx( pList, count, t, NULL);
				DeleteList( &pList );
				if( p == Around )
					p = NULL;
			}
			break;
		case FIND_ON:
			{
				PLIST pList;
				//lprintf( "(%d)Find on...", level );
				p = findbynameEx(pList=BuildAttachedList(Around),count,t, NULL);
				if( p == Around )
					p = NULL;
				DeleteList( &pList );
			}
			break;
		case FIND_NEAR:
			//lprintf( "(%d)Find near...", level );
			if( ( pContainer = FindContainer( Around ) ) )
			{
				p = findbynameEx( pContainer->pContains, count, t, Around );
				if( p == Around )
					p = NULL;
			}
			break;
		case FIND_ANYTHING_NEAR:
			//lprintf( "(%d)Find anything near...", level );
			if( ( pContainer = FindContainer( Around ) ) )
			{
				PLIST pList = NULL;
				INDEX idx;
				PENTITY pe;
				AddLink( &pList, Around );
				LIST_FORALL( pContainer->pContains, idx, PENTITY, pe )
				{
					AddLink( &pList, pe );
					BuildAttachedListEx( Around, &pList, pe, 0 );
				}
				p = findbynameEx( pList, count, t, NULL );
				DeleteList( &pList );
				if( p == Around )
					p = NULL;
			}
			break;
		case FIND_AROUND:
			//lprintf( "(%d)Find around...", level );
			if( ( pContainer = FindContainer( Around ) ) )
			{
				if( !p ) p = findbynameEx(pContainer->pAttached,count,t, Around);
				if( !p )
					if( NameIs( pContainer, t ) )
						p = pContainer;
			}
			break;
		case FIND_MACRO:
			//lprintf( "(%d)Find macro...", level );
			p = findbynameEx(Around->pMacros, count, t, NULL );
			break;
		case FIND_MACRO_INDEX:
			{
				INDEX idx;
				PENTITY workobject;
				LIST_FORALL(Around->pMacros,idx,PENTITY, workobject)
					if( NameIs( workobject, t ) )
						return (POINTER)idx;
			}
			return (POINTER)-1;
		}
	}
	level--;
	return p;
}

//------------------------------------------------------------------------

CORE_PROC( POINTER, FindThingEx )( PSENTIENT ps, PTEXT *tokens
											, PENTITY Around
											, enum FindWhere type
											, enum FindWhere *foundtype
											, PTEXT *pObject
											, PTEXT *pResult // if object was found, _this would be where paramters points
											 DBG_PASS )
{
	//TEXTCHAR *t, *sep;
	PTEXT pText;
	PVARTEXT vt = NULL;
	POINTER p = NULL;
	// at one point we get an extra token
	// which we created... _this needs to be
	// destroyed.  Data from GetParam is considered static/const (readonly)
	PTEXT delete_seg = NULL;
	PTEXT _tokens = *tokens;
	//size_t cnt;
	int64_t long_cnt = 1; // by default find the first.
	{
		//PTEXT line = BuildLine( *tokens );
		//lprintf( "finding [%s]", GetText( line ) );
		//LineRelease( line );
	}
	// find one thing only.
	do
	{
		pText = GetParam( ps, tokens );
		if( pText && ((pText->flags & TF_ENTITY )== TF_ENTITY) )
		{
			//lprintf( "Segment itself is an entity... therefore no searching required." );
			return GetApplicationPointer( pText );
		}
		if( pText && (pText->flags & TF_INDIRECT) )
		{
			// a literal name of an object such as %me or %room
			// will never be indirect.
			delete_seg = pText = BuildLine( GetIndirect( pText ) );
		}
		{
			//PTEXT line = BuildLine( *tokens );
			//lprintf( "finding [%s] token [%s]", GetText( line ), GetText( pText ) );
			//LineRelease( line );
		}
		// test literal equality, might be the same value as some other object
		// but this name is literally this object
		if( pText == GetName( Around ) )
		{
			// matches %me (?) well it's a proper name whoever it is...
			// and it's something we know SO....
			if( foundtype )
				*foundtype = FIND_SELF;
			return Around;
		}
		if( pObject )
		{
			if( !vt )
				vt = VarTextCreate();
			vtprintf( vt, "%s", GetText( pText ) );
		}

		if( type != FIND_MACRO )
		{
			(*tokens) = _tokens;
			p = ResolveEntity( ps, Around, type, tokens, FALSE );
			if( p == Around )
				p = NULL;
		}
		else
		{
			size_t cnt = 1;
			p = DoFindThing( Around, type, foundtype, &cnt, GetText( pText ) );
		}
#if 0
		t = GetText( pText );
		if( t )
		{
			if( ( sep = strchr( t, '.' ) ) )
			{
				lprintf( "Has or is a '.', and uhmm something." );
				if( ( sep == t ) && ( GetTextSize( pText ) == 1 ) )
				{
					continue;
				}
				else if( t[0] >= '0' && t[0] <= '9' )
				{
					lprintf( "it's a count at the start...." );
					sep[0] = 0;
					cnt = atoi( t );
					t = sep + 1;
					//p = DoFindThing( Around, type, foundtype, &cnt, t );
					sep[0] = '.';
					lprintf( "Object count is now %" _size_f " . %s", cnt, t );
				}
			}
			else if( IsIntNumber( pText, &long_cnt )  )
			{
				lprintf( "The token returned was a number, therefore it's not an entity?" );
				continue;
			}
			else
				cnt = (int)long_cnt;
			if( !p ) p = DoFindThing( Around, type, foundtype, &cnt, t );
			//lprintf( "And we managed to find... %p", p );
		}
#endif
		break;
	}
	while( 1 );
	if( delete_seg )
		LineRelease( delete_seg );
	if( pObject )
	{
		if( *pObject )
			LineRelease( *pObject );
		*pObject = VarTextGet( vt );
		VarTextDestroy( &vt );
	}
	if( pResult )
		*pResult = *tokens;
	if( !p )
		*tokens = _tokens;
	//lprintf( "And we still have %p", p );
	return p;
}

//--------------------------------------------------------------------------

PENTITY GetEntity( void *pe )
{
	while( ((PSHADOW_OBJECT)pe)->flags.bShadow )
		pe = (void *)GetEntity( ((PSHADOW_OBJECT)pe)->pForm );
	return (PENTITY)pe;
}

//--------------------------------------------------------------------------

PSHADOW_OBJECT CreateShadowIn( PENTITY pContainer, PENTITY pe )
{
	PSHADOW_OBJECT pShadow;
	pShadow = New( SHADOW_OBJECT );
	MemSet( pShadow, 0, sizeof( SHADOW_OBJECT ) );
	pShadow->flags.bShadow = TRUE;
	
	pe = GetEntity( pe ); // always shadow the real object never shadow a shadow
	pShadow->pForm = pe;

	pShadow->pShadowOf = pe;
	AddLink( &pe->pShadows, pShadow );
	if( pContainer )
		putin( pContainer, (PENTITY)pShadow );
	return pShadow;
}

//--------------------------------------------------------------------------

static PTEXT ObjectVolatileVariableGet( "core object", "scriptfile", "next temp scriptfilename to use" )( PENTITY pe, PTEXT *last )
//PTEXT CPROC GetScriptName( uintptr_t psv, PENTITY pe, PTEXT *last )
{
	static int nScript;
	PVARTEXT pvt = VarTextCreate();
	vtprintf( pvt, "script_%05d", nScript++ );
	if( *last )
		LineRelease( *last );
	*last = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	return *last;
}

//--------------------------------------------------------------------------

//volatile_variable_entry pveContents = { DEFTEXT( "contents" ), GetContents, NULL, NULL };
//volatile_variable_entry pveScript	= { DEFTEXT( "scriptfile" )
//													, GetScriptName, NULL, NULL };
//volatile_variable_entry pveContents = { DEFTEXT( "contents" ), GetContents, NULL, NULL };

//--------------------------------------------------------------------------

static int OnCreateObject( "core object", "Basic core object variables and methods." )( PSENTIENT ps, PENTITY pe, PTEXT params )
{
	// nothing really special... just registering the object extension really...
	return 0;
}

static PENTITY CreateEntity( PTEXT pName )
{
	PENTITY pe;
	pe = New( ENTITY );
	MemSet( pe, 0, sizeof( ENTITY ) ); // allocate returns zero mem...

	pName->flags &= ~(IS_DATA_FLAGS);
	pName->format.position.offset.spaces = 0;
	pe->pName = pName;
	Assimilate( pe, NULL, "core object", NULL );
#if 0
	AddVolatileVariable( pe, &pveContents, 0 );
	AddVolatileVariable( pe, &pveScript, 0 );
	{
		int n;
		extern int nStandardVVE;
		extern volatile_variable_entry standard_vves[];
		for( n = 0; n < nStandardVVE; n++ )
			AddVolatileVariable( pe, standard_vves + n, 0 );
	}
#endif
	return pe;
}

//--------------------------------------------------------------------------

CORE_PROC( PENTITY, CreateEntityIn )( PENTITY Location, PTEXT pName )
{
	PENTITY pTemp, peCreator;
	if( !Location )
		peCreator = global.THE_VOID;
	else
	{
		peCreator = Location;
		// find list of what people in _this near location
		// and announce to those that are operators that the enetity has
		// been created, and by whom
	}
	if( peCreator )
	{
		pTemp = CreateEntity( pName );
		putin( peCreator, pTemp );
		pTemp->pCreatedBy = peCreator;
		AddLink( &peCreator->pCreated, pTemp );
		return pTemp;
	}
	return NULL;
}

//--------------------------------------------------------------------------

CORE_PROC( PENTITY, Duplicate )(PENTITY object)
{
	// hmm duplicate creates an exact copy of _this thing
	// but does not create the awarenesses....
	// each entity should get a on Duplicate method also
	// and if such a method is defined, the copied object
	// will also be made aware to process _this startup....????
	PENTITY pNew, pDup;
	INDEX idx;
	PMACRO pm;
	PTEXT pVar;
	PLIST pList;

	object = GetEntity( object );

	pNew = CreateEntity( LineDuplicate( object->pName ) );
	pNew->pDescription = LineDuplicate( object->pDescription );

	pNew->pCreatedBy = object;
	AddLink( &object->pCreated, pNew );

	// copy macros on _this object
	pList = object->pMacros;
	LIST_FORALL( pList, idx, PMACRO, pm )
	{
		AddLink( &pNew->pMacros, DuplicateMacro( pm ) );
	}

	// copy Behavior macros
	{
		PTEXT name;
		pList = object->behaviors;
		LIST_FORALL( pList, idx, PTEXT, name )
		{
			SetLink( &pNew->behaviors, idx, SegDuplicate( name ) );
		}
	}
	pList = object->pBehaviors;
	LIST_FORALL( pList, idx, PMACRO, pm )
	{
		SetLink( &pNew->pBehaviors, idx, DuplicateMacro( pm ) );
	}
	pList = object->pGlobalBehaviors;
	LIST_FORALL( pList, idx, PMACRO, pm )
	{
		SetLink( &pNew->pGlobalBehaviors, idx, DuplicateMacro( pm ) );
	}

	// copy local variables
	pList = object->pVars;
	LIST_FORALL( pList, idx, PTEXT, pVar )
	{
		AddLink( &pNew->pVars, TextDuplicate( pVar, FALSE ) );
	}

	putin( FindContainer( object ), pNew );
	// duplicate all contents.
	LIST_FORALL( object->pContains, idx, PENTITY, pDup )
		putin( pNew, Duplicate( pDup ) );

  	{
  		void (*DupMethod)( PENTITY peSource, PENTITY peNew );
		LIST_FORALL( object->pDuplicate, idx, void(*)(PENTITY,PENTITY), DupMethod )
		{
			DupMethod( object, pNew );
		}
	}
	return(pNew);
}
//--------------------------------------------------------------------------

CORE_PROC( void, DestroyEntityEx )( PENTITY pe DBG_PASS )
{
	INDEX idx;
	PENTITY object, pWithin;
	PMACRO macro;
	PTEXT text;
	TEXTSTR string;
	//#ifdef _DEBUG
	_xlprintf( 1 DBG_RELAY )( "Destroying an entity(%s)", GetText( GetName( pe ) ) );
	//#endif
	if( pe == global.THE_VOID )
	{
		if( !gbExitNow )
		{
			gbExitNow = TRUE;
			Log( "Waking main thread to exit!" );
			WakeThreadID( GetThreadID( pMainThread ) );
			return;
		}
		global.THE_VOID = NULL;
	}
	if( !pe )
		return; // bad bad error - why ?!?

	if( pe->flags.bDestroy )
	{
		// if it's been destroyed, how are we here?
		// DebugBreak();
		// it could be destroyed already... and corrupt memory
		// this will still fail if someone thinks it knows a reference to this object.
		return;
	}

	pe->flags.bDestroy = TRUE;

	if( pe->flags.bShadow )
	{
		PSHADOW_OBJECT pshadow = (PSHADOW_OBJECT)pe;
		// uhmm stuff here...
		// remove from room contents...
		pullout( pshadow->pWithin, (PENTITY)pshadow );
		LIST_FORALL( pshadow->pAttached, idx, PENTITY, object )
		{
			detach( (PENTITY)pshadow, object );
		}

		DeleteLink( &pe->pShadowOf->pShadows, pe );

		ReleaseEx( pshadow DBG_RELAY );
		return;
	}


	{
		PLIST pList = (PLIST)LockedExchangePtrSzVal( (PVPTRSZVAL)&pe->pCreated, 0 );
		PENTITY pec;
		LIST_FORALL( pList, idx, PENTITY, pec )
		{
			DestroyEntity( pec );
			//SetLink( &pe->, idx, NULL );
		}
		DeleteList( &pList );
	}

	pWithin = FindContainer( pe );
	{
		PLIST pList = (PLIST)LockedExchangePtrSzVal( (PVPTRSZVAL)&pe->pContains, 0 );
		LIST_FORALL( pList, idx, PENTITY, object )
		{
			pullout( pe, object );
			putin( pWithin, object );
		}
		DeleteList( &pList );
	}

	{
		PLIST pList = (PLIST)LockedExchangePtrSzVal( (PVPTRSZVAL)&pe->pAttached, 0 );
		LIST_FORALL( pList, idx, PENTITY, object )
		{
			detach( pe, object );
			if( !FindContainer( object ) )
				putin( pWithin, object );
		}
		// on the last object check to see if the glob is
		// contained by something... if not, put it in my container.
		DeleteList( &pList );
	}

	lprintf( "within %p pe->pWithin %p", pWithin, pe->pWithin );
	if( pe->pWithin )
		pullout( pe->pWithin, pe );
	pe->pWithin = pWithin;


	LIST_FORALL( pe->pShadows, idx, PENTITY, object )
		DestroyEntity( object );


	if( pe->pShadowOf )
		DeleteLink( &pe->pShadowOf->pShadows, pe );
	// delete _this from the creator's list...
	if( pe->pCreatedBy )
	{
		// also at this point...
		//	I'm being destroyed from other external event
		//	and being destroyed as the universe is being unmade.


		// since I grab the list, I have created nothing
		// but am locked into destroying all things that were in that list.
		// also saves me from having the list modify in case destrion somehow causes creation
		_xlprintf(1 DBG_RELAY)( "Deleting (%s) from creator (%s)",GetText( GetName( pe ) ), GetText( GetName( pe->pCreatedBy ) ) );
		// if creator is being destroyed, allow him to not know I exist.
		if( !pe->pCreatedBy->flags.bDestroy && !DeleteLink( &pe->pCreatedBy->pCreated, pe ) )
		{

			lprintf( "Thing I think created me, doesn't know it created me..." );
			lprintf( "I am: %s and %s didn't create me?", GetText( GetName( pe ) ), GetText( GetName( pe->pCreatedBy ) ) );
			DebugBreak();
		}
		pe->pCreatedBy = NULL;
	}

	{
		volatile_variable_entry *pvve;
		PLIST list = (PLIST)LockedExchangePtrSzVal( (PVPTRSZVAL)&pe->pVariables, 0 );

		LIST_FORALL( list, idx, volatile_variable_entry *, pvve )
		{
			if( pvve->pLastValue )
			{
				LineRelease( pvve->pLastValue );
				pvve->pLastValue = NULL;
			}
			Release( pvve );
		}
		DeleteList( &list ); // delete list of volatile variables.
	}

	// the contents of pmethods are the statis tree portions in procreg...
	// just delete the list, the contents are persistant
	DeleteList( &pe->pMethods );

	if( pe->pControlledBy )
	{
		if( pe->pControlledBy == global.PLAYER )
			global.PLAYER = NULL;

		if( !DestroyAwarenessEx( pe->pControlledBy DBG_RELAY ) )
		{
			// Destruction will fail if processing or being destroyed...
			// _this entity could may already be being destroyed....
			pe->pControlledBy->flags.destroy_object = TRUE;
			return;
		}
	}

	// plugins may set methods on object if deleted...
	{
		void (*DeleteMethod)(PENTITY pe, INDEX iPlugin);
		{
			PLIST pList = (PLIST)LockedExchangePtrSzVal( (PVPTRSZVAL)&pe->pDestroy, 0 );
			LIST_FORALL( pList, idx,void(*)(PENTITY,INDEX), DeleteMethod )
			{
				if( DeleteMethod )
					DeleteMethod( pe, idx );
			}
			DeleteList( &pList );
		}
	}

	LIST_FORALL( pe->pMacros, idx, PMACRO, macro )
		DestroyMacro( pe, macro );
	DeleteList( &pe->pMacros );

	LIST_FORALL( pe->pBehaviors, idx, PMACRO, macro )
		DestroyMacro( pe, macro );
	DeleteList( &pe->pBehaviors );

	LIST_FORALL( pe->pGlobalBehaviors, idx, PMACRO, macro )
		DestroyMacro( pe, macro );
	DeleteList( &pe->pGlobalBehaviors );

	LIST_FORALL( pe->behaviors, idx, TEXTSTR, string )
		Release( string );
	DeleteList( &pe->behaviors );

	LIST_FORALL( pe->pVars, idx, PTEXT, text )
		LineReleaseEx( text  DBG_RELAY );
	DeleteListEx( &pe->pVars DBG_RELAY );

	LineReleaseEx( pe->pName DBG_RELAY );
	LineReleaseEx( pe->pDescription DBG_RELAY );

	Release( pe );
}

//------------------------------------------------------------------------

INDEX level=0;

PENTITY showall( PLINKQUEUE *ppOutput, PENTITY object)
{
	PENTITY next;
	INDEX idx;
	PTEXT pLine;

	if( !object )
		return NULL;
	if( object == global.THE_VOID )
		level = 0;
	pLine = SegCreate( 256 );
	pLine->data.size = 0;
	for (idx=0;idx<level;idx++)
	{
		pLine->data.size += snprintf( pLine->data.data + pLine->data.size, (256-pLine->data.size)*sizeof(TEXTCHAR), "	" );
	}

	// bad should be enqued copies of these strings...
	pLine->data.size += snprintf( pLine->data.data + pLine->data.size, (256-pLine->data.size)*sizeof(TEXTCHAR), "%s", GetText( GetName(object) ) );
	EnqueLink( ppOutput, pLine );

	LIST_FORALL( object->pContains, idx, PENTITY, next )
	{
		if (next!=object)  /* the void screws us all the time! */
		{
			level++;
			showall( ppOutput, next);
			level--;
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------
#undef CreateDataPath
CORE_PROC( PDATAPATH, CreateDataPath )( PDATAPATH *ppChannel, int nExtra )
{
	PDATAPATH pdp;
	pdp = NewPlus( DATAPATH, nExtra );
	MemSet( pdp, 0, sizeof( DATAPATH ) + nExtra );
	pdp->ppMe	  = ppChannel;
	//Log( "Linking in new path? " );
	if( ppChannel )
	{
		pdp->pPrior	= *ppChannel;
		if( pdp->pPrior )
			pdp->pPrior->ppMe = &pdp->pPrior;
		*ppChannel	 = pdp;
	}
	else
	{
		// *0 = 0; // CRASH!!!
	}
	return pdp;
}

//--------------------------------------------------------------------------

CORE_PROC( PDATAPATH, DestroyDataPathEx )( PDATAPATH pdp DBG_PASS )
{
	PTEXT p;
	PTEXT pName;
	PDATAPATH pReturn;
	if( !pdp )
		return NULL;
	pName = pdp->pName;
	{
		extern int gbTrace;
		if( gbTrace )
			Log1( "calling datapath close:%s", GetText( pName ) );
	}
	//_xlprintf(1 DBG_RELAY)( "Closing datapath %p(%d)", pdp, pdp->Type );
	if( !CloseDevice( pdp ) )
	{
		// device denied closing...
		return 0;
	}
	//lprintf( "Removed from it's device...." );
	//Log2( "%s(%d):Deleting datapath Input", pFile, nLine );
	while( ( p = (PTEXT)DequeLink( &pdp->Input ) ) )
		LineRelease( p );
	DeleteLinkQueue( &pdp->Input );

	//Log2( "%s(%d):Deleting datapath Output", pFile, nLine );
	while( ( p = (PTEXT)DequeLink( &pdp->Output ) ) )
	{
#ifdef _DEBUG
		{
			PTEXT out = BuildLine( p );
				Log1( "Trashing output line: %s", GetText( out ) );
				LineRelease( out );
			}
#endif
		LineRelease( p );
	}
	DeleteLinkQueue( &pdp->Output );
	//pdp->ppOutput = NULL;

	LineRelease( pdp->Partial );
	LineRelease( pdp->CurrentLine );
	if( pdp->pPrior )
			pdp->pPrior->ppMe = pdp->ppMe;

	if( pdp->ppMe )
		*(pdp->ppMe) = pdp->pPrior;
	pReturn = pdp->pPrior;
	LineRelease( pdp->pName );
	if( pdp->CommandInfo )
		DestroyCommandHistory( &pdp->CommandInfo );

	ReleaseEx( pdp DBG_RELAY );
	return pReturn;
}

//--------------------------------------------------------------------------
static int CPROC DefaultCommandOutput( PDATAPATH pdp )
{
	DECLTEXT( msg, ":" );
	PENTITY Current = pdp->Owner->Current;
	if( !IsQueueEmpty( &pdp->Output ) )
	{
		PENTITY peForwardTo;
		PTEXT pReQue, pOrig;
		pReQue = SegAppend( SegAppend( TextDuplicate( Current->pName, FALSE )
											  , SegCreateIndirect( (PTEXT)&msg ) )
								, pOrig = (PTEXT)DequeLink( &pdp->Output ) );
		// I dunno - seems output from a sub object should be
		// line oriented even if it's instructed to be noreturn/formatted
		// cause it's still gonna get mucked about with the output
		// from the primary/owner/creator object....

		if( pOrig->flags & TF_STATIC )
			SegSubst( pOrig, SegDuplicate( pOrig ) );

		peForwardTo = Current;
		while( peForwardTo->pCreatedBy && !peForwardTo->pCreatedBy->pControlledBy )
		{
			pReQue = SegAppend( SegAppend( TextDuplicate( peForwardTo->pCreatedBy->pName, FALSE ), SegCreateIndirect( (PTEXT)&msg ) ), pReQue );
			peForwardTo = peForwardTo->pCreatedBy;
		}

		if( peForwardTo->pCreatedBy &&
			peForwardTo->pCreatedBy->pControlledBy )
		{
			 EnqueLink( &peForwardTo->pCreatedBy->pControlledBy->Command->Output
						, pReQue );
		}
		else
		{
			PTEXT p, pOutput;
			pOutput = BuildLine( pReQue );
			LineRelease( pReQue );
			p = pOutput;
			while( p )
			{
				SystemLog( GetText( p ) );
				p = NEXTLINE( p );
			}
			LineRelease( pOutput );
		}
		return 1;
	}
	return 0;
}

//--------------------------------------------------------------------------

CORE_PROC( PSENTIENT, CreateAwareness )( PENTITY pEntity )
{
	PSENTIENT ps;

	if( pEntity->pControlledBy )
	{
		//DECLTEXT( msg, "Attempt to reawaken you...\n" );
		//EnqueLink( pEntity->pControlledBy->Command->ppOutput, (PTEXT)&msg );
		return NULL;
	}

	ps = New( SENTIENT );
	MemSet( ps, 0, sizeof( SENTIENT ) ); // must zero !
	ps->ProcessLock = 1; // LOCKED; // creator of awareness MUST unlock!
	ps->StepLock = 1;	 // step locked.
	ps->Current = pEntity;
	pEntity->pControlledBy = ps;

	// note _this syntax is different from plugins...
	CreateDataPath( &ps->Command, 0 );
	ps->Command->Owner = ps;
	ps->Command->Write = DefaultCommandOutput;
	ps->Command->pName = SegCreateFromText( "Default" );

	ps->MacroStack = CreateDataStack( sizeof(MACROSTATE) );

	ps->Next = AwareEntities;

	if( AwareEntities )
	{
		while( LockedExchange( &AwareEntities->StepLock, 1 ) )
			Sleep(0);
		AwareEntities->Prior = ps;
		AwareEntities->StepLock = 0;
	}
	{
		DECLTEXT( prompt, "prompt" );
		DECLTEXT( prompt_default, "[%%room/%%me]:" );
		PTEXT pDefault = burst( (PTEXT)&prompt_default );
		AddVariable( ps, ps->Current, (PTEXT)&prompt, pDefault );
		LineRelease( pDefault );
	}
	AwareEntities = ps;
	ps->StepLock = 0; // easy to clear....
	return ps;
}

//--------------------------------------------------------------------------

CORE_PROC( void, UnlockAwareness )( PSENTIENT ps )
{
	if( ps )
		ps->ProcessLock = 0;
}

//--------------------------------------------------------------------------

CORE_PROC( int, DestroyAwarenessEx )( PSENTIENT ps DBG_PASS )
{
	PTEXT pVar;
	INDEX idx;
	PMACROSTATE pms;
	if( ps->ProcessingLock &&
		 !ps->flags.force_destroy )
	{
		ps->flags.destroy = TRUE;
		return FALSE;
	}
	// lock the object NOW...
	while( !ps->flags.force_destroy &&
			 LockedExchange( &ps->ProcessingLock, 1 ) ) Sleep(0);

	ps->Current->pControlledBy = NULL;

	do
	{
			DestroyDataPath( ps->Data );
	} while( ps->Data );

	do
	{
		DestroyDataPath( ps->Command );
	} while( ps->Command );


	while( ( pms = (PMACROSTATE)PeekData( &ps->MacroStack ) ) )
	{
		LIST_FORALL( pms->pVars, idx, PTEXT, pVar )
		{
			PTEXT pNext;
			if( ( pNext = NEXTLINE( pVar ) ) )
			{
				LineRelease( GetIndirect( pNext ) );
				SetIndirect( pNext, NULL );
			}
			LineRelease( pVar );
		}
		DeleteList( &pms->pVars );
		pms->pVars = NULL;
		LineRelease( pms->pArgs );
		pms->pArgs = NULL;
		if( pms->StopEvent )
			pms->StopEvent( pms->psv_StopEvent, pms );
		if( pms->MacroEnd )
		{
			pms->MacroEnd( ps, pms );
		}
		else
		{
			pms->pMacro->flags.un.macro.bUsed = FALSE;
			if( pms->pMacro->flags.un.macro.bDelete )
					DestroyMacro( ps->Current, pms->pMacro );
		}
		PopData( &ps->MacroStack );
	}
	DeleteDataStack( &ps->MacroStack );

	if( ps->Prior )
	{
		while( LockedExchange( &ps->Prior->StepLock, 1 ) )
			Sleep(0);
		ps->Prior->Next = ps->Next;
		ps->Prior->StepLock = 0; // easy to clear....
	}
	else
		AwareEntities = ps->Next;

	if( ps->Next )
	{
		while( LockedExchange( &ps->Next->StepLock, 1 ) )
			Sleep(0);
		ps->Next->Prior = ps->Prior;
		ps->Next->StepLock = 0; // easy to clear....
	}
	if( ps->flags.destroy_object )
		DestroyEntityEx( ps->Current DBG_RELAY );

	ReleaseEx( ps DBG_RELAY );
	return TRUE; // success!!!
}

//--------------------------------------------------------------------------

void Story( PLINKQUEUE *ppOutput )
{
	INDEX idx;
	PTEXT pWork;
	for (idx=0;idx<NUMLINES;idx++)
	{
		pWork = SegCreateFromText( storytext[idx] );
		EnqueLink( ppOutput, pWork );
	}
}

//--------------------------------------------------------------------------

PENTITY Big_Bang( PTEXT pName )
{
	/* _this routine plays funny games to set up an initial starting place for the
		universe to come... Kids, do not try _this at home :> */
	PENTITY pTemp;

	pTemp=CreateEntity( pName );

	putin(pTemp,pTemp);
	// attach(pTemp,pTemp); // no maybe don't attach the universe to itself...
	return pTemp;
}

//--------------------------------------------------------------------------

CORE_PROC( PENTITY, GetTheVoid )( void )
{
	return global.THE_VOID;
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------

static PTEXT CPROC GetSentientName( uintptr_t psv, POINTER p )
{
	PSENTIENT ps = (PSENTIENT)p;
	return GetName( ps? ps->Current:NULL );
}
//--------------------------------------------------------------------------

static PTEXT CPROC GetEntityName( uintptr_t psv, POINTER p )	
{
	PENTITY pe = (PENTITY)p;
	return GetName( pe );
}


//--------------------------------------------------------------------------

void DoCommandf( PSENTIENT ps, CTEXTSTR f, ... )
{
	PVARTEXT pvt = VarTextCreate();
	PTEXT cmd, temp;
	va_list args;
	va_start( args, f );
	vvtprintf( pvt, f, args );
	cmd = burst( temp = VarTextGet( pvt ) );
	LineRelease( temp );
	EnqueLink( &ps->Command->Input, cmd);
	WakeAThread( ps );
}
//--------------------------------------------------------------------------

PTEXT global_command_line;

int InitSpace( const TEXTCHAR *command_line )
{
//	pg = Allocate( sizeof( GLOBAL ) );
	//	MemSet( pg, 0, sizeof( GLOBAL ) );
	global.flags.bLogAllCommands = SACK_GetProfileInt( "options", "Log All Commands", 0 );
	global.THE_VOID = Big_Bang( SegCreateFromText( "The Void" ) );
	{
		PTEXT tmp = SegCreateFromText( command_line );
		global_command_line = burst( tmp );
		{
			PTEXT t;
			for( t = global_command_line; t; t = NEXTLINE( t ) )
			{
				t->flags |= TF_STATIC;
			}
		}
		LineRelease( tmp );
	}
	global.THE_VOID->pDescription = SegCreateFromText( "Transparent black clouds swirl about." );
	RegisterTextExtension( TF_SENTIENT_FLAG, GetSentientName, 0 );
	RegisterTextExtension( TF_ENTITY_FLAG, GetEntityName, 0 );
	AddCommonBehavior( "Enter", "invoked on entity being entered by an entity" );
	AddCommonBehavior( "Close", "some device on datapath has closed" );
	AddCommonBehavior( "Conceal", "invoked on entity entering an entity" );
	AddCommonBehavior( "Leave", "invoked on entity leaving an entity" );
	AddCommonBehavior( "Inject", "invoked on entity being entered by an entity leaving a contained entity" );
	AddCommonBehavior( "Grab", "invoked on entity being grabbed" );
	AddCommonBehavior( "Store", "invoked on entity being stored" );
	AddCommonBehavior( "Insert", "invoked on entity being stored into" );
	AddCommonBehavior( "Place", "invoked on entity storing another entity" );
	AddCommonBehavior( "Pull", "..." );
	AddCommonBehavior( "Receive", "..." );
	{
		DECLTEXT( scriptname, "__do_script__" );
		PVARTEXT pvt = VarTextCreate();
		PTEXT temp, cmd;
		global.script = CreateMacro( NULL, NULL, (PTEXT)&scriptname );

		vtprintf( pvt, "/decl devname %%scriptfile" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/command %%devname file __input %%..." );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/if success" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);
		
		vtprintf( pvt, "/option %%devname close" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/command %%scriptfile bash noblank" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/else" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/command %%devname file __input %%scriptpath/%%..." );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/if success" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);
		
		vtprintf( pvt, "/option %%devname close" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/command %%scriptfile bash noblank" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/else" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/echo failed to open script: %%scriptpath/%%..." );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		vtprintf( pvt, "/endif" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);
		
		vtprintf( pvt, "/endif" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);

		/*
		vtprintf( pvt, "/command tempdebug debug __input __output %%scriptpath/%%..." );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd );
		*/

		vtprintf( pvt, "/endmac" );
		cmd = burst( temp = VarTextGet( pvt ) );
		LineRelease( temp );
		AddMacroCommand( global.script, cmd);
		VarTextDestroy( &pvt );
		global.script->nArgs = -1;
	}

	return TRUE;
}

//--------------------------------------------------------------------------
#define StepNextSentient() { while( LockedExchange( &ps->StepLock, 1 ) ) \
								Sleep(0);		  \
							pNext = ps->Next;	\
							ps->StepLock = 0; }


int ProcessSentients( THREAD_ID ThreadID )
{
	extern int gbTrace;
	PENTITY peMe;
	PSENTIENT ps, pNext;
	PTEXT pCommand;
	PMACROSTATE pms = NULL;
	int total;
	int bFree;
	total = 0;
	pNext = AwareEntities;
	while( ( ps = pNext ) )
	{
		int n = 0;
		//SystemLog( "Process Sentient." );
		pCommand = NULL;
		if( ps->ProcessLock || LockedExchange( &ps->ProcessingLock, 1 ) )
		{
			StepNextSentient();
			continue;
		}
		if( ps->flags.destroy )
		{
			ps->flags.force_destroy = TRUE;
			DestroyAwareness( ps );
			// can't step from _this - cause it's now gone - must restart...
			return total + 1; // might not end up asleep yet...
		}

		/*
		if( ps->flags.waiting_for_someone )
		{
			lprintf( "sentience is waiting on a posted command to complete" );
			StepNextSentient();
			continue;
			}
			*/
		if( gbTrace )
			lprintf( "---- PROCESS %p(%s)", ps, GetText( GetName( ps->Current ) ) );
		do
		{
			if( !ps->flags.bRelay	// not relay
				|| !ps->Data			// no datapath
				|| PeekData( &ps->MacroStack ) // running a macro - gotta end _this...
			  )
			{
				PTEXT *ppCommand = &pCommand;
				//lprintf( "Do free this command." );
				bFree = TRUE;
				ps->CurrentMacro = NULL; // gets restored next command....
				{
					if( ( pms = (PMACROSTATE)PeekData( &ps->MacroStack ) )
						&& pms->state.flags.forced_run )
					{
						if( gbTrace )
							lprintf( "Processing macro stack (forced): %s", GetText( pms->pMacro->pName ) );
						goto do_macro_command;
					}
				}
				if( ps->Command && !ps->flags.bHoldCommands && !ps->flags.bRelay )
				{
					//if( gbTrace )
					//	SystemLog( "Reading from input device..." );
					if( ps->Command->Read )
					{
						// if command processor is the datapath filter
						// the command will have been processed, and not
						// relayed through, otherwise it's data and it goes...
						// hmm would be helpful if the command was marked as TF_RELAY...
						n |= ps->Command->Read( ps->Command );
					}

					pCommand = (PTEXT)DequeLink( &ps->Command->Input );
					if( pCommand && ps->Data && ( ( pCommand->flags & TF_RELAY ) == TF_RELAY ) )
					{
						if( gbTrace )
							SystemLog( "Data is relayed." );
						EnqueLink( &ps->Data->Output, pCommand );
						continue;
					}

					if( !ps->flags.macro_input )
					{
						if( !pCommand )
						{
							if( ps->Command->flags.Closed )
							{
								if( gbTrace )
									SystemLog( "End of input, device is closed." );
								if( !DestroyDataPath( ps->Command ) )
								{
									SystemLog( "Device will not close, releasing sentience" );
									break;
								}
								continue;
							}
						}
					}
					else
					{
						lprintf( "Okay got data to result to input." );
						if( pCommand )
						{
							SetIndirect( ps->MacroInputVar, pCommand );
							// store _this result in the stored variable...
							ps->flags.macro_input = FALSE;
							pCommand = NULL;
						}
						else
						{
							if( ps->Command->flags.Closed )
							{
								DestroyDataPath( ps->Command );
								continue;
							}
						}
					}
				}
				if( pCommand ) // got command from input queue...
				{
					// _this indicates the command was sourced from a sentience...
					if( !( pCommand->flags & TF_SENTIENT ) )
						ps->flags.commanded = TRUE; // may indicate issue prompt
				}
			do_macro_command:
				if( !pCommand &&	// did not get command from command queue
					!ps->flags.macro_input /*&&  // /INPUT command has been issued...
					!ps->flags.macro_suspend*/ ) // flag prevents any macro processing
				{
					//lprintf( "%s No input command - check for a macro command...", GetText( GetName( ps->Current ) ) );
					pms = ps->CurrentMacro = (PMACROSTATE)PeekData( &ps->MacroStack );
					//SystemLog( "Checking for macro...." );
					if( ps->flags.resume_run && pms && pms->state.flags.macro_suspend )
					{
						ps->flags.resume_run = 0;
						pms->state.flags.macro_suspend = 0;
					}
					if( pms &&		 // a macro is available...
						!pms->state.flags.bInputWait && // and is not in /WAIT
						!pms->state.flags.macro_suspend )
					{
						PLIST pCommands;
						uint32_t ticks;
						//SystemLog( "Found a macro to run..." );
						//SystemLog( GetText( GetName( pms->pMacro ) ) );
						if( !pms->state.flags.macro_delay
							|| ( (ticks=GetTickCount()) >= pms->state.flags.data.delay_end ) )
						{
							if( pms->state.flags.macro_delay )
							{
								ps->CurrentMacro->state.flags.data.delay_end -= ticks;
								pms->state.flags.macro_delay = FALSE;
							}
							//SystemLog( "Getting a normal command..." );
							if( ( pCommands = pms->pMacro->pCommands ) )
							{
								if( pms->nCommand < pms->pMacro->nCommands )
								{
									//SystemLog( "Macro has commands to do..." );
									//_lprintf( 0, "pCommands->pNode[pms->nCommand=%d]=%p", pms->nCommand, pCommands->pNode[pms->nCommand] );
 									pCommand = (PTEXT)pCommands->pNode[pms->nCommand];
									ppCommand = (PTEXT*)(pCommands->pNode + pms->nCommand);
									pms->nCommand++;
								}
								else
								{
									//SystemLog( "Macro has ended... getting the last command." );
									// end macro most probably ?!
									ppCommand = (PTEXT*)(pCommands->pNode + pms->pMacro->nCommands - 1);
									pCommand = (*ppCommand);
								}
							}
							//else
							//	SystemLog( "Macro has no commands?" );
							// _this pCommand is static and must remain.
							//lprintf( "Do not free this..." );
							bFree = FALSE;
						}
						else
						{
							// hmm woke up too soon, reschedule timer.
							//SystemLog( "Woke up too soon?? huh?" );
							if( !ps->flags.scheduled )
							{
								AddTimerEx( pms->state.flags.data.delay_end - ticks, 0, TimerWake, (uintptr_t)ps );
								ps->flags.scheduled = 1;
							}
						}
					}
					else if( pms && ( pms->state.flags.bInputWait  // and is not in /WAIT
										  //|| pms->state.flags.macro_suspend
										 )
							 ) // macro suspend allows data process
					{
						// okay macro_suspend can perform exactly the same as /input
						// that is /suspend allows data processing to happen (read ps->data)
						// input also waits...
						// the result of _this code since it was implmeented originally to handle
						// /input, resets the input wait, which will not be set
						// if macro_suspend is set.
						// /suspend can be used to wait for a /on close event to wake up and resume closing the data path

						//SystemLog( "Uhmm No command..." );
						if( ps->Data ) // cannot wait if data channel is not open.
						{
							if( ps->Data->Read )
								ps->Data->Read( ps->Data );
							if( !IsQueueEmpty( &ps->Data->Input ) )
							{
								lprintf( "Queue is not empty now, we may proceed and get the data." );
								pms->state.flags.bInputWait = FALSE;
							}
							else // queue is empty...
								if( ps->Data->flags.Closed )
								{
									// go ahead and close it?
									pms->state.flags.bInputWait = FALSE;
									//DestroyDataPath( ps->Data );
									//InvokeBehavior( "Close", ps->Current, ps, NULL );
								}
						}
						else // can't wait for input that's never going to come.
						{
							pms->state.flags.bInputWait = FALSE;
						}
						// uhmm - _this has changed state, we
						// therefore need to re-evaluate for a command.
						if( !pms->state.flags.bInputWait )
						{
							continue; // re-scan from top - now we can get commands...
						}
					}
				}
			
				if( pCommand ) // got command from main queue or from a macro.
				{
					//if( gbTrace )
					//	SystemLog( "Okay finally - we have a command..." );
					if( ps->flags.commanded )
					{
						ps->flags.prompt = FALSE; // prompt invalid...
					}
					n++; // count commands processed...
					{
						PMACRO pRecording;
						// may either record macro from command stream
						// OR record from macro stream...
						if( ( !bFree && // running a macro step
							  ps->flags.command_recording ) // command entry is record...
							||( bFree &&  // running console command
								!ps->flags.command_recording ) ) // macro is recording...
						{
							pRecording = ps->pRecord;
							ps->pRecord = NULL;
						}
						else
							pRecording = NULL;

						// _this is really invoked on datapath->write
						peMe = ps->Current;
						if( ps->CurrentMacro && ps->CurrentMacro->peInvokedOn )
							ps->Current = ps->CurrentMacro->peInvokedOn;
						Process_Command( ps, ppCommand );
						ps->Current = peMe;
/*
						if( ps->pToldBy && ps->pToldBy->flags.waiting_for_someone )
						{
							lprintf( "Completed a command, waking the waiter..." );
							ps->pToldBy->flags.waiting_for_someone = 0;
							WakeAThread( ps->pToldBy );
						}
*/
						if( ps->pRecord )
						{
							if( bFree )
								ps->flags.command_recording = TRUE;
							else
								ps->flags.command_recording = FALSE;
						}
						if( ps->pRecord && pRecording )
							DebugBreak(); // hey starting ANOTHER recording.....
						if( !ps->pRecord && pRecording )
						{
							lprintf( "---------RECORD ------------" );
							ps->pRecord = pRecording; // restore...
						}
					}
					if( !bFree ) // by coincidence - was doing a macro command...
					{
						if( !ps->CurrentMacro )
						{
							//PMACROSTATE pms;
							pms = (PMACROSTATE)PeekData( &ps->MacroStack );
							if( pms )
							{
								if( gbTrace )
									SystemLog( "Macro has ended." );
								if( pms->state.flags.bFindLabel )
								{
									DECLTEXT( msg, "Label definition is missing..." );
									EnqueLink( &ps->Command->Output
												, SegAppend( SegCreateIndirect((PTEXT)&msg)
															  , SegDuplicate( pms->state.flags.data.pLabel ) ) );
								}

								if( pms->state.flags.bFindEndIf )
								{
									S_MSG( ps, "Endif definition is missing..." );
								}
								if( pms->state.flags.bFindElse )
								{
									S_MSG( ps, "ELSE definition is missing..." );
								}
								if( pms->StopEvent )
									pms->StopEvent( pms->psv_StopEvent, pms );
								if( pms->MacroEnd )
								{
									pms->MacroEnd( ps, pms );
								}
								else
								{
									pms->pMacro->flags.un.macro.bUsed = FALSE;
									if( pms->pMacro->flags.un.macro.bDelete )
									{
										DestroyMacro( ps->Current, pms->pMacro );
									}
									PopData( &ps->MacroStack );
								}
								pms = NULL;
							}
							else
							{
								S_MSG( ps, "Macro dissappeared?" );
								lprintf( "Macro dissappeared?" );
							}
						}
						//else there's a macro, it's running, and all is cool.
					}
					else // bFree is set - command input queue
					{
						LineRelease( pCommand );
					}
				}
				//else
				//	SystemLog( "No command from macro... or input." );
			}
			else
			{
				PTEXT line;
				while( ps->Command &&
						ps->Data &&
						( line = (PTEXT)DequeLink( &ps->Command->Input ) ) )
				{
					//lprintf( "Relayed output - add n..." );
					n++;
					EnqueLink( &ps->Data->Output, line );
				}
			}
			break;
		} while(1);

		if( ps->Data &&	// data channel open and...
			( !( pms = (PMACROSTATE)PeekData( &ps->MacroStack ) )
			  || pms->state.flags.macro_suspend) )// not running a macro
		{
			// automatic Data_input relay to Command_output
			//lprintf( "Starting to read the datapath... %p", pms );
			if( ps->Data->Read )
				ps->Data->Read( ps->Data );
			//lprintf( "Done with read might have a macro now... %p", PeekData( &ps->MacroStack ) );
			// if a new macro has started - mark N by 1 to indicate that
			// something was processed.... but don't eat the data
			// the macro may want it.
			// which is an intersting problem.......
			//	/trigger <some data>
			//  (data is held)
			//	 /macro ;/input %data ; will be the line that invoked the macro itself.
			if( !pms && PeekData( &ps->MacroStack ) )
				n++;
			while( !IsQueueEmpty( &ps->Data->Input ) && 
					( !( pms = (PMACROSTATE)PeekData( &ps->MacroStack ) )
					 || pms->state.flags.macro_suspend)// not running a macro
					//!PeekData( &ps->MacroStack )
				  ) // sometimes reading data can cause a macro to run....
			{
				PTEXT pIn;
				//if( gbTrace )
				//	lprintf( "Read data, no macro running, auto relay mode..." );
				n++;
				pIn = (PTEXT)DequeLink( &ps->Data->Input );
				EnqueLink( &ps->Command->Output, pIn );
			}
			{
				PDATAPATH _pdp; // the path we started with
				while( ( _pdp = ps->Data )
						&& IsQueueEmpty( &_pdp->Input )
						&& _pdp->flags.Closed )
				{
					if( gbTrace )
						lprintf( "datapath is closed... closing..." );
					DestroyDataPath( ps->Data );
					if( _pdp == ps->Data )
					{
						lprintf( "Macro was busy, try getting a command, it's stuck from closing..." );
						break;
					}
				}
				if( _pdp && !_pdp->flags.Closed )
					InvokeBehavior( "Close", ps->Current, ps, NULL );
			}
		}

		// data input may have occured, so we handle data input, command
		// output...
		// a command might have been issued which removed _this command queue...
		{
			PDATAPATH pdp = ps->Command;
			while( pdp )
			{
				LOGICAL bEmpty;
				while( !( bEmpty = IsQueueEmpty( &pdp->Output ) ) || pdp->flags.bWriteAgain )
				{
					if( bEmpty && pdp->flags.bWriteAgain )
						pdp->flags.bWriteAgain = 0;
					else
						pdp->flags.bWriteAgain = 1;
					n++;
					if( gbTrace )
					{
						if( bEmpty )
							xlprintf(LOG_NOISE+1)( "%s(%s)command output empty, call write.", GetText(GetName( ps->Current )), GetText( pdp->pName ) );
						//else
						//	xlprintf(LOG_NOISE+1)( "%s(%s)command output not empty, call write.", GetText(GetName( ps->Current )), GetText( pdp->pName ) );
					}
					if( pdp->Write )
					{
						// write has a result something like
						// the number of messages written?
						if( !pdp->Write( pdp ) )
							//lprintf( "Will call write one more time? : %s", pdp->flags.bWriteAgain?"Yes":"No" );
							break;
					}
					else
					{
						xlprintf(LOG_NOISE+1)( "Outbound command path has no write method.  Data will not move, releasing data." );
						LineRelease( (PTEXT)DequeLink( &pdp->Output ) );
					}
				}
				pdp = pdp->pPrior;
			}
		}
		{
			PDATAPATH pdp = ps->Data;
			while( pdp )
			{
				LOGICAL bEmpty;
				//Log( "Moving data on data path out..." );
				while( (!(bEmpty=IsQueueEmpty( &pdp->Output )) || pdp->flags.bWriteAgain ) )
				{
					if( bEmpty && pdp->flags.bWriteAgain )
						pdp->flags.bWriteAgain = 0;
					else
						pdp->flags.bWriteAgain = 1;
					n++;
					if( gbTrace )
						if( bEmpty )
							xlprintf(LOG_NOISE+1)( "%s(%s)Data output empty, call write.", GetText(GetName( ps->Current )), GetText( pdp->pName ) );
						else
							xlprintf(LOG_NOISE+1)( "%s(%s)Data output not empty, call write.", GetText(GetName( ps->Current )), GetText( pdp->pName ) );
					if( pdp->Write )
					{
						if( !pdp->Write( pdp ) )
							// if I hit any single write, then
							// that's all great and groovy?
							break;
					}
					else
					{
						xlprintf(LOG_NOISE+1)( "Outbound data path has no write method.  Data will not move, releasing data." );
						LineRelease( (PTEXT)DequeLink( &pdp->Output ) );
					}
				}
				pdp = pdp->pPrior;
			}
		}

		if( !n &&
			 !ps->flags.bRelay && // if it's clear relay no prompt noise please
			 !ps->flags.prompt && // needs a prompt - ie hasn't prompted yet.
			 !ps->flags.no_prompt && // prompt not supressed
			 !ps->Data &&				// don't prompt if datapath is open
			 ps->Command &&
			 ps->Command->Type	// command path is open....
		  )  // and is using it's own queue...
		{
			ps->flags.prompt = TRUE;
			// commanded is - issued a prompt which causes a prompt
			ps->flags.commanded = FALSE; // uncommand ... in case was /prompt
			if( ps->Command->CommandInfo &&
				 ps->Command->CommandInfo->Prompt )
			{
				ps->Command->CommandInfo->Prompt( ps->Command );
			}
			else
				prompt( ps );
			n++;
		}
		total += n;

		StepNextSentient();

		ps->ProcessingLock = 0;
	}
	return total;
}

//--------------------------------------------------------------------------

typedef struct process_thread_tag
{
	PTHREAD thread;
	DeclareLink( struct process_thread_tag );
} PROCESS_THREAD, *PPROCESS_THREAD;

static uint32_t updating_queues;
static PPROCESS_THREAD processing, sleeping;

uintptr_t CPROC ObjectProcessThread( PTHREAD thread )
{
	int delay = 0, n;
	PPROCESS_THREAD pMyThread = (PPROCESS_THREAD)GetThreadParam( thread );
	// some of these threads may have loaded DLLs
	// for now it is safe to assume that they did not - and do not
	// have to unload the plugins themselves...
	// but the DLL must remain for the UnloadPlugins() to work...
	while( !pMyThread->thread )
		Relinquish(); // think _this might happen...
	while( !gbExitNow )
	{
		if( ( n = ProcessSentients( GetThreadID( thread ) ) ) )
		{
			delay += n;
			if( delay > 500 ) // 500 instructions on all sentients...
			{
				delay = 0;
				Relinquish(); // delay every 500 instructions
			}
		}
		else
		{
			//Log( "Sleeping forever... should be events laying around..." );
			while( LockedExchange( &updating_queues, 1 ) )
				Relinquish();
			RelinkThing( sleeping, pMyThread );
			updating_queues = 0;
			WakeableSleep(5000 /*SLEEP_FOREVER*/); // 20 cycles a second idle...
		}
	}
	Log( "Found out we're to exit - so we do." );
	Release( UnlinkThing( pMyThread ) );
	return 0;
}
//#undef WakeAThread
//CORE_PROC( void, WakeAThread )( PSENTIENT ps )
//{
//	WakeAThreadEx( ps DBG_SRC );
//}

CORE_PROC( void, WakeAThreadEx )( PSENTIENT ps DBG_PASS )
{
	// might be smart one say to avoid
	// waking up threads to process objects which
	// are already running.  But, a thread could be in a
	// state such that it has done no work, and results with
	// no processing, which causes its thread to sleep.
	// Also -  as long as a thread is awake and is not locked
	// in a process processing, then the thread does not need to awaken.
	// I guess some flag - away in command - or something needs to be
	// set to determine whether a thread needs to be created/awakened.
	// well - _this should for the most part reduce executing threads, and
	// threads go to sleep permanently now, so a thread's existance is
// low impact.
	//lprintf( "Waking a thread..."DBG_FORMAT DBG_RELAY );
	if( ps )
	{
				ps->ProcessLock = 0; // someone woke this up.  Do something.
	}
	if( sleeping )
	{
		while( LockedExchange( &updating_queues, 1 ) )
			Relinquish();
		{
			PPROCESS_THREAD sleeper = sleeping;
			if( sleeper )
			{
				RelinkThing( processing, sleeper );
				WakeThreadEx( sleeper->thread DBG_RELAY );
			}
			updating_queues = 0;
		}
	}
	else
	{
		PPROCESS_THREAD thread = New( PROCESS_THREAD );
		MemSet( thread, 0, sizeof( PROCESS_THREAD ) );
		while( LockedExchange( &updating_queues, 1 ) )
			Relinquish();
		LinkThing( processing, thread );
		updating_queues = 0;
		thread->thread = ThreadTo( ObjectProcessThread, (uintptr_t)thread );
	}
}

CORE_CPROC( void, TimerWake )( uintptr_t psv )
{
	PSENTIENT ps = (PSENTIENT)psv;
	if( ps )
		ps->flags.scheduled = 0;
	WakeAThread( NULL );
}

CORE_PROC( void, ExitNexus )( void )
{
	gbExitNow = TRUE;
	WakeThread( pMainThread );
}

//------------------------------------------------------------------------
// _this section of code provides a default console device using
// FILE * input - intended for use on UNIX systems...
//------------------------------------------------------------------------
#ifdef __LINUX__
#ifndef WIN32

//----------------------------------------------------------------------

PTEXT get_line(FILE *source) /*FOLD00*/
{
	#define WORKSPACE 1024  // character for workspace
	PTEXT workline=(PTEXT)NULL, tmp;
	int length;
	TEXTCHAR *data;
	if( !source )
		return NULL;
	do
	{
		// create a workspace to read input from the file.
		SegAppend(workline,tmp=SegCreate(WORKSPACE));
		workline=tmp;
		data = GetText(workline);
		// read a line of input from the file.
		if( !fgets( data, WORKSPACE, source) ) // if no input read.
		{
			if (PRIORLINE(workline)) // if we've read some.
			{
				PTEXT t;
				workline=PRIORLINE(workline); // go back one.
				SegBreak(t = NEXTLINE(workline));
				LineRelease(t);  // destroy the current segment.
			}
			else
			{
				LineRelease(workline);				// destory only segment.
				workline = NULL;
			}
			break;  // get out of the loop- there is no more to read.
		}
		workline->data.size = length=strlen(data);  // get the length of the line.
	}
	while( data[length-1] != '\n' ); //while not at the end of the line.
	if( workline )  // if I got a line, and there was some length to it.
		SetStart( workline );	// set workline to the beginning.
	return( workline );		// return the line read from the file.
}

//----------------------------------------------------------------------

int bOutputPause = FALSE;
int nLines = 2500;
int bInputPause = FALSE;
int bInputPending = FALSE;
void UserInputThread( void )
{
	PTEXT command, p;
#include <sys/types.h>
#include <sys/time.h>
	fd_set rfds;
	struct timeval tval;

	while( TRUE )
	{
		while( bInputPause )
			Sleep( 50 );
		bInputPending = TRUE;
		command = (PTEXT)get_line( stdin );
		bInputPending = FALSE;
		if( !command ) // may be end of file... may be termination...
			continue; 
		if( bOutputPause )
		{
			bOutputPause = FALSE;
			LineRelease( command );
			continue;
		}
		p = burst( command );
		if( p )
		{
//		 printf( "enquing command...\n");
			EnqueLink( &global.PLAYER->Command->Input, p );
		}
		LineRelease( command );
	}
}

void OutputText( PTEXT pText )
{
	if( pText && GetTextSize( pText ) )
	{
		lprintf( "%s", GetText( pText ) );
	}
	else
	{
		lprintf( "\n" );
	}
}

int ConsoleWrite( PDATAPATH pdp )
{
	PTEXT pCommand;
	int lines = 0;
	static int bOutOfPause;
	if( bOutputPause )
	{
		bOutOfPause = TRUE;
		return 1;
	}

	while( pCommand = DequeLink( &pdp->Output ) )
	{
		PTEXT pOutput;
		if( !( bOutOfPause ) )
		{
			if( !(pCommand->flags & TF_NORETURN) )
				OutputText( NULL );
		}
		else
			bOutOfPause = FALSE;
		pOutput = BuildLine( pCommand );
		OutputText( pOutput );
		LineRelease( pOutput );
		LineRelease( pCommand ); // after successful output no longer need _this...

		lines++;
		if( lines >= nLines )
		{
			DECLTEXT( msg95, "Press any key to continue" );
			DECLTEXT( msg, "Press return to continue" );
			// double spaces if prior input was NO_RETURN with return postfix
			// although it might not have any return and.... BLAH
			OutputText( NULL ); // newline prefix...
			OutputText( (PTEXT)&msg );
			bOutputPause = TRUE;
			break;
		}
	}
	fflush( stderr );
	fflush( stdout );
	return 1;
}

#endif
#endif

CORE_PROC( void, S_MSG )( PSENTIENT ps, CTEXTSTR msg, ... ) 
{ 
	PVARTEXT pvt;	  
	va_list args;
	va_start( args, msg );
	pvt = VarTextCreate(); 
	vvtprintf( pvt, msg, args ); 
	EnqueLink(&(ps)->Command->Output,VarTextGet( pvt )); 
	VarTextDestroy( &pvt ); 
}
CORE_PROC( void, D_MSG )( PDATAPATH pd, CTEXTSTR msg, ... ) 
{
	PVARTEXT pvt;
	va_list args;
	va_start( args, msg );
	pvt = VarTextCreate();
	vvtprintf( pvt, msg, args );
	EnqueLink(&(pd)->Output,VarTextGet( pvt ));
	WakeAThread( NULL );
	VarTextDestroy( &pvt );
}
CORE_PROC( void, Q_MSG )( PLINKQUEUE *po, CTEXTSTR msg, ... ) 
{ PVARTEXT pvt;	  va_list args;
	 va_start( args, msg );
pvt = VarTextCreate(); vvtprintf( pvt, msg, args ); EnqueLink((po),VarTextGet( pvt )); VarTextDestroy( &pvt ); }

void NOTHING( uintptr_t n )
{
}

#ifdef __LINUX__
#define MAX_PROCESSES 2
#else
#define MAX_PROCESSES 16
#endif

#ifdef _WIN32
__declspec(dllexport)
#endif
int IsConsole;
#ifdef _WIN32
__declspec(dllexport)
#endif
int b95;
static int Began = FALSE;
CTEXTSTR load_path;
CTEXTSTR core_load_path;


PUBLIC( void, Startup)( TEXTCHAR *lpCmdLine )
{
	static int Started;
	if( !Started )
	{
		// _this eliminates a problem on my sorcerer linux box...
		// _this shouldn't have to be here... but somewhere the thread
		// creating the network thread creating the timer thread results
		// in a zombie thread
		//AddTimerEx( 0, 0, NOTHING, 0 );
		//TEXTCHAR pMyPath[256];
		TEXTCHAR pMyPluginPath[256];
		TEXTSTR pluginPath;
		Started = 1;

		for( ; lpCmdLine && lpCmdLine[0] == ' '; lpCmdLine++ );

#ifdef WIN32
		{
			HANDLE hToken, hProcess;
			TOKEN_PRIVILEGES tp;
			OSVERSIONINFO osvi;
			DWORD dwPriority;
			osvi.dwOSVersionInfoSize = sizeof( osvi );
			GetVersionEx( &osvi );
			if( osvi.dwPlatformId  == VER_PLATFORM_WIN32_NT )
			{
				b95 = FALSE;
				// allow shutdown priviledges....
				// wonder if /shutdown will work wihtout _this?
				if( DuplicateHandle( GetCurrentProcess(), GetCurrentProcess()
										 , GetCurrentProcess(), &hProcess, 0
										 , FALSE, DUPLICATE_SAME_ACCESS  ) )
					if( OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
					{
						tp.PrivilegeCount = 1;
						if( LookupPrivilegeValue( NULL
														, SE_SHUTDOWN_NAME
														, &tp.Privileges[0].Luid ) )
						{
							tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
							AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
						}
						else
							GetLastError();
					}
					else
						GetLastError();
				else
					GetLastError();
				CloseHandle( hProcess );
				CloseHandle( hToken );

				dwPriority = NORMAL_PRIORITY_CLASS;
			}
			else
			{
				b95 = TRUE;
				dwPriority = NORMAL_PRIORITY_CLASS;
			}
			//SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_IDLE );
			SetPriorityClass( GetCurrentProcess(), dwPriority );
		}
		//if( b95 )
		//	Log( "Running on a windows 9x type system..." );
		//else
		//	Log( "Running on a windows NT type system..." );
		{
			TEXTCHAR pMyLoadPath[256];
			TEXTCHAR *truncname;
			HMODULE mylib = 
#ifdef TARGETNAME
				LoadLibrary( TARGETNAME );
#else
				NULL;
#endif
			if( !GetModuleFileName( mylib, pMyLoadPath, sizeof( pMyLoadPath ) ) )
			{
				lprintf( "Not compiled correctly!  You didn't set targetname to my name, so I don't know where I'm running." );
				StrCpy( pMyLoadPath, "." );
			}
			// this is a warning in C (?) ...
			// in C++ there's two different functions with dfiferent results.
			// it's illegal realy to use a typecast... but it's a warning.
			/// pathrchr returns a pointer within the string... since loadpath is variable
			// the result should be variable.
			truncname = (TEXTCHAR*)pathrchr( pMyLoadPath );
			if( truncname )
				truncname[0] = 0;
			core_load_path = StrDup( pMyLoadPath );
		}
#else
		nice(20);
		Log( "Running on a unix like type system..." );
#endif
		InitSpace( lpCmdLine?lpCmdLine:"" ); // no command line.
#ifdef __ANDROID__
		snprintf( pMyPluginPath, sizeof( pMyPluginPath ), "%s", GetProgramPath() );
#else
		snprintf( pMyPluginPath, sizeof( pMyPluginPath ),  ",/lib/SACK/plugins" );
#endif
		pluginPath = ExpandPath( pMyPluginPath );
		Log1( "Loading plugins from: %s", pMyPluginPath );
		LoadPlugins( pluginPath );
		ReleaseEx( pluginPath DBG_SRC );
		if( StrCmp( pMyPluginPath, core_load_path ) )
		{
			Log1( "Loading plugins from: %s", core_load_path );
			LoadPlugins( core_load_path );
		}

		//GetCurrentPath( pMyPath, sizeof( pMyPath) );
		//Log1( "Setting internal path to for scripts: %s", pMyPath );

		RegisterCommands( NULL, NULL, 0 );

		// if during the course of a plugin, the master operator
		// was not created, we have to have something aware to be able
		// to process commands... let's wake up the void :)
		if( !global.PLAYER )
		{
			global.PLAYER = CreateAwareness( global.THE_VOID );
			UnlockAwareness( global.PLAYER );

			//DoCommandf( global.PLAYER, "/debug" );
			//DoCommandf( global.PLAYER, "/echo /script macros" );
			DoCommandf( global.PLAYER, "/script macros" );
		}
		//Log( "Start one sentience..." );
		WakeAThread( NULL );
		//Log( "Who Am I?" );
		pMainThread = MakeThread();
		Began = TRUE;
	}
}

void Cleanup( void )
{
	static int cleaned;
	if( !cleaned )
	{
		cleaned = 1;
		pMainThread = NULL;
		//UnmakeThread();
		Log( "Allowing threads to exit gracefully - wake them up first." );
		while( sleeping )
		{
			WakeAThread( NULL );
			Relinquish();
		}
		lprintf( "Woke all sleepers." );
		// allow object processing threads to exit gracefully...
		while( processing )
			Relinquish();

		if( Began )
		{
			DestroyMacro( NULL, global.script );
			global.script = NULL;
			{
				PENTITY pe;
				INDEX idx;
				do
				{
					LIST_FORALL( global.THE_VOID->pContains, idx, PENTITY, pe )
					{
						if( pe != global.THE_VOID )
						{
							DestroyEntity( pe );
							break;
						}
					}
				}
				while( pe );
			}
			pullout( global.THE_VOID, global.THE_VOID );
			DestroyEntity( global.THE_VOID );
			// for all Sentients, close all datapaths....
			{
				PSENTIENT ps;
				ps = AwareEntities;
				while( ps )
				{
					PSENTIENT Next = ps->Next;
					while( ps->Data )
						ps->Data = DestroyDataPath( ps->Data );
					while( ps->Command )
						ps->Command = DestroyDataPath( ps->Command );
					DestroyAwareness( ps );
					ps = Next;
				}
			}
			{
				PTEXT text;
				INDEX idx;
				LIST_FORALL( global.behaviors, idx, PTEXT, text )
				{
					LineRelease( text );
				}
				DeleteList( &global.behaviors );
			}

			SystemLogTime( SYSLOG_TIME_DISABLE );
			//DebugDumpMem();
			// should be able to unload plugins and then close the paths...
			// unload will close all datapaths now.
			{
				extern void UnloadPlugins( void );
				//UnloadPlugins();
			}
			ProtectLoggedFilenames( TRUE );

			SystemLogTime( SYSLOG_TIME_DISABLE );
			//DebugDumpMem();
		}
	}
}

static uintptr_t CPROC DekwareThread( PTHREAD thread )
{
	// now wait forever.
	while(!gbExitNow) // main program's idle loop - do nothing other than wait...
	{
		Log( "Goodnight!" );
		WakeableSleep( SLEEP_FOREVER );
	}
	//Cleanup();
	return 0;
}

PRELOAD( BeginDekwareSpace )
{
	SetMinAllocate( sizeof( TEXT ) + 4 );
	// if dekware is the launcher it will be doing arguments
	//Startup( NULL );
	//ThreadTo( DekwareThread, 0 );
}

ATEXIT( EndDekwareSpace )
{
	//Cleanup();
}

#ifdef __cplusplus
extern "C"
#endif
#ifdef _WIN32
__declspec(dllexport)
#endif
// set bCommand if was run from a console window proc....
int CPROC Begin( TEXTCHAR *lpCmdLine, int bCommand )
{
	// console determines some internal variables to pass to startup scripts for which console to launch
	IsConsole = bCommand;
	// we may have command line arguments; call startup
	Startup( strchr( lpCmdLine, ' ' ));

	if( !StrStr( lpCmdLine, "-tsr" ) )
	{
		// now wait forever.
		while(!gbExitNow) // main program's idle loop - do nothing other than wait...
		{
			Log( "Goodnight!" );
			WakeableSleep( SLEEP_FOREVER );
		}
		lprintf( "TODO: Work on Cleanup()..." );
		//Cleanup();
	}
	return 0;
}


#ifdef __cplusplus_cli
// build a .NET extension...
namespace dekware
{
	static public ref class core
	{
	public: static core() 
		{
			//SetSystemLog( SYSLOG_AUTO_FILE, 0 );
			Startup( NULL );

			//Begin( "", FALSE );
		}
	static TEXTCHAR *GetString( System::String ^str )
{
	// Pin memory so GC can't move it while native function is called
	pin_ptr<const wchar_t> wch = PtrToStringChars(str);

	// Conversion to TEXTCHAR* :
	// Can just convert wchar_t* to TEXTCHAR* using one of the 
	// conversion functions such as: 
	// WideCharToMultiByte()
	// wcstombs_s()
	// ... etc
	size_t convertedChars = 0;
	size_t  sizeInBytes = ((str->Length + 1) * 2);
	errno_t err = 0;
	TEXTCHAR	 *ch = NewArray( TEXTCHAR, sizeInBytes);

	err = wcstombs_s(&convertedChars, 
						  ch, sizeInBytes,
						  wch, sizeInBytes);
	//if (err != 0)
	//	printf_s("wcstombs_s  failed!\n");
	return ch;
	 //printf_s("%s\n", ch);
}

	};




public ref class Entity: System::Object
{
public:
	PENTITY pe;
public:
//	friend class Sentience;
	Entity::Entity(System::String ^name)
	{
		PTEXT pName = SegCreateFromText( core::GetString( name ) );
		pe = CreateEntity( pName );
	}
};

public ref class DataPath 
{
public: 
	
	//PDATAPATH pdp;

public: DataPath( )
		{
		}
public: DataPath( PSENTIENT ps, System::String ^str )
	{
//CORE_PROC( PDATAPATH, CreateDataPath	)( PDATAPATH *ppWhere, int nExtra );
//#define CreateDataPath(where, pathname) (P##pathname)CreateDataPath( where, sizeof(pathname) - sizeof( DATAPATH ) )
//		pdp = CreateDataPath( &sentient->ps->Command, CONSOLE_INFO );
	}
};

public ref class Sentience
{
public:
	PSENTIENT ps;
	//List<DataPath> extra;
	Sentience( Entity ^entity )
	{
		ps = CreateAwareness( entity->pe );
	}
	PSENTIENT GetNative( void )
	{
		return ps;
	}

};



} // namespace
#endif

// restore defintiion for amalgamation...
#define CreateDataPath(where, pathname) (P##pathname)CreateDataPath( where, sizeof(pathname) - sizeof( DATAPATH ) )
