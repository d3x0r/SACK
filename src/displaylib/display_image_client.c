#include <stdhdrs.h>
#include <sys/types.h>
#include <stdio.h>
#include <sack_types.h>
#include <sharemem.h>

// define this so AColor is a direct define instead of a function.
// this sort of computation should be handled locally and not with RPC
#define IMAGE_LIBRARY_SOURCE_MAIN
#define IMAGE_MAIN
#define DEFINE_IMAGE_PROTOCOL
#define BASE_MESSAGE_ID g.MsgBase
#define MyInterface image_interface_tag
#include <image.h>

#include <msgclient.h>

#define DISPLAY_IMAGE_CLIENT
#define GLOBAL_STRUCTURE_DEFINED
#include "client.h"

IMAGE_NAMESPACE
_32 _NULL = INVALID_INDEX;
#define SafeNULL(pimg) ((pimg)?(void*)(&(pimg)->RealImage):((void*)&_NULL))

static int CPROC ImageEventHandler( _32 EventMsg, _32 *data, _32 length )
{
// this really only has one event - server gone away.  (close all resources)
	//switch( EventMsg )
	//{
	//case MSG_EventImageChange:
   //   data[0]
   //   break;
	//}
   return 0;
}

static int ConnectToServer( void )
{
	if( g.flags.disconnected )
      return FALSE;
   if( !g.flags.connected )
   {
      if( InitMessageService() )
      {
         g.MsgBase = LoadService( WIDE("image"), ImageEventHandler );
         Log1( WIDE("Image message base is %") _32fs, g.MsgBase );
         if( g.MsgBase != INVALID_INDEX )
            g.flags.connected = 1;
      }
   }
   if( !g.flags.connected )
      Log( WIDE("Failed to connect") );
   return g.flags.connected;
}

static void DisconnectFromServer( void )
{
   if( g.flags.connected )
   {
      Log( WIDE("Disconnecting from service (image)") );
      UnloadService( g.MsgBase );
      g.flags.connected = 0;
      g.MsgBase = 0;
   }
}


IMAGE_PROC  void IMAGE_API  SetStringBehavior( Image pImage, _32 behavior )
{
   PMyImage MyImage = (PMyImage)pImage;
   if( !ConnectToServer() ) return;
   TransactServerMessage( MSG_SetStringBehavior, &MyImage->RealImage, 4
                  , NULL, NULL, 0 );
}
IMAGE_PROC  void IMAGE_API  SetBlotMethod     ( _32 method )
{
   if( !ConnectToServer() ) return;
   TransactServerMessage( MSG_SetBlotMethod, &method, 4
                  , NULL, NULL, 0 );
}

IMAGE_PROC  Image IMAGE_API BuildImageFileEx ( PCOLOR pc, _32 width, _32 height DBG_PASS)
{
   _32 ResultID;
   _32 Result[5];
   _32 ResultLen = 20;
	if( !ConnectToServer() ) return NULL;
	if( pc )
		lprintf( WIDE("Display Image Client cannot create images with custom pixel buffers") );
   else
	{
		PMyImage image = (PMyImage)Allocate( sizeof( struct my_image_tag ) );
		if( TransactServerMultiMessage( MSG_BuildImageFileEx, 2
												, &ResultID, Result, &ResultLen
												, &width, ParamLength( width, height )
												, &image, sizeof( image )
												) &&
			( ResultID == (MSG_BuildImageFileEx|SERVER_SUCCESS)))
		{
			//Log2( WIDE("Result image is : %p (into %p)"), (POINTER)Result[4], image );
			image->x = Result[0];
			image->y = Result[1];
			image->width = Result[2];
			image->height = Result[3];
			image->RealImage = *(Image*)(Result+4);
			return (Image)image;
		}
		else
			Release( image );
	}
   return NULL;
}

IMAGE_PROC  Image IMAGE_API MakeImageFileEx  (_32 Width, _32 Height DBG_PASS)
{
   _32 ResultID;
   _32 Result[5];
   _32 ResultLen = 20;
	PMyImage image;
   if( !ConnectToServer() ) return NULL;
	image = (PMyImage)Allocate( sizeof( struct my_image_tag ) );
	if( TransactServerMultiMessage( MSG_MakeImageFileEx, 2
									 , &ResultID, Result, &ResultLen
									 , &Width, ParamLength( Width, Height )
									 , &image, sizeof( image )
									 ) &&
      ( ResultID == (MSG_MakeImageFileEx|SERVER_SUCCESS)))
   {
      //Log2( WIDE("Result image is : %p (into %p)"), (POINTER)Result[4], image );
      image->x = Result[0];
      image->y = Result[1];
      image->width = Result[2];
      image->height = Result[3];
      image->RealImage = *(Image*)(Result+4);
      return (Image)image;
	}
	else
      Release( image );
   return NULL;
}

IMAGE_PROC  Image IMAGE_API MakeSubImageEx   ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS )
{
   PMyImage MyImage = (PMyImage)pImage;
   
   _32 ResultID;
   _32 Result[5];
   _32 ResultLen = 20;
	if( ConnectToServer() )
	{
		PMyImage image = (PMyImage)Allocate( sizeof( struct my_image_tag ) );
		if( TransactServerMultiMessage( MSG_MakeSubImageEx, 3
												, &ResultID, Result, &ResultLen
												, SafeNULL(MyImage), 4
												, &x, 16
												, &image, sizeof( image ) ) &&
			( ResultID == (MSG_MakeSubImageEx|SERVER_SUCCESS) ) )
		{
		//Log3( WIDE("Result image is : %p (into %p) uhh %d"), (POINTER)Result[4], image, sizeof( struct my_image_tag ) );
			image->x = Result[0];
			image->y = Result[1];
			image->width = Result[2];
			image->height = Result[3];
			image->RealImage = *(Image*)(Result+4);
			return (Image)image;
		}
		else
			Release( image );
   }
   return NULL;

}

