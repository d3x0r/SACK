/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 * Handle Loading Windows (not OS2) Bitmap Images into an Image structure.
 * 
 * 
 * 
 *  consult doc/image.html
 *
 */

#define FIX_RELEASE_COM_COLLISION
#ifndef WINVER
#define WINVER 0x500
#endif

#include <sack_types.h>
#include <stdhdrs.h>
#include <stddef.h>

#define IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE_MAIN

#include <imglib/imagestruct.h>
#include <image.h>
#include <logging.h>

extern int bGLColorMode;

#ifdef __cplusplus 
IMAGE_NAMESPACE
		namespace loader {
#endif

#if !defined( _WINGDI_H ) && !defined( _WINGDI_ ) && !defined( UNDER_CE )

typedef PREFIX_PACKED struct tagBITMAPFILEHEADER { // bmfh
   uint16_t    bfType;
   uint32_t    bfSize;
   uint16_t    bfReserved1;
   uint16_t    bfReserved2;
   uint32_t    bfOffBits;
} PACKED BITMAPFILEHEADER;

typedef PREFIX_PACKED struct tagBITMAPINFOHEADER{ // bmih
   uint32_t   biSize;
   int32_t  biWidth;
   int32_t  biHeight;
   uint16_t   biPlanes;
   uint16_t   biBitCount;
   uint32_t   biCompression;
   uint32_t   biSizeImage;
   int32_t  biXPelsPerMeter;
   int32_t  biYPelsPerMeter;
   uint32_t   biClrUsed;
   uint32_t   biClrImportant;
} PACKED BITMAPINFOHEADER;

typedef PREFIX_PACKED struct tagRGBQUAD { // rgbq
    uint8_t    rgbBlue;
    uint8_t    rgbGreen;
    uint8_t    rgbRed;
    uint8_t    rgbReserved;
} PACKED RGBQUAD;

#if 0  /* not used anymore... */
typedef PREFIX_PACKED struct tagBITMAPINFO { // bmi
   BITMAPINFOHEADER bmiHeader;
   RGBQUAD          bmiColors[1];
} PACKED BITMAPINFO;
#endif


typedef long FXPT16DOT16,*LPFXPT16DOT16;
typedef long FXPT2DOT30,*LPFXPT2DOT30;
typedef struct tagCIEXYZ {
	FXPT2DOT30 ciexyzX;
	FXPT2DOT30 ciexyzY;
	FXPT2DOT30 ciexyzZ;
} CIEXYZ,*LPCIEXYZ;
typedef struct tagCIEXYZTRIPLE {
	CIEXYZ ciexyzRed;
	CIEXYZ ciexyzGreen;
	CIEXYZ ciexyzBlue;
} CIEXYZTRIPLE,*LPCIEXYZTRIPLE;


//#if(WINVER >= 0x0400)
typedef struct {
        uint32_t        bV4Size;
        int32_t       bV4Width;
        int32_t       bV4Height;
        uint16_t        bV4Planes;
        uint16_t        bV4BitCount;
        uint32_t        bV4V4Compression;
        uint32_t        bV4SizeImage;
        int32_t       bV4XPelsPerMeter;
        int32_t       bV4YPelsPerMeter;
        uint32_t        bV4ClrUsed;
        uint32_t        bV4ClrImportant;
        uint32_t        bV4RedMask;
        uint32_t        bV4GreenMask;
        uint32_t        bV4BlueMask;
        uint32_t        bV4AlphaMask;
        uint32_t        bV4CSType;
        CIEXYZTRIPLE bV4Endpoints;
        uint32_t        bV4GammaRed;
        uint32_t        bV4GammaGreen;
        uint32_t        bV4GammaBlue;
} BITMAPV4HEADER, FAR *LPBITMAPV4HEADER, *PBITMAPV4HEADER;
//#endif /* WINVER >= 0x0400 */
#endif

/* compare _MSC_VER to some constant when you fail to compile here... and this will define missing structure */
#if UNDER_CE || ( (__WATCOMC__ <= 1250 ) || defined __LINUX__ ) && !defined( _MSC_VER ) && !defined( __CYGWIN__ ) && !defined( _WINGDI_H) && !defined( _WINGDI_ )
//#if (WINVER >= 0x0500)
//typedef long    FXPT2DOT30;
#if 0
typedef struct tagCIEXYZ {
    FXPT2DOT30  ciexyzX;
    FXPT2DOT30  ciexyzY;
    FXPT2DOT30  ciexyzZ;
} CIEXYZ;
typedef struct tagCIEXYZTRIPLE {
    CIEXYZ  ciexyzRed;
    CIEXYZ  ciexyzGreen;
    CIEXYZ  ciexyzBlue;
} CIEXYZTRIPLE;
#endif
typedef struct {
        uint32_t        bV5Size;
        int32_t       bV5Width;
        int32_t       bV5Height;
        uint16_t        bV5Planes;
        uint16_t        bV5BitCount;
        uint32_t        bV5Compression;
        uint32_t        bV5SizeImage;
        int32_t       bV5XPelsPerMeter;
        int32_t       bV5YPelsPerMeter;
        uint32_t        bV5ClrUsed;
        uint32_t        bV5ClrImportant;
        uint32_t        bV5RedMask;
        uint32_t        bV5GreenMask;
        uint32_t        bV5BlueMask;
        uint32_t        bV5AlphaMask;
        uint32_t        bV5CSType;
        CIEXYZTRIPLE bV5Endpoints;
        uint32_t        bV5GammaRed;
        uint32_t        bV5GammaGreen;
        uint32_t        bV5GammaBlue;
        uint32_t        bV5Intent;
        uint32_t        bV5ProfileData;
        uint32_t        bV5ProfileSize;
        uint32_t        bV5Reserved;
} BITMAPV5HEADER, FAR *LPBITMAPV5HEADER, *PBITMAPV5HEADER;
//#endif WINVer > 0X50

typedef PREFIX_PACKED struct tagBITMAPV5INFO { // bmi
   BITMAPINFOHEADER bmiHeader;
   RGBQUAD          bmiColors[1];
} PACKED BITMAPV5INFO;

#endif

#undef GLColor
#ifdef _OPENGL_DRIVER
#define GLColor( c )  (((c)&0xFF00FF00)|(((c)&0xFF0000)>>16)|(((c)&0x0000FF)<<16))
#else
#define GLColor(c) (c)
#endif
//BITMAPFILEHEADER bmfh;
//BITMAPINFOHEADER bmih;
//RGBQUAD          aColors[];
//BYTE             aBitmapBits[];

static ImageFile *Bitmap5ToImageFile( BITMAPV5HEADER *pbm, uint8_t* data )
{
   ImageFile *pif;
   uint32_t *dwPalette;
   uint8_t* pColor;
	int x, y, w, h;
   lprintf( WIDE( "Untested code here..." ) );
	if( pbm->bV5Height < 0 )
		pif = MakeImageFile( w = pbm->bV5Width,
								  h = -pbm->bV5Height );
	else
		pif = MakeImageFile( w = pbm->bV5Width,
								  h = pbm->bV5Height );
	Log3( WIDE("Load bitmamp image: %d by %d %d bits"), w, h, pbm->bV5BitCount );
   if( pif )
   {
      dwPalette = (uint32_t*)(((uint8_t*)pbm) + pbm->bV5Size);
      //pColor = (uint8_t*)(pbm->bmiColors + pbm->biClrUsed );
      pColor = data;
      switch( pbm->bV5BitCount )
		{
		case 32:
			if( !pbm->bV5AlphaMask )
			{
				for( y = 0; y < h; y++ )
				{
					for( x = 0; x < w; x++ )
					{
						((uint32_t*)pif->image)[y*w+x] = (*(uint32_t*)&pColor[x*4]) & 0xFFFFFF;
						((uint32_t*)pif->image)[y*w+x] |= 0xFF000000;
						if( bGLColorMode )
						{
							((uint32_t*)pif->image)[y*w+x] = GLColor( ((uint32_t*)pif->image)[y*w+x] );
						}
					}
					pColor += ( ( w * 3 ) + 3 ) & 0x7FFFFFFC;
				}
			}
			else
			{
				for( y = 0; y < h; y++ )
				{
					for( x = 0; x < w; x++ )
					{
						((uint32_t*)pif->image)[y*w+x] = (*(uint32_t*)&pColor[x*4]);
						if( bGLColorMode )
						{
							((uint32_t*)pif->image)[y*w+x] = GLColor( ((uint32_t*)pif->image)[y*w+x] );
						}
					}
					pColor += ( ( w * 3 ) + 3 ) & 0x7FFFFFFC;
				}
			}
         break;
      case 24:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
				{
					((uint32_t*)pif->image)[y*w+x] = (*(uint32_t*)&pColor[x*3]) & 0xFFFFFF;
					((uint32_t*)pif->image)[y*w+x] |= 0xFF000000;
					if( bGLColorMode )
					{
                  ((uint32_t*)pif->image)[y*w+x] = GLColor( ((uint32_t*)pif->image)[y*w+x] );
					}
				}
            pColor += ( ( w * 3 ) + 3 ) & 0x7FFFFFFC;
         }
         break;
      case 8:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               ((uint32_t*)pif->image)[y*w+x] = dwPalette[pColor[x]];
            }
            pColor += ( w + 3 ) & 0x7FFFFFFc;
         }
         break;
      case 4:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               int idx;
               if( x & 1 )
                  idx = pColor[(x/2)] & 0xF;
               else
                  idx = (pColor[(x/2)] >> 4)&0xf;
               ((uint32_t*)pif->image)[y*w+x] = dwPalette[idx];
            }
            pColor += ( ( (w+1)/2 ) + 3 ) & 0x7FFFFFFC;
         }
         break;
      case 1:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               ((uint32_t*)pif->image)[y*w+x] = dwPalette[!(!(pColor[y*(w/8)+(x/8)]&(0x80>>(x&7))))];
            }
            pColor += ( ( (w+7)/8 ) + 3 ) & 0xFFFFFc;
         }
         break;
      default:
         Log1( WIDE("Unknown BITMAP pixel format: %d\n"), pbm->bV5BitCount );
      }
   }
   return pif;
}


