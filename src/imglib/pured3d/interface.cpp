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

IMAGE_INTERFACE RealImageInterface = {
  SetStringBehavior
, SetBlotMethod

, BuildImageFileEx               
, MakeImageFileEx                
, MakeSubImageEx                 
, RemakeImageEx                  
, LoadImageFileEx
, UnmakeImageFileEx              

, SetImageBound
, FixImagePosition

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
                                     , MarkImageDirty
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

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 45
#endif

static int CPROC ComparePointer( PTRSZVAL oldnode, PTRSZVAL newnode )
{
	if( newnode > oldnode )
		return 1;
	else if( newnode < oldnode )
		return -1;
	return 0;
}

Image GetShadedImage( Image image, CDATA red, CDATA green, CDATA blue )
{
	POINTER node = FindInBinaryTree( l.shade_cache, (PTRSZVAL)image );
	if( node )
	{
		struct shade_cache_image *ci = (struct shade_cache_image *)node;
		struct shade_cache_element *ce;
		INDEX idx;
		int count = 0;
		struct shade_cache_element *oldest = NULL;

		LIST_FORALL( ci->elements, idx, struct shade_cache_element *, ce )
		{
			if( !oldest )
				oldest = ce;
			else
				if( ce->age < oldest->age )
					oldest = ce;
         count++;
			if( ce->r == red && ce->grn == green && ce->b == blue )
			{
				ce->age = timeGetTime();
				return ce->image;
			}
		}
		if( count > 16 )
		{
			// overwrite the oldest image... usually isn't that many
			ce = oldest;
		}
		else
		{
			ce = New( struct shade_cache_element );
			ce->image = MakeImageFile( image->real_width, image->real_height );
		}
		ce->r = red;
		ce->grn = green;
		ce->b = blue;
		ce->age = timeGetTime();
		BlotImageSizedEx( ce->image, ci->image, 0, 0, 0, 0, image->real_width, image->real_height, 0, BLOT_MULTISHADE, red, green, blue );
		ReloadD3DTexture( ce->image, 0 );
		AddLink( &ci->elements, ce );
		return ce->image;
	}
	else
	{
		struct shade_cache_image *ci = New( struct shade_cache_image );
		struct shade_cache_element *ce = New( struct shade_cache_element );
		ci->image = image;
		ci->elements = NULL;
		AddBinaryNode( l.shade_cache, ci, (PTRSZVAL)image );

		ce->image = MakeImageFile( image->real_width, image->real_height );
		ce->r = red;
		ce->grn = green;
		ce->b = blue;
		ce->age = timeGetTime();
		BlotImageSizedEx( ce->image, ci->image, 0, 0, 0, 0, image->real_width, image->real_height, 0, BLOT_MULTISHADE, red, green, blue );
		ReloadD3DTexture( ce->image, 0 );
		AddLink( &ci->elements, ce );
		return ce->image;
	}
}

PRIORITY_PRELOAD( ImageRegisterInterface, IMAGE_PRELOAD_PRIORITY )
{
	RegisterInterface( WIDE("d3d.image"), _ImageGetImageInterface, _ImageDropImageInterface );
   l.shade_cache = CreateBinaryTreeExtended( 0, ComparePointer, NULL DBG_SRC );
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
   SetBlotMethod( BLOT_C );
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
