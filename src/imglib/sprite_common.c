#define FIX_RELEASE_COM_COLLISION


#ifndef IMAGE_LIBRARY_SOURCE
#  define IMAGE_LIBRARY_SOURCE
#endif
#include <stdhdrs.h>
#include <stdio.h>
#include <string.h>
#define LIBRARY_DEF
#include <sharemem.h>
#include <vectlib.h>
#include "sprite_local.h"



//#define DEBUG_TIMING
#define OUTPUT_IMAGE

#define _farnspokeb(a,v) ( ( *(char*)(a) ) = (v) )
#define _farnspokew(a,v) ( ( *(short*)(a) ) = (v) )
#define _farnspokel(a,v) ( ( *(long*)(a) ) = (v) )

IMAGE_NAMESPACE


static PSPRITE MakeSpriteEx( DBG_VOIDPASS )
{
   PSPRITE ps;
   ps = (PSPRITE)AllocateEx( sizeof( SPRITE ) DBG_RELAY );
   MemSet( ps, 0, sizeof( SPRITE ) );
   return ps;
}

  PSPRITE  IMGVER(MakeSpriteImageEx )( ImageFile *Image DBG_PASS)
{
   PSPRITE ps = MakeSpriteEx( DBG_VOIDRELAY );
   ps->image = Image;
   return ps;
}

  PSPRITE  IMGVER(MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS )
{
   PSPRITE ps = MakeSpriteEx( DBG_VOIDRELAY );
   ps->image = IMGVER(LoadImageFileEx)( fname DBG_RELAY );
   if( !ps->image )
   {
      ReleaseEx( ps DBG_RELAY );
      return NULL;
   }
   return ps;
}

void IMGVER(UnmakeSprite)( PSPRITE sprite, int bForceImageAlso )
{
	if( bForceImageAlso )// of if the sprite was created by name...
	{
      IMGVER(UnmakeImageFileEx)( sprite->image DBG_SRC );
	}
   Deallocate( PSPRITE, sprite );
}




PSPRITE IMGVER(SetSpriteHotspot)( PSPRITE sprite, int32_t x, int32_t y )
{
	if( sprite )
	{
		sprite->hotx = x;
		sprite->hoty = y;
	}
   return sprite;
}

PSPRITE IMGVER(SetSpritePosition)( PSPRITE sprite, int32_t x, int32_t y )
{
	if( sprite )
	{
		sprite->curx = x;
		sprite->cury = y;
	}
   return sprite;
}

IMAGE_NAMESPACE_END
