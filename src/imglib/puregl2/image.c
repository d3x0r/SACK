#define IMAGE_LIBRARY_SOURCE_MAIN
#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define IMAGE_MAIN
#ifdef _MSC_VER
#ifndef UNDER_CE
#include <emmintrin.h>
#endif
// intrinsics
#endif

#include <stdhdrs.h>
#ifdef __ANDROID__
#include <gles/gl.h>
#else
#include <GL/glew.h>
#include <GL/gl.h>         // Header File For The OpenGL32 Library
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
//#define NO_LOGGING
#include <logging.h>
#include <sharemem.h>
#include <filesys.h>

#include <imglib/imagestruct.h>
#include <image.h>
#include <render3d.h>

#ifdef _OPENGL_DRIVER
int bGLColorMode = 1; // this gets set if we're working with BGR native or RGB native... (low byte is BLUE naturally)
#else
int bGLColorMode = 0; // this gets set if we're working with BGR native or RGB native... (low byte is BLUE naturally)
#endif

#define DO_GIF
#define DO_JPG
#define DO_PNG
#define DO_BMP

#ifdef DO_PNG
#include "pngimage.h"
#endif

#ifdef DO_GIF
#include "gifimage.h"
#endif

#ifdef DO_TGA
#include "tgaimage.h"
#endif

#ifdef DO_JPG
#include "jpgimage.h"
#endif

#ifdef DO_BMP
#include "bmpimage.h"
#endif
#ifdef __cplusplus
using namespace sack::image::loader;
#endif
#include "local.h"
#include "shaders.h"
#include "blotproto.h"


//#ifndef __arm__

ASM_IMAGE_NAMESPACE
extern unsigned char AlphaTable[256][256];
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE
//---------------------------------------------------------------------------
// a(alpha) parameter value 0 : in is clear, over opaque
//                         255: in is solid, over is clear

_32 DOALPHA( _32 over, _32 in, _8 a )
{
#if defined( _MSC_VER ) && 0 
	int out;
	/* blah, this code still generates a suplus of actual code...
	 * generating data on the stack...
	 */
	register __m64 zero;
	//static __declspec(align(8)) __m64 one;
	 register __m64 m_alpha;
	 register __m64 m_alpha2;
	 register __m64 m_tmp;
	 register __m64 m_color1;
	 register __m64 m_color2;
	_32 alpha;
	//alpha = 0x01010101 * (a);
	zero = _mm_setzero_si64();
	//one = _mm_set1_pi16( 1 );
	//m_tmp    = _m_from_int( alpha );
	m_alpha  = _mm_set1_pi16( a+1 );//_mm_unpacklo_pi8( m_tmp, zero );
	m_alpha2 = _mm_set1_pi16( 0x100-a );

	m_tmp = _m_from_int( over );
	m_color1 = _mm_unpacklo_pi8( m_tmp, zero );
	m_tmp = _m_from_int( in );
	m_color2 = _mm_unpacklo_pi8( m_tmp, zero );
	m_tmp = _mm_mullo_pi16( m_color1, m_alpha );

	m_color1 = _mm_mullo_pi16( m_color2, m_alpha2 );
	m_color2 = _mm_add_pi16( m_tmp, m_color2 );
	//m_tmp = _mm_srli_pi16( m_color2, 8 );
	m_color1 = _m_packuswb( m_color2, m_tmp );
	return _m_to_int( m_color1 );
#else
   int r, g, b, aout;
   if( !a )
      return over;
   //if( a > 255 )
   //   a = 255;
   if( a == 255 )
      return (in | 0xFF000000); // force alpha full on.
   aout = AlphaTable[a][AlphaVal( over )] << 24;
   r = ((((RedVal(in))  *(a+1)) + ((RedVal(over))  *(256-(a)))) >> 8 );
   if( r > (255) ) r = (255);
   g = (((GreenVal(in))*(a+1)) + ((GreenVal(over))*(256-(a)))) >> 8;
   if( g > (255) ) g = (255);
   b = ((((BlueVal(in)) *(a+1)) + ((BlueVal(over)) *(256-(a)))) >> 8 );
   if( b > 255 ) b = 255;
   return aout|(r<<16)|(g<<8)|b;
   //return AColor( r, g, b, aout );
#endif
}
//#endif

static void OnFirstDraw3d( WIDE( "@00 PUREGL Image Library" ) )( PTRSZVAL psv )
{
	l.glActiveSurface = (struct glSurfaceData *)psv;

#ifndef __ANDROID__
	if (GLEW_OK != glewInit() )
	{
		return;
	}
#endif
	InitShader();

	InitSuperSimpleShader( GetShader( "Simple Shader" ) );
	InitSimpleTextureShader( GetShader( "Simmple Texture" ) );
	InitSimpleShadedTextureShader( GetShader( "Simple Shaded Texture" ) );

}

static PTRSZVAL OnInit3d( WIDE( "@00 PUREGL Image Library" ) )( PTRANSFORM camera, RCOORD *pIdentity_depty, RCOORD *aspect )
{
	struct glSurfaceData *glSurface = New( struct glSurfaceData );
	MemSet( glSurface, 0, sizeof( *glSurface ) );
	glSurface->T_Camera = camera;
	glSurface->identity_depth = pIdentity_depty;
	glSurface->aspect = aspect;

	l.glActiveSurface = glSurface;

	AddLink( &l.glSurface, glSurface );
	{
		INDEX idx;
		struct glSurfaceData *data;
		LIST_FORALL( l.glSurface, idx, struct glSurfaceData *, data )
			if( data == glSurface )
			{
				glSurface->index = idx;
				break;
			}
	}
	return (PTRSZVAL)glSurface;
}

static void OnBeginDraw3d( WIDE( "@00 PUREGL Image Library" ) )( PTRSZVAL psvInit, PTRANSFORM camera )
{
	l.glActiveSurface = (struct glSurfaceData *)psvInit;
	l.glImageIndex = l.glActiveSurface->index;
	l.camera = camera;
	l.flags.projection_read = 0;
	l.flags.worldview_read = 0;
	ClearShaders();
}


int ReloadOpenGlTexture( Image child_image, int option )
{
	Image image;
	if( !child_image) 
		return 0;
	for( image = child_image; image && image->pParent; image = image->pParent );

	{
		struct glSurfaceImageData *image_data = 
			(struct glSurfaceImageData *)GetLink( &image->glSurface
			                                    , l.glImageIndex );
		//GLuint glIndex;

		if( !image_data )
		{
			// just call this to create the data then
			image_data = MarkImageUpdated( image );
		}
		// might have no displays open, so no GL contexts...
		if( image_data )
		{
			//lprintf( WIDE( "Reload %p %d" ), image, option );
			// should be checked outside.
			if( image_data->glIndex == 0 )
			{
				glGenTextures(1, &image_data->glIndex);			// Create One Texture
				if( glGetError() || !image_data->glIndex)
				{
					lprintf( WIDE( "gen text %d or bad surafce" ), glGetError() );
					return 0;
				}
			}
			if( image_data->flags.updated )
			{
				if( option & 2 )
				{

					//glPixelTransferf(GL_RED_SCALE,0.5f);                // Scale RGB By 50%, So That We Have Only
					//glPixelTransferf(GL_GREEN_SCALE,0.5f);              // Half Intenstity
					//glPixelTransferf(GL_BLUE_SCALE,0.5f);
#ifndef __ANDROID__
					glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);  // No Wrapping, Please!
					glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
#endif
					//glGenTextures(3, bump); 
				}
				//lprintf( WIDE( "gen text %d" ), glGetError() );
				// Create Linear Filtered Texture
				image_data->flags.updated = 0;
				glBindTexture(GL_TEXTURE_2D, image_data->glIndex);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				/**///glColor4ub( 255, 255, 255, 255 );
#ifdef __ANDROID__
				glTexImage2D(GL_TEXTURE_2D, 0, 4, image->real_width, image->real_height
								, 0, GL_RGBA, GL_UNSIGNED_BYTE
								, image->image );
#else
				glTexImage2D(GL_TEXTURE_2D, 0, 4, image->real_width, image->real_height
								, 0, option?GL_BGRA_EXT:GL_RGBA, GL_UNSIGNED_BYTE
								, image->image );
#endif
				if( glGetError() )
				{
					lprintf( WIDE( "gen text error %d" ), glGetError() );
				}
			}
			image->glActiveSurface = image_data->glIndex;
			child_image->glActiveSurface = image->glActiveSurface;
		}
	}
	return image->glActiveSurface;
}

int ReloadOpenGlShadedTexture( Image child_image, int option, CDATA color )
{
   return ReloadOpenGlTexture( child_image, option );
}

int ReloadOpenGlMultiShadedTexture( Image child_image, int option, CDATA r, CDATA g, CDATA b )
{
#if !defined( __ANDROID__ )
				InitShader();
				if( glUseProgram && l.glActiveSurface->shader.multi_shader )
				{
					int err;
		 			glEnable(GL_FRAGMENT_PROGRAM_ARB);
					glUseProgram( l.glActiveSurface->shader.multi_shader );
					err = glGetError();
					glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, (float)GetRedValue( r )/255.0f, (float)GetGreenValue( r )/255.0f, (float)GetBlueValue( r )/255.0f, (float)GetAlphaValue( r )/255.0f );
					err = glGetError();
					glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 1, (float)GetRedValue( g )/255.0f, (float)GetGreenValue( g )/255.0f, (float)GetBlueValue( g )/255.0f, (float)GetAlphaValue( g )/255.0f );
					err = glGetError();
					glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 2, (float)GetRedValue( b )/255.0f, (float)GetGreenValue( b )/255.0f, (float)GetBlueValue( b )/255.0f, (float)GetAlphaValue( b )/255.0f );					
					err = glGetError();
					return ReloadOpenGlTexture( child_image, option );
				}
				else