IMAGE_PROC  Image IMAGE_API RemakeImageEx    ( Image pImage, PCOLOR pc, _32 width, _32 height DBG_PASS)
{
   PMyImage MyImage = (PMyImage)pImage;
   _32 ResultID;
   _32 Result[5];
   _32 ResultLen = 20;
   if( !ConnectToServer() ) return NULL;
   if( TransactServerMultiMessage( MSG_RemakeImageEx, 2
                            , &ResultID, Result, &ResultLen
                            , SafeNULL( MyImage ), 4
                            , &pc, 6 ) &&
      ( ResultID == (MSG_RemakeImageEx|SERVER_SUCCESS ) ) )
   {
      // uhh transmit data!
   }
   return NULL;
}

IMAGE_PROC  Image IMAGE_API  LoadImageFileEx  ( CTEXTSTR filename DBG_PASS )
{
   if( !ConnectToServer() ) return NULL;
   {
   PMyImage image = NULL;
   static int Loading;
   FILE *file = fopen( filename, WIDE("rb") );
   P_8 filebytes;
   _32 filesize;
	Loading = 1;
	if( !file ) // failed to open the file, no reason to tell server anything.
      return NULL;
	lprintf( WIDE("Loading image file: %s"), filename );
   if( file )
   {
      if( fseek( file, 0, SEEK_END ) == 0 )
      {
         filesize = ftell( file );
         if( filesize )
         {
            _32 offset = 0;
            _32 sendlen = 8000;
            _32 Responce, DataState = 0, resultlen = 4;
            _32 ImageResult[5];
            fseek( file, 0, SEEK_SET );
				filebytes = (P_8)AllocateEx( filesize DBG_RELAY );
            if( ( fread( filebytes, 1, filesize, file ) == filesize ) )
            {
               if( filesize <= 8000 )
               {
                  goto send_decode_only;
               }
               if( TransactServerMultiMessage( MSG_BeginTransferData, 3
                                             , &Responce
                                             , &DataState
                                             , &resultlen
                                             , &filesize, sizeof( filesize )
                                             , &sendlen, sizeof( sendlen )
                                             , filebytes, sendlen
                                             ) &&
                  ( Responce == ( MSG_BeginTransferData | SERVER_SUCCESS ) ) )
               {
                  offset = 8000UL;
                  do
                  {
                  send_decode_only:
                     if( ( filesize - offset ) < 8000L )
                        sendlen = filesize - offset;
                     if( ( sendlen + offset ) < filesize )
                     {
                        if( !SendServerMultiMessage( MSG_ContinueTransferData, 3
                                                   , &DataState, sizeof( DataState )
                                                   , &sendlen, sizeof( sendlen )
                                                   , filebytes + offset, sendlen ) )
                        {
                           Log( WIDE("Continue transfer failed.") );
                           break;
                        }
                     }
                     else
                     {
                        //Log2( WIDE("Sending decode on %08lx %ld bytes"), DataState, sendlen );
                        resultlen = sizeof( ImageResult );
								image = (PMyImage)Allocate( sizeof( struct my_image_tag ) );
                        if( TransactServerMultiMessage( MSG_DecodeTransferredImage, 4
                                                      , &Responce, &ImageResult, &resultlen
                                                      , &DataState, sizeof( DataState )
                                                      , &sendlen, sizeof( sendlen )
																		, filebytes + offset, sendlen
																		, &image, sizeof( image ) ) &&
                           ( Responce == (MSG_DecodeTransferredImage|SERVER_SUCCESS) ) )
                        {
                           // okay server Has the file...
                           //Log3( WIDE("Result image is : %p (into %p) uhh %d"), (POINTER)Result[4], image, sizeof( struct my_image_tag ) );
                           image->x = ImageResult[0];
                           image->y = ImageResult[1];
                           image->width = ImageResult[2];
                           image->height = ImageResult[3];
                           image->RealImage = *(Image*)(ImageResult+4);
                        }
                        else
								{
                           Release( image );
                           Log( WIDE("Completion of image data failed... "));
                           break;
                        }
                     }
                     offset += sendlen;
                  } while( offset < filesize );
                  if( offset < filesize )
                  {
                     Log( WIDE("Some failure occured transferring image data to server") );
                  }
               }
               else
                  Log( WIDE("Failed to begin transfer of data") );
            }
            else
               Log( WIDE("Read image data failed...") );
            ReleaseEx( filebytes DBG_RELAY );
         }
      }
      // best place for this - catches all error paths on read
      // as well as common exit....
      fclose( file );
   }
   else
      Log1( WIDE("Failed to open image: %s"), filename );
   Loading = 0;
   return (Image)image;
   }

}

IMAGE_PROC  void IMAGE_API UnmakeImageFileEx ( Image pImage DBG_PASS )
{
   PMyImage MyImage = (PMyImage)pImage;
   if( !ConnectToServer() ) return;
   if( !TransactServerMessage( MSG_UnmakeImageFileEx, SafeNULL( MyImage ), 4
                             , NULL, NULL, 0 ) )
      Log( WIDE("Transact UnmakeImageFile failed!") );
}

