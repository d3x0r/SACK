#ifndef IMAGE_STRUCTURE_DEFINED
#include <colordef.h>

#ifdef _D3D_DRIVER
#include <d3d9.h>
#endif

#ifdef _D3D10_DRIVER
#include <D3D10_1.h>
#include <D3D10.h>
#endif

#ifdef _D3D11_DRIVER
#include <D3D11.h>
#endif

#ifdef _VULKAN_DRIVER
#include <vulkan/vulkan.h>
#endif


#include <vectlib.h>

#if defined( _WIN32 ) && !defined( _INVERT_IMAGE ) && !defined( _OPENGL_DRIVER ) && !defined( _D3D_DRIVER )
#define _INVERT_IMAGE
#endif

#define WILL_DEFINE_IMAGE_STRUCTURE
#define IMAGE_STRUCTURE_DEFINED
#include <image.h>

IMAGE_NAMESPACE

#ifdef __cplusplus
	namespace Interface
{
	struct image_interface_tag;
}
#ifdef _D3D_DRIVER
	using namespace sack::image::d3d::Interface;
#elif defined( _D3D10_DRIVER )
	using namespace sack::image::d3d10::Interface;
#elif defined( _D3D11_DRIVER )
	using namespace sack::image::d3d11::Interface;
#else
	using namespace sack::image::Interface;
#endif
#endif


#ifndef PSPRITE_METHOD
#define PSPRITE_METHOD PSPRITE_METHOD
/* pointer to a structure defining a sprite draw method this should be defined in render namespace...*/
	typedef struct sprite_method_tag *PSPRITE_METHOD;
#endif


	/* Flags which may be combined in <link sack::image::ImageFile_tag::flags, Image.flags> */
	enum ImageFlags {
IF_FLAG_FREE   =0x00001, // this has been freed - but contains sub images
IF_FLAG_HIDDEN =0x00002, // moved beyond parent image's bound
IF_FLAG_EXTERN_COLORS =0x00004, // built with a *image from external sources
IF_FLAG_HAS_CLIPS     =0x00008, // pay attention to (clips) array.

// with no _X_STRING flag - characters are shown as literal character glyph.
IF_FLAG_C_STRING       = 0x00010, // strings on this use 'c' processing
IF_FLAG_MENU_STRING    = 0x00020, // strings on this use menu processing ( &underline )
IF_FLAG_CONTROL_STRING = 0x00040, // strings use control chars (newline, tab)
IF_FLAG_OWN_DATA       = 0x00080, // this has been freed - but contains sub images
IF_FLAG_INVERTED       = 0x00100,  // image is inverted (standard under windows, but this allows images to be configured dynamically - a hack to match SDL lameness )
// DisplayLib uses this flag - indicates panel root
IF_FLAG_USER1          = 0x10000, // please #define user flag to this
// DisplayLib uses this flag - indicates is part of a displayed panel
IF_FLAG_USER2          = 0x20000, /* An extra flag that can be used by users of the image library. */

IF_FLAG_USER3          = 0x40000, /* An extra flag that can be used by users of the image library. */
IF_FLAG_FINAL_RENDER   = 0x00200, // output should render to opengl target (with transform); also used with proxy
IF_FLAG_UPDATED        = 0x00400, // set when a operation has changed the surface of a local image; requires update to remote device(s)
IF_FLAG_HAS_PUTSTRING  = 0x00800, // set when a operation has changed the surface of a local image; requires update to remote device(s)
IF_FLAG_IN_MEMORY      = 0x01000, // is an in-memory image; that is the surface can be written to directly with pixel ops (putstring)
	};
//#define _DRAWPOINT_X 0
//#define _DRAWPOINT_Y 1

struct ImageFile_tag
{
#if defined( IMAGE_LIBRARY_SOURCE ) || defined( NEED_REAL_IMAGE_STRUCTURE )
	int real_x;
	int real_y;
	int real_width;   // desired height and width may not be actual cause of 
	int real_height;  // resizing of parent image....
# ifdef HAVE_ANONYMOUS_STRUCTURES
	IMAGE_RECTANGLE;
# else
	int x; // need this for sub images - otherwise is irrelavent
	int y;
	int width;  /// Width of image.
	int height; /// Height of image.
# endif
#else
	/* X coordinate of the image within another image. */
	int x;
	/* Y coordinate of an image within another image. */
	int y;
	int width;   // desired height and width may not be actual cause of
	int height;  // resizing of parent image....
	int actual_x; // need this for sub images - otherwise is irrelavent
	int actual_y;  /* Y coordinate of the image. probably 0 if a parent image. */
	int actual_width;  // Width of image.
	int actual_height; // Height of image.
#endif

	int pwidth; // width of real physical layer

