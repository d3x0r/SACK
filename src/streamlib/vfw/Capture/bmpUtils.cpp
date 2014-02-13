// ************************************************************************
//	class CbmpUtils
//	Can capture bitmap from camera, display and save it.
//	Camera functionality provided by separate CVFWCapture class.
//  Author:	Audrey J. W. Mbogho
//	Email:	walegwa AT yahoo DOT com
//	Last Modified: January 2005
//	Previously, I tried to give credit here to all the
//	other people from whom I "borrowed" code. But that list
//	has grown to be too long for that. Apologies to all!
//	Environment: Visual C++, Windows 98, Logitech Webcam 4000 Pro.
// ************************************************************************

#include <fstream.h>
#include "bmpUtils.h"
#include "VFWCapture.h"

// Constructor
// Initializes object data
CbmpUtils::CbmpUtils()
{
	Width = 0;
	Height = 0;		
	bmpData = '\0';		
	pbmi = NULL;	
	BitmapSize = 0;
}

CbmpUtils::~CbmpUtils()
{
}

// LoadBMP();
// Loads a DIB captured from video camera into a CbmpUtils object
int CbmpUtils::LoadBMP()
{
	BITMAPINFOHEADER bmih;	// Contained in BITMAPINFO structure

	CVFWCapture cap;
	cap.Initialize();	// Initialize first found VFW device

	pbmi = NULL;	// CaptureDIB will automatically allocate this if set to NULL

	// Capture an image from the capture device.
	if (cap.CaptureDIB(&pbmi, 0, &BitmapSize))
	{
		// Obtain args for SetDIBitsToDevice
		bmih = pbmi->bmiHeader;
		Width = bmih.biWidth;
		Height = bmih.biHeight;
		Height = (bmih.biHeight>0) ? bmih.biHeight : -bmih.biHeight; // absolute value
		bmpData = (char *)pbmi;

		bmpData += cap.CalcBitmapInfoSize(bmih);
	}
	cap.Destroy();	// Done using VFW object
	return 0;
}


// CbmpUtils::GDIPaint (hdc,x,y);
// Paints bitmap to a Windows DC
int CbmpUtils::GDIPaint (HDC hdc,int x=0,int y=0)
{
	int ret = 0;
	// Paint the image to the device.
	ret = SetDIBitsToDevice (hdc,x,y,Width,Height,0,0,
		0,Height,(LPVOID)bmpData,pbmi,0);
	return ret;
}


// SaveBMP()
// Saves current bitmap to file
int CbmpUtils::SaveBMP(LPCSTR fileName)
{
	BITMAPFILEHEADER bfh;

	if (!pbmi)
		return 1;			// bitmap is empty

	ofstream bmpFile(fileName, ios::out | ios::binary);
	if (bmpFile.is_open())
	{
		bfh.bfType = 0x4d42;    // 0x42 = "B" 0x4d = "M"
		bfh.bfSize = (DWORD) BitmapSize + sizeof(BITMAPFILEHEADER);
		bfh.bfOffBits = (DWORD)   sizeof(BITMAPFILEHEADER) +
                                   sizeof(BITMAPINFOHEADER) +
                                    pbmi->bmiHeader.biClrUsed * 
		sizeof (RGBQUAD);
		bfh.bfReserved1 = 0;
		bfh.bfReserved2 = 0;

		bmpFile.write(reinterpret_cast<const char *>(&bfh),sizeof(bfh));
		bmpFile.write(reinterpret_cast<const char *>(pbmi),BitmapSize);
		bmpFile.close();
	}
	return 0;
}
