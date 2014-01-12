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
#if defined( USE_GLES2 )
//#include <GLES/gl.h>
#include <GLES2/gl2.h>
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


#include "local.h"
#include "shaders.h"
#include "blotproto.h"
#include "../image_common.h"



ASM_IMAGE_NAMESPACE
extern unsigned char AlphaTable[256][256];
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE

static void OnFirstDraw3d( WIDE( "@00 PUREGL Image Library" ) )( PTRSZVAL psv )
{
	GLboolean tmp;
	l.glActiveSurface = (struct glSurfaceData *)psv;

#if !defined( USE_GLES2 )
	if (GLEW_OK != glewInit() )
	{
		return;
	}
#endif
	tmp = 123;
	glGetBooleanv( GL_SHADER_COMPILER, &tmp );
	//lprintf( WIDE("Shader Compiler = %d"), tmp );
	if( !tmp )
	{
		lprintf( WIDE("No Shader Compiler") );
	}
	else
	{
		l.simple_shader = GetShader( WIDE("Simple Shader"), InitSuperSimpleShader );
		l.simple_texture_shader = GetShader( WIDE("Simple Texture"), InitSimpleTextureShader );
		l.simple_shaded_texture_shader = GetShader( WIDE("Simple Shaded Texture"), InitSimpleShadedTextureShader );
		l.simple_multi_shaded_texture_shader = GetShader( WIDE("Simple MultiShaded Texture"), InitSimpleMultiShadedTextureShader );
		//l.simple_inverse_texture_shader = GetShader( WIDE("Simple Inverse Texture"), InitSimpleShadedTextureShader );
	}
}

static PTRSZVAL OnInit3d( WIDE( "@00 PUREGL Image Library" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *pIdentity_depty, RCOORD *aspect )
{
	INDEX idx;
	struct glSurfaceData *glSurface;
	LIST_FORALL( l.glSurface, idx, struct glSurfaceData *, glSurface )
	{
		if( ( glSurface->T_Camera == camera )
         && ( glSurface->identity_depth == pIdentity_depty )
         && ( glSurface->aspect == aspect )
			&& ( glSurface->M_Projection == projection ) )
		{
			break;
		}
	}
	if( !glSurface )
	{
		glSurface = New( struct glSurfaceData );
		MemSet( glSurface, 0, sizeof( *glSurface ) );
		glSurface->M_Projection = projection;
		glSurface->T_Camera = camera;
		glSurface->identity_depth = pIdentity_depty;
		glSurface->aspect = aspect;
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
	}
	l.glActiveSurface = glSurface;

	return (PTRSZVAL)glSurface;
}

static PTRSZVAL CPROC ReleaseTexture( POINTER p, PTRSZVAL psv )
{
   Image image = (Image)p;
	struct glSurfaceData *glSurface = ((struct glSurfaceData *)psv);
   // if this image has no gl surfaces don't check it (it might make some)
	if( !image->glSurface )
      return 0;
	if( glSurface )
	{
		struct glSurfaceImageData *image_data =
			(struct glSurfaceImageData *)GetLink( &image->glSurface, glSurface->index );
		if( image_data && image_data->glIndex )
		{
			lprintf( WIDE("Release Texture %d"), image_data->glIndex );
			glDeleteTextures( 1, &image_data->glIndex );
			image_data->glIndex = 0;
		}
	}
	else
	{
		INDEX idx;
		struct glSurfaceImageData * image_data;
		LIST_FORALL( image->glSurface, idx, struct glSurfaceImageData *, image_data )
		{
			lprintf( WIDE("Release Texture %d"), image_data->glIndex );
			glDeleteTextures( 1, &image_data->glIndex );
			image_data->glIndex = 0;
		}
	}
   return 0;
}

static void ReleaseTextures( struct glSurfaceData *glSurface )
{
   ForAllInSet( ImageFile, l.Images, ReleaseTexture, (PTRSZVAL)glSurface );
}

static void OnClose3d( WIDE( "@00 PUREGL Image Library" ) )( PTRSZVAL psvInit )
{
	struct glSurfaceData *glSurface = (struct glSurfaceData *)psvInit;
	lprintf( WIDE("Should be cleaning up shaders here...") );
	CloseShaders( glSurface );
   lprintf( WIDE("and we need to release our textures; so they can be recreated") );
   ReleaseTextures( glSurface );
}

static void OnBeginDraw3d( WIDE( "@00 PUREGL Image Library" ) )( PTRSZVAL psvInit, PTRANSFORM camera )
{
	l.glActiveSurface = (struct glSurfaceData *)psvInit;
	l.glImageIndex = l.glActiveSurface->index;
	l.camera = camera;
	l.flags.projection_read = 0;
	l.flags.worldview_read = 0;
   // reset matrix settings
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
            lprintf( WIDE("texture is %d"), image_data->glIndex );
            image_data->flags.updated = 1;
			}
			if( image_data->flags.updated )
			{
				int err;
				if( option & 2 )
				{

					//glPixelTransferf(GL_RED_SCALE,0.5f);                // Scale RGB By 50%, So That We Have Only
					//glPixelTransferf(GL_GREEN_SCALE,0.5f);              // Half Intenstity
					//glPixelTransferf(GL_BLUE_SCALE,0.5f);
#ifndef USE_GLES2
					glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);  // No Wrapping, Please!
					glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);
#endif
					//glGenTextures(3, bump); 
				}
				//lprintf( WIDE( "gen text %d" ), glGetError() );
				// Create Linear Filtered Texture
				glBindTexture(GL_TEXTURE_2D, image_data->glIndex);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
				/**///glColor4ub( 255, 255, 255, 255 );
