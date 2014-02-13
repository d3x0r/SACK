/*******************************************************************************
    Title............... Video For Windows Class Interface
    Programmer.......... Ken Varn
    Date Created........ 9/20/2000
    Operating System.... Windows NT 4
    Compiler............ Microsoft Visual C++ 6
    File Type........... C++ Source

    Description:
       Class interface to Video For Windows.

       Before using any functions in this class, the Initialize() member
       function must be called on the instantiated object.

       When finished using this class, the Destroy() member function should
       be used.

    Revision History:
       Revision Date.... 8/12/2003
       Programmer....... Audrey Mbogho
       Comments......... No MFC, no Capture to file, no threads, no dialogs.
*******************************************************************************/


#include "VFWCapture.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

#pragma comment(lib,"vfw32")

UINT CVFWCapture::m_ValidDriverIndex[MAX_VFW_DEVICES];
USHORT CVFWCapture::m_TotalVideoDrivers = 0;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CVFWCapture::CVFWCapture()

{
   m_TransferBitmapInfo = NULL;
   m_TransferBitmapInfoSize = 0;
   m_hWndVideo = NULL;

   m_DriverIndex = -1;

   memset(&m_BitmapInfoHeader,0,sizeof(m_BitmapInfoHeader));
}

CVFWCapture::~CVFWCapture()
{
   Destroy();
}

/*******************************************************************************
   Function   : Initialize
   Arguments  : DriverIndex (input) - Index of VFW driver.
   Return     : TRUE Success, FALSE Failure
   Description: Inititlizes the object for using VFW interface to capture
                device.
*******************************************************************************/
BOOL CVFWCapture::Initialize(SHORT DriverIndex)
{
   BOOL Ret = FALSE;
   BOOL driverDesc = FALSE;
   BOOL driverConn = FALSE;
   SHORT Index;

   Destroy();

   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   m_hWndVideo = capCreateCaptureWindow(NULL,WS_POPUP,0,0,1,1,0,0);
   

   if (m_hWndVideo)
   {
      capSetUserData(m_hWndVideo,this);
      capSetCallbackOnError(m_hWndVideo,ErrorCallbackProc);
      capSetCallbackOnStatus(m_hWndVideo,StatusCallbackProc);
      capSetCallbackOnFrame(m_hWndVideo, FrameCallbackProc);

      // Construct list of valid video drivers.
      // This creates a contiguous virtual driver table.

      if (!m_TotalVideoDrivers)
      {
         char szDeviceName[80];
         char szDeviceVersion[80];

         for (Index = 0; Index < MAX_VFW_DEVICES; Index++)
         {
            driverDesc = capGetDriverDescription(Index,
                                        szDeviceName,
                                        sizeof(szDeviceName),
                                        szDeviceVersion,
                                        sizeof(szDeviceVersion));
    	    if (!driverDesc) break;
            driverConn = capDriverConnect(m_hWndVideo, Index);
			if (!driverConn) break;
            m_ValidDriverIndex[m_TotalVideoDrivers] = Index;
            m_TotalVideoDrivers++;
            capDriverDisconnect(m_hWndVideo);
         }
      }

      // Reset any error conditions.
      GetPreviousError(NULL,NULL,TRUE);

      Ret = SetDriver(DriverIndex);
   }

   if (!Ret)
   {
      if (m_ErrorID == 0)
         m_ErrorID = DV_ERR_NONSPECIFIC;

      Destroy();
   }

   return Ret;
}


/*******************************************************************************
   Function   : GetCapWindow
   Arguments  : none
   Return     : HWND of VFW window.
   Description: Used to retrieve the handle used for VFW image processing.
*******************************************************************************/
HWND CVFWCapture::GetCapWindow()
{
   return m_hWndVideo;
}

//
// Copy Constructor
//
CVFWCapture::CVFWCapture(const CVFWCapture &CopyFrom)
{
   m_TransferBitmapInfo = NULL;
   m_TransferBitmapInfoSize = 0;

   m_hWndVideo = NULL;
   m_DriverIndex = -1;
   memset(&m_BitmapInfoHeader,0,sizeof(m_BitmapInfoHeader));

   Copy(CopyFrom);
}


//
// Copy class using operator=
//
CVFWCapture &CVFWCapture::operator =(const CVFWCapture &CopyFrom)
{
   return Copy(CopyFrom);
}


