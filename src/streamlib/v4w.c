#define NEED_V4W
//#define USE_IMAGE_INTERFACE g.pii
#include <stdhdrs.h>
#include <image.h>
#include <vfw.h>


#include "streamstruct.h"
#include "global.h"

extern GLOBAL g;

#if __WATCOMC__ <1280
typedef struct videohdr_tag {
	LPBYTE      lpData;
	DWORD       dwBufferLength;
	DWORD       dwBytesUsed;
	DWORD       dwTimeCaptured;
	DWORD       dwUser;
	DWORD       dwFlags;
	DWORD_PTR   dwReserved[4];
} VIDEOHDR, NEAR *PVIDEOHDR, FAR * LPVIDEOHDR;

typedef struct capture_parms CAPTUREPARMS;
struct capture_parms{     DWORD dwRequestMicroSecPerFrame;     BOOL  fMakeUserHitOKToCapture;     UINT  wPercentDropForError;     BOOL  fYield;     DWORD dwIndexSize;     UINT  wChunkGranularity;     BOOL  fUsingDOSMemory;     UINT  wNumVideoRequested;     BOOL  fCaptureAudio;     UINT  wNumAudioRequested;     UINT  vKeyAbort;     BOOL  fAbortLeftMouse;     BOOL  fAbortRightMouse;     BOOL  fLimitEnabled;     UINT  wTimeLimit;     BOOL  fMCIControl;     BOOL  fStepMCIDevice;     DWORD dwMCIStartTime;     DWORD dwMCIStopTime;     BOOL  fStepCaptureAt2x;     UINT  wStepCaptureAverageFrames;     DWORD dwAudioBufferSize;     BOOL  fDisableWriteCache;     UINT  AVStreamMaster; } ;

typedef struct capture_parms *LPCAPTUREPARMS;
// ------------------------------------------------------------------
//  Window Messages  WM_CAP... which can be sent to an AVICAP window
// ------------------------------------------------------------------



// UNICODE
//
// The Win32 version of AVICAP on NT supports UNICODE applications:
// for each API or message that takes a char or string parameter, there are
// two versions, ApiNameA and ApiNameW. The default name ApiName is #defined
// to one or other depending on whether UNICODE is defined. Apps can call
// the A and W apis directly, and mix them.
//
// The 32-bit AVICAP on NT uses unicode exclusively internally.
// ApiNameA() will be implemented as a call to ApiNameW() together with
// translation of strings.




// Defines start of the message range
#define WM_CAP_START                    WM_USER

// start of unicode messages
#define WM_CAP_UNICODE_START            WM_USER+100

#define WM_CAP_GET_CAPSTREAMPTR         (WM_CAP_START+  1)

#define WM_CAP_SET_CALLBACK_ERRORW     (WM_CAP_UNICODE_START+  2)
#define WM_CAP_SET_CALLBACK_STATUSW    (WM_CAP_UNICODE_START+  3)
#define WM_CAP_SET_CALLBACK_ERRORA     (WM_CAP_START+  2)
#define WM_CAP_SET_CALLBACK_STATUSA    (WM_CAP_START+  3)
#ifdef UNICODE
#define WM_CAP_SET_CALLBACK_ERROR       WM_CAP_SET_CALLBACK_ERRORW
#define WM_CAP_SET_CALLBACK_STATUS      WM_CAP_SET_CALLBACK_STATUSW
#else
#define WM_CAP_SET_CALLBACK_ERROR       WM_CAP_SET_CALLBACK_ERRORA
#define WM_CAP_SET_CALLBACK_STATUS      WM_CAP_SET_CALLBACK_STATUSA
#endif


#define WM_CAP_SET_CALLBACK_YIELD       (WM_CAP_START+  4)
#define WM_CAP_SET_CALLBACK_FRAME       (WM_CAP_START+  5)
#define WM_CAP_SET_CALLBACK_VIDEOSTREAM (WM_CAP_START+  6)
#define WM_CAP_SET_CALLBACK_WAVESTREAM  (WM_CAP_START+  7)
#define WM_CAP_GET_USER_DATA		(WM_CAP_START+  8)
#define WM_CAP_SET_USER_DATA		(WM_CAP_START+  9)

#define WM_CAP_DRIVER_CONNECT           (WM_CAP_START+  10)
#define WM_CAP_DRIVER_DISCONNECT        (WM_CAP_START+  11)