#ifdef USE_GLES2
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->real_width, image->real_height
								, 0, GL_RGBA, GL_UNSIGNED_BYTE
								, image->image );
#else
				glTexImage2D(GL_TEXTURE_2D, 0, 4, image->real_width, image->real_height
								, 0, option?GL_BGRA_EXT:GL_RGBA, GL_UNSIGNED_BYTE
								, image->image );
#endif
				if( err = glGetError() )
				{
					lprintf( WIDE( "gen text error %d" ), err );
				}
				image_data->flags.updated = 0;
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
	if( !child_image )
		return 0;
	{
		Image output_image;
		output_image = GetShadedImage( child_image, r, g, b );
		return output_image->glActiveSurface;//ReloadOpenGlTexture( child_image, option );
	}
}

//------------------------------------------


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
			}
			if( data == l.glActiveSurface )
				current_image_data = image_data;
		}
		return current_image_data;
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
		VECTOR v[2][4];
		float _color[4];
		int vi = 0;
		_color[0] = RedVal( color ) / 255.0f;
		_color[1] = GreenVal( color ) / 255.0f;
		_color[2] = BlueVal( color ) / 255.0f;
		_color[3] = AlphaVal( color ) / 255.0f;
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

		EnableShader( l.simple_shader, v[vi], _color );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	}
	else
	{
		//lprintf( WIDE("Blotting %d,%d - %d,%d"), x, y, w, h );
		// start at origin on destination....
		if( pifDest->flags & IF_FLAG_INVERTED )
			oo = 4*( (-(S_32)w) - pifDest->pwidth);     // w is how much we can copy...
      else
			oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...
		po = IMG_ADDRESS(pifDest,x,y);
		//oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...
		BlatPixels( po, oo, w, h, color );
		MarkImageUpdated( pifDest );
	}
}

