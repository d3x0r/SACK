



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

#define NUM_PICS REEL_LENGTH
#define NUM_PICS_IN_WINDOW 3
#define NUM_BLURS 1
#define NUM_REELS 3
#define NUM_ITERATIONS 15
#define REEL_LENGTH 173
#define NUM_ICONS 10
#define ITERATIONS_OFFSET 3

#define REEL_STEPX 106
#define REEL_OFSX 167
#define REEL_OFSY 114
#define REEL_WIDTH 96
#define REEL_HEIGHT 288

#define DO_NOT_SPIN 32768

#include <psi.h>


//-------------------------------------------------------------------------------------------
// reel as a control

typedef struct reel_tag
{
	struct {
		BIT_FIELD bStarting : 1;
		BIT_FIELD bReelSpinning : 1;
		BIT_FIELD bStopping : 1;
		BIT_FIELD bInit : 1;
	} flags;
	INDEX position; // pos index into reel_idx[][pos];
   INDEX offset; // 96 points...
	Image images[REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	INDEX reel_idx[REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	S_32 speed; // positive or negative offset parameter... reels may spin backwards...
   // this is an idea, but I think that hrm..
	_32 next_event; // some time that if time now is greater than, do something else

	_32 stop_event; // tick at which we will be stopped.
	_32 stop_event_now; // the current now that the stop event thinks it is... so it can cat  up to 'now'
   _32 stop_event_tick;

	_32 start_event; // when started, and spinning...
	_32 start_event_now; // set at now, and itereated at
   _32 start_event_tick; // tick rate for start_event_now
   _32 target_idx;
} REEL, *PREEL;


//-------------------------------------------------------------------------------------------
// status as a control

typedef struct status_tag
{
	struct {
		BIT_FIELD bPlaying : 1;
	} flags;
   Image image;
} STATUS, *PSTATUS;
void SetStatus( PSI_CONTROL pc, int bPlaying );
int GetStatus( PSI_CONTROL pc );


//-------------------------------------------------------------------------------------------
// global ( actually a local since it's not in a .H file. )
typedef struct global_tag {
	struct {
		_32 bBackgroundInitialized : 1;
	} flags;
	S_32 ofs;
	_32 nReels;
   Image background, playing, playagain;
	Image strip; // raw strip of images loaded from a file.
   // oh I see this is a long lost global variable... wonder why this got lost in the mix....
	Image icons[NUM_ICONS];
	Image blank; // a blank square - may be laoded from a file.. but looks tacky otherwise.
	Image blurs[NUM_BLURS]; // one strip of REEL_LENGTH images blurred
	Image dodges[NUM_BLURS]; // one strip of REEL_LENGTH images dodges
	// this was previously images... but I forgot that icons were icons and images wehre something else.
   // this should be stored within the reel object...
	//Image images[NUM_REELS][REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	//INDEX reel_idx[NUM_REELS][REEL_LENGTH]; // REEL_LENGTH some unique constant to use in this code.
	//	Image images[NUM_REELS + 2][NUM_PICS + 2];
   PSI_CONTROL frame;
	//PRENDERER render;

	Image surface;
   PSI_CONTROL reel_pc[NUM_REELS];
   PREEL pReel[NUM_REELS];
	//Image subsurface[NUM_REELS];
   PSI_CONTROL status_pc;
	//Image statussurface;
   Image backgroundsurface;
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	_32 idx[NUM_REELS];


} GLOBAL;

GLOBAL g;


extern CONTROL_REGISTRATION reel_control, status_control;




//-------------------------------------------------------------------------------------------

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
		for( img = 1; img < NUM_PICS; img++ )
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
		for( img = 1; img < NUM_PICS; img++ )
		{
			for( y = 0; y < 96; y++ )
			{
				{
					CDATA pixel;
					if( src[img] )
					{
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
}

int CPROC DrawBackground( PSI_CONTROL pc )
{
	// this will ahve already been cleared, or set to the prior state...
   // besides, we're going to cover the whole thing?
	//ClearImageTo( GetControlSurface( g.frame ), BASE_COLOR_BLACK );
	BlotImage( GetControlSurface( g.frame ), g.background, 0, 0 );
	g.flags.bBackgroundInitialized = 1;
   return 1;
}

PTRSZVAL CPROC ReadInput( PTHREAD thread )
{
	char buf[256];
	PTRSZVAL retval = (PTRSZVAL)(0);
   // read buffer, if character is escape, exit nicely.
	while( fgets( buf, 256, stdin ) || buf[0] == '\x1b' )
	{

	}
	exit(0);
	return retval;//compiler warning: control reaches end of non-void function.  Well, of course it does. you're exiting, why would you return?
}

void CPROC DrawReels( PTRSZVAL psv )
{
	int n;
   for( n = 0; n < NUM_REELS; n++ )
		SmudgeCommon( g.reel_pc[n] );
}

void CPROC ComputeReels( PTRSZVAL psv )
{
	int n;
	_32 now;
	now=GetTickCount();
	for( n = 0; n < NUM_REELS; n++ )
	{
      PREEL reel = g.pReel[n];
		// if one wanted to go horizontally. one would rewrite the following code
		// with an option for x instead of Y
      if( reel )
		{
			S_32 y;
			if( reel->speed > 1000 )
            DebugBreak();
         lprintf( "speed is %ld", reel->speed );
			reel->offset += reel->speed;
			while( reel->offset > 96 )
			{
				lprintf( "Reel %p is position %d  offet %d ", reel, reel->position, reel->offset );
            if( reel->position )
					reel->position--;
				else
               reel->position = REEL_LENGTH - 1;
				reel->offset -= 96;
			}

			if( reel->flags.bStarting )
			{
				if( !reel->start_event )
				{
               lprintf( "Speed here..." );
					reel->speed = 1;
					reel->start_event_tick = 750;
               reel->start_event_now = now;
					reel->start_event = now + 4000;
				}
				else if( reel->start_event < now )
				{
					reel->flags.bStarting = 0;

					reel->flags.bStopping = 0;

					reel->flags.bReelSpinning = 1;
					reel->next_event = 0;
				}
				else while( reel->start_event_now < now )
				{
               lprintf( "Speeding up by 2-11..." );
					reel->speed += (rand()%10) + 2;
               reel->start_event_now += reel->start_event_tick;
				}
			}

			if( reel->flags.bReelSpinning )
			{
				if( !reel->next_event )
				{
               // spin for up to 3 seconds... stopping may be triggered by other things.
               reel->next_event = now + 3000;
				}
            else if( reel->next_event < now )
				{
               reel->flags.bReelSpinning = 0;
					reel->flags.bStopping = 1;
					reel->stop_event = 0;

				}
			}
			if( reel->flags.bStopping )
			{
				// there is a 10ms factor conversion ehre that is also expressed in stop_event_now
            //switch( now - reel->stop_event)
				if( !reel->stop_event )
				{
               reel->stop_event_tick = 100;
               reel->stop_event_now = now;
					reel->stop_event = now + 3000;
					reel->flags.bReelSpinning = 0;
					{
                  // come down to a sane margin
						//reel->speed = 96/2;
						// should compute how far at current speed, I would go...
						reel->position = reel->target_idx
							- (( 25/*staring speed 150 cycles thereof */ * (3000/reel->stop_event_tick) )+
							-  ( 5/*last 50 cycles at this speed*/ * (3000/reel->stop_event_tick) ))/96
							;
					}
				}
            lprintf( " reel end at %ld and now is %ld", reel->stop_event_now, now );
            while( reel->stop_event_now < now )
				{
               // if expired ( now is more than end.)
					if( ( reel->stop_event < now ) )
					{
						if( reel->offset > 90 )
						{
							reel->flags.bReelSpinning = 0;
							reel->flags.bStopping = 0;
							reel->speed = 0;
							reel->offset = 96; // set at max so we step to next full image.
							// stop, play again setup.
							//SetStatus( g.status_pc, 0 );
						}
						if( reel->offset < 6 )
						{
							reel->flags.bReelSpinning = 0;
							reel->flags.bStopping = 0;
							reel->speed = 0;
							reel->offset = 0; // set at max so we step to next full image.
							// stop, play again setup.
							//SetStatus( g.status_pc, 0 );
						}
					}
					else
					{
                  lprintf( "speed is %d", reel->speed);
					}
               // computation was based on 10 ms ticks from start.
               reel->stop_event_now += reel->stop_event_tick;
				}
				if( reel->stop_event > now )
				{
					reel->speed = ( 170 *( reel->stop_event - now ) / 3000 )  + 1;
                //96 - reel->offset
				}
			}
		}
	}
	for( n = 0; n < NUM_REELS; n++ )
	{
		if( !g.pReel[n]->flags.bStarting && !g.pReel[n]->flags.bReelSpinning && !g.pReel[n]->flags.bStopping )
			continue;
		SetStatus( g.status_pc, 0 );
      break;
	}
}


int main( void )
{
	_32 width, height, imagecount = 0;
	srand( time( NULL ) );


	g.pdi = GetDisplayInterface();
	g.pii = GetImageInterface();

			//SetSystemLog( SYSLOG_FILE, stdout );
   SetSystemLoggingLevel( 1000 + LOG_NOISE);
	GetDisplaySize( &width, &height );;
	g.frame = CreateFrame( "slot game ...", 0, 0, width, height, BORDER_NORMAL|BORDER_RESIZABLE, NULL );
   AddCommonDraw( g.frame, DrawBackground );
	//g.render = OpenDisplaySizedAt( 0, width, height, 0, 0 );
	g.surface = GetControlSurface( g.frame );
	//SetMouseHandler( g.render, MouseMethod, 0 );

//	blank = LoadImageFile( WIDE("blankimage.jpg"));
	g.blank = MakeImageFile(96,96);
   ClearImageTo( g.blank, BASE_COLOR_CYAN );
	g.playagain=LoadImageFile( WIDE("playagain.jpg"));
	g.playing  =LoadImageFile( WIDE("playing.jpg"));
   g.background = LoadImageFile( WIDE("background.jpg") );
//   g.background = blank;
	g.strip = LoadImageFile( WIDE("slot_strip.jpg") );
	g.nReels = NUM_REELS;


	{
		int n, m;
      INDEX idx;

		for( n = 0; n < NUM_ICONS; n++ )
		{
         g.icons[n] = MakeSubImage( g.strip, 96 * n, 0, 96, 96 );
		}
		n =  width = imagecount = height = 0;

		for( n = 0; n < NUM_REELS; n++)
		{
			g.reel_pc[n] = MakeNamedControl( g.frame, reel_control.name, REEL_OFSX + REEL_STEPX * n, REEL_OFSY, REEL_WIDTH, (96*NUM_PICS_IN_WINDOW), n);
		}
		for( n = 0; n < NUM_BLURS; n++ )
		{
			// these are long strings of images
         // they themselves can be slid, swapped, a NUM_PICS(10) by (20) length
			g.blurs[n] = MakeImageFile( 96, (REEL_LENGTH) * 96 );
         g.dodges[n] = MakeImageFile( 96, (REEL_LENGTH) * 96 );
			Blur( g.blurs[n], g.pReel[n%NUM_REELS]->images );
			DodgeEx( g.dodges[n], g.pReel[n%NUM_REELS]->images , 2);
		}
      g.status_pc = MakeNamedControl( g.frame, status_control.name
												, 490, 10
												, 140,  68
												, -1
												);
	}

	g.flags.bBackgroundInitialized = 0;
   DisplayFrame( g.frame );
	//UpdateDisplay(g.render);

	ThreadTo( ReadInput, 0 );
	//SetStatus( g.status_pc, TRUE ); // playing....
   AddTimer( 10, ComputeReels, 0 );
	AddTimer( 200, DrawReels, 0 );
	{
		_32 start = GetTickCount();
		xlprintf(LOG_NOISE)("Started at %lu"
								 , start);
      g.ofs = 0;
		while( 1 )
		{
         int n;
#ifndef __ARM__
         // scale to approx unit speeds..
  			WakeableSleep( 250 );
			//WakeableSleep( 33);
#endif
		}
	}
   DestroyFrame( &g.frame );
	//UnmakeImageFile( g.strip );
   return 0;
}


//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

int CPROC InitReel( PSI_CONTROL pc )
{
	ValidatedControlData( PREEL, reel_control.TypeID, reel, pc );
	if( reel )
	{
		// GetControlID( pc ) // is set as the index of the reel.... and things
		// like IDOK and IDCANCEL are overlapped with this ID space... FYI.
		{
			int m;
         reel->position = 5;
         reel->speed = 2;
			for( m = 0; m < REEL_LENGTH; m++ )
			{
				// I could create this ont he control, giving each control and
				// unknown sequence of reels... I could grab reels from a deck of cards
				reel->reel_idx[m] = rand()%NUM_ICONS;
				if( (( rand() % 5000 ) < 500 )|| ((rand() % 5000 ) > 3000 ) )
				{
					reel->reel_idx[m] = INVALID_INDEX;
					reel->images[m] = g.blank;
				}
				else
				{
					// the following was actually g.images
					reel->reel_idx[m] = reel->reel_idx[m];
					reel->images[m] = g.icons[reel->reel_idx[m]];
				}
			}
		}
		g.pReel[GetControlID(pc)] = reel;
		reel->flags.bInit = 1;
	}
	return TRUE;
}

int CPROC DrawReel( PSI_CONTROL pc )
{
	ValidatedControlData( PREEL, reel_control.TypeID, reel, pc );
	if( reel )
	{
		int n = GetControlID( pc );
		ValidatedControlData( PREEL, reel_control.TypeID, reel, pc );

		// if one wanted to go horizontally. one would rewrite the following code
		// with an option for x instead of Y
		{
			S_32 y;
			reel->offset += reel->speed;

			if( reel->flags.bReelSpinning )
			{
				lprintf( "Blot image %d"
						 ,-96*(rand()%(REEL_LENGTH - NUM_PICS_IN_WINDOW))
						 , (( reel->offset /25)*25) );
				BlotImage( GetControlSurface( pc )//g.subsurface[n]
							, g.dodges[0]
							 // by setting to a negative coordinate, drawing starts above
                      // the slot window, and draws only a size appropriate for window
							, 0, -96*((rand()%(REEL_LENGTH - NUM_PICS_IN_WINDOW)) + (( reel->offset /25)*25))
							);

			}
			else if( reel->speed > 32 )
			{
				BlotImage( GetControlSurface( pc )//g.subsurface[n]
							, g.blurs[0]
							 // by setting to a negative coordinate, drawing starts above
                      // the slot window, and draws only a size appropriate for window
							, 0, -96*(rand()%(REEL_LENGTH - NUM_PICS_IN_WINDOW)) + (( reel->offset /25)*25)
							);

			}
         else for( y = -1; y <  ( NUM_PICS_IN_WINDOW+1 ) ; y++)
			{
				INDEX idx = (REEL_LENGTH + reel->position + y)%REEL_LENGTH;
				BlotImage( GetControlSurface( pc )
							, reel->images[idx]
							, 0, y*96 + reel->offset
							);
            lprintf( "reel %d window %d index %d", n, y, idx );
			}
		}
	}
   return TRUE;
}


int CPROC MouseReel( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( PREEL, reel_control.TypeID, reel, pc );
	if( reel )
	{
		if( b )
		{
			reel->flags.bStopping = 1;
         reel->stop_event = 0;
		}
	}
   return TRUE;
}

CONTROL_REGISTRATION reel_control = { "Simple Slot Reel", { { 3*96, 96 }, sizeof( REEL ), BORDER_NONE }
												, InitReel
												, NULL
												, DrawReel
                                    , MouseReel
};

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

int CPROC InitStatus( PSI_CONTROL pc )
{
	ValidatedControlData( PSTATUS, status_control.TypeID, status, pc );
	if( status )
	{
	}
	return TRUE;
}

//---------------------------------------------------------------------------------------------------
int CPROC DrawStatus( PSI_CONTROL pc )
{
	ValidatedControlData( PSTATUS, status_control.TypeID, status, pc );
	if( status )
	{
		if( !status->flags.bPlaying )
		{
         if( g.playagain )
				BlotScaledImageSizedTo( GetControlSurface( pc )
											 , g.playagain
											 , 0, 0
											 , 140,  68
											 );
		}
		else
		{
         if( g.playing )
				BlotScaledImageSizedTo( GetControlSurface( pc )
											 , g.playing
											 , 0, 0
											 , 140,  68
											 );
		}

	}
   return TRUE;
}


//---------------------------------------------------------------------------------------------------
int CPROC MouseStatus( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( PSTATUS, status_control.TypeID, status, pc );
	if( status )
	{
		if( b )
		{
			if( !status->flags.bPlaying )
			{
				status->flags.bPlaying = 1;
				{
					int n;
					for( n = 0; n < NUM_REELS; n++ )
					{
						// might as well stop it too... descelaation trigger
						g.pReel[n]->flags.bStarting = 1;
						g.pReel[n]->start_event = 0; // init this part.

						// +2 is computation for negavitve one image length or plus one image length
						// the top and bottom could be wrapped together?
                  // maybe this sloppy math should be fixed, but at least it's nearly consistatn
                  g.pReel[n]->target_idx = (rand() % REEL_LENGTH - ( NUM_PICS_IN_WINDOW +2 )) + 1;
						g.pReel[n]->position = (rand() % REEL_LENGTH - ( NUM_PICS_IN_WINDOW +2 )) + 1;
					}
				}
            SmudgeCommon( pc );
			}
		}
	}
   return TRUE;
}

int GetStatus( PSI_CONTROL pc )
{
	ValidatedControlData( PSTATUS, status_control.TypeID, status, pc );
	if( status )
		return status->flags.bPlaying;
   return 0;
}

void SetStatus( PSI_CONTROL pc, int bPlaying )
{
	ValidatedControlData( PSTATUS, status_control.TypeID, status, pc );
	if( status )
	{
		if( bPlaying != status->flags.bPlaying )
		{
			status->flags.bPlaying = bPlaying;
         SmudgeCommon( pc ); //redraw.
		}
	}
}

CONTROL_REGISTRATION status_control = { "Simple Slot Status", { { 120, 96 }, sizeof( STATUS ), BORDER_NONE|BORDER_FIXED }
												, InitStatus
												, NULL
												, DrawStatus
                                    , MouseStatus
};

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------


PRELOAD( RegisterReel)
{
   DoRegisterControl( &reel_control );
   DoRegisterControl( &status_control );
}