IMAGE_PROC  void  IMAGE_API SetImageBound    ( Image pImage, P_IMAGE_RECTANGLE bound )
{
   PMyImage MyImage = (PMyImage)pImage;
   if( !ConnectToServer() ) return;
   // ick - well was going well - until this one points at some other data...
   TransactServerMultiMessage( MSG_SetImageBound, 2
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , bound, sizeof( *bound ) );
}
// reset clip rectangle to the full image (subimage part )
// Some operations (move, resize) will also reset the bound rect,
// this must be re-set afterwards.
// ALSO - one SHOULD be nice and reset the rectangle when done,
// otherwise other people may not have checked this.
IMAGE_PROC  void  IMAGE_API FixImagePosition ( Image pImage )
{
   PMyImage MyImage = (PMyImage)pImage;
   if( !ConnectToServer() ) return;
   TransactServerMessage( MSG_FixImagePosition, SafeNULL( MyImage ), 4
                        , NULL, NULL, 0 );
}

//-----------------------------------------------------

IMAGE_PROC  void IMAGE_API ResizeImageEx     ( Image pImage, S_32 width, S_32 height DBG_PASS)
{
   PMyImage MyImage = (PMyImage)pImage;
   if( !ConnectToServer() ) return;
   {
		pImage = MyImage?MyImage->RealImage:NULL;
      if( TransactServerMultiMessage( MSG_ResizeImageEx, 1
                                    , NULL, NULL, 0
                                    , &pImage, ParamLength( pImage, height ) ) )
      {
         MyImage->width = width;
         MyImage->height = height;
      }
      else
         Log( WIDE("Transact MSG_ResizeImageEx failed!") );
   }
}

IMAGE_PROC  void IMAGE_API MoveImage         ( Image pImage, S_32 x, S_32 y )
{
   PMyImage MyImage = (PMyImage)pImage;
   if( !ConnectToServer() ) return;
   if( TransactServerMultiMessage( MSG_MoveImage, 2
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                                 , &x, 8 ) )
   {
      MyImage->x = x;
      MyImage->y = y;
   }
}

//-----------------------------------------------------

IMAGE_PROC  void IMAGE_API BlatColor     ( Image pImage, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	PMyImage MyImage = (PMyImage)pImage;
   //lprintf( WIDE("Do blat color...") );
   if( !ConnectToServer() ) return;
   if( !TransactServerMultiMessage( MSG_BlatColor, 2
                                 , NULL, NULL, 0
                                 , SafeNULL( MyImage ), 4
                                 , &x, ParamLength( x, color ) ) )
   {
      Log( WIDE("Transact MSG_BlatColor failed.") );
   }

}

IMAGE_PROC  void IMAGE_API BlatColorAlpha( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
   PMyImage MyImage = (PMyImage)pifDest;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_BlatColorAlpha, 2
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, 5 * sizeof( _32 ) );
}


IMAGE_PROC  void IMAGE_API BlotImageEx     ( Image pDest, Image pIF, S_32 x, S_32 y, _32 nTransparent, _32 method, ...  ) // always to screen...
{
   PMyImage MyImage = (PMyImage)pDest;
   PMyImage MySrc = (PMyImage)pIF;
   if( !ConnectToServer() ) return;
   //Log( WIDE("generating blot event to server!") );
   TransactServerMultiMessage( MSG_BlotImageEx, 3
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
									  , SafeNULL(MySrc), 4
                             , &x, ParamLength( x, method ) + 3*sizeof(CDATA) );
}

IMAGE_PROC  void IMAGE_API BlotImageSizedEx( Image pDest, Image pIF, S_32 x, S_32 y, S_32 xs, S_32 ys, _32 wd, _32 ht, _32 nTransparent, _32 method, ...  )
{
   PMyImage MyImage = (PMyImage)pDest;
   PMyImage MySrc = (PMyImage)pIF;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_BlotImageSizedEx, 3
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), sizeof( MyImage->RealImage )
									  , SafeNULL(MySrc), sizeof( MySrc->RealImage )
                             , &x, ParamLength( x, method ) + 3 * sizeof(CDATA) );
}


IMAGE_PROC  void IMAGE_API BlotScaledImageSizedEx( Image pifDest, Image pifSrc
                                        , S_32 xd, S_32 yd
                                        , _32 wd, _32 hd
                                        , S_32 xs, S_32 ys
                                        , _32 ws, _32 hs
                                        , _32 nTransparent
                                        , _32 method, ... )
{
   PMyImage MyImage = (PMyImage)pifDest;
   PMyImage MySrc = (PMyImage)pifSrc;
   if( !ConnectToServer() ) return;
   //Log10( WIDE("BlotScaledImageSized %p to %p  (%d,%d)-(%d,%d) too (%d,%d)-(%d,%d)")
   //         , pifSrc, pifDest, xs, ys, xs+ws, ys+hs, xd, yd, xd+wd, yd+hd );
   TransactServerMultiMessage( MSG_BlotScaledImageSizedEx, 3
                             , NULL, NULL, 0
									  , SafeNULL(MyImage), sizeof( MyImage->RealImage )
									  , SafeNULL(MySrc), sizeof( MySrc->RealImage )
                             , &xd, ParamLength( xd, method ) + 3 * sizeof(CDATA) );
}



//-------------------------------
// Your basic PLOT functions  (Image.C, plotasm.asm)
//-------------------------------
//void,*plotraw)   ( Image pi, S_32 x, S_32 y, CDATA c )
void CPROC myplot(Image image, S_32 x, S_32 y, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
	if( !ConnectToServer() ) return;
	//lprintf( WIDE("Do plot color...") );
   TransactServerMultiMessage( MSG_plot, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 12);
}

