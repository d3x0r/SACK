#ifndef TEXTURE_DEFINED
#define TEXTURE_DEFINED

#include <colordef.h>
#include <types.h>
#include <worldstrucs.h>

PTEXTURE GetTexture( PTEXTURESET *pset, char *name );

int DeleteTexture( PTEXTURE texture );
void DeleteTextures( PTEXTURESET *pSet );

void SetSolidColor( PTEXTURE texture, CDATA color );


#endif
