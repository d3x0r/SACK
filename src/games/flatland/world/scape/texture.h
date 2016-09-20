#ifndef TEXTURE_DEFINED
#define TEXTURE_DEFINED

#include <colordef.h>
#include <sack_types.h>
#include <worldstrucs.h>

//PTEXTURE GetTexture( PTEXTURESET *pset, char *name );

//int DeleteTexture( PTEXTURE texture );
//void DeleteTextures( PTEXTURESET *pSet );

void SrvrSetSolidColor( uint32_t client_id, INDEX iWorld, INDEX texture, CDATA color );

//INDEX MakeTexture( INDEX iWorld, PC_POINT point, INDEX iName );
INDEX SrvrMakeTexture( uint32_t client_id, INDEX iWorld, INDEX iName );

/* probably actually a sector method? */
INDEX SrvrSetTexture( uint32_t client_id, INDEX iWorld, INDEX iSector, INDEX iTexture );


#endif
