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

#ifndef WINVER
#define WINVER 0x500
#endif

#define NO_OPEN_MACRO
#define FIX_RELEASE_COM_COLLISION
#include <sack_types.h>
#include <stdhdrs.h>
#include <stddef.h>

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
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
   _16    bfType;
   _32    bfSize;
   _16    bfReserved1;
   _16    bfReserved2;
   _32    bfOffBits;
} PACKED BITMAPFILEHEADER;

typedef PREFIX_PACKED struct tagBITMAPINFOHEADER{ // bmih
   _32   biSize;
   S_32  biWidth;
   S_32  biHeight;
   _16   biPlanes;
   _16   biBitCount;
   _32   biCompression;
   _32   biSizeImage;
   S_32  biXPelsPerMeter;
   S_32  biYPelsPerMeter;
   _32   biClrUsed;
   _32   biClrImportant;
} PACKED BITMAPINFOHEADER;

typedef PREFIX_PACKED struct tagRGBQUAD { // rgbq
    _8    rgbBlue;
    _8    rgbGreen;
    _8    rgbRed;
    _8    rgbReserved;
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
        _32        bV4Size;
        S_32       bV4Width;
        S_32       bV4Height;
        _16        bV4Planes;
        _16        bV4BitCount;
        _32        bV4V4Compression;
        _32        bV4SizeImage;
        S_32       bV4XPelsPerMeter;
        S_32       bV4YPelsPerMeter;
        _32        bV4ClrUsed;
        _32        bV4ClrImportant;
        _32        bV4RedMask;
        _32        bV4GreenMask;
        _32        bV4BlueMask;
        _32        bV4AlphaMask;
        _32        bV4CSType;
        CIEXYZTRIPLE bV4Endpoints;
        _32        bV4GammaRed;
        _32        bV4GammaGreen;
        _32        bV4GammaBlue;
} BITMAPV4HEADER, FAR *LPBITMAPV4HEADER, *PBITMAPV4HEADER;
//#endif /* WINVER >= 0x0400 */
#endif

/* compare _MSC_VER to some constant when you fail to compile here... and this will define missing structure */
#if UNDER_CE || ( (__WATCOMC__ <= 1250 ) || defined __LINUX__ ) && !defined( _MSC_VER ) && !defined( __CYGWIN__ ) && !defined( _WINGDI_H) 
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
        _32        bV5Size;
        S_32       bV5Width;
        S_32       bV5Height;
        _16        bV5Planes;
        _16        bV5BitCount;
        _32        bV5Compression;
        _32        bV5SizeImage;
        S_32       bV5XPelsPerMeter;
        S_32       bV5YPelsPerMeter;
        _32        bV5ClrUsed;
        _32        bV5ClrImportant;
        _32        bV5RedMask;
        _32        bV5GreenMask;
        _32        bV5BlueMask;
        _32        bV5AlphaMask;
        _32        bV5CSType;
        CIEXYZTRIPLE bV5Endpoints;
        _32        bV5GammaRed;
        _32        bV5GammaGreen;
        _32        bV5GammaBlue;
        _32        bV5Intent;
        _32        bV5ProfileData;
        _32        bV5ProfileSize;
        _32        bV5Reserved;
} BITMAPV5HEADER, FAR *LPBITMAPV5HEADER, *PBITMAPV5HEADER;
//#endif WINVer > 0X50

typedef PREFIX_PACKED struct tagBITMAPV5INFO { // bmi
   BITMAPINFOHEADER bmiHeader;
   RGBQUAD          bmiColors[1];
} PACKED BITMAPV5INFO;

#endif


//BITMAPFILEHEADER bmfh;
//BITMAPINFOHEADER bmih;
//RGBQUAD          aColors[];
//BYTE             aBitmapBits[];