#endif
				{
					Image output_image;
					output_image = GetShadedImage( child_image, r, g, b );
					//lprintf( "Output is %p", output_image );
					/**///glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					;/**///glColor4ub( 255,255,255,255 );
					return output_image->glActiveSurface;//ReloadOpenGlTexture( child_image, option );
				}

}
//------------------------------------------

#define Avg( c1, c2, d, max ) ((((c1)*(max-(d))) + ((c2)*(d)))/max)
#ifdef __cplusplus
extern "C" {
#endif

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
#ifdef __cplusplus
} //extern "C" {
#endif

//----------------------------------------------------------------------

 void  FixImagePosition ( ImageFile *pImage )
{
	if( pImage )
	{
		if( pImage->image )
			pImage->flags &= ~IF_FLAG_HIDDEN;

		pImage->x = pImage->eff_x;
		pImage->y = pImage->eff_y;

		// reset state - maximal position - which was also validated.
		pImage->width = pImage->eff_maxx - pImage->eff_x + 1;
		pImage->height = pImage->eff_maxy - pImage->eff_y + 1;
	}
}

//----------------------------------------------------------------------

 void  SetImageBound ( Image pImage, P_IMAGE_RECTANGLE bound )
{
   if( !bound )
      return;
   //bound->x += pImage->real_x;
   //bound->y += pImage->real_y;
   // set the clipping x to at least what's really really clipped...
   if( bound->x < pImage->eff_x )
   {
      bound->width += bound->x - pImage->eff_x;
      bound->x = pImage->eff_x;
   }
   if( bound->y < pImage->eff_y )
   {
      bound->height += bound->y - pImage->eff_y;
      bound->y = pImage->eff_x;
   }

   if( USS_GT( ( bound->width + bound->x - 1 ),IMAGE_SIZE_COORDINATE,pImage->eff_maxx,int) )
      bound->width = ( pImage->eff_maxx - bound->x ) + 1;

   if( USS_GT( ( bound->height + bound->y - 1 ),IMAGE_SIZE_COORDINATE,pImage->eff_maxy,int) )
      bound->height = ( pImage->eff_maxy - bound->y ) + 1;

   //Log4( WIDE("Setting image bound to (%d,%d)-(%d,%d)"),
    //    bound->x, bound->y, bound->width, bound->height );
   // what if it is hidden?
   if( ( pImage->image ) &&
      ( bound->width > 0 ) &&
      ( bound->height > 0 ) )
      pImage->flags &= ~IF_FLAG_HIDDEN;
   else
      pImage->flags |= IF_FLAG_HIDDEN;
   pImage->x = bound->x;
   pImage->y = bound->y;
   pImage->width = bound->width;
   pImage->height = bound->height;
}

//----------------------------------------------------------------------

static void ComputeImageData( ImageFile *pImage )
{
	ImageFile *pParent;
	int x, y;

	x = pImage->real_x;
	y = pImage->real_y;
	// effecitve maxx has to fit within the parent.
	// parent already has a maxx defined - and this maxx
	// is within our image - so update with
	// real_x + maxx to compare vs paretn
	pImage->eff_maxx = ( pImage->real_width - 1 );
	pImage->eff_maxy = ( pImage->real_height - 1 );
	pImage->eff_x = 0;
	pImage->eff_y = 0;

	if( pImage->flags & IF_FLAG_OWN_DATA )
      return;

   // check for parent to clip to...
   if( ( pParent = pImage->pParent ) )
   {
      //Log( WIDE("Subimage has a parent...") );
      if( x <= pParent->eff_x )
      {
         pImage->eff_x = pParent->eff_x - x;
         x = 0;
      }
      else
         x -= pParent->eff_x;
      if( y <= pParent->eff_y )
      {
         // first boundable y is here...
         pImage->eff_y = pParent->eff_y - y;
         y = 0;
      }
      else
         y -= pParent->eff_y;
      if( ( pImage->real_x + pImage->eff_maxx ) >= pParent->eff_maxx )
         pImage->eff_maxx = pParent->eff_maxx - pImage->real_x;

      if( ( pImage->real_y + pImage->eff_maxy ) >= pParent->eff_maxy )
         pImage->eff_maxy = pParent->eff_maxy - pImage->real_y;

      if( ( x > pParent->eff_maxx )
         ||( y > pParent->eff_maxy )
         ||( pImage->eff_maxx < 0 ) // can be 0 (if width = 1)
         ||( pImage->eff_maxy < 0 ) // can be 0 ( if height = 1)
         ||( pParent->flags & IF_FLAG_HIDDEN )
         ||( !pParent->image ))
      {
         pImage->flags |= IF_FLAG_HIDDEN;
         pImage->image = NULL;
      }
      else
      {
#ifdef _INVERT_IMAGE
  // so to find the very last line...
         // first last line possible is the parent's base pointer.
         // actual lastline will be hmm
         // real_height - (my_height + my_real_y)
         // so that as my height decreases ....
         //    real_height - less total si a bigger number which offsets it
         //   if realheight is my_



         // so - under inverted image condition (windows)
         // the data at -> image is the LAST line of the window
         // and goes UP from there...
         // so the offset to apply is the different of parent's height
         // minus y + height ( total offset to bottom of subimage)
         y = (pParent->eff_maxy - (pImage->eff_maxy + pImage->real_y) );
         if( y < 0 )
            DebugBreak();
#endif
         pImage->image = pParent->image + x
            + ( y * (pImage->pwidth = pParent->pwidth ));
      }
   }
   else
   {
      // This computes sub image data only
      // so if there is no parent, then there
      // is no surface, and it is hidden.
      //Log( WIDE("Subimage had no parent - hiding image") );
      pImage->image = NULL;
      pImage->flags |= IF_FLAG_HIDDEN;
      pImage->pwidth = 0;
   }
   FixImagePosition( pImage );
   {
      ImageFile *pSub;
      pSub = pImage->pChild;
      while( pSub )
      {
         ComputeImageData( pSub );
         pSub = pSub->pElder;
      }
   }
   //Log5( WIDE("Resulting real image: (%d,%d)-(%d,%d) %p")
   //  , pImage->real_x , pImage->real_y
    //   , pImage->real_x + pImage->real_width
   //  , pImage->real_y + pImage->real_height
   //    , pImage->image);
   //if( pParent )
   //{
   // Log4( WIDE("   Within image: (%d,%d)-(%d,%d)")
   ///       , pParent->real_x , pParent->real_y
   //     , pParent->real_x + pParent->real_width
   //     , pParent->real_y + pParent->real_height
   //     );
   //}
}

//----------------------------------------------------------------------

 void  MoveImage ( ImageFile *pImage, S_32 x, S_32 y )
{
	if( !pImage->pParent
		&& !( pImage->flags & IF_FLAG_OWN_DATA ) ) // cannot move master iamge... only sub images..
      return;
   pImage->real_x = x;
   pImage->real_y = y;
   //Log( WIDE("Result image data from move image") );
   ComputeImageData( pImage );
}

//----------------------------------------------------------------------

 void  GetImageSize ( ImageFile *image, _32 *width, _32 *height )
{
    if( image )
    {
        if( width )
            *width = image->real_width;
        if( height )
            *height = image->real_height;
    }
}
//----------------------------------------------------------------------

 PCDATA  GetImageSurface        ( Image pImage )
{
   if( pImage )
      return (PCDATA)pImage->image;
   return NULL;
}

//----------------------------------------------------------------------

 void  ResizeImageEx ( ImageFile *pImage, S_32 width, S_32 height DBG_PASS )
{
   if( !pImage )
      return;
   if( !pImage->pParent
		|| ( pImage->flags & IF_FLAG_OWN_DATA ) ) // reallocate the image buffer
	{
      //lprintf( WIDE("BLAH") );
	   if( pImage->real_height != height || pImage->real_width != width )
   	{
	      if( !( pImage->flags & IF_FLAG_EXTERN_COLORS ) )
   	      Release( pImage->image );
      	pImage->height = pImage->real_height =  height;
	      pImage->width = pImage->real_width =
   	      pImage->pwidth = width;
			pImage->image = (PCOLOR)AllocateEx( sizeof( COLOR ) * width * height DBG_RELAY);
			pImage->eff_maxx = ( pImage->real_width - 1 );
			pImage->eff_maxy = ( pImage->real_height - 1 );
			pImage->eff_x = 0;
			pImage->eff_y = 0;

	      pImage->flags &= ~IF_FLAG_EXTERN_COLORS;
			pImage->height = pImage->real_height =  height;
			pImage->width = pImage->real_width =
				pImage->pwidth = width;
			pImage->image = (PCOLOR)AllocateEx( sizeof( COLOR ) * width * height DBG_RELAY);
			pImage->eff_maxx =  pImage->x + ( pImage->real_width - 1 );
			pImage->eff_maxy =  pImage->y + ( pImage->real_height - 1 );
			pImage->eff_x = pImage->x;
			pImage->eff_y = pImage->y;
			//Log( WIDE("Compute from resize image ( no parent )") );
			// cannot compute this - because computeimage data would
			// hide an image without a parent.

			// if it has children...
			//ComputeImageData( pImage );
      }
   }
   else
   {
      // if the width or ehight are 0 or below then the
      // original real_width and real_height are correct....
      if( width > 0 )
         pImage->real_width = width;
      if( height > 0 )
         pImage->real_height = height;
      pImage->pwidth = pImage->pParent->pwidth;
      //Log2( DBG_FILELINEFMT "Compute from resize image ( with parent ) (%d,%d)" DBG_RELAY, width, height );
      ComputeImageData( pImage );
   }
   return;
}

//----------------------------------------------------------------------

 void  OrphanSubImage ( Image pImage )
{
   // if it owns its own data, it's not a child (it might have a parent)
   if( !pImage || !pImage->pParent
		|| ( pImage->flags & IF_FLAG_OWN_DATA ) )
      return;
   if( pImage->pYounger )
      pImage->pYounger->pElder = pImage->pElder;
   else
      pImage->pParent->pChild = pImage->pElder;

   if( pImage->pElder )
      pImage->pElder->pYounger = pImage->pYounger;

   pImage->pParent = NULL;
   ComputeImageData( pImage );
}

