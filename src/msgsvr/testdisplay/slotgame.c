


#define USE_IMAGE_INTERFACE g.pii
#define USE_RENDER_INTERFACE g.pdi

#include <sack_types.h>
#include <stdhdrs.h>
#include <time.h>
#include <stdlib.h>
#include <logging.h>
//#include <sharemem.h>
#include <timers.h>
#include <render.h>

#define NUM_PICS 5
#define NUM_PICS_IN_WINDOW 3
#define NUM_BLURS 20
#define NUM_REELS 3
#define NUM_ITERATIONS 15
#define NUM_IMAGES 42
#define NUM_ICONS 10
#define ITERATIONS_OFFSET 3

#define REEL_STEPX 106
#define REEL_OFSX 167
#define REEL_OFSY 114
#define REEL_WIDTH 96
#define REEL_HEIGHT 288

#define DO_NOT_SPIN 32768

typedef struct global_tag {
	struct {
		_32 bSpinning : 1;
		_32 bBackgroundInitialized : 1;
	} flags;
	_32 bReelSpinning[NUM_REELS];
	S_32 ofs;
	_32 nReels;
   Image background, playing, playagain;
	Image strip;
	Image images[NUM_IMAGES];
	Image blurs[NUM_BLURS];
   Image dodges[NUM_BLURS];
	Image reel[NUM_REELS][NUM_IMAGES];
//	Image reel[NUM_REELS + 2][NUM_PICS + 2];
	PRENDERER render;
	Image surface;
	Image subsurface[NUM_REELS];
	Image testsurface[NUM_REELS];
	Image statussurface;
   Image backgroundsurface;
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	_32 uiSpeedCounter[NUM_REELS];
   _32 uiSpeedStep[NUM_REELS];
	_32 uiStartStep[NUM_REELS];
	_32 idx[NUM_REELS];


} GLOBAL;

GLOBAL g;

INDEX GetPositionIndex( LOGICAL init, _32 reel )
{

	if( init )
		g.idx[reel]=rand()%( NUM_IMAGES - (( NUM_PICS - NUM_PICS_IN_WINDOW) ) );
   else
		g.idx[reel]++;
	if( g.idx[reel] >= ( NUM_IMAGES - (( NUM_PICS - NUM_PICS_IN_WINDOW) ) ) )
		g.idx[reel] = 0;
   return g.idx[reel];

}


void SetReelSpeedStep(void)
{
	int n;
	for( n = 0; n < NUM_REELS; n++ )
	{
		g.uiStartStep[n] =  rand()%3;//0;//rand()%10;
		g.uiSpeedStep[n] =  0;
		g.uiSpeedCounter[n] = 0;
	}
}
void DodgeEx( Image dst, Image src[] , _32 step )
#define Dodge( d, s ) DodgeEx( d, s, 1 )
{
   int y, x, row;
	for( x = 0; x < 96; x+=step)//x++ )
	{
		_32 idx;
      _32 divisor = 1;
		_8 rvals[96];
		_8 gvals[96];
		_8 bvals[96];
      _8 gain[96];
		_32 red = 0
	, green = 0
	, blue = 0, img = 0;
		idx = 0;
		for( row = 0; row < 96; row++ )
		{
			{
				CDATA pixel;
				pixel = getpixel( src[0], x, row );
				( rvals[row] = RedVal( pixel ) );
				( bvals[row] = BlueVal( pixel ) );
				( gvals[row] = GreenVal( pixel ) );
				gain[row] = ( (gvals[row] + bvals[row] + rvals[row]) / 768 ) + 1;
				divisor += gain[row] ;
				red += ( rvals[row] * gain[row] );
				blue += ( bvals[row] * gain[row] );
				green += ( gvals[row] * gain[row] );
			}
		}
		for( img = 1; img <= NUM_PICS; img++ )
		{
			for( y = 0; y < 96; y++ )
			{
				{
					CDATA pixel;
					pixel = getpixel( src[img], x, y);
					red -= rvals[idx] * gain[idx];
					blue -= bvals[idx] * gain[idx];
					green -= gvals[idx] * gain[idx];
					divisor -=  gain[idx] ;
					( rvals[idx] = RedVal( pixel ) );
					( bvals[idx] = BlueVal( pixel ) );
					( gvals[idx] = GreenVal( pixel ) );
					gain[idx] = ( ( gvals[idx] + bvals[idx] + rvals[idx]) / 768 )  + 1;
					divisor +=  gain[idx] ;
					red += ( rvals[idx] * gain[idx] );
					blue += ( bvals[idx] * gain[idx] );
					green += ( gvals[idx] * gain[idx] );
					idx++;
					if( idx >= 96 )
						idx = 0;
					plot( dst, x, y + (img-1)*96, Color( red/divisor, green/divisor, blue/divisor ) );
				}
			}
		}
	}
}


