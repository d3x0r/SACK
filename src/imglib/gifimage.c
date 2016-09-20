/*
 * Recast by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 * Load GIF image into internal Image file, based largely on original Allegro code.
 * 
 * 
 * 
 *  consult doc/image.html
 *
 */



/*
    Copyright (C) 1998 by Jorrit Tyberghein

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
	 */
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include <math.h>
#include <time.h>

#include <sharemem.h>
#define IMAGE_LIBRARY_SOURCE

#include <imglib/imagestruct.h>
#include "image.h"   // interface to internal IMAGE.C functions...
#include "gifimage.h"

extern int bGLColorMode;

IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif


//---------------------------------------------------------------------------
#define NEXTBYTE      (*ptr++)
#define IMAGESEP      0x2c
#define GRAPHIC_EXT   0xf9
#define PLAINTEXT_EXT 0x01
#define APPLICATION_EXT 0xff
#define COMMENT_EXT   0xfe
#define START_EXTENSION 0x21
#define INTERLACEMASK 0x40
#define COLORMAPMASK  0x80

int BitOffset = 0,      /* Bit Offset of next code */
    XC = 0, YC = 0,     /* Output X and Y coords of current pixel */
    Pass = 0,        /* Used by output routine if interlaced pic */
    OutCount = 0,    /* Decompressor output 'stack count' */
    RWidth, RHeight,    /* screen dimensions */
    Width, Height,      /* image dimensions */
    LeftOfs, TopOfs,    /* image offset */
    BitsPerPixel,    /* Bits per pixel, read from GIF header */
    BytesPerScanline,      /* bytes per scanline in output raster */
    ColorMapSize,    /* number of colors */
    Background,         /* background color */
    CodeSize,        /* Code size, read from GIF header */
    InitCodeSize,    /* Starting code size, used during Clear */
    Code,         /* Value returned by ReadCode */
    MaxCode,         /* limiting value for current code size */
    ClearCode,       /* GIF clear code */
    EOFCode,         /* GIF end-of-information code */
    CurCode, OldCode, InCode, /* Decompressor variables */
    FirstFree,       /* First free code, generated per GIF spec */
    FreeCode,        /* Decompressor, next free slot in hash table*/
    FinChar,         /* Decompressor variable */
    BitMask,         /* AND mask for data size */
    ReadMask;        /* Code AND mask for current code size */

int Interlace, HasColormap;

PCOLOR   ImagePixelData;                /* The result array */

/**
 * An RGB color.
 */
typedef struct RGBcolor
{
   unsigned char blue, green, red;
} RGBcolor;

   
RGBcolor *Palette;              /* The palette that is used */
uint8_t *Raster;       /* The raster data stream, unblocked */

uint8_t used[256];
int  numused;

const char *id87 = "GIF87a";
const char *id89 = "GIF89a";

int   ReadCode (void);
//int   log2 (int);
void  AddToPixel (uint8_t);
short transparency = -1;

