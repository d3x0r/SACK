#include <controls.h>
#include <deadstart.h>
#include <psi.h>

static CONTROL_REGISTRATION calendar;

typedef struct {
	struct {
		BIT_FIELD bShowMonth : 1;
		BIT_FIELD bStuff : 1;
		BIT_FIELD bMonth: 1;
		BIT_FIELD bWeek : 1;
		BIT_FIELD bDays : 1;
		BIT_FIELD bNow : 1;
	} flags;

} CALENDER, *PCALENDAR;



#if 0
int CPROC DrawCalender( PSI_CONTROL pc )
{
	// stuff...
	ValidatedControlData( PCALENDAR, calendar.nType, pCal, pc );
	if( pCal )
	{
		Image surface = GetCommonSurface( pc );

		if( pCal->flags.bMonth )
		{
		}
		else if( pCal->flags.bWeek )
		{
		}
		else if( pCal->flags.bDays )
		{
		}
		else if( pCal->flags.bNow )
		{
			if( pCal->flags.bTime )
			{
			}
		}

		{

		}
	}

}












CONTROL_REGISTRATION calendar = { "Calender Widget"
										  , { { 50, 50 }, BORDER_NONE, sizeof( CALENDER ) }
										  , NULL
};

PRELOAD(DoRegisterControl)
{
   DoRegisterControl( &calendar );
}


#endif
