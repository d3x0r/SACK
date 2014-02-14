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
#include "../image_common.h"

#include "local.h"
#include "blotproto.h"

IMAGE_NAMESPACE 
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

static void OnFirstDraw3d( WIDE( "@00 PUREGL Image Library" ) )( PTRSZVAL psv )
{
	l.glActiveSurface = (struct glSurfaceData *)psv;
}

static PTRSZVAL OnInit3d( WIDE( "@00 PUREGL Image Library" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *pIdentity_depty, RCOORD *aspect )
{
	struct glSurfaceData *glSurface = New( struct glSurfaceData );
	glSurface->T_Camera = camera;
	glSurface->identity_depth = pIdentity_depty;
	glSurface->aspect = aspect;
	MemSet( &glSurface->shader, 0, sizeof( glSurface->shader ) );
lprintf( "Init library..." );
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
}


int ReloadOpenGlTexture( Image child_image, int option )
{
	Image image;
	//lprintf( "reload... %p", child_image );
	if( !child_image) 
		return 0;
	for( image = child_image; image && image->pParent; image = image->pParent );

	{
		struct glSurfaceImageData *image_data = 
			(struct glSurfaceImageData *)GetLink( &image->glSurface
			                                    , l.glImageIndex );
		//GLuint glIndex;
		//lprintf( "image_data %p", image_data );
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
				int gl_error;
				gl_error = glGetError() ;
				if( gl_error )
				{
					lprintf( WIDE("Previous error") );
				}
				glGenTextures(1, &image_data->glIndex);			// Create One Texture
				if( ( gl_error = glGetError() ) || !image_data->glIndex)
				{
					lprintf( WIDE( "gen text %d or bad surafce" ), gl_error );
					return 0;
				}
			}
			if( image_data->flags.updated )
			{
				if( option & 2 )
				{
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
				glColor4ub( 255, 255, 255, 255 );
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
					glBindTexture( GL_TEXTURE_2D, output_image->glActiveSurface );
					glColor4ub( 255,255,255,255 );
					return output_image->glActiveSurface;//ReloadOpenGlTexture( child_image, option );
				}

}
IMAGE_NAMESPACE_END
ASM_IMAGE_NAMESPACE
extern void  (CPROC*BlatPixelsAlpha)( PCDATA po, int oo, int w, int h
                  , CDATA color );

extern void  (CPROC*BlatPixels)( PCDATA po, int oo, int w, int h
                  , CDATA color );
ASM_IMAGE_NAMESPACE_END
IMAGE_NAMESPACE

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
		VECTOR v1[2], v2[2];
		VECTOR v3[2], v4[2];
		int v = 0;

		TranslateCoord( pifDest, &x, &y );

		v1[v][0] = x;
		v1[v][1] = y;
		v1[v][2] = 0.0;
		v2[v][0] = x+w;
		v2[v][1] = y;
		v2[v][2] = 0.0;
		v3[v][0] = x;
		v3[v][1] = y+h;
		v3[v][2] = 0.0;
		v4[v][0] = x+w;
		v4[v][1] = y+h;
		v4[v][2] = 0.0;

		while( pifDest && pifDest->pParent )
		{
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v1[1-v], v1[v] );
				Apply( pifDest->transform, v2[1-v], v2[v] );
				Apply( pifDest->transform, v3[1-v], v3[v] );
				Apply( pifDest->transform, v4[1-v], v4[v] );
				v = 1-v;
			}
			pifDest = pifDest->pParent;
		}
		if( pifDest->transform )
		{
			Apply( pifDest->transform, v1[1-v], v1[v] );
			Apply( pifDest->transform, v2[1-v], v2[v] );
			Apply( pifDest->transform, v3[1-v], v3[v] );
			Apply( pifDest->transform, v4[1-v], v4[v] );
			v = 1-v;
		}
		scale( v1[v], v1[v], l.scale );
		scale( v2[v], v2[v], l.scale );
		scale( v3[v], v3[v], l.scale );
		scale( v4[v], v4[v], l.scale );

#ifndef __ANDROID__
		glBegin(GL_QUADS);
		glColor3ubv( (GLubyte*)&color );
		glVertex3fv( v1[v] );	// Bottom Left Of The Texture and Quad
		glVertex3fv( v2[v] );	// Bottom Right Of The Texture and Quad
		glVertex3fv( v4[v] );	// Top Left Of The Texture and Quad
		glVertex3fv( v3[v] );	// Top Right Of The Texture and Quad
		glEnd();
