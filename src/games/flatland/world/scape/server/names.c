#define WORLD_SERVICE
#define WORLD_SOURCE


#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>
#include "world.h"

#include "global.h"

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
	PNAME name = GetSetMember( NAME, &world->names, iName );
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
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	// anmes should be cached... and referenced...
	// ahh well we'll deal with this for now... there's not
	// that many names....
	//   see procreg for someone who needs to worry.
	PNAME name = GetFromSet( NAME, &world->names );
	INDEX iName = GetMemberIndex( NAME, &world->names, name );
	MemSet( name, 0, sizeof( NAME ) );
	if( text )
		SetName( iWorld, iName, text );
#ifdef WORLDSCAPE_SERVICE
	MarkNameUpdated( iWorld, iName );
#endif
	return iName;
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
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	(*name) = GetSetMember( NAME, &world->names, iName );
}


void OwnName( INDEX iName )
{
}
void DisownName( INDEX iName )
{

}