#define WM_CAP_DRIVER_GET_NAMEA        (WM_CAP_START+  12)
#define WM_CAP_DRIVER_GET_VERSIONA     (WM_CAP_START+  13)
#define WM_CAP_DRIVER_GET_NAMEW        (WM_CAP_UNICODE_START+  12)
#define WM_CAP_DRIVER_GET_VERSIONW     (WM_CAP_UNICODE_START+  13)
#ifdef UNICODE
#define WM_CAP_DRIVER_GET_NAME          WM_CAP_DRIVER_GET_NAMEW
#define WM_CAP_DRIVER_GET_VERSION       WM_CAP_DRIVER_GET_VERSIONW
#else
#define WM_CAP_DRIVER_GET_NAME          WM_CAP_DRIVER_GET_NAMEA
#define WM_CAP_DRIVER_GET_VERSION       WM_CAP_DRIVER_GET_VERSIONA
#endif

#define WM_CAP_DRIVER_GET_CAPS          (WM_CAP_START+  14)

#define WM_CAP_FILE_SET_CAPTURE_FILEA  (WM_CAP_START+  20)
#define WM_CAP_FILE_GET_CAPTURE_FILEA  (WM_CAP_START+  21)
#define WM_CAP_FILE_SAVEASA            (WM_CAP_START+  23)
#define WM_CAP_FILE_SAVEDIBA           (WM_CAP_START+  25)
#define WM_CAP_FILE_SET_CAPTURE_FILEW  (WM_CAP_UNICODE_START+  20)
#define WM_CAP_FILE_GET_CAPTURE_FILEW  (WM_CAP_UNICODE_START+  21)
#define WM_CAP_FILE_SAVEASW            (WM_CAP_UNICODE_START+  23)
#define WM_CAP_FILE_SAVEDIBW           (WM_CAP_UNICODE_START+  25)
#ifdef UNICODE
#define WM_CAP_FILE_SET_CAPTURE_FILE    WM_CAP_FILE_SET_CAPTURE_FILEW
#define WM_CAP_FILE_GET_CAPTURE_FILE    WM_CAP_FILE_GET_CAPTURE_FILEW
#define WM_CAP_FILE_SAVEAS              WM_CAP_FILE_SAVEASW
#define WM_CAP_FILE_SAVEDIB             WM_CAP_FILE_SAVEDIBW
#else
#define WM_CAP_FILE_SET_CAPTURE_FILE    WM_CAP_FILE_SET_CAPTURE_FILEA
#define WM_CAP_FILE_GET_CAPTURE_FILE    WM_CAP_FILE_GET_CAPTURE_FILEA
#define WM_CAP_FILE_SAVEAS              WM_CAP_FILE_SAVEASA
#define WM_CAP_FILE_SAVEDIB             WM_CAP_FILE_SAVEDIBA
#endif

// out of order to save on ifdefs
#define WM_CAP_FILE_ALLOCATE            (WM_CAP_START+  22)
#define WM_CAP_FILE_SET_INFOCHUNK       (WM_CAP_START+  24)

#define WM_CAP_EDIT_COPY                (WM_CAP_START+  30)

#define WM_CAP_SET_AUDIOFORMAT          (WM_CAP_START+  35)
#define WM_CAP_GET_AUDIOFORMAT          (WM_CAP_START+  36)

#define WM_CAP_DLG_VIDEOFORMAT          (WM_CAP_START+  41)
#define WM_CAP_DLG_VIDEOSOURCE          (WM_CAP_START+  42)
#define WM_CAP_DLG_VIDEODISPLAY         (WM_CAP_START+  43)
#define WM_CAP_GET_VIDEOFORMAT          (WM_CAP_START+  44)
#define WM_CAP_SET_VIDEOFORMAT          (WM_CAP_START+  45)
#define WM_CAP_DLG_VIDEOCOMPRESSION     (WM_CAP_START+  46)

#define WM_CAP_SET_PREVIEW              (WM_CAP_START+  50)
#define WM_CAP_SET_OVERLAY              (WM_CAP_START+  51)
#define WM_CAP_SET_PREVIEWRATE          (WM_CAP_START+  52)
#define WM_CAP_SET_SCALE                (WM_CAP_START+  53)
#define WM_CAP_GET_STATUS               (WM_CAP_START+  54)
#define WM_CAP_SET_SCROLL               (WM_CAP_START+  55)

