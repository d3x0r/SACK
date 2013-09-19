#include <sack_types.h>

#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;

GLOBAL g;


Image surface;
PSPRITE sprite;
void CPROC RedrawTwo( PTRSZVAL psvUser, PRENDERER renderer )
{
	Image image = GetDisplayImage( renderer );
   //ClearImage( image );
	Log2( WIDE("Update image two (0,0)-(%d,%d)"), image->width, image->height );
	rotate_scaled_sprite( image, sprite, 100, 0x10000, 0x10000 );
	{
		int n;
      for( n = 0; n < 600; n += 73 )
		do_line( image, 0, 0+n, 100, 100+n, Color( 255,255,255 ) );
	}
	//BlatColor( image, 0, 0, image->width, image->height, psvUser );
}



int main( char argc, char **argv )
{
	PRENDERER display;
   g.pdi = GetDisplayInterface();
   g.pii = GetImageInterface();
	surface = LoadImageFile( argv[1] );
	if( surface )
	{
      sprite = MakeSpriteImage( surface );
		display = OpenDisplaySizedAt( 0, 1280, 1024, 0/*x*/ , 0/*y*/  );
		SetRedrawHandler( display, RedrawTwo, 0/*color*/ );
		UpdateDisplay( display );
		while( 1 )
			Sleep( 10000 );
	}
}