void CPROC myplotalpha(Image image, S_32 x, S_32 y, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   //lprintf( WIDE("Do alpha plot color...") );
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_plotalpha, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 12);
}

CDATA CPROC mygetpixel(Image image, S_32 x, S_32 y )
{
   PMyImage MyImage = (PMyImage)image;
   if( !MyImage || !ConnectToServer() ) return 0;
   {
		_32 result, color, len = 4;
		if( TransactServerMultiMessage( MSG_getpixel, 2
												, &result, &color, &len
												, SafeNULL(MyImage), 4
												, &x, 12 ) &&
			( result == (MSG_getpixel|SERVER_SUCCESS ) ) )
		{
			return color;
		}
		return 0;
   }
}

//-------------------------------
// Line functions  (lineasm.asm) // should include a line.c ... for now core was assembly...
//-------------------------------
void CPROC mydo_line(Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_do_line, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 20 );
}

void CPROC mydo_lineAlpha(Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_do_lineAlpha, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 20 );
}

void CPROC mydo_hline(Image image, S_32 x, S_32 xto, S_32 yto, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_do_hline, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 16 );
}

void CPROC mydo_vline(Image image, S_32 x, S_32 xto, S_32 yto, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_do_vline, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 16 );
}

void CPROC mydo_hlineAlpha(Image image, S_32 x, S_32 xto, S_32 yto, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_do_hlineAlpha, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyImage), 4
                             , &x, 16 );
}


void CPROC mydo_vlineAlpha(Image image, S_32 x, S_32 xto, S_32 yto, CDATA c )
{
   PMyImage MyImage = (PMyImage)image;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_do_vlineAlpha, 2
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, 16 );
}

IMAGE_PROC  SFTFont IMAGE_API GetDefaultFont ( void )
{
   if( g.DefaultFont )
      return (SFTFont)g.DefaultFont;
   if( !ConnectToServer() ) return NULL;
   {
   _32 Result;
   _32 ResultBuffer[2048]; // max message length
   _32 ResultLength = 8192;
   if( !g.DefaultFont )
   {
      if( TransactServerMessage( MSG_GetDefaultFont, NULL, 0
                               , &Result, ResultBuffer, &ResultLength )
        && (Result == (SERVER_SUCCESS|MSG_GetDefaultFont) ) )
      {
         g.DefaultFont = (MYFONT*)Allocate( sizeof( MYFONT ) );
         g.DefaultFont->font = *(SFTFont*)(ResultBuffer+0);
         g.DefaultFont->data = (SFTFont)Allocate( ResultLength - 4 );
         MemCpy( g.DefaultFont->data, ResultBuffer + 1, ResultLength - 4 );
      }
   }
   return (SFTFont)g.DefaultFont;
   }
}

IMAGE_PROC  _32  IMAGE_API GetFontHeight  ( SFTFont font )
{
	_32 result[2];
   _32 resultlen = sizeof( result );
   _32 responce;
   if( !ConnectToServer() ) return 0;
	{
		if( TransactServerMultiMessage( MSG_GetFontHeight, 1
												, &responce, result, &resultlen
												, &font, sizeof( SFTFont ) )
			&& ( ( MSG_GetFontHeight | SERVER_SUCCESS ) ) )
		{
         //lprintf( WIDE("Resulting %d"), result[0] );
         return result[0];
		}
		return 0;
	}

   return 0;
}

IMAGE_PROC  _32  IMAGE_API GetStringSizeFontEx( CTEXTSTR pString, size_t len, _32 *width, _32 *height, SFTFont UseFont )
{
	_32 result[2];
   _32 resultlen = sizeof( result );
	_32 responce;
	if( UseFont == (SFTFont)0xDEADBEEF )
	{
      lprintf( WIDE("Application programmer attempting to ... *insert explative* "));
		fprintf( stderr, WIDE("Application programmer attempting to ... *insert explative* "));
      UseFont = NULL;
	}
   if( !ConnectToServer() ) return 0;
	{
		if( TransactServerMultiMessage( MSG_GetStringSizeFontEx, 2
												, &responce, result, &resultlen
												, &UseFont, sizeof( SFTFont )
												, pString, len )
			&& ( ( MSG_GetStringSizeFontEx | SERVER_SUCCESS ) ) )
		{
         //lprintf( WIDE("Measured font at %d,%d"), result[0], result[1] );
			if( width )
				(*width) = result[0];
			if( height )
				(*height) = result[1];
         return result[0];
		}
		return 0;
	}
   return 0;
}


// background of color 0,0,0 is transparent - alpha component does not
// matter....
IMAGE_PROC  void IMAGE_API PutCharacterFont  ( Image pImage
                                   , S_32 x, S_32 y
                                   , CDATA color, CDATA background
                                   , _32 c, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutCharacterFont, 4
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                              ,&c, 4
                             , &font, 4 );
   return;// MyFont->data->char_width[c];
}


IMAGE_PROC  void IMAGE_API PutCharacterVerticalFont      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutCharacterVerticalFont, 4
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                              ,&c, 4
                             , &MyFont->font, 4 );
   return;// MyFont->data->char_width[c];
}

IMAGE_PROC  void IMAGE_API PutCharacterInvertFont        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutCharacterInvertFont, 4
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                              ,&c, 4
                             , &MyFont->font, 4 );
   return;// MyFont->data->char_width[c];
}

IMAGE_PROC  void IMAGE_API PutCharacterVerticalInvertFont( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, _32 c, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutCharacterVerticalInvertFont, 4
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                              ,&c, 4
                             , &MyFont->font, 4 );
   return;// MyFont->data->char_width[c];
}