#define WM_CAP_GRAB_FRAME               (WM_CAP_START+  60)
#define WM_CAP_GRAB_FRAME_NOSTOP        (WM_CAP_START+  61)

#define WM_CAP_SEQUENCE                 (WM_CAP_START+  62)
#define WM_CAP_SEQUENCE_NOFILE          (WM_CAP_START+  63)
#define WM_CAP_SET_SEQUENCE_SETUP       (WM_CAP_START+  64)
#define WM_CAP_GET_SEQUENCE_SETUP       (WM_CAP_START+  65)

#define WM_CAP_SET_MCI_DEVICEA         (WM_CAP_START+  66)
#define WM_CAP_GET_MCI_DEVICEA         (WM_CAP_START+  67)
#define WM_CAP_SET_MCI_DEVICEW         (WM_CAP_UNICODE_START+  66)
#define WM_CAP_GET_MCI_DEVICEW         (WM_CAP_UNICODE_START+  67)
#ifdef UNICODE
#define WM_CAP_SET_MCI_DEVICE           WM_CAP_SET_MCI_DEVICEW
#define WM_CAP_GET_MCI_DEVICE           WM_CAP_GET_MCI_DEVICEW
#else
#define WM_CAP_SET_MCI_DEVICE           WM_CAP_SET_MCI_DEVICEA
#define WM_CAP_GET_MCI_DEVICE           WM_CAP_GET_MCI_DEVICEA
#endif



#define WM_CAP_STOP                     (WM_CAP_START+  68)
#define WM_CAP_ABORT                    (WM_CAP_START+  69)

#define WM_CAP_SINGLE_FRAME_OPEN        (WM_CAP_START+  70)
#define WM_CAP_SINGLE_FRAME_CLOSE       (WM_CAP_START+  71)
#define WM_CAP_SINGLE_FRAME             (WM_CAP_START+  72)

#define WM_CAP_PAL_OPENA               (WM_CAP_START+  80)
#define WM_CAP_PAL_SAVEA               (WM_CAP_START+  81)
#define WM_CAP_PAL_OPENW               (WM_CAP_UNICODE_START+  80)
#define WM_CAP_PAL_SAVEW               (WM_CAP_UNICODE_START+  81)
#ifdef UNICODE
#define WM_CAP_PAL_OPEN                 WM_CAP_PAL_OPENW
#define WM_CAP_PAL_SAVE                 WM_CAP_PAL_SAVEW
#else
#define WM_CAP_PAL_OPEN                 WM_CAP_PAL_OPENA
#define WM_CAP_PAL_SAVE                 WM_CAP_PAL_SAVEA
#endif

#define WM_CAP_PAL_PASTE                (WM_CAP_START+  82)
#define WM_CAP_PAL_AUTOCREATE           (WM_CAP_START+  83)
#define WM_CAP_PAL_MANUALCREATE         (WM_CAP_START+  84)

// Following added post VFW 1.1
#define WM_CAP_SET_CALLBACK_CAPCONTROL  (WM_CAP_START+  85)


// Defines end of the message range
#define WM_CAP_UNICODE_END              WM_CAP_PAL_SAVEW
#define WM_CAP_END                      WM_CAP_UNICODE_END

// ------------------------------------------------------------------
//  Message crackers for above
// ------------------------------------------------------------------

// message wrapper macros are defined for the default messages only. Apps
// that wish to mix Ansi and UNICODE message sending will have to
// reference the _A and _W messages directly

#ifdef __cplusplus
/* SendMessage in C++*/
#define AVICapSM(hwnd,m,w,l) ( (::IsWindow(hwnd)) ? ::SendMessage(hwnd,m,w,l) : 0)
#else
/* SendMessage in C */
#define AVICapSM(hwnd,m,w,l) ( (IsWindow(hwnd)) ?   SendMessage(hwnd,m,w,l) : 0)
#endif  /* __cplusplus */


#define capSetCallbackOnError(hwnd, fpProc)        ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_ERROR, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnStatus(hwnd, fpProc)       ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_STATUS, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnYield(hwnd, fpProc)        ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_YIELD, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnFrame(hwnd, fpProc)        ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_FRAME, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnVideoStream(hwnd, fpProc)  ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_VIDEOSTREAM, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnWaveStream(hwnd, fpProc)   ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_WAVESTREAM, 0, (LPARAM)(LPVOID)(fpProc)))
#define capSetCallbackOnCapControl(hwnd, fpProc)   ((BOOL)AVICapSM(hwnd, WM_CAP_SET_CALLBACK_CAPCONTROL, 0, (LPARAM)(LPVOID)(fpProc)))

