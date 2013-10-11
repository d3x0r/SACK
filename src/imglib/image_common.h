#ifndef IMAGE_COMMON_HEADER_INCLUDED
#define IMAGE_COMMON_HEADER_INCLUDED
ASM_IMAGE_NAMESPACE
_32 DOALPHA( _32 over, _32 in, _8 a );
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE

#if !defined( _D3D_DRIVER ) && !defined( _D3D10_DRIVER )
#if defined( REQUIRE_GLUINT ) && !defined( _OPENGL_DRIVER )
typedef unsigned int GLuint;
#endif

// this is actually specific, and is not common, but common needs it in CLR
// because of tight typechecking.
struct glSurfaceImageData {
	struct {
		BIT_FIELD updated : 1;
	} flags;
	GLuint glIndex;
};
#endif

void  CPROC cSetColorAlpha( PCDATA po, int oo, int w, int h, CDATA color );
void  CPROC cSetColor( PCDATA po, int oo, int w, int h, CDATA color );

Image GetInvertedImage( Image child_image );
Image GetShadedImage( Image child_image, CDATA red, CDATA green, CDATA blue );


IMAGE_NAMESPACE_END

#endif