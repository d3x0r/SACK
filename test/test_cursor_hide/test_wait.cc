
#include <windows.h>
#include <stdio.h>

HCURSOR hCursor;

LRESULT CALLBACK
    VideoWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    	switch (uMsg)
        {
		case WM_CREATE:
			return TRUE;
		case WM_CLOSE:
			DestroyWindow( hWnd );
			break;
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hWnd, &ps );

			// All painting occurs here, between BeginPaint and EndPaint.

			FillRect( hdc, &ps.rcPaint, (HBRUSH)( COLOR_WINDOW + 1 ) );

			EndPaint( hWnd, &ps );
		}
		case WM_SETCURSOR:
			SetCursor( hCursor );
			break;
			/*
		case WM_MOUSEMOVE:
			  printf( "MouseMove.." );
            return 0;
				*/
        }
	return DefWindowProc (hWnd, uMsg, wParam, lParam);

}

void OpenWindow() {

    WNDCLASS wc;
    ATOM aClass;
	{

		memset (&wc, 0, sizeof (WNDCLASS));
		wc.style = 0
			 |	CS_OWNDC 
			 | CS_GLOBALCLASS
			 ;

		wc.lpfnWndProc = (WNDPROC) VideoWindowProc;
		wc.hInstance = GetModuleHandle( NULL );
		wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
		wc.lpszClassName = "VideoOutputClass";
		wc.cbWndExtra = sizeof( uintptr_t );	// one extra pointer

		aClass = RegisterClass (&wc);
		if (!aClass)
		{
			printf( "Failed to register class %s %d\n", wc.lpszClassName, GetLastError() );
			return;
		}


	}


        HWND hWndInstance = CreateWindowEx (0
												  , (LPSTR)aClass
												  , "Set Wait Cursor"
												  , WS_OVERLAPPEDWINDOW
												  , CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT
												  , 0  // (GetDesktopWindow()), // Parent
												  , NULL // Menu
												  , GetModuleHandle( NULL )
												  , (void *) 1);
		  ShowWindow( hWndInstance, SW_SHOW );

}

int main( void ) {
	MSG msg;

	hCursor = LoadCursor( NULL, IDC_WAIT );

	OpenWindow();
	
	while( GetMessage( &msg, NULL, 0, 0 ) ){
		DispatchMessage( &msg );
	}
}
