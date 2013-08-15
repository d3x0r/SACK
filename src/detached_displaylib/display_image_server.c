#define USE_IMAGE_INTERFACE l.pii
#include <stdhdrs.h>
#include <stddef.h>
#include <sack_types.h>
#include <imglib/fontstruct.h>
#include <sharemem.h>
#include <image.h>
#include <render.h>
#include <msgclient.h>
//#define DISPLAY_PROC(type,name) type name
#define DISPLAY_PROC IMAGE_PROC



typedef struct data_state_tag
{
   _32 length;
   _32 offset;
   P_8 data;
} DATA_STATE, *PDATA_STATE;

typedef struct image_tracking_tag {
   _32 pid;
   Image image;
   IMAGE_RECTANGLE bound; // set from application...
   DeclareLink( struct image_tracking_tag );
} IMAGE_TRACKING, *PIMAGE_TRACKING;

#define MAXIMAGE_TRACKINGSPERSET 512
DeclareSet( IMAGE_TRACKING );

typedef struct font_tracking_tag {
   _32 pid;
	PFONT font;
   POINTER fontdata; // raw data that was sent...
   DeclareLink( struct font_tracking_tag );
} FONT_TRACKING, *PFONT_TRACKING;

#define MAXFONT_TRACKINGSPERSET 128
DeclareSet( FONT_TRACKING );

typedef struct local_tag
{
#ifdef STANDALONE_SERVICE
	struct {
      _32 bFailed;
	} flags;
	_32 MsgBase; //don't really need this no events.
#endif
   PIMAGE_TRACKINGSET pImages;
	PFONT_TRACKINGSET pFonts;
   PIMAGE_INTERFACE pii;
} LOCAL;
static LOCAL l;

RENDER_NAMESPACE

IMAGE_PROC  Image  IMAGE_API  DisplayMakeSubImageEx ( Image pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
IMAGE_PROC  CDATA IMAGE_API  DisplayGetPixel ( Image image, S_32 x, S_32 y );
IMAGE_PROC  void IMAGE_API  DisplayPlot ( Image image, S_32 x, S_32 y, CDATA c );
IMAGE_PROC  void IMAGE_API  DisplayPlotAlpha ( Image image, S_32 x, S_32 y, CDATA c );
IMAGE_PROC  void IMAGE_API  DisplayLine ( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  
#define DisplayLineInverse(pb,x,y,xto,yto,d) DisplayLine( pb,y,x,yto,xto,d)

IMAGE_PROC  void IMAGE_API  DisplayBlatColor ( Image image, S_32 x, S_32 y, _32 w, _32 h, _32 color );
IMAGE_PROC  void IMAGE_API  DisplayBlatColorAlpha ( Image image, S_32 x, S_32 y, _32 w, _32 h, _32 color );

IMAGE_PROC  void IMAGE_API  DisplayLineV ( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color, void(*proc)(Image Image, S_32 x, S_32 y, int d ) );
#define DisplayLineInverseV(pb,x,y,xto,yto,d,proc) DisplayLineV( pb,y,x,yto,xto,d, proc)

IMAGE_PROC  void IMAGE_API  DisplayLineAlpha ( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color );  
IMAGE_PROC  void IMAGE_API  DisplayHLine ( Image image, S_32 y, S_32 xfrom, S_32 xto, CDATA color );  
IMAGE_PROC  void IMAGE_API  DisplayHLineAlpha ( Image image, S_32 y, S_32 xfrom, S_32 xto, CDATA color );  
IMAGE_PROC  void IMAGE_API  DisplayVLine ( Image image, S_32 x, S_32 yfrom, S_32 yto, CDATA color );  
IMAGE_PROC  void IMAGE_API  DisplayVLineAlpha ( Image image, S_32 x, S_32 yfrom, S_32 yto, CDATA color );  
IMAGE_PROC  void IMAGE_API  DisplayBlotScaledImageSizedEx ( Image dest
                  , Image image
                  , S_32 xd, S_32 yd, _32 wd, _32 hd
                  , S_32 xs, S_32 ys, _32 ws, _32 hs
                  , _32 transparency
                  , _32 method
                  , ... );
IMAGE_PROC  void IMAGE_API  DisplayBlotImageSizedEx ( Image dest
                  , Image image
                  , S_32 xd, S_32 yd
                  , S_32 xs, S_32 ys, _32 ws, _32 hs
                  , _32 transparency
                  , _32 method
                  , ... );
IMAGE_PROC  void IMAGE_API  DisplayBlotImageEx ( Image dest
                  , Image image
                  , S_32 xd, S_32 yd
                  , _32 transparency
                  , _32 method
                  , ... );
IMAGE_PROC  void IMAGE_API  DisplayPutCharacterFont         ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
IMAGE_PROC  void IMAGE_API  DisplayPutCharacterVerticalFont ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
IMAGE_PROC  void IMAGE_API  DisplayPutCharacterInvertFont         ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );
IMAGE_PROC  void IMAGE_API  DisplayPutCharacterVerticalInvertFont ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, TEXTCHAR c, SFTFont font );