void BlurEx( Image dst, Image src[] , _32 step )
#define Blur( d, s ) BlurEx( d, s, 1 )
{
   int y, x, row;

	for( x = 0; x < 96; x+=step)//x++ )
	{
		_32 idx;
      _32 divisor = 1;
		_8 rvals[96];
		_8 gvals[96];
		_8 bvals[96];
      _8 gain[96];
		_32 red = 0
	, green = 0
	, blue = 0, img = 0;
		idx = 0;
		for( row = 0; row < 96; row++ )
		{
			{
				CDATA pixel;
				pixel = getpixel( src[0], x, row );
				( rvals[row] = RedVal( pixel ) );
				( bvals[row] = BlueVal( pixel ) );
				( gvals[row] = GreenVal( pixel ) );
				gain[row] = ( (gvals[row] + bvals[row] + rvals[row]) / 128 ) + 1;
				divisor += gain[row];
				red += ( rvals[row] * gain[row] );
				blue += ( bvals[row] * gain[row] );
				green += ( gvals[row] * gain[row] );
			}
		}
		for( img = 1; img <= NUM_PICS; img++ )
		{
			for( y = 0; y < 96; y++ )
			{
				{
					CDATA pixel;
					pixel = getpixel( src[img], x, y);
					red -= rvals[idx] * gain[idx];
					blue -= bvals[idx] * gain[idx];
					green -= gvals[idx] * gain[idx];
					divisor -= gain[idx];
					( rvals[idx] = RedVal( pixel ) );
					( bvals[idx] = BlueVal( pixel ) );
					( gvals[idx] = GreenVal( pixel ) );
					gain[idx] = ( ( gvals[idx] + bvals[idx] + rvals[idx]) / 128 )  + 1;
					divisor += gain[idx];
					red += ( rvals[idx] * gain[idx] );
					blue += ( bvals[idx] * gain[idx] );
					green += ( gvals[idx] * gain[idx] );
					idx++;
					if( idx >= 96 )
						idx = 0;
					plot( dst, x, y + (img-1)*96, Color( red/divisor, green/divisor, blue/divisor ) );
				}
			}
		}
	}
}
void DrawPosition( INDEX n, INDEX idx, INDEX x,  int yoff)
{

	BlotImageSizedTo( g.subsurface[n]
						 , g.images[idx]
						 //    															, 0, ( (-96) + g.ofs + x*96 )
						 //    															, 0, ( (-96) + iBobble[x] + x*96 )
						 , 0, ( (-96) + yoff + x*96 )
						 , 0, 0
						 , 96,  ( 96 * x)
						 );

}


void DrawReel( int nReel )
{
	int n;
	int i;
   n = nReel;
	{
		IMAGE_RECTANGLE rect;
		rect.x = REEL_OFSX + REEL_STEPX * n;
		rect.y = REEL_OFSY;
		rect.width = REEL_WIDTH;
		rect.height = REEL_HEIGHT;
//#ifndef __ARM__
		//SetImageBound( g.surface, &rect );
//#endif
//  		for( i = 0; i < NUM_PICS+2; i++ )
		for( i = 0; i < NUM_PICS; i++ )
		{
			BlotImage( g.surface, g.reel[n][i], REEL_OFSX + REEL_STEPX*n, (REEL_OFSY-96) + g.ofs + i*96 );
		}
	}
//#ifndef __ARM__
	//FixImagePosition( g.surface );
//#endif
}

