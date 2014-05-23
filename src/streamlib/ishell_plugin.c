#include <sqlgetoption.h>
#include "../InterShell/intershell_registry.h"

#include "../InterShell/vlc_hook/vlcint.h"



static PTRSZVAL OnCreateControl( WIDE("Video Capture-VLC")) (PSI_CONTROL parent,S_32 x,S_32 y,_32 width,_32 height)
{
	PSI_CONTROL pc = MakeNamedControl( parent, WIDE("Video Control"), x, y, width, height, -1 );
#if 0
	char argsline[256];
	PSI_CONTROL pc = MakeNamedControl( parent, STATIC_TEXT_NAME, x, y, width, height, -1 );
	SACK_GetProfileString( WIDE("streamlib"), WIDE("default capture vlc"), WIDE("dshow://"), argsline, sizeof( argsline ) );
	PlayItemIn( pc, argsline );
#endif
	return (PTRSZVAL)pc;
}

static PSI_CONTROL OnGetControl( WIDE("Video Capture-VLC") )( PTRSZVAL psv )
{
	return (PSI_CONTROL)psv;
}


