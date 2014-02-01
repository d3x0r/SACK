#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define STRUCT_ONLY
#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#include <imglib/imagestruct.h>
#include <sqlgetoption.h>
#include <procreg.h>
#undef IMAGE_SOURCE
#include <image.h>
#include "../fntglobal.h"
#include "local.h"

IMAGE_NAMESPACE

extern void MarkImageUpdated( Image child_image );

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
};

IMAGE_3D_INTERFACE Image3dInterface = {
	GetShader,
      CompileShader,
      CompileShaderEx,
		EnableShader,
		SetShaderEnable,
		SetShaderModelView,
};

#undef GetImageInterface
#undef DropImageInterface
#undef GetImage3dInterface


static POINTER CPROC _ImageGetImageInterface( void )
{
   //RealImageInterface._global_font_data = GetGlobalFonts();
   return &RealImageInterface;
}

 PIMAGE_INTERFACE  GetImageInterface ( void )
{
   return (PIMAGE_INTERFACE)_ImageGetImageInterface();
}

static POINTER CPROC GetImage3dInterface( void )
{
	return &Image3dInterface;
}

static void  CPROC DropImage3dInterface ( POINTER p )
{
	;
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

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 45
#endif



PRIORITY_PRELOAD( ImageRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( WIDE("d3d11.image"), _ImageGetImageInterface, _ImageDropImageInterface );
	RegisterInterface( WIDE("d3d11.image.3d"), GetImage3dInterface, DropImage3dInterface );
	l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Image Library/Scale"), 10 );
	if( l.scale == 0.0 )
	{
		l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), WIDE("SACK/Image Library/Inverse Scale"), 2 );
		if( l.scale == 0.0 )
			l.scale = 1;
	}
	else
		l.scale = 1.0f / l.scale;
	// this initializes some of the interface methods
	SetBlotMethod( BLOT_C );
	RealImageInterface._IsImageTargetFinal = IsImageTargetFinal;
}

int link_interface_please;
IMAGE_NAMESPACE_END

