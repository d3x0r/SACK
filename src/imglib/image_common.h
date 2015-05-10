#ifndef IMAGE_COMMON_HEADER_INCLUDED
#define IMAGE_COMMON_HEADER_INCLUDED
ASM_IMAGE_NAMESPACE
_32 DOALPHA( _32 over, _32 in, _8 a );
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE

#if !defined( _D3D_DRIVER ) && !defined( _D3D10_DRIVER ) && !defined( _D3D11_DRIVER )
#if defined( REQUIRE_GLUINT ) && !defined( _OPENGL_DRIVER )
typedef unsigned int GLuint;
#endif

#if defined( __3D__ )
// this is actually specific, and is not common, but common needs it in CLR
// because of tight typechecking.
struct glSurfaceImageData {
	struct glSurfaceImageData_flags{
		BIT_FIELD updated : 1;
	} flags;
	GLuint glIndex;
};
#endif
#endif

struct shade_cache_element {
	CDATA r,grn,b;
	Image image;
	_32 age;
	struct shade_cache_element_flags
	{
		BIT_FIELD parent_was_dirty : 1;
		BIT_FIELD inverted : 1;
	}flags;
};

struct shade_cache_image
{
	PLIST elements;
	Image image;
};

#define MAXImageFileSPERSET 256
DeclareSet( ImageFile );
typedef ImageFile *PImageFile;// for get from set

#ifndef IMAGE_MAIN
extern
#endif
struct image_common_local_data_tag {
	PTREEROOT shade_cache;
	PTREEROOT tint_cache;
	//GLuint glImageIndex;
	//PLIST glSurface; // list of struct glSurfaceData *
	//struct glSurfaceData *glActiveSurface;
	//RCOORD scale;
	//PTRANSFORM camera; // active camera at begindraw
	PImageFileSET Images;
} image_common_local;
//#define l image_common_local


void  CPROC cSetColorAlpha( PCDATA po, int oo, int w, int h, CDATA color );
void  CPROC cSetColor( PCDATA po, int oo, int w, int h, CDATA color );

Image CPROC GetInvertedImage( Image child_image );
Image CPROC GetShadedImage( Image child_image, CDATA red, CDATA green, CDATA blue );
Image CPROC GetTintedImage( Image child_image, CDATA color );

void CPROC SetFontBias( SFTFont font, S_32 x, S_32 y );

IMAGE_NAMESPACE_END

#endif