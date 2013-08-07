
#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "space.h"

#include "my_ver.h"

//-----------------------------------------------------------------------------
// Command Pack 1 contains basic object functions, and VERSION
//-----------------------------------------------------------------------------


void Unknown(PLINKQUEUE *Output, PTEXT Text)
{
    PVARTEXT vt;
    vt = VarTextCreate( );
	vtprintf( vt, WIDE("%s is an unknown object"), GetText( Text ) );
	EnqueLink( Output, VarTextGet( vt ) );
	VarTextDestroy( &vt );
}
//--------------------------------------------------------------------------

PTEXT WriteListNot( PLIST pSource
                  , PENTITY pNot
                  , PTEXT pLeader
                  , PLINKQUEUE *Output )
{
	int didone = FALSE;
	INDEX idx;
	PENTITY pe;
	PTEXT out;
	size_t length;
	typedef struct unique_tag {
		PTEXT name;
		_32 count;
	} *unique_name, new_unique_name;
	PDATALIST unique = NULL;
	TEXTCHAR tmp[12];
	unique_name name = NULL;

	unique = CreateDataList( sizeof( new_unique_name ) );
	length = 0;
	LIST_FORALL( pSource, idx, PENTITY, pe )
	{
		{
			INDEX idx2;
			if( pe == pNot )
				continue; // skip this guy
			DATA_FORALL( unique, idx2, unique_name, name )
			{
				if( SameText( name->name, GetName( pe ) ) == 0 )
				{
					name->count++;
					break;
				}
			}
		}
		if( !name )
		{
			new_unique_name newname;
         //lprintf( WIDE("Adding name %s"), GetText( GetName( pe ) ) );
			newname.name = GetName( pe );
			newname.count = 1;
			AddDataItem( &unique, &newname );
		}
	}

	DATA_FORALL( unique, idx, unique_name, name )
	{
		length += GetTextSize( name->name );
		if( name->count > 1 )
		{
			length += 2 + snprintf( tmp, sizeof(tmp), WIDE("%ld"), name->count );
		}
		if( didone )
			length += 2;
		else
			didone = TRUE;
	}

	if( length )
	{
		length++; // one for the ending period.
		out = SegCreate( length );
		out->data.size = 0; // clear length
		didone = FALSE;
		DATA_FORALL( unique, idx, unique_name, name )
		{
			out->data.size += snprintf( out->data.data + out->data.size, length*sizeof(TEXTCHAR), name->count>1?WIDE("%s%s[%d]"):WIDE("%s%s")
                                    , (didone)?WIDE(", "): WIDE("")
											 , GetText( name->name )
											 , name->count );
			didone = TRUE;
		}
		if( didone )
			out->data.size += snprintf( out->data.data + out->data.size, length*sizeof(TEXTCHAR), WIDE(".") );
		DeleteDataList( &unique );
	}
	else
		out = SegCreateFromText( WIDE("Nothing.") );
	pLeader = SegAppend( pLeader, out );
	if( Output )
	{
		EnqueLink( Output, pLeader );
		return NULL;
	}
	return pLeader;
//   return pLeader;
}

//--------------------------------------------------------------------------

void RoomsNear(PENTITY pe, PLINKQUEUE *Output )
{
   WriteList( FindContainer( pe )->pAttached
				, SegCreateFromText( WIDE("Rooms: ") ), Output );
}

//--------------------------------------------------------------------------

void ItemsNear(PENTITY pe, PLINKQUEUE *Output )
{
   WriteListNot( FindContainer( pe )->pContains, pe
					, SegCreateFromText( WIDE("Items: ") ), Output );
}

//--------------------------------------------------------------------------

void Possesses(PENTITY pe, PLINKQUEUE *Output )
{
	PLIST pAttached;
	PTEXT out;
	size_t len;
	out = SegCreate( len = GetTextSize( GetName(pe) ) + 13 );
	snprintf( out->data.data, len*sizeof(TEXTCHAR), WIDE("%s is holding: "), GetText( GetName(pe) ) );
	pAttached = BuildAttachedList( pe );
	WriteList( pAttached, out, Output );
	DeleteList( &pAttached );
}

