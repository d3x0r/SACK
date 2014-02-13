
     

#include <windows.h>								// Header File For Windows
#include "sharemem.h"
#include "GLFrame.h"


static KEYBOARD2 kbd;

LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);				// Declaration For WndProc
int LoadGLTextures();								// Load Bitmaps And Convert To Textures
//int DrawGLScene(GLvoid);								// Here's Where We Do All The Drawing
void SetLighting( void );

PGLFRAME pglRoot;

//------------------------------------------------------------------------

int IsKeyDown( unsigned char c )
{
   return kbd.key[c];
}

//------------------------------------------------------------------------

BOOL KeyDown( unsigned char c )
{
   if( kbd.key[c] && kbd.keydown[c] ) // don't repress...
   {
      return FALSE;
   }
   else
   {
      if( kbd.key[c] )  // if key IS hit
      {
         kbd.keydown[c] = TRUE;  // keydown was not also true...
#ifdef _WIN32
         if( ( kbd.keytime[c] + 250 ) <= GetTickCount() )
#else
         if( ( kbd.keytime[c] + 1 ) <= time( NULL ) )
#endif
            kbd.keydouble[c] = TRUE;
#ifdef _WIN32
         kbd.keytime[c] = GetTickCount();
#else
         kbd.keytime[c] = time(NULL);
#endif
      }
      else
      {
         kbd.keydown[c] = FALSE; // key is NOT down at all...
      }
      return kbd.keydown[c]; // return edge triggered state...
   }

}

//------------------------------------------------------------------------

BOOL KeyDouble( unsigned char c )
{
   if( KeyDown( c) )
      if( kbd.keydouble[c] )
      {
         kbd.keydouble[c] = FALSE;  // clear keydouble....... incomplete...
         return TRUE;
      }
   return FALSE;
}

//------------------------------------------------------------------------


GLvoid ReSizeGLScene(PGLFRAME pglFrame, GLsizei width, GLsizei height)				// Resize And Initialize The GL Window
{
	if (height==0)								// Prevent A Divide By Zero By
	{
		height=1;							// Making Height Equal One
	}

	glViewport(0, 0, width, height);					// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
   if( pglFrame )
   {
      pglFrame->aspect = (GLfloat)width/(GLfloat)height;
	   gluPerspective(45.0f,pglFrame->aspect,0.1f,1000.0f);
   }
   else
	   gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,10000.0f);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix
}


//------------------------------------------------------------------------
int InitGL(GLvoid)								// All Setup For OpenGL Goes Here
{
  	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping ( NEW )

	glShadeModel(GL_SMOOTH);						// Enables Smooth Shading

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);					// Black Background

	glClearDepth(1.0f);							// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);						// Enables Depth Testing
	glDepthFunc(GL_LESS);							// The Type Of Depth Test To Do

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Really Nice Perspective Calculations

   SetLighting();
	return TRUE;								// Initialization Went OK
}

