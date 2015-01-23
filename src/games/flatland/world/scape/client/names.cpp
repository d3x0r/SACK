#define WORLD_SOURCE
#define WORLDSCAPE_INTERFACE_USED
#define WORLD_CLIENT_LIBRARY
//#define WORLD_CLIENT_LIBRARY


#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>
#include <msgclient.h>
#include "world.h"

#include "global.h"
#include <world_proto.h>

// this really needs a name hash, 
// and a custom name space allocator
// the hash is to reference for duplicated names...

extern GLOBAL g;

int LineCount( CTEXTSTR text )
{
	int c, l = 1;
	if( !text || !text[0] )
		return 0;
	for( c = 0; text[c]; c++ )
		if( text[c] == '\n' )
			l++;
	return l;
}

WORLD_PROC( void, GetNameText )( INDEX iWorld, INDEX iName, TEXTCHAR *text, int maxlen )
{	
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PNAME name = (iName==INVALID_INDEX)?NULL:GetSetMember( NAME, &world->names, iName );
	int l, ofs = 0;
	if( !name )
	{
		if( text )
			text[0] = 0;
		return;
	}
	for( l = 0; l < name->lines; l++ )
	{
		if( l )
		{
			text[ofs++] = '\\';
			text[ofs++] = 'n';
		}	
		MemCpy( text+ofs, name->name[l].name, name->name[l].length );
		ofs += name->name[l].length;
	}
	text[ofs] = 0;
}

void SetName( INDEX iWorld, INDEX iName, CTEXTSTR text )
{
	GETWORLD( iWorld );
	PNAME name = GetName( iName );
	int l, start, end;
	if( !name )
		return;
	if( name->name )
	{
		for( l = 0; l < name->lines; l++ )
			Release( name->name[l].name );
		Release( name->name );
	}
	name->lines = LineCount( text );
	if( name->lines )
	{
		name->name = (struct name_data*)Allocate( sizeof( *name->name ) * name->lines );
		start = 0;
		end = 0;
		for( l= 0; l < name->lines; l++ )
		{
			while( text[end] && text[end] != '\n' ) end++;
			name->name[l].length = end-start;
			name->name[l].name = (TEXTCHAR*)Allocate( (end-start) + 1 );
			MemCpy( name->name[l].name, text + start, end-start );
			name->name[l].name[end-start] = 0;
			start = end+1;
			end = start;
		}
	}
	else
		name->name = NULL;
}

INDEX MakeName( INDEX iWorld, CTEXTSTR text )
{
	_32 ResultID;
	_32 Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(MakeName), 2
								, &ResultID, Result, &ResultLen
								, &iWorld, sizeof( iWorld )
								, text, StrLen(text)+1
								)
		&& ( ResultID == (MSG_ID(MakeName)|SERVER_SUCCESS)))
	{
		/* name will be sent as an event, making this valid?! 
		* it's not probable to guarnatee that this anme is created before this routine results.
		*/
		return Result[0];
	}
	return INVALID_INDEX;
}

static PTRSZVAL CPROC DeleteAName( PNAME name, INDEX iWorld )
{
	GETWORLD( iWorld );
	int l;
	if( name )
	{
		if( name->name )
		{
			for( l = 0; l < name->lines; l++ )
				Release( name->name[l].name );
			Release( name->name );
         name->name = NULL;
		}
		DeleteFromSet( NAME, world->names, name );
	}
	return 0;
}

void DeleteName( INDEX iWorld, INDEX iName )
{
   GETWORLD( iWorld );
   DeleteAName( GetName( iWorld ), iWorld );
}

void DeleteNames( INDEX iWorld  )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PNAMESET *ppNames = &world->names;
	ForAllInSet( NAME, *ppNames, (FAISCallback)DeleteAName, iWorld );
	DeleteSet( (GENERICSET**)ppNames );
}

void GetNameData( INDEX iWorld, INDEX iName, PNAME *name )
{
	if( iName == INVALID_INDEX )
	{
		if( name )
			(*name) = NULL;
		return;
	}
	{
		PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
		(*name) = GetUsedSetMember( NAME, &world->names, iName );
	}
}