void DrawReels( void )
{
//  	int n;
//  	for( n = 0; n < NUM_REELS; n++ )
//        DrawReel( n );
   UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(g.nReels-1) + REEL_WIDTH, REEL_HEIGHT );
}

void DrawSpinningReels( LOGICAL init )
{
//Real sorry about all the static variables.  It does look silly, but
//it is preferable to maintain the static-ly declared variables within this scope.
//could move them to the global, but right now it suits the task.
//Remember, this function is called 4 times a second, and it is only this function
//that uses these variables, so oh well.  Call a spade a spade....they're static to
//keep persistent data.
	int n;
	static LOGICAL bInit[NUM_REELS];  //static to maintain state.
	static _32 effectcount = 0;  //static to maintain state.
	static LOGICAL bPlaying = FALSE;  //static to maintain state.
	static int iBobble[5] = {0,-54,32,0,0}; //no need to declare this over and over 4 times a second.
   static INDEX idxBobbleCount[NUM_REELS]; //static to maintain state.
	_32 uiSmartYOffset = ( ( NUM_PICS - NUM_PICS_IN_WINDOW ) / 2 );
	static _32 bBobbling[NUM_REELS];

//  	xlprintf(LOG_NOISE)("DrawSpinningReels, init is %s   g.flags.bSpinning is %d"
//  							 , (init?"TRUE":"FALSE")
//  							 , g.flags.bSpinning
//    							 );
	for( n = 0; n < g.nReels; n++ )
	{
		if( init )
		{
			bInit[n] = TRUE;
			if( !bPlaying )
			{
				SetReelSpeedStep();
				bPlaying = TRUE;
				BlotScaledImageSizedTo( g.statussurface
											 , g.playing
											 , 0, 0
											 , 140,  68
											 );
			}
		}

		if( g.bReelSpinning[n] )
		{
         //uiStartStep is each reel's random starting order.
			if (  g.uiStartStep[n] )
			{
				g.uiStartStep[n]--;
			}
			else
			{
				{
               //uiSpeedStep demonstrates speed of reel movement (FAST, medium, and ....slllowwww). And stop.stutter.stop.
					if( g.uiSpeedStep[n] == 0 )
					{
  						BlotImageSizedTo( g.subsurface[n]
  											 , g.dodges[rand()%NUM_BLURS]
  											 , 0, 0
  											 , 0, 0, 96,  (96 * NUM_PICS_IN_WINDOW)
											 );
                  //effectcount is how long reels should be speedy-blurry, and normal-blurry.
						if( effectcount < (NUM_ITERATIONS / 2 ) )
						  effectcount++;
						else
						{
							effectcount = 0;
							g.uiSpeedStep[n]++;
						}
					}
					else if( g.uiSpeedStep[n] == 1 )
					{

						BlotImageSizedTo( g.subsurface[n]
  											 , g.blurs[rand()%NUM_BLURS]
											 ,  0 ,0
											 , 0, 0, 96,  (96 * NUM_PICS_IN_WINDOW)
											 );

						if( effectcount < (NUM_ITERATIONS / 2 ) )
						  effectcount++;
						else
						{
							effectcount = 0;
							g.uiSpeedStep[n]++;
						}
					}

					else if( g.uiSpeedStep[n] == 2 )
					{
						if(  g.uiSpeedCounter[n] == 0 )
						{
// could move this entire declaration into the global, and control the init process differently, but for now
// declaring static variable arrays suit the task.  the reason why these arrays are static is that
// state needs to be maintained every time the function is entered and exited, and these variables are
// not really global (that is, this scope is the only scope that counts).

// count[n] counts the number of times the reel has spun before it begins to stop.
// iteration[n] determines the maximum number of times the reel must spin before it begins to stop.
// last[y][x] maintains what the last pic was in that position, and is later incremented to demonstrate movement.
							static INDEX count[NUM_REELS], iteration[NUM_REELS];
							static INDEX last[NUM_REELS][NUM_PICS];

							if( bInit[n] )
							{
								int y;
								idxBobbleCount[n] = uiSmartYOffset;
                        bBobbling[n] = 0;
								for( y = 0; y < NUM_REELS; y++ )
								{
									last[y][0] = GetPositionIndex( TRUE, n );
									last[y][1] = last[y][0] + 1;
									last[y][2] = last[y][1] + 1;
									last[y][3] = last[y][2] + 1;
									last[y][4] = last[y][3] + 1;
								}

												  //this doesn't work yet....or does it?
								{

									IMAGE_RECTANGLE rect;
									rect.x = 0;//REEL_OFSX + REEL_STEPX * n;
									rect.y = 0;//REEL_OFSY;
									rect.width = REEL_WIDTH;
									rect.height = REEL_HEIGHT;
									//FixImagePosition( g.subsurface[n] );
									//SetImageBound( g.subsurface[n], &rect );
								}
								bInit[n] = FALSE;
								count[n] = 0;
								iteration[n] = (rand()%NUM_ITERATIONS) + ITERATIONS_OFFSET;
							}

							{
                        INDEX idx;
								int x;

  								for( x = uiSmartYOffset; x <  ( NUM_PICS_IN_WINDOW + uiSmartYOffset) ; x++)
  								{
									idx = last[n][x];
									DrawPosition( n, idx, x,  0);
								}
//--- This is just quality assurance.--------
//  								for( x = 0; x < NUM_PICS; x++ )
//  								{
//                               idx = last[n][x];
//  									  BlotImageSizedTo( g.testsurface[n]
//  															, g.images[idx]
//  															, 0, ( 96 * x )
//  															, 0, 0, REEL_WIDTH, ( 96 *  NUM_PICS )
//  															);
//  								}
//--- This is just quality assurance.--------

							}

							if( idxBobbleCount[n] == uiSmartYOffset )
							{
//  								xlprintf(LOG_NOISE)("Well, %d is shifting"
//  										  , n
//  										  );
								last[n][4] = last[n][3];
								last[n][3] = last[n][2];
								last[n][2] = last[n][1];
								last[n][1] = last[n][0];
								last[n][0] = GetPositionIndex( FALSE, n );
								count[n]++;
							}
							if(count[n] == (iteration[n] + 2 ) )
							{
								_32 x;
								for( x = uiSmartYOffset; x <  ( NUM_PICS_IN_WINDOW + uiSmartYOffset) ; x++)
								{
									DrawPosition( n, last[n][x], x,  iBobble[idxBobbleCount[n]]);
								}
								if( idxBobbleCount[n] >= ( NUM_PICS_IN_WINDOW + uiSmartYOffset) )
								{
                           xlprintf(LOG_NOISE)("Bobbling");
									g.uiSpeedStep[n] = DO_NOT_SPIN;
														  //g.bReelSpinning[n] = 0;
                           bBobbling[n] = 1;
									g.uiSpeedCounter[n] = 0;
                           g.ofs = -7; //might figure out if this is even necessary to maintain later.
									{
										int m;
										for( m = 0; m < NUM_REELS; m++ )
//  											if( g.bReelSpinning[m] )
											if( !bBobbling[m] )
											{
//  												xlprintf(LOG_NOISE)(" %d is * still *  spinning."
//  																		 , m
//  																		 );
												break;
											}
											else
											{
												idxBobbleCount[m] = uiSmartYOffset + 1;
											}

										if( m == NUM_REELS )
										{
											g.flags.bSpinning = 0;
											{
                                    //reset the bobbleness.
												int z;
												for( z = 0; z < NUM_REELS; z++)
												{
													g.bReelSpinning[z] = 0;
													idxBobbleCount[z] = uiSmartYOffset;
												}

											}
											if( bPlaying )
											{
												BlotScaledImageSizedTo( g.statussurface
																			 , g.playagain
																			 , 0 , 0  //490, 10
																			 , 140,  68
																			 );
												UpdateDisplay(g.render);
												bPlaying = FALSE;
											}
										}
									}
								}
								else
								{
									idxBobbleCount[n]++;
								}
							}
							else if( count[n] > iteration[n] )
							{
								g.uiSpeedCounter[n] = count[n] - iteration[n]  ;
							}
						}
						else
                     g.uiSpeedCounter[n]--;
					}
				}
			}
		}
   }
	UpdateDisplayPortion( g.render, REEL_OFSX, REEL_OFSY, REEL_STEPX*(g.nReels-1) + REEL_WIDTH,  REEL_HEIGHT   );
}

