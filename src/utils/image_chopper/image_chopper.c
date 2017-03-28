
#include <stdhdrs.h>
#include <filesys.h>
#define USE_IMAGE_INTERFACE l.pii

#include <image.h>

static struct {
   PIMAGE_INTERFACE pii;
} l;

void CPROC Process( uintptr_t psvUser, CTEXTSTR file, int flags )
{
	TEXTCHAR buf[256];
   TEXTSTR dup = StrDup( file );
	TEXTSTR ext;

	if( StrStr( file, "_out" ) )
		return;

	ext = (TEXTSTR)StrRChr( dup, '.' );

	ext[0] = 0;
	snprintf( buf, 256, "%s_out.png", dup );
   printf( "input %s output %s\n", file, buf );
	{
		Image image = LoadImageFile( file );
		Image out_image = MakeImageFile( 64, 64 );
		BlotScaledImageSizedEx( out_image, image
									 , 0, 0, 64, 64
									 , 0, image->height / 4
									 , image->width/4
									 , image->height/4, 0, BLOT_COPY );
		{
			uint8_t* data;
			size_t size;
         FILE *out;
			PngImageFile( out_image, &data, &size );
			out = sack_fopen( 0, buf, "wb" );
			if( out )
			{
				fwrite( data, 1, size, out );
            fclose( out );
			}
			UnmakeImageFile( out_image );
			UnmakeImageFile( image );
		}
	}
   Deallocate( TEXTSTR, dup );
}


SaneWinMain( argc, argv )
{
	POINTER info = NULL;
   l.pii = GetImageInterface();
	while( ScanFiles( NULL, "*", &info, Process, 0, 0 ) );
}
EndSaneWinMain()