IMAGE_PROC  void IMAGE_API  DisplayPutStringFontEx          ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, SFTFont font );
IMAGE_PROC  void IMAGE_API  DisplayPutStringVerticalFontEx  ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, SFTFont font );
IMAGE_PROC  void IMAGE_API  DisplayPutStringInvertFontEx          ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, SFTFont font );
IMAGE_PROC  void IMAGE_API  DisplayPutStringInvertVerticalFontEx  ( Image image, S_32 x, S_32 y, CDATA color, CDATA background, CTEXTSTR pc, _32 nLen, SFTFont font );


static PTRSZVAL CPROC CloseImageIfPid( void *member, PTRSZVAL pid )
{
	PIMAGE_TRACKING image = (PIMAGE_TRACKING)member;
	//Log3( WIDE("Checking to see if image %p %ld is owned by %ld")
	//	 , image->hDisplay, image->pid, pid );
	if( !pid || image->pid == pid )
	{
	// get this display out of the list
	// this will stop any events
	// from going backwards... err ...
      image->pid = 0;
		UnmakeImageFile( image->image );
		DeleteFromSet( IMAGE_TRACKING, &l.pImages, image );
	}
   return 0;
}

static PTRSZVAL CPROC CloseFontIfPid( void *member, PTRSZVAL pid )
{
	PFONT_TRACKING font = (PFONT_TRACKING)member;
	//Log3( WIDE("Checking to see if font %p %ld is owned by %ld")
	//	 , font->hDisplay, font->pid, pid );
	if( !pid || font->pid == pid )
	{
	// get this display out of the list
	// this will stop any events
	// from going backwards... err ...
      font->pid = 0;
		Release( font->fontdata );
		Release( font->font );
		DeleteFromSet( IMAGE_TRACKING, &l.pImages, font );
	}
   return 0;
}


static int CPROC ImageClientClosed( _32 *params, _32 param_length
                            , _32 *result, _32 *result_length )
{
   // result and resultlength will be NULL;
   // params[0] == pid of client closed.
   // need to release all resources which client opened.
	ForAllInSet( IMAGE_TRACKING
				  , l.pImages, CloseImageIfPid, params[-1] );
	ForAllInSet( FONT_TRACKING
				  , l.pFonts
				  , CloseFontIfPid
				  , params[-1] );
	*result_length = 0;
   return TRUE;
}

static int CPROC ServerImageUnload( _32 *params, _32 length
                              , _32 *result, _32 *result_length )
{

	ForAllInSet( IMAGE_TRACKING
				  , l.pImages, CloseImageIfPid, 0 );
	ForAllInSet( FONT_TRACKING
				  , l.pFonts
				  , CloseImageIfPid
				  , 0 );
   *result_length = 0;
   return TRUE;
}



_32 CPROC AddTrackedImage( _32 pid, Image image, PTRSZVAL client_image_id )
{
   //lprintf( WIDE("Tracking image %p"), image );
   if( image )
   {
		PIMAGE_TRACKING pit = GetFromSet( IMAGE_TRACKING, &l.pImages );
		_32 idx = GetMemberIndex( IMAGE_TRACKING, &l.pImages, pit );
      //lprintf( WIDE("Image index is %d"), idx );
      pit->pid = pid;
		pit->image = image;
      return idx+1;
	}
   return 0; //INVALID_INDEX;
}

Image GetTrackedImage( _32 image )
{
	PIMAGE_TRACKING pit;
// invalid_index would come from the client bein sucky
// cause we would return 0 for our index (since it's biased to +1 so 0 is not sent...
// often the number is considered a pointer, in which case 0 is the
// invalid, not -1
	if( image == 0xDEADBEEF )
	{
		lprintf( WIDE("Client is requesting a uninitialized image pointer.") );
      return NULL;
	}
	if( image == 0xFACEBEAD )
	{
		lprintf( WIDE("Client is reqeusting a image by a pointer within released memory.") );
      return NULL;
	}
	if( image == INVALID_INDEX || !image )
      return NULL;
   //lprintf( WIDE("Getting real image for handle %d"), image );
	pit = GetUsedSetMember( IMAGE_TRACKING, &l.pImages, image - 1 );
	if( pit )
		return pit->image;
   return NULL;
}

static void CPROC RemoveTrackedImage( _32 image )
{
	PIMAGE_TRACKING pit = GetUsedSetMember( IMAGE_TRACKING, &l.pImages, image - 1 );
	if( pit )
	{
		UnmakeImageFile( pit->image );
		DeleteFromSet( IMAGE_TRACKING, &l.pImages, pit );
	}
}

