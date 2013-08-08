

/*
 There are many applications which wish to share data, but, unfortunatly, they don't all want
 each other's data... they only want a small subst of data.

 These data structures could be allocated, and managed in a common place, with an event mechanism
 such that updates to these structures will be propagated to the correct places....

 This could even be nice and register this data with the procreglib so that it may be browsed
 using some other standard plugins...

 */
#include <sack_types.h>
#include <sharemem.h>

typedef struct link_tag
{
	// CTEXTSTR name;  ? :)
	_32 offset;
	INDEX expected_type;
} LINK, *PLINK;

typedef struct set_link_tag
{
	// CTEXTSTR name;  ? :)
	_32 unit_count; // maxcnt
	_32 unit_size; // unitsize
	_32 set_size; // setsize // really this should be implied from maxcnt * unitsize...
	_32 offset;
} SET_LINK, *PSET_LINK;

typedef struct type_tag
{
	PDATALIST link_types;
	_32 datasize;
	_32 slabsize;
	PGENERICSET member_set;
	CTEXTSTR name;
} TYPE, *PTYPE;

typedef struct storage_client_tag
{
	_32 pid;
} STORAGE_CLIENT, *PSTORAGE_CLIENT;

typedef struct local_tag
{
	struct {
		_32 bLocal : 1;
		_32 bClient : 1;
		_32 bService : 1;
	} flags;
	PLIST clients; // PLIST of PSTORAGE_CLIENTs
	_32 client_msgbase;
	_32 service_msgbase;
	PDATALIST types; // PLIST of PTYPEs
} LOCAL;

static LOCAL l;

int CPROC HandleStorageServiceEvents( _32 SourceRouteID, _32 MsgID, _32 *params, _32 param_len, _32 *result, _32 *result_length );



_32 FancyMsg2( _32 msg, _32 *response, _32 *buffer, _32 *buffer_result_len, _32 buffers
				 , POINTER b1, _32 l1
				 , POINTER b2, _32 l2 )
#define FancyMsg1(m,r,b,brl,b1,l1) FancyMsg2(m,r,b,brl,2,b1,l1,NULL,0)
#define FancyMsg(m,r,b,brl) FancyMsg2(m,r,b,brl,2,NULL,0,NULL,0)
{
retry:
	if( !TransactServerMultiMessage( l.service_msgbase + msg
											 , buffers
											  // result is a member index that has been (or will soon be)
											  // populated by an event generated from the server.
											 , response, buffer, buffer_result_len
											 , NULL, 0 /*pairs*/ ) )
	{
		// client has failed... attempt to become a server
		l.flags.bClient = 0;
		l.client_msgbase = 0; // no longer valid.
		l.service_msgbase = RegisterService( WIDE("Data Storage Share"), HandleStorageServiceEvents );
		if( !l.service_msgbase )
		{
			lprintf( WIDE("Client transaction failed to server, and server still exists...") );
			l.client_msgbase = LoadService( WIDE("Data Storage Share"), NULL );
			if( !l.client_msgbase )
			{
				lprintf( WIDE("Hmm service is still absent... should have had someone register by now...") );
				lprintf( WIDE("Fail to local only mode.") );
				l.flags.bLocal = 1;
			}
			else
			{
				lprintf( WIDE("Okay - so... either the base is the same or it's different...") );
				// re-set client bit... we're still slaving... go back up to retry
				l.flags.bClient = 1;
				goto retry;
			}
		}
		else
		{
			lprintf( WIDE("Okay, I'm now the server... clients will now talk to me, once they fail to aquire server.") );
			l.flags.bService = 1;
		}
	}
}

