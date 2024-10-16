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
#define PNG_GRACEFUL_ERRORS
#define FIX_RELEASE_COM_COLLISION
#ifdef _MSC_VER 
#define PNG_INTERNAL
#endif


#if defined( __GNUC__ ) || defined( __LINUX__ ) || defined( __CYGWIN__ )
#define PNG_STDIO_SUPPORTED
#else
#include <setjmp.h>
#endif

#include <zlib.h>
#include <png.h>

#include <stdhdrs.h>
#include <signed_unsigned_comparisons.h>
#ifndef IMAGE_LIBRARY_SOURCE
#  define IMAGE_LIBRARY_SOURCE
#endif
#include <imglib/imagestruct.h>
#include <image.h>
#include <logging.h>
#include <sharemem.h>

//extern IMAGE_INTERFACE RealImageInterface;
extern int IMGVER(bGLColorMode);
static const char *loadingPng;

typedef struct ImagePngRawData_tag
{
	// The buffer to "read" from
	uint8_t *r_data;
	// The buffer size
	size_t r_size;
  int alloced;  // need more info on the write side.
}ImagePngRawData;

IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif

static int ImagePngRead (png_structp png, png_bytep data, size_t size)
{
	ImagePngRawData *self = (ImagePngRawData *)png_get_io_ptr( png );
	if (self->r_size < size)
	{
		char msg[128];
#ifdef _MSC_VER
		_snprintf( msg, 128, "Space Read Error wanted %zd only had %zd", size, self->r_size );
#else
		snprintf( msg, 128, "Space Read Error wanted %zd only had %zd", size, self->r_size );
#endif
		png_warning(png, msg );
		return 0;
	}
	else
	{
		memcpy (data, self->r_data, size);
		self->r_size -= size;
		self->r_data += size;
	} /* endif */
	return 1;
}

static void PNGCBAPI NotSoFatalError( png_structp png_ptr, png_const_charp c )
{
	if( strcmp( c, "iCCP: known incorrect sRGB profile" ) != 0 
		&& strcmp( c, "iCCP: cHRM chunk does not match sRGB" ) != 0
		&& strcmp( c, "Interlace handling should be turned on when using png_read_image" ) != 0 )
		lprintf( "Error in PNG stuff:%s %s", loadingPng?loadingPng:"<DATA>", c );
}

// this is specific to a compiler so it needs to be non-decorated in any way.
#ifdef PNG_SETJMP_SUPPORTED
static void my_png_longjmp_ptr(jmp_buf b, int i)
{
	longjmp( b, i );
}
#endif

#if 0
static void my_png_longjmp_ptr2( png_structp b, int i )
{
  // png_longjmp( png,i );
}
#endif

#if (PNG_LIBPNG_VER >= 10502 && 0 )
static void PNGCAPI my_png_longjmp_ptr3(jmp_buf b, int i)
{
	longjmp( b, i );
}
#endif

void IMGVER(setPngImageName)( const char *filename ) {
	loadingPng = filename;
}

ImageFile *IMGVER(ImagePngFile) (uint8_t * buf, size_t size)
{
	ImageFile *pImage;
	png_image png_img;

	if( png_sig_cmp( buf, 0, size ) )
		return NULL;

	png_img.opaque = NULL;
	png_img.version = PNG_IMAGE_VERSION;

	png_image_begin_read_from_memory( &png_img, buf, size );
	png_img.format = PNG_FORMAT_FLAG_ALPHA|PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_RGBA;
	/* allocate buffer */
	uint32_t row_stride = 4 * png_img.width;
	pImage = IMGVER(MakeImageFileEx)( Width, Height DBG_SRC );
	if( pImage->flags & IF_FLAG_INVERTED ){
		buffer = (void*)(((uintptr_t)buffer)+ (png_img.height-1) * row_stride);
		row_stride = -row_stride;
	}

	png_image_finish_read( &png_img, NULL/*background*/, buffer, row_stride, NULL/*colormap*/ );

	png_image_free( &png_img );

	return pImage;
}

typedef struct ImagePngRawDataWriter_tag
{
	// The buffer to "read" from
	uint8_t **r_data;
	// The buffer size
	size_t *r_size;
	size_t alloced;  // need more info on the write side.
}ImagePngRawDataWriter;



//--------------------------------------
static int CPROC ImagePngWrite(png_structp png,
                           png_bytep   data,
                           size_t  length)
{
	// add this length, data into the buffer...
	// reallcoate buffer by 4096 until complete.
	//sg_pStream->write(length, data);
	ImagePngRawDataWriter *self = (ImagePngRawDataWriter *) png_get_io_ptr( png );
	if( ((*self->r_size) + length ) > self->alloced )
	{
		if( self->alloced )
		{
			self->alloced += ((length > 2048)?length:0) + 4096;
			(*self->r_data) = (uint8_t*)ReallocateEx( (*self->r_data), self->alloced DBG_SRC );
		}
		else
		{
			self->alloced += 4096;
			(*self->r_size) = 0;
			(*self->r_data) = NewArray( uint8_t, self->alloced );
		}
	}
	MemCpy( (*self->r_data)+(*self->r_size), data, length );
	(*self->r_size) = (*self->r_size) + length;
	return 1;
}


//--------------------------------------
static int CPROC ImagePngFlush(png_structp png_ptr)
{
	// not eally needed a flush here.
	//
	return 1;
}



static void CPROC NotSoFatalError2( png_structp png_ptr, png_const_charp c )
{
	lprintf( "Error in PNG stuff: %s", c );
}

LOGICAL IMGVER(PngImageFile) ( Image pImage, uint8_t ** buf, size_t *size)
{
	png_structp png_ptr;
	png_infop info_ptr;
	//png_infop end_info;
	ImagePngRawDataWriter raw;

	if( !pImage->width || !pImage->height )
		return FALSE;

	png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL,
				  NULL, NULL);
	if (!png_ptr)
		return FALSE;
	// this may have to be reverted for older png libraries....
	png_set_error_fn( png_ptr, png_get_error_ptr( png_ptr )
						 , NotSoFatalError2, NotSoFatalError2 );
	//png_ptr->error_fn = NotSoFatalError;
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,
			(png_infopp)NULL);
		return FALSE;
	}

	//end_info = png_create_info_struct(png_ptr);
	//if (!end_info)
	//{
	//	png_destroy_write_struct(&png_ptr, &info_ptr,
	//	  (png_infopp)NULL);
	//	return NULL;
	//}
	{
		raw.r_data = buf;
		raw.r_size = size;
		raw.alloced = 0;
		png_set_write_fn( png_ptr, &raw, ImagePngWrite, ImagePngFlush );
	}

	// Set the compression level, image filters, and compression strategy...
	//png_ptr->flags		  |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
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
		png_bytep * const row_pointers = NewArray( png_bytep, pImage->height );
		for (row=0; row< pImage->height; row++)
		{
			if( pImage->flags & IF_FLAG_INVERTED )
				row_pointers[row] = (png_bytep)(pImage->image + (pImage->height-row-1) * pImage->pwidth);
			else
				row_pointers[row] = (png_bytep)(pImage->image + row * pImage->pwidth);
		}
		png_write_image(png_ptr, row_pointers);
		Deallocate( png_bytep*, row_pointers );
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
