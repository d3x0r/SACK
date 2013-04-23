#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE
#include <network.h>
#include <sqlgetoption.h>
#include <vectlib.h>


#ifdef __ANDROID__
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#else
#define GLEW_NO_GLU
#include <GL/glew.h>
#include <GL/gl.h>
//#include <GL/glu.h>
#endif

#define INTERFACE_USED

#include <flashdrive.h>
#include <../src/fut/flashdrive/flashdrive_global.h>

#include "local.h"

static struct {
	PFLASH_INTERFACE flashdrive_interface;
	struct bingo_game_state *flashdrive_global;
} fl;

PRELOAD( InitFlashdrive )
{
	NetworkStart();
	fl.flashdrive_interface = GetFlashInterface();
	if( fl.flashdrive_interface )
		fl.flashdrive_global = fl.flashdrive_interface->_FLASHDRIVE_GetGlobal();
}


void DrawOverlay( PTRANSFORM camera )
{
	if( l.active_ball && ( l.active_ball_forward_tick && ( l.active_ball_forward_tick < l.last_tick ) ) )
	{
		int bound = 0;
		int n;
		VECTOR corners[4];
		VECTOR out_corners[4];

		corners[0][0] = -512;
		corners[0][1] = 768/2;
		corners[0][2] = l.identity_depth[0]/2;
		corners[1][0] = 512;
		corners[1][1] = 768/2;
		corners[1][2] = l.identity_depth[0]/2;
		corners[2][0] = 512;
		corners[2][1] = -768/2;
		corners[2][2] = l.identity_depth[0]/2;
		corners[3][0] = -512;
		corners[3][1] = -768/2;
		corners[3][2] = l.identity_depth[0]/2;

		if( fl.flashdrive_global )
		{
			if( l.player_image )
				if( l.active_ball == fl.flashdrive_global->default_stream->player_appreciation_ball )
				{
					bound = 1;
					glBindTexture( GL_TEXTURE_2D, ReloadTexture( l.player_image, 0 ) );
					/* show player_ball overlay */
				}
			{
				INDEX idx;
				for( idx = 0; idx < fl.flashdrive_global->default_stream->nHotBalls; idx++ )
				{
					if( !l.hotball_image[idx] )
						continue;
					if( l.active_ball == fl.flashdrive_global->default_stream->hot_ball_order[idx] )
					{
						/* handle an offset?  Troll Ball Image? */
						/* show hotball X overlay */
						bound = 1;
						glBindTexture( GL_TEXTURE_2D, ReloadTexture( l.hotball_image[idx], 0 ) );
						break;
					}
				}
			}
			if( l.wild_image )
				if( TESTFLAG( fl.flashdrive_global->default_stream->marked, l.active_ball ) )
				{
					if( l.active_ball != fl.flashdrive_global->default_stream->ball_order[fl.flashdrive_global->default_stream->nBallsCalled-1] )
					{
						/* show wild overlay */
						bound = 1;
						glBindTexture( GL_TEXTURE_2D, ReloadTexture( l.wild_image, 0 ) );
					}
				}
		}
		if( bound )
		{
#ifndef __ANDROID__
			for( n = 0; n < 4; n++ )
				Apply( camera, out_corners[n], corners[n] );
			glDisable( GL_DEPTH );
			glDisable( GL_DEPTH_TEST );
			glBegin( GL_QUADS );
			for( n = 0; n < 4; n++ )
			{
				glColor3d( 1.0, 1.0, 1.0 );

				glTexCoord2d( n==0?0
					:n==1?1
					:n==2?1
					:n==3?0:0
					,  n==0?0
					:n==1?0
					:n==2?1
					:n==3?1:0 );
				glVertex3dv( out_corners[n] );
			}
			glEnd();
#endif
		}

	}
}


static void SetDrawnBalls( int count, PDATAQUEUE *pdqInts )
{

	PSI_CONTROL pc;
	INDEX idx;
	int update = 0;
	int n;
	//int last_good_image = l.nNextBalls[0];
	int first_updated_sequence = -1;
	// will get an event after this 'OnBallDrawEvent'
#ifdef DEBUG_NEXTBALL_DRAWING
	lprintf( "Setting drawn balls to %d (maybe less, the queue may not have that many?)", count );
#endif
	// the queue may change here more than 1 ball... but the
	// ball draw event is limited to a single ball (the last ball)
	for( n = 0; n < count; n++ )
	{
		int ball;
		if( !PeekDataQueueEx( pdqInts, int*, &ball, n ) )
		{
			//if count lies, this will still result in the same effect as setting... uhmm actually it's this many.
			//   count will often be +1 of the balls in queue - but this has meaning that a ball was validly in the queue
			// and the queue is properly updating.
			count = n;
			ball = -1;
		}

		if( l.nNextBalls[n] != ball )
		{
			if( first_updated_sequence == -1 )
				first_updated_sequence = n;
			update++;
			l.nNextBalls[n] = ball;
		}
	}

	{
		int max_seq = 0;
		for( ; n < l.last_set && (!max_seq || ( n <= max_seq ) ); n++ )
		{
			if( l.nNextBalls[n] != -1 )
			{
				if( first_updated_sequence == -1 )
					first_updated_sequence = n;
				l.nNextBalls[n] = -1;
			}
#ifdef DEBUG_NEXTBALL_DRAWING
			else
			{
				lprintf( "Image was already blank (-1 ball) %p" );
			}
#endif
		}
	}

	l.last_set = count;
}


static void OnBallDrawEvent( "Blower Animation" )( int ball )
{
	//if( ball )
		;//BeginWatch( ball );
	//else
	{
		SetDrawnBalls( fl.flashdrive_global->default_stream->balls_drawn
			, &fl.flashdrive_global->default_stream->ball_queue );
	}
}

static void OnBallCallEvent( "Blower Animation" )( void )
{
	int n;
	int target = fl.flashdrive_global->default_stream->nBallsCalled;
   lprintf( "Ball Call Event...." );
	for( n = 0; n < (target-1); n++ )
	{
      lprintf( "balls that are already called %d", fl.flashdrive_global->default_stream->ball_order[n] );
		// these balls are already gone.
		FadeBall( fl.flashdrive_global->default_stream->ball_order[n] );
	}
	if( l.active_ball )
	{

		if( !l.flags.nextball_mode )
		{
			lprintf( "begin pivot, got a call event?" );
			BeginPivot();
			//l.drop_active_ball = l.last_tick + l.show_ball_time;
		}
		else
		{
			lprintf( "end watch ball here" );
			EndWatchBall();
		}
	}
}

static void OnDropBalls( "Blower Animation" )( void )
{
	l.flags.rack_balls = 1;
}