#else
		glBegin(GL_TRIANGLE_STRIP);
		glColor3ubv( (GLubyte*)&color );
		glVertex3fv( v1[v] );	// Bottom Left Of The Texture and Quad
		glVertex3fv( v4[v] );	// Top Left Of The Texture and Quad
		glVertex3fv( v2[v] );	// Bottom Right Of The Texture and Quad
		glVertex3fv( v3[v] );	// Top Right Of The Texture and Quad
		glEnd();
#endif
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
         /*
			lprintf( WIDE( "Blat color out of bounds" ) );
		lprintf( WIDE("Rects %d,%d %d,%d/%d,%d %d,%d/ %d,%d %d,%d")
				 , r1.x, r1.y
				 ,r1.width, r1.height
				 , r2.x, r2.y
				 ,r2.width, r2.height
				 , r.x, r.y, r.width, r.height );]
          */
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
		VECTOR v1[2], v2[2];
		VECTOR v3[2], v4[2];
		int v = 0;

		TranslateCoord( pifDest, &x, &y );

		v1[v][0] = x;
		v1[v][1] = y;
		v1[v][2] = 0.0;
		v2[v][0] = x+w;
		v2[v][1] = y;
		v2[v][2] = 0.0;
		v3[v][0] = x;
		v3[v][1] = y+h;
		v3[v][2] = 0.0;
		v4[v][0] = x+w;
		v4[v][1] = y+h;
		v4[v][2] = 0.0;

		while( pifDest && pifDest->pParent )
		{
			if( pifDest->transform )
			{
				Apply( pifDest->transform, v1[1-v], v1[v] );
				Apply( pifDest->transform, v2[1-v], v2[v] );
				Apply( pifDest->transform, v3[1-v], v3[v] );
				Apply( pifDest->transform, v4[1-v], v4[v] );
				v = 1-v;
			}
			pifDest = pifDest->pParent;
		}
		if( pifDest->transform )
		{
			Apply( pifDest->transform, v1[1-v], v1[v] );
			Apply( pifDest->transform, v2[1-v], v2[v] );
			Apply( pifDest->transform, v3[1-v], v3[v] );
			Apply( pifDest->transform, v4[1-v], v4[v] );
			v = 1-v;
		}
		scale( v1[v], v1[v], l.scale );
		scale( v2[v], v2[v], l.scale );
		scale( v3[v], v3[v], l.scale );
		scale( v4[v], v4[v], l.scale );
#ifndef __ANDROID__
		glBegin( GL_QUADS );
		glColor4ubv( (GLubyte*)&color );
		glVertex3fv( v1[v] );	// Bottom Left Of The Texture and Quad
		glVertex3fv( v2[v] );	// Bottom Right Of The Texture and Quad
		glVertex3fv( v4[v] );	// Top Left Of The Texture and Quad
		glVertex3fv( v3[v] );	// Top Right Of The Texture and Quad
		glEnd();
#else
		glBegin( GL_TRIANGLE_STRIP );
		glColor4ubv( (GLubyte*)&color );
		glVertex3fv( v1[v] );	// Bottom Left Of The Texture and Quad
		glVertex3fv( v2[v] );	// Bottom Right Of The Texture and Quad
		glVertex3fv( v3[v] );	// Top Right Of The Texture and Quad
		glVertex3fv( v4[v] );	// Top Left Of The Texture and Quad
		glEnd();
#endif
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
		BlatColor( pi, x, y, 1, 1, c );
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
			BlatColor( pi, x, y, 1, 1, c );
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
			BlatColor( pi, x, y, 1, 1, c );
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
	VECTOR v1[2], v3[2],v4[2],v2[2];
	int v;
	double x_size, x_size2, y_size, y_size2;
	PC_POINT origin;
	PC_POINT origin2;
	_POINT distance;
				
	RCOORD del;
	Image topmost_parent;

};

