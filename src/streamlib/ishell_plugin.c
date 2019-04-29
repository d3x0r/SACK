#include <sqlgetoption.h>
#include "../InterShell/intershell_registry.h"

#include "../InterShell/vlc_hook/vlcint.h"



static uintptr_t OnCreateControl( "Video Capture-VLC") (PSI_CONTROL parent,int32_t x,int32_t y,uint32_t width,uint32_t height)
{
	PSI_CONTROL pc = MakeNamedControl( parent, "Video Control", x, y, width, height, -1 );
#if 0
	char argsline[256];
	PSI_CONTROL pc = MakeNamedControl( parent, STATIC_TEXT_NAME, x, y, width, height, -1 );
	SACK_GetProfileString( "streamlib", "default capture vlc", "dshow://", argsline, sizeof( argsline ) );
	PlayItemIn( pc, argsline );
#endif
	return (uintptr_t)pc;
}

static PSI_CONTROL OnGetControl( "Video Capture-VLC" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}


