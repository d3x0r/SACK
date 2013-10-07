
#include "stdhdrs.h"

#include "rcomlib.h"

#include "resource.h"

static HINSTANCE myInst;

void CenterDialog( HWND hWnd )
{
   RECT r, rd;
   GetWindowRect( hWnd, &r );
   r.right -= r.left;
   r.bottom -= r.top;
   r.right /= 2;
   r.bottom /= 2;
   GetWindowRect( GetDesktopWindow(), &rd );
   rd.right -= rd.left;
   rd.bottom -= rd.top;
   rd.right /= 2;
   rd.bottom /= 2;
   SetWindowPos( hWnd, NULL, rd.right - r.right, rd.bottom - r.bottom, 0, 0, SWP_NOSIZE|SWP_NOOWNERZORDER );
}

BOOL CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   static char *pResult;
   static int nResult;
   switch( uMsg )
   {
   case WM_INITDIALOG:
      {
         char **pValues;
         pValues = (char**)lParam;
         SetDlgItemText( hWnd, TXT_QUERY, pValues[0] );
         pResult = pValues[1];
         nResult = (int)(pValues[2]);
      }
      CenterDialog( hWnd );
      break;
   case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case IDOK:
         GetDlgItemText( hWnd, EDT_RESPONSE, pResult, nResult );
         EndDialog( hWnd, TRUE );
         break;
      case IDCANCEL:
         EndDialog( hWnd, FALSE );
         break;
      }
   }
   return FALSE;
}

BOOL SimpleQuery( char *pText, char *pResult, int nResultLen )
{
   return DialogBoxParam( myInst, WIDE("SimpleQuery"), NULL, DialogProc, (long)&pText );
}


BOOL WINAPI DLLEntry( HINSTANCE hinstDLL,  // handle to DLL module
                    DWORD fdwReason,     // reason for calling function
                    LPVOID lpvReserved   // reserved
                   )
 
{
   if( fdwReason == DLL_PROCESS_ATTACH )
      myInst = hinstDLL;
   return TRUE;
}
// $Log: $