//------------------------------------------------------------------------
GLvoid KillGLWindow(PGLFRAME pglFrame)							// Properly Kill The Window
{

	if (pglFrame->fullscreen)								// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);						// Show Mouse Pointer
	}

	if (pglFrame->hRC)								// Do We Have A Rendering Context?
	{

		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{

			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(pglFrame->hRC))					// Are We Able To Delete The RC?
		{

			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		pglFrame->hRC=NULL;							// Set RC To NULL
	}

	if (pglFrame->hDC && !ReleaseDC(pglFrame->hWnd,pglFrame->hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		pglFrame->hDC=NULL;							// Set DC To NULL
	}

	if (pglFrame->hWnd && !DestroyWindow(pglFrame->hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		pglFrame->hWnd=NULL;							// Set hWnd To NULL
	}
}

//------------------------------------------------------------------------

void ExitGLWindows( void )
{
	if (!UnregisterClass("OpenGLWindow",GetModuleHandle(NULL)))			// Are We Able To Unregister Class
	{
//      if( GetLastError() )
   		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
//		pglFrame->hInstance=NULL;									// Set hInstance To NULL
	}
}
//------------------------------------------------------------------------

BOOL InitGLWindows( void )
{
	WNDCLASS	wc;							// Windows Class Structure
	wc.style		= CS_HREDRAW | CS_VREDRAW | CS_OWNDC|CS_GLOBALCLASS;		// Redraw On Move, And Own DC For Window
	wc.lpfnWndProc		= (WNDPROC) WndProc;				// WndProc Handles Messages
	wc.cbClsExtra		= 0;						// No Extra Window Data
	wc.cbWndExtra		= 0;						// No Extra Window Data
	wc.hInstance		= GetModuleHandle(NULL);					// Set The Instance
	wc.hIcon		= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor		= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;						// No Background Required For GL
	wc.lpszMenuName		= NULL;						// We Don't Want A Menu
	wc.lpszClassName	= "OpenGLWindow";					// Set The Class Name

	if (!RegisterClass(&wc))						// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							// Exit And Return FALSE
	}
   return TRUE;
}

//------------------------------------------------------------------------

static int  WindowBorderOfs_X,  WindowBorderOfs_Y;

BOOL CreateGLWindow(PGLFRAME pglFrame, char* title, int x, int y, int width, int height, int bits, int fullscreenflag)
{

	GLuint		PixelFormat;						// Holds The Results After Searching For A Match


	DWORD		dwExStyle;						// Window Extended Style
	DWORD		dwStyle;						// Window Style

	RECT WindowRect;							// Grabs Rectangle Upper Left / Lower Right Values
   WindowBorderOfs_X = GetSystemMetrics( SM_CXFRAME );
   WindowBorderOfs_Y = GetSystemMetrics( SM_CYFRAME )
                  + GetSystemMetrics( SM_CYCAPTION );
	WindowRect.left=(long)0;						// Set Left Value To 0
	WindowRect.right=(long)width;						// Set Right Value To Requested Width
	WindowRect.top=(long)0;							// Set Top Value To 0
	WindowRect.bottom=(long)height;						// Set Bottom Value To Requested Height
   TransformClear( &pglFrame->T );
   pglFrame->next = pglRoot;
   if( pglRoot )
      pglRoot->prior = pglFrame;
   pglRoot = pglFrame;

	pglFrame->fullscreen=fullscreenflag;						// Set The Global Fullscreen Flag

	if (pglFrame->fullscreen)								// Attempt Fullscreen Mode?
	{

		DEVMODE dmScreenSettings;					// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));		// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;			// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;			// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;				// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{

			// If The Mode Fails, Offer Two Options.  Quit Or Run In A Window.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{

				pglFrame->fullscreen=FALSE;				// Select Windowed Mode (Fullscreen=FALSE)
			}
			else
			{

				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;					// Exit And Return FALSE
			}
		}
	}

	if (pglFrame->fullscreen)								// Are We Still In Fullscreen Mode?
	{

		dwExStyle=WS_EX_APPWINDOW;					// Window Extended Style
		dwStyle=WS_POPUP;						// Windows Style
		ShowCursor(FALSE);						// Hide Mouse Pointer
	}
	else
	{

		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;					// Windows Style
      dwExStyle = 0;
      dwStyle = WS_POPUP|WS_VISIBLE;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	if (!(pglFrame->hWnd=CreateWindowEx(	dwExStyle,				// Extended Style For The Window
					"OpenGLWindow",				// Class Name
					title,					// Window Title
					WS_CLIPSIBLINGS |			// Required Window Style
					WS_CLIPCHILDREN |			// Required Window Style
					dwStyle,				// Selected Window Style
					x, y,					// Window Position
					WindowRect.right-WindowRect.left,	// Calculate Adjusted Window Width
					WindowRect.bottom-WindowRect.top,	// Calculate Adjusted Window Height
					NULL,					// No Parent Window
					NULL,					// No Menu
					GetModuleHandle(NULL),				// Instance
					NULL)))					// Don't Pass Anything To WM_CREATE

	{
      DWORD dwError = GetLastError();
		KillGLWindow(pglFrame);							// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							// Return FALSE
	}
   SetWindowLong( pglFrame->hWnd, GWL_USERDATA, (DWORD)pglFrame );
   {
	   static	PIXELFORMATDESCRIPTOR pfd=					// pfd Tells Windows How We Want Things To Be
	   {
		   sizeof(PIXELFORMATDESCRIPTOR),					// Size Of This Pixel Format Descriptor
		   1,								// Version Number
		   PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		   PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		   PFD_DOUBLEBUFFER,						// Must Support Double Buffering
		   PFD_TYPE_RGBA,							// Request An RGBA Format
		   0,								// Select Our Color Depth
		   0, 0, 0, 0, 0, 0,						// Color Bits Ignored
		   0,								// No Alpha Buffer
		   0,								// Shift Bit Ignored
		   0,								// No Accumulation Buffer
		   0, 0, 0, 0,							// Accumulation Bits Ignored
		   16,								// 16Bit Z-Buffer (Depth Buffer)
		   0,								// No Stencil Buffer
		   0,								// No Auxiliary Buffer
		   PFD_MAIN_PLANE,							// Main Drawing Layer
		   0,								// Reserved
		   0, 0, 0								// Layer Masks Ignored
	   };
      pfd.cColorBits = bits;

	   if (!(pglFrame->hDC=GetDC(pglFrame->hWnd)))							// Did We Get A Device Context?
	   {
		   KillGLWindow(pglFrame);							// Reset The Display
		   MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		   return FALSE;							// Return FALSE
	   }

	   if (!(PixelFormat=ChoosePixelFormat(pglFrame->hDC,&pfd)))				// Did Windows Find A Matching Pixel Format?
	   {
		   KillGLWindow(pglFrame);							// Reset The Display
		   MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		   return FALSE;							// Return FALSE
	   }

	   if(!SetPixelFormat(pglFrame->hDC,PixelFormat,&pfd))				// Are We Able To Set The Pixel Format?
	   {
		   KillGLWindow(pglFrame);							// Reset The Display
		   MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		   return FALSE;							// Return FALSE
	   }
   }

	if (!(pglFrame->hRC=wglCreateContext(pglFrame->hDC)))					// Are We Able To Get A Rendering Context?
	{
		KillGLWindow(pglFrame);							// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							// Return FALSE
	}

	if(!wglMakeCurrent(pglFrame->hDC,pglFrame->hRC))						// Try To Activate The Rendering Context
	{
		KillGLWindow(pglFrame);							// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							// Return FALSE
	}

	ShowWindow(pglFrame->hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(pglFrame->hWnd);						// Slightly Higher Priority
	SetFocus(pglFrame->hWnd);								// Sets Keyboard Focus To The Window
	ReSizeGLScene(pglFrame,width, height);						// Set Up Our Perspective GL Screen

	if (!InitGL())								// Initialize Our Newly Created GL Window
	{
		KillGLWindow(pglFrame);							// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;							// Return FALSE
	}

	return TRUE;								// Success
}

//------------------------------------------------------------------------

LRESULT	CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)				// Declaration For WndProc
{
	switch (uMsg)								// Check For Windows Messages
	{

		case WM_ACTIVATE:						// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				//glFrame.active=TRUE;					// Program Is Active
			}
			else
			{
				//glFrame.active=FALSE;					// Program Is No Longer Active
			}

			return 0;						// Return To The Message Loop
		}

		case WM_SYSCOMMAND:						// Intercept System Commands
		{
			switch (wParam)						// Check System Calls
			{
				case SC_SCREENSAVE:				// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;					// Prevent From Happening
			}
			break;							// Exit
		}

		case WM_CLOSE:							// Did We Receive A Close Message?
		{
			PostQuitMessage(0);					// Send A Quit Message
			return 0;						// Jump Back
		}

		case WM_KEYDOWN:						// Is A Key Being Held Down?
		{
         kbd.key[wParam] = TRUE;
			return 0;						// Jump Back
		}

		case WM_KEYUP:							// Has A Key Been Released?
		{
         kbd.key[wParam] = FALSE;
			return 0;						// Jump Back
		}

		case WM_SIZE:							// Resize The OpenGL Window
		{
			ReSizeGLScene(GetWindowLong( hWnd, GWL_USERDATA ), LOWORD(lParam),HIWORD(lParam));		// LoWord=Width, HiWord=Height
			return 0;						// Jump Back
		}

      {
         PGLFRAME pglFrame;
         if(0) {
   case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
         SetCapture( hWnd );
         }else if(0){
   case WM_LBUTTONUP:
	case WM_RBUTTONUP:
         ReleaseCapture();
         }else if(0){
   case WM_MOUSEMOVE:
      #ifdef __CYGWIN__
   		   asm( "nop" );
      #else
            _asm{ nop; 
            };
      #endif
         }
         pglFrame = (PGLFRAME)GetWindowLong( hWnd, GWL_USERDATA );
         {
            POINT p;
            GetCursorPos( &p );
            p.x -= pglFrame->x;
            // also consider hVideo->height - (y)
            p.y -= pglFrame->y;
            {
               p.x -= WindowBorderOfs_X;
               p.y -= WindowBorderOfs_Y;
            }
            pglFrame->mouse_x = p.x;
            pglFrame->mouse_y = p.y;
            pglFrame->mouse_b = wParam;
         }
         if( pglFrame->MouseMethod )
            pglFrame->MouseMethod( pglFrame->dwMouseData, pglFrame->mouse_x, 
                                    pglFrame->mouse_y, pglFrame->mouse_b ); 
      }
	}


	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}



//------------------------------------------------------------------------

void TrackPopup( PGLFRAME pglFrame, HMENU hMenu )
{
   {
      int nCmd;
      POINT p;
      GetCursorPos( &p );
#ifndef TPM_TOPALIGN
#define TPM_TOPALIGN 0
#endif
#ifndef TPM_NONOTIFY
#define TPM_NONOTIFY 128
#endif
#ifndef TPM_RETURNCMD
#define TPM_RETURNCMD 256
#endif
      nCmd = TrackPopupMenu( (HMENU)hMenu, TPM_CENTERALIGN 
                                 | TPM_TOPALIGN 
                                 | TPM_RIGHTBUTTON 
                                 | TPM_RETURNCMD 
                                 | TPM_NONOTIFY
                                 ,p.x 
                                 ,p.y 
                                 ,0
//#pragma message( "This line must be handled with a good window handle...\n" )
                                 ,pglFrame->hWnd
                                 ,NULL);
      return nCmd;
   }
}

// $Log: OpenGL.c,v $
// Revision 1.3  2003/03/25 09:41:11  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:01  panther
// Added CVS logging
//
