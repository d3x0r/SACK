#ifndef IMAGE_COMMON_HEADER_INCLUDED
#define IMAGE_COMMON_HEADER_INCLUDED
ASM_IMAGE_NAMESPACE
_32 DOALPHA( _32 over, _32 in, _8 a );
ASM_IMAGE_NAMESPACE_END

#if !defined( _D3D_DRIVER ) && !defined( _D3D10_DRIVER ) && !defined( _D3D11_DRIVER )
#if defined( REQUIRE_GLUINT ) //&& !defined( _OPENGL_DRIVER )
typedef unsigned int GLuint;
#endif

IMAGE_NAMESPACE


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


void  SetColorAlpha( PCDATA po, int oo, int w, int h, CDATA color );
void  SetColor( PCDATA po, int oo, int w, int h, CDATA color );

Image CPROC GetInvertedImage( Image child_image );
Image CPROC GetShadedImage( Image child_image, CDATA red, CDATA green, CDATA blue );
Image CPROC GetTintedImage( Image child_image, CDATA color );

void CPROC SetFontBias( SFTFont font, S_32 x, S_32 y );

SlicedImage MakeSlicedImage( Image source, _32 left, _32 right, _32 top, _32 bottom, LOGICAL output_center );
SlicedImage MakeSlicedImageComplex( Image source
										, _32 top_left_x, _32 top_left_y, _32 top_left_width, _32 top_left_height
										, _32 top_x, _32 top_y, _32 top_width, _32 top_height
										, _32 top_right_x, _32 top_right_y, _32 top_right_width, _32 top_right_height
										, _32 left_x, _32 left_y, _32 left_width, _32 left_height
										, _32 center_x, _32 center_y, _32 center_width, _32 center_height
										, _32 right_x, _32 right_y, _32 right_width, _32 right_height
										, _32 bottom_left_x, _32 bottom_left_y, _32 bottom_left_width, _32 bottom_left_height
										, _32 bottom_x, _32 bottom_y, _32 bottom_width, _32 bottom_height
										, _32 bottom_right_x, _32 bottom_right_y, _32 bottom_right_width, _32 bottom_right_height
										, LOGICAL output_center );
void UnmakeSlicedImage( SlicedImage image );
void BlotSlicedImageEx( Image dest, SlicedImage source, S_32 x, S_32 y, _32 width, _32 height, int alpha, enum BlotOperation op, ... );


IMAGE_NAMESPACE_END

#endif