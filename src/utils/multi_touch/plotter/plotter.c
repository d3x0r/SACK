#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#include <render.h>

typedef struct my_touch_point *PMyTouchPoint;
typedef struct my_touch_point MyTouchPoint;
struct my_touch_point {
   
   int32_t x,y;

   PMyTouchPoint prior, next;
};

typedef struct my_touch_glyph *PMyGlyph;
typedef struct my_touch_glyph MyGlyph;
struct my_touch_glyph {
	CDATA color;
	uint32_t id; // unique c9urrent id 
	RCOORD x,y,w,h;
	PMyTouchPoint start;
};

static  struct local {
	PRENDER_INTERFACE pri;
   PIMAGE_INTERFACE pii;
   PLIST touches;
   
   PLIST chains;

   int32_t x, y;
	uint32_t w, h;
	PRENDERER renderer;
	
} l;

void CPROC touch_plot( uintptr_t psv, PRENDERER renderer )
{
	Image surface = GetDisplayImage( renderer );
	ClearImage( surface );

	{
		INDEX segment;
		PMyTouchPoint current, next, prior = NULL;
		PMyGlyph current_glyph;
		LIST_FORALL( l.chains, segment, PMyGlyph, current_glyph )
		{
			next = current_glyph->start;
			while( ( current = next ) && ( next = current->next ) )
			{
				do_line( surface, current->x/100, current->y/100 - l.y, next->x/100, next->y/100 -l.y, current_glyph->color );
			}
		}

	}
	UpdateDisplay( renderer );
}

void AddPoint( PMyGlyph glyph, PINPUT_POINT touch )
{
	PMyTouchPoint point = New( MyTouchPoint );
	point->x = touch->x;
	point->y = touch->y;
	if( point->next = glyph->start )
	{
		if( point->x > (glyph->x+glyph->w ) )
			glyph->w = point->x - glyph->x;
		if( point->x < (glyph->x ) )
		{
			glyph->w += glyph->x - point->x;
			glyph->x = point->x;
		}
		if( point->y > (glyph->y+glyph->h ) )
			glyph->h = point->y - glyph->y;
		if( point->y < (glyph->y ) )
		{
			glyph->h += glyph->y - point->y;
			glyph->y = point->y;
		}
		glyph->start->prior = point;
	}
	else
	{
		glyph->x = point->x;
		glyph->y = point->y;
	}
	point->prior = NULL;
	glyph->start = point;

}


PMyGlyph GetGlyph( PINPUT_POINT touch, int n )
{
	INDEX segment;
	PMyGlyph current_glyph;
	LIST_FORALL( l.touches, segment, PMyGlyph, current_glyph )
	{
		if( current_glyph->id == n )
			return current_glyph;
	}
	current_glyph = New( MyGlyph );
	current_glyph->w=0;
	current_glyph->h=0;
	//current_glyph->x = touch->x;
	//current_glyph->y = touch->y;
	current_glyph->color = BASE_COLOR_WHITE;
	current_glyph->id = n;
	current_glyph->start = NULL;
	AddPoint( current_glyph, touch );
	AddLink( &l.touches, current_glyph );
	return current_glyph;
}

void start_event( PINPUT_POINT touch, int n )
{
	PMyGlyph glyph = GetGlyph( touch, n );
	AddLink( &l.chains, glyph );
}

void AddEvent( PINPUT_POINT touch, int n )
{
	PMyGlyph glyph = GetGlyph( touch, n );
	AddPoint( glyph, touch );
}
void EndEvent( PINPUT_POINT touch, int n )
{
	PMyGlyph glyph = GetGlyph( touch, n );
	AddPoint( glyph, touch );
	DeleteLink( &l.touches, glyph );
}

int CPROC touch_events( uintptr_t psv, PINPUT_POINT pTouches, int nTouches )
{
	int n;
	//lprintf( "-------------------- %d ------------------", nTouches );
	for( n = 0; n < nTouches; n++ )
	{
		if(pTouches[n].flags.new_event)
			start_event( pTouches + n, n );
		else if(pTouches[n].flags.end_event)
			EndEvent( pTouches + n, n );
		else
			AddEvent( pTouches + n, n );
		/*
		LogBinary( pTouches + n, sizeof( TOUCHINPUT ) );
		lprintf( "%3d %4d,%4d %3d,%3d %08x %08x %p  %s,%s,%s,%s", pTouches[n].dwID, pTouches[n].x, pTouches[n].y
			, pTouches[n].cxContact, pTouches[n].cyContact
			, pTouches[n].dwFlags, pTouches[n].dwMask, pTouches[n].hSource
			, (pTouches[n].flags.new_event)?"D":(pTouches[n].flags.end_event)?"U":""
			, (pTouches[n].dwMask & TOUCHINPUTMASKF_CONTACTAREA)?"CA":" "
			, (pTouches[n].dwFlags & TOUCHINPUTMASKF_EXTRAINFO)?"X":""
			, (pTouches[n].dwFlags & TOUCHINPUTMASKF_TIMEFROMSYSTEM)?"T":""
			);
			*/
	}
	Redraw( l.renderer );
	return 1;
}


int main( void )
{
	l.pii = GetImageInterface();
   l.pri = GetDisplayInterface();
	SetSystemLog( SYSLOG_FILE, stdout );
	GetDisplaySizeEx( 1, &l.x, &l.y, &l.w, &l.h );
	l.renderer = OpenDisplaySizedAt( 0, l.w, l.h*2, l.x, l.y );
	SetTouchHandler( l.renderer, touch_events, 0 );
	SetRedrawHandler( l.renderer, touch_plot, 0 );
	UpdateDisplay( l.renderer );
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

