#include "global.h"
#include <psi.h>

static struct reel_local_data
{
	uint32_t ID;
} reel_local;

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
		ClearImage( GetControlSurface( pc ) );
		// if one wanted to go horizontally. one would rewrite the following code
		// with an option for x instead of Y
		{
			int32_t y;
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


int CPROC MouseReel( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
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

CONTROL_REGISTRATION reel_control = { WIDE("Simple Slot Reel"), { { 3*96, 96 }, sizeof( REEL ), BORDER_NONE }
												, InitReel
												, NULL
												, DrawReel
												, MouseReel
};

PRELOAD( RegisterReel)
{
	DoRegisterControl( &reel_control );
}

static uintptr_t OnCreateControl( WIDE("Games/Slots/Slot Reel"))(PSI_CONTROL parent,int32_t x,int32_t y,uint32_t w,uint32_t h)
{
	return MakeNamedControl( parent, reel_control.name, x, y, w, h, reel_local.ID++ );
}

 static PSI_CONTROL OnGetControl(WIDE("Games/Slots/Slot Reel"))(uintptr_t psvInit)
 { 
	 return (PSI_CONTROL)psvInit; 
 }