//----------------------------------------------------------------------

static void SmearFlag( Image image, int flag )
{
	for( ; image; image = image->pElder )
	{
		image->flags &= ~flag;
		image->flags |= ( image->pParent->flags & flag );
      SmearFlag( image->pChild, flag );
	}
}

//----------------------------------------------------------------------

 void  AdoptSubImage ( Image pFoster, Image pOrphan )
{
   if( !pFoster || !pOrphan || pOrphan->pParent )
   {
      Log( WIDE("Cannot adopt an orphan that has parents - it's not an orphan!") );
      return;
	}
	//lprintf( WIDE("Adopting %p(%p) by %p(%p)")
	//		 , pOrphan
   //       , pFoster
	//		 );

   if( ( pOrphan->pElder = pFoster->pChild ) )
      pOrphan->pElder->pYounger = pOrphan;
   pOrphan->pParent = pFoster;
	pFoster->pChild = pOrphan;
   SmearFlag( pOrphan, IF_FLAG_FINAL_RENDER );
   // compute new image bounds within the parent...
   ComputeImageData( pOrphan );
}

//----------------------------------------------------------------------

 ImageFile * MakeSubImageEx ( ImageFile *pImage, S_32 x, S_32 y, _32 width, _32 height DBG_PASS)
{
	ImageFile *p;
	//if( !pImage )
	// return NULL;
	p = (ImageFile*)AllocateEx( sizeof( ImageFile ) DBG_RELAY );
	p->transform = NULL;
	p->flags = 0;
	p->width = p->real_width = width;
	p->height = p->real_height = height;
	if( pImage )
		p->pwidth = pImage->pwidth;
	else
		p->pwidth = 0;
	p->x = 0;
	p->y = 0;
	p->real_x = x;
	p->real_y = y;
	p->pChild = NULL;
	p->pYounger = NULL;
	p->pElder = NULL;
	p->image = NULL; // set it to nothing for now ComputeData will fix
	p->flags |= (pImage->flags & IF_FLAG_FINAL_RENDER);
	p->glActiveSurface = 0;
	p->glSurface = NULL;

	if( ( p->pParent = pImage ) )
	{
		if( ( p->pElder = pImage->pChild ) )
			p->pElder->pYounger = p;
		pImage->pChild = p;
	}
	ComputeImageData( p );
	return p;
}

//----------------------------------------------------------------------

 ImageFile * BuildImageFileEx ( PCOLOR pc, _32 Width, _32 Height DBG_PASS )
{
   ImageFile *p;
   p = (ImageFile*)AllocateEx( sizeof( ImageFile ) DBG_RELAY);
   p->transform = NULL;
   p->flags = 0;
   p->eff_x = p->x = p->real_x = 0;
   p->eff_y = p->y = p->real_y = 0;
   p->width = p->real_width = Width;
   p->eff_maxx = Width - 1;
   p->pwidth = Width;
   p->height = p->real_height = Height;
   p->eff_maxy = Height - 1;
   p->image = pc;
   p->pParent = NULL;
   p->pChild = NULL;
   p->pElder = NULL;
	p->pYounger = NULL;
   p->glSurface = 0;
	p->glActiveSurface = 0;
   // assume external colors...
	p->flags |= IF_FLAG_EXTERN_COLORS;
   return p;
}

//----------------------------------------------------------------------

 ImageFile *  RemakeImageEx ( ImageFile *pImage, PCOLOR pc
                                 , _32 width, _32 height DBG_PASS)
{
   // for this routine I'm gonna have to assume that the image
   // has been constantly remade ( this is only an entry point
   // use by vidlib at the moment for windows based systems...
   // therefore the caller is responcible for releasing any allocated
	// buffers...
   if( !pImage )
   {
      //Log3( WIDE("Building new image for buffer %08x (%d by %d)"), pc, width, height );
      pImage = BuildImageFileEx( pc, width, height DBG_RELAY );
      pImage->flags |= IF_FLAG_EXTERN_COLORS;
   }
   else
	{
		if( !pImage->pParent )
		{
			//Log3( WIDE("Re-building new image for buffer %08x (%d by %d)"), pc, width, height );
			pImage->image = pc;
			pImage->flags |= IF_FLAG_EXTERN_COLORS;
			pImage->real_width = width;
			pImage->width = width;
			pImage->pwidth = width;
			pImage->real_height = height;
			pImage->height = height;
			pImage->eff_maxx = ( pImage->real_width - 1 );
			pImage->eff_maxy = ( pImage->real_height - 1 );
			pImage->eff_x = 0;
			pImage->eff_y = 0;
			// recomput children associated with this image...
			// have a new suface - MUST move child surfaces...
			//ComputeImageData( pImage );
			{
				ImageFile *pSub;
				pSub = pImage->pChild;
				while( pSub )
				{
					ComputeImageData( pSub );
					pSub = pSub->pElder;
				}
			}
		}
		else
		{
			// okay if there's a parent - cannot do remake image...
			// that would mean this child will lose it's buffer
			// if the parent ever resizes or something...
         // AND.. should probably orphan this image if remade...
			// only do this compute when having a parent...
		}
	}
   return pImage;
}

//----------------------------------------------------------------------


 ImageFile * MakeImageFileEx (_32 Width, _32 Height DBG_PASS)
{
	//lprintf( WIDE("Allocate %d"),sizeof( COLOR ) * Width * Height  );
	Image tmp = BuildImageFileEx( (PCOLOR)AllocateEx( sizeof( COLOR ) * Width * Height DBG_RELAY )
										 , Width
										 , Height
										  DBG_RELAY );
	// even if it's parented, it should still be origined to itself...
	tmp->flags |= IF_FLAG_OWN_DATA;
	tmp->flags &= ~(IF_FLAG_EXTERN_COLORS); // not really extern colors
	return tmp;
}

//----------------------------------------------------------------------

 void  UnmakeImageFileEx ( ImageFile *pif DBG_PASS)
{
   if( pif )
   {
      if( pif->pChild ) // if there were sub images... cannot release yet
      {
         pif->flags |= IF_FLAG_FREE;
         return;
		}
      // it's a fake parented thing... should
      if( !pif->pParent || 
		    ( pif->flags & IF_FLAG_OWN_DATA ) ) // if this is not a sub image, and had no sub images
      {
         if( !( pif->flags & IF_FLAG_EXTERN_COLORS ) )
            ReleaseEx( pif->image DBG_RELAY );
      }
      else
      {
         // otherwise, unlink us from the chain of sub images...
         if( pif->pYounger )
            pif->pYounger->pElder = pif->pElder;
         else
            pif->pParent->pChild = pif->pElder;

         if( pif->pElder )
            pif->pElder->pYounger = pif->pYounger;
         if( !pif->pParent->pChild ) // we were last child...
         {
            // then check to see if the parent was already freed...
            if( pif->pParent->flags & IF_FLAG_FREE )
               UnmakeImageFile( pif->pParent );
         }
      }
      ReleaseEx( pif DBG_RELAY );
   }
}

//----------------------------------------------------------------------

Image DecodeMemoryToImage( P_8 buf, _32 size )
{
	Image file = NULL;
	//lprintf( WIDE("Attempting to decode an image...") );
#ifdef DO_PNG
	if( !file )
		file = ImagePngFile( buf, size );
#endif //DO_PNG

#ifdef DO_GIF
	if( !file )
		file = ImageGifFile( buf, size );
#endif //DO_GIF

#ifdef DO_BMP
	if( !file )
		file = ImageBMPFile( buf, size );
#endif

	// PLEASE NOTE: JPEG IS DUMB! and aborted our application
	//  PLEASE PLEASE take a look at this...
#ifdef DO_JPG
	if( !file )
		file = ImageJpgFile( buf, size );
#endif

#ifdef DO_TGA
	if( !file )
		file = ImageTgaFile( buf, size );
#endif //DO_TGA

	// consider a bitmap loader - though bmp has no header...

	return file;
}
//----------------------------------------------------------------------

ImageFile*  LoadImageFileFromGroupEx ( INDEX group, CTEXTSTR filename DBG_PASS )
{
	_32 size;
	P_8 buf;
	ImageFile* file = NULL;
	FILE* fp;

	fp = sack_fopen( group, filename, WIDE("rb"));
	if (!fp)
		return NULL;

	size = sack_fseek (fp, 0, SEEK_END);
	size = ftell (fp);
	sack_fseek (fp, 0, SEEK_SET);
	buf = (_8*) AllocateEx( size + 1 DBG_RELAY );
	sack_fread (buf, 1, size + 1, fp);
	sack_fclose (fp);

	file = 
		DecodeMemoryToImage( buf, size );

	if( file )
	{
		lprintf( WIDE( "making textue %s" ), filename );
		ReloadOpenGlTexture( file, 0 );
	}

	ReleaseEx( buf DBG_RELAY );
	return file;
}

//---------------------------------------------------------------------------

 ImageFile*  LoadImageFileEx ( CTEXTSTR filename DBG_PASS )
{
    return LoadImageFileFromGroupEx( 0, filename DBG_RELAY );
}

//---------------------------------------------------------------------------

void TranslateCoord( Image image, S_32 *x, S_32 *y )
{
	while( image )
	{
		//lprintf( WIDE( "%p adjust image %d,%d " ), image, image->real_x, image->real_y );
      (*x) += image->real_x;
		(*y) += image->real_y;
      image = image->pParent;
	}
}

//---------------------------------------------------------------------------

 int  MergeRectangle ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 )
{
   // results in the union of the area extents...
   // union will always result in a rectangle?...
   int tmp1, tmp2;
   
   if( r1->x < r2->x )
      r->x = r1->x;
   else
      r->x = r2->x;

   tmp1 = r1->width + r1->x;
   tmp2 = r2->width + r2->x;
   if( tmp1 > tmp2 )
      r->width = tmp1 - r->x;
   else
      r->width = tmp2 - r->x; 

   if( r1->y < r2->y )
      r->y = r1->y;
   else
      r->y = r2->y;

   tmp1 = r1->height + r1->y;
   tmp2 = r2->height + r2->y;
   if( tmp1 > tmp2 )
      r->height = tmp1 - r->y;
   else
      r->height = tmp2 - r->y;
   return TRUE;
}


