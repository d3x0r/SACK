// well do stuff here....
#include <windows.h>
#include <stdio.h>
#include "rcomlib.h"
#include "plugin.h"
#include "vidlib.h"

// common DLL plugin interface.....

HINSTANCE hInstDLL;

int APIENTRY DllMain( HANDLE hDLL, DWORD dwReason, void *pReserved )
{
   hInstDLL = hDLL;
   return TRUE; // success whatever the reason...
}

//int myTypeID; // supplied for uhmm... grins...

//PDATAPATH Open( PSENTIENT ps, PTEXT parameters );

HWND hWndDialog;
DWORD dwID;
extern BOOL CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
PENTITY pTray;
PSENTIENT psTray;


DWORD WINAPI ThreadProc( LPVOID lpParameter )
{
   MSG msg;
   hWndDialog = CreateDialog( hInstDLL, "WeatherTray", NULL, DialogProc );

   while( GetMessage( &msg, NULL, 0, 0 ) )
   {
      if( !IsDialogMessage( hWndDialog, &msg ) )
         DispatchMessage( &msg );
   }
   return 0;
}

int DisplayImage( PSENTIENT ps, PTEXT parameters );


extern char Browser[MAX_PATH];
extern char BrowserOpt[256];

char *RegisterRoutines( PEXPORTTABLE pExportTable )
{
   DECLTEXT( msg, "Weather Applet" );

   pExportedFunctions = pExportTable;
   RegisterVideoOutput();

   {
      char byValue[256], byNext[256];
		char BrowserVal[512];

      BrowserVal[0] = 0;
      strcpy( byValue, ".htm" );
      while( !GetRegistryItem( HKEY_CLASSES_ROOT, byValue, "\\shell\\open\\command", NULL, REG_SZ, BrowserVal, sizeof( BrowserVal ) ) 
            && GetRegistryItem( HKEY_CLASSES_ROOT, byValue, NULL, NULL, REG_SZ, byNext, sizeof( byNext ) ) )
         strcpy( byValue, byNext );
		{
			if( BrowserVal[0] == '\"' )
			{
				char *pEnd;
				pEnd = strchr( BrowserVal+1, '\"' );
				pEnd[0] = 0;
				strcpy( Browser, BrowserVal+1 );
				strcpy( BrowserOpt, pEnd+1 );
			}
			else
			{
				strcpy( Browser, BrowserVal );
				BrowserOpt[0] = 0;
			}
		}

   }

   CreateThread( NULL, 0, ThreadProc, 0, 0, &dwID );
   pTray = CreateEntityIn( NULL, SegDuplicate( (PTEXT)&msg ) );
   psTray = CreateAwareness( pTray );
	//psTray->Command->ppOutput = &PLAYER->Command->Output;
//   myTypeID = RegisterDevice( "console", "Windows based console....", Open );
   RegisterRoutine( "DisplayImage", "Weather tray applet banner display 1.", DisplayImage );
	return DekVersion;
}

void UnloadPlugin( void ) // this routine is called when /unload is invoked
{
   UnregisterRoutine( "DisplayImage" );
   // also destroy object....
}

// $Log: ntlink.c,v $
// Revision 1.3  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