static ImageFile *BitmapToImageFile( BITMAPINFOHEADER *pbm, uint8_t* data )
{
   ImageFile *pif;
   uint32_t *dwPalette;
   uint8_t* pColor;
	int x, y, w, h;
	if( pbm->biSize == sizeof( BITMAPV5HEADER ) )
	{
      return Bitmap5ToImageFile( (BITMAPV5HEADER*)pbm, data );
	}
   pif = MakeImageFile( w = pbm->biWidth,
                        h = pbm->biHeight );
   /*
	lprintf( WIDE("Load bitmamp image: %d by %d %d bits (%s) (%08x=%08x)"), w, h, pbm->biBitCount
			 , bGLColorMode?"GL Color":"Native Colors"
			 , 0x12345678
           , GLColor( 0x12345678)
			 );
	*/
   if( pif )
   {
		dwPalette = (uint32_t*)(((uint8_t*)pbm) + pbm->biSize);
      if( bGLColorMode && ( pbm->biBitCount <= 8 ) )
		{
			int n;
			for( n = 0; n < ( 1 << pbm->biBitCount ); n++ )
				dwPalette[n] = GLColor( dwPalette[n] );
		}
      //pColor = (uint8_t*)(pbm->bmiColors + pbm->biClrUsed );
      pColor = data;
      switch( pbm->biBitCount )
      {
	  case 32:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
				{
					//((uint32_t*)pif->image)[y*w+x] |= 0xFF000000;
					if( bGLColorMode )
					{
						((uint32_t*)pif->image)[y*w+x] = GLColor((*(uint32_t*)&pColor[x*4]) );
					}
					else
						((uint32_t*)pif->image)[y*w+x] = (*(uint32_t*)&pColor[x*4]);
				}
            pColor += ( ( w * 4 ) );
         }
         break;
      case 24:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
				{
					((uint32_t*)pif->image)[y*w+x] = (*(uint32_t*)&pColor[x*3]) & 0xFFFFFF;
					((uint32_t*)pif->image)[y*w+x] |= 0xFF000000;
					if( bGLColorMode )
					{
                  ((uint32_t*)pif->image)[y*w+x] = GLColor( ((uint32_t*)pif->image)[y*w+x] );
					}
				}
            pColor += ( ( w * 3 ) + 3 ) & 0x7FFFFFFC;
         }
         break;
      case 8:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               ((uint32_t*)pif->image)[y*w+x] = dwPalette[pColor[x]];
            }
            pColor += ( w + 3 ) & 0x7FFFFFFc;
         }
         break;
      case 4:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               int idx;
               if( x & 1 )
                  idx = pColor[(x/2)] & 0xF;
               else
                  idx = (pColor[(x/2)] >> 4)&0xf;
               ((uint32_t*)pif->image)[y*w+x] = dwPalette[idx];
            }
            pColor += ( ( (w+1)/2 ) + 3 ) & 0x7FFFFFFC;
         }
         break;
      case 1:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               ((uint32_t*)pif->image)[y*w+x] = dwPalette[!(!(pColor[y*(w/8)+(x/8)]&(0x80>>(x&7))))];
            }
            pColor += ( ( (w+7)/8 ) + 3 ) & 0xFFFFFc;
         }
         break;
      default:
         Log1( WIDE("Unknown BITMAP pixel format: %d\n"), pbm->biBitCount );
      }
	}
	if( !( pif->flags & IF_FLAG_INVERTED ) )
      FlipImage( pif );
   return pif;
}