//---------------------------------------------------------------------------

 int  IntersectRectangle ( IMAGE_RECTANGLE *r, IMAGE_RECTANGLE *r1, IMAGE_RECTANGLE *r2 )
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
   if( ( r1->x > (r2->x+(S_32)r2->width) ) ||
       ( r2->x > (r1->x+(S_32)r1->width) ) ||
       ( r1->y > (r2->y+(S_32)r2->height) ) ||
		( r2->y > (r1->y+(S_32)r1->height) ) )
	{
      //DebugBreak();
		return FALSE;
	}

   if( r1->x > r2->x )
      r->x = r1->x;
   else
      r->x = r2->x;

   tmp1 = r1->width + r1->x;
   tmp2 = r2->width + r2->x;
	if( tmp1 < tmp2 )
		if( r->x > tmp1 )
			r->width = 0;
		else
			r->width = tmp1 - r->x;
   else
		if( r->x > tmp2 )
			r->width = 0;
		else
			r->width = tmp2 - r->x;

	if( r1->y > r2->y )
		r->y = r1->y;
	else
		r->y = r2->y;

   tmp1 = r1->height + r1->y;
   tmp2 = r2->height + r2->y;
   if( tmp1 < tmp2 )
		if( r->y > tmp1 )
			r->height = 0;
		else
			r->height = tmp1 - r->y;
   else
		if( r->y > tmp2 )
			r->height = 0;
		else
			r->height = tmp2 - r->y;

   if( ( r->width == 0 )
		||( r->height == 0 ) )
	{
		return FALSE;
	}
   /*
   lprintf( WIDE("r : (%d,%d)-(%d,%d)  (%d,%d)-(%d,%d)")
           , r->x, r->y, r->width, r->height
           , r->x, r->y, r->x + r->width, r->y + r->height );
   */
   return TRUE;
}

//---------------------------------------------------------------------------

void  CPROC cSetColor( PCDATA po, int oo, int w, int h, CDATA color )
{
   oo /= 4;
   {
      int r;
      r = 0;
      while( r < h )
      {
         int col;
         col = 0;
         while( col < w )
         {
            *(CDATA*)po = color;
            po++;
            col++;
         }
         po += oo;
         r++;
      }
   }
}

void  CPROC cSetColorAlpha( PCDATA po, int oo, int w, int h, CDATA color )
{
   int alpha = AlphaVal(color);
   oo /= 4;
   {
      int r;
      r = 0;
      while( r < h )
      {
         int col;
         col = 0;
         while( col < w )
         {
            *po = DOALPHA( *po, color, alpha );
            po++;
            col++;
         }
         po += oo;
         r++;
      }
   }
}

struct glSurfaceImageData * MarkImageUpdated( Image child_image )
{
	Image image;
	for( image = child_image; image && image->pParent; image = image->pParent );

	;;

	{
		INDEX idx;
		struct glSurfaceData *data;
		struct glSurfaceImageData *current_image_data = NULL;
		LIST_FORALL( l.glSurface, idx, struct glSurfaceData *, data )
		{
			struct glSurfaceImageData *image_data;
			image_data = (struct glSurfaceImageData *)GetLink( &image->glSurface, idx );
			if( !image_data )
			{
				image_data = New( struct glSurfaceImageData );
				image_data->glIndex = 0;
				image_data->flags.updated = 1;
				SetLink( &image->glSurface, idx, image_data );
			}
			if( image_data->glIndex )
			{
				image_data->flags.updated = 1;
				//glDeleteTextures( 1, &image_data->glIndex );
				//image_data->glIndex = 0;
			}
			if( data == l.glActiveSurface )
				current_image_data = image_data;
		}
		return current_image_data;
	}
}

void  (CPROC*BlatPixelsAlpha)( PCDATA po, int oo, int w, int h
                  , CDATA color ) = cSetColorAlpha;

void  (CPROC*BlatPixels)( PCDATA po, int oo, int w, int h
                  , CDATA color ) = cSetColor;

//---------------------------------------------------------------------------
// This routine fills a rectangle with a solid color
// it is used for clear image, clear image to
// and for arbitrary rectangles - the direction
// of images does not matter.
void  BlatColor ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	PCDATA po;
	int  oo;
	if( !pifDest /*|| !pifDest->image*/ )
	{
		lprintf( WIDE( "No dest, or no dest image." ) );
		return;
	}

	if( !w )
		w = pifDest->width;
	if( !h )
		h = pifDest->height;
	{
		IMAGE_RECTANGLE r, r1, r2;
		// build rectangle of what we want to show
		r1.x      = x;
		r1.width  = w;
		r1.y      = y;
		r1.height = h;
		// build rectangle which is presently visible on image
		r2.x      = pifDest->x;
		r2.width  = pifDest->width;
		r2.y      = pifDest->y;
		r2.height = pifDest->height;
		if( !IntersectRectangle( &r, &r1, &r2 ) )
		{
			//lprintf( WIDE("blat color is out of bounds (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(") (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(")")
			//	, x, y, w, h
			//	, r2.x, r2.y, r2.width, r2.height );
			return;
		}
#ifdef DEBUG_BLATCOLOR
		// trying to figure out why there are stray lines for DISPLAY surfaces
      // apparently it's a logic in space node min/max to region conversion
		lprintf( WIDE("Rects %d,%d %d,%d/%d,%d %d,%d/ %d,%d %d,%d ofs %d,%d %d,%d")
				 , r1.x, r1.y
				 ,r1.width, r1.height
				 , r2.x, r2.y
				 ,r2.width, r2.height
				 , r.x, r.y, r.width, r.height
				 , pifDest->eff_x, pifDest->eff_y
				 , pifDest->eff_maxx, pifDest->eff_maxy

				 );
#endif
		x = r.x;
		w = r.width;
		y = r.y;
		h = r.height;
	}

	if( pifDest->flags & IF_FLAG_FINAL_RENDER )
	{
		VECTOR v[2][4];
		float _color[4];
		int vi = 0;
		_color[0] = RedVal( color );
		_color[1] = GreenVal( color );
		_color[2] = BlueVal( color );
		_color[3] = AlphaVal( color );
		TranslateCoord( pifDest, &x, &y );

		v[vi][0][0] = x;
		v[vi][0][1] = y;
		v[vi][0][2] = 0.0;
		v[vi][1][0] = x+w;
		v[vi][1][1] = y;
		v[vi][1][2] = 0.0;
		v[vi][2][0] = x;
		v[vi][2][1] = y+h;
		v[vi][2][2] = 0.0;
		v[vi][3][0] = x+w;
		v[vi][3][1] = y+h;
		v[vi][3][2] = 0.0;

		while( pifDest && pifDest->pParent )
		{
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
				Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
				Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
				Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
				vi = 1-vi;
			}
			pifDest = pifDest->pParent;
		}
		if( pifDest->transform )
		{
			Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
			Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
			Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
			Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
			vi = 1-vi;
		}
		scale( v[vi][0], v[vi][0], l.scale );
		scale( v[vi][1], v[vi][1], l.scale );
		scale( v[vi][2], v[vi][2], l.scale );
		scale( v[vi][3], v[vi][3], l.scale );

		EnableShader( "Simple Shader", v[vi], _color );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	}
	else
	{
		//lprintf( WIDE("Blotting %d,%d - %d,%d"), x, y, w, h );
		// start at origin on destination....
#ifdef _INVERT_IMAGE
		//y += h-1; // least address in memory.
		oo = 4*( (-(S_32)w) - pifDest->pwidth);     // w is how much we can copy...
#else
		oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...
#endif
		po = IMG_ADDRESS(pifDest,x,y);
		//oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...
		BlatPixels( po, oo, w, h, color );
		MarkImageUpdated( pifDest );
	}
}

void  BlatColorAlpha ( ImageFile *pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	PCDATA po;
	int  oo;
	if( !pifDest /*|| !pifDest->image */ )
	{
		lprintf( WIDE( "No dest, or no dest image." ) );
		return;
	}
	if( !w )
		w = pifDest->width;
	if( !h )
		h = pifDest->height;
	{
		IMAGE_RECTANGLE r, r1, r2;
		r1.x = x,
		r1.width = w;
		r1.y = y;
		r1.height = h;
		r2.x = pifDest->x;
		r2.width = pifDest->width;
		r2.y = pifDest->y;
		r2.height = pifDest->height;
		if( !IntersectRectangle( &r, &r1, &r2 ) )
		{
			lprintf( WIDE( "Blat color out of bounds" ) );
			return;
		}
#ifdef DEBUG_BLATCOLOR
		lprintf( WIDE("Rects %d,%d %d,%d/%d,%d %d,%d/ %d,%d %d,%d")
				 , r1.x, r1.y
				 ,r1.width, r1.height
				 , r2.x, r2.y
				 ,r2.width, r2.height
				 , r.x, r.y, r.width, r.height );
#endif
		// it's a same rectangle, it's safe to delete the comparison to <= 0 that was here
		x = r.x;
		w = r.width;
		y = r.y;
		h = r.height;
	}

	if( pifDest->flags & IF_FLAG_FINAL_RENDER )
	{
		VECTOR v[2][4];
		float _color[4];
		int vi = 0;

		_color[0] = RedVal( color );
		_color[1] = GreenVal( color );
		_color[2] = BlueVal( color );
		_color[3] = AlphaVal( color );
		TranslateCoord( pifDest, &x, &y );

		v[vi][0][0] = x;
		v[vi][0][1] = y;
		v[vi][0][2] = 0.0;
		v[vi][1][0] = x+w;
		v[vi][1][1] = y;
		v[vi][1][2] = 0.0;
		v[vi][2][0] = x;
		v[vi][2][1] = y+h;
		v[vi][2][2] = 0.0;
		v[vi][3][0] = x+w;
		v[vi][3][1] = y+h;
		v[vi][3][2] = 0.0;

		while( pifDest && pifDest->pParent )
		{
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
				Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
				Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
				Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
				vi = 1-vi;
			}
			pifDest = pifDest->pParent;
		}
		if( pifDest->transform )
		{
			Apply( pifDest->transform, v[1-vi][0], v[vi][0] );
			Apply( pifDest->transform, v[1-vi][1], v[vi][1] );
			Apply( pifDest->transform, v[1-vi][2], v[vi][2] );
			Apply( pifDest->transform, v[1-vi][3], v[vi][3] );
			vi = 1-vi;
		}
		scale( v[vi][0], v[vi][0], l.scale );
		scale( v[vi][1], v[vi][1], l.scale );
		scale( v[vi][2], v[vi][2], l.scale );
		scale( v[vi][3], v[vi][3], l.scale );

		EnableShader( "Simple Shader", v[vi], _color );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		CheckErr();
	}
	else
	{
		// start at origin on destination....
#ifdef _INVERT_IMAGE
		y += h-1;
#endif
		po = IMG_ADDRESS(pifDest,x,y);
		oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...

		BlatPixelsAlpha( po, oo, w, h, color );
		MarkImageUpdated( pifDest );
	}
}
//---------------------------------------------------------------------------
#undef ClearImageTo
 void  ClearImageTo ( ImageFile *pImage, CDATA c )
{
	BlatColor( pImage, 0, 0, pImage->real_width, pImage->real_height, c );
}