#define capSetUserData(hwnd, lUser)        ((BOOL)AVICapSM(hwnd, WM_CAP_SET_USER_DATA, 0, (LPARAM)lUser))
#define capGetUserData(hwnd)               (AVICapSM(hwnd, WM_CAP_GET_USER_DATA, 0, 0))

#define capDriverConnect(hwnd, i)                  ((BOOL)AVICapSM(hwnd, WM_CAP_DRIVER_CONNECT, (WPARAM)(i), 0L))
#define capDriverDisconnect(hwnd)                  ((BOOL)AVICapSM(hwnd, WM_CAP_DRIVER_DISCONNECT, (WPARAM)0, 0L))
#define capDriverGetName(hwnd, szName, wSize)      ((BOOL)AVICapSM(hwnd, WM_CAP_DRIVER_GET_NAME, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capDriverGetVersion(hwnd, szVer, wSize)    ((BOOL)AVICapSM(hwnd, WM_CAP_DRIVER_GET_VERSION, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPTSTR)(szVer)))
#define capDriverGetCaps(hwnd, s, wSize)           ((BOOL)AVICapSM(hwnd, WM_CAP_DRIVER_GET_CAPS, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPDRIVERCAPS)(s)))

#define capFileSetCaptureFile(hwnd, szName)        ((BOOL)AVICapSM(hwnd, WM_CAP_FILE_SET_CAPTURE_FILE, 0, (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capFileGetCaptureFile(hwnd, szName, wSize) ((BOOL)AVICapSM(hwnd, WM_CAP_FILE_GET_CAPTURE_FILE, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capFileAlloc(hwnd, dwSize)                 ((BOOL)AVICapSM(hwnd, WM_CAP_FILE_ALLOCATE, 0, (LPARAM)(DWORD)(dwSize)))
#define capFileSaveAs(hwnd, szName)                ((BOOL)AVICapSM(hwnd, WM_CAP_FILE_SAVEAS, 0, (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capFileSetInfoChunk(hwnd, lpInfoChunk)     ((BOOL)AVICapSM(hwnd, WM_CAP_FILE_SET_INFOCHUNK, (WPARAM)0, (LPARAM)(LPCAPINFOCHUNK)(lpInfoChunk)))
#define capFileSaveDIB(hwnd, szName)               ((BOOL)AVICapSM(hwnd, WM_CAP_FILE_SAVEDIB, 0, (LPARAM)(LPVOID)(LPTSTR)(szName)))

#define capEditCopy(hwnd)                          ((BOOL)AVICapSM(hwnd, WM_CAP_EDIT_COPY, 0, 0L))

#define capSetAudioFormat(hwnd, s, wSize)          ((BOOL)AVICapSM(hwnd, WM_CAP_SET_AUDIOFORMAT, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPWAVEFORMATEX)(s)))
#define capGetAudioFormat(hwnd, s, wSize)          ((DWORD)AVICapSM(hwnd, WM_CAP_GET_AUDIOFORMAT, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPWAVEFORMATEX)(s)))
#define capGetAudioFormatSize(hwnd)                ((DWORD)AVICapSM(hwnd, WM_CAP_GET_AUDIOFORMAT, (WPARAM)0, (LPARAM)0L))

#define capDlgVideoFormat(hwnd)                    ((BOOL)AVICapSM(hwnd, WM_CAP_DLG_VIDEOFORMAT, 0, 0L))
#define capDlgVideoSource(hwnd)                    ((BOOL)AVICapSM(hwnd, WM_CAP_DLG_VIDEOSOURCE, 0, 0L))
#define capDlgVideoDisplay(hwnd)                   ((BOOL)AVICapSM(hwnd, WM_CAP_DLG_VIDEODISPLAY, 0, 0L))
#define capDlgVideoCompression(hwnd)               ((BOOL)AVICapSM(hwnd, WM_CAP_DLG_VIDEOCOMPRESSION, 0, 0L))

