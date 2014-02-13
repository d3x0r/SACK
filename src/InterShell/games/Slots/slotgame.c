
#include "global.h"


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



//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------


PRELOAD( old_main )
{
	_32 width, height, imagecount = 0;

	{
		int n, m;
		INDEX idx;

		for( n = 0; n < NUM_ICONS; n++ )
		{
			g.icons[n] = MakeSubImage( g.strip, 96 * n, 0, 96, 96 );
		}
		n =  width = imagecount = height = 0;

		for( n = 0; n < NUM_BLURS; n++ )
		{
			// these are long strings of images
			// they themselves can be slid, swapped, a NUM_PICS(10) by (20) length
			g.blurs[n] = MakeImageFile( 96, (REEL_LENGTH) * 96 );
			g.dodges[n] = MakeImageFile( 96, (REEL_LENGTH) * 96 );
			Blur( g.blurs[n], g.pReel[n%NUM_REELS]->images );
			DodgeEx( g.dodges[n], g.pReel[n%NUM_REELS]->images , 2);
		}
	}

	g.flags.bBackgroundInitialized = 0;

	//SetStatus( g.status_pc, TRUE ); // playing....
	AddTimer( 10, ComputeReels, 0 );
	AddTimer( 200, DrawReels, 0 );
	g.ofs = 0;
}