//---------------------------------------------------------------------------
#undef ClearImage
 void  ClearImage ( ImageFile *pImage )
{
	// should use 1 bit blue to make it definatly non transparent?
	// hmm.... nah - clear is CLEAR image...
	BlatColor( pImage, 0, 0, pImage->real_width, pImage->real_height, 0 );
}

//---------------------------------------------------------------------------

// this should only be used by internal functions (load gif)
// and on DYNAMIC bitmaps - do NOT use on DISPLAY bitmaps...
//#pragma warning( " // this should only be used by internal functions (load gif)" )
//#warning // and on DYNAMIC bitmaps - do NOT use on DISPLAY bitmaps...

 void  FlipImageEx ( ImageFile *pif DBG_PASS )
{
   PCOLOR temp, del;
   int i;
   //( WIDE("Flip image is VERY VERY VERY DANGEROUS!!!!!!!!!") );
   // if related to any other image - do NOT  flip...
   if( pif->pParent || pif->pChild || pif->pElder || pif->pYounger )
      return;

   if( pif )
   {
      temp = (PCOLOR)AllocateEx( sizeof( COLOR) * pif->width * pif->height DBG_RELAY );
      for( i = 0; i < pif->height; i++ )
      {
         MemCpy( temp+i*pif->width, pif->image + (((pif->height-1)-i)*pif->width ), pif->width * sizeof(COLOR) );
      }
      del = pif->image;
      pif->image = temp;
      ReleaseEx(del DBG_RELAY);
   }
}

//---------------------------------------------------------------------------

 void  UnloadFont ( SFTFont font )
{
   // uhmm - release? I dunno... nothin really
   // the font passed to loadfont should be discarded by the
   // application.
}

 SFTFont  LoadFont ( SFTFont font )
{
   // with direct usage we need no further information.
   return font;
}

//---------------------------------------------------------------------------

IMAGE_NAMESPACE_END
ASM_IMAGE_NAMESPACE

void CPROC cplot( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c );
CDATA CPROC cgetpixel( ImageFile *pi, S_32 x, S_32 y );

