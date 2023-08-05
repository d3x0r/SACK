
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#undef CopyCursor
// simple - doesn't get right resolution.
//#define CopyCursor(icon) (HCURSOR)CopyImage( icon,IMAGE_CURSOR,0, 0,0)

#define CopyCursor(icon) (HCURSOR)CopyImage( icon,IMAGE_CURSOR,0, 0, LR_COPYFROMRESOURCE | LR_DEFAULTSIZE |LR_DEFAULTCOLOR)

// uses the Registry CursorBaseSize tocopy the image...
//#define CopyCursor(icon) (HCURSOR)CopyImage( icon,IMAGE_CURSOR,cursorSize , cursorSize , LR_COPYFROMRESOURCE | LR_DEFAULTSIZE |LR_DEFAULTCOLOR)

// The cursorSize is updated from a registry setting which indicates the base cursor size.
int cursorSize = 32;

HCURSOR hCursor;
#define ALL_CURSORS 17
int oldCursors[ALL_CURSORS] = {
 (int)(uintptr_t)IDC_ARROW,
 (int)(uintptr_t)IDC_IBEAM,
 (int)(uintptr_t)IDC_WAIT,
 (int)(uintptr_t)IDC_CROSS,
 (int)(uintptr_t)IDC_UPARROW,
 (int)(uintptr_t)IDC_SIZE,  /* OBSOLETE: use IDC_SIZEALL */
 (int)(uintptr_t)IDC_ICON,  /* OBSOLETE: use IDC_ARROW */
 (int)(uintptr_t)IDC_SIZENWSE,
 (int)(uintptr_t)IDC_SIZENESW,
 (int)(uintptr_t)IDC_SIZEWE,
 (int)(uintptr_t)IDC_SIZENS,
 (int)(uintptr_t)IDC_SIZEALL,
 (int)(uintptr_t)32647, /*not in win3.1 */
 (int)(uintptr_t)IDC_NO, /*not in win3.1 */
 (int)(uintptr_t)IDC_HAND,
 (int)(uintptr_t)IDC_APPSTARTING, /*not in win3.1 */
 (int)(uintptr_t)IDC_HELP,
};

HCURSOR hOldCursors[ALL_CURSORS];
int isHidden = 0;


void ResetCursor(void) {
	if( isHidden ) {
		for( int i = 0; i < ALL_CURSORS; i++ )
			SetSystemCursor( hOldCursors[i], oldCursors[i]);
	}
}


void GetCursorInfo( void ) {
	DWORD dwStatus;
	HKEY hTemp;
	DWORD dwRetType;
	char pValue[512];
	DWORD dwBufSize;

	dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER,
		"Control Panel\\Cursors", 0,
		KEY_READ, &hTemp );
	dwBufSize = 512;
	dwStatus = RegQueryValueEx( hTemp, "CursorBaseSize", 0
		, &dwRetType
		, (PBYTE)&pValue
		, &dwBufSize );
	if( dwRetType == REG_DWORD ) {
		DWORD size = ((DWORD*)pValue)[0];
		cursorSize = size;
		printf( "size:%d\n", size );
	}
}

DWORD WINAPI hideCursorThread( LPVOID param ) {
	int timeout = 1000; // hide after 1 second idle...

	int64_t now = GetTickCount();
	POINT oldPoint;
	POINT newPoint;
	GetCursorPos( &oldPoint );
	while( 1 ) {
		int64_t newNow = GetTickCount();
		GetCursorPos( &newPoint );
		if( ( newPoint.x != oldPoint.x ) || ( newPoint.y != oldPoint.y ) ) {
			if( isHidden ) {
				for( int i = 0; i < ALL_CURSORS; i++ )
					SetSystemCursor( CopyCursor( hOldCursors[i] ), oldCursors[i] );

				//HCURSOR hPass = CopyCursor( hOldCursor );
				//SetSystemCursor( hPass, 32512/*OCR_NORMAL*/ );
				isHidden = FALSE;
			}
			oldPoint = newPoint;  // update the point position.
			now = newNow;
		} else

		if( ( newNow - now ) > timeout ) {
			if( !isHidden ) {
				for( int i = 0; i < ALL_CURSORS; i++ )
					SetSystemCursor( CopyCursor( hCursor ), oldCursors[i] );
				//HCURSOR hPass = CopyCursor( hCursor );
				//SetSystemCursor( hPass, 32512/*OCR_NORMAL*/ );
				isHidden = TRUE;
			}
			now = newNow;
		}
		Sleep( 100 );
	}
}

