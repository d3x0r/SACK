#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii

#include <render.h>

static struct {
	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;
} l;

int CPROC touch_events( PTRSZVAL psv, PINPUT_POINT pTouches, int nTouches )
{
	int n;
	lprintf( "-------------------- %d ------------------", nTouches );
	for( n = 0; n < nTouches; n++ )
	{
		lprintf( "%3d %4d,%4d  %s", n, pTouches[n].x, pTouches[n].y
			, pTouches[n].flags.new_event?"D":(pTouches[n].flags.end_event)?"U":""
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