	// The image data.
	PCOLOR image;   
	/* a combination of <link ImageFlags, IF_FLAG_> (ImageFile Flag)
	   which apply to this image.                                    */
	int flags;
	/* This points to a peer image that existed before this one. If
	   NULL, there is no elder, otherwise, contains the next peer
	   image in the same parent image.                              */
	/* Points to the parent image of a sub-image. (The parent image
	   contains this image)                                         */
	/* Pointer to the youngest child sub-image. If there are no sub
	   images pChild will be NULL. Otherwise, pchild points at the
	   first of one or more sub images. Other sub images in this one
	   are found by following the pElder link of the pChild.         */
	/* This points at a more recently created sub-image. (another
	   sub image within the same parent, but younger)             */
	struct ImageFile_tag *pParent, *pChild, *pElder, *pYounger;
	   // effective x - clipped by reality real coordinate. 
	           // (often eff_x = -real_x )
	int eff_x; 
	/* this is used internally for knowing what the effective y of
	   the image is. If the sub-image spans a boundry of a parent
	   image, then the effective Y that will be worked with is only
	   a part of the subimage.                                      */
	int eff_y;
		// effective max - maximum coordinate...
	int eff_maxx;
		// effective maximum Y
	int eff_maxy;
		/* An extra rectangle that can be used to carry additional
		 information like update region.                         */
	IMAGE_RECTANGLE auxrect;
	// fonts need a way to output the font character subimages to the real image...
	// or for 3D; to reverse scale appropriately
	struct image_interface_tag  *reverse_interface;
	POINTER reverse_interface_instance; // what the interface thinks this is... 
	void (*extra_close)( struct ImageFile_tag *);
//DOM-IGNORE-BEGIN
#if defined( __3D__ )
	PTRANSFORM transform;
#endif

#ifdef _OPENGL_DRIVER
	/* gl context? */
	PLIST glSurface;
	int glActiveSurface; // most things will still use this, since reload image is called first, reload will set active
	VECTOR coords[4];  // updated with SetTransformRelation, otherwise defaults to image size.
#endif
#ifdef _D3D10_DRIVER
	PLIST Surfaces;
	ID3D10Texture2D *pActiveSurface;
#endif
#ifdef _D3D11_DRIVER
	PLIST Surfaces;
	ID3D11Texture2D *pActiveSurface;
#endif
#ifdef _D3D_DRIVER
	/* gl context? */
	PLIST Surfaces;
	IDirect3DBaseTexture9 *pActiveSurface;
#endif
#ifdef _VULKAN_DRIVER
	PLIST vkSurface;
	int vkActiveSurface; // most things will still use this, since reload image is called first, reload will set active
	VECTOR coords[4];  // updated with SetTransformRelation, otherwise defaults to image size.
#endif
#ifdef __cplusplus
#ifndef __WATCOMC__ // watcom limits protections in structs to protected and public
private:
#endif
#endif
//DOM-IGNORE-END
};

enum SlicedImageSection {
	SLICED_IMAGE_TOP_LEFT,
	SLICED_IMAGE_TOP,
	SLICED_IMAGE_TOP_RIGHT,
	SLICED_IMAGE_LEFT,
	SLICED_IMAGE_CENTER,
	SLICED_IMAGE_RIGHT,
	SLICED_IMAGE_BOTTOM_LEFT,
	SLICED_IMAGE_BOTTOM,
	SLICED_IMAGE_BOTTOM_RIGHT,
};

struct SlicedImageFile {
	struct ImageFile_tag *image;
	struct ImageFile_tag *slices[9];
	uint32_t left, right, top, bottom;
	uint32_t center_w, center_h;
	uint32_t right_w;
	uint32_t bottom_h;
	LOGICAL output_center;
	LOGICAL extended_slice;
};

/* The basic structure. This is referenced by applications as '<link sack::image::Image, Image>'
	This is the primary type that the image library works with.
	
	This is the internal definition.
	
	This is a actual data content, Image is (ImageFile *).                                        */
typedef struct ImageFile_tag ImageFile;
/* A simple wrapper to add dynamic changing position and
	orientation to an image. Sprites can be output at any angle. */
struct sprite_tag
{
	/* Current location of the sprite's origin. */
	/* Current location of the sprite's origin. */
	int32_t curx, cury;  // current x and current y for placement on image.
	int32_t hotx, hoty;  // int of bitmap hotspot... centers cur on hot
	Image image;
	// curx,y are kept for moving the sprite independantly
	fixed scalex, scaley;
	// radians from 0 -> 2*pi.  there is no negative...
	float angle; // radians for now... (used internally, set by blot rotated sprite)
	// should consider keeping the angle of rotation
	// and also should cosider keeping velocity/acceleration
	// but then limits would have to be kept also... so perhaps
	// the game module should keep such silly factors... but then couldn't
	// it also keep curx, cury ?  though hotx hoty is the actual
	// origin to rotate this image about, and to draw ON curx 0 cury 0
	// int orgx, orgy;  // rotated origin of bitmap.

