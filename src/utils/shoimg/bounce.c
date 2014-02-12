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

PRENDERER display;
_32 width, height;
int _ix[10], _iy[10];
int _n;
int ix, iy;
int dx, dy;
int not_first;

int wrapped;

void FillDifference( Image surface, int a, int b, int mix, CDATA color )
{
	if( !wrapped )
		return;

   //lprintf( "Filling %08x", color );
	if( _ix[a] < _ix[b] )
	{
      if( mix )
			BlatColorAlpha( surface, _ix[a], _iy[a], _ix[b]-_ix[a], imgGraphic->height, color );
      else
			BlatColor( surface, _ix[a], _iy[a], _ix[b]-_ix[a], imgGraphic->height, color );
		if( _iy[a] < _iy[b] )
		{
			// top left side...
      if( mix )
         BlatColorAlpha( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color );
            else
         BlatColor( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color );
		}
      else
		{
			// bottom left side...
			int t = _iy[a]-_iy[b];
			int bot = _iy[a] + imgGraphic->height - t;
         //lprintf( "..." );
      if( mix )
         BlatColorAlpha( surface, _ix[a], bot, imgGraphic->width, t, color );
            else
         BlatColor( surface, _ix[a], bot, imgGraphic->width, t, color );
		}
	}
	else
	{
      // right side...
		int t1 = _ix[a] - _ix[b];
		int right = _ix[a] + imgGraphic->width - t1;
      //lprintf( "Fill right side at %d (%d)", right, t1 );
      if( mix )
		BlatColorAlpha( surface, right, _iy[a], t1, imgGraphic->height, color );
            else
		BlatColor( surface, right, _iy[a], t1, imgGraphic->height, color );
		if( _iy[a] < _iy[b] )
		{
			// top left side...
      if( mix )
			BlatColorAlpha( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color );
            else
			BlatColor( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color );
		}
      else
		{
			// bottom left side...
         int t = _iy[a]-_iy[b];
         int bot = _iy[a] + imgGraphic->height - t;
         //lprintf( "..." );
			if( mix )
         BlatColorAlpha( surface, _ix[a], bot, imgGraphic->width, t, color );
            else
         BlatColor( surface, _ix[a], bot, imgGraphic->width, t, color );
		}
	}

}

void CPROC Output( PTRSZVAL psv, PRENDERER display )
{
	surface = GetDisplayImage( display );
{
   int step = 0;
	int n = _n;
	/*
	 // portion doesn't matter since we have to do the whole surface
    // beause layered windows are kinda stupid.
	int minx, maxx;
	int miny, maxy;

	minx = _ix[n];
	maxx = _ix[(n-1)<0?9:(n-1)];
	if( minx > maxx )
	{
		int tmp = minx;
		minx = maxx;
      maxx = tmp;
	}
	miny = _iy[n];
	maxy = _iy[(n-1)<0?9:(n-1)];
	if( miny > maxy )
	{
		int tmp = miny;
		miny = maxy;
      maxy = tmp;
	}
   */

	do
	{
		if( step < 5 )
		{
			int next = n+1;
			if( next == 10 )
				next = 0;
			FillDifference( surface, n, next
                        , step>1
							  , (step==0)?0xFF000000
								:(step==1)?0xc0000000
								:(step==2)?0x80000000
								:(step==3)?0x80000000
								:0x10000000
                        );
		}
      else
			BlatColorAlpha( surface, _ix[n], _iy[n], imgGraphic->width, imgGraphic->height
							  , step<9?0x10000000:0x80000000
							  );
		n++;
      step++;
		if( n == 10 )
			n = 0;
	}
   while( n != _n );
	ix += dx;
	if( ix < 0 )
	{
		ix = 0;
		dx = (rand(  ) * 12 / RAND_MAX + 1);
	}
	if( SUS_GTE( ix, int, (width-imgGraphic->width ), _32 ) )
	{
		ix = (width-imgGraphic->width )-1;
		dx = -(rand( ) * 12 / RAND_MAX + 1);
	}

	iy += dy;
	if( iy < 0 )
	{
		iy = 0;
		dy = (rand(  ) * 12 / RAND_MAX + 1);
	}
	if( SUS_GTE( iy, int, (height-imgGraphic->height ), _32 ) )
	{
		iy = (height-imgGraphic->height )-1;
		dy = -(rand( ) * 12 / RAND_MAX + 1);
	}
	BlotImageAlpha( surface, imgGraphic, ix, iy, ALPHA_TRANSPARENT );
	//UpdateDisplay( display );//, ix, iy, imgGraphic->width, imgGraphic->height  );
	_ix[_n] = ix;
	_iy[_n] = iy;
	_n++;
	if( _n == 10 )
	{
		wrapped = 1;
		_n = 0;
	}
}
	//lprintf( "Clearing image to almost black? ");
   if( !not_first )
		ClearImageTo( surface, 0xFF000000 );
BlotImageAlpha( surface, imgGraphic, ix, iy, ALPHA_TRANSPARENT );
UpdateDisplay( display );
   not_first = 1;
}

