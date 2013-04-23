#ifndef BMPUTILS_H
#define BMPUTILS_H


#include <windows.h>

class CbmpUtils
{
	public:
		CbmpUtils();
		virtual ~CbmpUtils();
		int LoadBMP();						// Load bitmap
		int GDIPaint(HDC hdc,int x,int y);	// Display bitmap
		int SaveBMP(LPCTSTR fileName);		// Save bitmap
	private:
		int Width,Height;		// Dimensions
		char * bmpData;			// Bits of the Image.
		BITMAPINFO * pbmi;		// BITMAPINFO structure
		ULONG BitmapSize;
};

#endif