static _32 CPROC AddTrackedFont( _32 pid, PFONT font, POINTER fontdata )
{
   if( font )
   {
		PFONT_TRACKING pft = GetFromSet( FONT_TRACKING, &l.pFonts );
		_32 idx = GetMemberIndex( FONT_TRACKING, &l.pFonts, pft );
      pft->pid = pid;
		pft->font = font;
		pft->fontdata = fontdata;
      lprintf( WIDE("Added font tracked %d"), idx + 1 );
      return idx + 1;
	}
   return 0;
}

static void CPROC RemoveTrackedFont( _32 font )
{
	PFONT_TRACKING pft = GetSetMember( FONT_TRACKING, &l.pFonts, font - 1 );
	Release( pft->fontdata );
	Release( pft->font );
   DeleteFromSet( FONT_TRACKING, &l.pFonts, pft );
}

static int CPROC ImageSetStringBehavior( _32 *params, _32 param_length
                     , _32 *result, _32 *result_length)
{
   // image library direct
   //SetStringBehavior( params[0] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

static int CPROC ImageSetBlotMethod( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   // image library direct
   Log1( WIDE("Setting blot method: %ld"), params[0] );
   SetBlotMethod( params[0] );
   *result_length = INVALID_INDEX;
   return TRUE;
}

// LoadImageFile ???
// This is done on the client side, and sent over to the server
// all bits of the data are sent to the server...
// quite inefficient, but hey, what do you expect, for a dime?

// alternative was to have the server load the image itself,
// since it already has the linkages to png, jpeg, bmp, gif....

// And - I can't really think of an application (yet) which
// needs its own raw image data.... - so - since we're definatly
// on the same box with the server, perhaps I can forward my current
// path, modified by the filename to the server to have it load
// the image - which is basically why BuildImage exists!

static int CPROC ImageBuildImageFileEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image SimpleImage;
   SimpleImage = BuildImageFile( NULL, params[0], params[1] );
   if( SimpleImage )
   {
      _32 image = AddTrackedImage( params[-1], SimpleImage, params[2] );
      result[0] = SimpleImage->x;
      result[1] = SimpleImage->y;
      result[2] = SimpleImage->width;
		result[3] = SimpleImage->height;
      //lprintf( WIDE("Result iamge is %d"), image );
      result[4] = (_32)image;
      *result_length = 20;
      return TRUE;
   }
   *result_length = 0;
   return FALSE;

   result[0] = 0;
   result[1] = 0;
   result[2] = 0;
   result[3] = 0;
   result[4] = 0;
   // this is a NIL operation - not possible
   // since the real er... yeah I guess it could
   // but the entire data buffer would have to be sent
   // which means fragmented code - check VIEW
   *result_length = 20;
   return TRUE;
}

static int CPROC ImageMakeImageFileEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image SimpleImage;
   SimpleImage = MakeImageFile( params[0], params[1] );
   if( SimpleImage )
   {
      INDEX image = AddTrackedImage( params[-1], SimpleImage, params[2] );
      result[0] = SimpleImage->x;
      result[1] = SimpleImage->y;
      result[2] = SimpleImage->width;
      result[3] = SimpleImage->height;
      //lprintf( WIDE("Result iamge is %d"), image );
      result[4] = (_32)image;
      *result_length = 20;
      return TRUE;
   }
   *result_length = 0;
   return FALSE;
}

static int CPROC ImageMakeSubImageEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		Image SimpleImage;
	// need to call display's make sub image so that
	// it can track the flags it needs
		SimpleImage = DisplayMakeSubImageEx( image
													  , params[1], params[2]
													  , params[3], params[4]
														DBG_SRC );
		if( SimpleImage )
		{
			INDEX image = AddTrackedImage( params[-1], SimpleImage, params[5] );
			result[0] = SimpleImage->x;
			result[1] = SimpleImage->y;
			result[2] = SimpleImage->width;
			result[3] = SimpleImage->height;
         //lprintf( WIDE("Result image is %d"), image );
			result[4] = (_32)image;
			(*result_length) = 20;
			return TRUE;
		}
	}
   *result_length = 0;
   return FALSE;
}
static int CPROC ImageRemakeImageEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   //Image SimpleImage;
   result[0] = 0;
   result[1] = 0;
   result[2] = 0;
   result[3] = 0;
   result[4] = 0;
   // hmm remake? this is an apparition of windows code which
   // resizes the window...
   //result[0] = (_32)SimpleImage;
   //result[1] = SimpleImage->x;
   //result[2] = SimpleImage->y;
   //result[3] = SimpleImage->width;
   //result[4] = SimpleImage->height;
   *result_length = 20;
   return TRUE;
}
static int CPROC ImageUnmakeImageFileEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   // remove image releases the image resource also..
   RemoveTrackedImage( params[0] );
   *result_length = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImageSetImageBound( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   // this should be able to be done image direct
   // applications shouldn't do this anyhow...
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		SetImageBound( image, (P_IMAGE_RECTANGLE)(params + 1) );
	}
	(*result_length) = INVALID_INDEX;
	return TRUE;
}