void  BlatColorAlpha ( Image pifDest, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	PCDATA po;
	int  oo;
	if( !pifDest /*|| !pifDest->image */ )
	{
		lprintf( WIDE( "No dest, or no dest image." ) );
		return;
	}
	if(  (S_32)w <= 0 || (S_32)h <= 0 )
	{
		//lprintf( "BlatColorAlpha; width or height less than 0 (%" _32fs "x%"_32fs")", w, h );
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
			lprintf( WIDE("blat color is out of bounds (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(") (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(")")
				, x, y, w, h
				, r2.x, r2.y, r2.width, r2.height );
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

		_color[0] = RedVal( color ) / 255.0f;
		_color[1] = GreenVal( color ) / 255.0f;
		_color[2] = BlueVal( color ) / 255.0f;
		_color[3] = AlphaVal( color ) / 255.0f;
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

		EnableShader( GetShader( WIDE("Simple Shader"), NULL ), v[vi], _color );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
		CheckErr();
	}
	else
	{
		// start at origin on destination....
		if( pifDest->flags & IF_FLAG_INVERTED )
			y += h-1;
		po = IMG_ADDRESS(pifDest,x,y);
		oo = 4*(pifDest->pwidth - w);     // w is how much we can copy...

		BlatPixelsAlpha( po, oo, w, h, color );
		MarkImageUpdated( pifDest );
	}
}
//---------------------------------------------------------------------------

IMAGE_NAMESPACE_END
ASM_IMAGE_NAMESPACE

void CPROC cplot( Image pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotraw( Image pi, S_32 x, S_32 y, CDATA c );
void CPROC cplotalpha( Image pi, S_32 x, S_32 y, CDATA c );
CDATA CPROC cgetpixel( Image pi, S_32 x, S_32 y );

#ifdef HAS_ASSEMBLY
void CPROC asmplot( Image pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
void CPROC asmplotraw( Image pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
void CPROC asmplotalpha( Image pi, S_32 x, S_32 y, CDATA c );
void CPROC asmplotalphaMMX( Image pi, S_32 x, S_32 y, CDATA c );
#endif

#ifdef HAS_ASSEMBLY
CDATA CPROC asmgetpixel( Image pi, S_32 x, S_32 y );
#endif

//---------------------------------------------------------------------------

void CPROC cplotraw( Image pi, S_32 x, S_32 y, CDATA c )
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
		EnableShader( GetShader( WIDE("Simple Shader"), NULL ), v, _color );

      glDrawArrays( GL_POINTS, 0, 1 );
		CheckErr();
	}
	else
	{
		*IMG_ADDRESS(pi,x,y) = c;
		MarkImageUpdated( pi );
	}
}

void CPROC cplot( Image pi, S_32 x, S_32 y, CDATA c )
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
			EnableShader( GetShader( WIDE("Simple Shader"), NULL ), v, _color );

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

CDATA CPROC cgetpixel( Image pi, S_32 x, S_32 y )
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

void CPROC cplotalpha( Image pi, S_32 x, S_32 y, CDATA c )
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
			EnableShader( GetShader( WIDE("Simple Shader"), NULL ), v, _color );

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

void CPROC do_linec( Image pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_lineasm( Image pImage, S_32 x, S_32 y
               , S_32 xto, S_32 yto, CDATA color );
#endif

void CPROC do_lineAlphac( Image pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_lineAlphaasm( Image pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color );
void CPROC do_lineAlphaMMX( Image pImage, S_32 x, S_32 y
                    , S_32 xto, S_32 yto, CDATA color );
#endif

void CPROC do_lineExVc( Image pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( Image pif, S_32 x, S_32 y, int d ) );
#ifdef HAS_ASSEMBLY
void CPROC do_lineExVasm( Image pImage, S_32 x, S_32 y
                            , S_32 xto, S_32 yto, CDATA color
                            , void (*func)( Image pif, S_32 x, S_32 y, int d ) );
#endif

void CPROC do_hlinec( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_hlineasm( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#endif

void CPROC do_vlinec( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_vlineasm( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#endif

void CPROC do_hlineAlphac( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_hlineAlphaasm( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
void CPROC do_hlineAlphaMMX( Image pImage, S_32 y, S_32 xfrom, S_32 xto, CDATA color );
#endif

void CPROC do_vlineAlphac( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#ifdef HAS_ASSEMBLY
void CPROC do_vlineAlphaasm( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
void CPROC do_vlineAlphaMMX( Image pImage, S_32 x, S_32 yfrom, S_32 yto, CDATA color );
#endif

ASM_IMAGE_NAMESPACE_END
IMAGE_NAMESPACE


//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

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
		pImage->coords[0][1] = pImage->height - (pImage->height/2);
		pImage->coords[0][2] = 0;

		pImage->coords[1][0] = pImage->width - (pImage->width/2);
		pImage->coords[1][1] = pImage->height - (pImage->height/2);
		pImage->coords[1][2] = 0;

		pImage->coords[2][0] = 0 - (pImage->width/2);
		pImage->coords[2][1] = 0 - (pImage->height/2);
		pImage->coords[2][2] = 0;

		pImage->coords[3][0] = pImage->width - (pImage->width/2);
		pImage->coords[3][1] = 0 - (pImage->height/2);
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
		tmp->v_image[0][1] = tmp->y_size;
		tmp->v_image[1][0] = tmp->x_size2;
		tmp->v_image[1][1] = tmp->y_size;
		tmp->v_image[2][0] = tmp->x_size;
		tmp->v_image[2][1] = tmp->y_size2;
		tmp->v_image[3][0] = tmp->x_size2;
		tmp->v_image[3][1] = tmp->y_size2;

		EnableShader( GetShader( WIDE("Simple Texture"), NULL ), tmp->v[tmp->vi], pifSrc->glActiveSurface, tmp->v_image );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

		Deallocate( struct workspace *, tmp );
	}
}


IMAGE_NAMESPACE_END


// $Log: image.c,v $
// Revision 1.78  2005/05/19 23:53:15  jim
// protect blatcoloralpha from working with an image without a surface.
//
// Revision 1.28  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
