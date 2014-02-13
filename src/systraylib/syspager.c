#include <windows.h>
#include <sack_types.h>
#include <logging.h>
typedef struct local_tag
{
   ATOM aClass[3];
   HWND hWnd;
} LOCAL;

static LOCAL l;

typedef struct Systray_register
{
	_32 _34753423; // or is it 0x23347534 ?
	_32 NIM_msg;  // which NofityIconData message...
   NOTIFYICONDATA nid; // NotifyIconData
} SYSTRAY_DATA, *PSYSTRAY_DATA;

WINPROC( int, PagerWindowProc )( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CREATE:
		break;
	case WM_COPYDATA:
		{
			PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
         PSYSTRAY_DATA psd = pcds->lpData;
			lprintf( WIDE("Something is trying to copy data to me... %08x, %08x  %ld"), wParam, pcds
					 , pcds->dwData );
			switch( psd->NIM_msg )
			{
			case NIM_ADD:

            break;
			case NIM_MODIFY:
            break;
			case NIM_DELETE:
				break;
#if _WIN32_IE >= 0x0500
			case NIM_SETFOCUS:
            break;
			case NIM_SETVERSION:
				break;
#endif
			}
         // hmm it might be an icon...
         return 1;
		}
      break;
	default:
		lprintf( WIDE("Received Message: %08x %08x %08x %08x"), hWnd, uMsg, wParam, lParam );
	}
   return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

void RegisterSystemClass( void )
{
   WNDCLASS wc;
	memset (&wc, 0, sizeof (WNDCLASS));
	wc.style = CS_OWNDC | CS_GLOBALCLASS;

	wc.lpfnWndProc = (WNDPROC)PagerWindowProc;
	wc.hInstance = GetModuleHandle (NULL);
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
	wc.lpszClassName = "Shell_TrayWnd"; // "TrayNotifyWnd";
	wc.cbWndExtra = 4;   // one extra DWORD

	l.aClass[0] = RegisterClass (&wc);
	if (!l.aClass[0])
	{
		lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
		return;
	}
/*
	wc.lpszClassName = "TrayNotifyWnd"; // "TrayNotifyWnd";
	l.aClass[1] = RegisterClass (&wc);
	if (!l.aClass[1])
	{
		lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
		return;
	}

	wc.lpszClassName = "SysPager"; // "TrayNotifyWnd";
	l.aClass[2] = RegisterClass (&wc);
	if (!l.aClass[2])
	{
		lprintf( WIDE("Failed to register class %s %d"), wc.lpszClassName, GetLastError() );
		return;
	}
*/
	l.hWnd = CreateWindowEx (0
									, (char *) l.aClass[0]
									, NULL //"System Tray Replacement"
									, WS_OVERLAPPEDWINDOW|WS_VISIBLE
									, 0, 0, 150, 150
									, l.hWnd
									, NULL // Menu
									, GetModuleHandle (NULL)
									, (void *) 1);
	lprintf( WIDE("Created %08x"), l.hWnd );
   /*
	l.hWnd = CreateWindowEx (0
									, (char *) l.aClass[1]
									, NULL //"System Tray Replacement"
									, WS_CHILD|WS_VISIBLE
									, 0, 0, 150, 150
									, l.hWnd
									, NULL // Menu
									, GetModuleHandle (NULL)
									, (void *) 1);
   lprintf( WIDE("Created %08x"), l.hWnd );
	l.hWnd = CreateWindowEx (0
									, (char *) l.aClass[2]
									, WIDE("") //"System Tray Replacement"
									, WS_CHILD|WS_VISIBLE
									, 0, 0, 150, 150
									, l.hWnd
									, NULL // Menu
									, GetModuleHandle (NULL)
									, (void *) 1);
   lprintf( WIDE("Created %08x"), l.hWnd );
	l.hWnd = CreateWindowEx (0
									, WIDE("ToolbaWindow32")
									, WIDE("Notification Area") //"System Tray Replacement"
									, WS_CHILD|WS_VISIBLE
									, 0, 0, 150, 150
									, l.hWnd
									, NULL // Menu
									, GetModuleHandle (NULL)
									, (void *) 1);
   lprintf( WIDE("Created %08x"), l.hWnd );
   */
}

int main( void )
{
	RegisterSystemClass();
	while( 1 )
	{
		MSG msg;
		GetMessage( &msg, NULL, 0, 0 );
      DispatchMessage( &msg );
	}
   return 0;
}

