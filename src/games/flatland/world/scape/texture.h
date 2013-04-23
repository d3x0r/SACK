#ifndef TEXTURE_DEFINED
#define TEXTURE_DEFINED

#include <colordef.h>
#include <sack_types.h>
#include <worldstrucs.h>

//PTEXTURE GetTexture( PTEXTURESET *pset, char *name );

//int DeleteTexture( PTEXTURE texture );
//void DeleteTextures( PTEXTURESET *pSet );

void SrvrSetSolidColor( _32 client_id, INDEX iWorld, INDEX texture, CDATA color );

//INDEX MakeTexture( INDEX iWorld, PC_POINT point, INDEX iName );
INDEX SrvrMakeTexture( _32 client_id, INDEX iWorld, INDEX iName );

/* probably actually a sector method? */
INDEX SrvrSetTexture( _32 client_id, INDEX iWorld, INDEX iSector, INDEX iTexture );


#endif
