// VFWImageProcessor.h: interface for the CVFWCapture class.
//
//////////////////////////////////////////////////////////////////////

#if !defined VFWIMAGEPROCESSOR_H
#define VFWIMAGEPROCESSOR_H

#include <windows.h>
#include <vfw.h>
#include <string.h>

#define MAX_VFW_DEVICES		10
#define ERROR_SIZE			100
class CVFWCapture
{
public:
   CVFWCapture();
   virtual ~CVFWCapture();
   CVFWCapture(const CVFWCapture &CopyFrom);
   CVFWCapture &operator =(const CVFWCapture &CopyFrom);

   BOOL Initialize(SHORT DriverIndex = 0);
   VOID Destroy();

   BOOL SetDriver(SHORT DriverIndex);

   HWND GetCapWindow();

   BOOL CaptureDIB(PBITMAPINFO *Bitmap, ULONG BitmapLength, ULONG *RetBitmapLength);                            
   BOOL EnablePreviewVideo(HWND Parent, INT x, INT y, INT PreviewRate = 30);
   BOOL EnablePreviewVideo(HWND Parent, INT x, INT y, INT Width, INT Height, INT PreviewRate = 30);
   BOOL DisablePreviewVideo();
   BOOL DriverGetCaps(CAPDRIVERCAPS *Caps);
   VOID CancelCapture();
   BOOL AllocDIBImage(PBITMAPINFO *ppImageData, ULONG *AllocatedSize);

   BITMAPINFOHEADER GetBitmapInfoHeader();

   VOID GetPreviousError(INT *ErrorID, char *ErrorString, BOOL ResetError = FALSE);

   static ULONG CalcBitmapSize(const BITMAPINFOHEADER &bmiHeader);
   static ULONG CalcBitmapInfoSize(const BITMAPINFOHEADER &bmiHeader);

   friend LRESULT CALLBACK ErrorCallbackProc(HWND hWnd, int nErrID, LPSTR lpErrorText);
   friend LRESULT CALLBACK StatusCallbackProc(HWND hWnd, int nID, LPCSTR lpsz);
   friend LRESULT CALLBACK FrameCallbackProc(HWND hWnd, LPVIDEOHDR lpVHdr);

private:    // Data
   HWND m_hWndVideo;
   BITMAPINFOHEADER m_BitmapInfoHeader;      // Used to store image dimensions.
   PBITMAPINFO m_TransferBitmapInfo;
   ULONG m_TransferBitmapInfoSize;
   INT m_DriverIndex;
   INT m_ErrorID;
   char m_ErrorText[ERROR_SIZE];


   static UINT m_ValidDriverIndex[MAX_VFW_DEVICES];
   static USHORT m_TotalVideoDrivers;

private:    // Functions
   CVFWCapture &Copy(const CVFWCapture &CopyFrom);
};

#endif // !defined VFWIMAGEPROCESSOR_H