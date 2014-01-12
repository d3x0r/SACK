
#include <stdhdrs.h>

#define USE_RENDER_INTERFACE pActImage
#define USE_IMAGE_INTERFACE pImageInterface

#include <image.h>
#include <render.h>
PRENDER_INTERFACE pActImage;
PIMAGE_INTERFACE pImageInterface;

#include "plugin.h"

#include "renderobj.h"

/*
   Image library plugin
   Need something like a whiteboard to paste pictures on
     /image create <size> 
     /image show <var> <where>
     /image 

  Parameter Interpretation 
     ###x### is size
     ###,### is position

*/

extern INDEX iImage, iDisplay, iRegion;

#define MSG_ID(method)  ( ( offsetof( struct image_interface_tag, _##method ) / sizeof( void(*)(void) ) ) + BASE_IMAGE_MESSAGE_ID + MSG_EventUser )
#define MSG_SetStringBehavior                  MSG_ID( SetStringBehavior )
#define MSG_SetBlotMethod                      MSG_ID( SetBlotMethod )
#define MSG_BuildImageFileEx                   MSG_ID( BuildImageFileEx )
#define MSG_MakeImageFileEx                    MSG_ID( MakeImageFileEx )
#define MSG_MakeSubImageEx                     MSG_ID( MakeSubImageEx )
#define MSG_RemakeImageEx                      MSG_ID( RemakeImageEx )
#define MSG_UnmakeImageFileEx                  MSG_ID( UnmakeImageFileEx )
#define MSG_ResizeImageEx                      MSG_ID( ResizeImageEx )
#define MSG_MoveImage                          MSG_ID( MoveImage )
#define MSG_SetImageBound                      MSG_ID( SetImageBound )
#define MSG_FixImagePosition                   MSG_ID( FixImagePosition )
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
#define MSG_MergeRectangle                 	  MSG_ID( MergeRectangle)
#define MSG_GetImageSurface                    MSG_ID( GetImageSurface )
#define MSG_SetImageAuxRect                    MSG_ID(SetImageAuxRect)
#define MSG_GetImageAuxRect                    MSG_ID(GetImageAuxRect)
#define MSG_OrphanSubImage                     MSG_ID(OrphanSubImage)
#define MSG_AdoptSubImage                      MSG_ID(AdoptSubImage)


//---------------------------------------------------------------------------

void MouseHandler( PTRSZVAL psv, int x, int y, int b )
{
	//PENTITY pe;
		
}

//---------------------------------------------------------------------------

