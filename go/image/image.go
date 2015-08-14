package image
//package image

/*
#cgo LDFLAGS:-lbag
//#cgo LDFLAGS: -Lc:/general/build/mingw/sack/debug_out/core/lib
//#cgo CFLAGS:-IM:/sack/include
//#cgo CFLAGS:-D_DEBUG
#include <image.h>


#define IMAGE_PROC_CALL(a,b)  a _##b 


IMAGE_PROC_CALL( void, SetStringBehavior)( Image pImage, _32 behavior );
IMAGE_PROC_CALL( void, SetBlotMethod)( _32 method );

IMAGE_PROC_CALL( Image,BuildImageFileEx)( PCOLOR pc, _32 width, _32 height DBG_PASS);
IMAGE_PROC_CALL( Image,MakeImageFileEx)(_32 Width, _32 Height DBG_PASS);
IMAGE_PROC_CALL( Image,MakeSubImageEx)( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
IMAGE_PROC_CALL( Image,RemakeImageEx)( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS);
IMAGE_PROC_CALL( Image,LoadImageFileEx)( CTEXTSTR name DBG_PASS );
IMAGE_PROC_CALL( void,UnmakeImageFileEx)( Image pif DBG_PASS );
void __UnmakeImageFile( Image pif )
{
	_UnmakeImageFileEx( pif DBG_SRC );
}
#define UnmakeImageFile(pif) UnmakeImageFileEx( pif DBG_SRC )
//-----------------------------------------------------

IMAGE_PROC_CALL( void,ResizeImageEx)( Image pImage, S_32 width, S_32 height DBG_PASS);
IMAGE_PROC_CALL( void,MoveImage)( Image pImage, S_32 x, S_32 y );

//-----------------------------------------------------

IMAGE_PROC_CALL( void,BlatColor)( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );
IMAGE_PROC_CALL( void,BlatColorAlpha)( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color );

IMAGE_PROC_CALL( void,BlotImageEx)( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ... );

void __BlotImage( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent )
{
	_BlotImageEx( pDest, pIF, x, y, nTransparent, BLOT_COPY );
}

void __BlotImageShaded( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, CDATA shade )
{
	_BlotImageEx( pDest, pIF, x, y, nTransparent, BLOT_SHADED, shade );
}

IMAGE_PROC_CALL( void,BlotImageSizedEx)( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ... );

IMAGE_PROC_CALL( void,BlotScaledImageSizedEx)( Image pifDest, Image pifSrc
                                   , S_32 xd, S_32 yd
                                   , _32 wd, _32 hd
                                   , S_32 xs, S_32 ys
                                   , _32 ws, _32 hs
                                   , _32 nTransparent
                                   , _32 method, ... );


IMAGE_PROC_CALL( void,plot)( Image pi, S_32 x, S_32 y, CDATA c );

   IMAGE_PROC_CALL( void,plotalpha)( Image pi, S_32 x, S_32 y, CDATA c );
   IMAGE_PROC_CALL( CDATA,getpixel)( Image pi, S_32 x, S_32 y );
   IMAGE_PROC_CALL( void,do_line)( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  // d is color data...
   IMAGE_PROC_CALL( void,do_lineAlpha)( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color);  // d is color data...

   IMAGE_PROC_CALL( void,do_hline)( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
   IMAGE_PROC_CALL( void,do_vline)( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
   IMAGE_PROC_CALL( void,do_hlineAlpha)( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
   IMAGE_PROC_CALL( void,do_vlineAlpha)( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );

   IMAGE_PROC_CALL( SFTFont,GetDefaultFont)( void );
   IMAGE_PROC_CALL( _32 ,GetFontHeight)( SFTFont );
   IMAGE_PROC_CALL( _32 ,GetStringSizeFontEx)( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont );

   IMAGE_PROC_CALL( void,PutCharacterFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
   IMAGE_PROC_CALL( void,PutCharacterVerticalFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
   IMAGE_PROC_CALL( void,PutCharacterInvertFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
   IMAGE_PROC_CALL( void,PutCharacterVerticalInvertFont)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );

   IMAGE_PROC_CALL( void,PutStringFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
   IMAGE_PROC_CALL( void,PutStringVerticalFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
   IMAGE_PROC_CALL( void,PutStringInvertFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );
   IMAGE_PROC_CALL( void,PutStringInvertVerticalFontEx)( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font );

   IMAGE_PROC_CALL( _32, GetMaxStringLengthFont )( _32 width, SFTFont UseFont );


         IMAGE_PROC_CALL( void, UnloadFont )( SFTFont font );

IMAGE_PROC_CALL( DataState, BeginTransferData )( _32 total_size, _32 segsize, CDATA data );
IMAGE_PROC_CALL( void, ContinueTransferData )( DataState state, _32 segsize, CDATA data );
IMAGE_PROC_CALL( Image, DecodeTransferredImage )( DataState state );
IMAGE_PROC_CALL( SFTFont, AcceptTransferredFont )( DataState state );
IMAGE_PROC_CALL( CDATA, ColorAverage )( CDATA c1, CDATA c2, int d, int max );
IMAGE_PROC_CALL( void, SyncImage )( void );
         IMAGE_PROC_CALL( PCDATA, GetImageSurface )( Image pImage );
         IMAGE_PROC_CALL( int, IntersectRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   IMAGE_PROC_CALL( int, MergeRectangle )( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 );
   IMAGE_PROC_CALL( void, GetImageAuxRect )( Image pImage, P_IMAGE_RECTANGLE pRect );
   IMAGE_PROC_CALL( void, SetImageAuxRect )( Image pImage, P_IMAGE_RECTANGLE pRect );
   IMAGE_PROC_CALL( void, OrphanSubImage )( Image pImage );
   IMAGE_PROC_CALL( void, AdoptSubImage )( Image pFoster, Image pOrphan );
	IMAGE_PROC_CALL( PSPRITE, MakeSpriteImageFileEx )( CTEXTSTR fname DBG_PASS );
	IMAGE_PROC_CALL( PSPRITE, MakeSpriteImageEx )( Image image DBG_PASS );
	IMAGE_PROC_CALL( void   , rotate_scaled_sprite )(Image bmp, PSPRITE sprite, fixed angle, fixed scale_width, fixed scale_height);
	IMAGE_PROC_CALL( void   , rotate_sprite )(Image bmp, PSPRITE sprite, fixed angle);
		IMAGE_PROC_CALL( void   , BlotSprite )( Image pdest, PSPRITE ps );
    IMAGE_PROC_CALL( Image, DecodeMemoryToImage )( P_8 buf, _32 size );

	IMAGE_PROC_CALL( SFTFont, InternalRenderFontFile )( CTEXTSTR file
       								 , S_32 nWidth
       								 , S_32 nHeight
       										, PFRACTION width_scale
       										, PFRACTION height_scale
       								 , _32 flags 
       								 );
	IMAGE_PROC_CALL( SFTFont, InternalRenderFont )( _32 nFamily
      							, _32 nStyle
      							, _32 nFile
      							, S_32 nWidth
      							, S_32 nHeight
      										, PFRACTION width_scale
      										, PFRACTION height_scale
      							, _32 flags
      							);
IMAGE_PROC_CALL( SFTFont, RenderScaledFontData)( PFONTDATA pfd, PFRACTION width_scale, PFRACTION height_scale );
IMAGE_PROC_CALL( SFTFont, RenderFontFileScaledEx )( CTEXTSTR file, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *size, POINTER *pFontData );
IMAGE_PROC_CALL( void, DestroyFont)( SFTFont *font );
struct font_global_tag *_global_font_data;
IMAGE_PROC_CALL( int, GetFontRenderData )( SFTFont font, POINTER *fontdata, size_t *fontdatalen );
IMAGE_PROC_CALL( void, SetFontRendererData )( SFTFont font, POINTER pResult, size_t size );
IMAGE_PROC_CALL( PSPRITE, SetSpriteHotspot )( PSPRITE sprite, S_32 x, S_32 y );
IMAGE_PROC_CALL( PSPRITE, SetSpritePosition )( PSPRITE sprite, S_32 x, S_32 y );
	IMAGE_PROC_CALL( void, UnmakeSprite )( PSPRITE sprite, int bForceImageAlso );
IMAGE_PROC_CALL( struct font_global_tag *, GetGlobalFonts)( void );

IMAGE_PROC_CALL( _32, GetStringRenderSizeFontEx )( CTEXTSTR pString, size_t nLen, _32 *width, _32 *height, _32 *charheight, SFTFont UseFont );

IMAGE_PROC_CALL( Image, LoadImageFileFromGroupEx )( INDEX group, CTEXTSTR filename DBG_PASS );

IMAGE_PROC_CALL( SFTFont, RenderScaledFont )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags );
IMAGE_PROC_CALL( SFTFont, RenderScaledFontEx )( CTEXTSTR name, _32 width, _32 height, PFRACTION width_scale, PFRACTION height_scale, _32 flags, size_t *pnFontDataSize, POINTER *pFontData );

IMAGE_PROC_CALL( COLOR_CHANNEL, GetRedValue )( CDATA color ) ;
IMAGE_PROC_CALL( COLOR_CHANNEL, GetGreenValue )( CDATA color );
IMAGE_PROC_CALL( COLOR_CHANNEL, GetBlueValue )( CDATA color );
IMAGE_PROC_CALL( COLOR_CHANNEL, GetAlphaValue )( CDATA color );
IMAGE_PROC_CALL( CDATA, SetRedValue )( CDATA color, COLOR_CHANNEL r ) ;
IMAGE_PROC_CALL( CDATA, SetGreenValue )( CDATA color, COLOR_CHANNEL green );
IMAGE_PROC_CALL( CDATA, SetBlueValue )( CDATA color, COLOR_CHANNEL b );
IMAGE_PROC_CALL( CDATA, SetAlphaValue )( CDATA color, COLOR_CHANNEL a );
IMAGE_PROC_CALL( CDATA, MakeColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b );
IMAGE_PROC_CALL( CDATA, MakeAlphaColor )( COLOR_CHANNEL r, COLOR_CHANNEL green, COLOR_CHANNEL b, COLOR_CHANNEL a );


IMAGE_PROC_CALL( PTRANSFORM, GetImageTransformation )( Image pImage );
IMAGE_PROC_CALL( void, SetImageRotation )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz );
IMAGE_PROC_CALL( void, RotateImageAbout )( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle );
IMAGE_PROC_CALL( void, MarkImageDirty )( Image pImage );

IMAGE_PROC_CALL( void, DumpFontCache )( void );
IMAGE_PROC_CALL( void, RerenderFont )( SFTFont font, S_32 width, S_32 height, PFRACTION width_scale, PFRACTION height_scale );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_CALL( int, ReloadTexture )( Image child_image, int option );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_CALL( int, ReloadShadedTexture )( Image child_image, int option, CDATA color );
// option(1) == use GL_RGBA_EXT; option(2)==clamp; option(4)==repeat
IMAGE_PROC_CALL( int, ReloadMultiShadedTexture )( Image child_image, int option, CDATA red, CDATA green, CDATA blue );

IMAGE_PROC_CALL( void, SetImageTransformRelation )( Image pImage, enum image_translation_relation relation, PRCOORD aux );
IMAGE_PROC_CALL( void, Render3dImage )( Image pImage, PCVECTOR o, LOGICAL render_pixel_scaled );
IMAGE_PROC_CALL( void, DumpFontFile )( CTEXTSTR name, SFTFont font_to_dump );
IMAGE_PROC_CALL( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, PCVECTOR o, LOGICAL render_pixel_scaled );

// transfer all sub images to new image using appropriate methods
// extension for internal fonts to be utilized by external plugins...
IMAGE_PROC_CALL( void, TransferSubImages )( Image pImageTo, Image pImageFrom );
// when using reverse interfaces, need a way to get the real image
// from the fake image (proxy image) 
IMAGE_PROC_CALL( Image, GetNativeImage )( Image pImageTo );

// low level support for proxy; this exposes some image_common.c routines
IMAGE_PROC_CALL( Image, GetTintedImage )( Image child_image, CDATA color );
IMAGE_PROC_CALL( Image, GetShadedImage )( Image child_image, CDATA red, CDATA green, CDATA blue );
// test for IF_FLAG_FINAL_RENDER (non physical surface/prevent local copy-restore)
IMAGE_PROC_CALL( LOGICAL, IsImageTargetFinal )( Image image );

// use image data to create a clone of the image for the new application instance...
// this is used when a common image resource is used for all application instances
// it should be triggered during onconnect.
// it is a new image instance that should be used for future app references...
IMAGE_PROC_CALL( Image, ReuseImage )( Image image );
IMAGE_PROC_CALL( void, PutStringFontExx )( Image pImage
											 , S_32 x, S_32 y
											 , CDATA color, CDATA background
											 , CTEXTSTR pc, size_t nLen, SFTFont font, int justification, _32 _width );
// sometimes it's not possible to use blatcolor to clear an imate...
// sometimes its parent is not redrawn?
IMAGE_PROC_CALL( void, ResetImageBuffers )( Image image, LOGICAL image_only );
	IMAGE_PROC_CALL(  LOGICAL, PngImageFile )( Image image, P_8 *buf, size_t *size );
	IMAGE_PROC_CALL(  LOGICAL, JpgImageFile )( Image image, P_8 *buf, size_t *size, int Q );
	IMAGE_PROC_CALL(  void, SetFontBias )( SFTFont font, S_32 x, S_32 y );



*/
import "C"
import "fmt"
import "unsafe"