#define capGetVideoFormat(hwnd, s, wSize)          ((DWORD)AVICapSM(hwnd, WM_CAP_GET_VIDEOFORMAT, (WPARAM)(wSize), (LPARAM)(LPVOID)(s)))
#define capGetVideoFormatSize(hwnd)            ((DWORD)AVICapSM(hwnd, WM_CAP_GET_VIDEOFORMAT, 0, 0L))
#define capSetVideoFormat(hwnd, s, wSize)          ((BOOL)AVICapSM(hwnd, WM_CAP_SET_VIDEOFORMAT, (WPARAM)(wSize), (LPARAM)(LPVOID)(s)))

#define capPreview(hwnd, f)                        ((BOOL)AVICapSM(hwnd, WM_CAP_SET_PREVIEW, (WPARAM)(BOOL)(f), 0L))
#define capPreviewRate(hwnd, wMS)                  ((BOOL)AVICapSM(hwnd, WM_CAP_SET_PREVIEWRATE, (WPARAM)(wMS), 0))
#define capOverlay(hwnd, f)                        ((BOOL)AVICapSM(hwnd, WM_CAP_SET_OVERLAY, (WPARAM)(BOOL)(f), 0L))
#define capPreviewScale(hwnd, f)                   ((BOOL)AVICapSM(hwnd, WM_CAP_SET_SCALE, (WPARAM)(BOOL)f, 0L))
#define capGetStatus(hwnd, s, wSize)               ((BOOL)AVICapSM(hwnd, WM_CAP_GET_STATUS, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPSTATUS)(s)))
#define capSetScrollPos(hwnd, lpP)                 ((BOOL)AVICapSM(hwnd, WM_CAP_SET_SCROLL, (WPARAM)0, (LPARAM)(LPPOINT)(lpP)))

#define capGrabFrame(hwnd)                         ((BOOL)AVICapSM(hwnd, WM_CAP_GRAB_FRAME, (WPARAM)0, (LPARAM)0L))
#define capGrabFrameNoStop(hwnd)                   ((BOOL)AVICapSM(hwnd, WM_CAP_GRAB_FRAME_NOSTOP, (WPARAM)0, (LPARAM)0L))

#define capCaptureSequence(hwnd)                   ((BOOL)AVICapSM(hwnd, WM_CAP_SEQUENCE, (WPARAM)0, (LPARAM)0L))
#define capCaptureSequenceNoFile(hwnd)             ((BOOL)AVICapSM(hwnd, WM_CAP_SEQUENCE_NOFILE, (WPARAM)0, (LPARAM)0L))
#define capCaptureStop(hwnd)                       ((BOOL)AVICapSM(hwnd, WM_CAP_STOP, (WPARAM)0, (LPARAM)0L))
#define capCaptureAbort(hwnd)                      ((BOOL)AVICapSM(hwnd, WM_CAP_ABORT, (WPARAM)0, (LPARAM)0L))

#define capCaptureSingleFrameOpen(hwnd)            ((BOOL)AVICapSM(hwnd, WM_CAP_SINGLE_FRAME_OPEN, (WPARAM)0, (LPARAM)0L))
#define capCaptureSingleFrameClose(hwnd)           ((BOOL)AVICapSM(hwnd, WM_CAP_SINGLE_FRAME_CLOSE, (WPARAM)0, (LPARAM)0L))
#define capCaptureSingleFrame(hwnd)                ((BOOL)AVICapSM(hwnd, WM_CAP_SINGLE_FRAME, (WPARAM)0, (LPARAM)0L))

#define capCaptureGetSetup(hwnd, s, wSize)         ((BOOL)AVICapSM(hwnd, WM_CAP_GET_SEQUENCE_SETUP, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPTUREPARMS)(s)))
#define capCaptureSetSetup(hwnd, s, wSize)         ((BOOL)AVICapSM(hwnd, WM_CAP_SET_SEQUENCE_SETUP, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPCAPTUREPARMS)(s)))

#define capSetMCIDeviceName(hwnd, szName)          ((BOOL)AVICapSM(hwnd, WM_CAP_SET_MCI_DEVICE, 0, (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capGetMCIDeviceName(hwnd, szName, wSize)   ((BOOL)AVICapSM(hwnd, WM_CAP_GET_MCI_DEVICE, (WPARAM)(wSize), (LPARAM)(LPVOID)(LPTSTR)(szName)))

