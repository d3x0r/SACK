#include <stdhdrs.h>
#include <winspool.h>
#include <image.h>
#define PRINTER_LIBRARY_SOURCE
#include <printer.h>
//#define USE_NEW_API
#ifdef _WIN32

static struct printer_local_tag
{
	PIMAGE_INTERFACE pii;
} printer_local;

struct printer_context
{
	struct printer_context_flags
	{
		BIT_FIELD in_page : 1; // is in a StartPage()
	} flags;
	DOCINFO di;
	HDC hdc;
	int cWidthPels, cHeightPels;
	HDC hdcMem;
	HBITMAP hBmp;
	HGDIOBJ hOldBmp;
	Image image;

	HANDLE hPrinter;
};

static void InitPrinter( struct printer_context *context )
{
	TEXTCHAR szPrinterName[255];
	unsigned long lPrinterNameLength = 255;

	if( !printer_local.pii )
	{
		printer_local.pii = GetImageInterface();
	}

	GetDefaultPrinter( szPrinterName, &lPrinterNameLength );
#ifdef USE_NEW_API
	{
		PRINTER_DEFAULTS pd;
		DEVMODE dm;
		DOC_INFO_1 docinfo;
		MemSet( &dm, 0, sizeof( DEVMODE ) );
		dm.dmSize = sizeof( DEVMODE );
		pd.DesiredAccess = PRINTER_ALL_ACCESS;
		pd.pDevMode = &dm;
		pd.pDatatype = NULL /*"RAW"*/;
		if( OpenPrinter( szPrinterName, &context->hPrinter, &pd ) )
		{
			docinfo.pDocName = "Print Job Name...";
			docinfo.pOutputFile = NULL;
			docinfo.pDatatype = NULL;
			StartDocPrinter( context->hPrinter, 1, (LPBYTE)&docinfo );
		}
		else
			lprintf( "Failed to open device: %d", GetLastError() );

	}
#else
	context->hdc = CreateDC( WIDE("WINSPOOL"), szPrinterName, NULL, NULL );
	if( context->hdc )
	{
		context->di.cbSize = sizeof(DOCINFO);
		context->di.lpszDocName = WIDE("PrintIt");
		context->di.lpszOutput = (LPTSTR) NULL;
		context->di.lpszDatatype = (LPTSTR) NULL;
		context->di.fwType = 0;
		{
			int nError = StartDoc(context->hdc, &context->di);
			if (nError == SP_ERROR)
			{
				lprintf( WIDE("Error - please check printer.") );
				// Handle the error intelligently
				DeleteDC( context->hdc );
				Release( context );
				return;
			}
			StartPage( context->hdc );
			context->flags.in_page = 1;
			{
				context->cWidthPels = GetDeviceCaps(context->hdc, HORZRES);
				context->cHeightPels = GetDeviceCaps(context->hdc, VERTRES);

				context->hdcMem = CreateCompatibleDC( context->hdc );
				context->hBmp	= CreateCompatibleBitmap( context->hdc, context->cWidthPels, context->cHeightPels );
				context->hOldBmp= SelectObject( context->hdcMem, context->hBmp);
			}
		}
	}
#endif
}

struct printer_context *Printer_Open( void )
{
	struct printer_context *context = New( struct printer_context );
	MemSet( context, 0, sizeof( struct printer_context ) );
	InitPrinter( context );
	return context;
}



void Printer_NewPage( struct printer_context *context )
{
	if( context->flags.in_page )
	{
#ifdef USE_NEW_API
		EndPagePrinter( context->hPrinter );
#else
		EndPage( context->hdc );
#endif
	}
#ifdef USE_NEW_API
	StartPagePrinter( context->hPrinter );
#else
	StartPage( context->hdc );
#endif
	context->flags.in_page = TRUE;
}

void Printer_Flush( struct printer_context *context )
{
	BitBlt( context->hdc, 0, 0, context->cWidthPels, context->cHeightPels,
			  context->hdcMem, 0, 0, SRCCOPY);
#ifdef USE_NEW_API
	EndPagePrinter( context->hPrinter );
	EndDocPrinter( context->hPrinter );
#else
	EndPage( context->hdc );
	EndDoc( context->hdc );
#endif
	SelectObject( context->hdcMem, context->hOldBmp );
	ReleaseDC( NULL, context->hdc );
	DeleteDC( context->hdcMem );
	context->flags.in_page = 0;

	InitPrinter( context );
}

void Printer_Close( struct printer_context *context )
{
	Printer_Flush( context );
	Release( context );
}

Image Printer_GetImage( struct printer_context *context )
{
	return context->image;
}

#endif