type Image C.Image
//export _Ctype_Image

//var src *C.char = C.CString("image.go")
//var img C.PIMAGE_INTERFACE = C.PIMAGE_INTERFACE(C.GetInterfaceDbg( C.CString("image"), src, 0 ));


const (
	ALPHA_TRANSPARENT = 0x100
        ALPHA_TRANSPARENT_INVERT = 0x200
	BLOT_COPY = 0
   // copy the pixels from one image to another with no color transform, scaling by a single color
	BLOT_SHADED = 1
   // copy the pixels from one image to another with no color transform, scaling independant R, G and B color channels to a combination of an R Color, B Color, G Color
	BLOT_MULTISHADE = 2
   /* copy the pixels from one image to another with simple color inversion transform*/
	BLOT_INVERTED = 3
 /* orientation blots for fonts to 3D and external displays */
	BLOT_ORIENT_NORMAL = 0x00
	BLOT_ORIENT_INVERT = 0x04
	BLOT_ORIENT_VERTICAL = 0x08
	BLOT_ORIENT_VERTICAL_INVERT = 0x0C
	BLOT_ORIENTATTION = 0x0C
)

type Color struct {	
	rgba uint32;
}

func MakeImage( image C.Image ) Image {
	return Image(image); 
}

func MakeImageFromPointer( image unsafe.Pointer ) Image {
	return Image( C.Image( image ) );
}

