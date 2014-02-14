// well do stuff here....
#include <stdhdrs.h>
#include "plugin.h"
#include <termios.h>
#define FORCE_95 1

// common DLL plugin interface.....
#ifndef __LINUX__
int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   return TRUE; // success whatever the reason...
}
#endif
int myTypeID; // supplied for uhmm... grins...

PDATAPATH Open( PSENTIENT ps, PTEXT parameters );
int SetLines( PSENTIENT ps, PTEXT parameters );
struct termios ts, ts2;

char *RegisterRoutines( PEXPORTTABLE pExportTable )
{
    tcgetattr( 0, &ts );
    ts2 = ts;
    cfmakeraw( &ts );
    ts.c_cc[VMIN] = 1;
    ts.c_cc[VTIME] = 0;    
    tcsetattr( 0, TCSADRAIN, &ts );
    
   pExportedFunctions = pExportTable;
   myTypeID = RegisterDevice( "console", "ANSI term based console....", Open );
   RegisterRoutine( "lines", "Set limit of console lines before pause", SetLines );
   {
      PENTITY pe;
      DECLTEXT( open_console, "/command console" );
      DECLTEXT( first, "/script macros" );
      DECLTEXT( story, "/story" );

      pe = CreateEntityIn( NULL, SegCreateFromText( "MOOSE" ) );
      PLAYER = CreateAwareness( pe );
      EnqueLink( PLAYER->Command->ppInput, burst( NULL, (PTEXT)&open_console ) );
      EnqueLink( PLAYER->Command->ppInput, burst( NULL, (PTEXT)&first ) );
//   EnqueLink( PLAYER->Command->ppInput, burst( NULL, (PTEXT)&story ) );
      PLAYER->Current->pDescription = SegCreateFromText( "Master Operator of System Entities." );
   }
   return DekVersion;
}

void UnloadPlugin( void ) // this routine is called when /unload is invoked
{
    tcsetattr( 0, TCSADRAIN, &ts2 );
   UnregisterRoutine( "lines" );
   UnregisterDevice( "console" );
}

// $Log: ntlink.c,v $
// Revision 1.3  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
