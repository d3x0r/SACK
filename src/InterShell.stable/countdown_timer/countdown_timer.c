#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define USE_IMAGE_INTERFACE countdown_local.pii
#include <stdhdrs.h>
#include <psi.h>
#include <sqlgetoption.h>
#include "../intershell_registry.h"
#include "../intershell_export.h"

static struct countdown_timer_local
{
	PIMAGE_INTERFACE pii;
	HHOOK nexthook;
	LOGICAL hidden;
	CDATA background;
	CDATA text;
	uint32_t target_tick;
	uint32_t default_tick_length;
	SFTFont *font;
	struct { int32_t x, y; uint32_t w, h; } show_location;
	PLIST timers;
	uint32_t tick_timer; // timer ID of periodic refresh
} countdown_local;

EasyRegisterControl( "Countdown Timer", 0 );

static LRESULT CALLBACK	MouseHook(int code,		// hook code
				WPARAM wParam,	 // virtual-key code
				LPARAM lParam	  // keystroke-message information
									)
{
	if( code < 0 )
	{
		lprintf( "negative code..." );
		return CallNextHookEx ( countdown_local.nexthook, code, wParam, lParam);
	}
	else
	{
		PMSLLHOOKSTRUCT hook = (PMSLLHOOKSTRUCT)lParam;
		switch( wParam )
		{
		case WM_LBUTTONDOWN:
			{
				WORD button = HIWORD( hook->mouseData );
				if( hook->pt.x >= countdown_local.show_location.x 
				    && hook->pt.x < ( countdown_local.show_location.x + countdown_local.show_location.w ) 
				    && hook->pt.y >= countdown_local.show_location.y 
				    && hook->pt.y < ( countdown_local.show_location.y + countdown_local.show_location.h ) )
				{
					countdown_local.target_tick = GetTickCount() + countdown_local.default_tick_length;
					if( countdown_local.hidden )
					{
						countdown_local.hidden = FALSE;
						{
							INDEX idx;
							PSI_CONTROL timer;
							LIST_FORALL( countdown_local.timers, idx, PSI_CONTROL, timer )
							{
								RevealCommon( timer );
								SmudgeCommon( timer );
							}
						}
					}
				}
				else
				{
					INDEX idx;
					PSI_CONTROL timer;
					countdown_local.hidden = TRUE;
					LIST_FORALL( countdown_local.timers, idx, PSI_CONTROL, timer )
					{
						SmudgeCommon( timer );
						HideControl( timer );
					}
				}
			}
			break;
#ifdef MINGW_SUX
#define WM_MOUSEHWHEEL 0x020E
#endif
		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			break;
		}
		//lprintf( "Mouse Debug: %d %d %08x", hook->pt.x, hook->pt.y );
	}
	return CallNextHookEx ( countdown_local.nexthook, code, wParam, lParam);
}

static uintptr_t CPROC MouseThread( PTHREAD unused )
{
	MSG msg;
	{
		HMODULE self = GetModuleHandle( _WIDE( TARGETNAME ) );

		countdown_local.nexthook =
			SetWindowsHookEx( WH_MOUSE_LL, (HOOKPROC)MouseHook
				, self, 0 );
	}
	while( GetMessage( &msg, NULL, 0, 0 ) )
		DispatchMessage( &msg );
	return 0;
}

static void CPROC RefreshProc( uintptr_t psvTimer )
{
	if( !countdown_local.hidden )
	{
		INDEX idx;
		PSI_CONTROL timer;
		LIST_FORALL( countdown_local.timers, idx, PSI_CONTROL, timer )
			SmudgeCommon( timer );
	}
}

PRELOAD( InitCountdownTimer )
{
	countdown_local.pii = GetImageInterface();
	countdown_local.hidden = TRUE;
	countdown_local.default_tick_length = SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Default Length", 12000 );
	countdown_local.show_location.x = SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Touch Location X", 0 );
	countdown_local.show_location.y = SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Touch Location Y", 200 );
	countdown_local.show_location.w = SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Touch Location Width", 200 );
	countdown_local.show_location.h = SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Touch Location Height", 200 );
	
	{
		DECLTEXTSZ( tmp, 32 );
		PTEXT updateable;
		SetTextSize( (PTEXT)&tmp, SACK_GetProfileString( GetProgramName(), "Countdown Timer/Background Color", "$FF001F4F", GetText( (PTEXT)&tmp ), 32 ) );
		updateable = (PTEXT)&tmp;
		if( !GetColorVar( &updateable, &countdown_local.background ) )
			countdown_local.background = 0xFFFFFFFF;
		SetTextSize( (PTEXT)&tmp, SACK_GetProfileString( GetProgramName(), "Countdown Timer/Text Color", "$FF3F7f4F", GetText( (PTEXT)&tmp ), 32 ) );
		updateable = (PTEXT)&tmp;
		if( !GetColorVar( &updateable, &countdown_local.text ) )
			countdown_local.background = 0xFF000000;
	}
	/*
	if( 0 )
	{
		TEXTCHAR namebuf[64];
		SACK_GetProfileString( GetProgramName(), "Countdown Timer/Font Name", "arialbd.ttf", namebuf, 64 );
		countdown_local.font = RenderFontFile( namebuf
			, SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Font Width", 32 )
			, SACK_GetProfileInt( GetProgramName(), "Countdown Timer/Font Height", 32 ), 3 );
	}
	*/
	ThreadTo( MouseThread, 0 );
	AddTimer( 500, RefreshProc, 0 );
}

static int OnDrawCommon( "Countdown Timer" )( PSI_CONTROL pc )
{
	Image surface = GetControlSurface( pc );
	if( countdown_local.hidden )
		ClearImageTo( surface, 0 );
	else
	{

		TEXTCHAR buf[12];
		uint32_t now = GetTickCount();
		ClearImageTo( surface, countdown_local.background );
		if( countdown_local.target_tick > now )
			snprintf( buf, 12, "%02d", 1 + ( ( countdown_local.target_tick - now ) / 1000 ) );
		else
			snprintf( buf, 12, "00" );
		{
			uint32_t w, h;
			GetStringSizeFontEx( buf, 2, &w, &h, *countdown_local.font );
			PutStringFontEx( surface, ( surface->width - w ) / 2, (surface->height - h ) / 2, countdown_local.text, 0, buf, 2, *countdown_local.font );
		}
	}
	return 1;
}

static int OnCreateCommon( "Countdown Timer" )( PSI_CONTROL pc )
{
	AddLink( &countdown_local.timers, pc );
	return 1;
}

static uintptr_t OnCreateControl( "Countdown Timer" )(PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h)
{
	if( !countdown_local.font )
		countdown_local.font = UseACanvasFont( parent, "Countdown Font" );
	return (uintptr_t)MakeNamedControl( parent, "Countdown Timer", x, y, w, h, 0 );
}

static PSI_CONTROL OnGetControl( "Countdown Timer" )(uintptr_t psv)
{
	return (PSI_CONTROL)psv;
}

static LOGICAL OnQueryShowControl( "Countdown Timer" )(uintptr_t psv)
{
	return !countdown_local.hidden;
}
