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
	struct
	{
		BIT_FIELD bBlacking : 1;
	}flags;

   _32 tick_to_switch;
} GLOBAL;
static GLOBAL g;
Image imgGraphic;
Image surface;

PRENDERER display;
_32 width, height;
#define MAX_STEPS 16
int _ix[MAX_STEPS], _iy[MAX_STEPS];
int _n;
int ix, iy;
int dx, dy;
int wrapped ;
int not_first;


void FillDifference( Image surface, int a, int b, int mix, CDATA color )
{
   // need full history buffer...
	if( !wrapped )
		return;
	//lprintf( "Filling %08x", color );


	if( _ix[a] <= _ix[b] )
	{
		// going to left... fill right side.

      //color = SetAlpha( BASE_COLOR_RED, AlphaVal( color ) );
      if( mix )
			BlatColorAlpha( surface, _ix[a], _iy[a], _ix[b]-_ix[a], imgGraphic->height, color );
      else
			BlatColor( surface, _ix[a], _iy[a], _ix[b]-_ix[a], imgGraphic->height, color );

      // (moving down)
		if( _iy[a] <= _iy[b] )
		{
			// top left side...
			//color = SetAlpha( BASE_COLOR_BROWN, AlphaVal(color ) );
			if( mix )
				BlatColorAlpha( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color );
			else
				BlatColor( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color );
		}
      else
		{
			// bottom left side...
			int t = _iy[a]-_iy[b];
			int bot = _iy[b] + imgGraphic->height;
         //lprintf( "..." );
			//color = SetAlpha( BASE_COLOR_WHITE, AlphaVal(color ) );
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
		int right = _ix[b] + imgGraphic->width;
      //color = SetAlpha( BASE_COLOR_BLUE, AlphaVal(color ) );
		//lprintf( "Fill right side at %d (%d)", right, t1 );

      if( mix )
			BlatColorAlpha( surface, right, _iy[a], t1, imgGraphic->height, color );
		else
			BlatColor( surface, right, _iy[a], t1, imgGraphic->height, color );

		if( _iy[a] <= _iy[b] )
		{
			//color = SetAlpha( BASE_COLOR_MAGENTA, AlphaVal(color ) );
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
			int bot = _iy[b] + imgGraphic->height;
			//lprintf( "..." );
			//color = SetAlpha( BASE_COLOR_YELLOW, AlphaVal(color ) );
			if( mix )
				BlatColorAlpha( surface, _ix[a], bot, imgGraphic->width, t, color );
			else
				BlatColor( surface, _ix[a], bot, imgGraphic->width, t, color );
		}
	}

}

void MyBlatAlpha( Image surface, int x, int y, int width, int height, int alpha, int a2 )
{
	int ix, iy;
	int dx, dy;
	PCOLOR bits = surface->image;
	PCOLOR out;
   int zz = surface->width+width;
	dx = x + width;
	dy = y + height;
	// should be fast enough...
   // but... video still doesn't really work the way I'd like it ot...
	out = bits + (surface->height-y-1) * surface->width + x;
	for( iy = 0; iy < height; iy++ )
	{
		if( y + iy >= surface->height )
         return;
		for( ix = 0; ix < width; ix++ )
		{
			if( x + iy >= surface->width )
				break;
 			if( (out[0][3] = out[0][3] * alpha / a2 ) < 1 )
            out[0][3] = 1;
			out++;
		}
      out -= zz;
	}

}

void FillDifference2( Image surface, int a, int b, int mix, int color, int c2 )
{
   // need full history buffer...
	if( !wrapped )
		return;
	//lprintf( "Filling %08x", color );


	if( _ix[a] <= _ix[b] )
	{
		// going to left... fill right side.

      //color = SetAlpha( BASE_COLOR_RED, AlphaVal( color ) );
			MyBlatAlpha( surface, _ix[a], _iy[a], _ix[b]-_ix[a], imgGraphic->height, color,c2 );

      // (moving down)
		if( _iy[a] <= _iy[b] )
		{
			// top left side...
			//color = SetAlpha( BASE_COLOR_BROWN, AlphaVal(color ) );
			MyBlatAlpha( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color,c2 );
		}
      else
		{
			// bottom left side...
			int t = _iy[a]-_iy[b];
			int bot = _iy[b] + imgGraphic->height;
         //lprintf( "..." );
			//color = SetAlpha( BASE_COLOR_WHITE, AlphaVal(color ) );
			MyBlatAlpha( surface, _ix[a], bot, imgGraphic->width, t, color,c2 );
		}
	}
	else
	{
      // right side...
		int t1 = _ix[a] - _ix[b];
		int right = _ix[b] + imgGraphic->width;
      //color = SetAlpha( BASE_COLOR_BLUE, AlphaVal(color ) );
		//lprintf( "Fill right side at %d (%d)", right, t1 );

			MyBlatAlpha( surface, right, _iy[a], t1, imgGraphic->height, color,c2 );

		if( _iy[a] <= _iy[b] )
		{
			//color = SetAlpha( BASE_COLOR_MAGENTA, AlphaVal(color ) );
			// top left side...
			MyBlatAlpha( surface, _ix[a], _iy[a], imgGraphic->width, _iy[b]-_iy[a], color,c2 );
		}
		else
		{

			// bottom left side...
			int t = _iy[a]-_iy[b];
			int bot = _iy[b] + imgGraphic->height;
			//lprintf( "..." );
			//color = SetAlpha( BASE_COLOR_YELLOW, AlphaVal(color ) );
			MyBlatAlpha( surface, _ix[a], bot, imgGraphic->width, t, color,c2 );
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
			int next = n+1;
			if( next == MAX_STEPS )
				next = 0;
			if( step < (MAX_STEPS-1) )
			{
				if( step == 0 )
				{
					FillDifference( surface, n, next
									  , 0
									  , g.flags.bBlacking?BASE_COLOR_BLACK:0x01000000
									  );
				}
				else
				{
               if( g.flags.bBlacking )
						FillDifference( surface, n, next
										  , g.flags.bBlacking?1:0
										  , g.flags.bBlacking?SetAlpha(BASE_COLOR_BLACK,(0xFF)/(step)):SetAlpha(BASE_COLOR_BLACK,0xFF>>(step/2))
										  );
               else
						FillDifference2( surface, n, next
										  , g.flags.bBlacking?1:0
										  , 15, 16
										  );
				}
			}
			n++;
			step++;
			if( n == MAX_STEPS )
				n = 0;
		}
		while( n != _n );
		ix += dx;
		if( ix < 0 )
		{
			ix = 0;
			dx = (rand(  ) * 12 / RAND_MAX + 1);
		}
		if( ix >= (width-imgGraphic->width ) )
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
		if( iy >= (height-imgGraphic->height ) )
		{
			iy = (height-imgGraphic->height )-1;
			dy = -(rand( ) * 12 / RAND_MAX + 1);
		}

		// wait until full history before drawing...
		//UpdateDisplay( display );//, ix, iy, imgGraphic->width, imgGraphic->height  );
		_ix[_n] = ix;
		_iy[_n] = iy;
		_n++;
		if( _n == MAX_STEPS )
		{
			wrapped = 1;
			_n = 0;
		}
	}
	//lprintf( "Clearing image to almost black? ");
	//if( !not_first )
	//	ClearImageTo( surface, 0xFF000000 );
	if( wrapped )
		BlotImageAlpha( surface, imgGraphic, ix, iy, ALPHA_TRANSPARENT );
	UpdateDisplay( display );
	not_first = 1;
}

void CPROC tick( PTRSZVAL psv )
{
	if( !g.tick_to_switch )
		g.tick_to_switch = GetTickCount() + 60000;
	else
		if( g.tick_to_switch < GetTickCount() )
		{
			g.tick_to_switch = 0;
         g.flags.bBlacking = !g.flags.bBlacking;
		}
	Redraw( display );
}

SaneWinMain(argc, argv )
//int main( int argc, char **argv )
{
   LOGICAL SuccessOrFailure = TRUE;
	_32 w,h, x = 0, y = 0;
	_64 z = MAX_STEPS;
	srand( GetTickCount() );
   dx = rand() * 12 / RAND_MAX + 2;
   dy = rand() * 12 / RAND_MAX + 2;
 	//SetSystemLog( SYSLOG_FILE, stdout );
	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();
	GetDisplaySize( &width, &height );
   w = width; h = height;
	ix = rand() * (_64)w / RAND_MAX;
   iy = rand() * (_64)h / RAND_MAX;


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
      lprintf( WIDE("Entered main at %lld"), a );
		SetRedrawHandler( display, Output, 0 );
      g.flags.bBlacking = 1;
		UpdateDisplay( display );
		//ClearImageTo( GetDisplayImage( display ), 0xFF000000 );
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