func MakeColor( r,g,b,a int ) Color {
	return Color { rgba : uint32(( r ) | ( b << 8 ) | ( g << 16 ) | ( a << 24 )) };
}
func SetColor( rgba uint32 ) Color {
	return Color { rgba : rgba };
}

func Clear( image Image ) {
	C._BlatColor( image, 0, 0, C._32(image.width), C._32(image.height), 0 );
}

func ClearTo( image Image, c Color ) {
	C._BlatColor( image, 0, 0, C._32(image.width), C._32(image.height), C.CDATA( c.rgba ) );
}

func FillRect( image Image,x,y int, w,h uint, c Color ) {
	fmt.Println( "image is ", image, " color is ", c.rgba );
	C._BlatColor( image, C.S_32(x), C.S_32(y), C._32(w), C._32(h), C.CDATA( c.rgba ) );
}


func LoadImage( filename string ) Image {
	tmp := C.CString( filename )
	img := Image( C._LoadImageFileEx( tmp ) ) 
        C.free( unsafe.Pointer( tmp ) );
        return img;
}

func DeleteImage( image Image ) {
	C.__UnmakeImageFile( image );
}

func BlotImage( dest Image, source Image, x, y int  ) {
	C.__BlotImage( dest, source, C.S_32(x), C.S_32(y), 0 );
}

func BlotImageAlpha( dest Image, image Image, x, y int  ) {
	C.__BlotImage( dest, image, C.S_32(x), C.S_32(y), ALPHA_TRANSPARENT );
}