//--------------------------------------------------------------------------

void Contents(PENTITY pe, PLINKQUEUE *Output )
{
	PTEXT out;
	size_t len;
	out = SegCreate( len = GetTextSize( GetName(pe) ) + 11 );
	snprintf( out->data.data, len*sizeof(TEXTCHAR), WIDE("%s contains: "), GetText( GetName(pe) )) ;
	WriteList( pe->pContains, out, Output );
}

//--------------------------------------------------------------------------

void Possessions(PENTITY pe, PLINKQUEUE *Output)
{
    Possesses( pe, Output );
    Contents( pe, Output );
}

//--------------------------------------------------------------------------

void Look( PLINKQUEUE *Output, PSENTIENT ps, PTEXT pObj )
{
   PTEXT _params = pObj;
   PTEXT param;
   if( param = GetParam( ps, &pObj ) )
   {
      if( TextLike( param, WIDE("in") ) )
		{
			// pobject is now the next token... sorta
         param = GetParam( ps, &pObj );
         if( param )
         {
            PENTITY pe;
            int FoundAs;
            pe = (PENTITY)FindThing( ps, &param, ps->Current, FIND_GRABBABLE, &FoundAs );
            if( pe )
            {
               Contents( pe, Output );
            }
            else
            {
               Q_MSG( Output, WIDE("Cannot see %s around here."), GetText( param ) );
            }
         }
      }
      else if( TextLike( param, WIDE("on") ) )
		{
			// pobject is now the next token... sorta
         param = GetParam( ps, &pObj );
         if( param )
         {
            PENTITY pe;
            int FoundAs;
            pe = (PENTITY)FindThing( ps, &param, ps->Current, FIND_GRABBABLE, &FoundAs );
            if( pe )
            {
               Possesses( pe, Output );
            }
            else
            {
               Q_MSG( Output, WIDE("Cannot see %s around here."), GetText( param ) );
            }
         }
		}
      else
		{
         int FoundAs;
         PENTITY pe;
         pe = (PENTITY)FindThing( ps, &_params, ps->Current, FIND_VISIBLE, &FoundAs );
         if( pe )
         {
            if( GetName( pe ) )
               EnqueLink( Output, SegDuplicate( GetName( pe ) ) );
            if( pe->pDescription )
               EnqueLink( Output, SegDuplicate( pe->pDescription ) );
            Possesses( pe, Output );
         }
         else
			{
				if( !ps->CurrentMacro )
				{
					DECLTEXT( msg, WIDE("Cannot see that around here.") );
					EnqueLink( Output, &msg );
				}
         }
      }
   }
   else
   {
		PTEXT room, desc;
      xlprintf(LOG_NOISE)( WIDE("doing look command...") );
      room = GetName( FindContainer( ps->Current ) );
      if( room )
         EnqueLink( Output, SegDuplicate( room ) );
      //desc = GetDescription( FindContainer( ps->Current ) );
      desc = NULL;
      if( desc )
         EnqueLink( Output, SegDuplicate( desc ) );
      ItemsNear( ps->Current, Output );
      RoomsNear( ps->Current, Output );
   }
}

enum GrabResult {
	GRAB_RESULT_NO_OBJECT
	  , GRAB_RESULT_FOUND_ON // already on the current
	  , GRAB_RESULT_FOUND_IN // in the current
	  , GRAB_RESULT_FOUND_NEAR // around the current
	  , GRAB_RESULT_FAILED_BAG // container to get object from not found
	  , GRAB_RESULT_FAILED_IN_BAG // object is not in container
     , GRAB_RESULT_FOUND_IN_BAG // successfully grabbed from the bag
} ;