/*******************************************************************************
   Function   : Destroy
   Arguments  : none
   Return     : none
   Description: Closes up the interface for VFW of capture device.
*******************************************************************************/
VOID CVFWCapture::Destroy()
{
   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   if (m_hWndVideo)
   {
      DisablePreviewVideo();

      capCaptureAbort(m_hWndVideo);
      capSetCallbackOnError(m_hWndVideo,NULL);
      capSetCallbackOnStatus(m_hWndVideo,NULL);
      capSetCallbackOnFrame(m_hWndVideo,NULL);

      capSetUserData(m_hWndVideo,NULL);
      capDriverDisconnect(m_hWndVideo);
      m_hWndVideo = NULL;
   }

   m_TransferBitmapInfo = NULL;
   m_TransferBitmapInfoSize = 0;
   m_DriverIndex = -1;

   memset(&m_BitmapInfoHeader,0,sizeof(m_BitmapInfoHeader));
}



/*******************************************************************************
   Function   : CaptureDIB
   Arguments  : Bitmap (output) - Pointer to bitmap to receive image.
                                  If *Bitmap = NULL, then allocation will
                                  be performed automatically.
                BitmapLength (input) - Size of Bitmap if *Bitmap is not NULL.
                RetBitmapLength (output) - Actual size of image.
   Return     : TRUE Success, FALSE Failed.
   Description: Captures a DIB image from video capture device.
*******************************************************************************/
BOOL CVFWCapture::CaptureDIB(PBITMAPINFO *Bitmap,
                                    ULONG BitmapLength,
                                    ULONG *RetBitmapLength)
{
   BOOL Ret = FALSE;

   DWORD Size = 0;

   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   if (*Bitmap == NULL)
   {
      AllocDIBImage(Bitmap,&Size);
      BitmapLength = Size;
   }
   else
   {
      AllocDIBImage(NULL,&Size);
   }

   if (*Bitmap && Size > 0)
   {
      if (RetBitmapLength)
      {
         *RetBitmapLength = Size;
      }

      // Must assign pointer to class member variable so that the
      // callback function can get to it.
      m_TransferBitmapInfo = *Bitmap;
      m_TransferBitmapInfoSize = BitmapLength;

      // Start capturing now.  Callback function will capture and signal us when done.
      Ret = capGrabFrame(m_hWndVideo);

      m_TransferBitmapInfo = NULL;
      m_TransferBitmapInfoSize = 0;

      if (!Ret)
      {
         if (RetBitmapLength)
         {
            *RetBitmapLength = (ULONG) 0;
         }
      }
   }

   if (!Ret && m_ErrorID == 0)
   {
      m_ErrorID = DV_ERR_NONSPECIFIC;
   }

   return Ret;
}


//
// Private function used to copy objects.
//
CVFWCapture &CVFWCapture::Copy(const CVFWCapture &CopyFrom)
{
   INT DeviceIdx;

   if (&CopyFrom != this)
   {
      Destroy();
	  strncpy(m_ErrorText, CopyFrom.m_ErrorText, ERROR_SIZE);

      if (CopyFrom.m_hWndVideo)
      {
         CAPDRIVERCAPS DriverCaps;

         capDriverGetCaps(CopyFrom.m_hWndVideo,&DriverCaps,sizeof(DriverCaps));

         // Find the device id in the virtual device list.
         for (DeviceIdx=0;DeviceIdx<MAX_VFW_DEVICES;++DeviceIdx)
         {
            if (m_ValidDriverIndex[DeviceIdx] == DriverCaps.wDeviceIndex)
            {
               Initialize(DeviceIdx);
               break;
            }
         }
      }
   }

   return *this;
}


/*******************************************************************************
   Function   : SetDriver
   Arguments  : DriverIndex (input) - Driver to set
   Return     : TRUE Success, FALSE Failed.
   Description: Sets curretn capture driver.
*******************************************************************************/
BOOL CVFWCapture::SetDriver(SHORT DriverIndex)
{
   BOOL Ret = TRUE;
   CAPTUREPARMS CapParms = {0};

   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   if (DriverIndex >= m_TotalVideoDrivers)
   {
      Ret = FALSE;
      m_ErrorID = DV_ERR_BADDEVICEID;
   }

   if (m_hWndVideo && m_DriverIndex != DriverIndex && Ret)
   {
      if (GetParent(m_hWndVideo) != NULL)
         capPreview(m_hWndVideo,FALSE);

      DisablePreviewVideo();
      capCaptureAbort(m_hWndVideo);

      Ret = capDriverConnect(m_hWndVideo, m_ValidDriverIndex[DriverIndex]);

      if (Ret)
      {
         capGetVideoFormat(m_hWndVideo,(PBITMAPINFO) &m_BitmapInfoHeader,sizeof(m_BitmapInfoHeader));
         capCaptureGetSetup(m_hWndVideo,&CapParms,sizeof(CapParms));
         CapParms.fAbortLeftMouse = FALSE;
         CapParms.fAbortRightMouse = FALSE;
         CapParms.fYield = TRUE;
         CapParms.fCaptureAudio = FALSE;
         CapParms.wPercentDropForError = 100;
         capCaptureSetSetup(m_hWndVideo,&CapParms,sizeof(CapParms));
         m_DriverIndex = DriverIndex;

         if (GetParent(m_hWndVideo) != NULL)
            capPreview(m_hWndVideo,TRUE);
      }
   }

   if (!Ret && m_ErrorID == 0)
   {
      m_ErrorID = DV_ERR_NONSPECIFIC;
   }

   return Ret;
}


