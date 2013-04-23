#define NEED_V4W
#include <stdhdrs.h>


#include <system.h>
#include <plugin.h>

//pGraph->lpVtbl->QueryInterface(pGraph, &IID_IMediaEvent, (void **)&pEvent);
//The following shows the equivalent call in C++:
//pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);



extern int myTypeID;

typedef struct mydatapath_tag {
	DATAPATH common;

} MYDATAPATH, *PMYDATAPATH;


static int CPROC Read(PDATAPATH pmdp)
{
   return 0;
}

static int CPROC Close(PDATAPATH pmdp)
{
   pmdp->Type = 0; // allow close.
   return 0;
}

#ifndef VFWAPI
#define  VFWAPI STDPROC
#endif
HWND VFWAPI (*MycapCreateCaptureWindowA) (
        LPCSTR lpszWindowName,
        DWORD dwStyle,
        int x, int y, int nWidth, int nHeight,
												 HWND hwndParent, int nID);

BOOL VFWAPI (*MycapGetDriverDescriptionA )(UINT wDriverIndex,
														 LPSTR lpszName, int cbName,
														 LPSTR lpszVer, int cbVer);


												 // FrameCallbackProc: frame callback function
												 // hWnd:              capture window handle
// lpVHdr:            pointer to struct containing captured 
//                    frame information 
// 
												 LRESULT PASCAL FrameCallbackProc(HWND hWnd, LPVIDEOHDR lpVHdr)
												 {
													 char buf[256];
                                        static DWORD gdwFrameNum;
    if (!hWnd) 
        return FALSE; 

    lprintf(buf, "Preview frame# %ld ", gdwFrameNum++);
    //SetWindowText(hWnd, (LPSTR)buf);
    return (LRESULT) TRUE ; 
} 


LRESULT CALLBACK MycapVideoStreamCallback(
  HWND hWnd,         
  LPVIDEOHDR lpVHdr  
													)
{
   return 0;
}

static int CALLBACK CaptureFrameWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
//   HVIDEO hVideo;

   switch( uMsg )
   {
   case WM_DESTROY:
      {
         PostQuitMessage( 0xD1E );
      }
      break;
   case WM_CREATE:
		{
         lprintf( "Created window..." );
      }
      break;
   }

   return DefWindowProc( hWnd, uMsg, wParam, lParam );
}



//----------------------------------------------------------------------------
static HWND RegisterWindows( void )
{
   HWND hWndFrame;
	static ATOM aClassFrame;
   extern HINSTANCE hInstMe;
	WNDCLASS wc;
//   WindowBorderHeight = ( GetSystemMetrics( SM_CYBORDER ) * 2 )
//                      + GetSystemMetrics( SM_CYCAPTION );
   if( !aClassFrame )
   {
      memset( &wc, 0, sizeof(WNDCLASS) );
      wc.style = CS_OWNDC | CS_GLOBALCLASS;

      wc.lpfnWndProc = (WNDPROC)CaptureFrameWindowProc;
      wc.hInstance = hInstMe; // GetModuleHandle(NULL);
      wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1);
      wc.hCursor = LoadCursor( NULL, IDC_ARROW );
      wc.lpszClassName = "CaptureFrameClass";
      wc.lpszMenuName = "FRAME_MENU";
      wc.cbWndExtra = 4;  // one extra DWORD

      aClassFrame = RegisterClass( &wc );
      if( !aClassFrame )
         return FALSE;
   }
   hWndFrame = CreateWindow( "CaptureFrameClass",
                 "Capture Preview?",
                 (WS_VISIBLE|WS_OVERLAPPEDWINDOW),
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 NULL, // Parent
                 NULL, // Menu
                 hInstMe, // GetModuleHandle(NULL),
									 NULL );
   return hWndFrame;
}

