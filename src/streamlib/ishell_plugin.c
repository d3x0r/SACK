#include <sqlgetoption.h>
#include "../InterShell/intershell_registry.h"

#include "../InterShell/vlc_hook/vlcint.h"



static uintptr_t OnCreateControl( WIDE("Video Capture-VLC")) (PSI_CONTROL parent,int32_t x,int32_t y,uint32_t width,uint32_t height)
{
	PSI_CONTROL pc = MakeNamedControl( parent, WIDE("Video Control"), x, y, width, height, -1 );
#if 0
	char argsline[256];
	PSI_CONTROL pc = MakeNamedControl( parent, STATIC_TEXT_NAME, x, y, width, height, -1 );
	SACK_GetProfileString( WIDE("streamlib"), WIDE("default capture vlc"), WIDE("dshow://"), argsline, sizeof( argsline ) );
	PlayItemIn( pc, argsline );
#endif
	return (uintptr_t)pc;
}

static PSI_CONTROL OnGetControl( WIDE("Video Capture-VLC") )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}


