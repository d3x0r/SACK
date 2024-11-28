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
	void* buffer = IMGVER(GetImageSurface)( pImage );
	if( pImage->flags & IF_FLAG_INVERTED ){
		buffer = (void*)(((uintptr_t)buffer)+ (png_img.height-1) * row_stride);
		row_stride = -row_stride;
	}

	png_image_finish_read( &png_img, NULL/*background*/, buffer, row_stride, NULL/*colormap*/ );

	png_image_free( &png_img );

	return pImage;
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
	//int yesno = 
	png_image_write_to_memory( &png_img
		, output
		, &outlen // 
		, FALSE // convert to 8
		, IMGVER(GetImageSurface)( pImage ) // buffer
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