static int CPROC ImageFixImagePosition( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		FixImagePosition( image );
	}
	(*result_length) = INVALID_INDEX;
	return TRUE;
}

static int CPROC ImageResizeImageEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		//Log3( WIDE("Client request resize of %p %d,%d")
		//	 , params[0], params[1], params[2] );
		ResizeImage( image, params[1], params[2] );
		result[0] = image->x;
		result[1] = image->y;
		result[2] = image->width;
		result[3] = image->height;
	}
	(*result_length) = INVALID_INDEX;
	return TRUE;
}
static int CPROC ImageMoveImage( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		MoveImage( image, params[1], params[2] );
		result[0] = image->x;
		result[1] = image->y;
		result[2] = image->width;
		result[3] = image->height;
	}
	(*result_length) = INVALID_INDEX;
	return TRUE;
}

static int CPROC ImageBlatColor( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayBlatColor( image
							 , params[1], params[2]
							 , params[3], params[4]
							 , params[5] );
	}
	(*result_length) = INVALID_INDEX;
	return TRUE;
}
static int CPROC ImageBlatColorAlpha( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayBlatColorAlpha( image
									, params[1], params[2]
									, params[3], params[4]
									, params[5] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ImageBlotImageEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   //Log2( WIDE("Blotting image %08lx to %08lx"), params[1], params[0] );
   Image image = GetTrackedImage( params[0] );
   Image image2 = GetTrackedImage( params[1] );
	if( image && image2 )
	{
		DisplayBlotImageEx( image, image2
								, params[2], params[3]
								, params[4], params[5]
								, params[6], params[7], params[8] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImageBlotImageSizedEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
   Image image2 = GetTrackedImage( params[1] );
	if( image && image2 )
	{
		DisplayBlotImageSizedEx( image, image2 // pifs
									  , params[2], params[3] // x,y
									  , params[4], params[5] // xs,ys
									  , params[6], params[7] // wd, hd
									  , params[8], params[9]
									  , params[10], params[11],params[12] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ImageBlotScaledImageSizedEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   //Log10( WIDE("BlotScaledImageSized %p to %p  (%d,%d)-(%d,%d) too (%d,%d)-(%d,%d)")
   //   , params[1], params[0]
   //   , params[6], params[7], params[6]+params[8], params[7]+params[9]
   //   , params[2], params[3], params[2]+params[4], params[3]+params[5] );
   Image image = GetTrackedImage( params[0] );
   Image image2 = GetTrackedImage( params[1] );
	if( image && image2 )
	{
		DisplayBlotScaledImageSizedEx( image, image2 // pifs
											  , params[2], params[3] // xd,yd
											  , params[4], params[5] // wd, hd
											  , params[6], params[7] // xs,ys
											  , params[8], params[9] // ws, hds
											  , params[10], params[11] // trans, method
											  , params[12], params[13],params[14] ); //colors
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imageplot( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPlot( image, params[1], params[2], params[3] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imageplotalpha( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPlotAlpha( image, params[1], params[2], params[3] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imagegetpixel( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		result[0] = DisplayGetPixel( image, params[1], params[2] );
	}
   (*result_length) = 4;
   return TRUE;
}

static int CPROC Imagedo_line( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayLine( image
					  , params[1], params[2]
					  , params[3], params[4]
					  , params[5] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imagedo_lineAlpha( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayLineAlpha( image
							 , params[1], params[2]
							 , params[3], params[4]
							 , params[5] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imagedo_hline( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   //Log4( WIDE("Command do_hline on %p at %d %d-%d")
   // , params[0], params[1], params[2], params[3] );
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayHLine( image
						, params[1]
						, params[2], params[3]
						, params[4] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imagedo_vline( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayVLine( image
						, params[1]
						, params[2], params[3]
						, params[4] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC Imagedo_hlineAlpha( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayHLineAlpha( image
							  , params[1]
							  , params[2], params[3]
							  , params[4] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC Imagedo_vlineAlpha( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{

   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayVLineAlpha( image
							  , params[1]
							  , params[2], params[3]
							  , params[4] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ResultFont( PFONT font, _32 *result, _32 *result_length )
{
   TEXTCHAR *width;
   SFTFont outfont = (SFTFont)(result + (sizeof(PTRSZVAL)/sizeof(_32)));
   int n, count;
   ((PTRSZVAL*)result)[0] = (PTRSZVAL)font;
   outfont->height = font->height;
   count = outfont->characters = font->characters;
   width = (TEXTCHAR *)(result + (sizeof(PTRSZVAL)/sizeof(_32))*2);
   for( n = 0; n < count; n++ )
   {
      outfont->character[n]->width = font->character[n]->width;
   }
   (*result_length) = 8 + ((width+n)-(TEXTCHAR *)(result+(sizeof(PTRSZVAL)/sizeof(_32))*2));
   //Log4( WIDE("Result font size is : %ld %08lx %08lx %08lx"), *result_length, result[0], result[1], result[2] );
   return TRUE;
}

static int CPROC ImageGetDefaultFont( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   return ResultFont( (PFONT)GetDefaultFont(), result, result_length );
}

static int CPROC ImageGetFontHeight( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   result[0] = GetFontHeight( (SFTFont)params[0] );
   // handled client local
   (*result_length) = 4;
   return TRUE;
}

static int CPROC ImageGetStringSizeFontEx( _32 *params, _32 param_length
													  , _32 *result, _32 *result_length)
{
   //lprintf( WIDE("String is %s(%d),%p"), params+1, param_length - 4, params[0] );
	GetStringSizeFontEx( (char*)(params + 1), param_length-4, result + 0, result + 1, (SFTFont)params[0] );
   //lprintf( WIDE("results are %d,%d"), result[0], result[1] );
   (*result_length) = 8;
   return TRUE;
}

static int CPROC ImagePutCharacterFont( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutCharacterFont( image
									  , params[1], params[2]
									  , params[3], params[4]
									  , params[5], (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImagePutCharacterVerticalFont( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutCharacterVerticalFont( image
												 , params[1], params[2]
												 , params[3], params[4]
												 , params[5]
												 , (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImagePutCharacterInvertFont( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutCharacterInvertFont( image
											  , params[1], params[2]
											  , params[3], params[4]
											  , params[5], (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImagePutCharacterVerticalInvertFont( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutCharacterVerticalInvertFont( image, params[1], params[2], params[3], params[4], params[5], (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ImagePutStringFontEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutStringFontEx( image
									 , params[1], params[2]
									 , params[3], params[4]
									 , (char*)(params+7), params[5], (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImagePutStringVerticalFontEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutStringVerticalFontEx( image, params[1], params[2], params[3], params[4], (char*)(params+7), params[5], (SFTFont)params[6] );
	}
   *result_length = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImagePutStringInvertFontEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutStringInvertFontEx( image, params[1], params[2], params[3], params[4], (char*)(params+7), params[5], (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}
static int CPROC ImagePutStringInvertVerticalFontEx( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		DisplayPutStringInvertVerticalFontEx( image, params[1], params[2], params[3], params[4], (char*)(params+7), params[5], (SFTFont)params[6] );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ImageGetImageSize( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   // handle client local
   *result_length = INVALID_INDEX;
   return TRUE;
}


static int CPROC ImageBeginTransfer( _32 *params, _32 param_length
                            , _32 *result, _32 *result_length)
//DataState ( _32 total_size, _32 segsize, CDATA data )
{
   PDATA_STATE pds = New( DATA_STATE );
   //Log2( WIDE("Beginning transfer of data %ld total %ld this"), params[0], params[1] );
   pds->data = NewArray( _8, pds->length = params[0] );
   pds->offset = params[1];
   MemCpy( pds->data, params + 2, params[1] );
   ((PTRSZVAL*)result)[0] = (PTRSZVAL)pds;
   (*result_length) = 4;
   return TRUE;
}

static int CPROC ImageContinueTransfer( _32 *params, _32 param_length
                               , _32 *result, _32 *result_length)
//( DataState state, _32 segsize, CDATA data )
{
   PDATA_STATE pds = (PDATA_STATE)params[0];
   //Log3( WIDE("Continue data %ld (%ld of %ld) this")
   //  ,params[1]
   //  , pds->offset
   //  , pds->length );
   MemCpy( pds->data + pds->offset, params + 2, params[1] );
   pds->offset += params[1];
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ImageDecodeTransfer( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
//Image ( DataState state )
{
   PDATA_STATE pds = (PDATA_STATE)params[0];
   Image image;
   if( !pds && (params[1] <= 8000) )
   {
      //Log1( WIDE("Complete data %ld")
      //  , params[1] );
      image = DecodeMemoryToImage( (P_8)(params + 2), params[1] );
   }
   else
   {
      //Log3( WIDE("Complete data %ld (%ld of %ld) this")
      // ,params[1]
      // , pds->offset
      // , pds->length );
      MemCpy( pds->data + pds->offset, params + 2, params[1] );
      pds->offset += params[1];
      if( pds->offset == pds->length )
		{
         image = DecodeMemoryToImage( (P_8)pds->data, pds->length );
      }
      else
      {
         image = NULL;
         //Log2( WIDE("Incomplete image data transfer (%d of %d received)")
         //  , pds->offset, pds->length );
      }
      Release( pds->data );
      Release( pds );
   }

   if( image )
   {
      //Log4( WIDE("Resulting image is (%d,%d) (%d,%d)")
      //  , image->x, image->y, image->width, image->height );
      INDEX image_idx = AddTrackedImage( params[-1], image, params[2] );
      result[0] = image->x;
      result[1] = image->y;
      result[2] = image->width;
      result[3] = image->height;
      result[4] = (_32)image_idx;
      (*result_length) = 5 * sizeof( _32 );
      return TRUE;
   }
   (*result_length) = 0;
   return FALSE;
}

static int CPROC ImageAcceptFont( _32 *params, _32 param_length
                              , _32 *result, _32 *result_length)
{
   PDATA_STATE pds = (PDATA_STATE)params[0];
   P_8 fontdata;
   _32 height;
   _32 characters;
   _32 offset = 0;
   PFONT font;
   int n, inc;
   if( !pds )
   {
      // need to keep this data... should
      // probably make sure that param_length is not 0 (8)
      fontdata = NewArray( _8, param_length - 8 );
      MemCpy( fontdata, params + 2, param_length - 8 );
   }
   else
   {
      MemCpy( pds->data + pds->offset, params + 2, params[1] );
      pds->offset += params[1];
      if( pds->offset != pds->length )
      {
         Log( WIDE("Failed to get all data for transfer") );
         (*result_length) = 0;
         return FALSE;
      }
      fontdata = pds->data;
   }

   height = ((P_16)fontdata)[0];
   characters = ((P_16)fontdata)[2];
	font = (SFTFont)Allocate( 12 + characters * sizeof( PCHARACTER ) );
   MemCpy( font, fontdata, 8 );
   offset = 8;
   for( n = 0; n < characters; n++ )
   {
      font->character[n] = (PCHARACTER)(fontdata + offset);
      /*
      {
         int r, c;
         TEXTSTR data;
         printf( WIDE("------------------------\n") );
         for( r = 0; r <= ( font->character[n]->ascent - font->character[n]->descent ) + 1; r++ )
         {
            data = font->character[n]->data
               + r * ((font->character[n]->size+7)/8);
            for( c = 0; c < font->character[n]->size; c++ )
            {
               if( data[c>>8] & (0x01 << (c & 7)) )
                  printf( WIDE("X") );
               else
                  printf( WIDE("_") );
            }
            printf( WIDE("\n") );
         }

         }
         */
		if( ( font->flags & 3 ) == FONT_FLAG_MONO )
			inc = (font->character[n]->size+7)/8;
		else if( ( font->flags & 3 ) == FONT_FLAG_2BIT )
			inc = (font->character[n]->size+3)/4;
		else if( ( font->flags & 3 ) == FONT_FLAG_8BIT )
			inc = (font->character[n]->size);
		offset += 6
         + ( ( ( font->character[n]->ascent - font->character[n]->descent ) + 1)
            * ( inc ) );
   }
   if( pds )
   {
      // keep the data - it actually contains our characters...
      //Release( pds->data );
      Release( pds );
   }
   AddTrackedFont( params[-1], font, fontdata );
   return ResultFont( font, result, result_length);
}

static int CPROC ImageUnloadFont( _32 *params, _32 param_length
                     , _32 *result, _32 *result_length)
{
   RemoveTrackedFont( params[0] );
   (*result_length) = INVALID_INDEX;
   return FALSE;
}

static int CPROC ImageSyncImage( _32 *params, _32 param_length
                     , _32 *result, _32 *result_length)
{
   // do need to result - caller is waiting for confirmation.
   (*result_length) = 0;
   return TRUE;
}

extern void ClearImagePanelFlag( Image image );
extern void SetImagePanelFlag( Image parent, Image image );


static int CPROC ImageOrphanSubImage( _32 *params, _32 param_length
                     , _32 *result, _32 *result_length)
{
   Image image = GetTrackedImage( params[0] );
	if( image )
	{
		ClearImagePanelFlag( image );
		OrphanSubImage( image );
	}
	(*result_length) = INVALID_INDEX;
	return TRUE;
}

static int CPROC ImageAdoptSubImage( _32 *params, _32 param_length
                     , _32 *result, _32 *result_length)
{
    Image image = GetTrackedImage( params[0] );
    Image image2 = GetTrackedImage( params[1] );
	if( image && image2 )
	{
		SetImagePanelFlag( image, image2 );
		AdoptSubImage( image, image2 );
	}
   (*result_length) = INVALID_INDEX;
   return TRUE;
}

static int CPROC ServerInternalRenderFont( _32 *params, _32 param_length
						  , _32 *result, _32 *result_length)
{
	SFTFont ResultFont;
	ResultFont = InternalRenderFont( params[0], params[1], params[2], params[3], params[4], params[5] );
	((PTRSZVAL*)result)[0] = (PTRSZVAL)ResultFont;
	(*result_length) = sizeof( ResultFont );
   return TRUE;

}

static int CPROC ServerInternalRenderFontFile( _32 *params, _32 param_length
															, _32 *result, _32 *result_length)
{
	SFTFont ResultFont;
	ResultFont = InternalRenderFontFile( (char*)(params + 3), params[0], params[1], params[2] );
	((PTRSZVAL*)result)[0] = (PTRSZVAL)ResultFont;
	(*result_length) = sizeof( ResultFont );
   return TRUE;
}

static int CPROC ServerRenderFontData( _32 *params, _32 param_length
												 , _32 *result, _32 *result_length)
{
	SFTFont ResultFont;
	ResultFont = RenderScaledFontData( (PFONTDATA)params
                                     // if both fractions were passed
												, (PFRACTION)(param_length>4)?(PFRACTION)(params+(1+(0*sizeof(FRACTION)/4))):NULL
												, (PFRACTION)(param_length>4)?(PFRACTION)(params+(1+(1*sizeof(FRACTION)/4))):NULL
												);
	((PTRSZVAL*)result)[0] = (PTRSZVAL)ResultFont;
	(*result_length) = sizeof( ResultFont );
   return TRUE;
}

static int CPROC ServerRenderFontFileEx( _32 *params, _32 param_length
													, _32 *result, _32 *result_length)
{
	SFTFont ResultFont;
	POINTER pResult;
	_32 nResultSize;
	//lprintf( WIDE("Rendering %s %d,%d %d"), params+3, params[0], params[1], params[3] );
	ResultFont = RenderFontFileEx( (char*)(params + 3), params[0], params[1], params[3], &nResultSize, &pResult );
	if( ResultFont )
	{
		//lprintf( WIDE("Resulting data block is %ld"), nResultSize );
            MemCpy( result, pResult, nResultSize );
            DebugBreak(); // this is broken on _64_
		result[(nResultSize+3)/4] = (_32)(PTRSZVAL)ResultFont;
		(*result_length) = (((nResultSize+3)/4)*4) + sizeof( ResultFont );
		//lprintf( WIDE("Resulting font is %lx"), ResultFont );
	}
	else
	{
		result[0] = 0;
		result[1] = 0;
      (*result_length) = 4;
	}
   return TRUE;

}


static int CPROC ServerDestroyFont( _32 *params, _32 param_length
											 , _32 *result, _32 *result_length)
{
	DestroyFont( (SFTFont*)params );
	(*result_length) = INVALID_INDEX;
   return TRUE;
}


static int CPROC DoNothing( _32 *params, _32 param_length
                     , _32 *result, _32 *result_length)
{
   (*result_length) = INVALID_INDEX;
   return FALSE;
}

RENDER_NAMESPACE_END
IMAGE_NAMESPACE

void SetSavePortion( void (CPROC*_SavePortion )( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h ) )
{
	// not sure how to implement this
}
IMAGE_NAMESPACE_END
RENDER_NAMESPACE



#define NUM_FUNCTIONS (sizeof(MyMessageHandlerTable)/sizeof( server_function))
static SERVER_FUNCTION MyMessageHandlerTable[] = {
#ifdef GCC
#ifndef __cplusplus
    [MSG_MateEnded]    =
#endif
#endif
      ServerFunctionEntry( ImageClientClosed ),
   {0},
      // these NULLS would be better skipped with C99 array init - please do look
		// that up and implement here.
	{0},
	{0},
#ifdef GCC
#ifndef __cplusplus
      [MSG_EventUser] =
#endif
#endif
      ServerFunctionEntry( ImageSetStringBehavior ),
     ServerFunctionEntry( ImageSetBlotMethod )
, ServerFunctionEntry( ImageBuildImageFileEx )
, ServerFunctionEntry( ImageMakeImageFileEx )
, ServerFunctionEntry( ImageMakeSubImageEx )
, ServerFunctionEntry( ImageRemakeImageEx )
, ServerFunctionEntry( DoNothing )  // LoadImageFile - aliased to other messages later...
, ServerFunctionEntry( ImageUnmakeImageFileEx )
, ServerFunctionEntry( ImageSetImageBound )
, ServerFunctionEntry( ImageFixImagePosition )

, ServerFunctionEntry( ImageResizeImageEx )
, ServerFunctionEntry( ImageMoveImage )

, ServerFunctionEntry( ImageBlatColor )
, ServerFunctionEntry( ImageBlatColorAlpha )

, ServerFunctionEntry( ImageBlotImageEx )
, ServerFunctionEntry( ImageBlotImageSizedEx )

, ServerFunctionEntry( ImageBlotScaledImageSizedEx )

, ServerFunctionEntry( Imageplot )
, ServerFunctionEntry( Imageplotalpha )
, ServerFunctionEntry( Imagegetpixel )

, ServerFunctionEntry( Imagedo_line )
, ServerFunctionEntry( Imagedo_lineAlpha )

, ServerFunctionEntry( Imagedo_hline )
, ServerFunctionEntry( Imagedo_vline )
, ServerFunctionEntry( Imagedo_hlineAlpha )
, ServerFunctionEntry( Imagedo_vlineAlpha )
, ServerFunctionEntry( ImageGetDefaultFont )
, ServerFunctionEntry( ImageGetFontHeight )
, ServerFunctionEntry( ImageGetStringSizeFontEx )

, ServerFunctionEntry( ImagePutCharacterFont )
, ServerFunctionEntry( ImagePutCharacterVerticalFont )
, ServerFunctionEntry( ImagePutCharacterInvertFont )
, ServerFunctionEntry( ImagePutCharacterVerticalInvertFont )

, ServerFunctionEntry( ImagePutStringFontEx )
, ServerFunctionEntry( ImagePutStringVerticalFontEx )
, ServerFunctionEntry( ImagePutStringInvertFontEx )
, ServerFunctionEntry( ImagePutStringInvertVerticalFontEx )
, ServerFunctionEntry( DoNothing ) // ImageGetMaxStringLengthFont
, ServerFunctionEntry( ImageGetImageSize )
, ServerFunctionEntry( DoNothing ) // LoadFont
, ServerFunctionEntry( ImageUnloadFont )
, ServerFunctionEntry( ImageBeginTransfer )
, ServerFunctionEntry( ImageContinueTransfer )
, ServerFunctionEntry( ImageDecodeTransfer )
, ServerFunctionEntry( ImageAcceptFont )
, ServerFunctionEntry( DoNothing ) // color avg done on client side.
                                                , ServerFunctionEntry( ImageSyncImage )
                                                , ServerFunctionEntry( DoNothing ) // GetImageSurface
                                                , ServerFunctionEntry( DoNothing ) // IntersectRectangle
                                                , ServerFunctionEntry( DoNothing ) // MergeRectangle
                                                , ServerFunctionEntry( DoNothing ) // GetAuxRect
                                                , ServerFunctionEntry( DoNothing ) // SetAuxRect
                                                , ServerFunctionEntry( ImageOrphanSubImage )
																 , ServerFunctionEntry( ImageAdoptSubImage )
																 , ServerFunctionEntry( DoNothing ) // MakeSpriteImageFile
																 , ServerFunctionEntry( DoNothing ) // MakeSpriteImage
																 , ServerFunctionEntry( DoNothing ) // rotate_scaled
																 , ServerFunctionEntry( DoNothing ) // rotate_sprite
																 , ServerFunctionEntry( DoNothing ) // BlotSpite
																 , ServerFunctionEntry( DoNothing ) // DecodeMemoryToImage
																 , ServerFunctionEntry( ServerInternalRenderFontFile )
																 , ServerFunctionEntry( ServerInternalRenderFont )
																 , ServerFunctionEntry( ServerRenderFontData )
																 , ServerFunctionEntry( ServerRenderFontFileEx )
																 , ServerFunctionEntry( ServerDestroyFont )
};

#ifndef STANDALONE_SERVICE
static int CPROC GetImageFunctionTable( server_function_table *table, int *entries )
{
   l.pii = (PIMAGE_INTERFACE)GetInterface( WIDE("real_image") );
   *table = MyMessageHandlerTable;
   *entries = NUM_FUNCTIONS;
   // good as a on_entry procedure.
   return TRUE; // don't need auto init for image lib
}
#endif

PRELOAD( RegisterImageService )
{
//
#ifdef STANDALONE_SERVICE
	//DebugBreak();

	extern LOGICAL InitMemory( void );
	if( InitMemory() )
	{
   SetInterfaceConfigFile( WIDE("DisplayService.conf") );
	l.MsgBase = RegisterService( WIDE("image"), MyMessageHandlerTable, NUM_FUNCTIONS );
	if( !l.MsgBase )
	{
      printf( WIDE("Failed to register image service...\n") );
		l.flags.bFailed = 1;
	}
	else
	{
		l.pii = (PIMAGE_INTERFACE)GetInterface( WIDE("real_image") );
	}
	}
#else
	//RegisterFunction( WIDE("system/interfaces/msg_service"), GetImageFunctionTable
	//					 , WIDE("int"), WIDE("image"), WIDE("(server_function_table*,int*,_32)") );
#endif
}

#ifdef STANDALONE_SERVICE
LOGICAL ImageRegisteredOkay( void )
{
   return !l.flags.bFailed;
}
#endif
RENDER_NAMESPACE_END

