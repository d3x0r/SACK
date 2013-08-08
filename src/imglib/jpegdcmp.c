Image DecompressPicture( char *pfile )
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	FILE * infile;
   ImageFile *pif;
   int i;
   JSAMPARRAY jsa;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

   if ((infile = fopen( pfile, WIDE("rb"))) == NULL) {
	    fprintf(stderr, WIDE("can't open %s\n"), pfile);
	    exit(1);
	}
  // fseek( infile, 6, SEEK_SET );
	jpeg_stdio_src(&cinfo, infile);

	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
   pif = MakeImageFile( cinfo.image_width, cinfo.image_height );

   jsa = (JSAMPARRAY)malloc( sizeof( JSAMPROW ) * pif->height );

   for( i = 0; i < pif->height; i++ )
      jsa[i] = (JSAMPROW)(pif->image + 
                  ( ( (pif->height - i) - 1 ) * pif->width ));

   while (cinfo.output_scanline < cinfo.output_height)
      jpeg_read_scanlines( &cinfo, jsa + cinfo.output_scanline, cinfo.image_height );
   jpeg_finish_decompress( &cinfo );
   fclose( infile );

	jpeg_destroy_decompress(&cinfo);

   free( jsa );

   return pif;
}
// $Log: $
