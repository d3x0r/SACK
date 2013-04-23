#include <stdhdrs.h>
#include <stdio.h>
#ifdef _DEBUG
#include <sharemem.h>
#include <logging.h>
#endif
#include <controls.h>

int main( char argc, char **argv )
{
#ifdef _DEBUG
//	SetCriticalLogging( TRUE );
	SystemLogTime( TRUE );
	SetSystemLog( SYSLOG_FILE, fopen( "guiremote.log", "wt" ) );	
#endif
	AlignBaseToWindows();
	SetControlInterface( GetDisplayInterface() );
   SetControlImageInterface( GetImageInterface() );
	DoController();
	return 0;
}
// Checked in by: $Author: panther $
// $Revision: 1.6 $
// $Log: main.c,v $
// Revision 1.6  2002/05/22 18:58:34  panther
// *** empty log message ***
//
// Revision 1.5  2002/05/14 21:08:41  panther
// Added holdoff message box
//
// Revision 1.4  2002/04/23 16:45:38  panther
// *** empty log message ***
//
// Revision 1.3  2002/04/15 16:26:39  panther
// Added revision tags.
//

