#define STATUS_SOURCE
#include "global.h"

static struct status_local_data
{
	_32 ID;
} status_local;


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

PRELOAD( RegisterReel)
{
	DoRegisterControl( &status_control );
}


static PTRSZVAL OnCreateControl( WIDE("Games/Slots/Status Patch"))(PSI_CONTROL parent,S_32 x,S_32 y,_32 w,_32 h)
{
	return (PTRSZVAL)MakeNamedControl( parent, reel_control.name, x, y, w, h, status_local.ID++ );
}

 static PSI_CONTROL OnGetControl(WIDE("Games/Slots/Status Patch"))(PTRSZVAL psvInit)
 { 
	 return (PSI_CONTROL)psvInit; 
 }

