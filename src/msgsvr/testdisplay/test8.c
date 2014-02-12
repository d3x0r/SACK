
#include <stdhdrs.h>
#include <sharemem.h>

#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>

typedef struct image_display_tag
{
	PRENDERER display;
	Image Loaded;
   struct image_display_tag *next, **me;
} IMAGE_DISPLAY, *PIMAGE_DISPLAY;

typedef struct global_tag
{
	struct {
		_32 exit : 1;
	} flags;
   _32 x, y;
   PIMAGE_DISPLAY images;
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
   //PRENDER_INTERFACE Render;
} GLOBAL;

GLOBAL g;

void CPROC DrawImage( PTRSZVAL psv, PRENDERER out )
{
   PIMAGE_DISPLAY pdi = (PIMAGE_DISPLAY)psv;
	BlotImage( GetDisplayImage( pdi->display ), pdi->Loaded, 0, 0 );
   UpdateDisplay( pdi->display );
}

int CPROC KeyHandler( PTRSZVAL psv, _32 key )
{
	if( GetKeyText( key ) == '\x1b' )
		g.flags.exit = 1;
   return 0; // nope, didn't use it.
}

void AddImage( char *name )
{
   PIMAGE_DISPLAY pdi;
	pdi = New( IMAGE_DISPLAY );

	pdi->Loaded = LoadImageFile( name );
	if( !pdi->Loaded )
	{
		Release( pdi );
      return;
	}
   pdi->display = OpenDisplaySizedAt( 0
												, pdi->Loaded->width, pdi->Loaded->height
												, g.x, g.y );
	SetRedrawHandler( pdi->display, DrawImage, (PTRSZVAL)pdi );
   DrawImage( (PTRSZVAL)pdi, pdi->display );
	SetKeyboardHandler( pdi->display, KeyHandler, 0 );
	g.x += 10;
   g.y += 10;
	if( ( pdi->next = g.images ) )
      pdi->next->me = &pdi->next;
	pdi->me = &g.images;
	g.images = pdi;
}


int main( int argc, char **argv )
{
   int i;
	SetSystemLog( SYSLOG_FILE, stdout );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	g.x = 10;
	g.y = 10;
   for( i = 1; i < argc; i++ )
	{
      AddImage( argv[i] );
	}

	while( !g.flags.exit )
		Relinquish(); // hmm what do we have to do here?
	{
		PIMAGE_DISPLAY pdi;
		pdi = g.images;
		while( pdi )
		{
			CloseDisplay( pdi->display );
			UnmakeImageFile( pdi->Loaded );
         pdi = pdi->next;
		}
	}
   return 0;
}

// $Log: test8.c,v $
// Revision 1.4  2004/03/23 22:48:47  d3x0r
// Mass changes to get test programs to compile...
//
// Revision 1.3  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
