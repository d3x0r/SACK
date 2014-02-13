#define WORLD_SOURCE
#include <stdhdrs.h>
#include <string.h>
#include <sharemem.h>
#include <logging.h>

#include "world.h"

#include "global.h"
// #ifdef WORLD_SERVER ?
#include "service.h"
// #endif ?


extern GLOBAL g;

//----------------------------------------------------------------------------

typedef struct find_thing_tag {
	INDEX iWorld;
	INDEX iName;
} FINDTHING, *PFINDTHING;

static INDEX CPROC FindTextureName( INDEX texture, PTRSZVAL psv )
{
	PFINDTHING pft = ( PFINDTHING)psv;
	TEXTCHAR textname[256];
	TEXTCHAR name[256];
	GetTextureNameText( pft->iWorld, texture, textname, sizeof( textname ) );
	GetNameText( pft->iWorld, pft->iName, name, sizeof( name ) );
	if( !StrCmp( name, textname ) )
	{
		return texture;
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

INDEX SrvrMakeTexture( _32 client_id, INDEX iWorld, INDEX iName )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	INDEX iTexture = INVALID_INDEX;
	FINDTHING ft;
	ft.iWorld = iWorld;
	ft.iName  = iName;
	if( iName != INVALID_INDEX )
	{
		iTexture = ForAllTextures( iWorld, FindTextureName, (PTRSZVAL)&ft );
		if( iTexture != INVALID_INDEX )
			texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
		else
		{
			texture = GetFromSet( FLATLAND_TEXTURE, &world->textures );
			iTexture = GetMemberIndex( FLATLAND_TEXTURE, &world->textures, texture );
			texture->iName = iName;
		}
	}
#if defined( WORLD_SCAPE_SERVER_EXPORTS ) || defined( WORLDSCAPE_SERVICE )
	MarkTextureUpdated( client_id, iWorld, iTexture );
#endif
	return iTexture;
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

void SrvrSetSolidColor( _32 client_id, INDEX iWorld, INDEX iTexture, CDATA color )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	if( iTexture == INVALID_INDEX )
		return;
	texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	texture->flags.bColor = TRUE;
	texture->data.color = color;
#if defined( WORLD_SCAPE_SERVER_EXPORTS ) || defined( WORLDSCAPE_SERVICE )
	MarkTextureUpdated( client_id, iWorld, iTexture );
#endif
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

void GetTextureNameText( INDEX iWorld, INDEX iTexture, char *buf, int bufsize )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	if( iTexture == INVALID_INDEX )
		return;
	texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	GetNameText( iWorld, texture->iName, buf, bufsize );
}

INDEX SrvrSetTexture( _32 client_id, INDEX iWorld, INDEX iSector, INDEX iTexture )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector;
	if( !world || iSector == INVALID_INDEX )
		return INVALID_INDEX;
	sector= GetSetMember( SECTOR, &world->sectors, iSector );
	sector->iTexture = iTexture;
#if defined( WORLD_SCAPE_SERVER_EXPORTS ) || defined( WORLDSCAPE_SERVICE )
	MarkSectorUpdated( client_id, iWorld, iSector );
#endif
	return iSector;
}

