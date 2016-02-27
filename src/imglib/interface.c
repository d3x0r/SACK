#define IMAGE_LIBRARY_SOURCE
#define STRUCT_ONLY
#include <stdhdrs.h>
#include <imglib/imagestruct.h>
#include <procreg.h>
#undef IMAGE_SOURCE
#include <image.h>
#include "fntglobal.h"
#include "image_common.h"
#include "pngimage.h"
#include "jpgimage.h"
#include "local.h"
IMAGE_NAMESPACE

extern void CPROC MarkImageUpdated( Image child_image );


void CPROC Nothing( void )
{
   return;
}

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
  										 , ColorAverage
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

												 , DumpFontCache
												 , RerenderFont
												 , (int(CPROC*)(Image,int))Nothing // ReloadTexture
												 , (int(CPROC*)(Image,int,CDATA))Nothing // ReloadShadedTexture
												 , (int(CPROC*)(Image,int,CDATA,CDATA,CDATA))Nothing // ReloadMultiShadedTexture
												 , SetImageTransformRelation // set image transofrm
												 , NULL // render 3d image
                                     , DumpFontFile
									 , NULL //IMAGE_PROC_PTR( void, Render3dText )( CTEXTSTR string, int characters, CDATA color, SFTFont font, VECTOR o, LOGICAL render_pixel_scaled );
									 , TransferSubImages //IMAGE_PROC_PTR( void, TransferSubImages )( Image pImageTo, Image pImageFrom );
									 , NULL //IMAGE_PROC_PTR( Image, GetNativeImage )( Image pImageTo );
									 , GetTintedImage
									, GetShadedImage 
									, IsImageTargetFinal
									, NULL
									, PutStringFontExx
												 , NULL  // reset image buffers... proxy layer
												 , PngImageFile
                                     , JpgImageFile
									 , SetFontBias
									 , MakeSlicedImage
									 , MakeSlicedImageComplex
									 , UnmakeSlicedImage
									 , BlotSlicedImageEx
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

#if !(defined( SACK_BAG_EXPORTS ) && defined( __LINUX__ ))
 void  DropImageInterface ( PIMAGE_INTERFACE p )
{
   _ImageDropImageInterface(p );
}
#endif

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 45
#endif
PRIORITY_PRELOAD( ImageRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
#ifdef SACK_BAG_EXPORTS
#  ifdef __cplusplus_cli 
#     define NAME WIDE("sack.image")
#  else
#    ifdef __cplusplus
#define NAME WIDE("sack.image++")
#    else
#define NAME WIDE("sack.image")
#    endif
#  endif
#else
#  ifdef UNDER_CE
#define NAME WIDE("image")
#  else
#    ifdef __cplusplus
#      ifdef __cplusplus_cli 
#         define NAME WIDE("sack.image")
#      else
#         define NAME WIDE("sack.image++")
#      endif
#    else
#      define NAME WIDE("sack.image")
#    endif
#  endif
#endif

	RegisterInterface( NAME, _ImageGetImageInterface, _ImageDropImageInterface );
//#ifndef _DEBUG
//  MMX/Assembly do not support 
//   alpha translation of multishaded imaged.
//   mono shaded images were updated a while ago also,
//   and the related changes may not exist in assembly objects.
//   The only safe method is C anyhow.  AND it's not that slow
//   what with modern compilers and all.
   //SetBlotMethod( BLOT_C );
//#else
	// cleans up declarations.
//   SetBlotMethod( BLOT_C );
//#endif
}

int link_interface_please;
IMAGE_NAMESPACE_END