IMAGE_PROC  void IMAGE_API PutStringFontEx ( Image pImage
                                 , S_32 x, S_32 y
                                 , CDATA color, CDATA background
                                 , CTEXTSTR pc, size_t nLen, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutStringFontEx, 5
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                             , &nLen, 4
                             , &font, 4
                             , pc, nLen );
   // add the sum of the chars... based on the current rules of
   // string processing on the image...
   return;// 0;
}

IMAGE_PROC  void IMAGE_API PutStringVerticalFontEx      ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutStringVerticalFontEx, 5
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                             , &nLen, 4
                             , &font, 4
                             , pc, nLen );
   // add the sum of the chars... based on the current rules of
   // string processing on the image...
   return;// 0;
}

IMAGE_PROC  void IMAGE_API PutStringInvertFontEx        ( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutStringInvertFontEx, 5
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                             , &nLen, 4
                             , &font, 4
                             , pc, nLen );
   // add the sum of the chars... based on the current rules of
   // string processing on the image...
   return;// 0;
}

IMAGE_PROC  void IMAGE_API PutStringInvertVerticalFontEx( Image pImage, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, size_t nLen, SFTFont font )
{
   PMyImage MyImage = (PMyImage)pImage;
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;// 0;
   // setup the default font - but pass the default font through
   if( !MyFont )
   {
      if( !g.DefaultFont )
      {
         if( !GetDefaultFont() )
            return;// 0;
      }
      MyFont = g.DefaultFont;
   }
   TransactServerMultiMessage( MSG_PutStringInvertVerticalFontEx, 5
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4
                             , &x, ParamLength( x, background )
                             , &nLen, 4
                             , &font, 4
                             , pc, nLen );
   // add the sum of the chars... based on the current rules of
   // string processing on the image...
   return;// 0;
}

IMAGE_PROC  _32 IMAGE_API  GetMaxStringLengthFont ( _32 width, SFTFont UseFont )
{
   return width / 8; //junk for now.
   /*
   if( !UseFont )
      UseFont = DEFAULTFONT;
   if( width > 0 )
      // character 0 should contain the average width of a character or is it max?
      return width/UseFont->character[0]->width;
      return 0;
      */
}  


IMAGE_PROC  void IMAGE_API  GetImageSize   ( Image pImage, _32 *width, _32 *height )
{
   if( !ConnectToServer() ) return;
   if( width )
      *width = pImage->width;
   if( height )
      *height = pImage->height;
}

IMAGE_NAMESPACE_END
#include <imglib/fontstruct.h>
IMAGE_NAMESPACE

IMAGE_PROC  SFTFont IMAGE_API  LoadFont  ( SFTFont font )
{
   if( !ConnectToServer() ) return NULL;
   {
		PMYFONT resultfont = NULL;
		PFONT pFont = (PFONT)font;
		_32 Responce, DataState = 0, resultlen;
		P_8 sendbytes;
		_32 sendsize;
		_32 sendlen;
		_32 sendofs;
		_32 inc;

#define FONT_STRUCT 8
#define CHAR_STRUCT 6
   {
      _32 i;
      sendsize = FONT_STRUCT; // height/characters/baseline
      for( i = 0; i < pFont->characters; i++ )
		{
			if( pFont->character[i] )
			{
				if( ( pFont->flags & 3 ) == FONT_FLAG_MONO )
					inc = (pFont->character[i]->size+7)/8;
				else if( ( pFont->flags & 3 ) == FONT_FLAG_2BIT )
					inc = (pFont->character[i]->size+3)/4;
				else if( ( pFont->flags & 3 ) == FONT_FLAG_8BIT )
					inc = (pFont->character[i]->size);
			}
			else
            inc = 0;
         if( pFont->character[i] )
         {
            sendsize += CHAR_STRUCT
               + ( ( ( pFont->character[i]->ascent - pFont->character[i]->descent ) + 1)
                  * ( inc ) );
			}
			else
            sendsize += CHAR_STRUCT;
      }
   }
   sendbytes = (_8*)Allocate( sendsize );
   {
      _32 i;
      _32 offset = 0, charsize;
      CHARACTER zerochar, *current_char;
      MemCpy( sendbytes, pFont, FONT_STRUCT );
      MemSet( &zerochar, 0, sizeof( zerochar ) );
      offset = FONT_STRUCT;
      for( i = 0; i < pFont->characters; i++ )
      {
			if( pFont->character[i] )
			{
				if( ( pFont->flags & 3 ) == FONT_FLAG_MONO )
					inc = (pFont->character[i]->size+7)/8;
				else if( ( pFont->flags & 3 ) == FONT_FLAG_2BIT )
					inc = (pFont->character[i]->size+3)/4;
				else if( ( pFont->flags & 3 ) == FONT_FLAG_8BIT )
					inc = (pFont->character[i]->size);
				current_char = pFont->character[i];
				charsize = CHAR_STRUCT
							+ ( ( ( current_char->ascent - current_char->descent ) + 1 )
							  * ( inc ) );
			}
			else
			{
				current_char = &zerochar;
				charsize = CHAR_STRUCT;
			}
         //lprintf( WIDE("Character %d bytes: %d at %d"), i, charsize, offset );
         MemCpy( sendbytes + offset, current_char, charsize );
         offset += charsize;
      }
   }

   resultlen = 4;
   sendofs = 0;
   sendlen = 8000;
   if( sendsize < 8000 )
      goto send_accept_only;
   if( TransactServerMultiMessage( MSG_BeginTransferData, 3
                                 , &Responce, &DataState, &resultlen
                                 , &sendsize, sizeof( sendsize )
                                 , &sendlen, sizeof( sendlen )
                                 , sendbytes, sendlen ) &&
      ( Responce == ( MSG_BeginTransferData | SERVER_SUCCESS ) ) )
   {
      sendofs = 8000;
      do
      {
      send_accept_only:
         if( ( sendsize - sendofs ) < 8000 )
            sendlen = sendsize - sendofs;
         if( ( sendlen + sendofs ) < sendsize )
         {
            if( !SendServerMultiMessage( MSG_ContinueTransferData, 3
                                       , &DataState, sizeof( DataState )
                                       , &sendlen, sizeof( sendlen )
                                       , sendbytes + sendofs, sendlen ) )
            {
               Log( WIDE("Continue Transfer font failed.") );
               break;
            }
         }
         else
         {
            _32 ResultBuffer[2048];
            resultlen = 8192;
            if( TransactServerMultiMessage( MSG_AcceptTransferredFont, 3
                                          , &Responce, ResultBuffer, &resultlen
                                          , &DataState, sizeof( DataState )
                                          , &sendlen, sizeof( sendlen )
                                          , sendbytes + sendofs, sendlen ) &&
               ( Responce == ( MSG_AcceptTransferredFont | SERVER_SUCCESS ) ) )
            {
               // get font thing here
               resultfont = (MYFONT*)Allocate( sizeof( MYFONT ) );
               resultfont->font = *(SFTFont*)(ResultBuffer+0);
               resultfont->data = (SFTFont)Allocate( resultlen - 4 );
               MemCpy( resultfont->data, ResultBuffer + 1, resultlen - 4 );
            }
            else
            {
               Log( WIDE("Failed font transfer or failed responce...") );
               break;
            }
         }
         sendofs += sendlen;
      } while( sendofs < sendsize );
   }
   Release( sendbytes );
   return (SFTFont)resultfont;
   }
}