PTEXT ImageGetX( PTRSZVAL psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

PTEXT ImageGetY( PTRSZVAL psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

PTEXT ImageSetX( PTRSZVAL psv, PENTITY pe, PTEXT pData )
{
	return pData;
}

//---------------------------------------------------------------------------

PTEXT ImageSetY( PTRSZVAL psv, PENTITY pe, PTEXT pData )
{
	return pData;
}

//---------------------------------------------------------------------------

PTEXT ImageGetWidth( PTRSZVAL psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}

//---------------------------------------------------------------------------

PTEXT ImageGetHeight( PTRSZVAL psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}


//---------------------------------------------------------------------------

PTEXT ImageGetParent( PTRSZVAL psv, PENTITY pe, PTEXT *pData )
{
	return *pData;
}


//---------------------------------------------------------------------------

#define METHOD(name) int Image##name( PSENTIENT ps, PTEXT parameters ) { return 0; }

METHOD( SetStringBehavior )
METHOD( SetBlotMethod )
METHOD( SetImageBound )
METHOD( FixImagePosition )
METHOD( BlatColor )
METHOD( BlotImage )
METHOD( plot )
METHOD( getpixel )
METHOD( do_line )
METHOD( do_hline )
METHOD( do_vline )

METHOD( PutString )
METHOD( GetMaxStringLengthFont )

METHOD( PickFont )
METHOD( UnloadFont )
METHOD( GetFontHeight )
METHOD( GetStringSizeFontEx )

METHOD( ColorAverage )

METHOD( IntersectRectangle )
METHOD( MergeRectangle )

METHOD( SetImageAuxRect )
METHOD( GetImageAuxRect )

METHOD( SyncImage )
#define NUM_METHODS  ( sizeof( ImageMethods ) / sizeof( ImageMethods[0] ) )
#undef METHOD
#undef METHODX
#define METHOD(method)   { DEFTEXT( #method ),1,0,DEFTEXT( "See SACK/docs/image.html" ), Image##method, NULL }
#define METHODX(method,short)   { DEFTEXT( #short ),1,0,DEFTEXT( "See SACK/docs/image.html" ), Image##method, NULL }
static command_entry ImageMethods[]={ METHOD( SetStringBehavior )
												, METHOD( SetBlotMethod )
												, METHOD( SetImageBound )
												, METHOD( FixImagePosition )
												, METHOD( BlatColor )
												, METHOD( BlotImage )
												, METHOD( plot )
												, METHOD( getpixel )
												, METHODX( do_line, line )
												, METHODX( do_hline, hline )
												, METHODX( do_vline, vline )

												, METHOD( PutString )
												, METHOD( GetMaxStringLengthFont )

												, METHOD( PickFont )
												, METHOD( UnloadFont )
												, METHOD( GetFontHeight )
												, METHOD( GetStringSizeFontEx )

												, METHOD( ColorAverage )

												, METHOD( IntersectRectangle )
												, METHOD( MergeRectangle )

												, METHOD( SetImageAuxRect )
												, METHOD( GetImageAuxRect )

												, METHOD( SyncImage )

												// these are originally based off of macros, and as such should of course
												// be reflected as direct methods.
};

//---------------------------------------------------------------------------

#define NUM_IMAGE_VARS ( sizeof( ImageVars ) / sizeof( ImageVars[0] ) )
volatile_variable_entry ImageVars[] = 
   { { DEFTEXT( "x" ), ImageGetX, ImageSetX }
   , { DEFTEXT( "y" ), ImageGetY, ImageSetY }
   , { DEFTEXT( "width" ), ImageGetWidth, NULL }
	, { DEFTEXT( "height" ), ImageGetHeight, NULL }
	//, { DEFTEXT( "parent" ), ImageGetParent, NULL }
   };


//---------------------------------------------------------------------------

int InitImage( PSENTIENT ps, PENTITY pe, PTEXT parameters )
{
	INDEX idx;
	for( idx = 0; idx < NUM_IMAGE_VARS; idx++ )
	{
      PTEXT saveparms = parameters;
		PTEXT arg;
		Image parent = GetLink( &FindContainer( pe )->pPlugin, iImage );
		arg = GetParam( ps, &parameters );
		if( IsNumber( arg ) )
		{
			PTEXT arg2 = GetParam( ps, &parameters );
			if( IsNumber( arg2 ) )
			{
            Image image;
				if( parent )
					image = MakeSubImage( parent
											  , 0, 0 // need other 2 arguments.
											  , atoi( GetText( arg ) )
											  , atoi( GetText( arg2 ) )
											  );
            else
					image = MakeImageFile( atoi( GetText( arg ) )
													, atoi( GetText( arg2 ) ) );
				SetLink( &pe->pPlugin, iImage, image );
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
				else
				{
					DECLTEXT( msg, "Invalid second parameter, expecting a number" );
               EnqueLink( &ps->Command->Output, &msg );
				}
				// if more args - warn?
			}
		}
		else
		{
         Image image;
			// expect that the remainder is a name..
			parameters = saveparms;
			arg = GetFileName( ps, &parameters );
         // loaded image file doesn't care for parent status...
			image = LoadImageFile( GetText( arg ) );
			if( image )
			{
				SetLink( &pe->pPlugin, iImage, image );
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
				else
				{
					DECLTEXT( msg, "Failed to load image..." );
               EnqueLink( &ps->Command->Output, &msg );
				}
			}
		}
		AddVolatileVariable( pe, ImageVars + idx, 0 );
		{
			int n;
			for( n = 0; n < NUM_METHODS; n++ )
            AddMethod( pe, ImageMethods + n );
		}

	}
	return 0; // return success
}

//---------------------------------------------------------------------------
// $Log: image.c,v $
// Revision 1.11  2004/06/12 08:43:16  d3x0r
// Overall fixes since some uninitialized values demonstrated unhandled error paths.
//
// Revision 1.10  2004/04/02 16:37:58  d3x0r
// Quick trimming of image interface... in script it's going to be easier - other than passing a font
//
// Revision 1.9  2004/04/01 08:59:32  d3x0r
// Updates to implement interface to image and render libraries
//
// Revision 1.8  2003/07/28 09:07:38  panther
// Fix makefiles, fix cprocs on netlib interfaces... fix a couple badly formed functions
//
// Revision 1.7  2003/04/07 20:45:41  panther
// Compatibility fixes.  Cleaned makefiles.
//
// Revision 1.6  2003/02/24 08:46:30  panther
// Updates for new render/image interface changes
//
// Revision 1.5  2003/01/13 00:37:50  panther
// Added visual studio projects, removed old msvc projects.
// Minor macro mods
// Mods to compile cleanly under visual studio.
//
// Revision 1.4  2002/09/16 01:13:56  panther
// Removed the ppInput/ppOutput things - handle device closes on
// plugin unload.  Attempting to make sure that we can restore to a
// clean state on exit.... this lets us track lost memory...
//
// Revision 1.3  2002/08/07 22:48:02  panther
// ...
//
// Revision 1.2  2002/08/04 00:51:19  panther
// Begin implementing actual image functions.
//