/*******************************************************************************
   Function   : GetPreviousError
   Arguments  : ErrorID (output) - ID of Error
                ErrorString (output) - Description of error.
                ResetError (input) - TRUE Reset error condition.
   Return     : none
   Description: Gets the last Error ID and Error Description.
*******************************************************************************/
VOID CVFWCapture::GetPreviousError(INT *ErrorID, char ErrorString[], BOOL ResetError)
{
   if (ErrorID)
      *ErrorID = m_ErrorID;

   if (ErrorString)
      strncpy(ErrorString, m_ErrorText, ERROR_SIZE);

   if (ResetError)
   {
      m_ErrorID = 0;
      m_ErrorText[0] = '\0';
   }
}


/*******************************************************************************
   Function   : EnablePreviewVideo
   Arguments  : Parent (input) - Parent window that will display video.
                x (input) - X Location in parent where video will be shown.
                y (input) - Y location in parent where video will be shown.
                PreviewRate (input) - Rate of preview in FPS.
   Return     : TRUE Success, FALSE Failed.
   Description: Enables preview video mode.
*******************************************************************************/
BOOL CVFWCapture::EnablePreviewVideo(HWND Parent, INT x, INT y, INT PreviewRate)
{
   // Reset any error conditions.
   return EnablePreviewVideo(Parent,
                             x,y,
                             m_BitmapInfoHeader.biWidth,
                             m_BitmapInfoHeader.biHeight,
                             PreviewRate);
}


/*******************************************************************************
   Function   : EnablePreviewVideo
   Arguments  : Parent (input) - Parent window that will display video.
                x (input) - X Location in parent where video will be shown.
                y (input) - Y location in parent where video will be shown.
                Width (input) - Width of preview window.
                Height (input) - Height of preview window.
                PreviewRate (input) - Rate of preview in FPS.
   Return     : TRUE Success, FALSE Failed.
   Description: Enables preview video mode.
*******************************************************************************/
BOOL CVFWCapture::EnablePreviewVideo(HWND Parent, INT x, INT y, INT Width, INT Height, INT PreviewRate)
{
   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   SetParent(m_hWndVideo,Parent);
   SetWindowLong(m_hWndVideo,GWL_STYLE,WS_CHILD);

   SetWindowPos(m_hWndVideo,NULL,x,y,
                Width,
                Height,
                SWP_NOZORDER);
   ShowWindow(m_hWndVideo,SW_SHOW);
   capPreviewRate(m_hWndVideo, PreviewRate);

   return capPreview(m_hWndVideo,TRUE);
}




/*******************************************************************************
   Function   : DisablePreviewVideo
   Arguments  : none
   Return     : TRUE Success, FALSE Failed.
   Description: Disables preview video.
*******************************************************************************/
BOOL CVFWCapture::DisablePreviewVideo()
{
   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   BOOL Ret = capPreview(m_hWndVideo,FALSE);

   SetWindowPos(m_hWndVideo,NULL,0,0,0,0,SWP_NOZORDER);
   SetParent(m_hWndVideo,NULL);
   SetWindowLong(m_hWndVideo,GWL_STYLE,WS_POPUP);

   return Ret;
}


/*******************************************************************************
   Function   : DriverGetCaps
   Arguments  : Caps (output)
   Return     : See capDriverGetCaps()
   Description: Wrapper function for capDriverGetCaps().
*******************************************************************************/
BOOL CVFWCapture::DriverGetCaps(CAPDRIVERCAPS *Caps)
{
   // Reset any error conditions.
  GetPreviousError(NULL,NULL,TRUE);

  return capDriverGetCaps(m_hWndVideo,Caps,sizeof(*Caps));
}



/*******************************************************************************
   Function   : CancelCapture
   Arguments  : none
   Return     : none
   Description: Cancels current AVI capture.
*******************************************************************************/
VOID CVFWCapture::CancelCapture()
{
   capCaptureAbort(m_hWndVideo);
}


