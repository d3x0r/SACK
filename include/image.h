/* Crafted by Jim Buckeyne (c)1999-2006++ Freedom Collective
   
   
   
   Image building tracking, and simple manipulations.        */

// if the library is to have it's own idea of what
// an image is - then it should have included
// the definition for 'SFTFont', and 'Image' before 
// including this... otherwise, it is assumed to 
// be a client, and therefore does not need the information
// if a custom structure is used - then it MUST define 
// it's ACTUAL x,y,width,height as the first 4 S_32 bit values.

#ifndef IMAGE_H
// multiple inclusion protection symbol
#define IMAGE_H

#if defined( _MSC_VER ) && defined( SACK_BAG_EXPORTS ) && 0
#define HAS_ASSEMBLY
#endif

#include <sack_types.h>
#include <colordef.h>
#include <fractions.h>
#ifndef __NO_INTERFACES__
#include <procreg.h>
#endif

# ifndef SECOND_IMAGE_LEVEL
#  define SECOND_IMAGE_LEVEL _2
/* This is a macro used for building name changes for
   interfaces.                                        */
#  define PASTE(sym,name) name
# else
#  define PASTE2(sym,name) sym##name
#  define PASTE(sym,name) PASTE2(sym,name)
# endif
/* Macro to do symbol concatenation. */
#define _PASTE2(sym,name) sym##name
/* A second level paste macro so macro substitution is done on
   \parameters.                                                */
#define _PASTE(sym,name) _PASTE2(sym,name)


/* Define the default call type of image routines. CPROC is
   __cdecl.                                                 */
#define IMAGE_API CPROC

#     ifdef IMAGE_LIBRARY_SOURCE
#        define IMAGE_PROC  EXPORT_METHOD
// this sometimes needs an extra 'extern'
//#           ifdef IMAGE_MAIN
//#        define IMAGE_PROC_D EXPORT_METHOD
//#           else
//#        define IMAGE_PROC_D extern EXPORT_METHOD
//#           endif
#     else
/* Define the linkage type of the routine... probably
   __declspec(dllimport) if not building the library. */
#        define IMAGE_PROC IMPORT_METHOD
// this sometimes needs an extra 'extern'
//#        define IMAGE_PROC_D  IMPORT_METHOD
#     endif

#if defined( _WIN32 ) && !defined( _OPENGL_DRIVER ) && !defined( _D3D_DRIVER ) && !defined( _D3D10_DRIVER ) && !defined( _D3D11_DRIVER )
#define _INVERT_IMAGE
#endif

#ifdef __cplusplus
/* Define the namespace of image routines, when building under
   C++.                                                        */
#ifdef _D3D_DRIVER
#define IMAGE_NAMESPACE namespace sack { namespace image { namespace d3d {
#define _IMAGE_NAMESPACE namespace image { namespace d3d {
#define BASE_IMAGE_NAMESPACE namespace image {
/* Define the namespace of image routines, when building under
   C++. This ends a namespace.                                 */
#define IMAGE_NAMESPACE_END }}}
#elif defined( _D3D10_DRIVER )
#define IMAGE_NAMESPACE namespace sack { namespace image { namespace d3d10 {
#define _IMAGE_NAMESPACE namespace image { namespace d3d10 {
#define BASE_IMAGE_NAMESPACE namespace image {
/* Define the namespace of image routines, when building under
   C++. This ends a namespace.                                 */
#define IMAGE_NAMESPACE_END }}}
#elif defined( _D3D11_DRIVER )
#define IMAGE_NAMESPACE namespace sack { namespace image { namespace d3d11 {
#define _IMAGE_NAMESPACE namespace image { namespace d3d11 {
#define BASE_IMAGE_NAMESPACE namespace image {
/* Define the namespace of image routines, when building under
   C++. This ends a namespace.                                 */
#define IMAGE_NAMESPACE_END }}}
#else
#define BASE_IMAGE_NAMESPACE namespace image {
#define IMAGE_NAMESPACE namespace sack { namespace image {
#define _IMAGE_NAMESPACE namespace image {
/* Define the namespace of image routines, when building under
   C++. This ends a namespace.                                 */
#define IMAGE_NAMESPACE_END }}
#endif
/* Define the namespace of image routines, when building under
   C++. This ends the namespace. Assembly routines only have the
   ability to export C names, so extern"c" { } is used instead
   of namespace ___ { }.                                         */
#define ASM_IMAGE_NAMESPACE extern "C" {
/* Define the namespace of image routines, when building under
   C++. This ends the namespace. Assembly routines only have the
   ability to export C names, so extern"c" { } is used instead
   of namespace ___ { }.                                         */
#define ASM_IMAGE_NAMESPACE_END }
#else
#define BASE_IMAGE_NAMESPACE
#define IMAGE_NAMESPACE 
#define _IMAGE_NAMESPACE
#define IMAGE_NAMESPACE_END
#define ASM_IMAGE_NAMESPACE /* Defined Image API.
   
   
   
   
   See Also
   <link sack::image::Image, Image>
   
   <link sack::image::SFTFont, SFTFont>
   
   <link Colors>
                                    */

#define ASM_IMAGE_NAMESPACE_END
#endif
SACK_NAMESPACE
/* Deals with images and image processing.
   
   
   
   Image is the primary type of this.
   
   SFTFont is a secondary type for putting text on images.
   
   
   
   render namespace is contained in image, because without
   image, there could be no render. see PRENDERER.         */
	_IMAGE_NAMESPACE

/* A fixed point decimal number (for freetype font rendering) */
typedef S_64 fixed;
//#ifndef IMAGE_STRUCTURE_DEFINED
//#define IMAGE_STRUCTURE_DEFINED
// consider minimal size - +/- 32000 should be enough for display purposes.
// print... well that's another matter.
   typedef S_32 IMAGE_COORDINATE;
   /* Represents the width and height of an image (unsigned values) */
   typedef _32  IMAGE_SIZE_COORDINATE;

   /* An array of 2 IMAGE_COORDINATES - [0] = x, [1] = y */
   typedef IMAGE_COORDINATE IMAGE_POINT[2];
   /* An unsigned value coordinate pair to track the size of
      images.                                                */
   typedef IMAGE_SIZE_COORDINATE IMAGE_EXTENT[2];
   /* Pointer to an <link sack::image::IMAGE_POINT, IMAGE_POINT> */
   typedef IMAGE_COORDINATE *P_IMAGE_POINT;
   /* Pointer to a <link sack::image::IMAGE_EXTENT, IMAGE_EXTENT> */
   typedef IMAGE_SIZE_COORDINATE *P_IMAGE_EXTENT;

#ifdef HAVE_ANONYMOUS_STRUCTURES
typedef struct boundry_rectangle_tag
{  
   union {
      IMAGE_POINT position;
      struct {
         IMAGE_COORDINATE x, y;
      };
   };
   union {
      IMAGE_EXTENT size;
      struct {
         IMAGE_SIZE_COORDINATE width, height;
      };
   };
} IMAGE_RECTANGLE, *P_IMAGE_RECTANGLE;
#else
/* Defines the coordinates of a rectangle. */
/* Pointer to an image rectangle.  */
typedef struct boundry_rectangle_tag
{  
   /* anonymous union containing position information. */
   union {
      /* An anonymous structure containing x,y and width,height of a
         rectangle.                                                  */
      struct {
         /* the left coordinate of a rectangle. */
         /* the top coordinate of a rectangle */
         IMAGE_COORDINATE x, y;
         /* The Y span of the rectangle */
         /* the X Span of the rectangle */
         IMAGE_SIZE_COORDINATE width, height;
      };
      /* Anonymous structure containing position (x,y) and size
         (width,height).                                        */
      struct {
         /* The location of a rectangle (upper left x, y) */
         IMAGE_POINT position;
         /* the size of a rectangle (width and height) */
         IMAGE_EXTENT size;
      };
   };
} IMAGE_RECTANGLE, *P_IMAGE_RECTANGLE;
#endif
/* A macro for accessing vertical (Y) information of an <link sack::image::IMAGE_POINT, IMAGE_POINT>. */
#define IMAGE_POINT_H(ImagePoint) ((ImagePoint)[0])
/* A macro for accessing vertical (Y) information of an <link sack::image::IMAGE_POINT, IMAGE_POINT>. */
#define IMAGE_POINT_V(ImagePoint) ((ImagePoint)[1])

// the image at exactly this position and size 
// is the one being referenced, the actual size and position 
// may vary depending on use (a sub-image outside the
// boundry of its parent).
#define ImageData union {                           \
      struct {                                      \
         IMAGE_COORDINATE x, y;                     \
         IMAGE_SIZE_COORDINATE width, height;       \
      };                                            \
      struct {                                      \
         IMAGE_POINT position;                      \
         IMAGE_EXTENT size;                         \
      };                                            \
   }


/* One of the two primary types that the image library works
   with.
   
   
   Example
   <code lang="c++">
   void LoadImage( char *name )
   {
       Image image = LoadImageFile( name );
       if( image )
       {
          // the image file loaded successfully.
       }
   }
   </code>                                                   */
typedef struct ImageFile_tag *Image;
typedef struct SlicedImageFile *SlicedImage;

#if defined( IMAGE_STRUCTURE_DEFINED )
#if defined(__cplusplus_cli ) && !defined( IMAGE_SOURCE )
IMAGE_PROC  PCDATA IMAGE_API ImageAddress( Image image, S_32 x, S_32 y );
#define IMG_ADDRESS(i,x,y) ImageAddress( i,x,y )
#endif
#endif


#if defined( __cplusplus )
IMAGE_NAMESPACE_END
#endif
#include <imglib/imagestruct.h>
#if defined( __cplusplus )
IMAGE_NAMESPACE
#endif

/* pointer to a sprite type. */
typedef struct sprite_tag *PSPRITE;
//#endif
// at some point, it may be VERY useful
// to have this structure also have a public member.
//

#ifndef NO_FONT
typedef struct simple_font_tag {
   _16 height; // all characters same height
   _16 characters; // number of characters in the set
   _8 char_width[1]; // open ended array size characters...
} FontData;

/* Contains information about a font for drawing and rendering
   from a font file.                                           */
typedef struct font_tag *SFTFont;
#endif
/* A definition of a block structure to transport font and image
   data across message queues.                                   */
/* Type of buffer used to transfer data across message queues. */
typedef struct data_transfer_state_tag {
   /* size of this block of data. */
   _32 size;
   /* offset of the data in the total message. Have to break up
      large buffers into smaller chunks for transfer.           */
   _32 offset;
   /* buffer containing the data to transfer. */
   CDATA buffer;
} *DataState;

//-----------------------------------------------------

enum string_behavior {
   STRING_PRINT_RAW   // every character assumed to have a glyph-including '\0'
   ,STRING_PRINT_CONTROL // control characters perform 'typical' actions - newline, tab, backspace...
   ,STRING_PRINT_C  // c style escape characters are handled \n \b \x## - literal text
   ,STRING_PRINT_MENU /* &amp; performs an underline, also does C style handling. \\&amp;
                         == &amp;                                                         */
};

/* Definitions of symbols to pass to <link SetBlotMethod> to
   specify optimization method.                              */
enum blot_methods {
    /* A Symbol to pass to <link SetBlotMethod> to specify using C
      coded primitives. (for shading and alpha blending).         */
    BLOT_C    
   , BLOT_ASM/* A Symbol to pass to <link SetBlotMethod> to specify using
      primitives with assembly optimization (for shading and alpha
      blending).                                                   */
						,
                  /* A Symbol to pass to <link SetBlotMethod> to specify using
      primitives with MMX optimization (for shading and alpha
      blending).                                                */
    BLOT_MMX 
              
}; 

// specify the method that pixels are copied from one image to another
enum BlotOperation {
   /* copy the pixels from one image to another with no color transform*/
 BLOT_COPY = 0,
   // copy the pixels from one image to another with no color transform, scaling by a single color
 BLOT_SHADED = 1,
   // copy the pixels from one image to another with no color transform, scaling independant R, G and B color channels to a combination of an R Color, B Color, G Color
 BLOT_MULTISHADE = 2,
   /* copy the pixels from one image to another with simple color inversion transform*/
 BLOT_INVERTED = 3,
 /* orientation blots for fonts to 3D and external displays */
 BLOT_ORIENT_NORMAL = 0x00,
 BLOT_ORIENT_INVERT = 0x04,
 BLOT_ORIENT_VERTICAL = 0x08,
 BLOT_ORIENT_VERTICAL_INVERT = 0x0C,
 BLOT_ORIENTATTION = 0x0C,
};


/* Transparency parameter definition
   
   0 : no transparency - completely opaque
   
   1 (TRUE): 0 colors (absolute transparency) only
   
   2-255 : 0 color transparent, plus transparency factor applied
   to all 2 - mostly almost completely transparent 255 not
   transparent (opaque)
   
   257-511 : alpha transparency in pixel plus transparency value
   \- 256 0 pixels will be transparent 257 - slightly more
   opaquen than the original 511 - image totally opaque - alpha
   will be totally overriden no addition 511 nearly completely
   transparent 512-767 ; the low byte of this is subtracted from
   the alpha of the image ; this allows images to be more
   transparent than they were originally 512 - no modification
   alpha imge normal 600 - mid range... more transparent 767 -
   totally transparent any value of transparent greater than the
   max will be clipped to max this will make very high values
   opaque totally...                                             */
enum AlphaModifier {
   /* Direct alpha copy - whatever the alpha is is what the output will be.  Adding a value of 0-255 here will increase the base opacity by that much */
	ALPHA_TRANSPARENT = 0x100,
   // Inverse alpha copy - whatever the alpha is is what the output will be.  Adding a value of 0-255 here will decrease the base opacity by that much
ALPHA_TRANSPARENT_INVERT = 0x200,

   // more than this clips to total transparency
	// for line, plot more than 255 will
// be total opaque... this max only
	// applies to blotted images
ALPHA_TRANSPARENT_MAX = 0x2FF
};