static ImageFile *Bitmap5ToImageFile( BITMAPV5HEADER *pbm, P_8 data )
{
   ImageFile *pif;
   _32 *dwPalette;
   P_8 pColor;
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
      dwPalette = (P_32)(((P_8)pbm) + pbm->bV5Size);
      //pColor = (P_8)(pbm->bmiColors + pbm->biClrUsed );
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
						((P_32)pif->image)[y*w+x] = (*(_32*)&pColor[x*4]) & 0xFFFFFF;
						((P_32)pif->image)[y*w+x] |= 0xFF000000;
						if( bGLColorMode )
						{
							((P_32)pif->image)[y*w+x] = GLColor( ((P_32)pif->image)[y*w+x] );
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
						((P_32)pif->image)[y*w+x] = (*(_32*)&pColor[x*4]);
						if( bGLColorMode )
						{
							((P_32)pif->image)[y*w+x] = GLColor( ((P_32)pif->image)[y*w+x] );
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
					((P_32)pif->image)[y*w+x] = (*(_32*)&pColor[x*3]) & 0xFFFFFF;
					((P_32)pif->image)[y*w+x] |= 0xFF000000;
					if( bGLColorMode )
					{
                  ((P_32)pif->image)[y*w+x] = GLColor( ((P_32)pif->image)[y*w+x] );
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
               ((P_32)pif->image)[y*w+x] = dwPalette[pColor[x]];
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
               ((P_32)pif->image)[y*w+x] = dwPalette[idx];
            }
            pColor += ( ( (w+1)/2 ) + 3 ) & 0x7FFFFFFC;
         }
         break;
      case 1:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               ((P_32)pif->image)[y*w+x] = dwPalette[!(!(pColor[y*(w/8)+(x/8)]&(0x80>>(x&7))))];
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


static ImageFile *BitmapToImageFile( BITMAPINFOHEADER *pbm, P_8 data )
{
   ImageFile *pif;
   _32 *dwPalette;
   P_8 pColor;
	int x, y, w, h;
	if( pbm->biSize == sizeof( BITMAPV5HEADER ) )
	{
      return Bitmap5ToImageFile( (BITMAPV5HEADER*)pbm, data );
	}
   pif = MakeImageFile( w = pbm->biWidth,
                        h = pbm->biHeight );
   Log3( WIDE("Load bitmamp image: %d by %d %d bits"), w, h, pbm->biBitCount );
   if( pif )
   {
      dwPalette = (P_32)(((P_8)pbm) + pbm->biSize);
      //pColor = (P_8)(pbm->bmiColors + pbm->biClrUsed );
      pColor = data;
      switch( pbm->biBitCount )
      {
      case 24:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
				{
					((P_32)pif->image)[y*w+x] = (*(_32*)&pColor[x*3]) & 0xFFFFFF;
					((P_32)pif->image)[y*w+x] |= 0xFF000000;
					if( bGLColorMode )
					{
                  ((P_32)pif->image)[y*w+x] = GLColor( ((P_32)pif->image)[y*w+x] );
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
               ((P_32)pif->image)[y*w+x] = dwPalette[pColor[x]];
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
               ((P_32)pif->image)[y*w+x] = dwPalette[idx];
            }
            pColor += ( ( (w+1)/2 ) + 3 ) & 0x7FFFFFFC;
         }
         break;
      case 1:
         for( y = 0; y < h; y++ )
         {
            for( x = 0; x < w; x++ )
            {
               ((P_32)pif->image)[y*w+x] = dwPalette[!(!(pColor[y*(w/8)+(x/8)]&(0x80>>(x&7))))];
            }
            pColor += ( ( (w+7)/8 ) + 3 ) & 0xFFFFFc;
         }
         break;
      default:
         Log1( WIDE("Unknown BITMAP pixel format: %d\n"), pbm->biBitCount );
      }
   }
   return pif;
}

Image ImageBMPFile (_8* ptr, _32 filesize)
{
   ImageFile *image;
   _8 *data;
   if( ptr[0] != 'B' || ptr[1] != 'M' )
      return 0;
   data = ptr + ((BITMAPFILEHEADER*)ptr)->bfOffBits;
      image = BitmapToImageFile( (BITMAPINFOHEADER*)(((BITMAPFILEHEADER*)ptr)+1), data );
#ifndef _INVERT_IMAGE
   FlipImage( image );
#endif
   return image;
}

Image ImageRawBMPFile (_8* ptr, _32 filesize)
{
	BITMAPINFOHEADER *bmp = (BITMAPINFOHEADER*)ptr;
   /* do some basic checking on the data... */
	switch( bmp->biSize )
	{
	case sizeof( BITMAPINFOHEADER ):
		{
			//ImageFile *image;
			//_8 *data;
			return BitmapToImageFile( (BITMAPINFOHEADER*)(ptr), ptr + bmp->biSize );
#ifndef _INVERT_IMAGE
		//FlipImage( image );
#endif
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