//--------------------------------------------------------------------------
int Grab(PENTITY pEntity, PTEXT info
			, PENTITY *pOldRoom
			, PENTITY *pNewAttached
			, PENTITY *pThing
			, PTEXT from)
{
   /* takes a name for a parameter, and tries to find the object specified to
      take and put into your hand... first search Location(pEntity) then
      search creation space. */
	PENTITY pe;
	int FoundAt;
	if( from )
	{
   		PENTITY pe2;
	   pe = (PENTITY)FindThing( NULL, &from, pEntity, FIND_GRABBABLE, &FoundAt );
	   if( !pe )
	   	return GRAB_RESULT_FAILED_BAG;
		pe2 = (PENTITY)FindThing( NULL, &info, pe, FIND_IN, &FoundAt );
		if( !pe2 )
			return GRAB_RESULT_FAILED_IN_BAG;

      if( pe2->pWithin )
  	      pullout( *pOldRoom = pe2->pWithin, pe2 );
		attach( *pNewAttached = pEntity, pe2 );
		*pThing = pe2;
		return GRAB_RESULT_FOUND_IN_BAG;
	}
	else
	{
		pe = (PENTITY)FindThing( NULL, &info, pEntity, FIND_GRABBABLE, &FoundAt );
   		if( pe )
		{
			//lprintf( WIDE("Result found at %d"), FoundAt );
			switch ( FoundAt )
			{
			case FIND_ON:
				return GRAB_RESULT_FOUND_ON;
			case FIND_IN:
			case FIND_WITHIN:
   	      pullout( *pOldRoom = pEntity, pe );
      	   attach( *pNewAttached = pEntity, pe );
				*pThing = pe;
				return GRAB_RESULT_FOUND_IN;
			case FIND_NEAR:
				{
					PENTITY mount_point; // the place the related attached objects are.
					*pOldRoom = FindContainerEx( pe, &mount_point );
					lprintf( WIDE("Finding container of %s is %s and mount point is %s")
							 , GetText( GetName( pe ) )
							 , GetText( GetName( *pOldRoom ) )
							 , GetText( GetName( mount_point ) ) );
					pullout( *pOldRoom, mount_point );
					attach( *pNewAttached = pEntity, pe );
					*pThing = pe;
				}
	         return GRAB_RESULT_FOUND_NEAR;
			}
   	}
   }
   return GRAB_RESULT_NO_OBJECT;
}

//--------------------------------------------------------------------------

int Store( PENTITY pEntity, PTEXT what, PTEXT into )
{
   PENTITY pe, pInto;
   pe = (PENTITY)FindThing( NULL, &what, pEntity, FIND_ON, NULL );
   if( pe )
   {
      pInto = pEntity; // default remove from held, store
      if( into )
      {
      	// if can find the thing visible, store in that...
         pInto = (PENTITY)FindThing( NULL, &into, pEntity, FIND_VISIBLE, NULL );
         if( !pInto )
         	return -1;
      }
      if( pInto == pe )
      {
      	return -2;
      }
      detach( pEntity, pe );
      putin( pInto, pe );
      if( pe->pControlledBy )
			InvokeBehavior( WIDE("store"), pe, pe->pControlledBy, NULL );
      if( pInto->pControlledBy )
			InvokeBehavior( WIDE("insert"), pe, pInto->pControlledBy, NULL );
      if( pEntity->pControlledBy )
			InvokeBehavior( WIDE("place"), pe, pEntity->pControlledBy, NULL );

      return 1;
   }
	return 0;
}   


//--------------------------------------------------------------------------

PENTITY Attach( PSENTIENT ps, PTEXT info1, PTEXT info2)
{
    PENTITY pe, pe2;
    PVARTEXT vt;
    vt = VarTextCreate( );
	 pe = (PENTITY)FindThing( ps, &info1, ps->Current, FIND_ON, NULL );
	 if( !pe )
	 {
		 vtprintf( vt, WIDE("%s is not holding %s."),
					 GetText( GetName( ps->Current ) ), GetText(info1) );
		 EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		 VarTextDestroy( &vt );
		 return NULL;
	 }
	 pe2 = (PENTITY)FindThing( ps, &info2, ps->Current, FIND_ON, NULL );
	 if( !pe2 )
	 {
		 vtprintf( vt, WIDE("%s is not holding %s."),
					 GetText( GetName( ps->Current ) ), GetText(info2) );
		 EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		 VarTextDestroy( &vt );
		 return NULL;
	 }
	 detach( ps->Current, pe );
	 attach( pe, pe2 );
	 return pe;
}

//--------------------------------------------------------------------------

