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
	png_img.format = PNG_FORMAT_FLAG_ALPHA|PNG_FORMAT_FLAG_COLOR|PNG_FORMAT_BGRA;
	/* allocate buffer */
	uint32_t row_stride = 4 * png_img.width;
	pImage = IMGVER(MakeImageFileEx)( png_img.width, png_img.height DBG_SRC );
	void* buffer = GetImageSurface( pImage );
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
	png_image png_img;
	MemSet( &png_img, 0, sizeof( png_img ) );
	png_img.opaque = NULL;
	png_img.version = PNG_IMAGE_VERSION;
	png_img.width = pImage->width;
	png_img.height = pImage->height;
	png_img.format = 0;
	png_img.flags = 0;
	size_t outlen;
	void *output = NewArray( uint8_t, outlen = pImage->height*pImage->pwidth );
	int yesno = png_image_write_to_memory( &png_img
		, output
		, &outlen // 
		, FALSE // convert to 8
		, GetImageSurface( pImage ) // buffer
		, pImage->pwidth // stride
		, NULL ); // colormap

	buf[0] = output;
	size[0] = outlen;
	png_image_free( &png_img );


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