INDEX DataStore_RegisterNamedDataType( CTEXTSTR name, _32 size )
{
	TYPE type;
	type.name = StrDup( name );
	type.datasize = size;
	type.slabsize = 64; // this is definatly a MUST configure!
	type.member_set = NULL;
	type.link_types = CreateDataList( sizeof( LINK ) );
	if( l.flags.bLocal )
	{
		return AddDataLink( &l.types, type );
	}
	else if( l.flags.bClient )
	{
		if( FancyMsg2( MSG_RegisterType, &type_result, sizeof( type_result )
						 , &type, sizeof( TYPE )
						 , type.name, strlen( type.name ) + 1 ) )
		{
         Release( type.name );
		}

      /*
		_32 responce;
      _32 type_result
	retry:
		if( !TransactServerMultiMessage( l.service_msgbase + MSG_RegisterType
												 , 2
												  // result is a member index that has been (or will soon be)
                                      // populated by an event generated from the server.
												 , &responce, &type_result, sizeof( type_result )
												 , &type, sizeof( TYPE )
												 , type.name, strlen( type.name ) + 1 ) )
		{
			// client has failed... attempt to become a server
         l.flags.bClient = 0;
         l.client_msgbase = 0; // no longer valid.
			l.service_msgbase = RegisterService( WIDE("Data Storage Share"), HandleStorageServiceEvents );
			if( !l.service_msgbase )
			{
				lprintf( WIDE("Client transaction failed to server, and server still exists...") );
				l.client_msgbase = LoadService( WIDE("Data Storage Share"), NULL );
				if( !l.client_msgbase )
				{
					lprintf( WIDE("Hmm service is still absent... should have had someone register by now...") );
               lprintf( WIDE("Fail to local only mode.") );
               l.flags.bLocal = 1;
				}
				else
				{
					lprintf( WIDE("Okay - so... either the base is the same or it's different...") );
               // re-set client bit... we're still slaving... go back up to retry
					l.flags.bClient = 1;
               goto retry;
				}
			}
			else
			{
				lprintf( WIDE("Okay, I'm now the server... clients will now talk to me, once they fail to aquire server.") );
            l.flags.bService = 1;
			}
		}
		else
		{
			Release( type.name );
			// may wait here until
			// this member's name is set...
			// which confirms that the event has been handled...
         // it should be like one or fewer task switchs...
         return GetDataItem( &l.types, type_result );
			}
         */
	}
	else if( l.flags.bService )
	{
		INDEX idx;
		// the new type index created by this call... the authoritative index for clients to use
      // until this guy fails and they become local only mode...
		INDEX iType;
		PSTORAGE_CLIENT client;
		// keep this one for my reference...
      // also use this index
		iType = AddDataItem( &l.types, type );
		LIST_FORALL( l.clients, idx, STORAGE_CLIENT, client )
		{
			// send to all clients, even, and especially, the original creator...
			SendMultiServiceEvent( client->pid, l.service_msgbase + MSG_RegisterType, 3
                              , &iType, sizeof( iType ) // cleaned blah...
										, &type, sizeof( TYPE )
										, type.name, strlen( type.name ) + 1 );
		}

	}
   return type;
}

// the resultant data is updated when remote systems
// issue such updates...
POINTER DataStore_CreateDataType( INDEX iType )
{
	// *** FIX1 ***
	// resolve iType into type through set reference
	if( g.flags.bClient )
	{
      TransactSer
	}
	else if( g.flags.bService )
	{
      _32 type_result;
		if( FancyMsg1( MSG_CreateType, &type_result, sizeof( type_result )
						 , &iType, sizeof( iType ) ) )
		{
			PTYPE type = (PTYPE)GetLink( &l.types, iType );
			POINTER p = GetSetMember( &type->member_set
											, type_result
											 // this member needs to be uhmm
											 // deprecated... creategenericset( size, count )
											, SetSize( type->datasize, type->slabsize )
											, type->datasize
											, type->slabsize
											);
         return p;
		}
	}
	else if( g.flags.bLocal )
	{
		PTYPE type = (PTYPE)GetLink( &l.types, iType );
		return GetFromSet( &type->member_set
								// this member needs to be uhmm
								// deprecated... creategenericset( size, count )
							  , SetSize( type->datasize, type->slabsize )
							  , type->datasize
							  , type->slabsize
							  );
	}

}

INDEX DataStore_CreateDataSetLinkEx( INDEX iType, _32 offsetof_set_pointer, _32 setsize, _32 setunits, _32 maxcnt )
{
	PTYPE type = (PTYPE)GetLink( &l.types, iType );
	SET_LINK link;
   link.unit_count = maxcnt;
	link.unit_size = unitsize;
   link.set_size = setsize;
   link.offset = offsetof_set_pointer;
   return AddDataItem( &type->set_link_types, &link );

}

POINTER DataStore_GetFromDataSet( INDEX iType, POINTER member, INDEX iSet )
{
	PTYPE type = (PTYPE)GetLink( &l.types, iType );
	if( type )
	{
		SET_LINK link = GetDataItem( &type->set_link_types, iSet );
		if( link )
		{
         // invoke member creation in remote also...
         return GetFromSetEx( (GENERICSET **)((int)member + link->offset), link->set_size, link->unit_count, link->unit_size );
		}
	}
   return NULL;
}

