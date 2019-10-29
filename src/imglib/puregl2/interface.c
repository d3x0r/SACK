#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define STRUCT_ONLY
#include <stdhdrs.h>
#include <procreg.h>
#include <sqlgetoption.h>
#undef IMAGE_SOURCE
#include <imglib/imagestruct.h>
#include <image.h>
#include <image3d.h>
#include "../fntglobal.h"
#include "local.h"
#include "shaders.h"
#include "../pngimage.h"
#include "../jpgimage.h"
IMAGE_NAMESPACE

#undef CreateShaderBuffer
static struct shader_buffer* CPROC CreateShaderBuffer( int dimensions, int start_size, int expand_by ) {
	return CreateShaderBuffer_( dimensions, start_size, expand_by DBG_SRC );
}

IMAGE_INTERFACE RealImageInterface = {
  IMGVER(SetStringBehavior)
, NULL //SetBlotMethod

, IMGVER(BuildImageFileEx               )
, IMGVER(MakeImageFileEx                )
, IMGVER(MakeSubImageEx                 )
, IMGVER(RemakeImageEx                  )
, IMGVER(LoadImageFileEx)
, IMGVER(UnmakeImageFileEx              )

, IMGVER(ResizeImageEx                  )
, IMGVER(MoveImage                      )

   , IMGVER(BlatColor                      )
   , IMGVER(BlatColorAlpha                 )
   , IMGVER(BlotImageEx                    )
   , IMGVER(BlotImageSizedEx               )
   , IMGVER(BlotScaledImageSizedEx         )

   //, &plotraw                        
   , IMGVER(plot)
   , IMGVER(plotalpha)
   , IMGVER(getpixel)

   , IMGVER(do_line)
   , IMGVER(do_lineAlpha)

   , IMGVER(do_hline)
   , IMGVER(do_vline)
   , IMGVER(do_hlineAlpha )
   , IMGVER(do_vlineAlpha )
   , IMGVER(GetDefaultFont)
   , IMGVER(GetFontHeight)
   , IMGVER(GetStringSizeFontEx)

   , IMGVER(PutCharacterFont               )
   , IMGVER(PutCharacterVerticalFont       )
   , IMGVER(PutCharacterInvertFont         )
   , IMGVER(PutCharacterVerticalInvertFont )

   , IMGVER(PutStringFontEx)
   , IMGVER(PutStringVerticalFontEx)
   , IMGVER(PutStringInvertFontEx)
   , IMGVER(PutStringInvertVerticalFontEx)

   //, PutMenuStringFontEx
   //, PutCStringFontEx
   , IMGVER(GetMaxStringLengthFont)
, IMGVER(GetImageSize)
, IMGVER(LoadFont)
                                 , IMGVER(UnloadFont)
                                 , NULL // begin transfer
                                 , NULL // contin transfer
                                 , NULL // decode image
                                 , NULL // accept font
#ifdef STUPID_NO_DATA_EXPORTS
												 , &IMGVER(_ColorAverage)
#else
												 , &IMGVER(ColorAverage)
#endif
                                 , IMGVER(SyncImage)
                                , IMGVER(GetImageSurface)
                                , IMGVER(IntersectRectangle)
                                , IMGVER(MergeRectangle)
                                , IMGVER(GetImageAuxRect)
                                , IMGVER(SetImageAuxRect)
                                , IMGVER(OrphanSubImage)
                                , IMGVER(AdoptSubImage)

,IMGVER(MakeSpriteImageFileEx)
,IMGVER(MakeSpriteImageEx)
,IMGVER(rotate_scaled_sprite)
,IMGVER(rotate_sprite)
													 ,NULL //BlotSprite
                                        ,IMGVER(DecodeMemoryToImage)

													 ,IMGVER(InternalRenderFontFile)
													 ,IMGVER(InternalRenderFont)

													 ,IMGVER(RenderScaledFontData)
													 ,IMGVER(RenderFontFileScaledEx)
												 , IMGVER(DestroyFont)
                                     , NULL // *global_font_data
												 , IMGVER(GetFontRenderData)
                                     , IMGVER(SetFontRendererData)

												 , IMGVER(SetSpriteHotspot)
												 , IMGVER(SetSpritePosition)
                                     , IMGVER(UnmakeSprite)
												 , IMGVER(GetGlobalFonts)
												 , IMGVER(GetStringRenderSizeFontEx)
												 , IMGVER(LoadImageFileFromGroupEx)
                                     , IMGVER(RenderScaledFont)
                                     , IMGVER(RenderScaledFontEx)

									 , IMGVER(GetRedValue     )
									 , IMGVER(GetGreenValue   )
									 , IMGVER(GetBlueValue    )
									 , IMGVER(GetAlphaValue   )
									 , IMGVER(SetRedValue     )
									 , IMGVER(SetGreenValue   )
									 , IMGVER(SetBlueValue    )
									 , IMGVER(SetAlphaValue   )
									 , IMGVER(MakeColor       )
												 , IMGVER(MakeAlphaColor)
												 , IMGVER(GetImageTransformation)
												 , IMGVER(SetImageRotation)
												 , IMGVER(RotateImageAbout)
												 , IMGVER(MarkImageUpdated)

                                     , IMGVER(DumpFontCache)  // DumpFontCache
                                     , IMGVER(RerenderFont)  // RerenderFont

												 , IMGVER(ReloadOpenGlTexture)
												 , IMGVER(ReloadOpenGlShadedTexture)
												 , IMGVER(ReloadOpenGlMultiShadedTexture)
												 , IMGVER(SetImageTransformRelation)
                                     , IMGVER(Render3dImage)
                                     , IMGVER(DumpFontFile)
                                     , IMGVER(Render3dText)
									 , IMGVER(TransferSubImages)
									 , NULL //IMAGE_PROC_PTR( Image, GetNativeImage )( Image pImageTo );
									 , IMGVER(GetTintedImage)
									, IMGVER(GetShadedImage )
									, IMGVER(IsImageTargetFinal)
									, NULL
									, IMGVER(PutStringFontExx)
									 , NULL  // reset image buffers... proxy layer
									 , IMGVER(PngImageFile)
                                     , IMGVER(JpgImageFile)
									 , IMGVER(SetFontBias)
};


