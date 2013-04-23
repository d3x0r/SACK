/*
 *  Crafted by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 * Load PNG images into an internal Image structure.
 * 
 * 
 * 
 *  consult doc/image.html
 *
 */

//#define USE_IMAGE_INTERFACE (&RealImageInterface)
#ifdef _MSC_VER 
#define PNG_INTERNAL
#endif
#if defined( __GNUC__ ) || defined( __LINUX__ ) || defined( __CYGWIN__ )
#include <zlib.h>
#include <png.h>
#else
//#include <setjmp.h>
#include <zlib.h> // include this before, and we get the types we need...
#include <png.h>
#endif

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif
#include <imglib/imagestruct.h>
#include <image.h>
#include <logging.h>
#include <sharemem.h>

//extern IMAGE_INTERFACE RealImageInterface;
extern int bGLColorMode;


typedef struct ImagePngRawData_tag
{
	// The buffer to "read" from
	_8 *r_data;
	// The buffer size
	png_size_t r_size;
}ImagePngRawData;

IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif

void ImagePngRead (png_structp png, png_bytep data, png_size_t size)
{
	ImagePngRawData *self = (ImagePngRawData *)png_get_io_ptr( png );
	if (self->r_size < size)
	{
		png_error (png, "Space Read Error");
	}
	else
	{
		memcpy (data, self->r_data, size);
		self->r_size -= size;
		self->r_data += size;
	} /* endif */
}

void NotSoFatalError( png_structp png_ptr, png_const_charp c )
{
	lprintf( WIDE("Error in PNG stuff: %s"), c );
}

#ifdef  PNG_SETJMP_SUPPORTED
// this is specific to a compiler so it needs to be non-decorated in any way.
static void my_png_longjmp_ptr(jmp_buf b, int i)
{
   longjmp( b, i );
}
#endif

ImageFile *ImagePngFile (_8 * buf, long size)
{
   ImageFile *pImage;
   png_structp png_ptr;
   png_infop info_ptr;
   png_infop end_info;
   ImagePngRawData raw;

   if( png_sig_cmp( buf, 0, size ) )
      return NULL;

   png_ptr = png_create_read_struct
             ( PNG_LIBPNG_VER_STRING, NULL,
				  NULL, NULL);
	if (!png_ptr)
		return NULL;


	png_set_error_fn( png_ptr, png_get_error_ptr( png_ptr )
						 , (png_error_ptr)NotSoFatalError, (png_error_ptr)NotSoFatalError );
	//png_ptr->error_fn = NotSoFatalError;
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_read_struct(&png_ptr,
         (png_infopp)NULL, (png_infopp)NULL);
      return NULL;
   }

   end_info = png_create_info_struct(png_ptr);
   if (!end_info)
   {
no_mem2:
      png_destroy_read_struct(&png_ptr, &info_ptr,
        (png_infopp)NULL);
      return NULL;
   }
   {
      raw.r_data = buf;
      raw.r_size = size;
      png_set_read_fn( png_ptr, &raw, (png_rw_ptr)ImagePngRead );
	}
#ifdef  PNG_SETJMP_SUPPORTED
#if (PNG_LIBPNG_VER>10400) && 0
	if (setjmp( *(png_set_longjmp_fn((png_ptr), my_png_longjmp_ptr, sizeof (jmp_buf)))
				 // png_jmpbuf(png_ptr)
				 ))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
										&end_info);
		return NULL;
	}
#else
	if (setjmp( png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
										&end_info);
		return NULL;
	}
#endif
#endif
   png_read_info (png_ptr, info_ptr);
   
   {
      // Get picture info
      png_uint_32 Width, Height;
      int bit_depth, color_type;

      png_get_IHDR (png_ptr, info_ptr, &Width, &Height, &bit_depth, &color_type,
                  NULL, NULL, NULL);

      /* this message is important to someone for some reason or another... */
      //lprintf( WIDE("Image is %ld by %ld - %d/%d\n"), Width, Height, bit_depth, color_type );

      if (bit_depth > 8)
      // tell libpng to strip 16 bit/color files down to 8 bits/color
         png_set_strip_16 (png_ptr);
      else if (bit_depth < 8)
      // Expand pictures with less than 8bpp to 8bpp
         png_set_packing (png_ptr);

      if( (color_type & PNG_COLOR_MASK_PALETTE ) )
		{
         png_set_palette_to_rgb( png_ptr );
         //Log( WIDE("At the moment only 8 bit RGB/Grey images are allowed...\n") );
         //goto no_mem2;
      }
      if( color_type & PNG_COLOR_TYPE_GRAY )
          png_set_gray_to_rgb(png_ptr);

		/* flip the RGB pixels to BGR (or RGBA to BGRA) */
      if( !bGLColorMode )
			if (color_type & PNG_COLOR_MASK_COLOR)
				png_set_bgr(png_ptr);

      if( !(color_type & PNG_COLOR_MASK_ALPHA ) )
      {
         png_set_filler(png_ptr,
                        0x00, PNG_FILLER_AFTER);
      }
   // else
   //    png_set_invert_alpha(png_ptr);
      pImage = MakeImageFile( Width, Height );
      {
         size_t rowbytes;
		 int row;
         png_bytep * const row_pointers = (png_bytep*const)Allocate( sizeof( png_bytep ) * pImage->height );

         rowbytes = png_get_rowbytes (png_ptr, info_ptr);
         if (rowbytes != pImage->pwidth*4 )
         {
            Log2(WIDE(" bytes generated and bytes allocated mismatched! %d %d\n"), rowbytes, pImage->pwidth*4 );
            if( rowbytes > pImage->pwidth * 4 )
            goto no_mem2;                        // Yuck! Something went wrong!
            Log( WIDE("We're okay as long as what it wants is less...(first number)") );
         }
         for( row = 0; row < pImage->height; row++ )
         {
#ifdef _INVERT_IMAGE
				row_pointers[row] = (png_bytep)(pImage->image + (pImage->height-row-1) * pImage->pwidth);
#else
				row_pointers[row] = (png_bytep)(pImage->image + row * pImage->pwidth);
#endif
         }
        // Read image data
        png_read_image (png_ptr, row_pointers);
        // read rest of file, and get additional chunks in info_ptr
        png_read_end (png_ptr, (png_infop)NULL);
      }
   }
   png_destroy_read_struct( &png_ptr, &info_ptr,
                            &end_info);

   return pImage;
}
#ifdef __cplusplus
}//namespace loader {
#endif

IMAGE_NAMESPACE_END


// $Log: pngimage.c,v $
// Revision 1.9  2004/06/21 07:47:13  d3x0r
// Account for newly moved structure files.
//
// Revision 1.8  2003/07/24 15:21:34  panther
// Changes to make watcom happy
//
// Revision 1.7  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