PDATAPATH CPROC CreateVideoCapture( PDATAPATH *pChannel, PSENTIENT ps, PTEXT params )
{
   PMYDATAPATH pdp = NULL;
   //PTEXT option;
   // parameters
	//    none
	if( !MycapCreateCaptureWindowA )
		MycapCreateCaptureWindowA =(HWND  (VFWAPI*)(LPCSTR,DWORD,int,int,int,int,
												 HWND,int))LoadFunction( "avicap32.dll", "capCreateCaptureWindowA" );
	if( !MycapGetDriverDescriptionA )
      MycapGetDriverDescriptionA = (BOOL  (VFWAPI*)(UINT,LPSTR,int,LPSTR,int))LoadFunction( "avicap32.dll", "capGetDriverDescriptionA" );
   pdp = CreateDataPath( pChannel, MYDATAPATH );
   pdp->common.Type = myTypeID;
   pdp->common.Read = Read;
   pdp->common.Write = NULL;
	pdp->common.Close = Close;

  // DebugBreak();
	{
		HWND hwndParent = RegisterWindows();
		int nID = 1055;
      int fOK;
		HWND hWndC = MycapCreateCaptureWindowA (
														 (LPSTR) "My Capture Window", // window name if pop-up
														 WS_CHILD | WS_VISIBLE,       // window style
														 0, 0, 160, 120,              // window position and dimensions
														 (HWND)hwndParent,
															 (int) nID /* child ID */);
		lprintf( "Parent Window: %p %p", hWndC, hwndParent );
		{

			char szDeviceName[80];
			char szDeviceVersion[80];
         INDEX wIndex;
			for (wIndex = 0; wIndex < 10; wIndex++)
			{
				lprintf( "1" );
				if (MycapGetDriverDescriptionA (wIndex, szDeviceName,
													  sizeof (szDeviceName), szDeviceVersion,
													  sizeof (szDeviceVersion)))
				{
					// Append name to list of installed capture drivers
					// and then let the user select a driver to use.
					lprintf( "Strings: %d %s %s", wIndex, szDeviceName, szDeviceVersion );
				}
				//else
            //   break;

			}
		}
      lprintf( "1" );
		//fOK = SendMessage (hWndC, WM_CAP_DRIVER_CONNECT, 0, 0L);
		//
		// Or, use the macro to connect to the MSVIDEO driver:

		{
			CAPTUREPARMS CapParams;
         DebugBreak();
			capCaptureGetSetup( hWndC, &CapParams, sizeof( CapParams ) );

			capCaptureSetSetup( hWndC, &CapParams, sizeof( CapParams ) );
		}
		//
		// Place code to set up and capture video here.
		//
		// capDriverDisconnect (hWndC);
      lprintf( "1" );
		capSetCallbackOnVideoStream(hWndC, MycapVideoStreamCallback);
      lprintf( "1" );
      capSetCallbackOnFrame( hWndC, FrameCallbackProc );
      lprintf( "1" );

		{
			CAPDRIVERCAPS CapDrvCaps;

			SendMessage (hWndC, WM_CAP_DRIVER_GET_CAPS,
							 sizeof (CAPDRIVERCAPS), (LONG) (LPVOID) &CapDrvCaps);

			// Or, use the macro to retrieve the driver capabilities.
			// capDriverGetCaps(hWndC, &CapDrvCaps, sizeof (CAPDRIVERCAPS));
		}
		fOK = capDriverConnect(hWndC, 0);
		if( !fOK )
         lprintf( "Failed driver connect..." );
      lprintf( "1" );
		{
			CAPSTATUS CapStatus;
		  // DebugBreak();
			{
				int n;
            lprintf( "size is %d", sizeof( CAPSTATUS ) );
				for( n = 0; n < 20 * sizeof( CAPSTATUS ); n++ )
					if( capGetStatus(hWndC, &CapStatus, n) )
                  lprintf( "%d success! %d natural", n, sizeof( CAPSTATUS ) );

			}
			capGetStatus(hWndC, &CapStatus, sizeof (CAPSTATUS));
			lprintf( "result : %d", GetLastError() );
         lprintf( "Frame is %d by %d",CapStatus.uiImageWidth,
							 CapStatus.uiImageHeight );
			SetWindowPos(hWndC, NULL, 0, 0, CapStatus.uiImageWidth,
							 CapStatus.uiImageHeight, SWP_NOZORDER | SWP_NOMOVE);
		}
      lprintf( "1" );

		capPreviewRate(hWndC, 66);     // rate, in milliseconds
		capPreview(hWndC, TRUE);       // starts preview

		{
			CAPDRIVERCAPS CapDrvCaps;

			capDriverGetCaps(hWndC, &CapDrvCaps, sizeof (CAPDRIVERCAPS));

			if (CapDrvCaps.fHasOverlay)
				capOverlay(hWndC, TRUE);
		}
      lprintf( "1" );
      ShowWindow( hWndC, SW_NORMAL );
      lprintf( "1" );
	  // fOK = capDriverConnect(hWndC, 0);
      lprintf( "1" );
	}
	return (PDATAPATH)pdp;
}


//--------------------------------------------------------------------
//
// $Log: vidcap.c,v $
// Revision 1.2  2004/04/05 21:15:50  d3x0r
// Sweeping update - macros, probably some logging enabled
//
// Revision 1.1  2004/04/05 15:41:51  d3x0r
// *** empty log message ***
//