#define capPaletteOpen(hwnd, szName)               ((BOOL)AVICapSM(hwnd, WM_CAP_PAL_OPEN, 0, (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capPaletteSave(hwnd, szName)               ((BOOL)AVICapSM(hwnd, WM_CAP_PAL_SAVE, 0, (LPARAM)(LPVOID)(LPTSTR)(szName)))
#define capPalettePaste(hwnd)                      ((BOOL)AVICapSM(hwnd, WM_CAP_PAL_PASTE, (WPARAM) 0, (LPARAM)0L))
#define capPaletteAuto(hwnd, iFrames, iColors)     ((BOOL)AVICapSM(hwnd, WM_CAP_PAL_AUTOCREATE, (WPARAM)(iFrames), (LPARAM)(DWORD)(iColors)))
#define capPaletteManual(hwnd, fGrab, iColors)     ((BOOL)AVICapSM(hwnd, WM_CAP_PAL_MANUALCREATE, (WPARAM)(fGrab), (LPARAM)(DWORD)(iColors)))


#endif

static struct {
   Image image;
} l;

LRESULT CALLBACK MycapStatusCallback(
  HWND hWnd,  
  int nID,    
  LPCSTR lpsz 
												)
{
	lprintf( "status: %d %s", nID, lpsz );
   return 1;
}


LRESULT CALLBACK MycapErrorCallback(
  HWND hWnd,  
  int nID,    
  LPCSTR lpsz 
											  )
{
   lprintf( "error %d %s", nID, lpsz );
   return 1;
}


LRESULT CALLBACK MycapCapControlCallback(
  HWND hWnd, int nState
)
{
lprintf( "Control message? %d", nState );
return 1;
}
LRESULT CALLBACK MycapYieldCallback(
  HWND hWnd  
											)
{
   Relinquish();
// result 0 to stop capture...
   return 1;
}

LRESULT CALLBACK handle_frame( HWND hWndCap, LPVIDEOHDR lpVhdr )
{
//   lprintf( "Received frame..." );
	//l.image = RemakeImage( l.image, lpVhdr->lpData, l.image->width, l.image->height );
	{
		P_8 data = lpVhdr->lpData;
		int x, y;
		PCDATA p = GetImageSurface( l.image );
		for( y = 0; y < l.image->width; y++ )
		{
			for( x = 0; x < l.image->height; x++ )
			{
				p[0] = AColor( data[2], data[1], data[0], 255 );
				p++;
				data += 3;
			}
		}
	}
	{

		PCAPTURE_DEVICE pDevice = (PCAPTURE_DEVICE)GetWindowLong( hWndCap, GWL_USERDATA );
      if( pDevice )
		{
			PCAPTURE_CALLBACK callback;
			for( callback = pDevice->callbacks; callback; callback = NextLink( callback ) )
			{
				pDevice->data = l.image;
				//lprintf( WIDE("Process callback: %p"), callback );
				//lprintf( WIDE("Process callback: %p (%p)"), callback, callback->callback );
				if( !callback->callback( callback->psv, pDevice ) )
					break;
				//lprintf( WIDE("Processed callback: %p (%p)"), callback, callback->callback );
			}
		}
	}
//	lprintf( "done..." );
	return 1;
}

static HWND hWndCap;

int filter1(void)
{
   return EXCEPTION_EXECUTE_HANDLER;
}