int CPROC MouseMethod( PTRSZVAL psv, S_32 x, S_32 y, _32 b )
{
	if( !g.flags.bSpinning )
	{
		if( !b  )
		{
			g.ofs = -7;
			DrawReels();

		}
		else if( b == 1 )
		{
			if( g.ofs == -7 )
			{
				int n;
				if( !g.flags.bBackgroundInitialized)
				{
					ClearImageTo( g.backgroundsurface, BASE_COLOR_BLACK );
					BlotImage( g.backgroundsurface, g.background, 0, 0 );
					g.flags.bBackgroundInitialized = 1;
				}
				for( n = 0; n < NUM_REELS; n++ )
				{
               g.bReelSpinning[n] = 1;
				}
				g.flags.bSpinning = 1;

            if( 0 )//yucky. real cheesey.
				{
					int n, i;
					for( n = 0; n < g.nReels; n++ )
						for( i = 0; i < NUM_PICS+2; i++ )
						{
							g.reel[n][i] = g.images[rand()%10];
						}
				}
				DrawSpinningReels(TRUE);
			}
		}
	}
	else
	{
		if( b )
		{
			int reel = (x - REEL_OFSX) / REEL_STEPX;
			if( reel >= 0 && reel < NUM_REELS )
			{
				if( g.bReelSpinning[reel] )
				{
					g.bReelSpinning[reel] = 0;
					g.ofs = -32;
//    					DrawReel(reel);
					UpdateDisplayPortion( g.render
											  , REEL_OFSX
											  , REEL_OFSY
											  , REEL_STEPX*(reel)
											  , REEL_WIDTH );
					g.ofs = 7;
//  					DrawReel(reel);
					UpdateDisplayPortion( g.render
											  , REEL_OFSX
											  , REEL_OFSY
											  , REEL_STEPX*(reel)
											  , REEL_WIDTH );
					WakeableSleep( 100 );
					g.ofs = 0;
//  					DrawReel(reel);
					UpdateDisplayPortion( g.render
											  , REEL_OFSX
											  , REEL_OFSY
											  , REEL_STEPX*(reel)
											  , REEL_WIDTH );
					{
						int n;
						for( n = 0; n < NUM_REELS; n++ )
							if( g.bReelSpinning[n] )
								break;
						if( n == NUM_REELS )
						{
							g.flags.bSpinning = 0;
                     SyncRender( g.render);
//  							SetReelSpeedStep();

						}
					}
				}
			}
		}
	}
   return 1;
}