/* library global changes. string behavior cannot be tracked per
   image. string behavior should, for all strings, be the same
   usage for an application... so behavior is associated with
   the particular stream and/or image family. does not modify
   character handling behavior - only strings.
   See Also
   <link sack::image::string_behavior, String Behaviors>         */
   IMAGE_PROC  void IMAGE_API  SetStringBehavior( Image pImage, _32 behavior );
   /* Specify the optimized code to draw with. There are 3 levels,
      C - routines coded in C, ASM - assembly optimization (32bit
      NASM), MMX assembly but taking advantage of MMX features.    */
   IMAGE_PROC  void IMAGE_API  SetBlotMethod    ( _32 method );
   /* This routine can be used to generically scale to any point
      between two colors.
      Parameters
      Color 1 :   CDATA color to scale from
      Color 2 :   CDATA color to scale to
      distance :  How from from 0 to max distance to scale.
      max :       How wide the scalar is.
      
      Remarks
      Max is the scale that distance can go from. Distance 0 is the
      first color, Distance == max is the second color. The
      distance from 0 to max proportionately scaled the color....
      Example
      <code lang="c++">
      CDATA green = BASE_COLOR_GREEN;
      CDATA blue = BASE_COLOR_BLUE;
      CDATA red = BASE_COLOR_RED;
      </code>
      
      Compute a color that is halfway from blue to green. (if the
      total distance is 100, then 50 is half way).
      <code lang="c++">
      CDATA blue_green = ColorAverage( blue, green, 50, 100 );
      </code>
      
      Compute a color that's mostly red.
      <code lang="c++">
      CDATA red_blue_green = ColorAverage( blue_green, red, 240, 255 );
      </code>
      
      Iterate through a whole scaled range...
      <code lang="c++">
      int n;
      for( n = 0; n \< 100; n++ )
      {
          CDATA scaled = ColorAverage( BASE_COLOR_WHITE, BASE_COLOR_BLACK, n, 100 );
          // as n increases, the color slowly goes from WHITE to BLACK.
      }
      </code>                                                                        */
   IMAGE_PROC  CDATA ColorAverage( CDATA c1, CDATA c2, int d, int max );

   /* Creates an image from user defined parts. The buffer used is
      from the user. This was used by the video library, but
      RemakeImage accomplishes this also.
      Parameters
      pc :      the color buffer to use for the image.
      width :   how wide the color buffer is
      height :  How tall the color buffer is                       */
   IMAGE_PROC  Image IMAGE_API BuildImageFileEx ( PCOLOR pc, _32 width, _32 height DBG_PASS); 
   /* <combine sack::image::MakeImageFile>
      
      Adds <link sack::DBG_PASS, DBG_PASS> parameter. */
   /* Creates an Image with a specified width and height. The
      image's color is undefined to start.
      
      
      Parameters
      Width :     how wide to make the image. Cannot be negative.
      Height :    how tall to make the image. Cannot be negative.
      DBG_PASS :  _nt_
      
      Example
      See <link sack::image::Image, Image>                        */
   IMAGE_PROC  Image IMAGE_API MakeImageFileEx  (_32 Width, _32 Height DBG_PASS);
   /* Creates a sub image region on an image. Sub-images may be
      used like any other image. There are two uses for this sort
      of thing. OH, the sub image shares the exact data of the
      parent image, and is not a copy.
      Parameters
      pImage :  image to make the sub image in
      x :       signed location of the top side of the sub\-image
      y :       signed location of the left side of the sub\-image
      width :   how wide to make the sub\-image
      height :  how tall to make the sub\-image
      
      Returns
      NULL if the input image is NULL.
      
      Otherwise returns an Image.
      Example
      Use 1: An image might contain a grid of symbols or
      characters, each exactly the same size. These may be token
      peices used in a game or a special graphic font.
      <code lang="c++">
      Image peices_image = LoadImageFile( "Game Peices.image" );
      PLIST peices = NULL;
      int x, y;
      \#define PEICE_WIDTH 32
      \#define PEICE_HEIGHT 32
      for( x = 0; x \< 10; x++ )
         for( y = 0; y \< 2; y++ )
         {
             AddLink( &amp;peices, MakeSubImage( peices_image
                                           , x * PEICE_WIDTH, y * PEICE_HEIGHT
                                           , PEICE_WIDTH, PEICE_HEIGHT );
         }
      
      // at this point there we have a list with all the tokens,
      // which were 32x32 pixels each.
      // Any of these peice images may be output using a scaled or direct blot.
      </code>
      
      Use 2: Partitioning views on an image for things like
      controls and other clipped regions.
      <code lang="c++">
      Image image = MakeImageFile( 1024, 768 );
      Image clock = MakeSubImage( image, 32, 32, 150, 16 );
      
      DrawString( clock, 0, 0, BASE_COLOR_WHITE, BASE_COLOR_BLACK, "Current Time..." );
      </code>                                                                           */
   IMAGE_PROC  Image IMAGE_API MakeSubImageEx   ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
   /* Adds an image as a sub-image of another image. The image
      being added as a sub image must not already have a parent.
      Sub-images are like views into the parent, and share the same
      pixel buffer that the parent has.
      Parameters
      pFoster :  This is the parent image to received the new
                 subimage
      pOrphan :  this is the subimage to be added                   */
   IMAGE_PROC  void IMAGE_API  AdoptSubImage    ( Image pFoster, Image pOrphan );
   /* Removes a sub-image (child image) from a parent image. The
      sub image my then be moved to another image with
      AdoptSubImage.
      Parameters
      pImage :  the sub\-image to orphan.                        */
   IMAGE_PROC  void IMAGE_API  OrphanSubImage   ( Image pImage );
   /* Create or recreate an image using the specified color buffer,
      and size. All sub-images have their color data reference
      updated.
      Example
      <code>
      Image image = NULL;
      
      POINTER data = NewArray( CDATA, 100* 100 );
      image = RemakeImage( image, data, 100, 100 );
      
      
      
      </code>
      Remarks
      If the source image is NULL, a new image will be built using
      the color buffer and size specified.
      
      Image.flags has IF_FLAG_EXTERN_COLORS set if made this way,
      since the color buffer is an external resource. This causes
      UnmakeImage() to not attempt to free the color buffer.
      
      If the original image does exist, its color buffer is swapped
      for the one specified, and coordinates are updated. The video
      system uses this to create an image that has the color data
      surface the surface of the display.
      See Also
      <link sack::image::BuildImageFile, BuildImageFile>
      
      GetDisplayImage
      Parameters
      data :    Pointer to a buffer of 32 bit color data. ARGB and
                ABGR available via compile option.
      width :   the width of the data in pixels.
      height :  the height of the data in pixels.
      Returns
      \Returns the original image if not NULL, otherwise results
      with an image who's color plane is defined by a user defined
      buffer of width by height size. The user must have allocated
      this buffer appropriately, and is responsible for its
      destruction.                                                  */
   IMAGE_PROC  Image IMAGE_API RemakeImageEx    ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS);
   /* Load an image file. Today we support PNG, JPG, GIF, BMP.
      Tomorrow consider tapping into that FreeImage project on
      sourceforge, that combines all readers into one.
      Parameters
      name :      Filename to read from. Opens in 'Current Directory'
                  if not an absolute path.
      DBG_PASS :  _nt_
      Example
      See <link sack::image::Image, Image>                            */
	IMAGE_PROC  Image IMAGE_API LoadImageFileEx  ( CTEXTSTR name DBG_PASS );

	/* <combinewith sack::image::LoadImageFileEx@CTEXTSTR name>
	   
	   Extended load image file. This allows specifying a file group
	   to load from. (Groups were added for platforms without
	   support of current working directory).
	   
	   
	   Parameters
	   group :  Group to load the file from
	   _nt_ :   _nt_                                                 */
	IMAGE_PROC Image  IMAGE_API LoadImageFileFromGroupEx ( INDEX group, CTEXTSTR filename DBG_PASS );

   /* Decodes a block of memory into an image. This is used
      internally so, LoadImageFile() opens the file and reads it
      into a buffer, which it then passes to DecodeMemoryToImage().
      Images stored in custom user structures may be passed for
      decoding also.
      Parameters
      buf :   Pointer to bytes of data to decode
      size :  the size of the buffer to decode
      Returns
      NULL is returned if the data does not decode as an image.
      
      an Image is returned otherwise.
      Example
      This pretends that you have a FILE* open to some image
      already, and that the image is tiny (less than 4k bytes).
      <code lang="c#">
      char buffer[4096];
      int length;
      length = fread( buffer, 1, 4096, some_file );
      
      Image image = DecodeMemoryToImage( buffer, length );
      if( image )
      {
         // buffer decoded okay.
      }
      </code>                                                       */
			IMAGE_PROC  Image IMAGE_API  DecodeMemoryToImage ( P_8 buf, _32 size );
#ifdef __cplusplus
		namespace loader{
#endif
	IMAGE_PROC  LOGICAL IMAGE_API  PngImageFile ( Image image, P_8 *buf, size_t *size );
	IMAGE_PROC  LOGICAL IMAGE_API  JpgImageFile ( Image image, P_8 *buf, size_t *size, int Q );
#ifdef __cplusplus
		}
#endif
      /* direct hack for processing clipboard data... probably does some massaging of the databefore calling DecodeMemoryToImage */
   IMAGE_PROC  Image IMAGE_API  ImageRawBMPFile (_8* ptr, _32 filesize); 

	/* Releases an image, has extra debug parameters.
	   Parameters
	   Image :     the Image to release.
	   DBG_PASS :  Adds <link sack::DBG_PASS, DBG_PASS> parameter for
	               the release memory tracking.                       */
	IMAGE_PROC  void IMAGE_API UnmakeImageFileEx ( Image pif DBG_PASS );
   /* Sets the active image rectangle to the bounding rectangle
      specified. This can be used to limit artificially drawing
      onto an image. (It is easier to track to create a subimage in
      the location to draw instead of masking with a bound rect,
      which has problems restoring back to initial conditions)
      Parameters
      pImage :  Image to set the drawing clipping rectangle.
      bound :   a pointer to an IMAGE_RECTANGLE to set the image
                boundaries to.                                      */
   IMAGE_PROC  void  IMAGE_API SetImageBound    ( Image pImage, P_IMAGE_RECTANGLE bound );
// reset clip rectangle to the full image (subimage part )
// Some operations (move, resize) will also reset the bound rect, 
// this must be re-set afterwards.  
// ALSO - one SHOULD be nice and reset the rectangle when done, 
// otherwise other people may not have checked this.

/* Change the size of an image, reallocating the color buffer as
   necessary.
   
   <b>Parameters</b>
   
   <b>Remarks</b>
   
   If the image is a sub image (located within a parent), the
   subimage view on the parent image is updated to the new width
   and height. The color buffer remains the parent's buffer.
   
   If the image is a parent, a new buffer is allocated. If the
   previous buffer was specified by the user in RemakeImage,
   that buffer is not freed, but a new buffer is still created.
   
   
   
   <b>Bugs</b>
   
   If the image is a parent image, the child images are not
   updated to the newly allocated buffer. Resize works really
   well for subimages though.                                    */
   IMAGE_PROC  void IMAGE_API ResizeImageEx     ( Image pImage, S_32 width, S_32 height DBG_PASS);
   /* Moves an image within a parent image. Top level images and
      images which have a user color buffer do not move.
      Parameters
      pImage :  The image to move.
      x :       the new X coordinate of the image.
      y :       the new Y coordinate of the image.               */
   IMAGE_PROC  void IMAGE_API MoveImage         ( Image pImage, S_32 x, S_32 y );

//-----------------------------------------------------

   IMAGE_PROC  void IMAGE_API BlatColor         ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );
   /* Blat is the sound a trumpet makes when it spews forth
      noise... so Blat color is just fill a rectangle with a color,
      quickly. Apply alpha transparency of the color specified.
      Parameters
      pifDest :  The destination image to fill the rectangle on
      x :        left coordinate of the rectangle
      y :        right coordinate of the rectangle
      w :        width of the rectangle
      h :        height of the rectangle
      color :    color to fill the rectangle with. The alpha of this
                 color will be applied.                              */
   IMAGE_PROC  void IMAGE_API BlatColorAlpha    ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );

   /* \ \ 
      Parameters
      pDest :         destination image (the one to copy to)
      pIF :           source image 
      x :             destination top coordinate
      y :             destination left coordinate
      nTransparent :  <link sack::image::AlphaModifier, Alpha Operation>
      method :        <link sack::image::blot_methods, Blot Method>
      _nt_ :          _nt_                                               */
   IMAGE_PROC  void IMAGE_API BlotImageEx       ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... ); 
   /* Copies an image from one image onto another. The copy is done
      directly and no scaling is applied. If a width or height
      larget than the image to copy is specified, only the amount
      of the image that is valid is copied.
      
      
      Parameters
      pDest :         Destination image
      pIF :           Image file to copy
      x :             X position to put copy at
      y :             Y position to put copy at
      xs :            X position to copy from.
      ys :            Y position to copy from.
      wd :            how much of the image horizontally to copy
      ht :            how much of the image vertically to copy
      nTransparent :  <link sack::image::AlphaModifier, Alpha Transparency method>
      method :        <link sack::image::blot_methods, BlotMethods>
      <b>Method == BLOT_SHADED extra parameters</b>
      red :    Color to use the red channel to output the scale from
               black to color
      green :  Color to use the red channel to output the scale from
               black to color
      blue :   Color to use the red channel to output the scale from
               black to color 
      <b>Method == BLOT_SHADED extra parameters</b>
      shade :  _nt_
      
      See Also                                                                     */
   IMAGE_PROC  void IMAGE_API BlotImageSizedEx  ( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... );

   /* Copies some or all of an image to a destination image of
      specified width and height. This does linear interpolation
      scaling.
      
      
      
      There are simple forms of this function as macros, since
      commonly you want to output the entire image, a macro which
      automatically sets (0,0),(width,height) as the source
      \parameters to output the whole image exists.
      Parameters
      \ \ 
      pifDest :       Destination image
      pifSrc :        image to copy from
      xd :            destination x coordinate
      yd :            destination y coordinate
      wd :            destination width (source image width will be
                      scaled to this)
      hd :            destination height (source image height will
                      be scaled to this)
      xs :            source x coordinate (where to copy from)
      ys :            source y coordinate (where to copy from)
      ws :            source width (how much of the image to copy)
      hs :            source height (how much of the image to copy)
      nTransparent :  Alpha method...
      method :        specifies how the source color data is
                      transformed if at all. See BlotMethods
      ... :           possible extra parameters depending on method
      <b>Method == BLOT_MULTISHADE extra parameters</b>
      red :    Color to use the red channel to output the scale from
               black to color
      green :  Color to use the red channel to output the scale from
               black to color
      blue :   Color to use the red channel to output the scale from
               black to color
      <b>Method == BLOT_SHADED extra parameters</b>
      shade :  _nt_
      
      See Also
      <link sack::image::AlphaModifier, Alpha Methods>
      
      <link sack::image::blot_methods, Blot Methods>
      
      
      
      <link sack::image::BlotScaledImage, BlotScaledImage>
      
      <link sack::image::BlotScaledImageShaded, BlotScaledImageShaded>
      
      <link sack::image::BlotScaledImageShadedAlpha, BlotScaledImageShadedAlpha>
      
      
      
      
                                                                                 */
   IMAGE_PROC  void IMAGE_API BlotScaledImageSizedEx( Image pifDest, Image pifSrc
                                   , S_32 xd, S_32 yd
                                   , _32 wd, _32 hd
                                   , S_32 xs, S_32 ys
                                   , _32 ws, _32 hs
                                   , _32 nTransparent
                                   , _32 method, ... );