IMAGE_PROC  void IMAGE_API  UnloadFont  ( SFTFont font )
{
   PMYFONT MyFont = (PMYFONT)font;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_UnloadFont, 1
                             , NULL, NULL, 0
                             , &MyFont->font, 4 );
   Release( MyFont->data );
   Release( MyFont );

}

#define Avg( c1, c2, d, max ) ((((c1)*(max-(d))) + ((c2)*(d)))/max)

// where d is from 0 to 255 between c1, c2
static CDATA CPROC cColorAverage( CDATA c1, CDATA c2
                          , int d, int max )
{
  CDATA res;
  int r, g, b, a;
  r = Avg( RedVal(c1),   RedVal(c2),   d, max );
  g = Avg( GreenVal(c1), GreenVal(c2), d, max );
  b = Avg( BlueVal(c1),  BlueVal(c2),  d, max );
  a = Avg( AlphaVal(c1), AlphaVal(c2), d, max );
  res = AColor( r, g, b, a );
  return res;
}

// this could be assembly and MMX optimized...

//---------------------------------------------------------------------------

IMAGE_PROC  int IMAGE_API  MergeRectangle ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 )
{
   // results in the union of the area extents...
   // union will always result in a rectangle?...
   int tmp1, tmp2;
   
   tmp1 = r1->width + r1->x;
   tmp2 = r2->width + r2->x;
   if( r1->x < r2->x )
      r->x = r1->x;
   else
      r->x = r2->x;
   if( tmp1 > tmp2 )
      r->width = tmp1 - r->x;
   else
      r->width = tmp2 - r->x; 

   tmp1 = r1->height + r1->y;
   tmp2 = r2->height + r2->y;
   if( r1->y < r2->y )
      r->y = r1->y;
   else
      r->y = r2->y;
   if( tmp1 > tmp2 )
      r->height = tmp1 - r->y;
   else
      r->height = tmp2 - r->y;
   return TRUE;
}


//---------------------------------------------------------------------------

IMAGE_PROC  int IMAGE_API  IntersectRectangle ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 )
{
   int tmp1, tmp2;
   /*
   lprintf( WIDE("Intersecting rectangles.........") );
   lprintf( WIDE("r1: (%d,%d)-(%d,%d)  (%d,%d)-(%d,%d)")
           , r1->x, r1->y, r1->width, r1->height
           , r1->x, r1->y, r1->x + r1->width, r1->y + r1->height );
   lprintf( WIDE("r2: (%d,%d)-(%d,%d)  (%d,%d)-(%d,%d)")
           , r2->x, r2->y, r2->width, r2->height
           , r2->x, r2->y, r2->x + r2->width, r2->y + r2->height );
   */
   if( ( r1->x > (r2->x+r2->width) ) ||
       ( r2->x > (r1->x+r1->width) ) ||
       ( r1->y > (r2->y+r2->height) ) ||
       ( r2->y > (r1->y+r1->height) ) )
      return FALSE;

   if( r1->x > r2->x )
      r->x = r1->x;
   else
      r->x = r2->x;

   tmp1 = r1->width + r1->x;
   tmp2 = r2->width + r2->x;
   if( tmp1 < tmp2 )
      r->width = tmp1 - r->x;
   else
      r->width = tmp2 - r->x;

   if( r1->y > r2->y )
      r->y = r1->y;
   else
      r->y = r2->y;

   tmp1 = r1->height + r1->y;
   tmp2 = r2->height + r2->y;
   if( tmp1 < tmp2 )
      r->height = tmp1 - r->y;
   else
      r->height = tmp2 - r->y;

   if( ( r->width & 0xC0000000 )
     ||( r->height & 0xC0000000 ) )
      return FALSE;
   /*
   lprintf( WIDE("r : (%d,%d)-(%d,%d)  (%d,%d)-(%d,%d)")
           , r->x, r->y, r->width, r->height
           , r->x, r->y, r->x + r->width, r->y + r->height );
   */
   return TRUE;
}



