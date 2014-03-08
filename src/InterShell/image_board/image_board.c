#define WINVER 0x500
#include <image.h>
#include <psi.h>

#include "intershell_registry.h"

typedef struct {
   PSI_CONTROL pc;
	struct {
		BIT_FIELD bWrite : 1;
	} flags;
	Image image;
   TEXTSTR name; // for saving/restoring images...
}MY_IMAGE, *PMY_IMAGE;

EasyRegisterControl( "Image Paste-board", sizeof( MY_IMAGE ) );

static int OnDrawCommon( "Image Paste-board" )( PSI_CONTROL pc )
{
	ValidatedControlData( PMY_IMAGE, MyControlID, image, pc );
	if( image )
	{
		if( image->image )
		{
         //DebugBreak();
			BlotScaledImage( GetControlSurface( pc ), image->image);
			//BlotScaledImageAlpha( GetControlSurface( pc ), image->image, 0 /*ALPHA_TRANSPARENT */);
		}
	}
   return 1;
}

#if 0
int OnMouseCommon( "Image Paste-board")( PSI_CONTROL pc,S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( PMY_IMAGE, MyControlID, image, pc );
	if( IsKeyDown( NULL, KEY_CONTROL ) && IsKeyDown( NULL, KEY_V ) )
	{
		KeystrokePaste( image );
      SmudgeCommon( pc );
	}
   return 1;
}
#endif