/* Your basic PLOT functions (Image.C, plotasm.asm)
   
   A function pointer to the function which sets a pixel in an
   image at a specified x, y coordinate.
   Parameters
   Image :  The image to get the pixel from
   X :      x coordinate to get pixel color
   Y :      y coordinate to get pixel color
   Color :  color to put at the coordinate. image will be set
            exactly to this color, and whatever the alpha of the
            color is.                                            */
   IMAGE_PROC  void plot       ( Image pi, S_32 x, S_32 y, CDATA c );
   /* A function pointer to the function which sets a pixel in an
      image at a specified x, y coordinate.
      Parameters
      Image :  The image to get the pixel from
      X :      x coordinate to get pixel color
      Y :      y coordinate to get pixel color
      Color :  color to put at the coordinate. Alpha blending will be
               done.                                                  */
   IMAGE_PROC  void plotalpha  ( Image pi, S_32 x, S_32 y, CDATA c );
   /* A function pointer to the function which gets a pixel from an
      image at a specified x, y coordinate.
      Parameters
      Image :  The image to get the pixel from
      X :      x coordinate to get pixel color
      Y :      y coordinate to get pixel color
      
      Returns
      CDATA color in the Image at the specified coordinate.         */
   IMAGE_PROC  CDATA getpixel  ( Image pi, S_32 x, S_32 y );