void CPROC tick( PTRSZVAL psv )
{
   Redraw( display );
}

SaneWinMain(argc, argv )
//int main( int argc, char **argv )
{
	LOGICAL SuccessOrFailure = TRUE;
	_32 w,h;
	int x = 0, y = 0;
	_64 z = 10;
	srand( GetTickCount() );
	dx = rand() * 12 / RAND_MAX + 2;
	dy = rand() * 12 / RAND_MAX + 2;
 	//SetSystemLog( SYSLOG_FILE, stdout );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	GetDisplaySize( &width, &height );
	w = width; h = height;

	for( x = 0; x < argc; x++ )
	{
		lprintf(WIDE("[%u] %s"), x, argv[x] );
	}
	y = x  = 0;

	switch( argc )
	{
	case 7:
		{
			w = IntCreateFromText( argv[2] );
			h = IntCreateFromText( argv[3] );
			x = IntCreateFromText( argv[4] );
			y = IntCreateFromText( argv[5] );
			if( strchr( argv[6] , '-' ) )
			{
            lprintf( WIDE("\n\n %s was passed as seconds parameter.  Setting it to zero, which will be sleep forever"), argv[6]);
				z = 0;
			}
			else
			{
				z = IntCreateFromText( argv[6] );
			}

		}
		break;
	case 6:
		{
			w = IntCreateFromText( argv[2] );
			h = IntCreateFromText( argv[3] );
			x = IntCreateFromText( argv[4] );
			y = IntCreateFromText( argv[5] );
		}
		break;
	case 4:
		{
			w = IntCreateFromText( argv[2] );
			h = IntCreateFromText( argv[3] );
		}
		break;
	case 2:
		{
			if( !( StrCaseCmpEx( argv[1], WIDE("--help"), 6 ) ) )
			{
				lprintf( WIDE("\n\n\tThere is no help.\n") );
				printf(WIDE("\n\n\tThere is no help. Try examining shoimg.log.\n"));
				SuccessOrFailure = FALSE;
			}
			else if( !( StrCaseCmpEx( argv[1], WIDE("--ver"), 5 ) ) )
			{
            lprintf( WIDE("\n\n\tThere is only one version.\n") );
				printf(WIDE("\n\n\tThere is only one version.\n"));
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
      _64 a = GetTickCount();
		display = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED
														  , w //width
														  , h //height
														  , x //0
														  , y //0
											 );
      AddTimer( 33, tick, 0 );
      lprintf( WIDE("Entered main at %") _64f, a );
		SetRedrawHandler( display, Output, 0 );
		UpdateDisplay( display );
		ClearImageTo( GetDisplayImage( display ), 0xFF000000 );
      MakeTopmost( display );
		//if( z )
		//{
		//	while( (( GetTickCount() - a ) / 1000 ) < z )
		//		WakeableSleep( 1000 );
		//}
		//else
		{
			while( 1 )
				WakeableSleep( 10000 );
		}
	}
	else
	{
		_32 x;
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
EndSaneWinMain()

