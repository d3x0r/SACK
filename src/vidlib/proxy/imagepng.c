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

#include <imglib/imagestruct.h>
#include <image.h>
#ifdef __LINUX__
#include <zlib.h>
#include <png.h>
#else
#include <zlib/zlib.h>
#include <setjmp.h>
#include <png/png.h>
#endif
#include <logging.h>
#include <sharemem.h>

//extern IMAGE_INTERFACE RealImageInterface;

typedef struct ImagePngRawData_tag
{
  // The buffer to "read" from
  _8 **r_data;
  // The buffer size
  int *r_size;
  int alloced;
}ImagePngRawData;

IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif

//--------------------------------------
static void CPROC ImagePngWrite(png_structp png,
                           png_bytep   data,
                           png_size_t  length)
{
	// add this length, data into the buffer...
   // reallcoate buffer by 4096 until complete.
	//sg_pStream->write(length, data);
#if ( PNG_LIBPNG_VER > 10405 )
	ImagePngRawData *self = (ImagePngRawData *)png_get_io_ptr( png );
#else
	ImagePngRawData *self = (ImagePngRawData *) png->io_ptr;
#endif
	if( ((*self->r_size) + length ) > self->alloced )
	{
		if( self->alloced )
		{
			self->alloced += ((length > 2048)?length:0) + 4096;
			(*self->r_data) = (_8*)Reallocate( (*self->r_data), self->alloced );
		}
		else
		{
			self->alloced += 4096;
			(*self->r_size) = 0;
			(*self->r_data) = (_8*)Allocate( self->alloced );
		}
	}
	MemCpy( (*self->r_data)+(*self->r_size), data, length );
   (*self->r_size) = (*self->r_size) + length;
}


//--------------------------------------
static void CPROC ImagePngFlush(png_structp png_ptr)
{
   // not eally needed a flush here.
   //
}



static void CPROC NotSoFatalError( png_structp png_ptr, png_const_charp c )
{
	lprintf( WIDE("Error in PNG stuff: %s"), c );
}

LOGICAL PngImageFile ( Image pImage, _8 ** buf, int *size)
{
   png_structp png_ptr;
   png_infop info_ptr;
   //png_infop end_info;
	ImagePngRawData raw;

	if( !pImage->width || !pImage->height )
      return FALSE;

   png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL,
				  NULL, NULL);
   if (!png_ptr)
		return FALSE;
	png_set_error_fn( png_ptr, (void*)NotSoFatalError //png_get_error_ptr( png_ptr )
						 , NotSoFatalError, NotSoFatalError );
	//png_ptr->error_fn = NotSoFatalError;
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
no_mem2:
      png_destroy_write_struct(&png_ptr,
         (png_infopp)NULL);
      return FALSE;
   }

   //end_info = png_create_info_struct(png_ptr);
   //if (!end_info)
   //{
   //   png_destroy_write_struct(&png_ptr, &info_ptr,
   //     (png_infopp)NULL);
   //   return NULL;
   //}
   {
      raw.r_data = buf;
		raw.r_size = size;
      raw.alloced = 0;
      png_set_write_fn( png_ptr, &raw, ImagePngWrite, ImagePngFlush );
   }

   // Set the compression level, image filters, and compression strategy...
   //png_ptr->flags        |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
   //png_ptr->zlib_strategy = Z_DEFAULT_STRATEGY;
   png_set_compression_window_bits(png_ptr, 15);
   png_set_compression_level(png_ptr, /*0-9*/ 3 ); // gzip level?
   png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

   png_set_bgr( png_ptr );
   //png_set_swap_alpha( png_ptr );
	png_set_IHDR(png_ptr, info_ptr,
					 pImage->width, pImage->height,               // the width & height
					 8, PNG_COLOR_TYPE_RGB_ALPHA, // bit_depth, color_type,
					 PNG_INTERLACE_NONE,          // no interlace
					 PNG_COMPRESSION_TYPE_BASE,   // compression type
					 PNG_FILTER_TYPE_BASE);       // filter type
   png_write_info(png_ptr, info_ptr);

	{
      int row;
		png_bytep * const row_pointers = (png_bytep*const)Allocate( sizeof( png_bytep ) * pImage->height );
		for (row=0; row< pImage->height; row++)
		{
#ifdef _INVERT_IMAGE
			row_pointers[row] = (png_bytep)(pImage->image + (pImage->height-row-1) * pImage->pwidth);
#else
		   row_pointers[row] = (png_bytep)(pImage->image + row * pImage->pwidth);
#endif
		}
		png_write_image(png_ptr, row_pointers);
      Release( row_pointers );
	}

   png_write_end(png_ptr, info_ptr);

   png_destroy_write_struct( &png_ptr, &info_ptr
                            );

   return TRUE;
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
