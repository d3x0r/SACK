// well do stuff here....
#include <stdhdrs.h>
#include <stdio.h>
#include <deadstart.h>
#ifdef __DEKWARE_PLUGIN__
#define DEFINES_DEKWARE_INTERFACE
#include "plugin.h"
extern PDATAPATH CPROC CreateConsole( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters );
#endif
#define FORCE_95 1

#include "consolestruc.h"
// common DLL plugin interface.....
extern void UnregisterWindows( void );
#ifdef _WIN32
HINSTANCE hInstMe;
//DllEntryPoint

LIBMAIN()
{
	hInstMe = GetModuleHandle (_WIDE(TARGETNAME));//hInstance;
   return 1;
}
LIBEXIT()
{
   return 1;
}
LIBMAIN_END();
#endif

#if 0
#ifdef _WIN32
__declspec(dllimport)
#else
extern
#endif
int b95;
#endif
static int myTypeID; // supplied for uhmm... grins...

#ifdef __DEKWARE_PLUGIN__

#ifdef _WIN32
__declspec(dllimport)
//  extern
#else
	extern
#endif
	_OptionHandler KeyBind, KeyUnBind, ConSetColor, HistoryStat, SetMode, SetTabs, SetHistory, DumpHistory;

#ifdef WINCON
	extern
	_OptionHandler SetFlash;
#endif
;

#ifndef __GNUC__

#define NUM_METHODS ( sizeof( methods ) / sizeof( methods[0] ))
static option_entry methods[] = { { DEFTEXT( WIDE("keybind") ), 4, 7, DEFTEXT( WIDE("Redefine a key...") ), KeyBind }
                         , { DEFTEXT( WIDE("keyunbind") ), 4, 9, DEFTEXT( WIDE("Undefine a key...") ), KeyUnBind }
                         , { DEFTEXT( WIDE("setcolor") ), 4, 8, DEFTEXT( WIDE("Set default terminal color") ), ConSetColor }
                                 , { DEFTEXT( WIDE("history") ), 3, 7, DEFTEXT( WIDE("Show status of histor") ), HistoryStat }
                                 , { DEFTEXT( WIDE("mode") ), 4, 4, DEFTEXT( WIDE("Set display modes (direct/line) (TEXTCHAR/buffer)")), SetMode }
#ifdef WINCON
                         , { DEFTEXT( WIDE("flash") ), 5, 5, DEFTEXT( WIDE("Flash the window in the taskbar")), SetFlash }
#endif
                         , { DEFTEXT( WIDE("tabsize")), 3, 7, DEFTEXT( WIDE("Set the size of the tab character") ), SetTabs }
                         , { DEFTEXT( WIDE("sethistory")), 4, 10, DEFTEXT( WIDE("Set the size of history") ), SetHistory }
                         //, { DEFTEXT( WIDE("status") ), 3, 6, DEFTEXT( WIDE("Define a status field in the status bar")), StatusField }
//, { DEFTEXT( WIDE("update") ), 3, 6, DEFTEXT( WIDE("Refresh that status bar's values") ), UpdateStatusBar }
//#if 0
                           , { DEFTEXT( WIDE("dump") ), 1, 4, DEFTEXT( WIDE("Dumps history to a file")), DumpHistory }
//#endif
};
#endif

PUBLIC( TEXTCHAR *, RegisterRoutines )( void )
{
#if defined( CURSECON )
#define NAME WIDE("cursecon")
#elif defined( PSICON )
#define NAME WIDE("psicon")
#elif defined( WINCON )
#define NAME WIDE("wincon")
#elif defined( CONSOLECON )
#define NAME WIDE("console")
#endif

	//	DebugBreak();
#ifdef __WATCOMC__
   // huh - something changed somewhere along the way...
	{
		int n;
		for( n = 0; n < NUM_METHODS; n++ )
		{
         methods[n].function = (**((_OptionHandler**)(methods[n].function)));
		}
	}
#endif
   //myTypeID = RegisterDeviceOpts( NAME, WIDE("interface device...."), CreateConsole, methods, NUM_METHODS  );
   return DekVersion;
}

PUBLIC( void, UnloadPlugin )( void ) // this routine is called when /unload is invoked
{
	// destory all existing windows...
	UnregisterDevice( NAME );
	UnregisterWindows();
}
#endif

// $Log: psiconlink.c,v $
// Revision 1.18  2005/08/08 15:24:12  d3x0r
// Move updated rectangle struct to common space.  Improved curses console handling....
//
// Revision 1.17  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.16  2005/02/22 12:28:51  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.15  2005/01/20 06:10:19  d3x0r
// One down, 3 to convert... concore library should serve to encapsulate drawing logic and history code...
//
// Revision 1.14  2005/01/17 09:01:17  d3x0r
// checkpoint ...
//
// Revision 1.13  2004/09/20 10:00:16  d3x0r
// Okay line up, wrapped, partial line, line up, history alignment, all sorts of things work very well now.
//
// Revision 1.12  2004/06/14 09:07:14  d3x0r
// Mods to reduce function depth
//
// Revision 1.11  2004/06/12 23:57:14  d3x0r
// Hmm some strange issues left... just need to simply code now... a lot of redunancy is left...
//
// Revision 1.10  2004/05/04 07:15:01  d3x0r
// Fix hinstance, implement relay segments out, clean, straighten and shape up
//
// Revision 1.9  2004/04/06 01:50:31  d3x0r
// Update to standardize device options and the processing thereof.
//
// Revision 1.8  2004/01/21 08:32:08  d3x0r
// Massive mods - working on unifying rendercommandline
//
// Revision 1.7  2003/03/28 12:16:29  panther
// Fix some minor issues with PSI interface
//
// Revision 1.6  2003/03/26 01:08:20  panther
// Fix CVS conflict error.  Clean some warnings with typecasting
//
// Revision 1.5  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.4  2003/03/25 08:59:02  panther
// Added CVS logging
//