PENTITY Detach( PSENTIENT ps, PTEXT info1, PTEXT info2)
{
    PENTITY pe, pe2;
    PVARTEXT vt;
    vt = VarTextCreate( );
   pe = (PENTITY)FindThing( ps, &info1, ps->Current, FIND_VISIBLE, NULL );
   if( !pe )
   {
      vtprintf( vt, WIDE("%s cannot see %s."),
               GetText( GetName( ps->Current ) ), GetText(info1) );
      EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
      VarTextDestroy( &vt );
      return NULL;
   }
   pe2 = (PENTITY)FindThing( ps, &info2, pe, FIND_ON, NULL );
   if( !pe2 )
   {
      vtprintf( vt, WIDE("%s was not attached to %s."),
               GetText( info2 ), GetText(info1) );
      EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
      VarTextDestroy( &vt );
      return NULL;
   }
   detach( pe, pe2 );
   attach( ps->Current, pe2 );
   return pe;
}

//--------------------------------------------------------------------------

PENTITY Join( PLINKQUEUE *Output, PSENTIENT ps, PTEXT info1)
{
    PENTITY pe;
    PVARTEXT vt;
    vt = VarTextCreate( );
   pe = (PENTITY)FindThing( ps, &info1, ps->Current, FIND_VISIBLE, NULL );
   if( !pe )
   {
      vtprintf( vt, WIDE("%s cannot see %s."),
               GetText( GetName( ps->Current ) ), GetText(info1) );
      EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
      VarTextDestroy( &vt );
      return NULL;
   }
   if( pe->pWithin )
      pullout( pe->pWithin, pe );
   attach( ps->Current, pe );
   return pe;
}

//--------------------------------------------------------------------------

PENTITY Enter( PLINKQUEUE *Output, PSENTIENT ps, PTEXT info)
{
	PENTITY pe;
	PVARTEXT vt;
   int bFoundNear = 1;
	vt = VarTextCreate( );
	pe = (PENTITY)FindThing( ps, &info, ps->Current, FIND_NEAR, NULL );
	if( !pe )
	{
		pe = (PENTITY)FindThing( ps, &info, ps->Current, FIND_AROUND, NULL );
      bFoundNear  = 0;
	}
	if (pe && ( pe != ps->Current->pWithin ) )
	{
		PMACROSTATE pNewMS = NULL;
		PENTITY pRoom;
		if( !ps->Current->pWithin )
		{
			DECLTEXT( msg, WIDE("Sorry, don't know how to detach myself") );
			EnqueLink( Output, (PTEXT)&msg );
			return NULL;
		}
		else
			pullout( pRoom = ps->Current->pWithin, ps->Current );
		putin( pe, ps->Current );
		if( bFoundNear )
		{
         if( pe->pControlledBy )
				InvokeBehavior( WIDE("enter"), ps->Current, pe->pControlledBy, NULL );
         if( pRoom->pControlledBy )
				InvokeBehavior( WIDE("conceal"), ps->Current, pRoom->pControlledBy, NULL );
         /*
			if( pe->pControlledBy &&
				pe->pBehaviors &&
				pe->pBehaviors->OnEnter )
			{
				pe->pControlledBy->pToldBy = ps;
				pNewMS = InvokeMacro( pe->pControlledBy
										  , pe->pBehaviors->OnEnter
										  , NULL );
			}
			if( pRoom &&
				 pRoom->pControlledBy &&
				pRoom->pBehaviors &&
				pRoom->pBehaviors->OnConceal )
			{
				pRoom->pControlledBy->pToldBy = ps;
				pNewMS = InvokeMacro( pRoom->pControlledBy
										  , pRoom->pBehaviors->OnConceal
										  , NULL );
										  }
                                */
		}
		else
		{
			// I dunno what to call these...
			// they should trigger different events
			// [ you ] - [    ]
			// where [a] and [b] are rooms which are attached
         // and you move from a within to b within
		}

		if( !ps->CurrentMacro && !pNewMS )
		{
			vtprintf( vt , WIDE("Now in %s."), GetText( GetName(pe)));
			EnqueLink( Output, VarTextGet( vt ) );
		}
		else if( ps->CurrentMacro )
			ps->CurrentMacro->state.flags.bSuccess = TRUE;
	}
	else
	{
		if( !ps->CurrentMacro )
		{
			vtprintf( vt ,WIDE("Nothing by that name resides in this location.") );
			EnqueLink( Output, VarTextGet( vt ) );
		}
	}
	VarTextDestroy( &vt );
	return(NULL);
}

