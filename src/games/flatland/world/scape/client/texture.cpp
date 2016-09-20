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

static INDEX CPROC FindTextureName( INDEX texture, uintptr_t psv )
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
	uint32_t Result[1];
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

static INDEX CPROC DeleteATexture( INDEX iTexture, uintptr_t psv )
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
	ForAllTextures( iWorld, DeleteATexture, (uintptr_t)iWorld );
	DeleteSet( (GENERICSET**)&world->textures );
}

//----------------------------------------------------------------------------

void SetSolidColor( INDEX iWorld, INDEX iTexture, CDATA color )
{
	MSGIDTYPE ResultID;
	uint32_t Result[1];
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
//PFLATLAND_TEXTURE ScanAllTextures( char *name, void (CPROC*userproc)(uintptr_t, PFLATLAND_TEXTURE)
 //  						  , uintptr_t userdata )
//{
//   return ForAllTextures( &world->textures, userproc, userdata );
//}

//----------------------------------------------------------------------------

void GetTextureData( INDEX iWorld, INDEX iTexture, PFLATLAND_TEXTURE *pptexture )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	(*pptexture) = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
}

uintptr_t CPROC TextureFilter( INDEX idx, uintptr_t psv )
{
	struct thisstruc{
		INDEX(CPROC*f)(INDEX,uintptr_t);
		uintptr_t psv;
	} *info = (struct thisstruc*)psv;
	return info->f( idx, info->psv ) + 1; // INDEX 0 is valid, -1 is failure so skew result by 1 to have control work.
}


INDEX ForAllTextures( INDEX iWorld, INDEX(CPROC*f)(INDEX,uintptr_t), uintptr_t psv )
{
	struct {
		INDEX(CPROC*f)(INDEX,uintptr_t);
		uintptr_t psv;
	} info;
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	info.f = f;
	info.psv = psv;
	return (INDEX)ForEachSetMember( FLATLAND_TEXTURE, world->textures, TextureFilter, (uintptr_t)&info )-1;
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
	uint32_t Result[1];
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

