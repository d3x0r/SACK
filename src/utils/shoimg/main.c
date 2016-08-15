//Requires a GUI environment, so if you are building with
//__NO_GUI__=1 this will never work.

#include <stdhdrs.h>
#include <stdio.h>
#include <stdlib.h>
#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi
#include <render.h>
#include <image.h>
#include <timers.h>

typedef struct global_tag {
	PIMAGE_INTERFACE pii;
   PRENDER_INTERFACE pdi;
} GLOBAL;
static GLOBAL g;
Image imgGraphic;
Image surface;

void CPROC Output( uintptr_t psv, PRENDERER display )
{
   surface = GetDisplayImage( display );
	BlotScaledImageAlpha( surface, imgGraphic, ALPHA_TRANSPARENT );

}

int main( int argc, char **argv )
{
   LOGICAL SuccessOrFailure = TRUE;
	uint32_t width, height, w, h, x = 0, y = 0;
	uint64_t z = 10;

	SetSystemLog( SYSLOG_FILE, stdout );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	GetDisplaySize( &width, &height );
   w = width; h = height;

	for( x = 0; x < argc; x++ )
	{
		lprintf("[%u] %s", x, argv[x] );
	}
   y = x  = 0;

	switch( argc )
	{
	case 7:
		{
			w = atol( argv[2] );
			h = atol( argv[3] );
			x = atol( argv[4] );
			y = atol( argv[5] );
			if( strchr( argv[6] , '-' ) )
			{
            lprintf( WIDE("\n\n %s was passed as seconds parameter.  Setting it to zero, which will be sleep forever"), argv[6]);
				z = 0;
			}
			else
			{
				z = atol( argv[6] );
			}

		}
		break;
	case 6:
		{
			w = atol( argv[2] );
			h = atol( argv[3] );
			x = atol( argv[4] );
			y = atol( argv[5] );
		}
		break;
	case 4:
		{
			w = atol( argv[2] );
			h = atol( argv[3] );
		}
		break;
	case 2:
		{
			if( !( strnicmp( argv[1], "--help", 6 ) ) )
			{
				lprintf( WIDE("\n\n\tThere is no help.\n") );
				printf("\n\n\tThere is no help. Try examining shoimg.log.\n");
				SuccessOrFailure = FALSE;
			}
			else if( !( strnicmp( argv[1], "--ver", 5 ) ) )
			{
            lprintf( WIDE("\n\n\tThere is only one version.\n") );
				printf("\n\n\tThere is only one version.\n");
            SuccessOrFailure = FALSE;
			}
			else
			{
				lprintf( WIDE("Using default parameters, which means %s full screen for better or for worse, for ten seconds."), argv[1] );
				w = ( width -1 );
				h = ( height - 1 );
			}
		}
		break;
	case 1:
	default:
		{
			SuccessOrFailure = FALSE;
		}
      break;
	}

	if( ( SuccessOrFailure ) &&
		( !( imgGraphic = LoadImageFile( argv[1] ) ) )
	  )
	{
		lprintf( WIDE("Tried to load %s but couldn't.  Sorry it didn't work out."),argv[1] );
		SuccessOrFailure = FALSE;
	}

	if( SuccessOrFailure &&
		( !w || !h  )
	  )
	{
		lprintf( WIDE("Cannot have width ( %u ), height ( %u ).  Sorry it didn't work out.")
				 , w, h );
		SuccessOrFailure = FALSE;
      DebugBreak();
	}

   if( SuccessOrFailure &&
		( w <= width ) &&
		( h <= height ) &&
		( (w + x ) <= width  ) &&
		( (h + y ) <= height )
      )
	{
      uint64_t a = GetTickCount();
		PRENDERER display = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED
														  , w //width
														  , h //height
														  , x //0
														  , y //0
														  );
      lprintf( WIDE("Entered main at %lld"), a );
      SetRedrawHandler( display, Output, 0 );
		UpdateDisplay( display );
		if( z )
		{
			while( (( GetTickCount() - a ) / 1000 ) < z )
				WakeableSleep( 1000 );
		}
		else
		{
			while( 1 )
				WakeableSleep( 10000 );
		}
	}
	else
	{
		uint32_t x;
		lprintf( WIDE("\n\n\tUsage: %s <image> [[[<width> <height>] <x> <y>] <seconds>].  \n\t\
						  Must be width of %u by height of %u or less, controlled by Display.Config. \n\t\
						  Example:  shoimg sky.jpg  orients at default (top left) and will use default resolution (maximum) and display full screen for the default time period (ten seconds).\n\t\
						  Example:  shoimg ball.jpg 999 743 orients at default (top left) and sizes to 999 width 743 height to display for the default time period (ten seconds).\n\t\
						  Example:  shoimg bingo.jpg 1220 944 30 30 orients at 30,30 and has the effect of a 30 pixel border ( given a 1280x1024 setup ) and shows for the default time period (ten seconds).\n\t\
						  Example:  shoimg slotstrip.jpg 1004 758 10 5 25 places a 10 pixel/5 pixel border (given a 1024x768 setup) and shows for twenty five seconds.\n\t\
						  Your parameters:\n\n"), argv[0] , width, height);
		for( x = 0; x < argc; x++ )
		{
         lprintf( WIDE( "\t[Parameter %u] is %s"),x,argv[x] );
		}

	}
   return 0;
}