INDEX DataStore_CreateLink( INDEX iType, _32 offsetof_pointer, INDEX iTypeLinked )
{
	// member is ignored for now
   // *** FIX1 ***
	PTYPE type = (PTYPE)GetLink( &l.types, iType );
	LINK link;
   link.expected_type = iTypeLinked;
   link.offset = offsetof_pointer;
   return AddDataItem( &type->link_types, &link );
}


POINTER DataStore_SetLink( INDEX iType, POINTER member, _32 iLink, _32 iOtherType, POINTER othermember )
{
	// member->link = othermember
   PTYPE type = (PTYPE)GetLink( &l.types, iType );
	PLINK link = GetDataItem( &type->link_types, iLink );
   //  othermember maybe needs to be an index...
   //PTYPE othertype = (PTYPE)iOtherType;
	//PLINK otherlink = GetDataItem( &othertype->link_types, iOtherLink );

   (*(POINTER*)(((PTRSZVAL)member) + link->offset) = othermember;
}

POINTER DataStore_SetLinkList( INDEX iType, POINTER member, _32 iLink, _32 iOtherType, POINTER othermember )
{
   // AddLink( member->list, othermember )
   PTYPE type = (PTYPE)GetLink( &l.types, iType );
	PLINK link = GetDataItem( &type->link_types, iLink );
   //  othermember maybe needs to be an index...
   //PTYPE othertype = (PTYPE)iOtherType;
	//PLINK otherlink = GetDataItem( &othertype->link_types, iOtherLink );

   (*(POINTER*)(((PTRSZVAL)member) + link->offset) = othermember;
}

POINTER SetBiLink( INDEX iType, POINTER member, _32 iLink, _32 iOtherType, POINTER othermember, _32 iOtherLink )
{
   PTYPE type = (PTYPE)GetLink( &l.types, iType );
	// member->link = othermember
   // othermember->otherlink = member

}

POINTER SetMeLink( INDEX iType, POINTER member, _32 iLink, _32 iOtherType, POINTER othermember, _32 iOtherLink )
{
   PTYPE type = (PTYPE)GetLink( &l.types, iType );
	// member->link = othermember
   // othermember->otherlink = &member->link
}

LOGICAL SetData( INDEX iType, INDEX iMember, POINTER pMember )
{
	if( !l.flags.bLocal )
	{
		PTYPE type = (PTYPE)GetLink( &l.types, iType );
		// send event to all peers about change...
		// applications receive their data magically... there is no get data result
		POINTER p = GetFromSet( &type->member_set
									  // this member needs to be uhmm
									  // deprecated... creategenericset( size, count )
									 , SetSize( type->datasize, type->slabsize )
									 , type->datasize
									 , type->slabsize
									 );
	}
	else
	{
		// local mode - set data is no-op...
		if( l.flags.bClient )
		{
		}
		else if( l.flags.bService )
		{
         // service source, generate update events to clients.
		}
	}

}

enum {
	MSG_RegisterType = MSG_EventUser
	  , MSG_CreateDataType
	  , MSG_CreateLink

};

int CPROC HandleStorageServiceEvents( _32 SourceRouteID, _32 MsgID, _32 *params, _32 param_len, _32 *result, _32 *result_length )
{
}

int CPROC HandleStorageEvents( _32 MsgID, _32 *params, _32 paramlen )
{
	// events to client - create structure... etc
	swtich( MsgID )
	{
	case MSG_RegisterType:
		{
			INDEX iType = params[0];
			PTYPE p = (PTYPE)GetDataItem( &l.types, iType );
			MemCpy( p, params + 1, sizeof( TYPE ) );
			p->name = StrDup( (CTEXTSTR)(( (PTRSZVAL)(params + 1)) + sizeof( TYPE )));
		}
		break;
	//case
	}
}

PRELOAD( RegisterDataStorageService )
{
	l.service_msgbase = RegisterServiceHandler( WIDE("Data Storage Share"), HandleStorageServiceEvents );
	if( !l.service_msgbase )
	{
		l.client_msgbase = LoadService( WIDE("Data Storage Share"), HandleStorageEvents );
		if( !l.client_msgbase )
		{
			lprintf( WIDE("message service not enabled... enabling local mode.") );
			l.flags.bLocal = 1;
		}
		else
			l.flags.bClient = 1;
	}
	else
		l.flags.bService = 1;
}

EXPORTED_DATA INDEX iTransform;

PRELOAD( RegisterSackKnownTypes )
{
	iTransform = RegisterNamedDataType( WIDE("space frame transform"), sizeof( TRANSFORM ) );
}


PUBLIC( Start )( void )
{
	// RegisterIcon
	while( 1 )
	{
		WakeableSleep();
	}
}

