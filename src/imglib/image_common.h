#ifndef IMAGE_COMMON_HEADER_INCLUDED
#define IMAGE_COMMON_HEADER_INCLUDED
ASM_IMAGE_NAMESPACE
uint32_t DOALPHA( uint32_t over, uint32_t in, uint8_t a );


#define DOALPHA_( over, in, a )  (                                                                        \
	/*int r, g, b, aout;   */                                                                             \
	(!a)?(over):(                                                                                         \
		( (a) == 255 )?(in | 0xFF000000/* force alpha full on.*/):(                                         \
                                                                                                          \
			(aout = AlphaTable[a][AlphaVal( over )] << 24),                                               \
                                                                                                          \
			(r = ((((((in) & 0x00FF0000) >> 8)  *((a) + 1)) + ((((over) & 0x00FF0000) >> 8)*(256 - (a)))))),    \
			(r = ( r & 0xFF000000 )?(0x00FF0000):(r & 0x00FF0000)),                                       \
                                                                                                          \
			(g = ((((((in) & 0x0000FF00) >> 8)  *((a) + 1)) + ((((over) & 0x0000FF00) >> 8)*(256 - (a)))))),    \
			(g=( g & 0x00FF0000 )?0x0000FF00:(g & 0x0000FF00) ),                                          \
                                                                                                          \
			(b = ((((in) & 0x000000FF)*((a) + 1)) + (((over) & 0x000000FF)*(256 - (a)))) >> 8 ),                \
			b = ( b & 0x0000FF00 )?0x000000FF:(b),                                                        \
                                                                                                          \
			(aout | b | g | r)                                                                            \
		)                                                                                                 \
	)                                                                                                     \
)

//CDATA *po;
//int r, g, b, aout;                                                                                    \

#define plotalpha_( pi, x_, y_, c ) (                         \
	( !pi || !pi->image )?0:(                                   \
	( ((x_) >= pi->x) && ((x_) < (pi->x + pi->width)) &&        \
		((y_) >= pi->y) && ((y_) < (pi->y + pi->height)) )?(    \
			(po = IMG_ADDRESS( pi, (x_), (y_) )),               \
			(*po = DOALPHA_( *po, c, AlphaVal( c ) ))       \
		):0                                                 \
	)      \
)


ASM_IMAGE_NAMESPACE_END

#if !defined( _D3D_DRIVER ) && !defined( _D3D10_DRIVER ) && !defined( _D3D11_DRIVER )
#if defined( REQUIRE_GLUINT ) //&& !defined( _OPENGL_DRIVER )
typedef unsigned int GLuint;
#endif

IMAGE_NAMESPACE

#if defined( _VULKAN_DRIVER )
struct vkSurfaceImageData {
	struct vkSurfaceImageData_flags {
		BIT_FIELD updated : 1;
	} flags;
	uint32_t index;
};
#elif defined( __3D__ )
// this is actually specific, and is not common, but common needs it in CLR
// because of tight typechecking.
struct glSurfaceImageData {
	struct glSurfaceImageData_flags{
		BIT_FIELD updated : 1;
	} flags;
	GLuint glIndex;
};
#else
IMAGE_NAMESPACE
#endif
#else
IMAGE_NAMESPACE
#endif

struct shade_cache_element {
	CDATA r,grn,b;
	Image image;
	uint32_t age;
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

void CPROC SetFontBias( SFTFont font, int32_t x, int32_t y );

SlicedImage MakeSlicedImage( Image source, uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, LOGICAL output_center );
SlicedImage MakeSlicedImageComplex( Image source
										, uint32_t top_left_x, uint32_t top_left_y, uint32_t top_left_width, uint32_t top_left_height
										, uint32_t top_x, uint32_t top_y, uint32_t top_width, uint32_t top_height
										, uint32_t top_right_x, uint32_t top_right_y, uint32_t top_right_width, uint32_t top_right_height
										, uint32_t left_x, uint32_t left_y, uint32_t left_width, uint32_t left_height
										, uint32_t center_x, uint32_t center_y, uint32_t center_width, uint32_t center_height
										, uint32_t right_x, uint32_t right_y, uint32_t right_width, uint32_t right_height
										, uint32_t bottom_left_x, uint32_t bottom_left_y, uint32_t bottom_left_width, uint32_t bottom_left_height
										, uint32_t bottom_x, uint32_t bottom_y, uint32_t bottom_width, uint32_t bottom_height
										, uint32_t bottom_right_x, uint32_t bottom_right_y, uint32_t bottom_right_width, uint32_t bottom_right_height
										, LOGICAL output_center );
void UnmakeSlicedImage( SlicedImage image );
void BlotSlicedImageEx( Image dest, SlicedImage source, int32_t x, int32_t y, uint32_t width, uint32_t height, int alpha, enum BlotOperation op, ... );


IMAGE_NAMESPACE_END

#endif