PTRSZVAL OpenV4W( POINTER p )
{
	lprintf( "opening v4w..." );

	hWndCap = capCreateCaptureWindow("Video Stream",WS_POPUP,0,0,1,1,0,0);
	//capDriverDisconnect( hWndCap );

   capSetUserData( hWndCap, 0 );

	capSetCallbackOnError( hWndCap, MycapErrorCallback );
	capSetCallbackOnStatus( hWndCap, MycapStatusCallback );
	capSetCallbackOnVideoStream( hWndCap, handle_frame );

	{
      int wIndex;
		char szDeviceName[80];
		char szDeviceVersion[80];
		LOGICAL valid = FALSE;
      int index;

		for (wIndex = 0; wIndex < 10; wIndex++)
		{
			if (capGetDriverDescription(
												 wIndex,
												 szDeviceName,
												 sizeof (szDeviceName),
												 szDeviceVersion,
												 sizeof (szDeviceVersion)
												))
			{
				// Append name to list of installed capture drivers
				// and then let the user select a driver to use.
				lprintf( "Capture driver found: %d [%s] (%s)", wIndex, szDeviceName, szDeviceVersion );
#if defined( __WATCOMC__ )
#ifndef __cplusplus
				_try
#endif
				{
#endif
					if( !capDriverConnect( hWndCap, 0 ) )
					{
						lprintf( "Failed to connect to WM_CAP device..." );

						//DebugBreak();
						break;
					}
				}
#if defined( __WATCOMC__ )
#ifndef __cplusplus
				_except( filter1() )
				{
					lprintf( "watcom - recovered exception 1" );
               break;
					//return 0;
				}
#endif
#endif
				if( !capDriverDisconnect( hWndCap ) )
					lprintf( "Disconnect failed" );
				valid = TRUE;
            index = wIndex;

			}
		}
		if( !valid )
		{
			DestroyWindow( hWndCap );
         return 0;
		}
   lprintf( "..." );
	if( !capDriverConnect( hWndCap, index ) )
	{
		lprintf( "Failed to connect to WM_CAP device..." );

		//DebugBreak();
		return 0;
	}
   SetWindowLong( hWndCap, GWL_USERDATA, (long)p );
#if 0
		{
         CAPDRIVERCAPS CapDrvCaps; 
			//SendMessage (hWndC, WM_CAP_DRIVER_GET_CAPS,
			//				 sizeof (CAPDRIVERCAPS), (LONG) (LPVOID) &CapDrvCaps);

			// Or, use the macro to retrieve the driver capabilities.
			capDriverGetCaps(hWndCap, &CapDrvCaps, sizeof (CAPDRIVERCAPS));

		}
#endif

		{
			CAPTUREPARMS CaptureParms;
			float FramesPerSec = 10.0;
   lprintf( "..." );
			capCaptureGetSetup(hWndCap
									, &CaptureParms, sizeof(CAPTUREPARMS));
			CaptureParms.fAbortLeftMouse = FALSE;
			CaptureParms.fAbortRightMouse = FALSE;
			CaptureParms.vKeyAbort = FALSE;
			CaptureParms.fCaptureAudio = FALSE;
			CaptureParms.fYield = TRUE; // one way to allow other processes to go during capture.
			CaptureParms.dwRequestMicroSecPerFrame = (DWORD) (33000);
			capCaptureSetSetup(hWndCap, &CaptureParms
									, sizeof (CAPTUREPARMS));
		}
		{
			int result;
			BITMAPINFO format;
			result = capGetVideoFormatSize( hWndCap ) ;
			format.bmiHeader.biSize = result;
			capGetVideoFormat( hWndCap, &format, result ) ;
			format.bmiHeader.biBitCount = 24;   // 24, 16, ...
			format.bmiHeader.biCompression = BI_RGB;
			format.bmiHeader.biSizeImage =0;
			result = capSetVideoFormat( hWndCap, &format, sizeof( format ) ) ;
			lprintf( "result = %d", result );
			capGetVideoFormat( hWndCap, &format, sizeof( format ) ) ;
			l.image = MakeImageFile( format.bmiHeader.biWidth, format.bmiHeader.biHeight );
		}
	}
   return 1;
	//   capSetCallbackOnFrame( hWndCap, handle_frame );
	//capSetCallbackOnYield( hWndCap, MycapYieldCallback );
	//capSetCallbackOnCapControl( hWndCap, MycapCapControlCallback );

}

void Start( void )
{
//	capPreviewRate( hWndCap, 100 );

//	capPreview( hWndCap, TRUE );
	capCaptureSequenceNoFile( hWndCap );
}

void Stop(void)
{
	//capCaptureStop(hWndCap);
	if( !capCaptureStop( hWndCap ) )
      lprintf( "stop failed." );
	if( !capCaptureAbort(hWndCap) )
      lprintf( "Abort failed." );
	//capSetCallbackOnFrame(hWndCap, NULL);
	capSetCallbackOnVideoStream(hWndCap, NULL);
	capSetCallbackOnError( hWndCap, NULL );
	capSetCallbackOnStatus( hWndCap, NULL );
	if( !capDriverDisconnect( hWndCap ) )
      lprintf( "Disconnect failed" );
   DestroyWindow( hWndCap );
   hWndCap = NULL;
}


ATEXIT( CloseVidCap )
{
	if( g.flags.bRunning )
	{
		g.flags.bExit = 1;
		// wake up the capture thread...
		PostThreadMessage( g.capture_thread, WM_USER, 0, 0 );
		while( g.flags.bRunning )
			Relinquish();
	}
}
