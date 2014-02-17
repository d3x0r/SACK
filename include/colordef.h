/* Define COLOR type. Basically the image library regards color
   as 32 bits of data. User applications end up needing to
   specify colors in the correct method for the platform they
   are working on. This provides aliases to rearrange colors.
   For instance the colors on windows and the colors for OpenGL
   are not exactly the same. If the OpenGL driver is specified
   as the output device, the entire code would need to be
   rebuilt for specifying colors correctly for opengl. While
   otherwise they are both 32 bits, and peices work, they get
   very ugly colors output.
   See Also
   <link Colors>                                                */
#ifndef COLOR_STRUCTURE_DEFINED
/* An exclusion symbol for defining CDATA and color operations. */
#define COLOR_STRUCTURE_DEFINED

#include <sack_types.h>

#ifdef __cplusplus
SACK_NAMESPACE
	namespace image {
#endif

		// byte index values for colors on the video buffer...
		enum color_byte_index {
 I_BLUE  = 0, /* index of blue byte in a byte array of color. */
 
 I_GREEN = 1, /* index of green byte in a byte array of color. */
 
 I_RED   = 2, /* index of red byte in a byte array of color. */
 
 I_ALPHA = 3 /* index of alpha byte in a byte array of color. */
 
		};

#if !defined( IMAGE_LIBRARY_SOURCE_MAIN ) && !defined( FORCE_NO_INTERFACE )

#define Color( r,g,b ) MakeColor(r,g,b)
#define AColor( r,g,b,a ) MakeAlphaColor(r,g,b,a)
#define SetAlpha( rgb, a ) SetAlphaValue( rgb, a )
#define SetGreen( rgb, g ) SetGreeValue(rgb,g )
#define AlphaVal(color) GetAlphaValue( color )
#define RedVal(color)   GetRedValue(color)
#define GreenVal(color) GetGreenValue(color)
#define BlueVal(color)  GetBlueValue(color)

#else
#if defined( _OPENGL_DRIVER ) || defined( USE_OPENGL_COMPAT_COLORS )
#  define Color( r,g,b ) (((_32)( ((_8)(r))|((_16)((_8)(g))<<8))|(((_32)((_8)(b))<<16)))|0xFF000000)
#  define AColor( r,g,b,a ) (((_32)( ((_8)(r))|((_16)((_8)(g))<<8))|(((_32)((_8)(b))<<16)))|((a)<<24))
#  define SetAlpha( rgb, a ) ( ((rgb)&0xFFFFFF) | ( (a)<<24 ) )
#  define SetGreen( rgb, g ) ( ((rgb)&0xFFFF00FF) | ( ((g)&0xFF)<<8 ) )
#  define SetBlue( rgb, b ) ( ((rgb)&0xFF00FFFF) | ( ((b)&0xFF)<<16 ) )
#  define SetRed( rgb, r ) ( ((rgb)&0xFFFFFF00) | ( ((r)&0xFF)<<0 ) )
#  define GLColor( c )  (c)
#  define AlphaVal(color) (((color) >> 24) & 0xFF)
#  define RedVal(color)   (((color)) & 0xFF)
#  define GreenVal(color) (((color) >> 8) & 0xFF)
#  define BlueVal(color)  (((color) >> 16) & 0xFF)
#else
#  ifdef _WIN64
#    define AND_FF &0xFF
#  else
/* This is a macro to cure a 64bit warning in visual studio. */
#    define AND_FF
#  endif
/* A macro to create a solid color from R G B coordinates.
   Example
   <code lang="c++">
   CDATA color1 = Color( 255,0,0 ); // Red only, so this is bright red
   CDATA color2 = Color( 0,255,0); // green only, this is bright green
   CDATA color3 = Color( 0,0,255); // blue only, this is birght blue
   CDATA color4 = Color(93,93,32); // this is probably a goldish grey
   </code>                                                             */
#define Color( r,g,b ) (((_32)( ((_8)((b)AND_FF))|((_16)((_8)((g))AND_FF)<<8))|(((_32)((_8)((r))AND_FF)<<16)))|0xFF000000)
/* Build a color with alpha specified. */
#define AColor( r,g,b,a ) (((_32)( ((_8)((b)AND_FF))|((_16)((_8)((g))AND_FF)<<8))|(((_32)((_8)((r))AND_FF)<<16)))|(((a)AND_FF)<<24))
/* Sets the alpha part of a color. (0-255 value, 0 being
   transparent, and 255 solid(opaque))
   Example
   <code lang="c++">
   CDATA color = BASE_COLOR_RED;
   
   CDATA hazy_color = SetAlpha( color, 128 );  
   </code>
 */
#define SetAlpha( rgb, a ) ( ((rgb)&0x00FFFFFF) | ( (a)<<24 ) )
/* Sets the green channel of a color. Expects a value 0-255.  */
#define SetGreen( rgb, g ) ( ((rgb)&0xFFFF00FF) | ( ((g)&0x0000FF)<<8 ) )
/* Sets the blue channel of a color. Expects a value 0-255.  */
#define SetBlue( rgb, b ) ( ((rgb)&0xFFFFFF00) | ( ((b)&0x0000FF)<<0 ) )
/* Sets the red channel of a color. Expects a value 0-255.  */
#define SetRed( rgb, r ) ( ((rgb)&0xFF00FFFF) | ( ((r)&0x0000FF)<<16 ) )
/* Return a CDATA that is meant for output to OpenGL. */
#define GLColor( c )  (((c)&0xFF00FF00)|(((c)&0xFF0000)>>16)|(((c)&0x0000FF)<<16))
/* Get the alpha value of a color. This is a 0-255 unsigned
   byte.                                                    */
#define AlphaVal(color) (((color) >> 24) & 0xFF)
/* Get the red value of a color. This is a 0-255 unsigned byte. */
#define RedVal(color)   (((color) >> 16) & 0xFF)
/* Get the green value of a color. This is a 0-255 unsigned
   byte.                                                    */
#define GreenVal(color) (((color) >> 8) & 0xFF)
/* Get the blue value of a color. This is a 0-255 unsigned byte. */
#define BlueVal(color)  (((color)) & 0xFF)
#endif

#endif // IMAGE_LIBRARY_SOURCE
		/* a definition for a single color channel - for function replacements for ___Val macros*/
		typedef unsigned char COLOR_CHANNEL;
        /* a 4 byte array of color (not really used, we mostly went with CDATA and PCDATA instead of COLOR and PCOLOR */
		typedef COLOR_CHANNEL COLOR[4];
		// color data raw...
		typedef _32 CDATA;
		/* pointer to an array of 32 bit colors */
		typedef _32 *PCDATA;
		/* A Pointer to <link COLOR>. Probably an array of color (a
		 block of pixels for instance)                            */
		typedef COLOR *PCOLOR;

//-----------------------------------------------
// common color definitions....
//-----------------------------------------------
// both yellows need to be fixed.
#define BASE_COLOR_BLACK         Color( 0,0,0 )
#define BASE_COLOR_BLUE          Color( 0, 0, 128 )
#define BASE_COLOR_DARKBLUE          Color( 0, 0, 42 )
/* An opaque Green.
   See Also
   <link Colors>    */
#define BASE_COLOR_GREEN         Color( 0, 128, 0 )
/* An opaque cyan - kind of a light sky like blue.
   See Also
   <link Colors>                                   */
#define BASE_COLOR_CYAN          Color( 0, 128, 128 )
/* An opaque red.
   See Also
   <link Colors>  */
#define BASE_COLOR_RED           Color( 192, 32, 32 )
/* An opaque BROWN. Brown is dark yellow... so this might be
   more like a gold sort of color instead.
   See Also
   <link Colors>                                             */
#define BASE_COLOR_BROWN         Color( 140, 140, 0 )
#define BASE_COLOR_LIGHTBROWN         Color( 221, 221, 85 )
#define BASE_COLOR_MAGENTA       Color( 160, 0, 160 )
#define BASE_COLOR_LIGHTGREY     Color( 192, 192, 192 )
/* An opaque darker grey (gray?).
   See Also
   <link Colors>                  */
#define BASE_COLOR_DARKGREY      Color( 128, 128, 128 )
/* An opaque a bight or light color blue.
   See Also
   <link Colors>                          */
#define BASE_COLOR_LIGHTBLUE     Color( 0, 0, 255 )
/* An opaque lighter, brighter green color.
   See Also
   <link Colors>                            */
#define BASE_COLOR_LIGHTGREEN    Color( 0, 255, 0 )
/* An opaque a lighter, more bight cyan color.
   See Also
   <link Colors>                               */
#define BASE_COLOR_LIGHTCYAN     Color( 0, 255, 255 )
/* An opaque bright red.
   See Also
   <link Colors>         */
#define BASE_COLOR_LIGHTRED      Color( 255, 0, 0 )
/* An opaque Lighter pink sort of red-blue color.
   See Also
   <link Colors>                                  */
#define BASE_COLOR_LIGHTMAGENTA  Color( 255, 0, 255 )
/* An opaque bright yellow.
   See Also
   <link Colors>            */
#define BASE_COLOR_YELLOW        Color( 255, 255, 0 )
/* An opaque White.
   See Also
   <link Colors>    */
#define BASE_COLOR_WHITE         Color( 255, 255, 255 )
#define BASE_COLOR_ORANGE        Color( 204,96,7 )
#define BASE_COLOR_NICE_ORANGE   Color( 0xE9, 0x7D, 0x26 )
#define BASE_COLOR_PURPLE        0xFF7A117C
#ifdef __cplusplus

}; // 	namespace image {
SACK_NAMESPACE_END
using namespace sack::image;
#endif
#endif



// $Log: colordef.h,v $
// Revision 1.4  2003/04/24 00:03:49  panther
// Added ColorAverage to image... Fixed a couple macros
//
// Revision 1.3  2003/03/25 08:38:11  panther
// Add logging
//
