#include <stdhdrs.h>
#include <configscript.h>

#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <image.h>


IMAGE_NAMESPACE
#ifdef __cplusplus
namespace loader {
#endif
extern LOGICAL PngImageFile ( Image pImage, _8 ** buf, int *size);
#ifdef __cplusplus
};
#endif
IMAGE_NAMESPACE_END
#ifdef __cplusplus
using namespace sack::image::loader;
#endif
int main( int argc,  char **argv )
{
	//printf( "content-type:plain/text\r\n\r\n" );
   if( argc > 5 )
	{
		const char *file = argv[1];
      PTEXT red = SegCreateFromText( argv[2] );
      PTEXT green = SegCreateFromText( argv[3] );
		PTEXT blue = SegCreateFromText( argv[4] );
      int x, y, width,height;
		CDATA cred, cblue, cgreen;
		if( file && red && blue && green )
		{
			Image image = LoadImageFile( file );
			if( argc > 9 )
			{
				x = IntCreateFromText( argv[6] );
				y = IntCreateFromText( argv[7] );
				width = IntCreateFromText( argv[8] );
				height = IntCreateFromText( argv[9] );
			}
			else
			{
				x = 0;
				y = 0;
				width = image->width;
            height = image->height;
			}
			if( image )
			{
				Image out = MakeImageFile( width, height );
            lprintf("%s %s %s", GetText( red ), GetText( green) , GetText( blue ) );
				if( !GetColorVar( &red, &cred ) )
               lprintf( "FAIL RED" );
				if( !GetColorVar( &blue, &cblue ) )
					lprintf( "FAIL BLUE" );
				if( !GetColorVar( &green, &cgreen ) )
					lprintf( "FAIL gREEN" );

				ClearImage( out );
				//lprintf( "uhmm... %08x %08x %08x", cred, cgreen, cblue );
				BlotImageSizedEx( out, image, 0, 0, x, y, width, height, ALPHA_TRANSPARENT, BLOT_MULTISHADE, cred, cgreen, cblue );

				{
					int size;
					_8 *buf;
					if( PngImageFile( out, &buf, &size ) )
					{
						FILE *output = fopen( argv[5], "wb" );
						fwrite( buf, 1, size, output );
					}
				}
			}
		}
	}
	else
	{
		fprintf( stderr, "%s [input image] [red] [green] [blue] [out image] <x> <y> <width> <height>\n", argv[0] );
		fprintf( stderr, " [...] arguments are not optional.\n" );
      fprintf( stderr, " <...> arguments must all be specified else all are ignored\n" );
	}




	return 0;
}
