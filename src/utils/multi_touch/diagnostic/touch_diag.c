#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii

#include <render.h>

static struct {
	PRENDER_INTERFACE pri;
   PIMAGE_INTERFACE pii;
} l;

int CPROC touch_events( PTRSZVAL psv, PTOUCHINPUT pTouches, int nTouches )
{
	int n;
	lprintf( "-------------------- %d ------------------", nTouches );
	for( n = 0; n < nTouches; n++ )
	{
		LogBinary( pTouches + n, sizeof( TOUCHINPUT ) );
		lprintf( "%3d %4d,%4d %3d,%3d %08x %08x %p  %s,%s,%s,%s", pTouches[n].dwID, pTouches[n].x, pTouches[n].y
			, pTouches[n].cxContact, pTouches[n].cyContact
			, pTouches[n].dwFlags, pTouches[n].dwMask, pTouches[n].hSource
			, (pTouches[n].dwFlags & TOUCHEVENTF_DOWN)?"D":(pTouches[n].dwFlags & TOUCHEVENTF_UP)?"U":""
			, (pTouches[n].dwMask & TOUCHINPUTMASKF_CONTACTAREA)?"CA":" "
			, (pTouches[n].dwFlags & TOUCHINPUTMASKF_EXTRAINFO)?"X":""
			, (pTouches[n].dwFlags & TOUCHINPUTMASKF_TIMEFROMSYSTEM)?"T":""
			);
	}
	return 1;
}


int main( void )
{
	S_32 x, y;
	_32 w, h;
	PRENDERER renderer;

	l.pii = GetImageInterface();
   l.pri = GetDisplayInterface();
	SetSystemLog( SYSLOG_FILE, stdout );
	GetDisplaySizeEx( 0, &x, &y, &w, &h );
	renderer = OpenDisplaySizedAt( 0, w, h, x, y );
	SetTouchHandler( renderer, touch_events, 0 );
	UpdateDisplay( renderer );
	while( 1 )
	{
      WakeableSleep( 1000 );
	}

#if 0
   HOOKPROC hkprcSysMsg;
static HINSTANCE hinstDLL; 
static HHOOK hhookSysMsg; 
 
hinstDLL = LoadLibrary(TEXT("c:\\myapp\\sysmsg.dll")); 
hkprcSysMsg = (HOOKPROC)GetProcAddress(hinstDLL, "SysMessageProc"); 

hhookSysMsg = SetWindowsHookEx( 
                    WH_SYSMSGFILTER,
                    hkprcSysMsg,
                    hinstDLL,
                    0); 

SetWindowsHookEx(
#endif
   return 0;
}

