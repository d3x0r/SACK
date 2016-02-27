#define WORLD_SOURCE
#define WORLD_CLIENT_LIBRARY
#define WORLDSCAPE_INTERFACE_USED

#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>
#include <msgclient.h>

#include "world.h"
#include "global.h"
#include <world_proto.h>

extern GLOBAL g;

//----------------------------------------------------------------------------

typedef struct find_thing_tag {
	INDEX iWorld;
   char *name;
} FINDTHING, *PFINDTHING;

static INDEX CPROC FindTextureName( INDEX texture, PTRSZVAL psv )
{
	PFINDTHING pft = ( PFINDTHING)psv;
	TEXTCHAR textname[256];
	GetTextureNameText( pft->iWorld, texture, textname, 256 );
	if( !StrCmp( pft->name, textname ) )
	{
		return texture;
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

INDEX MakeTexture( INDEX iWorld, INDEX iName )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(MakeTexture), 2
								, &ResultID, Result, &ResultLen
								, &iWorld, sizeof( iWorld )
								, &iName, sizeof( iName )
								)
		&& ( ResultID == (MSG_ID(MakeTexture)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

static INDEX CPROC DeleteATexture( INDEX iTexture, PTRSZVAL psv )
{
	INDEX iWorld = (INDEX)psv;
	GETWORLD( iWorld );
	PFLATLAND_TEXTURE texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	if( texture )
		DeleteName( iWorld, texture->iName );
	DeleteFromSet( FLATLAND_TEXTURE, world->textures, texture );
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

void DeleteTexture( INDEX iWorld, INDEX iTexture )
{
	//GETWORLD( iWorld );
   //PFLATLAND_TEXTURE texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
   DeleteATexture( iTexture, iWorld );
}

//----------------------------------------------------------------------------

void DeleteTextures( INDEX iWorld )
{
	GETWORLD( iWorld );
	ForAllTextures( iWorld, DeleteATexture, (PTRSZVAL)iWorld );
	DeleteSet( (GENERICSET**)&world->textures );
}

//----------------------------------------------------------------------------

void SetSolidColor( INDEX iWorld, INDEX iTexture, CDATA color )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = 0;//sizeof( INDEX );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(SetSolidColor), 1
								, &ResultID, Result, &ResultLen 
								/* param length fails uncer certain compilers... 
								// and this would be cleaner with sizeof( CDATA )
								*/
								, &iWorld, ParamLength( iWorld, iTexture )
								)
		&& ( ResultID == (MSG_ID(SetSolidColor)|SERVER_SUCCESS)))
	{
		return /*Result[0]*/;
	}
	return /*FALSE*/;
}

//----------------------------------------------------------------------------
// if name is null,
//PFLATLAND_TEXTURE ScanAllTextures( char *name, void (CPROC*userproc)(PTRSZVAL, PFLATLAND_TEXTURE)
 //  						  , PTRSZVAL userdata )
//{
//   return ForAllTextures( &world->textures, userproc, userdata );
//}

//----------------------------------------------------------------------------

void GetTextureData( INDEX iWorld, INDEX iTexture, PFLATLAND_TEXTURE *pptexture )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	(*pptexture) = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
}

PTRSZVAL CPROC TextureFilter( INDEX idx, PTRSZVAL psv )
{
	struct thisstruc{
		INDEX(CPROC*f)(INDEX,PTRSZVAL);
		PTRSZVAL psv;
	} *info = (struct thisstruc*)psv;
	return info->f( idx, info->psv ) + 1; // INDEX 0 is valid, -1 is failure so skew result by 1 to have control work.
}


INDEX ForAllTextures( INDEX iWorld, INDEX(CPROC*f)(INDEX,PTRSZVAL), PTRSZVAL psv )
{
	struct {
		INDEX(CPROC*f)(INDEX,PTRSZVAL);
		PTRSZVAL psv;
	} info;
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	info.f = f;
	info.psv = psv;
	return (INDEX)ForEachSetMember( FLATLAND_TEXTURE, world->textures, TextureFilter, (PTRSZVAL)&info )-1;
}

void GetTextureNameText( INDEX iWorld, INDEX iTexture, TEXTCHAR *buf, int bufsize )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	if( iTexture == INVALID_INDEX )
		return;
	texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	GetNameText( iWorld, texture->iName, buf, bufsize );
}

INDEX SetTexture( INDEX iWorld, INDEX iSector, INDEX iTexture )
{
	MSGIDTYPE ResultID;
	_32 Result[1];
	size_t ResultLen = sizeof( Result );
	if( ConnectToServer()
		&& TransactServerMultiMessage( MSG_ID(SetTexture), 1
								, &ResultID, Result, &ResultLen 
								/* param length fails uncer certain compilers... 
								// and this would be cleaner with sizeof( CDATA )
								*/
								, &iWorld, ParamLength( iWorld, iTexture )
								)
		&& ( ResultID == (MSG_ID(SetSolidColor)|SERVER_SUCCESS)))
	{
		return Result[0];
	}
	return INVALID_INDEX;
#if 0
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector;
	if( !world || iSector == INVALID_INDEX )
		return INVALID_INDEX;
	sector= GetSetMember( SECTOR, &world->sectors, iSector );
	sector->iTexture = iTexture;
	MarkSectorUpdated( iWorld, iSector );
	return iSector;
#endif
}