int KeystrokePaste( PMY_IMAGE image )
{
	int status = 0;
	if( OpenClipboard(NULL) )
	{
		int success = 0;
		_32 format;
		// successful open...
		format = EnumClipboardFormats( 0 );
		while( format )
		{
			//DECLTEXT( msg, "                                     " );
			//msg.data.size = sprintf( msg.data.data, "Format: %d", format );
			//EnqueLink( pdp->ps->Command->ppOutput, SegDuplicate( (PTEXT)&msg ) );
#ifndef CF_TEXT
#define CF_TEXT 1
#endif
			if( format == CF_TEXT )
			{
				HANDLE hData = GetClipboardData( CF_TEXT );
				LPVOID pData = GlobalLock( hData );
				PTEXT pStroke = SegCreateFromText( pData );
				int ofs, n;
				GlobalUnlock( hData );
				{
					_32 w, h;
					GetStringSize( (CTEXTSTR)pData, &w, &h );
					image->image = MakeImageFile( w + 15, h + 15 );
					status = TRUE;
					do_line( image->image, 0, 0, w, h, BASE_COLOR_WHITE );
					PutString( image->image, 7, 7, BASE_COLOR_WHITE, 0, pData );
				}
				LineRelease( pStroke );
				//break;
			}
			else if( !success && format == CF_BITMAP )
			{
				HANDLE hData = GetClipboardData( format );
				BITMAP *pbm = GlobalLock( hData );

				if( pbm )
				{
               lprintf( "Got a pointer back!" );
					if( pbm )
						image->image = RemakeImage( image->image, pbm->bmBits, pbm->bmWidth, pbm->bmHeight );
				}
				GlobalUnlock( hData );
			}
			else if( !success && ( ( format == CF_DIB ) || ( format == CF_DIBV5 ) ) )
			{
				HANDLE hData = GetClipboardData( format );
				BITMAPINFO *pData = GlobalLock( hData );
            void *bits = (void*)(pData + 1);
				if( pData)
				{
					image->image = ImageRawBMPFile( (P_8)pData, 0 );
#if 0
					if( pData->bmiHeader.biBitCount == 32 )
					{
                  Image tmp;
						tmp = BuildImageFile( bits, pData->bmiHeader.biWidth
												  , pData->bmiHeader.biHeight );
                  if( !image->image )
							image->image = MakeImageFile( pData->bmiHeader.biWidth
																 , pData->bmiHeader.biHeight );
						else
							ResizeImage( image->image,pData->bmiHeader.biWidth
										 , pData->bmiHeader.biHeight );
						BlotImage( image->image, tmp, 0, 0 );
                  UnmakeImageFile( tmp );
                  status = TRUE;
					}
					else
					{
                  //DebugBreak();
						lprintf( "Invalid input pixel format - must be 32... (or pass to decode image pointer - will load bitmap of multi formats from memory print" );
					}
					lprintf( "Got a pointer back!" );
#endif
				}
				GlobalUnlock( hData );
			}
			else if( !success && format == CF_ENHMETAFILE )
			{
            HANDLE hData = GetClipboardData( format );
				HANDLE hDataDIB = GetClipboardData( CF_DIB );
				BITMAPINFO *pDataDIB = GlobalLock( hDataDIB );
				POINTER bits = NULL;
				_32 size = 0;
				//DebugBreak();
				{
					{
						BITMAPINFO bmInfo;

						bmInfo.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
						bmInfo.bmiHeader.biWidth = pDataDIB->bmiHeader.biWidth; // size of window...
						bmInfo.bmiHeader.biHeight = pDataDIB->bmiHeader.biHeight;
						bmInfo.bmiHeader.biPlanes = 1;
						bmInfo.bmiHeader.biBitCount = 32;   // 24, 16, ...
						bmInfo.bmiHeader.biCompression = BI_RGB;
						bmInfo.bmiHeader.biSizeImage = 0;   // zero for BI_RGB
						bmInfo.bmiHeader.biXPelsPerMeter = 0;
						bmInfo.bmiHeader.biYPelsPerMeter = 0;
						bmInfo.bmiHeader.biClrUsed = 0;
						bmInfo.bmiHeader.biClrImportant = 0;
						{
							PCOLOR pBuffer;
							RECT r;
                     Image tmpimage;
							HBITMAP tmphBm = CreateDIBSection (NULL, &bmInfo, DIB_RGB_COLORS, (void **) &pBuffer, NULL,   // hVideo (hMemView)
																	  0); // offset DWORD multiple
							//lprintf( WIDE("New drawing surface, remaking the image, dispatch draw event...") );
							if( !image->image )
								image->image = MakeImageFile( bmInfo.bmiHeader.biWidth,
												 bmInfo.bmiHeader.biHeight );
							else
								ResizeImage( image->image,bmInfo.bmiHeader.biWidth,
												 bmInfo.bmiHeader.biHeight );
							tmpimage =
								BuildImageFile( pBuffer, bmInfo.bmiHeader.biWidth,
												 bmInfo.bmiHeader.biHeight);

							if (tmphBm)
							{
								{
									HDC hdc = CreateCompatibleDC( GetDC( NULL ) );
									HDC old = SelectObject (hdc,tmphBm);
									SetRect( &r, 0, 0, bmInfo.bmiHeader.biWidth,
											  bmInfo.bmiHeader.biHeight );
									ClearImageTo( tmpimage, BASE_COLOR_PURPLE );
                           SetBkColor( hdc, 0 );
                           SetBkMode (hdc, TRANSPARENT);
									if( !PlayEnhMetaFile( hdc, hData, &r ) )
										lprintf( "!!!!   Error playing meta file - %d", GetLastError() );
									BlotImage( image->image, tmpimage, 0, 0 );
									UnmakeImageFile( tmpimage );
									SelectObject( hdc, old );
                           DeleteObject( tmphBm );
									ReleaseDC( NULL, hdc );

								}
								//DWORD dwError = GetLastError();
								// this is normal if window minimizes...
							}
						}
					}
				}
#if 0
				size = GetMetaFileBitsEx(hData
												, size     // buffer size
												, bits);
				bits = Allocate( size );
				size = GetMetaFileBitsEx(hData
												, size     // buffer size
												, bits);

				BOOL PlayEnhMetaFile(
											HDC hdc,            // handle to DC
											HENHMETAFILE hemf,  // handle to an enhanced metafile
											CONST RECT *lpRect  // bounding rectangle
);
				image->image = RemakeImage( image->image, bits, pDataDIB->bmiHeader.biWidth, pDataDIB->bmiHeader.biHeight );
#endif
            status = TRUE;
			}
			else if( !success && format == CF_METAFILEPICT )
			{
				HANDLE hData = GetClipboardData( format );
				LPVOID pData = GlobalLock( hData );
				GlobalUnlock( hData );
			}
			else if( !success && format == CF_MAX )
			{
            lprintf( "No idea what this is, but it was a screen cpature in vista." );
			}
			else
			{
				//lprintf( "Format %d is on the clipboard...", format );
			}
			{
				char name[256];
            GetClipboardFormatName(  format,name, sizeof( name ) );
				lprintf( "Format %d(%s) is on the clipboard...", format, name );
			}
			format = EnumClipboardFormats( format );

		}
		CloseClipboard();
	}
	else
	{
#ifdef __DEKWARE__PLUGIN__
        DECLTEXT( msg, "Clipboard was not available" );
		  EnqueLink( &pdp->common.Owner->Command->Output, &msg );
#endif
    }
    return status;

}

int OnKeyCommon( "Image Paste-board")( PSI_CONTROL pc,_32 key )
{
	ValidatedControlData( PMY_IMAGE, MyControlID, image, pc );
	if( IsKeyDown( NULL, KEY_CONTROL ) && IsKeyDown( NULL, KEY_V ) )
	{
		KeystrokePaste( image );
      SmudgeCommon( pc );
	}
   return 0;
}

OnGetControl( "Image paste-board" )( PTRSZVAL psv )
{
	PMY_IMAGE image = (PMY_IMAGE)psv;
   if( image )
		return image->pc;
   return NULL;
}

OnShowControl( "Image paste-board" )( PTRSZVAL psv )
{
   PMY_IMAGE image = (PMY_IMAGE)psv;
}



// OnDestroyCommon( ... )
// { /* delete image in image if there is an image
//
// }

OnCreateControl( "Image Paste-board" )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc = MakeNamedControl( parent, "Image Paste-board", x, y, w, h, -1 );
	{
		ValidatedControlData( PMY_IMAGE, MyControlID, image, pc );
		{
         image->pc = pc;
			if( !KeystrokePaste( image ) )
			{
            DestroyFrame( &pc );
            return 0;
			}
		}


		return (PTRSZVAL)image;
	}
   return 0;
}