IMAGE_PROC  void IMAGE_API  SyncImage ( void )
{
   _32 Responce;
   if( !ConnectToServer() ) return;
   if( !TransactServerMultiMessage( MSG_SyncImage, 0
                                , &Responce, NULL, NULL ) ||
      ( Responce != ( MSG_SyncImage | SERVER_SUCCESS ) ) )
   {
      Log( WIDE("Failed to sync image operations.") );
   }
}

IMAGE_PROC  void IMAGE_API  AdoptSubImage    ( Image pFoster, Image pOrphan )
{
   PMyImage MyFoster = (PMyImage)pFoster;
   PMyImage MyOrphan = (PMyImage)pOrphan;

   if( !MyFoster || !MyOrphan )
      return;
   if( !ConnectToServer() ) return;
   lprintf( WIDE("Adopting %p(%p) by %p(%p)")
          , pFoster, MyFoster->RealImage
          , pOrphan, MyOrphan->RealImage );
   TransactServerMultiMessage( MSG_AdoptSubImage, 2
                             , NULL, NULL, 0
                             , SafeNULL(MyFoster), 4
                             , SafeNULL(MyOrphan), 4 );
}

IMAGE_PROC  void IMAGE_API  OrphanSubImage   ( Image image )
{
   PMyImage MyImage = (PMyImage)image;
   if( !MyImage )
      return;
   if( !ConnectToServer() ) return;
   TransactServerMultiMessage( MSG_OrphanSubImage, 1
                             , NULL, NULL, 0
                             , SafeNULL( MyImage ), 4 );
}

//---------------------------------------------------------------------------

IMAGE_PROC  void IMAGE_API  SetImageAuxRect ( Image image, P_IMAGE_RECTANGLE pRect )
{
   PMyImage MyImage = (PMyImage)image;
   if( !MyImage )
      return;
   //lprintf( WIDE("Setting aux rect on %p = %d,%d - %d,%d"), pImage, pRect->x, pRect->y, pRect->width, pRect->height );
   if( MyImage && pRect )
		MyImage->auxrect = *pRect;
}

//---------------------------------------------------------------------------

IMAGE_PROC  void IMAGE_API  GetImageAuxRect ( Image image, P_IMAGE_RECTANGLE pRect )
{
   PMyImage MyImage = (PMyImage)image;
   if( !MyImage )
      return;
   if( MyImage && pRect )
      *pRect = MyImage->auxrect;
}

IMAGE_PROC  SFTFont IMAGE_API  InternalRenderFontFile ( CTEXTSTR file
																		, S_32 nWidth
																		, S_32 nHeight
																		, PFRACTION width_scale
																		, PFRACTION height_scale
																		, _32 flags // whether to render 1(0/1), 2(2), 8(3) bits, ...
																		)
{
	SFTFont ResultFont;
	_32 responce;
   _32 ResultSize = sizeof( ResultFont );
   if( !ConnectToServer() ) return NULL;
   if( TransactServerMultiMessage( MSG_InternalRenderFontFile, 2
											, &responce, &ResultFont, &ResultSize
											, (_32*)&nWidth, ParamLength( nWidth, flags )
											, file, strlen( file ) + 1 )
		&& ( responce == (MSG_InternalRenderFontFile | SERVER_SUCCESS) ) )
	{
      return ResultFont;
	}
   return NULL;
}

IMAGE_PROC  SFTFont IMAGE_API  InternalRenderFont ( _32 nFamily
																  , _32 nStyle
																  , _32 nFile
																  , S_32 nWidth
																  , S_32 nHeight
																  , PFRACTION width_scale
																  , PFRACTION height_scale
																  , _32 flags
																  )
{
	PTRSZVAL ResultFont;
   _32 ResultSize = sizeof( ResultFont );
   _32 responce;
   if( !ConnectToServer() ) return NULL;
   if( TransactServerMultiMessage( MSG_InternalRenderFont, 1
											, &responce, &ResultFont, &ResultSize
											, &nFamily, ParamLength( nFamily, flags ) )
		&& ( responce == (MSG_InternalRenderFont | SERVER_SUCCESS) ) )
	{
      return (SFTFont)ResultFont;
	}
   return NULL;
}
IMAGE_PROC  void IMAGE_API  DestroyFont( SFTFont *font )
{
   _32 responce;
   if( !ConnectToServer() ) return;
   if( TransactServerMultiMessage( MSG_DestroyFont, 1
											, &responce, NULL, 0
											, font?NULL:&font, sizeof( SFTFont* ) )
		&& ( responce == (MSG_DestroyFont | SERVER_SUCCESS) ) )
	{
      (*font) = NULL;
	}
}

//#define MAGIC_PICK_FONT 'PICK'
//#define MAGIC_RENDER_FONT 'FONT'

