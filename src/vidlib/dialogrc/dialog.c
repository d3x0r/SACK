#include <windows.h>

#include "vidlib.h"

// this returns immediate after creating the dialog... non-modal...
int VidCreateDialogParam( HINSTANCE hInst, LPSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogProc, DWORD dwParam )
{
   HRSRC hrsrc;
   HGLOBAL hglob;
   void *Dialog;
   hrsrc = FindResource( hInst, lpTemplate, RT_DIALOG );
   if( hrsrc )
   {
      hglob = LoadResource( hInst, hrsrc );
      if( hglob )
      {
         Dialog = LockResource( hglob );
         if( Dialog )
         {
            DLGTEMPLATE *dtemp;
            dtemp = (DLGTEMPLATE*)Dialog;
            if( ( *(unsigned short*)dtemp ) == 0xFFFF )
            {
               // this function requires ATLWIN to be compiled....
               // since we HATE microsoft ATL and or CLASSLIB
               //DLGTEMPLATEEX *dtempex;
               //dtempex = (DLGTEMPLATEEX*)dtemp;
            }
            else
            {
            }
         }
      }
   }
   return 0;

}

int VidCreateDialog( HINSTANCE hInst, LPSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogProc )
{
   return VidCreateDialogParam( hInst, lpTemplate, hWndParent, lpDialogProc, 0 );
}

// $Log: dialog.c,v $
// Revision 1.2  2003/03/25 08:45:58  panther
// Added CVS logging tag
//
