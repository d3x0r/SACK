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

#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#include <deadstart.h>
#include <d3d9.h>

// have to include image.h first to get INVERT_IMAGE definition
#include <image.h>
#include <imglib/imagestruct.h>
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
#include "../pngimage.h"
#endif

#ifdef DO_GIF
#include "../gifimage.h"
#endif

#ifdef DO_TGA
#include "../tgaimage.h"
#endif

#ifdef DO_JPG
#include "../jpgimage.h"
#endif

#ifdef DO_BMP
#include "../bmpimage.h"
#endif
#ifdef __cplusplus
using namespace sack::image::d3d::loader;
#endif
#include "local.h"
#include "blotproto.h"


#ifndef __arm__

ASM_IMAGE_NAMESPACE
extern unsigned char AlphaTable[256][256];
ASM_IMAGE_NAMESPACE_END

IMAGE_NAMESPACE

PRIORITY_PRELOAD( InitImageD3d, VIDLIB_PRELOAD_PRIORITY+1 )
{
	l.pr3di = GetRender3dInterface();
}
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
#endif

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
			image_data = MarkImageUpdated( image );
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
			if( pImage->flags & IF_FLAG_INVERTED )
			{
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
			}
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
					Deallocate( PCOLOR, pImage->image );
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
	pImage->pElder = NULL;
	pImage->pYounger = NULL;
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
	pOrphan->pYounger = NULL; // may be undefined

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
	p->pActiveSurface = 0;
	p->Surfaces = NULL;

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
	p->Surfaces = 0;
	// assume external colors...
	p->flags |= IF_FLAG_EXTERN_COLORS/*|IF_FLAG_INVERTED*/;
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
			pImage->flags |= IF_FLAG_EXTERN_COLORS|IF_FLAG_INVERTED;
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
	tmp->flags |= IF_FLAG_OWN_DATA/*|IF_FLAG_INVERTED*/;
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
		ReloadD3DTexture( file, 0 );
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
		//lprintf( "%p adjust image %d,%d ", image, image->real_x, image->real_y );
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

struct d3dSurfaceImageData * MarkImageUpdated( Image child_image )
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
			pImage->transform = CreateNamedTransform( NULL );
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
