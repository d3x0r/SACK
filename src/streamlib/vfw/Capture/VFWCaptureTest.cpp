
#include "bmpUtils.h"
#include "Resource.h"

char szAppName [] = "WebcamCapture";

LRESULT CALLBACK WindowProc (HWND, UINT, WPARAM, LPARAM);


// Windows Main Function. 
// 
int WINAPI WinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance,
					LPSTR lpCmdLine, int nCmdShow )
{
	HWND hwnd;
	MSG msg;

	WNDCLASS wc;
	wc.style = CS_HREDRAW|CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor (NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
	wc.lpszMenuName = MAKEINTRESOURCE( IDR_MENU1 );
	wc.lpszClassName = szAppName;

	RegisterClass (&wc);

	hwnd = CreateWindow (szAppName,"Webcam Capture",WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
		0,0,hInstance,0);

	ShowWindow (hwnd,nCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage(&msg,0,0,0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}

	return msg.wParam;
}


// Main Window Procedure.
// Processes Window Messages
LRESULT CALLBACK WindowProc
	(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static CbmpUtils bmp;
	HDC hdc;
	PAINTSTRUCT ps;
	int wmId, wmEvent;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam); // Remember, these are...
        wmEvent = HIWORD(wParam); // ...different for Win32!

         //Parse the menu selections:
         switch (wmId)
		 {
		 case ID_EXIT:
			DestroyWindow (hWnd);
			break;

		 case ID_GRABFRAME:
			bmp.LoadBMP();
			InvalidateRect(hWnd,NULL,TRUE);
			break;

		 case ID_SAVEFRAME:
			bmp.SaveBMP("a.bmp");
			MessageBox (hWnd, "File a.bmp saved",
                              szAppName, MB_OK|MB_ICONINFORMATION);
			break;
		
		 default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
		 }
         break;

	case WM_CREATE:
		bmp.LoadBMP();
		break;
		 
	case WM_PAINT:
		hdc=BeginPaint (hWnd,&ps);
		bmp.GDIPaint (hdc,10,10);
		EndPaint (hWnd,&ps);
		break;

	case WM_DESTROY:
		PostQuitMessage (0);
		break;

	default:
         return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return(0);
}
