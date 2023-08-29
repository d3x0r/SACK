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
#define IMAGE_LIBRARY_SOURCE
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

	png_set_option(png_ptr
	              , PNG_MAXIMUM_INFLATE_WINDOW
	              , PNG_OPTION_ON);

	png_set_error_fn( png_ptr, png_get_error_ptr( png_ptr )
						 , NotSoFatalError, NotSoFatalError );
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
#if (PNG_LIBPNG_VER>10400)

#if (PNG_LIBPNG_VER < 10502)
#error This is somewhat old png lib support... (after 1.4.x and before 1.5.2)
	if (setjmp( *(png_set_longjmp_fn((png_ptr), my_png_longjmp_ptr, sizeof (jmp_buf)))
				 // png_jmpbuf(png_ptr)
				 ))
#  else
#	 ifdef PNG_SETJMP_SUPPORTED
//	if (setjmp( *(png_set_longjmp_fn((png_ptr), (png_longjmp_ptr)my_png_longjmp_ptr3, sizeof (jmp_buf)))
//				 // png_jmpbuf(png_ptr)
//				 ))
#	 else
	if( 0 )
#	 endif
	if( 0 )
#  endif
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
										&end_info);
		return NULL;
	}
#else
#error This is really old png lib support... (before 1.4.x)
	if (setjmp( png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
										&end_info);
		return NULL;
	}
#endif
	png_read_info (png_ptr, info_ptr);

	{
		// Get picture info
		png_uint_32 Width, Height;
		int bit_depth, color_type;

		png_get_IHDR (png_ptr, info_ptr, &Width, &Height, &bit_depth, &color_type
		             , NULL, NULL, NULL);

		/* this message is important to someone for some reason or another... */
		//lprintf( "Image is %ld by %ld - %d/%d\n", Width, Height, bit_depth, color_type );

		if (bit_depth > 8)
		// tell libpng to strip 16 bit/color files down to 8 bits/color
			png_set_strip_16 (png_ptr);
		else if (bit_depth < 8)
		// Expand pictures with less than 8bpp to 8bpp
			png_set_packing (png_ptr);

		if( (color_type & PNG_COLOR_MASK_PALETTE ) )
		{
			png_set_palette_to_rgb( png_ptr );
			//Log( "At the moment only 8 bit RGB/Grey images are allowed...\n" );
			//goto no_mem2;
		}

		// gray is 0
		if( color_type == PNG_COLOR_TYPE_GRAY )
			png_set_gray_to_rgb(png_ptr);

		/* flip the RGB pixels to BGR (or RGBA to BGRA) */
		if( !IMGVER(bGLColorMode) )
			if (color_type & PNG_COLOR_MASK_COLOR)
				png_set_bgr(png_ptr);

		if( !(color_type & PNG_COLOR_MASK_ALPHA ) )
		{
			//lprintf( "missing alpha in color, so add color" );
			png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
		}
		// else
		//    png_set_invert_alpha(png_ptr);
		pImage = IMGVER(MakeImageFileEx)( Width, Height DBG_SRC );
		{
			size_t rowbytes;
			int row;
			png_bytep * const row_pointers = (png_bytep*const)NewArray( png_bytep, pImage->height );
			png_read_update_info( png_ptr, info_ptr );

			rowbytes = png_get_rowbytes (png_ptr, info_ptr);

			if (rowbytes != pImage->pwidth*4 )
			{
				if( USS_GT( rowbytes, size_t, pImage->pwidth * 4, int ) )
				{				
					lprintf(" bytes generated and bytes allocated mismatched! %" _size_f "  %" _32f, rowbytes, pImage->pwidth*4 );

						goto no_mem2;                        // Yuck! Something went wrong!
					//Log( "We're okay as long as what it wants is less...(first number)" );
				}
			}

			for( row = 0; row < pImage->height; row++ )
			{
				if( pImage->flags & IF_FLAG_INVERTED )
					row_pointers[row] = (png_bytep)(pImage->image + (pImage->height-row-1) * pImage->pwidth);
				else
					row_pointers[row] = (png_bytep)(pImage->image + row * pImage->pwidth);
			}

			// Read image data
			png_read_image (png_ptr, row_pointers);
			/*
			if( !png_read_image (png_ptr, row_pointers) )
			{
				UnmakeImageFile( pImage );
				pImage = NULL;
			}
		*/
			// read rest of file, and get additional chunks in info_ptr
			png_read_end (png_ptr, (png_infop)NULL);
		}
	}
	png_destroy_read_struct( &png_ptr, &info_ptr
	                       , &end_info);

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
static void CPROC ImagePngFlush(png_structp png_ptr)
{
	// not eally needed a flush here.
	//
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