ImageFile *ImageGifFile (uint8_t* ptr, long filesize)
{
  ImageFile *file = NULL;
  uint8_t     *sptr = ptr;  //save pointer
  int       numcols;
  unsigned  char ch, ch1;
  uint8_t     *ptr1;
  int   i;

  /* The hash table used by the decompressor */
  static int Prefix[4096]; // please don't put these on the stack...
  static int Suffix[4096]; // please don't put these on the stack...

  /* An output array used by the decompressor */

  static int OutCode[1025]; // please don't put these on the stack...

  static int bDecoding;


  if( strncmp ((char*)ptr, "GIF87a", 6) &&
      strncmp ((char*)ptr, "GIF89a", 6) )
  {
      return file;
  }
  while( bDecoding )
	  Relinquish();
  bDecoding = 1;
  ptr += 6;

  memset( Prefix, 0, sizeof( Prefix ) );
  memset( Suffix, 0, sizeof( Suffix ) );
  memset( OutCode, 0, sizeof( OutCode ) );

  Raster = NewArray( uint8_t, filesize );
  memset( Raster, 0, filesize );

  BitOffset = 0;
  XC = YC = 0;
  Pass = 0;
  OutCount = 0;

/* Get variables from the GIF screen descriptor */

  ch           = NEXTBYTE;
  RWidth       = ch + 0x100 * NEXTBYTE; /* screen dimensions... not used. */
  ch           = NEXTBYTE;
  RHeight      = ch + 0x100 * NEXTBYTE;

  ch           = NEXTBYTE;
  HasColormap  = ((ch & COLORMAPMASK) ? TRUE : FALSE);

  BitsPerPixel = (ch & 7) + 1;
  numcols      = ColorMapSize = 1 << BitsPerPixel;
  BitMask      = ColorMapSize - 1;

  Background   = NEXTBYTE;    /* background color... not used. */
   if (NEXTBYTE)   /* supposed to be NULL */
   {
      goto cleanup;
   }

/* Read in global colormap. */

   Palette = (RGBcolor*)NewArray( RGBcolor, ColorMapSize );

   if (HasColormap)
   {
      for (i = 0; i < ColorMapSize; i++)
		{
			if( bGLColorMode )
			{
				Palette[i].blue = NEXTBYTE;
				Palette[i].green = NEXTBYTE;
				Palette[i].red = NEXTBYTE;
			}
			else
			{
				Palette[i].red = NEXTBYTE;
				Palette[i].green = NEXTBYTE;
				Palette[i].blue = NEXTBYTE;
			}
         if( !Palette[i].red &&
             !Palette[i].green &&
             !Palette[i].blue )
         {
            Palette[i].blue = 1; // ALMOST black...
         }
         used[i]  = 0;
      }

      numused = 0;
   } /* else no colormap in GIF file */

  /* look for image separator */
  for (ch = NEXTBYTE ; ch != IMAGESEP ; ch = NEXTBYTE)
  {
    i = ch;
    if (ch != START_EXTENSION)
    {
      goto cleanup;
    }

    /* handle image extensions */
    switch (ch = NEXTBYTE)
    {
      case GRAPHIC_EXT:
         ch = NEXTBYTE;
         if (ptr[0] & 0x1)
         {
            transparency = ptr[3];   /* transparent color index */
         }
         ptr += ch;
      break;
      case PLAINTEXT_EXT:
      break;
      case APPLICATION_EXT:
      break;
      case COMMENT_EXT:
      break;
      default:
        goto cleanup;
    }

    while ((ch = NEXTBYTE)) ptr += ch;
  }

/* Now read in values from the image descriptor */

  ch        = NEXTBYTE;
  LeftOfs   = ch + 0x100 * NEXTBYTE;
  ch        = NEXTBYTE;
  TopOfs    = ch + 0x100 * NEXTBYTE;
  ch        = NEXTBYTE;
  Width     = ch + 0x100 * NEXTBYTE;
  ch        = NEXTBYTE;
  Height    = ch + 0x100 * NEXTBYTE;
  Interlace = ((NEXTBYTE & INTERLACEMASK) ? TRUE : FALSE);

  // Set the dimensions which will also allocate the image data
  // buffer.
  file = MakeImageFile( Width, Height );
  ImagePixelData = file->image;

/* Note that I ignore the possible existence of a local color map.
 * I'm told there aren't many files around that use them, and the spec
 * says it's defined for future use.  This could lead to an error
 * reading some files.
 */

/* Start reading the raster data. First we get the intial code size
 * and compute decompressor constant values, based on this code size.
 */

    CodeSize  = NEXTBYTE;
    ClearCode = (1 << CodeSize);
    EOFCode   = ClearCode + 1;
    FreeCode  = FirstFree = ClearCode + 2;

/* The GIF spec has it that the code size is the code size used to
 * compute the above values is the code size given in the file, but the
 * code size used in compression/decompression is the code size given in
 * the file plus one. (thus the ++).
 */

    CodeSize++;
    InitCodeSize = CodeSize;
    MaxCode      = (1 << CodeSize);
    ReadMask     = MaxCode - 1;

/* Read the raster data.  Here we just transpose it from the GIF array
 * to the Raster array, turning it from a series of blocks into one long
 * data stream, which makes life much easier for ReadCode().
 */

    ptr1 = Raster;
    do
    {
      ch = ch1 = NEXTBYTE;
      while (ch--) *ptr1++ = NEXTBYTE;
      if ((ptr1 - Raster) > filesize)
      {
        goto cleanup;
      }
    }
    while (ch1);

    BytesPerScanline    = Width;


/* Decompress the file, continuing until you see the GIF EOF code.
 * One obvious enhancement is to add checking for corrupt files here.
 */
//TryAgain:
    Code = ReadCode ();
    while (Code != EOFCode)
    {

/* Clear code sets everything back to its initial value, then reads the
 * immediately subsequent code as uncompressed data.
 */

      if (Code == ClearCode)
      {
         CodeSize = InitCodeSize;
         MaxCode  = (1 << CodeSize);
         ReadMask = MaxCode - 1;
         FreeCode = FirstFree;
         CurCode  = OldCode = Code = ReadCode();
         FinChar  = CurCode & BitMask;
         AddToPixel (FinChar);
      }
      else
      {
/* If not a clear code, then must be data: save same as CurCode and InCode */
         CurCode = InCode = Code;

/* If greater or equal to FreeCode, not in the hash table yet;
 * repeat the last character decoded
 */

         if (CurCode >= FreeCode)
         {
            CurCode = OldCode;
            OutCode[OutCount++] = FinChar;
         }

/* Unless this code is raw data, pursue the chain pointed to by CurCode
 * through the hash table to its end; each code in the chain puts its
 * associated output code on the output queue.
 */

         while (CurCode > BitMask)
         {
            if (OutCount > 1024)
            {
               goto cleanup;
            }
            OutCode[OutCount++] = Suffix[CurCode];
            CurCode = Prefix[CurCode];
         }

/* The last code in the chain is treated as raw data. */

         FinChar             = CurCode & BitMask;
         OutCode[OutCount++] = FinChar;

/* Now we put the data out to the Output routine.
 * It's been stacked LIFO, so deal with it that way...
 */

   for (i = OutCount - 1; i >= 0; i--)
     AddToPixel (OutCode[i]);
   OutCount = 0;

/* Build the hash table on-the-fly. No table is stored in the file. */

   Prefix[FreeCode] = OldCode;
   Suffix[FreeCode] = FinChar;
   OldCode          = InCode;

/* Point to the next slot in the table.  If we exceed the current
 * MaxCode value, increment the code size unless it's already 12.  If it
 * is, do nothing: the next code decompressed better be CLEAR
 */

      FreeCode++;
      if (FreeCode >= MaxCode)
         {
             if (CodeSize < 12)
             {
               CodeSize++;
               MaxCode *= 2;
               ReadMask = (1 << CodeSize) - 1;
             }
         }
      }
      Code = ReadCode ();
    }
    if( ptr - sptr < filesize )
    {
    // trying to figure out multi-frame images...
    //   char byOutput[64];
    //   wsprintf( byOutput, WIDE(" Extra image data passed: %d\n"), filesize - (ptr - sptr) );
    //   OutputDebugString( byOutput );
//      goto TryAgain;
    }
    goto file_okay;
cleanup:
  UnmakeImageFile( file );
  file = NULL;
file_okay:
  if (Raster) { Deallocate( uint8_t*, Raster ); Raster = NULL; }
  if (Palette) { Deallocate( RGBcolor*, Palette ); Palette = NULL; }
/*
  // stack variables HAH!
  if (Prefix) { CHK (delete [] Prefix); }
  if (Suffix) { CHK (delete [] Suffix); }
  if (OutCode) { CHK (delete [] OutCode); }
 */
   if( file )
		if( file->flags & IF_FLAG_INVERTED )
	      FlipImage( file );
  bDecoding = 0;
  return file;
}

