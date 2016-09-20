/*
 * Recast by Jim Buckeyne
 *   (c)1999-2006++ Freedom Collective
 * 
 * Load JPG image into internal Image file, based largely on original Allegro code.
 * 
 * 
 * 
 *  consult doc/image.html
 *
 */


#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>

#include <setjmp.h>
#include <string.h>

#define IMAGE_LIBRARY_SOURCE

#include <imglib/imagestruct.h>
#include "jpgimage.h"

#if defined (OS_WIN32)
#if !defined (COMP_GCC) // Avoid defining "boolean" in libjpeg headers
#  define HAVE_BOOLEAN
#endif
#endif

#ifdef __CYGWIN__
#define XMD_H // lie.  This allows IN32 to be defined by jpeglib. (CYGWIN HACK)
#endif
//#define JDCT_DEFAULT JDCT_FLOAT  // use floating-point for decompression
#if defined( WIN32 ) && !defined( __GNUC__ )
#include <jpeglib.h>
#include <jerror.h>
#else
#ifdef __cplusplus
//extern "C" {
#endif
#include <jpeglib.h>
#include <jerror.h>
#ifdef __cplusplus
//}
#endif
#endif



extern int bGLColorMode;

#ifdef __cplusplus 
IMAGE_NAMESPACE
	namespace loader {
#endif


//---------------------------------------------------------------------------


/* ==== Error mgmnt ==== */

struct my_error_mgr
{
  struct jpeg_error_mgr pub;  /* "public" fields */
  jmp_buf setjmp_buffer;   /* for return to caller */
};

typedef struct my_error_mgr *my_error_ptr;

static void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;
  //DebugBreak();
  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

/* Expanded data source object for memory buffer input */
typedef struct
{
  struct jpeg_source_mgr pub; /* public fields */
  FILE *infile;         /* source stream */
  JOCTET *buffer;    /* start of buffer */
  boolean start_of_file;   /* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr *my_src_ptr;

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */
static void init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  /* We reset the empty-input-file flag for each image,
   * but we don't clear the input buffer.
   * This is correct behavior for reading a series of images from one source.
   */
  src->start_of_file = TRUE;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 * should never happen :)
 */
static boolean fill_input_buffer (j_decompress_ptr cinfo)
{
  /* no-op */ (void)cinfo;
  return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 */
static void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  if (num_bytes > 0)
  {
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
static void term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */ (void)cinfo;
}

/*
 * Prepare for input from mem buffer.
 * Leaves buffer untouched.
 */
static void jpeg_memory_src (j_decompress_ptr cinfo, char *inbfr, int len)
{
  my_src_ptr src;

  /* The source object and input buffer are made permanent so that a series
   * of JPEG images can be read from the same file by calling jpeg_stdio_src
   * only before the first one.  (If we discarded the buffer at the end of
   * one image, we'd likely lose the start of the next one.)
   * This makes it unsafe to use this manager and a different source
   * manager serially with the same JPEG object.  Caveat programmer.
   */
  if (cinfo->src == NULL)
  {
    /* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
              (size_t)sizeof(my_source_mgr));
    src = (my_src_ptr) cinfo->src;
    src->buffer = (JOCTET *) inbfr;
  }

  src = (my_src_ptr) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  src->infile = 0L;
  src->pub.bytes_in_buffer = len;      /* sets to entire file len */
  src->pub.next_input_byte = (JOCTET *)inbfr;   /* at start of buffer */
}

/* ==== Constructor ==== */

Image ImageJpgFile (uint8_t * buf, uint32_t size)
{
   ImageFile *Image;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  int row_stride;    /* physical row width in output buffer */
  JSAMPARRAY buffer;
  int bufp;
  int i;
//  RGBcolor *out;

  /* ==== Step 1: allocate and initialize JPEG decompression object */
  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  
  jerr.pub.error_exit = my_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
	  jpeg_destroy_decompress(&cinfo);
	  return NULL;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress (&cinfo);

  /* ==== Step 2: specify data source (memory buffer, in this case) */
  jpeg_memory_src (&cinfo, (char *)buf, size);

  /* ==== Step 3: read file parameters with jpeg_read_header() */
  (void) jpeg_read_header(&cinfo, TRUE);

  /* ==== Step 4: set parameters for decompression */
  // We want max quality, doesnt matter too much it can be a bit slow

  // We almost always want RGB output (no grayscale, yuv etc)
  if (cinfo.jpeg_color_space != JCS_GRAYSCALE)
    cinfo.out_color_space = JCS_RGB;

  // Recalculate output image dimensions
  jpeg_calc_output_dimensions (&cinfo);

  /* ==== Step 5: Start decompressor */

  (void) jpeg_start_decompress (&cinfo);
  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */
  Image = MakeImageFile(cinfo.output_width, cinfo.output_height);

  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;
  /* Make a one-row-high sample array that will go away when done with image */
   buffer = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, cinfo.output_height);

   // read into the buffer upside down...
   for( i = 0; i < Image->height; i++ )
	{
		if( Image->flags & IF_FLAG_INVERTED )
   	   buffer[i] = (JSAMPROW)(Image->image + 
                  ( ( (Image->height - i) - 1 ) * Image->width ));
		else
	      buffer[i] = (JSAMPROW)(Image->image + (  i * Image->width ));
	}

  /* ==== Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  bufp = 0;
   while (cinfo.output_scanline < cinfo.output_height)
      jpeg_read_scanlines( &cinfo, buffer + cinfo.output_scanline, cinfo.image_height );


  /* blah! we're in 32 bit color space - and this reads 
   * into 24 bit space - WHY oh WHY is that... ahh well
   * we need to update this */
   for( i = 0; i < Image->height; i++ )
   {
      int j;
      char *row;
      row = (char*)buffer[i];
      for( j = Image->width; j > 0; j-- )
      {
			row[j * 4 - 1] = (uint8_t)0xff;
			if( bGLColorMode )
			{
				row[j * 4 - 2] = row[j*3-1];
				row[j * 4 - 3] = row[j*3-2];
				row[j * 4 - 4] = row[j*3-3];
			}
			else
			{
				row[j * 4 - 2] = row[j*3-3];
				row[j * 4 - 3] = row[j*3-2];
				row[j * 4 - 4] = row[j*3-1];
			}
      }
   }
  /* ==== Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the buffer data source.
   */

  /* ==== Step 8: Release JPEG decompression object */
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);


  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */
  /* And we're done! */
  return Image;
}



/*
 * example.c
 *
 * This file illustrates how to use the IJG code as a subroutine library
 * to read or write JPEG image files.  You should look at this code in
 * conjunction with the documentation file libjpeg.doc.
 *
 * This code will not do anything useful as-is, but it may be helpful as a
 * skeleton for constructing routines that call the JPEG library.  
 *
 * We present these routines in the same coding style used in the JPEG code
 * (ANSI function definitions, etc); but you are of course free to code your
 * routines in a different style if you prefer.
 */

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */


/******************** JPEG COMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to feed data into the JPEG compressor.
 * We present a minimal version that does not worry about refinements such
 * as error recovery (the JPEG code will just exit() if it gets an error).
 */


/*
 * IMAGE DATA FORMATS:
 *
 * The standard input image format is a rectangular array of pixels, with
 * each pixel having the same number of "component" values (color channels).
 * Each pixel row is an array of JSAMPLEs (which typically are unsigned chars).
 * If you are working with color data, then the color values for each pixel
 * must be adjacent in the row; for example, R,G,B,R,G,B,R,G,B,... for 24-bit
 * RGB color.
 *
 * For this example, we'll assume that this data structure matches the way
 * our application has stored the image in memory, so we can just pass a
 * pointer to our image buffer.  In particular, let's say that the image is
 * RGB color and is described by:
 */

//extern JSAMPLE * image_buffer;	/* Points to large array of R,G,B-order data */
//extern int image_height;	/* Number of rows in image */
//extern int image_width;		/* Number of columns in image */


/*
 * Sample routine for JPEG compression.  We assume that the target file name
 * and a compression quality factor are passed in.
 */

LOGICAL JpgImageFile( Image image, uint8_t **buf, size_t *size, int Q )
{
  /* This struct contains the JPEG compression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   * It is possible to have several such structures, representing multiple
   * compression/decompression processes, in existence at once.  We refer
   * to any one struct (and its associated working data) as a "JPEG object".
   */
  struct jpeg_compress_struct cinfo;
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  unsigned char *outbuf;
  unsigned long outsize;
  /* More stuff */
  uint8_t* tmpbuf = NewArray( uint8_t, image->width * image->height * 3 );
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */

  {
	  int n;
	  int m = image->pwidth * image->height;
	  uint8_t* in = (uint8_t*)image->image;
	  uint8_t* out = tmpbuf;
	  for( n = 0; n < m; n++ )
	  {
			if( bGLColorMode )
			{
			  out[0] = (*in++);
			  out[1] = (*in++);
			  out[2] = (*in++);
			}
			else
			{
			  out[2] = (*in++);
			  out[1] = (*in++);
			  out[0] = (*in++);
			}
			(*in++);
			out += 3;
	  }
  }
  /* Step 1: allocate and initialize JPEG compression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG compression object. */
  jpeg_create_compress(&cinfo);

  /* Step 2: specify data destination (eg, a file) */
  /* Note: steps 2 and 3 can be done in either order. */

  /* Here we use the library-supplied code to send compressed data to a
   * stdio stream.  You can also write your own code to do something else.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */
  //jpeg_stdio_dest(&cinfo, outfile);
  outbuf = NULL;
  outsize = 0;
  jpeg_mem_dest( &cinfo, &outbuf, &outsize );
  /* Step 3: set parameters for compression */

  /* First we supply a description of the input image.
   * Four fields of the cinfo struct must be filled in:
   */
  cinfo.image_width = image->width; 	/* image width and height, in pixels */
  cinfo.image_height = image->height;
  cinfo.input_components = 3;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
  /* Now use the library's routine to set default compression parameters.
   * (You must set at least cinfo.in_color_space before calling this,
   * since the defaults depend on the source color space.)
   */
  jpeg_set_defaults(&cinfo);
  /* Now you can set any non-default parameters you wish to.
   * Here we just illustrate the use of quality (quantization table) scaling:
   */
  jpeg_set_quality(&cinfo, Q, TRUE /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */

  /* TRUE ensures that we will write a complete interchange-JPEG file.
   * Pass TRUE unless you are very sure of what you're doing.
   */
  jpeg_start_compress(&cinfo, TRUE);

  /* Step 5: while (scan lines remain to be written) */
  /*           jpeg_write_scanlines(...); */

  /* Here we use the library's state variable cinfo.next_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   * To keep things simple, we pass one scanline per call; you can pass
   * more if you wish, though.
   */
  row_stride = image->pwidth * 3;	/* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & tmpbuf[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  /* Step 6: Finish compression */

  jpeg_finish_compress(&cinfo);
  /* After finish_compress, we can close the output file. */
  //fclose(outfile);
  Deallocate( uint8_t*, tmpbuf );
  /* Step 7: release JPEG compression object */
  (*buf) = outbuf;
  (*size) = outsize;
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&cinfo);
  return TRUE;
  /* And we're done! */
}




#ifdef __cplusplus
	}//	namespace loader {
IMAGE_NAMESPACE_END
#endif

// $Log: jpgimage.c,v $
// Revision 1.7  2003/05/18 16:22:13  panther
// Remove carriage returns, include headers more correctly
//
// Revision 1.6  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