//-------------------------------
// Line functions  (lineasm.asm) // should include a line.c ... for now core was assembly...
//-------------------------------
   IMAGE_PROC  void do_line      ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  // d is color data...
   IMAGE_PROC  void do_lineAlpha ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color);  // d is color data...

   /* This is a function pointer that references a function to do
      optimized horizontal lines. The function pointer is updated
      when SetBlotMethod() is called.
      Parameters
      Image :   the image to draw to
      Y :       the y coordinate of the line (how far down from top to
                draw it)
      x_from :  X coordinate to draw from
      x_to :    X coordinate to draw to
      color :   the color of the line. This color will be set to the
                surface, the alpha result will be the alpha of this
                color.                                                 */
   IMAGE_PROC  void do_hline      ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
   /* This is a function pointer that references a function to do
      optimized vertical lines. The function pointer is updated
      when SetBlotMethod() is called.
      
      
      Parameters
      Image :   the image to draw to
      X :       the x coordinate of the line (how far over to draw
                it)
      y_from :  Y coordinate to draw from
      y_to :    Y coordinate to draw to
      color :   the color of the line. This color will be set to the
                surface, the alpha result will be the alpha of this
                color.                                               */
   IMAGE_PROC  void do_vline      ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
   /* This is a function pointer that references a function to do
      optimized horizontal lines with alpha blending. The function
      pointer is updated when SetBlotMethod() is called.
      Parameters
      Image :   the image to draw to
      Y :       the Y coordinate of the line (how far down from top
                of image to draw it)
      x_from :  X coordinate to draw from
      x_to :    X coordinate to draw to
      color :   the color of the line (alpha component of the color
                will be applied)                                    */
   IMAGE_PROC  void do_hlineAlpha ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
   /* This is a function pointer that references a function to do
      optimized vertical lines with alpha blending. The function
      pointer is updated when SetBlotMethod() is called.
      
      
      Parameters
      Image :   the image to draw to
      X :       the x coordinate of the line (how far over to draw
                it)
      y_from :  Y coordinate to draw from
      y_to :    Y coordinate to draw to
      color :   the color of the line (alpha component of the color
                will be applied)                                    */
   IMAGE_PROC  void do_vlineAlpha ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
	/* routine which iterates through the points along a lone from
	   x,y to xto,yto, calling a user function at each point.
	   Parameters
	   Image :  the image to pretend to draw on
	   x :      draw from this x coordinate
	   y :      draw from this y coordinate
	   xto :    draw to this x coordinate
	   yto :    draw to this y coordinate
	   d :      userdata (color data)
	   func :   user callback function to a function of type...<p />void
	            func( Image pif, S_32 x, S_32 y, int d ) ;
	   
	   Remarks
	   The Image passed does not HAVE to be an Image, it can be any
	   user POINTER.
	   
	   The data passed is limited to 32 bits, and will not hold a
	   pointer if built for 64 bit platform.
	   Example
	   <code lang="c++">
	   Image image;
	   
	   void MyPlotter( Image image, S_32 x, S_32 y, CDATA color )
	   {
	       // do something with the image at x,y
	   }
	   
	   void UseMyPlotter( Image image )
	   {
	       do_lineExV( image, 10, 10, 80, 80, BASE_COLOR_BLACK, MyPlotter );
	   }
	   </code>                                                               */
	IMAGE_PROC  void do_lineExV    ( Image pImage, S_32 x, S_32 y
									, S_32 xto, S_32 yto, PTRSZVAL color
		                            , void (*func)( Image pif, S_32 x, S_32 y, PTRSZVAL d ) );
   /* \Returns the correct SFTFont pointer to the default font. In all
      font functions, NULL may be used as the font, and this is the
      font that will be used.
      Parameters
      None.
      Example
      <code lang="c++">
      SFTFont font = GetDefaultFont();
      </code>                                                       */
   IMAGE_PROC  SFTFont IMAGE_API GetDefaultFont ( void );
   /* \Returns the height of a font for purposes of spacing between
      lines. Characters may render outside of this height.
      
      
      Parameters
      SFTFont :  SFTFont to get the height of. if NULL returns an internal
              font's height.
      
      Returns
      the height of the font.                                        */
   IMAGE_PROC  _32  IMAGE_API GetFontHeight  ( SFTFont );
   /* \Returns the approximate rectangle that would be used for a
      string. It only counts using the line measurement. Newlines
      in strings count to wrap text to subsequent lines and start
      recounting the width, returning the maximum length of string
      horizontally.
      Parameters
      pString :  The string to measure
      len :      the length of characters to count in string
      width :    a pointer to a _32 to receive the width of the
                 string
      height :   a pointer to a _32 to receive the height of the
                 string
      UseFont :  A SFTFont to use. 
      
      Returns
      \Returns the width parameter. If NULL are passed for width
      and height, this is OK. One of the simple macros just passes
      the string and gets the return - this is for how wide the
      string would be.                                             */
   IMAGE_PROC  _32  IMAGE_API GetStringSizeFontEx( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont );
   /* Fill the width and height with the actual size of the string
      as it is drawn. (may be above or below the original
      rectangle)
      Parameters
      pString :     the string to measure
      nLen :        the number of characters in the string
      width :       a pointer to a 32 bit value to get resulting
                    width
      height :      a pointer to a 32 bit value to get resulting
                    height
      charheight :  the actual height of the characters (as reports
                    by line)
      UseFont :     a SFTFont to use. If NULL use a default internal
                    font.                                           */
   IMAGE_PROC  _32 IMAGE_API  GetStringRenderSizeFontEx ( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

// background of color 0,0,0 is transparent - alpha component does not
// matter....
   IMAGE_PROC  void IMAGE_API PutCharacterFont              ( Image pImage
                                                  , S_32 x, S_32 y
                                                  , CDATA color, CDATA background,
                                                   TEXTCHAR c, SFTFont font );
   /* Outputs a string in the specified font, from the specified
      point, text is drawn from the point up, with the characters
      aligned with the top to the left; it goes up from the point.
      the point becomes the bottom left of the rectangle output.
      Parameters
      pImage :      image to draw string into
      x :           x position of the string
      y :           y position of the string
      color :       color of the data drawn in the font
      background :  color of the data not drawn in the font
      c :           the character to output
      font :        the font to use. NULL use an internal default
                    font.                                          */
   IMAGE_PROC  void IMAGE_API PutCharacterVerticalFont      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
   /* Outputs a string in the specified font, from the specified
      point, text is drawn from the point to the left, with the
      characters aligned with the top to the left; it goes up from
      the point. the point becomes the bottom left of the rectangle
      \output.
      Parameters
      pImage :      image to draw string into
      x :           x position of the string
      y :           y position of the string
      color :       color of the data drawn in the font
      background :  color of the data not drawn in the font
      pc :          pointer to constant text
      nLen :        length of text to output
      font :        the font to use. NULL use an internal default
                    font.                                           */
   IMAGE_PROC  void IMAGE_API PutCharacterInvertFont        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
   /* Outputs a character in the specified font, from the specified
      point, text is drawn from the point up, with the characters
      aligned with the top to the left; it goes up from the point. the
      point becomes the bottom left of the rectangle output.
      Parameters
                                                                       */
   IMAGE_PROC  void IMAGE_API PutCharacterVerticalInvertFont( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );

   /* Outputs a string in the specified font, from the specified
      point, text is drawn right side up and godes from left to
      right. The point becomes the top left of the rectangle
      \output.
      Parameters
      pImage :      image to draw string into
      x :           x position of the string
      y :           y position of the string
      color :       color of the data drawn in the font
      background :  color of the data not drawn in the font
      pc :          pointer to constant text
      nLen :        length of text to output
      font :        the font to use. NULL use an internal default
                    font.                                         */
   IMAGE_PROC  void IMAGE_API PutStringFontEx              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
   /* justification 0 is left, 1 is right, 2 is center */
   IMAGE_PROC  void IMAGE_API PutStringFontExx              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font, int justication, _32 _width );
   /* Outputs a string in the specified font, from the specified
      point, text is drawn from the point down, with the characters
      aligned with the top to the right; it goes down from the
      point. the point becomes the top right of the rectangle
      \output.
      Parameters
      pImage :      image to draw string into
      x :           x position of the string
      y :           y position of the string
      color :       color of the data drawn in the font
      background :  color of the data not drawn in the font
      pc :          pointer to constant text
      nLen :        length of text to output
      font :        the font to use. NULL use an internal default
                    font.                                           */
   IMAGE_PROC  void IMAGE_API PutStringVerticalFontEx      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

   /* Outputs a string in the specified font, from the specified
      point, text is drawn upside down, and goes to the left from
      the point. the point becomes the bottom right of the
      rectangle output.
      Parameters
      pImage :      image to draw string into
      x :           x position of the string
      y :           y position of the string
      color :       color of the data drawn in the font
      background :  color of the data not drawn in the font
      pc :          pointer to constant text
      nLen :        length of text to output
      font :        the font to use. NULL use an internal default
                    font.                                         */
   IMAGE_PROC  void IMAGE_API PutStringInvertFontEx        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
   /* Outputs a string in the specified font, from the specified
      point, text is drawn from the point up, with the characters
      aligned with the top to the left; it goes up from the point. the
      point becomes the bottom left of the rectangle output.
      Parameters
      pImage :      image to draw string into
      x :           x position of the string
      y :           y position of the string
      color :       color of the data drawn in the font
      background :  color of the data not drawn in the font
      pc :          pointer to constant text
      nLen :        length of text to output
      font :        the font to use. NULL use an internal default
                    font.                                              */
   IMAGE_PROC  void IMAGE_API PutStringInvertVerticalFontEx( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

   //_32 (*PutMenuStringFontEx)            ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, SFTFont font );
   //_32 (*PutCStringFontEx)               ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, SFTFont font );
   IMAGE_PROC  _32 IMAGE_API  GetMaxStringLengthFont  ( _32 width, SFTFont UseFont );

   /* Used as a proper accessor method to get an image's width and
      height. Decided to allow the image structure to be mostly
      public, so the first 4 members are the images x,y, width and
      height, and are immediately accessable by the Image pointer.
      Parameters
      pImage :  image to get the size of
      width :   pointer to a 32 bit unsigned value to result with the
                width, if NULL ignored.
      height :  pointer to a 32 bit unsigned value to result with the
                height, if NULL ignored.                              */
   IMAGE_PROC  void IMAGE_API  GetImageSize            ( Image pImage, _32 *width, _32 *height );
   /* \Returns the pointer to the color buffer currently used
      \internal to the image.
      Parameters
      pImage :  Image to get the surface of.
      
      Example
      <code lang="c#">
      Image image = MakeImageFile( 100, 100 );
      PCDATA pointer_color_data = GetImageSurface( image );
      </code>
      Note
      This might be used to do an optimized output routine. Drawing
      to the image with plot and line are not necessarily the best
      for things like circles. Provides ability for user to output
      directly to the color buffer.                                 */
   IMAGE_PROC  PCDATA IMAGE_API  GetImageSurface        ( Image pImage );

   // would seem silly to load fonts - but for server implementations
   // the handle received is not the same as the font sent.
   IMAGE_PROC  SFTFont IMAGE_API  LoadFont                ( SFTFont font );
   /* Destroys a font, releasing all resources associated with
      character data and font rendering.                       */
   IMAGE_PROC  void IMAGE_API  UnloadFont              ( SFTFont font );
	/* This is a function used to synchronize image operations when
	   the image interface is across a message server.              */
	IMAGE_PROC  void IMAGE_API  SyncImage                  ( void );
	// intersect rectangle, results with the overlapping portion of R1 and R2
   // into R ...
   IMAGE_PROC  int IMAGE_API  IntersectRectangle ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* Merges two image rectangles. The resulting rectangle is a
      rectangle that includes both rectangles.
      Parameters
      r :   Pointer to an IMAGE_RECTANGLE for the result.
      r1 :  PIMAGE_RECTANGLE one rectangle.
      r2 :  PIMAGE_RECTANGLE the other rectangle.               */
   IMAGE_PROC  int IMAGE_API  MergeRectangle ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* User applications may use an aux rect attatched to an image. The
      'Display' render library used this itself however, making
      this mostly an internal feature.
      Parameters
      pImage :  image to get the aux rect of.
      pRect :   pointer to an IMAGE_RECTANGLE to get the aux
                rectangle data in.                                     */
   IMAGE_PROC  void IMAGE_API  GetImageAuxRect    ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* User applications may use an aux rect attatched to an image.
      The 'Display' render library used this itself however, making
      this mostly an internal feature.
      Parameters
      pImage :  image to set the aux rect of.
      pRect :   pointer to an IMAGE_RECTANGLE to set the aux
                rectangle to.                                       */
   IMAGE_PROC  void IMAGE_API  SetImageAuxRect    ( Image pImage, P_IMAGE_RECTANGLE pRect );

	/* \ \ 
	   Parameters
	   Filename :  \file name of image to load. Converts image into
	               sprite automatically, resulting with a sprite.
	   DBG_PASS :  See <link sack::DBG_PASS, DBG_PASS>              */
		IMAGE_PROC  PSPRITE IMAGE_API  MakeSpriteImageFileEx ( CTEXTSTR fname DBG_PASS );
      /* create a sprite from an Image */
	IMAGE_PROC  PSPRITE IMAGE_API  MakeSpriteImageEx ( Image image DBG_PASS );
	/* Release a Sprite. */
	IMAGE_PROC  void IMAGE_API  UnmakeSprite ( PSPRITE sprite, int bForceImageAlso );
	// angle is a fixed scaled integer with 0x1 0000 0000 being the full circle.
	IMAGE_PROC  void IMAGE_API  rotate_scaled_sprite (Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height );

   /* output a rotated sprite to destination image, using and angle specified.  The angle is represented as 0x1 0000 0000 is 360 degrees */
	IMAGE_PROC  void IMAGE_API  rotate_sprite (Image bmp, PSPRITE sprite, fixed angle);

   /* output a sprite at its current location */
	IMAGE_PROC  void IMAGE_API  BlotSprite ( Image pdest, PSPRITE ps ); 
/* Sets the point on a sprite which is the 'hotspot' the hotspot
   is the point that is drawn at the specified coordinate when
   outputting a sprite.
   Parameters
   sprite :  The PSPRITE to set the hotspot of.
   x :       x coordinate in the sprite's image that becomes the
             hotspot.
   y :       y coordinate in the sprite's image that becomes the
             hotspot.                                            */
IMAGE_PROC  PSPRITE IMAGE_API  SetSpriteHotspot ( PSPRITE sprite, S_32 x, S_32 y );
/* This function sets the current location of a sprite. When
   asked to render, the sprite will draw itself here.
   Parameters
   sprite :  the sprite to move
   x :       the new x coordinate of the parent image to draw at
   y :       the new y coordinate of the parent image to draw at */
IMAGE_PROC  PSPRITE IMAGE_API  SetSpritePosition ( PSPRITE sprite, S_32 x, S_32 y );

/* Use a font file to get a font that can be used for outputting
   characters and strings.
   Parameters
   file\ :    Filename of a font to render.
   nWidth :   desired width in pixels to render the font.
   nHeight :  desired height in pixels to render the font.
   flags :    0 = render mono. 2=render 2 bits, 3=render 8 bit.  */
IMAGE_PROC  SFTFont IMAGE_API  InternalRenderFontFile ( CTEXTSTR file
																		, S_32 nWidth
																		, S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
																		, _32 flags
																		);
/* Rerender the current font with a new size. */
IMAGE_PROC void IMAGE_API RerenderFont( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );

	/* Dumps the whole cache to log file, shows family, style, path and filename.
    Is the same sort of dump that OpenFontFile uses.
	 */
IMAGE_PROC void IMAGE_API DumpFontCache( void );

#ifndef INTERNAL_DUMP_FONT_FILE
/* takes a font and dumps a header-file formatted file; then the font can be
 statically built into code. */
IMAGE_PROC void IMAGE_API DumpFontFile( CTEXTSTR name, SFTFont font_to_dump );
#endif

/* Creates a font based on indexes from the internal font cache.
   This is used by the FontPicker dialog to choose a font. The
   data the dialog used to render the font is available to the
   application, and may be passed back for rendering a font
   without knowing specifically what the values mean.
   Parameters
   nFamily :  The number of the family in the cache.
   nStyle :   The number of the style in the cache.
   nFile :    The number of the file in the cache.
   nWidth :   the width to use for rendering characters (in
              pixels)
   nHeight :  the height to use for rendering characters (in
              pixels)
   flags :    0 = render mono. 2=render 2 bits, 3=render 8 bit.
   
   Returns
   A SFTFont which can be used to output. If the file exists. NULL
   on failure.
   Example
   Used internally for FontPicker dialog, see <link sack::image::InternalRenderFontFile@CTEXTSTR@S_32@S_32@_32, InternalRenderFontFile> */
IMAGE_PROC  SFTFont IMAGE_API  InternalRenderFont ( _32 nFamily
																  , _32 nStyle
																  , _32 nFile
																  , S_32 nWidth
																  , S_32 nHeight
																  , PFRACTION width_scale
																  , PFRACTION height_scale
																  , _32 flags
																  );
/* Releases all resources for a SFTFont.  */
IMAGE_PROC  void IMAGE_API  DestroyFont( SFTFont *font );
/* Get the global font data structure. This is an internal
   structure, and it's definition may not be exported. Currently
   the definition is in documentation.
   See Also
   <link sack::image::FONT_GLOBAL, SFTFont Global>                  */
IMAGE_PROC  struct font_global_tag * IMAGE_API  GetGlobalFonts( void );
// types of data which may result...
typedef struct font_data_tag *PFONTDATA;
/* Information to render a font from a file to memory. */
typedef struct render_font_data_tag *PRENDER_FONTDATA;

/* Recreates a SFTFont based on saved FontData. The resulting font
   may be scaled from its original size.
   Parameters
   pfd :           pointer to font data.
   width_scale :   FRACTION to scale the original font height
                   \description by. if NULL uses the original
                   font's size.
   height_scale :  FRACTION to scale the original font height
                   \description by.  if NULL uses the original
                   font's size.
   
   Example
   <code lang="c++">
   POINTER some_loaded_data; // pretend it is initialized to something valid
   
   SFTFont font = RenderScaledFontData( some_loaded_data, NULL, NULL );
   PutStringFont( image, 0, 0, BASE_COLOR_WHITE, 0, "Hello World", font );
   </code>
   
   Or, maybe your original designed screen was 1024x768, and
   it's now showing on 1600x1200, for the text to remain the
   same...
   <code lang="c++">
   FRACTION width_scale;
   FRACTION height_scale;
   _32 w, h;
   GetDisplaySize( &amp;w, &amp;h );
   SetFraction( width_scale, w, 1024 );
   SetFraction( height_scale, h, 768 );
   
   SFTFont font2 = RenderScaledFontData( some_loaded_data, &amp;width_scale, &amp;height_scale );
   PutStringFont( image, 0, 0, BASE_COLOR_WHITE, 0, "Hello World", font2 );
   </code>                                                                                     */
IMAGE_PROC  SFTFont IMAGE_API  RenderScaledFontData( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale );
/* <combine sack::image::RenderScaledFontData@PFONTDATA@PFRACTION@PFRACTION>
   
   \ \                                                                       */
#define RenderFontData(pfd) RenderScaledFontData( pfd,NULL,NULL )

/* <combinewith sack::image::RenderScaledFontEx@CTEXTSTR@_32@_32@PFRACTION@PFRACTION@_32@size_t *@POINTER *, sack::image::RenderScaledFontData@PFONTDATA@PFRACTION@PFRACTION>
   
   \ \                                                                                                                                                                        */
IMAGE_PROC SFTFont IMAGE_API RenderScaledFontEx( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );
/* Renders a font with a FRACTION scalar for the X and Y sizes.
   
   
   Parameters
   name :          Name of the font (file).
   width :         Original width (in pels) to make the font.
   height :        Original height (in pels) to make the font.
   width_scale :   scalar to apply to the width
   height_scale :  scalar to apply to the height
   flags :         Flags specifying how many bits to render the
                   font with (and other info?) See enum
                   FontFlags.                                   */
IMAGE_PROC SFTFont IMAGE_API RenderScaledFont( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
#define RenderScaledFont(n,w,h,ws,hs) RenderScaledFontEx(n,w,h,ws,hs,NULL,NULL)
/* Renders a font file and returns a SFTFont. The font can then be
   used in string output functions to images.
   Parameters
   file\ :           \File name of a font to render. Any font
                     that freetype supports.
   width :           width of characters to render in.
   height :          height of characters to render.
   flags :           if( ( flags &amp; 3 ) == 3 )<p /> font\-\>flags
                     = FONT_FLAG_8BIT;<p /> else if( ( flags &amp;
                     3 ) == 2 )<p /> font\-\>flags =
                     FONT_FLAG_2BIT;<p /> else<p /> font\-\>flags
                     = FONT_FLAG_MONO;<p />
   pnFontDataSize :  optional pointer to a 32 bit value to
                     receive the size of rendered data.
   pFontData :       The render data. This data can be used to
                     recreate this font.                             */
IMAGE_PROC  SFTFont IMAGE_API  RenderFontFileScaledEx ( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );
/* <combine sack::image::RenderFontFileEx@CTEXTSTR@_32@_32@_32@P_32@POINTER *>
   
   \ \                                                                         */
#define RenderFontFile(file,w,h,flags) RenderFontFileScaledEx(file,w,h,NULL,NULL,flags,NULL,NULL)
#define RenderFontFileEx(file,w,h,flags,a,b) RenderFontFileScaledEx(file,w,h,NULL,NULL,flags,a,b )

		/* This can be used to get the internal description of a font,
		   which the user may then save, and use later to recreate the
		   font the same way.
		   Parameters
		   font :         SFTFont to get the render description from.
		   fontdata :     a pointer to a pointer which will be filled
		                  with a pointer buffer that has the font data.
		   fontdatalen :  a pointer to 32 bit value to receive the length
		                  of data.                                        */
		IMAGE_PROC  int IMAGE_API  GetFontRenderData ( SFTFont font, POINTER *fontdata, size_t *fontdatalen );
// exported for the PSI font chooser to set the data for the font
// to be retreived later when only the font handle remains.
IMAGE_PROC  void IMAGE_API  SetFontRendererData ( SFTFont font, POINTER pResult, size_t size );

#ifndef PSPRITE_METHOD
/* <combine sack::image::PSPRITE_METHOD>
   
   \ \                                   */
#define PSPRITE_METHOD PSPRITE_METHOD
	typedef struct sprite_method_tag *PSPRITE_METHOD;
#endif
	// provided for display rendering portion to define this method for sprites to use.
   // deliberately out of namespace... please do not move this up.
IMAGE_PROC  void IMAGE_API  SetSavePortion ( void (CPROC*_SavePortion )( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h ) );

/* \Returns the red channel of the color
   Parameters
   color :  Color to get the red channel of.
   
   Returns
   The COLOR_CHANNEL (byte) of the red channel in the color. */
IMAGE_PROC COLOR_CHANNEL IMAGE_API GetRedValue( CDATA color ) ;
/* \Returns the green channel of the color
   Parameters
   color :  Color to get the green channel of.
   
   Returns
   The COLOR_CHANNEL (byte) of the green channel in the color. */
IMAGE_PROC COLOR_CHANNEL IMAGE_API GetGreenValue( CDATA color );
/* \Returns the blue channel of the color
   Parameters
   color :  Color to get the blue channel of.
   
   Returns
   The COLOR_CHANNEL (byte) of the blue channel in the color. */
IMAGE_PROC COLOR_CHANNEL IMAGE_API GetBlueValue( CDATA color );
/* \Returns the alpha channel of the color
   Parameters
   color :  Color to get the alpha channel of.
   
   Returns
   The COLOR_CHANNEL (byte) of the alpha channel in the color. */
IMAGE_PROC COLOR_CHANNEL IMAGE_API GetAlphaValue( CDATA color );
/* Sets the red channel in a color value.
   Parameters
   color :  Original color to modify
   b :      new red channel value         */
IMAGE_PROC CDATA IMAGE_API SetRedValue( CDATA color, COLOR_CHANNEL r ) ;
/* Sets the green channel in a color value.
   Parameters
   color :  Original color to modify
   g :      new green channel value         */
IMAGE_PROC CDATA IMAGE_API SetGreenValue( CDATA color, COLOR_CHANNEL green );
/* Sets the blue channel in a color value.
   Parameters
   color :  Original color to modify
   b :      new blue channel value         */
IMAGE_PROC CDATA IMAGE_API SetBlueValue( CDATA color, COLOR_CHANNEL b );
/* Sets the alpha channel in a color value.
   Parameters
   color :  Original color to modify
   a :      new alpha channel value         */
IMAGE_PROC CDATA IMAGE_API SetAlphaValue( CDATA color, COLOR_CHANNEL a );
/* Makes a CDATA color from the RGB components. Sets the alpha
   as 100% opaque.
   Parameters
   r :      red channel of new color
   green :  green channel of new color
   b :      blue channel of new color                          */
IMAGE_PROC CDATA IMAGE_API MakeColor( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
/* Create a CDATA color from components.
   Parameters
   r :      Red channel value
   green :  green channel value
   b :      blue channel value
   a :      alpha channel value
   
   Returns
   A CDATA representing the color specified. */
IMAGE_PROC CDATA IMAGE_API MakeAlphaColor( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


/* With 3d renderer, images have a transformation matrix. This
   function allows you to get the transformation matrix.
   Parameters
   pImage :  image to get the transformation matrix of.        */
IMAGE_PROC  PTRANSFORM IMAGE_API GetImageTransformation( Image pImage );

enum image_translation_relation
{
   IMAGE_TRANSFORM_RELATIVE_CENTER = 0,
   IMAGE_TRANSFORM_RELATIVE_LEFT,
   IMAGE_TRANSFORM_RELATIVE_RIGHT,
   IMAGE_TRANSFORM_RELATIVE_TOP,
   IMAGE_TRANSFORM_RELATIVE_BOTTOM,
   IMAGE_TRANSFORM_RELATIVE_TOP_LEFT,
   IMAGE_TRANSFORM_RELATIVE_TOP_RIGHT,
   IMAGE_TRANSFORM_RELATIVE_BOTTOM_LEFT,
   IMAGE_TRANSFORM_RELATIVE_BOTTOM_RIGHT,
   IMAGE_TRANSFORM_RELATIVE_OTHER // only mode that uses the 'aux' parameter of SetImageTransformRelation
};
/*
 This sets flags on the image, so when it's called for rendering to the screen
 this is how
    */
IMAGE_PROC  void IMAGE_API SetImageTransformRelation( Image pImage, enum image_translation_relation relation, PRCOORD aux );

/*
 This just draws the image into the current 3d context.
 This is a point-sprite engine too....
 It does not setup anything about rendering this, just generates the texture at the right coords.
 Parameters
 render_pixel_scaled : when drawing, reverse compute from the angle of the view, and the depth of the thing to scale orthagonal, but at depth.  (help 3d vision)
 */
IMAGE_PROC  void IMAGE_API Render3dImage( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );

IMAGE_PROC  void IMAGE_API Render3dText( CTEXTSTR string, int characters, CDATA color, SFTFont font, PCVECTOR o, LOGICAL render_pixel_scaled );

/* 
  Utilized by fonts with images with reverse_interface set to transfer child images;
  may be generally useful; but had to be exposed through interface
  Might be a shallow move....
 */
IMAGE_PROC  void IMAGE_API TransferSubImages( Image pImageTo, Image pImageFrom );
IMAGE_PROC  LOGICAL IMAGE_API IsImageTargetFinal( Image image );

/* These flags are used in SetImageRotation and RotateImageAbout
   functions - these are part of the 3D driver interface
   extension. They allow for controlling how the rotation is
   performed.                                                    */
enum image_rotation_flags {
	IMAGE_ROTATE_FLAG_CENTER = 0, // relative to center of image (center if not left, right, top or bottom )
   IMAGE_ROTATE_FLAG_TOP, // relative to top edge (center if not left or right)
   IMAGE_ROTATE_FLAG_LEFT, // relative to left edge (center if not top or bottom)
   IMAGE_ROTATE_FLAG_RIGHT, // relative to right edge (center if not top or bottom)
   IMAGE_ROTATE_FLAG_BOTTOM, // relative to bottom edge (center if not left or right )
	IMAGE_ROTATE_FLAG_ADD_CUSTOM_OFFSET // use the offset relative to the image orientation
};
/* Sets the rotation matrix of an image to an arbitrary
   yaw/pitch/roll coordinate.
   
   
   Parameters
   pImage :     Image to rotate
   edge_flag :  what edge the rotation is relative to
   offset_x :   offset from the edge to get the center
   offset_y :   offset from the edge to get the center
   rx :         rotation about x axis (horizontal)
   ry :         rotation about y axis (vertical)
   rz :         rotation about z axis (into screen)     */
IMAGE_PROC void IMAGE_API SetImageRotation( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
/* Allows arbitrary rotation of an image in 3d render mode.
   Parameters
   pImage :     image to rotate
   edge_flag :  see enum image_rotation_flags
   offset_x :   offset from top left of image to center the
                rotation
   offset_y :   offset from top left of image to center the
                rotation
   vAxis :      axis to rotate around, can be any arbitrary
                direction
   angle :      angle of rotation around the axis.
   
   Remarks
   \See Also <link sack::image::image_rotation_flags, image_rotation_flags Enumeration> */
IMAGE_PROC void IMAGE_API RotateImageAbout( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );


IMAGE_PROC void IMAGE_API MarkImageDirty( Image pImage );


_INTERFACE_NAMESPACE

/* Defines a pointer member of the interface structure. */
#define IMAGE_PROC_PTR(type,name) type (CPROC*_##name)
/* Macro to build function pointer entries in the image
   interface.                                           */
//#define DIMAGE_PROC_PTR(type,name) type (CPROC**_##name)
/* This defines the interface call table. each function
   available in the API is reflected in this interface. It
   provdes a function table so applications don't have to be
   directly linked to the image API. This allows replacing the
   image API.                                                  */
typedef struct image_interface_tag 
{
/* <combine sack::image::SetStringBehavior@Image@_32>
   
   Internal
   Interface index 4                                  */ IMAGE_PROC_PTR( void, SetStringBehavior) ( Image pImage, _32 behavior );
/* <combine sack::image::SetBlotMethod@_32>
   
   \ \ 
   Internal
   Interface index 5                        */ IMAGE_PROC_PTR( void, SetBlotMethod)     ( _32 method );

/*
   Internal
   Interface index 6*/   IMAGE_PROC_PTR( Image,BuildImageFileEx) ( PCOLOR pc, _32 width, _32 height DBG_PASS);
/* <combine sack::image::MakeImageFileEx@_32@_32 Height>
   
   Internal
   Interface index 7*/  IMAGE_PROC_PTR( Image,MakeImageFileEx)  (_32 Width, _32 Height DBG_PASS);
/* <combine sack::image::MakeSubImageEx@Image@S_32@S_32@_32@_32 height>
   
   Internal
   Interface index 8                                                                    */   IMAGE_PROC_PTR( Image,MakeSubImageEx)   ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
/* <combine sack::image::RemakeImageEx@Image@PCOLOR@_32@_32 height>
   
   \ \ 
   
   <b>Internal</b>
   
   Interface index 9                                                */   IMAGE_PROC_PTR( Image,RemakeImageEx)    ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS);
/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
   
   Internal
   Interface index 10                                                   */  IMAGE_PROC_PTR( Image,LoadImageFileEx)  ( CTEXTSTR name DBG_PASS );
/* <combine sack::image::UnmakeImageFileEx@Image pif>
   
   Internal
   Interface index 11                                                 */  IMAGE_PROC_PTR( void,UnmakeImageFileEx) ( Image pif DBG_PASS );
#define UnmakeImageFile(pif) UnmakeImageFileEx( pif DBG_SRC )
//-----------------------------------------------------

/* <combine sack::image::ResizeImageEx@Image@S_32@S_32 height>
   
   Internal
   Interface index 14                                                          */  IMAGE_PROC_PTR( void,ResizeImageEx)     ( Image pImage, S_32 width, S_32 height DBG_PASS);
/* <combine sack::image::MoveImage@Image@S_32@S_32>
   
   Internal
   Interface index 15                                               */   IMAGE_PROC_PTR( void,MoveImage)         ( Image pImage, S_32 x, S_32 y );

//-----------------------------------------------------

/* <combine sack::image::BlatColor@Image@S_32@S_32@_32@_32@CDATA>
   
   Internal
   Interface index 16                                                             */   IMAGE_PROC_PTR( void,BlatColor)     ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );
/* <combine sack::image::BlatColorAlpha@Image@S_32@S_32@_32@_32@CDATA>
   
   Internal
   Interface index 17                                                                  */   IMAGE_PROC_PTR( void,BlatColorAlpha)( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );

/* <combine sack::image::BlotImageEx@Image@Image@S_32@S_32@_32@_32@...>
                                                                                                                   
   Internal
	Interface index 18*/   IMAGE_PROC_PTR( void,BlotImageEx)     ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... );

 /* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>

   Internal
	Interface index 19*/   IMAGE_PROC_PTR( void,BlotImageSizedEx)( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... );

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
  Internal
   Interface index  20                                                                                                        */   IMAGE_PROC_PTR( void,BlotScaledImageSizedEx)( Image pifDest, Image pifSrc
                                   , S_32 xd, S_32 yd
                                   , _32 wd, _32 hd
                                   , S_32 xs, S_32 ys
                                   , _32 ws, _32 hs
                                   , _32 nTransparent
                                   , _32 method, ... );


/*Internal
   Interface index 21*/   IMAGE_PROC_PTR( void,plot)      ( Image pi, S_32 x, S_32 y, CDATA c );
/*Internal
   Interface index 22*/   IMAGE_PROC_PTR( void,plotalpha) ( Image pi, S_32 x, S_32 y, CDATA c );
/*Internal
   Interface index 23*/   IMAGE_PROC_PTR( CDATA,getpixel) ( Image pi, S_32 x, S_32 y );
/*Internal
   Interface index 24*/   IMAGE_PROC_PTR( void,do_line)     ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  // d is color data...
/*Internal
   Interface index 25*/   IMAGE_PROC_PTR( void,do_lineAlpha)( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color);  // d is color data...

/*Internal
   Interface index 26*/   IMAGE_PROC_PTR( void,do_hline)     ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
/*Internal
   Interface index 27*/   IMAGE_PROC_PTR( void,do_vline)     ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
/*Internal
   Interface index 28*/   IMAGE_PROC_PTR( void,do_hlineAlpha)( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
/*Internal
   Interface index 29*/   IMAGE_PROC_PTR( void,do_vlineAlpha)( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );

/* <combine sack::image::GetDefaultFont>
   
   Internal
   Interface index 30                    */   IMAGE_PROC_PTR( SFTFont,GetDefaultFont) ( void );
/* <combine sack::image::GetFontHeight@SFTFont>
   
   Internal
   Interface index 31                                        */   IMAGE_PROC_PTR( _32 ,GetFontHeight)  ( SFTFont );
/* <combine sack::image::GetStringSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@SFTFont>
   
   Internal
   Interface index 32                                                          */   IMAGE_PROC_PTR( _32 ,GetStringSizeFontEx)( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont );

/* <combine sack::image::PutCharacterFont@Image@S_32@S_32@CDATA@CDATA@_32@SFTFont>
   
   Internal
   Interface index 33                                                           */   IMAGE_PROC_PTR( void,PutCharacterFont)              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 34                                                                                        */   IMAGE_PROC_PTR( void,PutCharacterVerticalFont)      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
/* <combine sack::image::PutCharacterInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 35                                                                                      */   IMAGE_PROC_PTR( void,PutCharacterInvertFont)        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
/* <combine sack::image::PutCharacterVerticalInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Internal
   Interface index 36                                                                                              */   IMAGE_PROC_PTR( void,PutCharacterVerticalInvertFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );

/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 37                                                                                   */   IMAGE_PROC_PTR( void,PutStringFontEx)              ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 38                                                                                           */   IMAGE_PROC_PTR( void,PutStringVerticalFontEx)      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 39                                                                                         */   IMAGE_PROC_PTR( void,PutStringInvertFontEx)        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   Internal
   Interface index 40                                                                                                 */   IMAGE_PROC_PTR( void,PutStringInvertVerticalFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

/* <combine sack::image::GetMaxStringLengthFont@_32@SFTFont>
   
   Internal
   Interface index 41                                     */   IMAGE_PROC_PTR( _32, GetMaxStringLengthFont )( _32 width, SFTFont UseFont );

/* <combine sack::image::GetImageSize@Image@_32 *@_32 *>
   
   Internal
   Interface index 42                                                    */   IMAGE_PROC_PTR( void, GetImageSize)                ( Image pImage, _32 *width, _32 *height );
/* <combine sack::image::LoadFont@SFTFont>
   
   Internal
   Interface index 43                                   */   IMAGE_PROC_PTR( SFTFont, LoadFont )                   ( SFTFont font );
         /* <combine sack::image::UnloadFont@SFTFont>
            
            \ \                                    */
         IMAGE_PROC_PTR( void, UnloadFont )                 ( SFTFont font );

/* Internal
   Interface index 44
   
   This is used by internal methods to transfer image and font
   data to the render agent.                                   */   IMAGE_PROC_PTR( DataState, BeginTransferData )    ( _32 total_size, _32 segsize, CDATA data );
/* Internal
   Interface index 45
   
   Used internally to transfer data to render agent. */   IMAGE_PROC_PTR( void, ContinueTransferData )      ( DataState state, _32 segsize, CDATA data );
/* Internal
   Interface index 46
   
   Command issues at end of data transfer to decode the data
   into an image.                                            */   IMAGE_PROC_PTR( Image, DecodeTransferredImage )    ( DataState state );
/* After a data transfer decode the information as a font.
   Internal
   Interface index 47                                      */   IMAGE_PROC_PTR( SFTFont, AcceptTransferredFont )     ( DataState state );
/*Internal
   Interface index 48*/   IMAGE_PROC_PTR( CDATA, ColorAverage )( CDATA c1, CDATA c2
                                              , int d, int max );
/* <combine sack::image::SyncImage>
   
   Internal
   Interface index 49               */   IMAGE_PROC_PTR( void, SyncImage )                 ( void );
         /* <combine sack::image::GetImageSurface@Image>
            
            \ \                                          */
         IMAGE_PROC_PTR( PCDATA, GetImageSurface )       ( Image pImage );
         /* <combine sack::image::IntersectRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
            
            \ \                                                                                             */
         IMAGE_PROC_PTR( int, IntersectRectangle )      ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* <combine sack::image::MergeRectangle@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *@IMAGE_RECTANGLE *>
      
      \ \                                                                                         */
   IMAGE_PROC_PTR( int, MergeRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   /* <combine sack::image::GetImageAuxRect@Image@P_IMAGE_RECTANGLE>
      
      \ \                                                            */
   IMAGE_PROC_PTR( void, GetImageAuxRect )   ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* <combine sack::image::SetImageAuxRect@Image@P_IMAGE_RECTANGLE>
      
      \ \                                                            */
   IMAGE_PROC_PTR( void, SetImageAuxRect )   ( Image pImage, P_IMAGE_RECTANGLE pRect );
   /* <combine sack::image::OrphanSubImage@Image>
      
      \ \                                         */
   IMAGE_PROC_PTR( void, OrphanSubImage )  ( Image pImage );
   /* <combine sack::image::AdoptSubImage@Image@Image>
      
      \ \                                              */
   IMAGE_PROC_PTR( void, AdoptSubImage )   ( Image pFoster, Image pOrphan );
	/* <combine sack::image::MakeSpriteImageFileEx@CTEXTSTR fname>
	   
	   \ \                                                         */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS );
	/* <combine sack::image::MakeSpriteImageEx@Image image>
	   
	   \ \                                                  */
	IMAGE_PROC_PTR( PSPRITE, MakeSpriteImageEx )( Image image DBG_PASS );
	/* <combine sack::image::rotate_scaled_sprite@Image@PSPRITE@fixed@fixed@fixed>
	   
	   \ \                                                                         */
	IMAGE_PROC_PTR( void   , rotate_scaled_sprite )(Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height);
	/* <combine sack::image::rotate_sprite@Image@PSPRITE@fixed>
	   
	   \ \                                                      */
	IMAGE_PROC_PTR( void   , rotate_sprite )(Image bmp, PSPRITE sprite, fixed angle);
 /* <combine sack::image::BlotSprite@Image@PSPRITE>
	                                                  
	 Internal
   Interface index 61                                              */
		IMAGE_PROC_PTR( void   , BlotSprite )( Image pdest, PSPRITE ps );
    /* <combine sack::image::DecodeMemoryToImage@P_8@_32>
       
       \ \                                                */
    IMAGE_PROC_PTR( Image, DecodeMemoryToImage )( P_8 buf, _32 size );

   /* <combine sack::image::InternalRenderFontFile@CTEXTSTR@S_32@S_32@_32>
      
      \returns a SFTFont                                                      */
	IMAGE_PROC_PTR( SFTFont, InternalRenderFontFile )( CTEXTSTR file
																 , S_32 nWidth
																 , S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
																 , _32 flags 
																 );
   /* <combine sack::image::InternalRenderFont@_32@_32@_32@S_32@S_32@_32>
      
      requires knowing the font cache....                                 */
	IMAGE_PROC_PTR( SFTFont, InternalRenderFont )( _32 nFamily
															, _32 nStyle
															, _32 nFile
															, S_32 nWidth
															, S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
															, _32 flags
															);
/* <combine sack::image::RenderScaledFontData@PFONTDATA@PFRACTION@PFRACTION>
   
   \ \                                                                       */
IMAGE_PROC_PTR( SFTFont, RenderScaledFontData)( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale );
/* <combine sack::image::RenderFontFileEx@CTEXTSTR@_32@_32@_32@P_32@POINTER *>
   
   \ \                                                                         */
IMAGE_PROC_PTR( SFTFont, RenderFontFileScaledEx )( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *size, POINTER *pFontData );
/* <combine sack::image::DestroyFont@SFTFont *>
   
   \ \                                       */
IMAGE_PROC_PTR( void, DestroyFont)( SFTFont *font );
/* <combine sack::image::GetGlobalFonts>
   
   global_font_data in interface is really a global font data. Don't
   have to call GetGlobalFont to get this.                           */
struct font_global_tag *_global_font_data;
/* <combine sack::image::GetFontRenderData@SFTFont@POINTER *@_32 *>
   
   \ \                                                           */
IMAGE_PROC_PTR( int, GetFontRenderData )( SFTFont font, POINTER *fontdata, size_t *fontdatalen );
/* <combine sack::image::SetFontRendererData@SFTFont@POINTER@_32>
   
   \ \                                                         */
IMAGE_PROC_PTR( void, SetFontRendererData )( SFTFont font, POINTER pResult, size_t size );
/* <combine sack::image::SetSpriteHotspot@PSPRITE@S_32@S_32>
   
   \ \                                                       */
IMAGE_PROC_PTR( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, S_32 x, S_32 y );
/* <combine sack::image::SetSpritePosition@PSPRITE@S_32@S_32>
   
   \ \                                                        */
IMAGE_PROC_PTR( PSPRITE, SetSpritePosition )( PSPRITE sprite, S_32 x, S_32 y );
	/* <combine sack::image::UnmakeImageFileEx@Image pif>
	   
	   \ \                                                */
	IMAGE_PROC_PTR( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
/* <combine sack::image::GetGlobalFonts>
   
   \ \                                   */
IMAGE_PROC_PTR( struct font_global_tag *, GetGlobalFonts)( void );

/* <combinewith sack::image::GetStringRenderSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@_32 *@SFTFont, sack::image::GetStringRenderSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@_32 *@SFTFont>
   
   \ \                                                                                                                                                                     */
IMAGE_PROC_PTR( _32, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

IMAGE_PROC_PTR( Image, LoadImageFileFromGroupEx )( INDEX group, CTEXTSTR filename DBG_PASS );

IMAGE_PROC_PTR( SFTFont, RenderScaledFont )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
IMAGE_PROC_PTR( SFTFont, RenderScaledFontEx )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );

IMAGE_PROC_PTR( COLOR_CHANNEL, GetRedValue )( CDATA color ) ;
IMAGE_PROC_PTR( COLOR_CHANNEL, GetGreenValue )( CDATA color );
IMAGE_PROC_PTR( COLOR_CHANNEL, GetBlueValue )( CDATA color );
IMAGE_PROC_PTR( COLOR_CHANNEL, GetAlphaValue )( CDATA color );
IMAGE_PROC_PTR( CDATA, SetRedValue )( CDATA color, COLOR_CHANNEL r ) ;
IMAGE_PROC_PTR( CDATA, SetGreenValue )( CDATA color, COLOR_CHANNEL green );
IMAGE_PROC_PTR( CDATA, SetBlueValue )( CDATA color, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, SetAlphaValue )( CDATA color, COLOR_CHANNEL a );
IMAGE_PROC_PTR( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
IMAGE_PROC_PTR( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


IMAGE_PROC_PTR( PTRANSFORM, GetImageTransformation )( Image pImage );
IMAGE_PROC_PTR( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
IMAGE_PROC_PTR( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );
IMAGE_PROC_PTR( void, MarkImageDirty )( Image pImage );

IMAGE_PROC_PTR( void, DumpFontCache )( void );
IMAGE_PROC_PTR( void, RerenderFont )( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_PTR( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

IMAGE_PROC_PTR( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
IMAGE_PROC_PTR( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
IMAGE_PROC_PTR( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, PCVECTOR o, LOGICAL render_pixel_scaled );

// transfer all sub images to new image using appropriate methods
// extension for internal fonts to be utilized by external plugins...
IMAGE_PROC_PTR( void, TransferSubImages )( Image pImageTo, Image pImageFrom );
// when using reverse interfaces, need a way to get the real image
// from the fake image (proxy image) 
IMAGE_PROC_PTR( Image, GetNativeImage )( Image pImageTo );

// low level support for proxy; this exposes some image_common.c routines
IMAGE_PROC_PTR( Image, GetTintedImage )( Image child_image, CDATA color );
IMAGE_PROC_PTR( Image, GetShadedImage )( Image child_image, CDATA red, CDATA green, CDATA blue );
// test for IF_FLAG_FINAL_RENDER (non physical surface/prevent local copy-restore)
IMAGE_PROC_PTR( LOGICAL, IsImageTargetFinal )( Image image );

// use image data to create a clone of the image for the new application instance...
// this is used when a common image resource is used for all application instances
// it should be triggered during onconnect.
// it is a new image instance that should be used for future app references...
IMAGE_PROC_PTR( Image, ReuseImage )( Image image );
IMAGE_PROC_PTR( void, PutStringFontExx )( Image pImage
											 , S_32 x, S_32 y
											 , CDATA color, CDATA background
											 , CTEXTSTR pc, size_t nLen, SFTFont font, int justification, _32 _width );
// sometimes it's not possible to use blatcolor to clear an imate...
// sometimes its parent is not redrawn?
IMAGE_PROC_PTR( void, ResetImageBuffers )( Image image, LOGICAL image_only );
	IMAGE_PROC_PTR(  LOGICAL, PngImageFile )( Image image, P_8 *buf, size_t *size );
	IMAGE_PROC_PTR(  LOGICAL, JpgImageFile )( Image image, P_8 *buf, size_t *size, int Q );
	IMAGE_PROC_PTR(  void, SetFontBias )( SFTFont font, S_32 x, S_32 y );
	IMAGE_PROC_PTR( SlicedImage, MakeSlicedImage )( Image source, _32 left, _32 right, _32 top, _32 bottom, LOGICAL output_center );
	IMAGE_PROC_PTR( SlicedImage, MakeSlicedImageComplex )( Image source
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
	IMAGE_PROC_PTR( void, UnmakeSlicedImage )( SlicedImage image );
	IMAGE_PROC_PTR( void, BlotSlicedImageEx )( Image dest, SlicedImage source, S_32 x, S_32 y, _32 width, _32 height, int alpha, enum BlotOperation op, ... );

} IMAGE_INTERFACE, *PIMAGE_INTERFACE;



/* Method to define automatic name translation from standard
   names Like BlatColorAlphaEx to the interface the user has
   specified to be using.                                    */
#define PROC_ALIAS(name) ((USE_IMAGE_INTERFACE)->_##name)
/* Method to define automatic name translation from standard
   names Like BlatColorAlphaEx to the interface the user has
   specified to be using. For function pointers.             */
#define PPROC_ALIAS(name) (*(USE_IMAGE_INTERFACE)->_##name)

#ifdef DEFINE_DEFAULT_IMAGE_INTERFACE
//static PIMAGE_INTERFACE always_defined_interface_that_makes_this_efficient;
#  define USE_IMAGE_INTERFACE GetImageInterface()
#endif

#if defined( FORCE_NO_INTERFACE ) && !defined( ALLOW_IMAGE_INTERFACES )
#  undef USE_IMAGE_INTERFACE
#else
#  define GetImageInterface() (PIMAGE_INTERFACE)GetInterface( WIDE("image") )
/* <combine sack::image::DropImageInterface@PIMAGE_INTERFACE>
   
   \ \                                                        */
#  define DropImageInterface(x) DropInterface( WIDE("image"), NULL )

#endif

#ifdef USE_IMAGE_INTERFACE

#define GetRedValue                          PROC_ALIAS(GetRedValue )
#define GetBlueValue                          PROC_ALIAS(GetBlueValue )
#define GetGreenValue                          PROC_ALIAS(GetGreenValue )
#define GetAlphaValue                          PROC_ALIAS(GetAlphaValue )
#define SetRedValue                          PROC_ALIAS(SetRedValue )
#define SetBlueValue                          PROC_ALIAS(SetBlueValue )
#define SetGreenValue                          PROC_ALIAS(SetGreenValue )
#define SetAlphaValue                          PROC_ALIAS(SetAlphaValue )
#define MakeColor                          PROC_ALIAS(MakeColor )
#define MakeAlphaColor                          PROC_ALIAS(MakeAlphaColor )
#define MarkImageDirty                    PROC_ALIAS(MarkImageDirty)

#define GetStringRenderSizeFontEx          PROC_ALIAS(GetStringRenderSizeFontEx )
#define LoadImageFileFromGroupEx          PROC_ALIAS(LoadImageFileFromGroupEx )
#define SetStringBehavior                  PROC_ALIAS(SetStringBehavior )
#define SetBlotMethod                      //PROC_ALIAS(SetBlotMethod )
#define BuildImageFileEx                   PROC_ALIAS(BuildImageFileEx )
#define MakeImageFileEx                    PROC_ALIAS(MakeImageFileEx )
#define MakeSubImageEx                     PROC_ALIAS(MakeSubImageEx )
#define RemakeImageEx                      PROC_ALIAS(RemakeImageEx )
#define ResizeImageEx                      PROC_ALIAS(ResizeImageEx )
#define MoveImage                          PROC_ALIAS(MoveImage )
#define LoadImageFileEx                    PROC_ALIAS(LoadImageFileEx )
#define DecodeMemoryToImage                PROC_ALIAS(DecodeMemoryToImage )
#define UnmakeImageFileEx                  PROC_ALIAS(UnmakeImageFileEx )
#define BlatColor                          PROC_ALIAS(BlatColor )
#define BlatColorAlpha                     PROC_ALIAS(BlatColorAlpha )
#define BlotImageSizedEx                   PROC_ALIAS(BlotImageSizedEx )
#define BlotImageEx                        PROC_ALIAS(BlotImageEx )
#define BlotScaledImageSizedEx             PROC_ALIAS(BlotScaledImageSizedEx )
#define plot                               PPROC_ALIAS(plot )
#define plotalpha                          PPROC_ALIAS(plotalpha )
#define getpixel                           PPROC_ALIAS(getpixel )
#define do_line                            PPROC_ALIAS(do_line )
#define do_lineAlpha                       PPROC_ALIAS(do_lineAlpha )
#define do_hline                           PPROC_ALIAS(do_hline )
#define do_vline                           PPROC_ALIAS(do_vline )
#define do_hlineAlpha                      PPROC_ALIAS(do_hlineAlpha )
#define do_vlineAlpha                      PPROC_ALIAS(do_vlineAlpha )
#define GetDefaultFont                     PROC_ALIAS(GetDefaultFont )
#define GetFontHeight                      PROC_ALIAS(GetFontHeight )
#define GetStringSizeFontEx                PROC_ALIAS(GetStringSizeFontEx )
#define PutCharacterFont                   PROC_ALIAS(PutCharacterFont )
#define PutCharacterVerticalFont           PROC_ALIAS(PutCharacterVerticalFont )
#define PutCharacterInvertFont             PROC_ALIAS(PutCharacterInvertFont )
#define PutCharacterVerticalInvertFont     PROC_ALIAS(PutCharacterVerticalInvertFont )
#define PutStringFontExx                   PROC_ALIAS(PutStringFontExx)
#define PutStringFontEx                    PROC_ALIAS(PutStringFontEx )
#define PutStringVerticalFontEx            PROC_ALIAS(PutStringVerticalFontEx )
#define PutStringInvertFontEx              PROC_ALIAS(PutStringInvertFontEx )
#define PutStringInvertVerticalFontEx      PROC_ALIAS(PutStringInvertVerticalFontEx )
#define GetMaxStringLengthFont             PROC_ALIAS(GetMaxStringLengthFont )
#define GetImageSize                       PROC_ALIAS(GetImageSize )
#define LoadFont                           PROC_ALIAS(LoadFont )
#define UnloadFont                         PROC_ALIAS(UnloadFont )
#define ColorAverage                       PPROC_ALIAS(ColorAverage)
#define TransferSubImages                  PROC_ALIAS(TransferSubImages)
#define SyncImage                          PROC_ALIAS(SyncImage )
#define IntersectRectangle                 PROC_ALIAS(IntersectRectangle)
#define MergeRectangle                     PROC_ALIAS(MergeRectangle)
#define GetImageSurface                    PROC_ALIAS(GetImageSurface)
#define SetImageAuxRect                    PROC_ALIAS(SetImageAuxRect)
#define GetImageAuxRect                    PROC_ALIAS(GetImageAuxRect)
#define OrphanSubImage                     PROC_ALIAS(OrphanSubImage)
#define GetGlobalFonts                     PROC_ALIAS(GetGlobalFonts)
#define GetTintedImage                     PROC_ALIAS(GetTintedImage)
#define GetShadedImage                     PROC_ALIAS(GetShadedImage)
#define AdoptSubImage                      PROC_ALIAS(AdoptSubImage)

#define MakeSpriteImageFileEx   PROC_ALIAS(MakeSpriteImageFileEx)
#define MakeSpriteImageEx       PROC_ALIAS(MakeSpriteImageEx)
#define UnmakeSprite            PROC_ALIAS(UnmakeSprite )
#define rotate_scaled_sprite    PROC_ALIAS(rotate_scaled_sprite)
#define rotate_sprite           PROC_ALIAS(rotate_sprite)
#define BlotSprite              PROC_ALIAS(BlotSprite)
#define SetSpritePosition  PROC_ALIAS(  SetSpritePosition )
#define SetSpriteHotspot  PROC_ALIAS(  SetSpriteHotspot )

#define InternalRenderFont          PROC_ALIAS(InternalRenderFont)
#define InternalRenderFontFile      PROC_ALIAS(InternalRenderFontFile)
#define RenderScaledFontData              PROC_ALIAS(RenderScaledFontData)
//#define RenderScaledFont              PROC_ALIAS(RenderScaledFont)
#define RenderScaledFontEx              PROC_ALIAS(RenderScaledFontEx)
#define DumpFontCache              PROC_ALIAS(DumpFontCache)
#define RerenderFont              PROC_ALIAS(RerenderFont)
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
#define ReloadTexture              PROC_ALIAS(ReloadTexture)
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
#define ReloadShadedTexture              PROC_ALIAS(ReloadShadedTexture)
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
#define ReloadMultiShadedTexture              PROC_ALIAS(ReloadMultiShadedTexture)
#define DestroyFont              PROC_ALIAS(DestroyFont)
#define GetFontRenderData              PROC_ALIAS(GetFontRenderData)
#define SetFontRendererData              PROC_ALIAS(SetFontRendererData)
#define RenderFontFileScaledEx              PROC_ALIAS(RenderFontFileScaledEx)
#define GetImageTransformation              PROC_ALIAS(GetImageTransformation)
#define SetImageTransformRelation      PROC_ALIAS( SetImageTransformRelation )
#define Render3dImage                  PROC_ALIAS( Render3dImage )
#define Render3dText                   PROC_ALIAS( Render3dText )
#define DumpFontFile                   PROC_ALIAS( DumpFontFile )
#define IsImageTargetFinal                   PROC_ALIAS( IsImageTargetFinal )
#define ReuseImage                      if((USE_IMAGE_INTERFACE)->_ReuseImage) PROC_ALIAS( ReuseImage )
#define ResetImageBuffers                      if((USE_IMAGE_INTERFACE)->_ResetImageBuffers) PROC_ALIAS( ResetImageBuffers )
#define PngImageFile                    PROC_ALIAS( PngImageFile )
#define JpgImageFile                    PROC_ALIAS( JpgImageFile )
#define SetFontBias                     PROC_ALIAS( SetFontBias )
#define MakeSlicedImage                 PROC_ALIAS( MakeSlicedImage )
#define MakeSlicedImageComplex          PROC_ALIAS( MakeSlicedImageComplex )
#define UnmakeSlicedImage                 PROC_ALIAS( UnmakeSlicedImage )
#define BlotSlicedImageEx               PROC_ALIAS( BlotSlicedImageEx )
//#define global_font_data         (*PROC_ALIAS(global_font_data))
#endif

/* <combine sack::image::GetMaxStringLengthFont@_32@SFTFont>
   
   \ \                                                    */
#define GetMaxStringLength(w) GetMaxStringLengthFont(w, NULL )

#ifdef DEFINE_IMAGE_PROTOCOL
//#include <msgprotocol.h>
#include <stddef.h>
// need to define BASE_IMAGE_MESSAGE_ID before hand to determine what the base message is.
//#define MSG_ID(method)  ( ( offsetof( struct image_interface_tag, _##method ) / sizeof( void(*)(void) ) ) + BASE_IMAGE_MESSAGE_ID + MSG_EventUser )
#define MSG_SetStringBehavior                  MSG_ID( SetStringBehavior )
#define MSG_SetBlotMethod                      MSG_ID( SetBlotMethod )
#define MSG_BuildImageFileEx                   MSG_ID( BuildImageFileEx )
#define MSG_MakeImageFileEx                    MSG_ID( MakeImageFileEx )
#define MSG_MakeSubImageEx                     MSG_ID( MakeSubImageEx )
#define MSG_RemakeImageEx                      MSG_ID( RemakeImageEx )
#define MSG_UnmakeImageFileEx                  MSG_ID( UnmakeImageFileEx )
#define MSG_ResizeImageEx                      MSG_ID( ResizeImageEx )
#define DecodeMemoryToImage                    MSG_ID( DecodeMemoryToImage )
#define MSG_MoveImage                          MSG_ID( MoveImage )
#define MSG_BlatColor                          MSG_ID( BlatColor )
#define MSG_BlatColorAlpha                     MSG_ID( BlatColorAlpha )
#define MSG_BlotImageSizedEx                   MSG_ID( BlotImageSizedEx )
#define MSG_BlotImageEx                        MSG_ID( BlotImageEx )
#define MSG_BlotScaledImageSizedEx             MSG_ID( BlotScaledImageSizedEx )
#define MSG_plot                               MSG_ID( plot )
#define MSG_plotalpha                          MSG_ID( plotalpha )
#define MSG_getpixel                           MSG_ID( getpixel )
#define MSG_do_line                            MSG_ID( do_line )
#define MSG_do_lineAlpha                       MSG_ID( do_lineAlpha )
#define MSG_do_hline                           MSG_ID( do_hline )
#define MSG_do_vline                           MSG_ID( do_vline )
#define MSG_do_hlineAlpha                      MSG_ID( do_hlineAlpha )
#define MSG_do_vlineAlpha                      MSG_ID( do_vlineAlpha )
#define MSG_GetDefaultFont                     MSG_ID( GetDefaultFont )
#define MSG_GetFontHeight                      MSG_ID( GetFontHeight )
#define MSG_GetStringSizeFontEx                MSG_ID( GetStringSizeFontEx )
#define MSG_PutCharacterFont                   MSG_ID( PutCharacterFont )
#define MSG_PutCharacterVerticalFont           MSG_ID( PutCharacterVerticalFont )
#define MSG_PutCharacterInvertFont             MSG_ID( PutCharacterInvertFont )
#define MSG_PutCharacterVerticalInvertFont     MSG_ID( PutCharacterVerticalInvertFont )
#define MSG_PutStringFontEx                    MSG_ID( PutStringFontEx )
#define MSG_PutStringVerticalFontEx            MSG_ID( PutStringVerticalFontEx )
#define MSG_PutStringInvertFontEx              MSG_ID( PutStringInvertFontEx )
#define MSG_PutStringInvertVerticalFontEx      MSG_ID( PutStringInvertVerticalFontEx )
#define MSG_GetMaxStringLengthFont             MSG_ID( GetMaxStringLengthFont )
#define MSG_GetImageSize                       MSG_ID( GetImageSize )
#define MSG_ColorAverage                       MSG_IC( ColorAverage )
// these messages follow all others... and are present to handle
// LoadImageFile
// #define MSG_LoadImageFile (no message)
// #define MSG_LoadFont      (no message)
#define MSG_UnloadFont                         MSG_ID( UnloadFont )
#define MSG_BeginTransferData                  MSG_ID( BeginTransferData )
#define MSG_ContinueTransferData               MSG_ID( ContinueTransferData )
#define MSG_DecodeTransferredImage             MSG_ID( DecodeTransferredImage )
#define MSG_AcceptTransferredFont              MSG_ID( AcceptTransferredFont )
#define MSG_SyncImage                          MSG_ID( SyncImage )
#define MSG_IntersectRectangle                 MSG_ID( IntersectRectangle )
#define MSG_MergeRectangle                     MSG_ID( MergeRectangle)
#define MSG_GetImageSurface                    MSG_ID( GetImageSurface )
#define MSG_SetImageAuxRect                    MSG_ID(SetImageAuxRect)
#define MSG_GetImageAuxRect                    MSG_ID(GetImageAuxRect)
#define MSG_OrphanSubImage                     MSG_ID(OrphanSubImage)
#define MSG_GetGlobalFonts                     MSG_ID(GetGlobalFonts)
#define MSG_AdoptSubImage                      MSG_ID(AdoptSubImage)


#define MSG_MakeSpriteImageFileEx   MSG_ID(MakeSpriteImageFileEx)
#define MSG_MakeSpriteImageEx       MSG_ID(MakeSpriteImageEx)
#define MSG_UnmakeSprite            MSG_ID(UnmakeSprite )
#define MSG_rotate_scaled_sprite    MSG_ID(rotate_scaled_sprite)
#define MSG_rotate_sprite           MSG_ID(rotate_sprite)
#define MSG_BlotSprite              MSG_ID(BlotSprite)
#define MSG_SetSpritePosition  MSG_ID(  SetSpritePosition )
#define MSG_SetSpriteHotspot  MSG_ID(  SetSpriteHotspot )
#define MSG_InternalRenderFont          MSG_ID(InternalRenderFont)
#define MSG_InternalRenderFontFile      MSG_ID(InternalRenderFontFile)
#define MSG_RenderScaledFontData              MSG_ID(RenderScaledFontData)
#define MSG_RenderScaledFont              MSG_ID(RenderScaledFont)
#define MSG_RenderFontData              MSG_ID(RenderFontData)
#define MSG_DestroyFont              MSG_ID(DestroyFont)
#define MSG_GetFontRenderData              MSG_ID(GetFontRenderData)
#define MSG_SetFontRendererData              MSG_ID(SetFontRendererData)
#endif

#ifdef USE_IMAGE_LEVEL
#warning ...
#define PASTELEVEL(level,name) level##name
#define LEVEL_ALIAS(name)      PASTELEVEL(USE_IMAGE_LEVEL,name)
#  ifdef STUPID_NO_DATA_EXPORTS
#define PLEVEL_ALIAS(name)      (*PASTELEVEL(USE_IMAGE_LEVEL,_PASTE(_,name)))
#  else
#define PLEVEL_ALIAS(name)      (*PASTELEVEL(USE_IMAGE_LEVEL,name))
#  endif
#define SetStringBehavior                  LEVEL_ALIAS(SetStringBehavior )
#define SetBlotMethod                      //LEVEL_ALIAS(SetBlotMethod )
#define BuildImageFileEx                   LEVEL_ALIAS(BuildImageFileEx )
#define MakeImageFileEx                    LEVEL_ALIAS(MakeImageFileEx )
#define MakeSubImageEx                     LEVEL_ALIAS(MakeSubImageEx )
#define RemakeImageEx                      LEVEL_ALIAS(RemakeImageEx )
#define ResizeImageEx                      LEVEL_ALIAS(ResizeImageEx )
#define MoveImage                          LEVEL_ALIAS(MoveImage )
#define LoadImageFileEx                    LEVEL_ALIAS(LoadImageFileEx )
#define DecodeMemoryToImage                LEVEL_ALIAS(DecodeMemoryToImage )
#define UnmakeImageFileEx                  LEVEL_ALIAS(UnmakeImageFileEx )
#define BlatColor                          LEVEL_ALIAS(BlatColor )
#define BlatColorAlpha                     LEVEL_ALIAS(BlatColorAlpha )
#define BlotImageSizedEx                   LEVEL_ALIAS(BlotImageSizedEx )
#define BlotImageEx                        LEVEL_ALIAS(BlotImageEx )
#define BlotScaledImageSizedEx             LEVEL_ALIAS(BlotScaledImageSizedEx )
#define plot                               LEVEL_ALIAS(plot )
#define plotalpha                          LEVEL_ALIAS(plotalpha )
#error 566
#define getpixel                           LEVEL_ALIAS(getpixel )
#define do_line                            LEVEL_ALIAS(do_line )
#define do_lineAlpha                       LEVEL_ALIAS(do_lineAlpha )
#define do_hline                           LEVEL_ALIAS(do_hline )
#define do_vline                           LEVEL_ALIAS(do_vline )
#define do_hlineAlpha                      LEVEL_ALIAS(do_hlineAlpha )
#define do_vlineAlpha                      LEVEL_ALIAS(do_vlineAlpha )
#define GetDefaultFont                     LEVEL_ALIAS(GetDefaultFont )
#define GetFontHeight                      LEVEL_ALIAS(GetFontHeight )
#define GetStringSizeFontEx                LEVEL_ALIAS(GetStringSizeFontEx )
#define PutCharacterFont                   LEVEL_ALIAS(PutCharacterFont )
#define PutCharacterVerticalFont           LEVEL_ALIAS(PutCharacterVerticalFont )
#define PutCharacterInvertFont             LEVEL_ALIAS(PutCharacterInvertFont )
#define PutCharacterVerticalInvertFont     LEVEL_ALIAS(PutCharacterVerticalInvertFont )
#define PutStringFontEx                    LEVEL_ALIAS(PutStringFontEx )
#define PutStringVerticalFontEx            LEVEL_ALIAS(PutStringVerticalFontEx )
#define PutStringInvertFontEx              LEVEL_ALIAS(PutStringInvertFontEx )
#define PutStringInvertVerticalFontEx      LEVEL_ALIAS(PutStringInvertVerticalFontEx )
#define GetMaxStringLengthFont             LEVEL_ALIAS(GetMaxStringLengthFont )
#define GetImageSize                       LEVEL_ALIAS(GetImageSize )
#define LoadFont                           LEVEL_ALIAS(LoadFont )
#define UnloadFont                         LEVEL_ALIAS(UnloadFont )
#define ColorAverage                       LEVEL_ALIAS(ColorAverage)
#define SyncImage                          LEVEL_ALIAS(SyncImage )
#define IntersectRectangle                 LEVEL_ALIAS( IntersectRectangle )
#define MergeRectangle                     LEVEL_ALIAS(MergeRectangle)
#define GetImageSurface                    LEVEL_ALIAS(GetImageSurface)
#define SetImageAuxRect                    LEVEL_ALIAS(SetImageAuxRect)
#define GetImageAuxRect                    LEVEL_ALIAS(GetImageAuxRect)
#define OrphanSubImage                     LEVEL_ALIAS(OrphanSubImage)
#define GetGlobalFonts                     LEVEL_ALIAS(GetGlobalFonts)
#define AdoptSubImage                      LEVEL_ALIAS(AdoptSubImage)
#define InternalRenderFont          LEVEL_ALIAS(InternalRenderFont)
#define InternalRenderFontFile      LEVEL_ALIAS(InternalRenderFontFile)
#define RenderScaledFontData              LEVEL_ALIAS(RenderScaledFontData)
#define RenderFontData              LEVEL_ALIAS(RenderFontData)
#define RenderFontFileScaledEx              LEVEL_ALIAS(RenderFontFileScaledEx)
#endif

_INTERFACE_NAMESPACE_END
#ifdef __cplusplus
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

// these macros provide common extensions for 
// commonly used shorthands of the above routines.
// no worry - one way or another, the extra data is 
// created, and the base function called, it's a sad 
// truth of life, that one codebase is easier to maintain
// than a duplicate copy for each minor case.
// although - special forwards - such as DBG_SRC will just dissappear
// in certain compilation modes (NON_DEBUG)


/* <combine sack::image::BuildImageFileEx@PCOLOR@_32@_32>
   
   \ \                                                           */
#define BuildImageFile(p,w,h) BuildImageFileEx( p,w,h DBG_SRC )
/* <combine sack::image::MakeImageFileEx@_32@_32>
   
   \ \                                                   */
#define MakeImageFile(w,h) MakeImageFileEx( w,h DBG_SRC )
/* <combine sack::image::MakeSubImageEx@Image@S_32@S_32@_32@_32>
   
   \ \                                                                  */
#define MakeSubImage( image, x, y, w, h ) MakeSubImageEx( image, x, y, w, h DBG_SRC )
/* <combine sack::image::RemakeImageEx@Image@PCOLOR@_32@_32>
   
   \ \                                                              */
#define RemakeImage(p,pc,w,h) RemakeImageEx(p,pc,w,h DBG_SRC)
/* <combine sack::image::ResizeImageEx@Image@_32@_32>
   
   \ \                                                              */
#define ResizeImage( p,w,h) ResizeImageEx( p,w,h DBG_SRC )
/* <combine sack::image::UnmakeImageFileEx@Image pif>
   
   Destroys an image. Does not automatically destroy child
   images created on the image.
   Parameters
   Image :  an image to destroy
   Example
   <code lang="c++">
   Image image = MakeImageFile( 100, 100 );
   UnmakeImageFile( image );
   </code>                                                 */
#define UnmakeImageFile(pif) UnmakeImageFileEx( pif DBG_SRC )
/* <combine sack::image::MakeSpriteImageEx@Image image>
   
   \ \                                                  */
#define MakeSpriteImage(image) MakeSpriteImageEx(image DBG_SRC)
/* <combine sack::image::MakeSpriteImageFileEx@CTEXTSTR fname>
   
   \ \                                                         */
#define MakeSpriteImageFile(file) MakeSpriteImageFileEx( image DBG_SRC )

/* This function flips an image top to bottom. This if for
   building windows compatible images. Internally images are
   kept in platform-native direction. If an image is created
   from another source, this might be a method to flip the image
   top-to-bottom if required.
   
   
   Parameters
   pImage :                           Image to flip.
   <link sack::DBG_PASS, DBG_PASS> :  _nt_
   
   Note
   There has been a warning around flip image for a while, it
   does its job right now (reversing jpeg images on windows),
   but not necessarily suited for the masses.                    */
IMAGE_PROC  void IMAGE_API  FlipImageEx ( Image pif DBG_PASS );
/* <combine sack::image::FlipImageEx@Image pif>
   
   \ \                                          */
#define FlipImage(pif) FlipImageEx( pif DBG_SRC )

/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
   
   \ \                                                  */
#define LoadImageFile(file) LoadImageFileEx( file DBG_SRC )
/* <combine sack::image::LoadImageFileEx@CTEXTSTR name>
   
   \ \                                                  */
#define LoadImageFileFromGroup(group,file) LoadImageFileFromGroupEx( group, file DBG_SRC )
/* <combine sack::image::BlatColor@Image@S_32@S_32@_32@_32@CDATA>
   
   \ \                                                            */
#define ClearImageTo(img,color) BlatColor(img,0,0,(img)->width,(img)->height, color )
/* <combine sack::image::BlatColor@Image@S_32@S_32@_32@_32@CDATA>
   
   \ \                                                            */
#define ClearImage(img) BlatColor(img,0,0,(img)->width,(img)->height, 0 )

/* Copy one image to another. Copies the source from 0,0 to the
   destination 0,0 of the minimum width and height of the
   smaller of the source or destination.
   Parameters
   pifDest :  Image to copy to
   pifSrc :   Image to copy from
   X :        left coordinate to copy image to
   Y :        upper coordinate to copy image to
   Example
   This creates an image to write to, creates an image to copy
   (a 64 by 64 square that is filled with 50% green color). And
   copies the image to the output buffer.
   <code>
   Image output = MakeImageFile( 1024, 768 );
   Image source = MakeImageFile( 64, 64 );
   
   // 50% transparent
   ClearImageTo( source, SetAlpha( BASE_COLOR_GREEN, 128 ) );
   ClearImage( output );
   
   BlotImage( output, source, 100, 100 );
   BlotImageAlpha( output, source, 200, 200 );
   </code>                                                      */
#define BlotImage( pd, ps, x, y ) BlotImageEx( pd, ps, x, y, 0, BLOT_COPY )

/* Output a sliced image to an image surface
  sliced images scale center portions, but copy output corner images 
  */
#define BlotSlicedImage( pd, ps, x, y, w, h ) BlotSlicedImageEx( pd, ps, x, y, w, h, ALPHA_TRANSPARENT, BLOT_COPY )

/* Copy one image to another at the specified coordinate in the
   destination.
   Parameters
   Destination :  Image to output to
   Source :       Image to copy
   X :            Location to copy to
   Y :            Location to copy to <link sack::image::AlphaModifier, Alpha>
                  \: Specify how to write the alpha                            */
#define BlotImageAlpha( pd, ps, x, y, a ) BlotImageEx( pd, ps, x, y, a, BLOT_COPY )
/* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                         */
#define BlotImageSized( pd, ps, x, y, w, h ) BlotImageSizedEx( pd, ps, x, y, 0, 0, w, h, TRUE, BLOT_COPY )
/* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                         */
#define BlotImageSizedAlpha( pd, ps, x, y, w, h, a ) BlotImageSizedEx( pd, ps, x, y, 0, 0, w, h, a, BLOT_COPY )
/* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                         */
#define BlotImageSizedTo( pd, ps, xd, yd, xs, ys, w, h )  BlotImageSizedEx( pd, ps, xd, yd, xs, ys, w, h, TRUE, BLOT_COPY )

/* Copy one image to another at the specified coordinate in the
   destination. Shade the image on copy with a color.
   Parameters
   Destination :  Image to output to
   Source :       Image to copy
   X :            Location to copy to
   Y :            Location to copy to
   Color :        color to multiply the source color by to shade
                  on copy.                                       */
#define BlotImageShaded( pd, ps, xd, yd, c ) BlotImageEx( pd, ps, xd, yd, TRUE, BLOT_SHADED, c )
/* <combine sack::image::BlotImageSizedEx@Image@Image@S_32@S_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                         */
#define BlotImageShadedSized( pd, ps, xd, yd, xs, ys, ws, hs, c ) BlotImageSizedEx( pd, ps, xd, yd, xs, ys, ws, hs, TRUE, BLOT_SHADED, c )

/* Copy one image to another at the specified coordinate in the
   destination. Scale RGB channels to specified colors.
   Parameters
   Destination :  Image to output to
   Source :       Image to copy
   X :            Location to copy to
   Y :            Location to copy to
   X_source :     the left coordinate of the image source
   Y_source :     the top coordinate of the image source
   Width :        How wide to copy the image
   Height :       How wide to copy the image
   color :        color mutiplier to shade the image.           */
#define BlotImageMultiShaded( pd, ps, xd, yd, r, g, b ) BlotImageEx( pd, ps, xd, yd, ALPHA_TRANSPARENT, BLOT_MULTISHADE, r, g, b )
/* Copy one image to another at the specified coordinate in the
   destination. Scale RGB channels to specified colors.
   Parameters
   Destination :  Image to output to
   Source :       Image to copy
   X :            Location to copy to
   Y :            Location to copy to
   X_source :     the left coordinate of the image source
   Y_source :     the top coordinate of the image source
   Width :        How wide to copy the image
   Height :       How wide to copy the image
   color :        color mutiplier to shade the image.           */
#define BlotImageMultiShadedSized( pd, ps, xd, yd, xs, ys, ws, hs, r, g, b ) BlotImageSizedEx( pd, ps, xd, yd, xs, ys, ws, hs, TRUE, BLOT_MULTISHADE, r, g, b )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSized( pd, ps, xd, yd, wd, hd, xs, ys, ws, hs ) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, xs, ys, ws, hs, 0, BLOT_COPY )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedMultiShaded( pd, ps, xd, yd, wd, hd, xs, ys, ws, hs,r,g,b ) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, xs, ys, ws, hs, 0, BLOT_MULTISHADE,r,g,b )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedTo( pd, ps, xd, yd, wd, hd) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, 0, 0, (ps)->width, (ps)->height, 0, BLOT_COPY )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedToAlpha( pd, ps, xd, yd, wd, hd, a) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, 0, 0, (ps)->width, (ps)->height, a, BLOT_COPY )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedToShaded( pd, ps, xd, yd, wd, hd,shade) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, 0, 0, (ps)->width, (ps)->height, 0,BLOT_SHADED, shade )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedToShadedAlpha( pd, ps, xd, yd, wd, hd,a,shade) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, 0, 0, (ps)->width, (ps)->height, a, BLOT_SHADED, shade )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedToMultiShaded( pd, ps, xd, yd, wd, hd,r,g,b) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, 0, 0, (ps)->width, (ps)->height, 0,BLOT_MULTISHADE, r,g,b )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageSizedToMultiShadedAlpha( pd, ps, xd, yd, wd, hd,a,r,g,b) BlotScaledImageSizedEx( pd, ps, xd, yd, wd, hd, 0, 0, (ps)->width, (ps)->height, a,BLOT_MULTISHADE, r,g,b )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageAlpha( pd, ps, t ) BlotScaledImageSizedEx( pd, ps, 0, 0, (pd)->width, (pd)->height, 0, 0, (ps)->width, (ps)->height, t, BLOT_COPY )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageShadedAlpha( pd, ps, t, shade ) BlotScaledImageSizedEx( pd, ps, 0, 0, (pd)->width, (pd)->height, 0, 0, (ps)->width, (ps)->height, t, BLOT_SHADED, shade )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageMultiShadedAlpha( pd, ps, t, r, g, b ) BlotScaledImageSizedEx( pd, ps, 0, 0, (pd)->width, (pd)->height, 0, 0, (ps)->width, (ps)->height, t, BLOT_MULTISHADE, r, g, b )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImage( pd, ps ) BlotScaledImageSizedEx( pd, ps, 0, 0, (pd)->width, (pd)->height, 0, 0, (ps)->width, (ps)->height, 0, BLOT_COPY )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageShaded( pd, ps, shade ) BlotScaledImageSizedEx( pd, ps, 0, 0, (pd)->width, (pd)->height, 0, 0, (ps)->width, (ps)->height, 0, BLOT_SHADED, shade )
/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageMultiShaded( pd, ps, r, g, b ) BlotScaledImageSizedEx( pd, ps, 0, 0, (pd)->width, (pd)->height, 0, 0, (ps)->width, (ps)->height, 0, BLOT_MULTISHADE, r, g, b )

/* <combine sack::image::BlotScaledImageSizedEx@Image@Image@S_32@S_32@_32@_32@S_32@S_32@_32@_32@_32@_32@...>
   
   \ \                                                                                                       */
#define BlotScaledImageTo( pd, ps )  BlotScaledImageToEx( pd, ps, FALSE, BLOT_COPY )

/* now why would we need an inverse line? I don't get it....
   anyhow this would draw from the end to the start... basically
   this accounts for rounding errors on the orward way.          */
#define do_inv_line(pb,x,y,xto,yto,d) do_line( pb,y,x,yto,xto,d)

/* <combine sack::image::PutCharacterFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   \ \                                                                               */
#define PutCharacter(i,x,y,fore,back,c)               PutCharacterFont(i,x,y,fore,back,c,NULL )
/* <combine sack::image::PutCharacterVerticalFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   Passes default font if not specified.                                                     */
#define PutCharacterVertical(i,x,y,fore,back,c)       PutCharacterVerticalFont(i,x,y,fore,back,c,NULL )
/* <combine sack::image::PutCharacterInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   \ \                                                                                     */
#define PutCharacterInvert(i,x,y,fore,back,c)         PutCharacterInvertFont(i,x,y,fore,back,c,NULL )
/* <combine sack::image::PutCharacterVerticalInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   \ \                                                                                             */
#define PutCharacterInvertVertical(i,x,y,fore,back,c) PutCharacterInvertVerticalFont(i,x,y,fore,back,c,NULL )
/* <combine sack::image::PutCharacterVerticalInvertFont@Image@S_32@S_32@CDATA@CDATA@TEXTCHAR@SFTFont>
   
   \ \                                                                                             */
#define PutCharacterInvertVerticalFont(i,x,y,fore,back,c,f) PutCharacterVerticalInvertFont(i,x,y,fore,back,c,f )

/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                  */
#define PutString(pi,x,y,fore,back,pc) PutStringFontEx( pi, x, y, fore, back, pc, StrLen(pc), NULL )
/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                  */
#define PutStringEx(pi,x,y,color,back,pc,len) PutStringFontEx( pi, x, y, color,back,pc,len,NULL )
/* <combine sack::image::PutStringFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                  */
#define PutStringFont(pi,x,y,fore,back,pc,font) PutStringFontEx(pi,x,y,fore,back,pc,StrLen(pc), font )

/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                          */
#define PutStringVertical(pi,x,y,fore,back,pc) PutStringVerticalFontEx( pi, x, y, fore, back, pc, StrLen(pc), NULL )
/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                          */
#define PutStringVerticalEx(pi,x,y,color,back,pc,len) PutStringVerticalFontEx( pi, x, y, color,back,pc,len,NULL )
/* <combine sack::image::PutStringVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                          */
#define PutStringVerticalFont(pi,x,y,fore,back,pc,font) PutStringVerticalFontEx(pi,x,y,fore,back,pc,StrLen(pc), font )

/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                        */
#define PutStringInvert( pi, x, y, fore, back, pc ) PutStringInvertFontEx( pi, x, y, fore, back, pc,StrLen(pc), NULL )
/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                        */
#define PutStringInvertEx( pi, x, y, fore, back, pc, nLen ) PutStringInvertFontEx( pi, x, y, fore, back, pc, nLen, NULL )
/* <combine sack::image::PutStringInvertFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   The non Ex Version doesn't pass the string length.                                         */
#define PutStringInvertFont( pi, x, y, fore, back, pc, nLen ) PutStringInvertFontEx( pi, x, y, fore, back, pc, StrLen(pc), font )

/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                                */
#define PutStringInvertVertical( pi, x, y, fore, back, pc ) PutStringInvertVerticalFontEx( pi, x, y, fore, back, pc, StrLen(pc), NULL )
/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                                */
#define PutStringInvertVerticalEx( pi, x, y, fore, back, pc, nLen ) PutStringInvertVerticalFontEx( pi, x, y, fore, back, pc, nLen, NULL )
/* <combine sack::image::PutStringInvertVerticalFontEx@Image@S_32@S_32@CDATA@CDATA@CTEXTSTR@_32@SFTFont>
   
   \ \                                                                                                */
#define PutStringInvertVerticalFont( pi, x, y, fore, back, pc, font ) PutStringInvertVerticalFontEx( pi, x, y, fore, back, pc, StrLen(pc), font )

//IMG_PROC _32 PutMenuStringFontEx        ( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, PFONT font );
//#define PutMenuStringFont(img,x,y,fore,back,string,font) PutMenuStringFontEx( img,x,y,fore,back,string,StrLen(string),font)
//#define PutMenuString(img,x,y,fore,back,str)           PutMenuStringFont(img,x,y,fore,back,str,NULL)
//
//IMG_PROC _32 PutCStringFontEx           ( ImageFile *pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, PFONT font );
//#define PutCStringFont(img,x,y,fore,back,string,font) PutCStringFontEx( img,x,y,fore,back,string,StrLen(string),font)
//#define PutCString( img,x,y,fore,back,string) PutCStringFont(img,x,y,fore,back,string,NULL )

/* <combine sack::image::GetStringSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@SFTFont>
   
   \ \                                                                      */

#define GetStringSizeEx(s,len,pw,ph) GetStringSizeFontEx( (s),len,pw,ph,NULL)
/* <combine sack::image::GetStringSizeFontEx@CTEXTSTR@size_t@_32 *@_32 *@SFTFont>
   
   \ \                                                                         */
#define GetStringSize(s,pw,ph)       GetStringSizeFontEx( (s),StrLen(s),pw,ph,NULL)
/* <combine sack::image::GetStringSizeFontEx@CTEXTSTR@_32@_32 *@_32 *@SFTFont>
   
   \ \                                                                      */
#define GetStringSizeFont(s,pw,ph,f) GetStringSizeFontEx( (s),StrLen(s),pw,ph,f )

#ifdef __cplusplus
IMAGE_NAMESPACE_END
#ifdef _D3D_DRIVER
using namespace sack::image::d3d;
#elif defined( _D3D10_DRIVER )
using namespace sack::image::d3d10;
#elif defined( _D3D11_DRIVER )
using namespace sack::image::d3d11;
#else
using namespace sack::image;
#endif
#endif

#endif



/*   */