Image ImageBMPFile (uint8_t* ptr, uint32_t filesize)
{
   ImageFile *image;
   uint8_t *data;
   if( ptr[0] != 'B' || ptr[1] != 'M' )
      return 0;
   data = ptr + ((BITMAPFILEHEADER*)ptr)->bfOffBits;
      image = BitmapToImageFile( (BITMAPINFOHEADER*)(((BITMAPFILEHEADER*)ptr)+1), data );
   //if( image->flags & IF_FLAG_INVERTED )
	//   FlipImage( image );
   return image;
}

Image ImageRawBMPFile (uint8_t* ptr, uint32_t filesize)
{
	BITMAPINFOHEADER *bmp = (BITMAPINFOHEADER*)ptr;
   /* do some basic checking on the data... */
	switch( bmp->biSize )
	{
	case sizeof( BITMAPINFOHEADER ):
		{
			//ImageFile *image;
			//uint8_t *data;
			return BitmapToImageFile( (BITMAPINFOHEADER*)(ptr), ptr + bmp->biSize );
		  // return image;
		}
		break;
	case sizeof( BITMAPV5HEADER ):
		return Bitmap5ToImageFile( (BITMAPV5HEADER*)(ptr), ptr + bmp->biSize );
      break;
#ifndef UNDER_CE
	case sizeof( BITMAPV4HEADER ):
		//return Bitmap5ToImageFile( (BITMAPV5HEADER*)(ptr), ptr + bmp->biSize );
		break;
#endif
	}
	return NULL;
}

#ifdef __cplusplus
		}; //namespace loader {
IMAGE_NAMESPACE_END
#endif

// $Log: bmpimage.c,v $
// Revision 1.14  2005/01/27 08:20:57  panther
// These should be cleaned up soon... but they're messy and sprite acutally used them at one time.
//
// Revision 1.13  2004/06/21 07:47:08  d3x0r
// Account for newly moved structure files.
//
// Revision 1.12  2003/09/29 13:18:35  panther
// Do flip the image on non-windows... badly used flag...
//
// Revision 1.11  2003/09/29 08:53:38  panther
// Don't flip bitmaps?
//
// Revision 1.10  2003/08/20 08:07:12  panther
// some fixes to blot scaled... fixed to makefiles test projects... fixes to export containters lib funcs
//
// Revision 1.9  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
