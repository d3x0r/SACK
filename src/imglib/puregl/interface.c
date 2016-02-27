#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define STRUCT_ONLY
#include <stdhdrs.h>
#include <imglib/imagestruct.h>
#include <procreg.h>
#include <sqlgetoption.h>
#undef IMAGE_SOURCE
#include <image.h>
#include "../fntglobal.h"
#define REQUIRE_GLUINT
#include "../image_common.h"
#include "../pngimage.h"
#include "../jpgimage.h"
#include "local.h"

IMAGE_NAMESPACE

IMAGE_INTERFACE RealImageInterface = {
  SetStringBehavior
, NULL//SetBlotMethod

, BuildImageFileEx               
, MakeImageFileEx                
, MakeSubImageEx                 
, RemakeImageEx                  
, LoadImageFileEx
, UnmakeImageFileEx              

, ResizeImageEx                  
, MoveImage                      

   , BlatColor                      
   , BlatColorAlpha                 
   , BlotImageEx                    
   , BlotImageSizedEx               
   , BlotScaledImageSizedEx         

   //, &plotraw                        
   , plot                           
   , plotalpha                      
   , getpixel

   , do_line                        
   , do_lineAlpha                   

   , do_hline                       
   , do_vline                       
   , do_hlineAlpha                  
   , do_vlineAlpha
   , GetDefaultFont
   , GetFontHeight
   , GetStringSizeFontEx

   , PutCharacterFont               
   , PutCharacterVerticalFont       
   , PutCharacterInvertFont         
   , PutCharacterVerticalInvertFont 

   , PutStringFontEx
   , PutStringVerticalFontEx
   , PutStringInvertFontEx
   , PutStringInvertVerticalFontEx

   //, PutMenuStringFontEx
   //, PutCStringFontEx
   , GetMaxStringLengthFont
, GetImageSize
, LoadFont
                                 , UnloadFont
                                 , NULL // begin transfer
                                 , NULL // contin transfer
                                 , NULL // decode image
                                 , NULL // accept font
#ifdef STUPID_NO_DATA_EXPORTS
												 , &_ColorAverage
#else
												 , &ColorAverage
#endif
                                 , SyncImage
                                , GetImageSurface
                                , IntersectRectangle
                                , MergeRectangle
                                , GetImageAuxRect
                                , SetImageAuxRect
                                , OrphanSubImage
                                , AdoptSubImage

,MakeSpriteImageFileEx
,MakeSpriteImageEx
,rotate_scaled_sprite
,rotate_sprite
													 ,NULL //BlotSprite
                                        ,DecodeMemoryToImage

													 ,InternalRenderFontFile
													 ,InternalRenderFont

													 ,RenderScaledFontData
													 ,RenderFontFileScaledEx
												 , DestroyFont
                                     , NULL // *global_font_data
												 , GetFontRenderData
                                     , SetFontRendererData

												 , SetSpriteHotspot
												 , SetSpritePosition
                                     , UnmakeSprite
												 , GetGlobalFonts
												 , GetStringRenderSizeFontEx
												 , LoadImageFileFromGroupEx
                                     , RenderScaledFont
                                     , RenderScaledFontEx

									 , GetRedValue     
									 , GetGreenValue   
									 , GetBlueValue    
									 , GetAlphaValue   
									 , SetRedValue     
									 , SetGreenValue   
									 , SetBlueValue    
									 , SetAlphaValue   
									 , MakeColor       
												 , MakeAlphaColor
												 , GetImageTransformation
												 , SetImageRotation
												 , RotateImageAbout
												 , MarkImageUpdated

                                     , DumpFontCache // DumpFontCache
                                     , RerenderFont  // RerenderFont

												 , ReloadOpenGlTexture
												 , ReloadOpenGlShadedTexture
												 , ReloadOpenGlMultiShadedTexture
												 , SetImageTransformRelation
                                     , Render3dImage

                                     , DumpFontFile
									 , Render3dText
									 , TransferSubImages
									 , NULL //IMAGE_PROC_PTR( Image, GetNativeImage )( Image pImageTo );
									 , NULL //GetTintedImage
									, NULL //GetShadedImage 
									, IsImageTargetFinal
									, NULL
									, PutStringFontExx
												 , NULL  // reset image buffers... proxy layer
												 , PngImageFile
                                     , JpgImageFile
									 , SetFontBias
};

#undef GetImageInterface
#undef DropImageInterface
static POINTER CPROC _ImageGetImageInterface( void )
{
   //RealImageInterface._global_font_data = GetGlobalFonts();
   return &RealImageInterface;
}

 PIMAGE_INTERFACE  GetImageInterface ( void )
{
   return (PIMAGE_INTERFACE)_ImageGetImageInterface();
}


static void CPROC _ImageDropImageInterface( POINTER p )
{
}

#ifndef __STATIC__
#if !(defined( SACK_BAG_EXPORTS ) && defined( __LINUX__ ))
 void  DropImageInterface ( PIMAGE_INTERFACE p )
{
   _ImageDropImageInterface(p );
}
#endif
#endif

PRIORITY_PRELOAD( ImageRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( WIDE("puregl.image"), _ImageGetImageInterface, _ImageDropImageInterface );
	l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Image Library/Scale"), 10 );
	if( l.scale == 0.0 )
	{
		l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Image Library/Inverse Scale"), 2 );
		if( l.scale == 0.0 )
			l.scale = 1;
	}
	else
		l.scale = 1.0 / l.scale;

	// this initializes some of the interface methods
	//SetBlotMethod( BLOT_C );
	RealImageInterface._IsImageTargetFinal = IsImageTargetFinal;
}

int link_interface_please;
IMAGE_NAMESPACE_END