//--------------------------------------------------------------------------
PENTITY Leave( PLINKQUEUE *Output, PSENTIENT ps )
{
	PENTITY thing, pe, pe2;
	pe = FindContainer( thing = ps->Current ); // points to current room...
	pe2 = FindContainer( pe ); // outer room...

	putin( pe2, pullout( pe, thing ) );


	InvokeBehavior( WIDE("Inject"), pe, pe2->pControlledBy, SegDuplicate( GetName( pe ) ) );
	InvokeBehavior( WIDE("Leave"), pe, pe2->pControlledBy, SegDuplicate( GetName( pe ) ) );
 	if( !ps->CurrentMacro )
	{
		PVARTEXT vt;
		vt = VarTextCreate( );
		vtprintf( vt, WIDE("Now in %s."), GetText( GetName( pe2 ) ));
		EnqueLink( Output, VarTextGet( vt ) );
		VarTextDestroy( &vt );
	}
	else
		ps->CurrentMacro->state.flags.bSuccess = TRUE;
	return(NULL);
}

//-----------------------------------------------------------------------------

int CPROC VERSION( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   temp = GetParam( ps, &parameters );
   if( !temp )
   {
       DECLTEXT( msg, WIDE("Version: 1.0b13xxxxxxxx") );
       msg.data.size = snprintf( msg.data.data, sizeof( msg.data.data ), WIDE("Version: %s"), DekVersion );
       EnqueLink( &ps->Command->Output, &msg );
   }
   else
   {
       AddVariable( ps, ps->Current, temp, SegCreateFromText( DekVersion ) );
   }
   return FALSE;
}


//-----------------------------------------------------------------------------