IMAGE_3D_INTERFACE Image3dInterface = {
	GetShaderInit,
	  CompileShader,
	  CompileShaderEx,
		EnableShader,
		SetShaderEnable,
		SetShaderModelView,
		BeginShaderOp,
		AppendShaderTristripQuad,
		SetShaderAppendTristrip,
		SetShaderOutput,
		SetShaderResetOp,
		AppendShaderTristrip,
		//BeginShaderOp,
		//ClearShaderOp,
		//AppendShaderOpTristrop
		CreateShaderBuffer,
		AppendShaderBufferData,

		SetShaderDepth,

		GetShaderUniformLocation,
		SetUniform4f,
		SetUniform4fv,
		SetUniform3fv,
		SetUniform1f,
		SetUniformMatrix4fv,
};

#undef GetImageInterface
#undef DropImageInterface
#undef GetImage3dInterface

static PIMAGE_INTERFACE CPROC GetImageInterface ( void )
{
	return (PIMAGE_INTERFACE)&RealImageInterface;
}

static POINTER CPROC GetImage3dInterface( void )
{
	return &Image3dInterface;
}

static void  CPROC DropImage3dInterface ( POINTER p )
{
	;
}


static void  CPROC DropImageInterface ( PIMAGE_INTERFACE p )
{
	;
}

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 45
#endif


PRIORITY_PRELOAD( ImageRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( "puregl2.image", (void*(CPROC*)(void))GetImageInterface, (void(CPROC*)(void*))DropImageInterface );
	RegisterInterface( "puregl2.image.3d", GetImage3dInterface, DropImage3dInterface );

#ifdef __EMSCRIPTEN__
	RegisterClassAlias( "system/interfaces/puregl2.image", "system/interfaces/image" );
	RegisterClassAlias( "system/interfaces/puregl2.image.3d", "system/interfaces/image.3d" );
#endif

	l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), "SACK/Image Library/Scale", 10 );
	if( l.scale == 0.0 )
	{
		l.scale = (RCOORD)SACK_GetProfileInt( GetProgramName(), "SACK/Image Library/Inverse Scale", 2 );
		if( l.scale == 0.0 )
			l.scale = 1;
	}
	else
		l.scale = 1.0f / l.scale;

	// this initializes some of the interface methods
	//SetBlotMethod( BLOT_C );
	RealImageInterface._IsImageTargetFinal = IMGVER(IsImageTargetFinal);
}

int link_interface_please;
IMAGE_NAMESPACE_END


