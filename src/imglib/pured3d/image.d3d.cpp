#define IMAGE_LIBRARY_SOURCE_MAIN
#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#define IMAGE_MAIN
#define NEED_ALPHA2

#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#include <deadstart.h>
#include <d3d9.h>

// have to include image.h first to get INVERT_IMAGE definition
#include <image.h>
#include <imglib/imagestruct.h>
#include <render3d.h>

extern int bGLColorMode;

#include "local.h"
#include "blotproto.h"
#include "../image_common.h"

#ifndef __arm__

ASM_IMAGE_NAMESPACE
extern unsigned char AlphaTable[256][256];
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE

PRIORITY_PRELOAD( InitImageD3d, VIDLIB_PRELOAD_PRIORITY+1 )
{
	l.pr3di = GetRender3dInterface();
}


int LoadD3DImage( Image image, int *result )
{
	/* totally different method here */
	return 0;//nTextures++;
}

static void OnFirstDraw3d( WIDE( "@00 DirectX Image Library" ) )( PTRSZVAL psv )
{
	l.d3dActiveSurface = (struct d3dSurfaceData *)psv;
}

static PTRSZVAL OnInit3d( WIDE( "@00 DirectX Image Library" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	struct d3dSurfaceData *Surfaces = New( struct d3dSurfaceData );
	Surfaces->T_Camera = camera;
	Surfaces->identity_depth = identity_depth;
	Surfaces->aspect = aspect;
	AddLink( &l.d3dSurfaces, Surfaces );
	{
		INDEX idx;
		struct d3dSurfaceData *data;
		LIST_FORALL( l.d3dSurfaces, idx, struct d3dSurfaceData *, data )
			if( data == Surfaces )
			{
				Surfaces->index = idx;
				break;
			}
	}
	return (PTRSZVAL)Surfaces;
}

static void OnBeginDraw3d( WIDE( "@00 DirectX Image Library" ) )( PTRSZVAL psvInit, PTRANSFORM camera )
{
	l.d3dActiveSurface = (struct d3dSurfaceData *)psvInit;
	l.glImageIndex = l.d3dActiveSurface->index;
}

IDirect3DBaseTexture9 *ReloadD3DTexture( Image child_image, int option )
{
	Image image;
	if( !g_d3d_device )
		return NULL;
	for( image = child_image; image && image->pParent; image = image->pParent );

	{
		struct d3dSurfaceImageData *image_data = 
			(d3dSurfaceImageData *)GetLink( &image->Surfaces
															, l.glImageIndex );
		int d3dTexture;

		if( !image_data )
		{
			// just call this to create the data then

			MarkImageUpdated( image );
			image_data = (d3dSurfaceImageData *)GetLink( &image->Surfaces
																	 , l.glImageIndex );
		}
		//lprintf( WIDE( "Reload %p %d" ), image, option );
		// should be checked outside.
		if( !image_data->d3dTexture )
		{
			D3DLOCKED_RECT rect;
			g_d3d_device->CreateTexture( image->width, image->height, 1
				, D3DUSAGE_DYNAMIC
				, D3DFORMAT::D3DFMT_A8R8G8B8
				, D3DPOOL::D3DPOOL_DEFAULT
				, &image_data->d3tex, NULL );
			image_data->d3dTexture = image_data->d3tex;
			image_data->d3tex->LockRect(0, &rect, NULL, D3DLOCK_DISCARD);
			CopyMemory(rect.pBits, image->image, image->width*image->height*sizeof(CDATA));
			image_data->d3tex->UnlockRect(0);
		}
		image->pActiveSurface = image_data->d3dTexture;
		child_image->pActiveSurface = image->pActiveSurface;
	}
	return image->pActiveSurface;
}


void MarkImageUpdated( Image child_image )
{
	Image image;
	for( image = child_image; image && image->pParent; image = image->pParent );

	;;

	{
		INDEX idx;
		struct d3dSurfaceData *data;
		struct d3dSurfaceImageData *current_image_data = NULL;
		LIST_FORALL( l.d3dSurfaces, idx, struct d3dSurfaceData *, data )
		{
			struct d3dSurfaceImageData *image_data;
			image_data = (struct d3dSurfaceImageData *)GetLink( &image->Surfaces, idx );
			if( !image_data )
			{
				image_data = New( struct d3dSurfaceImageData );
				image_data->d3dTexture = 0;
				SetLink( &image->Surfaces, idx, image_data );
			}
			if( image_data->d3dTexture )
			{
				//glDeleteTextures( 1, &image_data->d3dTexture );
				image_data->d3dTexture = 0;
			}
			if( data == l.d3dActiveSurface )
				current_image_data = image_data;
		}
		//return current_image_data;
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
			lprintf( WIDE("blat color is out of bounds (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(") (%")_32fs WIDE(",%")_32fs WIDE(")x(%")_32f WIDE(",%")_32f WIDE(")")
					 , x, y, w, h
					 , r2.x, r2.y, r2.width, r2.height );
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
		int glDepth = 1;
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
			glDepth = 0;
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
#if 0
		if( glDepth )
			glEnable( GL_DEPTH_TEST );
		else
			glDisable( GL_DEPTH_TEST );
#endif
			LPDIRECT3DVERTEXBUFFER9 pQuadVB;

			if( g_d3d_device )
			{
			g_d3d_device->CreateVertexBuffer(sizeof( D3DVERTEX )*4,
												  D3DUSAGE_WRITEONLY,
												  D3DFVF_CUSTOMVERTEX,
												  D3DPOOL_MANAGED,
												  &pQuadVB,
												  NULL);
			D3DVERTEX* pData;
			//lock buffer (NEW)
			pQuadVB->Lock(0,sizeof(pData),(void**)&pData,0);
			//copy data to buffer (NEW)
			{
				pData[0].fX = v1[v][vRight];
				pData[0].fY = v1[v][vUp];
				pData[0].fZ = v1[v][vForward];
				pData[0].dwColor = color;
				pData[1].fX = v2[v][vRight];
				pData[1].fY = v2[v][vUp];
				pData[1].fZ = v2[v][vForward];
				pData[1].dwColor = color;
				pData[2].fX = v3[v][vRight];
				pData[2].fY = v3[v][vUp];
				pData[2].fZ = v3[v][vForward];
				pData[2].dwColor = color;
				pData[3].fX = v4[v][vRight];
				pData[3].fY = v4[v][vUp];
				pData[3].fZ = v4[v][vForward];
				pData[3].dwColor = color;
			}
			//unlock buffer (NEW)
			pQuadVB->Unlock();
			g_d3d_device->SetFVF( D3DFVF_CUSTOMVERTEX );
			g_d3d_device->SetRenderState(D3DRS_AMBIENT,RGB(255,255,255));
			g_d3d_device->SetRenderState( D3DRS_LIGHTING,false);
			g_d3d_device->SetStreamSource(0,pQuadVB,0,sizeof(D3DVERTEX));
			//draw quad (NEW)


			HRESULT br = g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
			if( br )
			{
				int tmp;
				lprintf( WIDE( "error %d" ), br );
			}
			pQuadVB->Release();
		}
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

			LPDIRECT3DVERTEXBUFFER9 pQuadVB;
			struct D3DVERTEX
{
float fX,
		fY,
		fZ;
DWORD dwColor;
};
			struct D3DXVECTOR3
{
float fX,
		fY,
		fZ;
};


			g_d3d_device->CreateVertexBuffer(sizeof( D3DVERTEX )*4,
												  D3DUSAGE_WRITEONLY,
												  D3DFVF_CUSTOMVERTEX,
												  D3DPOOL_MANAGED,
												  &pQuadVB,
												  NULL);
			D3DVERTEX* pData;
			//lock buffer (NEW)
			pQuadVB->Lock(0,sizeof(pData),(void**)&pData,0);
			//copy data to buffer (NEW)
			{
				pData[0].fX = v1[v][vRight];
				pData[0].fY = v1[v][vUp];
				pData[0].fZ = v1[v][vForward];
				pData[0].dwColor = color;
				pData[1].fX = v2[v][vRight];
				pData[1].fY = v2[v][vUp];
				pData[1].fZ = v2[v][vForward];
				pData[1].dwColor = color;
				pData[2].fX = v3[v][vRight];
				pData[2].fY = v3[v][vUp];
				pData[2].fZ = v3[v][vForward];
				pData[2].dwColor = color;
				pData[3].fX = v4[v][vRight];
				pData[3].fY = v4[v][vUp];
				pData[3].fZ = v4[v][vForward];
				pData[3].dwColor = color;
			}
			//unlock buffer (NEW)
			pQuadVB->Unlock();
			g_d3d_device->SetFVF( D3DFVF_CUSTOMVERTEX );
			g_d3d_device->SetRenderState(D3DRS_AMBIENT,RGB(255,255,255));
			g_d3d_device->SetRenderState( D3DRS_LIGHTING,false);
			//g_d3d_device->SetRenderState( D3DRS_ALPHATESTENABLE, TRUE );
			g_d3d_device->SetStreamSource(0,pQuadVB,0,sizeof(D3DVERTEX));
			//draw quad (NEW)
			g_d3d_device->DrawPrimitive(D3DPT_TRIANGLESTRIP,0,2);
			pQuadVB->Release();
#if 0
		glBegin( GL_TRIANGLE_STRIP );
		glColor4ubv( (GLubyte*)&color );
		glVertex3dv( v1[v] );	// Bottom Left Of The Texture and Quad
		glVertex3dv( v2[v] );	// Bottom Right Of The Texture and Quad
		glVertex3dv( v3[v] );	// Top Right Of The Texture and Quad
		glVertex3dv( v4[v] );	// Top Left Of The Texture and Quad
		glEnd();
#endif
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
	if( pi->flags & IF_FLAG_FINAL_RENDER )
	{
#if 0
		glBegin( GL_POINTS );
		TranslateCoord( pi, &x, &y );
		glColor4ub( RedVal(c), GreenVal(c),BlueVal(c), AlphaVal(c) );
		glVertex3f((float)x, (float)y,  0.0f);	// Bottom Left Of The Texture and Quad
		glEnd();
#endif
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
		if( pi->flags & IF_FLAG_FINAL_RENDER )
		{
#if 0
			glBegin( GL_POINTS );
			TranslateCoord( pi, &x, &y );
			glColor4ub( RedVal(c), GreenVal(c),BlueVal(c), 255/*AlphaVal(d)*/ );
			glVertex3f((float)x, (float)y,  0.0f);	// Bottom Left Of The Texture and Quad
			glEnd();
#endif
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
		if( pi->flags & IF_FLAG_FINAL_RENDER )
		{
#if 0
			glBegin( GL_POINTS );
			TranslateCoord( pi, &x, &y );
			glColor4ub( RedVal(c), GreenVal(c),BlueVal(c), AlphaVal(c) );
			glVertex3f((float)x, (float)y,  0.0f);	// Bottom Left Of The Texture and Quad
			glEnd();
#endif
		}
		else if( pi->image )
		{
			po = IMG_ADDRESS(pi,x,y);
			*po = DOALPHA2( *po, c, AlphaVal(c) );
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



#endif
IMAGE_NAMESPACE_END


// $Log: image.c,v $
// Revision 1.78  2005/05/19 23:53:15  jim
// protect blatcoloralpha from working with an image without a surface.
//
// Revision 1.28  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