/* Fetch the next code from the raster data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit bytes, so we have to
 * maintain our location in the Raster array as a BIT Offset.  We compute
 * the byte Offset into the raster array by dividing this by 8, pick up
 * three bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it.
 */
int ReadCode (void)
{
  int RawCode, ByteOffset;

  ByteOffset = BitOffset / 8;
  RawCode    = Raster[ByteOffset] + (0x100 * Raster[ByteOffset + 1]);

  if (CodeSize >= 8)
    RawCode += (0x10000 * Raster[ByteOffset + 2]);

  RawCode  >>= (BitOffset % 8);
  BitOffset += CodeSize;

  return RawCode & ReadMask;
}

void AddToPixel (uint8_t Index)
{
   if (YC<Height)
   {
      PCOLOR p = ImagePixelData + YC * BytesPerScanline + XC;
      if( transparency > -1 && Index == transparency )
      {
         *(uint32_t*)p = 0;
      }
      else
      {
         *(uint32_t*)p = ( *(uint32_t*)(Palette+Index) & 0x00FFFFFF ) | (0xFF000000);
      }
   }

  if (!used[Index]) { used[Index]=1;  numused++; }

/* Update the X-coordinate, and if it overflows, update the Y-coordinate */

  if (++XC == Width)
  {

/* If a non-interlaced picture, just increment YC to the next scan line.
 * If it's interlaced, deal with the interlace as described in the GIF
 * spec.  Put the decoded scan line out to the screen if we haven't gone
 * past the bottom of it
 */

    XC = 0;
    if (!Interlace) YC++;
    else
    {
      switch (Pass)
      {
   case 0:
     YC += 8;
     if (YC >= Height)
     {
       Pass++;
       YC = 4;
     }
     break;
   case 1:
     YC += 8;
     if (YC >= Height)
     {
       Pass++;
       YC = 2;
     }
     break;
   case 2:
     YC += 4;
     if (YC >= Height)
     {
       Pass++;
       YC = 1;
     }
     break;
   case 3:
     YC += 2;
     break;
   default:
     break;
      }
    }
  }
}


#ifdef __cplusplus
}//namespace loader {
#endif
IMAGE_NAMESPACE_END

//---------------------------------------------------------------------------
// $Log: gifimage.c,v $
// Revision 1.7  2004/12/05 11:40:16  panther
// Fix gif image loader which had a bit of stack overflow from large static arrays allocated thereupon.
//
// Revision 1.6  2004/06/21 07:47:12  d3x0r
// Account for newly moved structure files.
//
// Revision 1.5  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