PTRSZVAL CPROC ReadInput( PTHREAD thread )
{
	char buf[256];
   PTRSZVAL retval = (PTRSZVAL)(0);
	while( fgets( buf, 256, stdin ) || buf[0] == '\x1b' )
	{
		if( !g.flags.bSpinning )
		{
			MouseMethod( 0, 0, 0, 1 );
			MouseMethod( 0, 0, 0, 0 );
		}
		else
		{
			int n;
			for( n = 0; n < NUM_REELS; n++ )
			{
				if( g.bReelSpinning[n] )
				{
					MouseMethod( 0, REEL_OFSX + n * REEL_STEPX, 0, 1 );
					MouseMethod( 0, REEL_OFSX + n * REEL_STEPX, 0, 0 );
					break;
				}
			}
		}
	}
	exit(0);
	return retval;//compiler warning: control reaches end of non-void function.  Well, of course it does. you're exiting, why would you return?
}



int main( void )
{
	_32 width, height, imagecount = 0, testimagesatinitialization =0;
	Image blank;

	srand( time( NULL ) );

	for(width=0; width< NUM_REELS; width++)
	{
		g.uiSpeedCounter[width] = 0;
		g.uiSpeedStep[width] = 2;
		g.idx[width]= 0;
	}

	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

			//SetSystemLog( SYSLOG_FILE, stdout );
   SetSystemLoggingLevel( 1000 + LOG_NOISE);
	GetDisplaySize( &width, &height );;
	g.render = OpenDisplaySizedAt( 0, width, height, 0, 0 );
	UpdateDisplay(g.render);
	g.surface = GetDisplayImage( g.render );
	SetMouseHandler( g.render, MouseMethod, 0 );

//	blank = LoadImageFile( WIDE("blankimage.jpg"));
	blank = MakeImageFile(96,96);
   ClearImageTo( blank, BASE_COLOR_CYAN );
	g.playagain=LoadImageFile( WIDE("%image%/playagain.jpg"));
	g.playing  =LoadImageFile( WIDE("%image%/playing.jpg"));
   g.background = LoadImageFile( WIDE("%image%/background.jpg") );
//   g.background = blank;
	g.strip = LoadImageFile( WIDE("%image%/slot_strip.jpg") );
	g.nReels = NUM_REELS;


	{
      Image icons[NUM_ICONS];
		int n, m;
      INDEX idx;

		for( n = 0; n < NUM_ICONS; n++ )
		{
         icons[n] = MakeSubImage( g.strip, 96 * n, 0, 96, 96 );
		}
		n =  width = imagecount = height = 0;
		while(imagecount < NUM_IMAGES )
		{
			idx = rand()%NUM_ICONS;
			g.images[imagecount] = icons[idx];
			if( testimagesatinitialization )
			{
				BlotImage( g.surface, g.images[imagecount], width * 96, height * 96 );
				width++;
				if(!( width % 8 ))
				{
					width=0;
					height++;
				}
			}
         imagecount++;
  			for( m = 0; m < (( rand()%2 )  ); m++)
  			{
				g.images[imagecount] = blank;
				if( testimagesatinitialization )
				{
					BlotImage( g.surface, g.images[imagecount], width * 96, height * 96 );
					width++;
					if(!( width % 8 ))
					{
						width=0;
						height++;
					}
				}
				imagecount++;
			}

			if( testimagesatinitialization )
			{
				SyncRender( g.render);
				UpdateDisplay(g.render);
			}

		}
		if( !testimagesatinitialization )
		{
				SyncRender( g.render);
				UpdateDisplay(g.render);
		}


		for( n = 0; n < NUM_BLURS; n++ )
		{
			g.blurs[n] = MakeImageFile( 96, (NUM_PICS) * 96 );
         g.dodges[n] = MakeImageFile( 96, (NUM_PICS) * 96 );
			for( m = 0; m < NUM_IMAGES; m++ )
			{
            idx = rand()%NUM_IMAGES;
				g.reel[0][m] = g.images[idx];
			}
			Blur( g.blurs[n], g.reel[0] );
			DodgeEx( g.dodges[n], g.reel[0] , 2);
		}
		for( n = 0; n < NUM_REELS; n++)
		{
			g.subsurface[n]  = MakeSubImage( g.surface
													 , REEL_OFSX + REEL_STEPX * n
													 ,  REEL_OFSY
													 , REEL_WIDTH, (96 * NUM_PICS_IN_WINDOW) );
			g.testsurface[n] = MakeSubImage( g.surface, REEL_OFSX + REEL_STEPX * n + 480,  REEL_OFSY , REEL_WIDTH, (96 * NUM_PICS) );
		}
		g.statussurface = MakeSubImage( g.surface
												, 490, 10
												, 140,  68
												);


		g.backgroundsurface = MakeSubImage( g.surface
												, 0, 0
												, 640,  460
												);
	}
   g.flags.bBackgroundInitialized = 0;

	ThreadTo( ReadInput, 0 );

	{
		_32 start = GetTickCount();
		xlprintf(LOG_NOISE)("Started at %lu"
								 , start);
      g.ofs = 0;
		while( 1 )
		{
			if( g.flags.bSpinning )
			{
				DrawSpinningReels(FALSE);
			}

#ifndef __ARM__
         // scale to approx unit speeds..
  			WakeableSleep( 250 );
			//WakeableSleep( 33);
#endif
		}
	}
	CloseDisplay( g.render );
	UnmakeImageFile( g.strip );
   return 0;
}