#define NO_FONT_GLOBAL
IMAGE_NAMESPACE_END
#include "../imglib/fntglobal.h"
IMAGE_NAMESPACE
IMAGE_PROC  SFTFont IMAGE_API  RenderScaledFontData ( PFONTDATA pfd, PFRACTION w, PFRACTION h )
{
   // this needs to be somewhat aware of the font data structure...
	PTRSZVAL ResultFont;
   _32 ResultSize = sizeof( ResultFont );
	_32 responce;
	_32 DataSize;
	if( !ConnectToServer() ) return NULL;
	if( (*(_32*)pfd) == MAGIC_PICK_FONT )
      DataSize = sizeof( FONTDATA );
	else if( (*(_32*)pfd) == MAGIC_RENDER_FONT )
		DataSize = sizeof( RENDER_FONTDATA );
	if( TransactServerMultiMessage( MSG_RenderScaledFontData, 1+((w&&h)?2:0)
											, &responce, &ResultFont, &ResultSize
											, pfd, DataSize
											, (w&&h)?w:NULL, sizeof(FRACTION)
											, (w&&h)?h:NULL, sizeof(FRACTION)
											)
		&& ( responce == (MSG_RenderScaledFontData | SERVER_SUCCESS) ) )
	{
      return (SFTFont)ResultFont;
	}
   return NULL;
}

/*
IMAGE_PROC  SFTFont IMAGE_API  RenderFontFileEx ( CTEXTSTR file, _32 width, _32 height, _32 flags, P_32 nFontDataSize, POINTER *pFontData )
{
	struct result_tag {
		RENDER_FONTDATA prd;
		SFTFont font;
	} *result = (struct result_tag*)Allocate( sizeof( struct result_tag ) * 4 );
   _32 ResultSize= sizeof( struct result_tag ) * 4;
	_32 responce;
	if( !ConnectToServer() ) return NULL;
   //lprintf( WIDE("Rendering font file...%d  %s %d,%d"), ResultSize, file, width, height );
   if( TransactServerMultiMessage( MSG_RenderFontFileEx, 2
											, &responce, result, &ResultSize
											, &width, ParamLength( width, flags )
											, file, strlen( file ) + 1 )
		&& ( responce == (MSG_RenderFontFileEx | SERVER_SUCCESS) ) )
	{
		//lprintf( WIDE("Result is %d"), ResultSize );
      //LogBinary( result, ResultSize );
		if( ResultSize > 4 )
		{
			if( pFontData )
				(*pFontData) = &result->prd;
			if( nFontDataSize )
				(*nFontDataSize) = sizeof( RENDER_FONTDATA );
			return *(SFTFont*)(((char*)result) + ResultSize - 4);
		}
	}

   return NULL;
}
*/

int CPROC ClientGetFontRenderData( SFTFont font, POINTER *fontdata, _32 *fontdatalen )
{
   return 0;
}
void CPROC ClientSetFontRendererData( SFTFont font, POINTER pResult, _32 size )
{
}


#if 0
static PTRSZVAL DoNothing( void )
{
   return 0;
}
#endif

static IMAGE_INTERFACE MyImageInterface = {
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

, &plot
, &plotalpha
, &getpixel

, &do_line
, &do_lineAlpha

, &do_hline
, &do_vline
, &do_hlineAlpha
, &do_vlineAlpha
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
, GetMaxStringLengthFont
, GetImageSize
, LoadFont
                                         , UnloadFont
                                         , NULL // transfer isn't used
                                         , NULL // transfer isn't used
                                         , NULL  //decode image isn't used
                                         , NULL // accept font isn't used
                                         , &ColorAverage
                                         , SyncImage
                                         , NULL //GetImageSurface
                                         , IntersectRectangle
                                         , MergeRectangle
														, GetImageAuxRect
														, SetImageAuxRect
														, OrphanSubImage
														, AdoptSubImage
														, NULL // MakeSPriteImage
														, NULL // MakeSPriteImage
														, NULL // rotate_scaled_sprite
														, NULL // rotate_sprite
														, NULL // BlotSprite
                                          , NULL // decode memorytoimage
														, InternalRenderFontFile
														, InternalRenderFont
														, RenderScaledFontData
														, NULL//RenderFontFileEx
														, DestroyFont
                                          , NULL // global font data
														, ClientGetFontRenderData
                                          , ClientSetFontRendererData
};                                        


#undef GetImageInterface
#undef DropImageInterface
static _32 references;

static PIMAGE_INTERFACE CPROC _GetClientDisplayImageInterface( void )
{
   if( ConnectToServer() )
   {
      //atexit( (void(*)(void))DropImageInterface );
      references++;
      return &MyImageInterface;
   }
   return NULL;
}

IMAGE_PROC  PIMAGE_INTERFACE IMAGE_API  GetImageInterface ( void )
{
   return _GetClientDisplayImageInterface();
}

ATEXIT( _DropClientDisplayImageInterface )
{
   Log1( WIDE("Dropping 1 of %") _32fs WIDE(" display connections.."), references );

   references--;
	if( !references )
	{
		DisconnectFromServer();
	   // lock down disconnect so
      // we don't accidentally reconnect.
      g.flags.disconnected = 1;
	}
}
IMAGE_PROC  void IMAGE_API  DropImageInterface ( PIMAGE_INTERFACE p )
{
   _DropClientDisplayImageInterface();
}

PRELOAD( ClientDisplayImageRegisterInterface )
{
	plot		= myplot;
	plotalpha = myplotalpha;
	getpixel		= mygetpixel;
	do_line		= mydo_line; // d is color data...
	do_lineAlpha	= mydo_lineAlpha; // d is color data...
	do_hline	= mydo_hline;
	do_vline	= mydo_vline;
	do_hlineAlpha	= mydo_hlineAlpha;
	do_vlineAlpha	= mydo_vlineAlpha;
	ColorAverage = cColorAverage;
	//lprintf( WIDE("Registering display image interface") );
	RegisterInterface( WIDE("sack.msgsvr.image"), (POINTER(CPROC*)(void))_GetClientDisplayImageInterface, (void(CPROC*)(POINTER))_DropClientDisplayImageInterface );
}

IMAGE_NAMESPACE_END