#ifdef HAS_ASSEMBLY
void CPROC asmplot( ImageFile *pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
void CPROC asmplotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
void CPROC asmplotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c );
void CPROC asmplotalphaMMX( ImageFile *pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
CDATA CPROC asmgetpixel( ImageFile *pi, S_32 x, S_32 y );
#endif

//---------------------------------------------------------------------------

void CPROC cplotraw( ImageFile *pi, S_32 x, S_32 y, CDATA c )
{
#ifdef _INVERT_IMAGE
   //y = (pi->real_height-1) - y;
#endif
	if( pi->flags & IF_FLAG_FINAL_RENDER )
	{
      VECTOR v;
      float _color[4];

      _color[0] = RedVal( c )/255.0f;
      _color[1] = GreenVal( c )/255.0f;
      _color[2] = BlueVal( c )/255.0f;
      _color[3] = AlphaVal( c )/255.0f;
		TranslateCoord( pi, &x, &y );
		v[0] = (float)(x/l.scale);
		v[1] = (float)(y/l.scale);
		v[2] = 0.0f;
		EnableShader( "Simple Shader", v, _color );

      glDrawArrays( GL_POINTS, 0, 1 );
		CheckErr();
	}
	else
	{
		*IMG_ADDRESS(pi,x,y) = c;
		MarkImageUpdated( pi );
	}
}

void CPROC cplot( ImageFile *pi, S_32 x, S_32 y, CDATA c )
{
   if( !pi ) return;
   if( ( x >= pi->x ) && ( x < (pi->x + pi->width )) &&
       ( y >= pi->y ) && ( y < (pi->y + pi->height )) )
   {
#ifdef _INVERT_IMAGE
     // y = ( pi->real_height - 1 )- y;
#endif
		if( pi->flags & IF_FLAG_FINAL_RENDER )
		{
			VECTOR v;
			float _color[4];

			_color[0] = RedVal( c )/255.0f;
			_color[1] = GreenVal( c )/255.0f;
			_color[2] = BlueVal( c )/255.0f;
			_color[3] = AlphaVal( c )/255.0f;
			TranslateCoord( pi, &x, &y );
			v[0] = (float)(x/l.scale);
			v[1] = (float)(y/l.scale);
			v[2] = 0.0f;
			EnableShader( "Simple Shader", v, _color );

			glDrawArrays( GL_POINTS, 0, 1 );
		}
		else if( pi->image )
		{
			*IMG_ADDRESS(pi,x,y)= c;
			MarkImageUpdated( pi );
		}
   }
}

//---------------------------------------------------------------------------

CDATA CPROC cgetpixel( ImageFile *pi, S_32 x, S_32 y )
{
   if( !pi || !pi->image ) return 0;
   if( ( x >= pi->x ) && ( x < (pi->x + pi->width )) &&
       ( y >= pi->y ) && ( y < (pi->y + pi->height )) )
   {
#ifdef _INVERT_IMAGE
      //y = (pi->real_height-1) - y;
#endif
		if( pi->flags & IF_FLAG_FINAL_RENDER )
		{
			lprintf( WIDE( "get pixel option on opengl surface" ) );
		}
		else
		{
			return *IMG_ADDRESS(pi,x,y);
		}
	}
   return 0;
}

//---------------------------------------------------------------------------

void CPROC cplotalpha( ImageFile *pi, S_32 x, S_32 y, CDATA c )
{
   CDATA *po;
   if( !pi ) return;
   if( ( x >= pi->x ) && ( x < (pi->x + pi->width )) &&
       ( y >= pi->y ) && ( y < (pi->y + pi->height) ) )
   {
#ifdef _INVERT_IMAGE
      //y = ( pi->height - 1 )- y;
#endif
		if( pi->flags & IF_FLAG_FINAL_RENDER )
		{
			VECTOR v;
			float _color[4];

			_color[0] = RedVal( c )/255.0f;
			_color[1] = GreenVal( c )/255.0f;
			_color[2] = BlueVal( c )/255.0f;
			_color[3] = AlphaVal( c )/255.0f;
			TranslateCoord( pi, &x, &y );
			v[0] = (float)(x/l.scale);
			v[1] = (float)(y/l.scale);
			v[2] = 0.0f;
			EnableShader( "Simple Shader", v, _color );

			glDrawArrays( GL_POINTS, 0, 1 );
		}
		else if( pi->image )
		{
			po = IMG_ADDRESS(pi,x,y);
			*po = DOALPHA( *po, c, AlphaVal(c) );
			MarkImageUpdated( pi );
		}
   }
}

//---------------------------------------------------------------------------

void CPROC do_linec( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_lineasm( ImageFile *pImage, S_32 x, S_32 y
               , S_32 xto, S_32 yto, CDATA color );
#endif

void CPROC do_lineAlphac( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_lineAlphaasm( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
void CPROC do_lineAlphaMMX( ImageFile *pImage, S_32 x, S_32 y
                    , S_32 xto, S_32 yto, CDATA color );
#endif

void CPROC do_lineExVc( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( ImageFile*pif, S_32 x, S_32 y, int d ) );
#ifdef HAS_ASSEMBLY
void CPROC do_lineExVasm( ImageFile *pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( ImageFile*pif, S_32 x, S_32 y, int d ) );
#endif

void CPROC do_hlinec( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_hlineasm( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#endif

void CPROC do_vlinec( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_vlineasm( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#endif

void CPROC do_hlineAlphac( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_hlineAlphaasm( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
void CPROC do_hlineAlphaMMX( ImageFile *pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#endif

void CPROC do_vlineAlphac( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_vlineAlphaasm( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
void CPROC do_vlineAlphaMMX( ImageFile *pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#endif

ASM_IMAGE_NAMESPACE_END
IMAGE_NAMESPACE


//---------------------------------------------------------------------------

 void  SetStringBehavior ( ImageFile *pImage, _32 behavior )
{
   pImage->flags &= ~(IF_FLAG_C_STRING|IF_FLAG_MENU_STRING|IF_FLAG_CONTROL_STRING );
   if( behavior == STRING_PRINT_RAW )
      ; // do nothing - already cleared all flags.
   else if( behavior == STRING_PRINT_CONTROL )
      pImage->flags |= IF_FLAG_CONTROL_STRING;
   else if( behavior == STRING_PRINT_C )
      pImage->flags |= IF_FLAG_C_STRING;
   else if( behavior == STRING_PRINT_MENU )
      pImage->flags |= IF_FLAG_MENU_STRING;
}

//---------------------------------------------------------------------------

 void  SetImageAuxRect ( Image pImage, P_IMAGE_RECTANGLE pRect )
{
   //lprintf( WIDE("Setting aux rect on %p = %d,%d - %d,%d"), pImage, pRect->x, pRect->y, pRect->width, pRect->height );
   if( pImage && pRect )
      pImage->auxrect = *pRect;
}

//---------------------------------------------------------------------------

 void  GetImageAuxRect ( Image pImage, P_IMAGE_RECTANGLE pRect )
{
   if( pImage && pRect )
      *pRect = pImage->auxrect;
}

//---------------------------------------------------------------------------

COLOR_CHANNEL GetRedValue( CDATA color )
{
	return RedVal( color );
}

//---------------------------------------------------------------------------

COLOR_CHANNEL GetGreenValue( CDATA color )
{
	return GreenVal( color );
}

//---------------------------------------------------------------------------

COLOR_CHANNEL GetBlueValue( CDATA color )
{
	return BlueVal( color );
}

//---------------------------------------------------------------------------

COLOR_CHANNEL GetAlphaValue( CDATA color )
{
	return AlphaVal( color );
}

//---------------------------------------------------------------------------

CDATA SetRedValue( CDATA color, COLOR_CHANNEL r )
{
	return SetRed( color, r );
}

//---------------------------------------------------------------------------

CDATA SetGreenValue( CDATA color, COLOR_CHANNEL g )
{
	return SetGreen( color, g );
}

//---------------------------------------------------------------------------

CDATA SetBlueValue( CDATA color, COLOR_CHANNEL b )
{
	return SetBlue( color, b );
}

//---------------------------------------------------------------------------

CDATA SetAlphaValue( CDATA color, COLOR_CHANNEL a )
{
	return SetAlpha( color, a  );
}

//---------------------------------------------------------------------------

CDATA MakeColor( COLOR_CHANNEL r, COLOR_CHANNEL g, COLOR_CHANNEL b )
{
	return Color(r,g,b);
}

//---------------------------------------------------------------------------

CDATA MakeAlphaColor( COLOR_CHANNEL r, COLOR_CHANNEL g, COLOR_CHANNEL b, COLOR_CHANNEL a )
{
	return AColor(r,g,b,a);
}

//---------------------------------------------------------------------------

PTRANSFORM GetImageTransformation( Image pImage )
{
	if( pImage )
	{
		if( !pImage->transform )
		{
			pImage->transform = CreateNamedTransform( NULL );
         pImage->coords[0][0] = 0;
         pImage->coords[0][1] = 0;
			pImage->coords[0][2] = 0;

         pImage->coords[1][0] = pImage->width;
         pImage->coords[1][1] = 0;
			pImage->coords[1][2] = 0;

         pImage->coords[2][0] = pImage->width;
         pImage->coords[2][1] = pImage->height;
         pImage->coords[2][2] = 0;
         pImage->coords[3][0] = 0;
         pImage->coords[3][1] = pImage->height;
         pImage->coords[3][2] = 0;
		}
		return pImage->transform;
	}
	return NULL;
}
void SetImageRotation( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, RCOORD rx, RCOORD ry, RCOORD rz )
{
	PTRANSFORM trans = CreateNamedTransform( NULL );
	VECTOR v_o;
	RCOORD x, y;
	if( edge_flag & IMAGE_ROTATE_FLAG_TOP )
		y = 0;
	else if( edge_flag & IMAGE_ROTATE_FLAG_BOTTOM )
		y = pImage->real_height;
	else
      y = ((RCOORD)pImage->real_height) / 2;
	if( edge_flag & IMAGE_ROTATE_FLAG_LEFT )
		x = 0;
	else if( edge_flag & IMAGE_ROTATE_FLAG_RIGHT )
		x = pImage->real_width;
	else
      x = ((RCOORD)pImage->real_width) / 2;
	Translate( trans, x, y, 0.0 );
	RotateAbs( trans, rx, ry, rz );
	Apply( trans, v_o, GetOrigin( pImage->transform ) );


   DestroyTransform( trans );
}
void RotateImageAbout( Image pImage, int edge_flag, RCOORD offset_x, RCOORD offset_y, PVECTOR vAxis, RCOORD angle )
{
	PTRANSFORM trans = CreateNamedTransform( NULL );
   VECTOR v_o;
	RCOORD x, y;
	if( edge_flag & IMAGE_ROTATE_FLAG_TOP )
		y = 0;
	else if( edge_flag & IMAGE_ROTATE_FLAG_BOTTOM )
		y = pImage->real_height;
	else
      y = ((RCOORD)pImage->real_height) / 2;
	if( edge_flag & IMAGE_ROTATE_FLAG_LEFT )
		x = 0;
	else if( edge_flag & IMAGE_ROTATE_FLAG_RIGHT )
		x = pImage->real_width;
	else
      x = ((RCOORD)pImage->real_width) / 2;
	Translate( trans, x, y, 0.0 );
	RotateAround( trans, vAxis, angle );
	Apply( trans, v_o, GetOrigin( pImage->transform ) );

   DestroyTransform( trans );
}

void MarkImageDirty( Image pImage )
{
	MarkImageUpdated( pImage );
}


//---------------------------------------------------------------------------
#define SetCopySetC( name ) ( name##T0 = c##name##T0,          \
                              name##T1 = c##name##T1,          \
                              name##TA = c##name##TA,          \
                              name##TImgA = c##name##TImgA,    \
                              name##TImgAI = c##name##TImgAI )

#ifdef STUPID_NO_DATA_EXPORTS
#define VFUNC(n) _PASTE(_,n)
#else
#define VFUNC(n) n
#endif

 void  SetBlotMethod ( _32 method )
{
   {
	   // common ... no alternative.
		VFUNC(ColorAverage) = cColorAverage;
      //Log( WIDE("Setting C Blots") );
      BlatPixels = cSetColor;
      BlatPixelsAlpha = cSetColorAlpha;
      SetCopySetC( CopyPixels );
      SetCopySetC( CopyPixelsShaded );
      SetCopySetC( CopyPixelsMulti );
      SetCopySetC( BlotScaled );
      SetCopySetC( BlotScaledShaded );
      SetCopySetC( BlotScaledMulti );
      VFUNC(plot) = cplot;
      VFUNC(plotalpha) = cplotalpha;
      VFUNC(getpixel) = cgetpixel;
      VFUNC(do_lineExV) = do_lineExVc;
      VFUNC(do_lineAlpha) = do_lineAlphac;
      VFUNC(do_line) = do_linec;
      VFUNC(do_hline) = do_hlinec;
      VFUNC(do_vline) = do_vlinec;
      VFUNC(do_hlineAlpha) = do_hlineAlphac;
      VFUNC(do_vlineAlpha) = do_vlineAlphac;
      //printf( WIDE("Blot Method C --------------------------\n ") );
   }
}

extern int link_interface_please;
void f(void )
{
   link_interface_please = 1;
}

void  SyncImage ( void )
{
   // if used directlyt his is alwasy syncronzied...
}


void SetImageTransformRelation( Image pImage, enum image_translation_relation relation, PRCOORD aux )
{
   // just call this to make sure the ptransform exists.
	GetImageTransformation( pImage );
	switch( relation )
	{
	case IMAGE_TRANSFORM_RELATIVE_TOP_LEFT:
		pImage->coords[0][0] = 0;
		pImage->coords[0][1] = 0;
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width;
		pImage->coords[1][1] = 0;
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width;
		pImage->coords[2][1] = pImage->height;
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0;
		pImage->coords[3][1] = pImage->height;
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_BOTTOM_RIGHT:
		pImage->coords[0][0] = -pImage->width;
		pImage->coords[0][1] = -pImage->height;
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = 0;
		pImage->coords[1][1] = -pImage->height;
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = 0;
		pImage->coords[2][1] = 0;
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = -pImage->width;
		pImage->coords[3][1] = 0;
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_TOP_RIGHT:
		pImage->coords[0][0] = -pImage->width;
		pImage->coords[0][1] = 0;
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = 0;
		pImage->coords[1][1] = 0;
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = 0;
		pImage->coords[2][1] = pImage->height;
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = -pImage->width;
		pImage->coords[3][1] = pImage->height;
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_BOTTOM_LEFT:
		pImage->coords[0][0] = 0;
		pImage->coords[0][1] = -pImage->height;
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width;
		pImage->coords[1][1] = -pImage->height;
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width;
		pImage->coords[2][1] = 0;
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0;
		pImage->coords[3][1] = 0;
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_BOTTOM:
		pImage->coords[0][0] = 0 - (pImage->width/2);
		pImage->coords[0][1] = 0 - (pImage->height);
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width - (pImage->width/2);
		pImage->coords[1][1] = 0 - (pImage->height);
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width - (pImage->width/2);
		pImage->coords[2][1] = 0;
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0 - (pImage->width/2);
		pImage->coords[3][1] = 0;
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_LEFT:
		pImage->coords[0][0] = 0;
		pImage->coords[0][1] = 0 - (pImage->height/2);
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width;
		pImage->coords[1][1] = 0 - (pImage->height/2);
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width;
		pImage->coords[2][1] = pImage->height - (pImage->height/2);
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0;
		pImage->coords[3][1] = pImage->height - (pImage->height/2);
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_TOP:
		pImage->coords[0][0] = 0 - (pImage->width/2);
		pImage->coords[0][1] = 0;
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width - (pImage->width/2);
		pImage->coords[1][1] = 0;
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width - (pImage->width/2);
		pImage->coords[2][1] = pImage->height;
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0 - (pImage->width/2);
		pImage->coords[3][1] = pImage->height;
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_RIGHT:
		pImage->coords[0][0] = -pImage->width;
		pImage->coords[0][1] = 0 - (pImage->height/2);
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = 0;
		pImage->coords[1][1] = 0 - (pImage->height/2);
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = 0;
		pImage->coords[2][1] = pImage->height - (pImage->height/2);
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = -pImage->width;
		pImage->coords[3][1] = pImage->height - (pImage->height/2);
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_CENTER:
		pImage->coords[0][0] = 0 - (pImage->width/2);
		pImage->coords[0][1] = 0 - (pImage->height/2);
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width - (pImage->width/2);
		pImage->coords[1][1] = 0 - (pImage->height/2);
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width - (pImage->width/2);
		pImage->coords[2][1] = pImage->height - (pImage->height/2);
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0 - (pImage->width/2);
		pImage->coords[3][1] = pImage->height - (pImage->height/2);
		pImage->coords[3][2] = 0;
		break;
	case IMAGE_TRANSFORM_RELATIVE_OTHER:
		pImage->coords[0][0] = 0 - (pImage->width/2);
		pImage->coords[0][1] = 0 - (pImage->height/2);
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width - (pImage->width/2);
		pImage->coords[1][1] = 0 - (pImage->height/2);
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = pImage->width - (pImage->width/2);
		pImage->coords[2][1] = pImage->height - (pImage->height/2);
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = 0 - (pImage->width/2);
		pImage->coords[3][1] = pImage->height - (pImage->height/2);
		pImage->coords[3][2] = 0;
		if( aux )
		{
			sub( pImage->coords[0], pImage->coords[0], aux );
			sub( pImage->coords[1], pImage->coords[1], aux );
			sub( pImage->coords[2], pImage->coords[2], aux );
			sub( pImage->coords[3], pImage->coords[3], aux );
		}
      break;
	}
}

struct workspace
{
	VECTOR v[2][4];
   float v_image[4][2];
	int vi;
	double x_size, x_size2, y_size, y_size2;
	PC_POINT origin;
	PC_POINT origin2;
	_POINT distance;
				
	RCOORD del;
	Image topmost_parent;

};

// this is a point-sprite engine too....
void Render3dImage( Image pifSrc, LOGICAL render_pixel_scaled )
{
	struct workspace _tmp;
	struct workspace *tmp = &_tmp;

	// closed loop to get the top imgae size.
	for( tmp->topmost_parent = pifSrc; tmp->topmost_parent->pParent; tmp->topmost_parent = tmp->topmost_parent->pParent );

	ReloadOpenGlTexture( pifSrc, 0 );
	if( !pifSrc->glActiveSurface )
	{
		lprintf( WIDE( "gl texture hasn't downloaded or went away?" ) );
		return;
	}
	//lprintf( WIDE( "use regular texture %p (%d,%d)" ), pifSrc, pifSrc->width, pifSrc->height );

	{
		tmp->vi = 0;
		/*
			* only a portion of the image is actually used, the rest is filled with blank space
			*
			*/
		SetPoint( tmp->v[tmp->vi][0], pifSrc->coords[0] );
		SetPoint( tmp->v[tmp->vi][1], pifSrc->coords[1] );
		SetPoint( tmp->v[tmp->vi][2], pifSrc->coords[2] );
		SetPoint( tmp->v[tmp->vi][3], pifSrc->coords[3] );

		tmp->x_size = (double) pifSrc->x/ (double)tmp->topmost_parent->width;
		tmp->x_size2 = (double) (pifSrc->x+pifSrc->width)/ (double)tmp->topmost_parent->width;
		tmp->y_size = (double) pifSrc->y/ (double)tmp->topmost_parent->height;
		tmp->y_size2 = (double) (pifSrc->y+pifSrc->height)/ (double)tmp->topmost_parent->height;

		if( render_pixel_scaled )
		{
			tmp->origin = GetOrigin( l.camera );
			tmp->origin2 = GetOrigin( pifSrc->transform );
			
			sub( tmp->distance, tmp->origin2, tmp->origin );
			tmp->del = dotproduct( tmp->distance, GetAxis( l.camera, vForward ) );
			// no point, it's behind the camera.
			if( tmp->del < 1.0 )
				return;
			scale( tmp->v[1-tmp->vi][0], tmp->v[tmp->vi][0], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			scale( tmp->v[1-tmp->vi][1], tmp->v[tmp->vi][1], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			scale( tmp->v[1-tmp->vi][2], tmp->v[tmp->vi][2], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			scale( tmp->v[1-tmp->vi][3], tmp->v[tmp->vi][3], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			tmp->vi = 1-tmp->vi;
		}

		ApplyRotation( l.camera, tmp->v[1-tmp->vi][0], tmp->v[tmp->vi][0] );
		ApplyRotation( l.camera, tmp->v[1-tmp->vi][1], tmp->v[tmp->vi][1] );
		ApplyRotation( l.camera, tmp->v[1-tmp->vi][2], tmp->v[tmp->vi][2] );
		ApplyRotation( l.camera, tmp->v[1-tmp->vi][3], tmp->v[tmp->vi][3] );
		tmp->vi = 1-tmp->vi;

		while( pifSrc && pifSrc->pParent )
		{
			if( pifSrc->transform )
			{
				Apply( pifSrc->transform, tmp->v[1-tmp->vi][0], tmp->v[tmp->vi][0] );
				Apply( pifSrc->transform, tmp->v[1-tmp->vi][1], tmp->v[tmp->vi][1] );
				Apply( pifSrc->transform, tmp->v[1-tmp->vi][2], tmp->v[tmp->vi][2] );
				Apply( pifSrc->transform, tmp->v[1-tmp->vi][3], tmp->v[tmp->vi][3] );
				tmp->vi = 1-tmp->vi;
			}
			pifSrc = pifSrc->pParent;
		}
		if( pifSrc->transform )
		{
			Apply( pifSrc->transform, tmp->v[1-tmp->vi][0], tmp->v[tmp->vi][0] );
			Apply( pifSrc->transform, tmp->v[1-tmp->vi][1], tmp->v[tmp->vi][1] );
			Apply( pifSrc->transform, tmp->v[1-tmp->vi][2], tmp->v[tmp->vi][2] );
			Apply( pifSrc->transform, tmp->v[1-tmp->vi][3], tmp->v[tmp->vi][3] );
			tmp->vi = 1-tmp->vi;
		}

      tmp->v_image[0][0] = tmp->x_size;
      tmp->v_image[0][1] = tmp->y_size2;
      tmp->v_image[1][0] = tmp->x_size2;
      tmp->v_image[1][1] = tmp->y_size2;
      tmp->v_image[2][0] = tmp->x_size2;
      tmp->v_image[2][1] = tmp->y_size;
      tmp->v_image[3][0] = tmp->x_size;
		tmp->v_image[3][1] = tmp->y_size;

		EnableShader( "Simple Texture Shader", tmp->v[tmp->vi], tmp->v_image, pifSrc->glActiveSurface );
      glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		Deallocate( struct workspace *, tmp );
	}
}



void InitShader( void )
{
#ifndef __ANDROID__
	const char *string = glGetString(GL_SHADING_LANGUAGE_VERSION);
	lprintf( "Supported version: %s", string );
#endif
	           
#ifdef __ANDROID__
#else
	if( !l.glActiveSurface )
		return;

	if( !l.glActiveSurface->shader.flags.init_ok )
	{
		int result;
 		l.glActiveSurface->shader.flags.init_ok = 1;
		if (GLEW_OK != glewInit() )
		{
			return;
		}

		if( !glUseProgram )
		{
			lprintf(WIDE( "glUseProgram failed to resolve." ) );
			return;
		}



		glEnable(GL_FRAGMENT_PROGRAM_ARB);
      CheckErr();

 		glEnable(GL_VERTEX_PROGRAM_ARB);
      CheckErr();

		{
			//---------------------------------------------------------------

			glGenProgramsARB(1, &l.glActiveSurface->shader.multi_shader);
			result = glGetError();
			if( result )
			{
					lprintf( WIDE( "shader resulted %d" ), result );
			}
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, l.glActiveSurface->shader.multi_shader);
			{
				int result;
				char *program_string = ""
					//"!!NVfp4.0\n"
					"!!ARBfp1.0\n"
					"# cgc version 3.0.0015, build date Nov 15 2010\n"
					"# command line args: -oglsl -profile gpu_fp\n"
					"# source file: multishade.fx\n"
					"#vendor NVIDIA Corporation\n"
					"#version 3.0.0.15\n"
					"#profile gpu_fp\n"
					"#program main\n"
					"#semantic input\n"
					"#semantic multishade_r\n"
					"#semantic multishade_g\n"
					"#semantic multishade_b\n"
					"#var float4 gl_TexCoord[0] : $vin.TEX0 : TEX0 : -1 : 1\n"
					"#var float4 gl_TexCoord[1] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[2] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[3] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[4] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[5] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[6] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[7] :  :  : -1 : 0\n"
					"#var float4 gl_FragColor : $vout.COLOR : COL0[0] : -1 : 1\n"
					"#var sampler2D input :  : texunit 0 : -1 : 1\n"
					"#var float4 multishade_r :  : c[0] : -1 : 1\n"
					"#var float4 multishade_g :  : c[1] : -1 : 1\n"
					"#var float4 multishade_b :  : c[2] : -1 : 1\n"
					"PARAM c[4] = { program.local[0..2],\n"
							"{ 1, 0 } };\n"
					"TEMP R0;\n"
					"TEMP R1;\n"
					"TEMP R2;\n"
					"TEX R0, fragment.texcoord[0], texture[0], 2D;\n"
					"ABS R1.z, R0.y;\n"
					"ABS R1.x, R0.z;\n"
					"CMP R1.x, -R1, c[3], c[3].y;\n"
					"ABS R1.x, R1;\n"
					"ABS R1.y, R0.x;\n"
					"CMP R1.z, -R1, c[3].x, c[3].y;\n"
					"CMP R1.y, -R1, c[3].x, c[3];\n"
					"ABS R1.z, R1;\n"
					"ABS R1.y, R1;\n"
					"CMP R1.y, -R1, c[3], c[3].x;\n"
					"CMP R1.z, -R1, c[3].y, c[3].x;\n"
					"MUL R1.z, R1.y, R1;\n"
					"CMP R1.x, -R1, c[3].y, c[3];\n"
					"MUL R1.x, R1.z, R1;\n"
					"MUL R1.w, R0, c[2];\n"
					"MUL R2.x, R0.w, c[1].w;\n"
					"CMP R1.x, -R1, c[3].y, R1.w;\n"
					"CMP R1.z, -R1, R1.x, R2.x;\n"
					"MUL R1.x, R0.y, c[1];\n"
					"MUL R0.w, R0, c[0];\n"
					"MAD R1.x, R0.z, c[2], R1;\n"
					"CMP result.color.w, -R1.y, R1.z, R0;\n"
					"MUL R0.w, R0.y, c[1].y;\n"
					"MUL R0.y, R0, c[1].z;\n"
					"MAD R0.w, R0.z, c[2].y, R0;\n"
					"MAD R0.y, R0.z, c[2].z, R0;\n"
					"MAD result.color.x, R0, c[0], R1;\n"
					"MAD result.color.y, R0.x, c[0], R0.w;\n"
					"MAD result.color.z, R0.x, c[0], R0.y;\n"
					"END\n"
					"";
#undef strlen
				glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(program_string), program_string);

				result = glGetError();
				if( result )
				{
					static char infoLog[4096];
					int length = 0;
					const GLubyte *tmp = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
					lprintf( WIDE( "error: %s" ), tmp);
					lprintf( WIDE( "shader resulted %d" ), result );
				}
#if 0
				glUseProgramObjectARB( l.glActiveSurface->shader.multi_shader );

				result = glGetError();
				if( result )
				{
					static char infoLog[4096];
					int length = 0;
					const GLubyte *tmp = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
					glGetInfoLogARB( l.shader.multi_shader, 4096, &length, infoLog );
					lprintf( WIDE( "error: %s" ), tmp);
					lprintf( WIDE( "shader resulted %d" ), result );
				}

				{
					int count;
					int length = 0;
					char buffer[4096];
					int size = 0;
					int i;
					int type;
					//glGetProgramivARB( l.glActiveSurface->shader.multi_shader,  GL_ACTIVE_UNIFORMS, &count );
					glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_ATTRIBS_ARB, &count );
					glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_PARAMETERS_ARB, &count );
					glGetProgramivARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_NATIVE_PARAMETERS_ARB, &count );

					//glProgramLocalParameter4fARB( GL_FRAGMENT_PROGRAM_ARB, 0, 
					//glGetObjectParameterivARB (l.glActiveSurface->shader.multi_shader, GL_OBJECT_ACTIVE_UNIFORMS, &count);
					count = 3;
					// for i in 0 to count:
					//for( i = 0; i < count; i++ )
						//glGetProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, 
//						glGetActiveUniform  (l.glActiveSurface->shader.multi_shader, i, 4096, &length, &size, &type, buffer);
				}

				glUseProgram( l.glActiveSurface->shader.multi_shader );
				{
					const char *names[10] = { "multishade_r", "multishade_g", "multishade_b" };
					int indeces[10];

					glGetUniformIndices(l.glActiveSurface->shader.multi_shader, 3, names, indeces);
					lprintf( WIDE( "ya..." ) );
					result = glGetError();
					if( result )
					{
						const GLubyte *tmp = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
						lprintf( WIDE( "error: %s" ), tmp);
						lprintf( WIDE( "shader resulted %d" ), result );
					}
					//glGetUniform
					result = glGetUniformLocation(l.glActiveSurface->shader.multi_shader, "c[0]");
					result = glGetUniformLocation(l.glActiveSurface->shader.multi_shader, "c");

					//glUniform1ui( l.glActiveSurface->shader.multi_shader, 0, 
					lprintf( WIDE( "ya..." ) );
				}
#endif
			}

//---------------------------------------------------------------

			glGenProgramsARB(1, &l.glActiveSurface->shader.inverse_shader);
			result = glGetError();
			if( result )
			{
					lprintf( WIDE( "shader resulted %d" ), result );
			}
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, l.glActiveSurface->shader.inverse_shader);
			{
				int result;
				char *program_string = 
#if 0
					"// glslf output by Cg compiler\n"
					"// cgc version 3.0.0015, build date Nov 15 2010\n"
					"// command line args: -oglsl -profile glslf\n"
					"// source file: invert.frag\n"
					"//vendor NVIDIA Corporation\n"
					"//version 3.0.0.15\n"
					"//profile glslf\n"
					"//program main\n"
					"//semantic input\n"
					"//var sampler2D input :  : _input : -1 : 1\n"
					"//var float4 gl_TexCoord[0] : $vin.$TEX0 : $TEX0 : -1 : 1\n"
					"//var float4 gl_TexCoord[1] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[2] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[3] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[4] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[5] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[6] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[7] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[8] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_TexCoord[9] : $vin.<null atom> :  : -1 : 0\n"
					"//var float4 gl_FragColor : $vout.COLOR : $COL0 : -1 : 1\n"
					"\n"
					"vec4 _TMP1;\n"
					"varying vec4 TEX0;\n"
					"varying vec4 COL0;\n"
					"uniform sampler2D _input;\n"
					"\n"
					" // main procedure, the original name was main\n"
					"void main()\n"
					"{\n"
					"\n"
					"\n"
					"    _TMP1 = texture2D(_input, TEX0.xy);\n"
					"    gl_FragColor = vec4(1.00000000E+000 - _TMP1.x, 1.00000000E+000 - _TMP1.y, 1.00000000E+000 - _TMP1.z, _TMP1.w);\n"
					"    COL0 = gl_FragColor;\n"
					"} // main end\n"
#endif
#if 1
					"!!ARBfp1.0\n"
					"# cgc version 3.0.0015, build date Nov 15 2010\n"
					"# command line args: -oglsl -profile arbfp1\n"
					"# source file: invert.frag\n"
					"#vendor NVIDIA Corporation\n"
					"#version 3.0.0.15\n"
					"#profile arbfp1\n"
					"#program main\n"
					"#semantic input\n"
					"#var float4 gl_TexCoord[0] : $vin.TEX0 : TEX0 : -1 : 1\n"
					"#var float4 gl_TexCoord[1] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[2] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[3] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[4] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[5] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[6] :  :  : -1 : 0\n"
					"#var float4 gl_TexCoord[7] :  :  : -1 : 0\n"
					"#var float4 gl_FragColor : $vout.COLOR : COL : -1 : 1\n"
					"#var sampler2D input :  : texunit 0 : -1 : 1\n"
					"#const c[0] = 1.0\n"
					"PARAM c[1] = { { 1.0,1.0,1.0 } };\n"
					"TEMP R0;\n"
					"TEX R0, fragment.texcoord[0], texture[0], 2D;\n"
				   "ADD result.color.xyz, -R0, c[0].x;\n"
				   "MOV result.color.w, R0;\n"
				  // "MOV result.color, R0;\n"
					"END\n"
					"# 3 instructions, 1 R-regs\n"
#endif
					"";
				glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(program_string), program_string);

				result = glGetError();
				if( result )
				{
					static char infoLog[4096];
					int length = 0;
					const GLubyte *tmp = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
					lprintf( WIDE( "error: %s" ), tmp);
					lprintf( WIDE( "shader resulted %d" ), result );
				}
			}

//---------------------------------------------------------------

			glDisable(GL_FRAGMENT_PROGRAM_ARB);
		}
	}
#endif
}

PUBLIC( void, InvokePreloads )( void )
{
}

 PCDATA  ImageAddress ( Image i, S_32 x, S_32 y )
{
	return ((CDATA*) \
                               ((i)->image + (( (x) - (i)->eff_x ) \
					                +(INVERTY( (i), (y) ) * (i)->pwidth ) \
                               ))   
										)
										;
}

#ifdef STUPID_NO_DATA_EXPORTS
#define Noa b a CPROC b
No CDATA  ColorAverage ( CDATA c1, CDATA c2
											, int d, int max )
{
   _ColorAverage(c1,c2,d,max);
}


No void plot      ( Image pi, S_32 x, S_32 y, CDATA c )
{
   _plot(pi,x,y,c);
}
No void plotalpha ( Image pi, S_32 x, S_32 y, CDATA c )
{
   _plotalpha(pi,x,y,c);
}
No CDATA getpixel ( Image pi, S_32 x, S_32 y )
{
   return _getpixel(pi,x,y);
}
//-------------------------------
// Line functions  (lineasm.asm) // should include a line.c ... for now core was assembly...
//-------------------------------
No void do_line     ( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color )  // d is color data...
{
   _do_line( pBuffer, x, y, xto, yto, color );
}
No void do_lineAlpha( Image pBuffer, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color)  // d is color data...

{
	_do_lineAlpha( pBuffer, x, y, xto, yto, color );
}
No void do_hline     ( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color )
{
	_do_hline( pImage, y, xfrom, xto, color );
}
No void do_vline     ( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color )
{
	_do_vline( pImage, x, yfrom, yto, color );
}
No void do_hlineAlpha( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color )
{
	_do_hlineAlpha( pImage, y, xfrom, xto, color );
}
No void do_vlineAlpha( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color )
{
	_do_vlineAlpha( pImage, x,yfrom, yto, color );
}
No void  do_lineExV( Image pImage, S_32 x, S_32 y
									  , S_32 xto, S_32 yto, CDATA color
									  , void (*func)( Image pif, S_32 x, S_32 y, int d ) )
{
	_do_lineExV(pImage,x,y,xto,yto,color,func);
}

#endif
IMAGE_NAMESPACE_END


// $Log: image.c,v $
// Revision 1.78  2005/05/19 23:53:15  jim
// protect blatcoloralpha from working with an image without a surface.
//
// Revision 1.28  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