static void initBlankCursor( void ) {

	BYTE ANDmaskCursor[] =
	{
		0xff, 0xff, 0xff, 0xff,   // line 1 
		0xff, 0xff, 0xff, 0xff,   // line 2 
		0xff, 0xff, 0xff, 0xff,   // line 3 
		0xff, 0xff, 0xff, 0xff,   // line 4 

		0xff, 0xff, 0xff, 0xff,   // line 5 
		0xff, 0xff, 0xff, 0xff,   // line 6 
		0xff, 0xff, 0xff, 0xff,   // line 7 
		0xff, 0xff, 0xff, 0xff,   // line 8 

		0xff, 0xff, 0xff, 0xff,   // line 9 
		0xff, 0xff, 0xff, 0xff,   // line 10 
		0xff, 0xff, 0xff, 0xff,   // line 11 
		0xff, 0xff, 0xff, 0xff,   // line 12 

		0xff, 0xff, 0xff, 0xff,   // line 13 
		0xff, 0xff, 0xff, 0xff,   // line 14 
		0xff, 0xff, 0xff, 0xff,   // line 15 
		0xff, 0xff, 0xff, 0xff,   // line 16 

		0xff, 0xff, 0xff, 0xff,   // line 17 
		0xff, 0xff, 0xff, 0xff,   // line 18 
		0xff, 0xff, 0xff, 0xff,   // line 19 
		0xff, 0xff, 0xff, 0xff,   // line 20 

		0xff, 0xff, 0xff, 0xff,   // line 21 
		0xff, 0xff, 0xff, 0xff,   // line 22 
		0xff, 0xff, 0xff, 0xff,   // line 23 
		0xff, 0xff, 0xff, 0xff,   // line 24 

		0xff, 0xff, 0xff, 0xff,   // line 25 
		0xff, 0xff, 0xff, 0xff,   // line 26 
		0xff, 0xff, 0xff, 0xff,   // line 27 
		0xff, 0xff, 0xff, 0xff,   // line 28 

		0xff, 0xff, 0xff, 0xff,   // line 29 
		0xff, 0xff, 0xff, 0xff,   // line 30 
		0xff, 0xff, 0xff, 0xff,   // line 31 
		0xff, 0xff, 0xff, 0xff    // line 32 
	};

	// Yin-shaped cursor XOR mask 

	BYTE XORmaskCursor[] =
	{
		0, 0, 0, 0,   // line 1 
		0, 0, 0, 0,   // line 2 
		0, 0, 0, 0,   // line 3 
		0, 0, 0, 0,   // line 4 

		0, 0, 0, 0,   // line 5 
		0, 0, 0, 0,   // line 6 
		0, 0, 0, 0,   // line 7 
		0, 0, 0, 0,   // line 8 

		0, 0, 0, 0,   // line 9 
		0, 0, 0, 0,   // line 10 
		0, 0, 0, 0,   // line 11 
		0, 0, 0, 0,   // line 12 

		0, 0, 0, 0,   // line 13 
		0, 0, 0, 0,   // line 14 
		0, 0, 0, 0,   // line 15 
		0, 0, 0, 0,   // line 16 

		0, 0, 0, 0,   // line 17 
		0, 0, 0, 0,   // line 18 
		0, 0, 0, 0,   // line 19 
		0, 0, 0, 0,   // line 20 

		0, 0, 0, 0,   // line 21 
		0, 0, 0, 0,   // line 22 
		0, 0, 0, 0,   // line 23 
		0, 0, 0, 0,   // line 24 

		0, 0, 0, 0,   // line 25 
		0, 0, 0, 0,   // line 26 
		0, 0, 0, 0,   // line 27 
		0, 0, 0, 0,   // line 28 

		0, 0, 0, 0,   // line 29 
		0, 0, 0, 0,   // line 30 
		0, 0, 0, 0,   // line 31 
		0, 0, 0, 0    // line 32 
	};

	// Create a custom cursor at run time. 
	//return ;
	{
		//int x = SystemParametersInfo( )
		//int x = GetSystemMetrics( SM_CXCURSOR );
		//int y = GetSystemMetrics( SM_CYCURSOR );
		//GetCursorInfo();
		//SystemParametersInfo( )
		//lprintf( "cursor is %d %d", x, y );
		//hOldCursor = CopyCursor( LoadCursor( NULL, IDC_ARROW ) );
		for( int i = 0; i < ALL_CURSORS; i++ ) {
			//if( i == 0 )
			//	hOldCursors[i] = LoadCursorFromFile( "C:\\Users\\d3x0r\\AppData\\Local\\Microsoft\\Windows\\Cursors\\arrow_eoa.cur");
			//else
			hOldCursors[i] = CopyCursor( LoadCursor( NULL, (LPCSTR)(uintptr_t)oldCursors[i] ) );
		}
			//hOldCursors[i] = CopyCursor( LoadImage( NULL, (LPCSTR)(uintptr_t)oldCursors[i], IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE ) );
	}
	//lprintf( "Cursors: %p %p", hOldCursor, hOldCursor2 );
	hCursor = CreateCursor( GetModuleHandle( NULL ),   // app. instance 
		19,                // horizontal position of hot spot 
		2,                 // vertical position of hot spot 
		32,                // cursor width 
		32,                // cursor height 
		ANDmaskCursor,     // AND mask 
		XORmaskCursor );   // XOR mask 

	return;
}



int main( void ) {
	uint32_t tick = GetTickCount();

	atexit( ResetCursor );
	GetCursorInfo();
	initBlankCursor();
	CreateThread( NULL, 0, hideCursorThread, NULL, 0, 0 );
	
	while( (GetTickCount() - tick) < 15000 ) 
		Sleep( 1000 );

	return 0;		
}