	// after being drawn the min(x,y) and max(x,y) are set.
	int32_t minx, maxx; // after draw, these are the extent of the sprite.
	int32_t miny, maxy; // after draw, these are the extent of the sprite.
	PSPRITE_METHOD pSpriteMethod;
};
/* A Sprite type. Adds position and rotation and motion factors
	to an image. Hooks into the render system to get an update to
	draw on a temporary layer after the base rendering is done.   */
typedef struct sprite_tag SPRITE;



#ifdef _INVERT_IMAGE
// inversion does not account for eff_y - only eff_maxy
// eff maxy - eff_minY???
#define INVERTY(i,y)     ( (((i)->eff_maxy) - (y))/*+((i)->eff_y)*/)
#else
/* This is a macro is used when image data is inverted on a
	platform. (Windows images, the first row of data is the
	bottom of the image, all Image operations are specified from
	the top-left as 0,0)                                         */
#define INVERTY(i,y)     ((y) - (i)->eff_y)
#endif

#define INVERTY_INVERTED(i,y)     ( (((i)->eff_maxy) - (y))/*+((i)->eff_y)*/)
#define INVERTY_NON_INVERTED(i,y)     ((y) - (i)->eff_y)

#if defined(__cplusplus_cli ) && !defined( IMAGE_SOURCE )
//IMAGE_PROC( PCDATA, ImageAddress )( Image image, int32_t x, int32_t y );
//#define IMG_ADDRESS(i,x,y) ImageAddress( i,x,y )
#else
#define IMG_ADDRESS(i,x,y)    ((CDATA*) \
	                            ((i)->image + (( (x) - (i)->eff_x ) \
	+(((i)->flags&IF_FLAG_INVERTED)?(INVERTY_INVERTED( (i), (y) ) * (i)->pwidth ):(INVERTY_NON_INVERTED( (i), (y) ) * (i)->pwidth )) \
	                            ))   \
										)
#endif

#if 0
#if defined( __arm__ ) && defined( IMAGE_LIBRARY_SOURCE ) && !defined( DISPLAY_SOURCE )
extern unsigned char AlphaTable[256][256];

static CDATA DOALPHA( CDATA over, CDATA in, uint8_t a )
{
	int r, g, b, aout;
	if( !a )
		return over;
	if( a > 255 )
		a = 255;
	if( a == 255 )
		return (in | 0xFF000000); // force alpha full on.
	aout = AlphaTable[a][AlphaVal( over )] << 24;
	r = ((((RedVal(in))  *(a+1)) + ((RedVal(over))  *(256-(a)))) >> 8 );
	if( r > (255) ) r = (255);
	g = (((GreenVal(in))*(a+1)) + ((GreenVal(over))*(256-(a)))) >> 8;
	if( g > (255) ) g = (255);
	b = ((((BlueVal(in)) *(a+1)) + ((BlueVal(over)) *(256-(a)))) >> 8 );
	if( b > 255 ) b = 255;
	return aout|(r<<16)|(g<<8)|b;
	//return AColor( r, g, b, aout );
}
#endif
#endif
IMAGE_NAMESPACE_END
// end if_not_included
#endif

// $Log: imagestruct.h,v $
// Revision 1.2  2005/04/05 11:56:04  panther
// Adding sprite support - might have added an extra draw callback...
//
// Revision 1.1  2004/06/21 07:38:39  d3x0r
// Move structures into common...
//
// Revision 1.20  2003/10/14 20:48:55  panther
// Tweak mmx a bit - no improvement visible but shorter
//
// Revision 1.19  2003/10/14 16:36:45  panther
// Oops doalpha was outside of known inclusion frame
//
// Revision 1.18  2003/10/14 00:43:03  panther
// Arm optimizations.  Looks like I'm about maxed.
//
// Revision 1.17  2003/09/15 17:06:37  panther
// Fixed to image, display, controls, support user defined clipping , nearly clearing correct portions of frame when clearing hotspots...
//
// Revision 1.16  2003/04/25 08:33:09  panther
// Okay move the -1's back out of IMG_ADDRESS
//
// Revision 1.15  2003/04/21 23:33:09  panther
// fix certain image ops - should check blot direct...
//
// Revision 1.14  2003/03/30 18:39:03  panther
// Update image blotters to use IMG_ADDRESS
//
// Revision 1.13  2003/03/30 16:11:03  panther
// Clipping images works now... blat image untested
//
// Revision 1.12  2003/03/30 06:24:56  panther
// Turns out I had badly implemented clipping...
//
// Revision 1.11  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
