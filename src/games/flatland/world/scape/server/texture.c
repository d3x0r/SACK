#define WORLD_SERVICE
#define WORLD_SOURCE

#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>

#include "world.h"
#include "global.h"
#include "names.h"
extern GLOBAL g;

//----------------------------------------------------------------------------

typedef struct find_thing_tag {
	INDEX iWorld;
   CTEXTSTR name;
} FINDTHING, *PFINDTHING;


void GetTextureNameText( INDEX iWorld, INDEX iTexture, TEXTCHAR *buf, int bufsize )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	if( iTexture == INVALID_INDEX )
		return;
	texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	GetNameText( iWorld, texture->iName, buf, bufsize );
}


INDEX ForAllTextures( INDEX iWorld, INDEX(CPROC*f)(INDEX,uintptr_t), uintptr_t psv );

static INDEX CPROC FindTextureName( INDEX texture, uintptr_t psv )
{
	PFINDTHING pft = ( PFINDTHING)psv;
	TEXTCHAR textname[256];
	GetTextureNameText( pft->iWorld, texture, textname, sizeof( textname ) );
	if( !strcmp( pft->name, textname ) )
	{
		return texture;
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------

INDEX MakeTexture( INDEX iWorld, INDEX iName )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	INDEX iTexture = INVALID_INDEX;
	TEXTCHAR textname[256];
	FINDTHING ft;
	ft.iWorld = iWorld;
	GetTextureNameText( iWorld, iName, textname, sizeof( textname ) );
	ft.name  = textname;

	if( iName != INVALID_INDEX )
	{
		iTexture = ForAllTextures( iWorld, FindTextureName, (uintptr_t)&ft );
		if( iTexture != INVALID_INDEX )
			texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
		else
		{
			texture = GetFromSet( FLATLAND_TEXTURE, &world->textures );
			iTexture = GetMemberIndex( FLATLAND_TEXTURE, &world->textures, texture );
			//OwnName( iName );
			texture->iName = iName;
		}
	}
	MarkTextureUpdated( iWorld, iTexture );
	return iTexture;
}

//----------------------------------------------------------------------------

static INDEX CPROC DeleteATexture( INDEX iTexture, uintptr_t psv )
{
	INDEX iWorld = (INDEX)psv;
	GETWORLD( iWorld );
	PFLATLAND_TEXTURE texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
   //DisownName( texture->iName );
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
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PFLATLAND_TEXTURE texture;
	if( iTexture == INVALID_INDEX )
		return;
	texture = GetSetMember( FLATLAND_TEXTURE, &world->textures, iTexture );
	texture->flags.bColor = TRUE;
	texture->data.color = color;
	MarkTextureUpdated( iWorld, iTexture );
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

INDEX SetTexture( INDEX iWorld, INDEX iSector, INDEX iTexture )
{
	PWORLD world = GetSetMember( WORLD, &g.worlds, iWorld );
	PSECTOR sector;
	if( !world || iSector == INVALID_INDEX )
		return INVALID_INDEX;
	sector= GetSetMember( SECTOR, &world->sectors, iSector );
	sector->iTexture = iTexture;
	MarkSectorUpdated( iWorld, iSector );
	return iSector;
}