/*******************************************************************************
   Function   : AllocDIBImage
   Arguments  : ppImageData (output)     - Return pointer to allocated
                                           memory.  If passed as NULL,
                                           not used.
                AllocatedSize (output)   - Size of allocated block.
                                           If passed as NULL, not used.
   Return     : none
   Description: Allocates image buffer for DIB capture.
*******************************************************************************/
BOOL CVFWCapture::AllocDIBImage(PBITMAPINFO *ppImageData,
                                       ULONG *AllocatedSize)
{
   BOOL Ret = TRUE;
   DWORD Size = 0;

   // Reset any error conditions.
   GetPreviousError(NULL,NULL,TRUE);

   Size = CalcBitmapInfoSize(m_BitmapInfoHeader) + CalcBitmapSize(m_BitmapInfoHeader);

   if (Size > 0)
   {
      if (ppImageData)
      {
         *ppImageData = (BITMAPINFO *) new BYTE[Size];
         (**ppImageData).bmiHeader = m_BitmapInfoHeader;
      }
   }
   else
   {
      Ret = FALSE;
   }

   if (AllocatedSize)
   {
      *AllocatedSize = Size;
   }

   return Ret;
}



/*******************************************************************************
   Function   : GetBitmapInfoHeader()
   Arguments  : none
   Return     : BitmapInfo of capture device.
   Description: See return.
*******************************************************************************/
BITMAPINFOHEADER CVFWCapture::GetBitmapInfoHeader()
{
   return m_BitmapInfoHeader;
}



/*******************************************************************************
   Function   : CalcBitmapSize()
   Arguments  : bmiHeader (input) - BITMAPINFOHEADER from which to calculate
                                    bitmap size.
   Return     : Size of Bitmap.
   Description: Calculates the size of a bitmap based upon the contents of
                the BITMAPINFOHEADER passed in.
*******************************************************************************/
ULONG CVFWCapture::CalcBitmapSize(const BITMAPINFOHEADER &bmiHeader)

{
   ULONG Size = 0;

   if (bmiHeader.biSizeImage == 0)
   {
      Size = bmiHeader.biWidth *
             bmiHeader.biHeight *
             bmiHeader.biBitCount / 8;
   }
   else
   {
      Size = bmiHeader.biSizeImage;
   }

   return Size;
}


/*******************************************************************************
   Function   : CalcBitmapInfoSize()
   Arguments  : bmiHeader (input) - BITMAPINFOHEADER from which to calculate
                                    bitmap size.
   Return     : Size of Bitmap Info Header.
   Description: Calculates the size of a bitmap info header based upon the
                contents of the BITMAPINFOHEADER passed in.  This function
                can be used to determine the offset from the BITMAPINFOHEADER
                to the actual bitmap data.
*******************************************************************************/
ULONG CVFWCapture::CalcBitmapInfoSize(const BITMAPINFOHEADER &bmiHeader)

{
   UINT bmiSize = (bmiHeader.biSize != 0) ? bmiHeader.biSize : sizeof(BITMAPINFOHEADER);
   return bmiSize + bmiHeader.biClrUsed * sizeof (RGBQUAD);
}


//
// Internal callback functions.
//
static LRESULT CALLBACK ErrorCallbackProc(HWND hWnd, int nErrID, LPSTR lpErrorText)
{
   CVFWCapture *VFWObj = (CVFWCapture *) capGetUserData(hWnd);

   if (VFWObj)
   {
      VFWObj->m_ErrorID = nErrID;
      strncpy(VFWObj->m_ErrorText, lpErrorText, ERROR_SIZE);
   }

   return (LRESULT) TRUE;
}


static LRESULT CALLBACK StatusCallbackProc(HWND hWnd, int nID, LPCSTR lpsz)

{
   CVFWCapture *VFWObj = (CVFWCapture *) capGetUserData(hWnd);

   switch(nID)
   {
      case IDS_CAP_BEGIN:
         break;

      case IDS_CAP_END:
         break;
   }

   return (LRESULT) TRUE;
}


static LRESULT CALLBACK FrameCallbackProc(HWND hWnd, LPVIDEOHDR lpVHdr)

{
   CVFWCapture *VFWObj = (CVFWCapture *) capGetUserData(hWnd);
   LRESULT Ret = TRUE;

   if (VFWObj)
   {
      if (!VFWObj->m_hWndVideo)
      {
         Ret = FALSE;
      }
      else
      {
         if (VFWObj->m_TransferBitmapInfo)
         {
            ULONG Size;

            VFWObj->m_TransferBitmapInfo->bmiHeader = VFWObj->m_BitmapInfoHeader;

            Size =  min(VFWObj->m_TransferBitmapInfoSize - VFWObj->CalcBitmapInfoSize(VFWObj->m_TransferBitmapInfo->bmiHeader),
                        lpVHdr->dwBytesUsed);

            memcpy(((CHAR *) VFWObj->m_TransferBitmapInfo) + VFWObj->CalcBitmapInfoSize(VFWObj->m_TransferBitmapInfo->bmiHeader),
                   lpVHdr->lpData,
                   Size);
         }
      }
   }
   else
   {
      Ret = FALSE;
   }

   return Ret;
}