// this is a point-sprite engine too....
void Render3dImage( Image pifSrc, PCVECTOR o, LOGICAL render_pixel_scaled )
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
		tmp->v = 0;
		/*
			* only a portion of the image is actually used, the rest is filled with blank space
			*
			*/
		SetPoint( tmp->v1[tmp->v], pifSrc->coords[0] );
		SetPoint( tmp->v2[tmp->v], pifSrc->coords[1] );
		SetPoint( tmp->v3[tmp->v], pifSrc->coords[2] );
		SetPoint( tmp->v4[tmp->v], pifSrc->coords[3] );

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
			scale( tmp->v1[1-tmp->v], tmp->v1[tmp->v], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			scale( tmp->v2[1-tmp->v], tmp->v2[tmp->v], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			scale( tmp->v3[1-tmp->v], tmp->v3[tmp->v], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			scale( tmp->v4[1-tmp->v], tmp->v4[tmp->v], ( (l.glActiveSurface->aspect[0])*2*tmp->del ) / l.glActiveSurface->identity_depth[0] );
			tmp->v = 1-tmp->v;
		}

		ApplyRotation( l.camera, tmp->v1[1-tmp->v], tmp->v1[tmp->v] );
		ApplyRotation( l.camera, tmp->v2[1-tmp->v], tmp->v2[tmp->v] );
		ApplyRotation( l.camera, tmp->v3[1-tmp->v], tmp->v3[tmp->v] );
		ApplyRotation( l.camera, tmp->v4[1-tmp->v], tmp->v4[tmp->v] );
		tmp->v = 1-tmp->v;

		while( pifSrc && pifSrc->pParent )
		{
			if( pifSrc->transform )
			{
				Apply( pifSrc->transform, tmp->v1[1-tmp->v], tmp->v1[tmp->v] );
				Apply( pifSrc->transform, tmp->v2[1-tmp->v], tmp->v2[tmp->v] );
				Apply( pifSrc->transform, tmp->v3[1-tmp->v], tmp->v3[tmp->v] );
				Apply( pifSrc->transform, tmp->v4[1-tmp->v], tmp->v4[tmp->v] );
				tmp->v = 1-tmp->v;
			}
			pifSrc = pifSrc->pParent;
		}
		if( pifSrc->transform )
		{
			Apply( pifSrc->transform, tmp->v1[1-tmp->v], tmp->v1[tmp->v] );
			Apply( pifSrc->transform, tmp->v2[1-tmp->v], tmp->v2[tmp->v] );
			Apply( pifSrc->transform, tmp->v3[1-tmp->v], tmp->v3[tmp->v] );
			Apply( pifSrc->transform, tmp->v4[1-tmp->v], tmp->v4[tmp->v] );
			tmp->v = 1-tmp->v;
		}

		glBindTexture(GL_TEXTURE_2D, pifSrc->glActiveSurface);				// Select Our Texture

#ifndef __ANDROID__
		glBegin(GL_QUADS);
		glTexCoord2f(tmp->x_size, tmp->y_size2); glVertex3fv(tmp->v1[tmp->v]);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(tmp->x_size, tmp->y_size); glVertex3fv(tmp->v4[tmp->v]);	// Top Left Of The Texture and Quad
		glTexCoord2f(tmp->x_size2, tmp->y_size); glVertex3fv(tmp->v3[tmp->v]);	// Top Right Of The Texture and Quad
		glTexCoord2f(tmp->x_size2, tmp->y_size2); glVertex3fv(tmp->v2[tmp->v]);	// Bottom Right Of The Texture and Quad
		glEnd();
#else
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(tmp->x_size, tmp->y_size2); glVertex3fv(tmp->v1[tmp->v]);	// Bottom Left Of The Texture and Quad
		glTexCoord2f(tmp->x_size, tmp->y_size); glVertex3fv(tmp->v4[tmp->v]);	// Top Left Of The Texture and Quad
		glTexCoord2f(tmp->x_size2, tmp->y_size2); glVertex3fv(tmp->v2[tmp->v]);	// Bottom Right Of The Texture and Quad
		glTexCoord2f(tmp->x_size2, tmp->y_size); glVertex3fv(tmp->v3[tmp->v]);	// Top Right Of The Texture and Quad
		glEnd();
#endif
		glBindTexture(GL_TEXTURE_2D, 0);				// Select Our Texture
		Deallocate( struct workspace *, tmp );
	}
}



void InitShader( void )
{
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
		result = glGetError();
		if( result )
		{
				lprintf( WIDE( "fragment shader enable resulted %d" ), result );
		}

 		glEnable(GL_VERTEX_PROGRAM_ARB);
		result = glGetError();
		if( result )
		{
				lprintf( WIDE( "vertext shader enable resulted %d" ), result );
		}

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


IMAGE_NAMESPACE_END


// $Log: image.c,v $
// Revision 1.78  2005/05/19 23:53:15  jim
// protect blatcoloralpha from working with an image without a surface.
//
// Revision 1.28  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
