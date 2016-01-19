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
IMAGE_NAMESPACE

extern void CPROC MarkImageUpdated( Image child_image );


void CPROC Nothing( void )
{
   return;
}

IMAGE_INTERFACE RealImageInterface = {
  SetStringBehavior
, SetBlotMethod

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
#ifdef STUPID_NO_DATA_EXPORTS
   , &_plot                           
   , &_plotalpha                      
   , &_getpixel

   , &_do_line                        
   , &_do_lineAlpha                   

   , &_do_hline                       
   , &_do_vline                       
   , &_do_hlineAlpha                  
   , &_do_vlineAlpha
#else
   , &plot                           
   , &plotalpha                      
   , &getpixel

   , &do_line                        
   , &do_lineAlpha                   

   , &do_hline                       
   , &do_vline                       
   , &do_hlineAlpha                  
   , &do_vlineAlpha
#endif
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
   SetBlotMethod( BLOT_C );
//#else
	// cleans up declarations.
//   SetBlotMethod( BLOT_C );
//#endif
}

int link_interface_please;
IMAGE_NAMESPACE_END


// $Log: interface.c,v $
// Revision 1.27  2005/04/13 18:29:14  jim
// Export DecodeMemoryToImage in interface.
//
// Revision 1.26  2005/04/05 11:56:04  panther
// Adding sprite support - might have added an extra draw callback...
//
// Revision 1.25  2005/01/26 06:51:58  panther
// Make image interface declaration static (private)
//
// Revision 1.24  2005/01/18 10:48:19  panther
// Define image interface export so there's no conflict between image and display_image
//
// Revision 1.23  2004/10/25 10:39:58  d3x0r
// Linux compilation cleaning requirements...
//
// Revision 1.22  2004/10/04 20:08:38  d3x0r
// Minor adjustments for static linking
//
// Revision 1.21  2004/06/21 07:47:13  d3x0r
// Account for newly moved structure files.
//
// Revision 1.20  2004/03/04 01:09:50  d3x0r
// Modifications to force slashes to wlib.  Updates to Interfaces to be gotten from common interface manager.
//
// Revision 1.19  2003/09/19 16:40:35  panther
// Implement Adopt and Orphan sub image - for up coming Sheet Control
//
// Revision 1.18  2003/09/18 12:14:49  panther
// MergeRectangle Added.  Seems Control edit near done (fixing move/size errors)
//
// Revision 1.17  2003/09/15 17:06:37  panther
// Fixed to image, display, controls, support user defined clipping , nearly clearing correct portions of frame when clearing hotspots...
//
// Revision 1.16  2003/08/30 10:05:01  panther
// Fix clipping blotted images beyond dest boundries
//
// Revision 1.15  2003/07/24 15:21:34  panther
// Changes to make watcom happy
//
// Revision 1.14  2003/04/24 00:03:49  panther
// Added ColorAverage to image... Fixed a couple macros
//
// Revision 1.13  2003/03/29 15:52:53  panther
// Add DropImageInterface
//
// Revision 1.12  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