int CPROC CREATE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	while( (temp = GetParam( ps, &parameters )) )
	{
		TEXTCHAR c = GetText( temp )[0];
		if( ( c >= '9' ) || ( c <= '0' ) )
		{
			PTEXT pName;
			if( temp->flags & TF_INDIRECT )
			{
				pName = BuildLine( GetIndirect( temp ) );
			}
			else
				pName = TextDuplicate(temp,TRUE);
			CreateEntityIn( ps->Current, pName );
		}
		else
		{
			DECLTEXT( msg,WIDE("Invalid object name. Must not start with a digit.") );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

int CPROC _SHADOW( PSENTIENT ps, PTEXT parameters )
{
	if( ps )
	{
		PENTITY pe;
		pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
		if( pe )
			CreateShadowIn( ps->Current, pe );
    }
    return FALSE;
}

//-----------------------------------------------------------------------------

int CPROC _DUPLICATE( PSENTIENT ps, PTEXT parameters )
{
   return UNIMPLEMENTED( ps, parameters );
   /*
         goto unimplement;
         if( (temp = GetParam( ps, &parameters ) ))
         {
            PENTITY pe;
            pe = FindThing( ps->Current, FIND_ON, temp );
            if( pe )
            {
               attach( ps->Current, Duplicate( pe ) );
            }
            else
            {
               pe = FindThing( ps->Current, FIND_IN, temp );
               if( pe )
               {
                  Duplicate( pe );
               }
               else
               {
                  // near is within the same room or
                  // attached to the same room
                  // may NOT duplicate the ROOM
                  pe = FindThing( ps->Current, FIND_NEAR, temp );
                  if( pe )
                  {
                     Duplicate( pe );
                  }
                  else
                     if( !pms )
                        Unknown( &ps->Command->Output, temp );
               }
            }
         }
    */
}
//-----------------------------------------------------------------------------
int CPROC DESCRIBE( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   {
       PENTITY pe;
       pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
       if( !pe ) pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_MACRO, NULL );
       if( pe )
       {
           temp = MacroDuplicateEx( ps, parameters, FALSE, TRUE );
           SetDescription( pe, BuildLine( temp ) );
           LineRelease( temp );
       }
   }
   return FALSE;
}

//-----------------------------------------------------------------------------
int CPROC DESTROY( PSENTIENT ps, PTEXT parameters )
{
	PTEXT _parameters;
	PTEXT temp;
	PTEXT pName = NULL;
	PTEXT pNewParams = NULL;
	PVARTEXT vt = NULL;
	while( ( _parameters = parameters ) )
	{
		PENTITY pe;
		// ( destroy %me ONLY : destroy <myname> will NOT work)
		pe = (PENTITY)FindThingEx( ps, &parameters, ps->Current, FIND_GRABBABLE, NULL
							 , &pName
							 , &pNewParams DBG_SRC );
		if( pe )
		{
			if( !ps->CurrentMacro )
			{
				if( !vt ) vt = VarTextCreate( );
				vtprintf( vt, WIDE("Destroyed Entity: %s."), GetText( GetName( pe ) ) );
				EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			}
			else
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
			DestroyEntity( pe );
		}
		else
		{
			PMACRO pm;
			//lprintf( WIDE("Find macro..") );
			// this is like the only place that uses FindThing to get the macro...
			// maybe should get a new thing for this... since this is a different
         // section of code anyhow...
			pm = (PMACRO)FindThingEx( ps, &parameters, ps->Current, FIND_MACRO, NULL
							    , &pName, &pNewParams DBG_SRC );
			if( !pm )
			{
				//lprintf( WIDE("Not found, eat param.") );
				temp = GetParam( ps, &parameters );
				if( temp == ps->Current->pName )
				{
					DestroyEntity( ps->Current );
				}
				else
					if( !ps->CurrentMacro )
						Unknown( &ps->Command->Output, pName);
				continue;
				//return 0;
			}
			else
			{
				xlprintf(LOG_NOISE+1)( WIDE("Found it as a macro?") );
				if( !ps->CurrentMacro )
				{
					if( !vt ) vt = VarTextCreate( );
					vtprintf( vt, WIDE("Destroyed Macro: %s."), GetText( GetName( pm ) ) );
					EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
				}
				else
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
				DestroyMacro( ps->Current, pm );
			}
		}
	}
   if( vt )
		VarTextDestroy( &vt );
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC RENAME( PSENTIENT ps, PTEXT parameters )
{
	PENTITY pe;
	pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
	if( pe &&
		!pe->flags.bShadow )
	{
		PTEXT pNew;
		if(( pNew = GetParam( ps, &parameters ) ))
			if( GetName( pe ) )
				LineRelease( pe->pName );
		pe->pName = SegDuplicate( pNew );
	}
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC GRAB( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp, from;
   PVARTEXT vt;
   vt = VarTextCreate( );
   if( (temp = GetParam( ps, &parameters) ) )
   {
      _32 x;
		PENTITY pThing, pRoom, pOwner;
		x = Grab( ps->Current, temp
				  , &pRoom
				  , &pOwner
				  , &pThing
				  , from = GetParam( ps, &parameters ) );
      //lprintf( WIDE("Result of grab is %d"), x );
		switch( x )
		{
		//case GRAB_RESULT_FOUND_ON:
		case GRAB_RESULT_FOUND_IN:
		case GRAB_RESULT_FOUND_NEAR:
		case GRAB_RESULT_FOUND_IN_BAG:
			InvokeBehavior( WIDE("Grab"), ps->Current, pThing->pControlledBy, NULL );
			InvokeBehavior( WIDE("Pull"), ps->Current, pRoom->pControlledBy, NULL );
			InvokeBehavior( WIDE("Receive"), ps->Current, pOwner->pControlledBy, NULL );
         /*
			if( pThing->pControlledBy &&
				pThing->pBehaviors &&
				pThing->pBehaviors->OnGrab )
			{
				InvokeMacro( pThing->pControlledBy
								, pThing->pBehaviors->OnGrab
								, NULL );
			}
			if( pRoom->pControlledBy &&
				pRoom->pBehaviors &&
				pRoom->pBehaviors->OnPull )
			{
				InvokeMacro( pRoom->pControlledBy
								, pRoom->pBehaviors->OnPull
								, NULL );
			}
			if( pOwner->pControlledBy &&
				pOwner->pBehaviors &&
				pOwner->pBehaviors->OnReceive )
			{
				InvokeMacro( pOwner->pControlledBy
								, pOwner->pBehaviors->OnReceive
								, NULL );
								}
                        */
			break;
		}
		if( ps->CurrentMacro )
		{
			ps->CurrentMacro->state.flags.bSuccess =
				( x != GRAB_RESULT_NO_OBJECT )
				&& ( x != GRAB_RESULT_FAILED_BAG )
				&& ( x != GRAB_RESULT_FAILED_IN_BAG );
		}
		else
		{
	      switch( x )
   	   {
      	case GRAB_RESULT_NO_OBJECT:
         	vtprintf( vt ,WIDE("No such object %s."),GetText(temp));
	         break;
   	   case GRAB_RESULT_FOUND_ON:
      	   vtprintf( vt ,WIDE("%s was in your hand already."), GetText( temp ));
         	break;
	      case GRAB_RESULT_FOUND_IN:
   	      vtprintf( vt ,WIDE("%s found in pocket."), GetText( temp ));
      	   break;
	      case GRAB_RESULT_FOUND_NEAR:
   	      vtprintf( vt ,WIDE("%s found in location."), GetText( temp ));
      	   break;
	      case GRAB_RESULT_FAILED_BAG:
   	      vtprintf( vt ,WIDE("No such object %s."),GetText(from));
      	   break;
	      case GRAB_RESULT_FAILED_IN_BAG:
   	      vtprintf( vt ,WIDE("%s did not contain %s."),GetText(from), GetText(temp));
				break;       	
	      case GRAB_RESULT_FOUND_IN_BAG:
   	      vtprintf( vt ,WIDE("Grabbed %s from %s."),GetText(temp), GetText(from));
				break;
	      }
         EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
      }
   }
   VarTextDestroy( &vt );
   return FALSE;
}
//--------------------------------------

int CPROC DROP( PSENTIENT ps, PTEXT parameters )
{
    PVARTEXT vt;
    vt = VarTextCreate( );
	 {
       PENTITY pEntity = ps->Current;
		 PENTITY pe;
		 while( pe = (PENTITY)FindThing( ps, &parameters, pEntity, FIND_ON, NULL ) )
		 {
			 PENTITY pRoom = FindContainer( pEntity );
			 detach( pEntity, pe );
			 if( pe->pWithin )
				 putin( pe->pWithin, pEntity );
			 else
				 if( pEntity->pWithin )
					 putin( pEntity->pWithin, pe );
			 {
				 if( ps->CurrentMacro )
					 ps->CurrentMacro->state.flags.bSuccess = TRUE;
				 else
				 {
					 vtprintf( vt, WIDE("Dropped %s."), GetText( GetName( pe ) ) );
					 EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
				 }
			 }
		 }
	 }
   VarTextDestroy( &vt );
   return FALSE;
}

//--------------------------------------

int CPROC STORE( PSENTIENT ps, PTEXT parameters )
{
    PTEXT temp, into;
    PVARTEXT vt;
    int result;
    vt = VarTextCreate( );
   if( (temp = GetParam( ps, &parameters ) ) )
   {
      result = Store( ps->Current, temp, into = GetParam( ps, &parameters ) );
      if( result <= 0 )
      {
         if( !ps->CurrentMacro )
         {
         	if( result == -2 )
         	{
         		vtprintf( vt, WIDE("You are not allowed to store %s in itself."), GetText( temp ) );
         	}
         	else
         	{
	        		if( result < 0 )
		         	vtprintf( vt, WIDE("Could not see %s."), GetText( into ) );
					else
	      	      vtprintf( vt, WIDE("Not holding %s."), GetText( temp ) );
				}
            EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
         }
      }
      else
      {
         if( ps->CurrentMacro )
            ps->CurrentMacro->state.flags.bSuccess = TRUE;
         else
         {
         	if( into )
	         	vtprintf( vt, WIDE("Stored %s in %s."), GetText( temp ), GetText( into ) );
	         else
	         	vtprintf( vt, WIDE("Stored %s."), GetText( temp ) );
	         EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
         }
		}
   }
   VarTextDestroy( &vt );
   return FALSE;
}
//--------------------------------------

int CPROC ATTACH( PSENTIENT ps, PTEXT parameters )
{
   PTEXT pt1, pt2;
   pt1 = GetParam( ps, &parameters);
   pt2 = GetParam( ps, &parameters );
   if( pt1 && pt2 )
   {
      Attach( ps, pt1, pt2 );  // passes name of object to attach to.
   }
   else
   {
      DECLTEXT( msg, WIDE("Must supply 2 object names to Attach to each other") );
      EnqueLink( &ps->Command->Output, &msg );
   }
   return FALSE;
}

//--------------------------------------

int CPROC DETACH( PSENTIENT ps, PTEXT parameters )
{
    PTEXT pt1, pt2;
    pt1 = GetParam( ps, &parameters );
    pt2 = GetParam( ps, &parameters );
    if( pt1 && pt2 )
    {
        Detach( ps, pt1, pt2 );
    }
    else
    {
        DECLTEXT( msg, WIDE("Must supply 2 object names to Attach to each other") );
        EnqueLink( &ps->Command->Output, &msg );
    }
    return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC JOIN( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
         if( (temp = GetParam( ps, &parameters ) ))
         {
            Join( &ps->Command->Output, ps, temp );
         }
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC ENTER( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   if( (temp = GetParam( ps, &parameters) ))
      Enter(&ps->Command->Output, ps, temp);
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC LEAVE( PSENTIENT ps, PTEXT parameters )
{
   Leave( &ps->Command->Output, ps );  // pass current entity...
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC INVENTORY( PSENTIENT ps, PTEXT parameters )
{
   Possessions( ps->Current, &ps->Command->Output );
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC LOOK( PSENTIENT ps, PTEXT parameters )
{
   Look( &ps->Command->Output, ps, parameters );
   return FALSE;
}
//-----------------------------------------------------------------------------
int CPROC MAP( PSENTIENT ps, PTEXT parameters )
{
   PTEXT temp;
   PVARTEXT vt;
   if( !( temp = GetParam( ps, &parameters ) ) )
   {
       vt = VarTextCreate( );
       vtprintf( vt, WIDE("All Objects from -The Void-") );
       EnqueLink( &ps->Command->Output, VarTextGet(vt) );
       showall( &ps->Command->Output, THE_VOID);
   }
   else
   {
      // of course can look up a specific object to map from....
      //showall();
   }
   return FALSE;
}
//-----------------------------------------------------------------------------

int CPROC LOOKFOR( PSENTIENT ps, PTEXT parameters )
{
	
	return FALSE;
}

//-----------------------------------------------------------------------------
// $Log: entcmds.c,v $
// Revision 1.19  2005/05/30 11:51:42  d3x0r
// Remove many messages...
//
// Revision 1.18  2005/04/24 10:00:20  d3x0r
// Many fixes to finding things, etc.  Also fixed resuming on /wait command, previously fell back to the 5 second poll on sentients, instead of when data became available re-evaluating for a command.
//
// Revision 1.17  2005/04/20 06:20:25  d3x0r
// Okay a massive refit to 'FindThing' To behave like GetParam(SubstToken) to handle count.object references better.
//
// Revision 1.16  2005/04/15 16:21:31  d3x0r
// Extend find object - and look...
//
// Revision 1.15  2005/02/24 03:10:51  d3x0r
// Fix behaviors to work better... now need to register terminal close and a couple other behaviors...
//
// Revision 1.14  2005/02/21 12:08:56  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.13  2004/09/27 16:06:56  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.12  2004/04/05 22:57:15  d3x0r
// Revert plugin loading to standard library
//
// Revision 1.11  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.10  2003/11/07 23:45:28  panther
// Port to new vartext abstraction
//
// Revision 1.9  2003/11/01 21:25:38  panther
// Fix prompt issue issues
//
// Revision 1.8  2003/08/02 17:39:15  panther
// Implement some of the behavior methods.
//
// Revision 1.7  2003/03/25 08:59:03  panther
// Added CVS logging
//
