#include <windows.h>
#include <commctrl.H>
#include <math.h>  // exp()
#include "brainres.h"

#include "neuron.h"

#include <render.h>

//----------------------------------------------

BOOL CALLBACK NeuronPropProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   PNEURON pn;
   switch( uMsg )
   {
   case WM_INITDIALOG:
      // Set hWndLong with lParam for future 
      // operations on selected neuron ....
      pn = (PNEURON)lParam;
      {
         BYTE byValue[64];
         SetWindowLong( hWnd, GWL_USERDATA, (DWORD)pn );
         wsprintf( (char*)byValue, "Neuron - %s", pn->pName );
         SetWindowText( hWnd, (char*)byValue );
         SendDlgItemMessage( hWnd, SLD_THRESHOLD, TBM_SETRANGE, TRUE, MAKELONG( -MAX_THRESHOLD,
                                                                                 MAX_THRESHOLD) );
         SendDlgItemMessage( hWnd, SLD_THRESHOLD, TBM_SETPOS, TRUE, pn->nThreshold.n );
         wsprintf( (char*)byValue, "%d", pn->nThreshold );
         SetDlgItemText( hWnd, EDT_THRESHOLD, (char*)byValue );
      }
      CheckDlgButton( hWnd, RDO_ANALOG, pn->nType == NT_ANALOG?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, RDO_DIGITAL, pn->nType == NT_DIGITAL?BST_CHECKED:BST_UNCHECKED );
      CheckDlgButton( hWnd, RDO_SIGMOID, pn->nType == NT_SIGMOID?BST_CHECKED:BST_UNCHECKED );
      SetTimer( hWnd, 1005, 50, NULL ); // well - SetTimer();
   case WM_TIMER:
      {      
         BYTE byValue[64];

         // read interlock....
//         BOARD *pb;
//         pb = (BOARD *)GetWindowLong( GetParent( hWnd ), GWL_USERDATA );

//         pb->Brain->bLock = TRUE;
//         while( !pb->Brain->bLocked ) Sleep(0); // relinquish....
         
         // brain locked and 'stable' CAN pull values NOW.
         pn = (PNEURON)GetWindowLong( hWnd, GWL_USERDATA );
         wsprintf( (char*)byValue, "%d", pn->nInput );
         SetDlgItemText( hWnd, EDT_CURRENT, (char*)byValue );

//         pb->Brain->bLock = FALSE;
      }
      return FALSE;

   case WM_NOTIFY:
      {
         LPNMHDR ph;
         ph = (LPNMHDR)lParam;
         switch( wParam )
         {
         case SLD_THRESHOLD:
            {
               BYTE byValue[64];

               pn = (PNEURON)GetWindowLong( hWnd, GWL_USERDATA );
               pn->nThreshold.n = SendDlgItemMessage( hWnd, SLD_THRESHOLD, TBM_GETPOS, 0, 0  );

               wsprintf( (char*)byValue, "%d", pn->nThreshold );
               SetDlgItemText( hWnd, EDT_THRESHOLD, (char*)byValue );
            }
            break;
         case EDT_THRESHOLD:  // fetch value from here???
            break;
         }
      }    
      break;
   case WM_COMMAND:
      switch( LOWORD(wParam ) )
      {
      case RDO_DIGITAL:
         pn = (PNEURON)GetWindowLong( hWnd, GWL_USERDATA );
         pn->nType = NT_DIGITAL;
         break;
      case RDO_ANALOG:
         pn = (PNEURON)GetWindowLong( hWnd, GWL_USERDATA );
         pn->nType = NT_ANALOG;
         break;
      case RDO_SIGMOID:
         pn = (PNEURON)GetWindowLong( hWnd, GWL_USERDATA );
         pn->nType = NT_SIGMOID;
         break;
      case IDCANCEL:
      case BTN_DONE:
         if( !KillTimer( hWnd, 1005 ) )
            DebugBreak(); // couldn't kill the timer?

         EndDialog( hWnd, 0 );
         break;
      case BTN_DELETE:
         /*
         if( MessageBox( hWnd, "Are you sure?", "Confirm", MB_OKCANCEL ) == IDOK )
         {
            BOARD *pb;
            pb = (BOARD *)GetWindowLong( GetParent( hWnd ), GWL_USERDATA );
            pn = (PNEURON)GetWindowLong( hWnd, GWL_USERDATA );
            pb->DeleteNeuron( pn );
            EndDialog( hWnd, 0 );
         }
         */
         break;

      }
   }

   return FALSE;
}

//---------------------------------------------------

BOOL CALLBACK SynapsePropProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   PSYNAPSE ps;
   switch( uMsg )
   {
   case WM_INITDIALOG:
      // Set hWndLong with lParam for future 
      // operations on selected neuron ....
      ps = (PSYNAPSE)lParam;
      if( ps )
      {
         char pTemp[64];
         ps->bEditDecay = FALSE;
         CheckDlgButton( hWnd, RDO_GAIN, BST_CHECKED );
         CheckDlgButton( hWnd, RDO_DECAY, BST_UNCHECKED );
         SetWindowLong( hWnd, GWL_USERDATA, (DWORD)ps );
         wsprintf( (char*)pTemp, "Synapse - %s", ps->pName );
         SetWindowText( hWnd, (char*)pTemp );
Reshape:      
         if( ps->bEditDecay )
         {
            SendDlgItemMessage( hWnd, SLD_GAIN, TBM_SETRANGE, TRUE, MAKELONG(-MAX_DECAY,MAX_DECAY) );
            SendDlgItemMessage( hWnd, SLD_GAIN, TBM_SETPOS, TRUE, ps->nDecay.n );
            SetDlgItemText( hWnd, TXT_MODE, "Decay" );
            // setting range causes a notifcation
            {      
               BYTE byValue[64];
               wsprintf( (char*)byValue, "%d", ps->nDecay );
               SetDlgItemText( hWnd, EDT_GAIN, (char*)byValue );
            }
         }
         else
         {
            SendDlgItemMessage( hWnd, SLD_GAIN, TBM_SETRANGE, TRUE, MAKELONG(-MAX_GAIN,MAX_GAIN) );
            SendDlgItemMessage( hWnd, SLD_GAIN, TBM_SETPOS, TRUE, ps->nGain.n );
            SetDlgItemText( hWnd, TXT_MODE, "Gain" );
            {      
               BYTE byValue[64];
               wsprintf( (char*)byValue, "%d", ps->nGain );
               SetDlgItemText( hWnd, EDT_GAIN, (char*)byValue );
            }
         }
         
      }
      else
         EndDialog( hWnd, 0 );
      return FALSE;
   case WM_HSCROLL:
      switch( LOWORD( wParam ) )
      {
      case TB_ENDTRACK:
      case TB_THUMBTRACK:
         {
            BYTE byValue[64];
            ps = (PSYNAPSE)GetWindowLong( hWnd, GWL_USERDATA );

            if( ps->bEditDecay )
            {
               ps->nDecay.n = SendDlgItemMessage( hWnd, SLD_GAIN, TBM_GETPOS, 0, 0  );
               wsprintf( (char*)byValue, "%d", ps->nDecay );
               SetDlgItemText( hWnd, EDT_GAIN, (char*)byValue );
            }
            else
            {
               ps->nGain.n = SendDlgItemMessage( hWnd, SLD_GAIN, TBM_GETPOS, 0, 0  );
               wsprintf( (char*)byValue, "%d", ps->nGain );
               SetDlgItemText( hWnd, EDT_GAIN, (char*)byValue );
            }
         }
         break;
      }
      break;
   case WM_COMMAND:
      switch( LOWORD(wParam ) )
      {
      case RDO_DECAY:
         ps = (PSYNAPSE)GetWindowLong( hWnd, GWL_USERDATA );
         ps->bEditDecay = TRUE;
         goto Reshape;
         break;
      case RDO_GAIN:
         ps = (PSYNAPSE)GetWindowLong( hWnd, GWL_USERDATA );
         ps->bEditDecay = FALSE;
         goto Reshape;
         break;
      case IDCANCEL:
      case BTN_DONE:
         EndDialog( hWnd, 0 );
         break;

      }
      break;
   }

   return FALSE;
}

void DrawGraph( PRENDERER hVideo, float k )
{
   Image pif = GetDisplayImage( hVideo );
   int x, width, y;
   ClearImage( pif );
   width = pif->width;
   for( x = 0; x < width; x++ )
   {
      // top is top(?)
      y = (int)(pif->height - ( ((pif->height-2)/(1 + exp( -k * (x - width/2))))+1));
      plot( pif, x, y, Color( 255,255,255) );
   }
   UpdateDisplay( hVideo, 0, 0, 0, 0 );
}

#define WD_HVIDEO   0   // WindowData_HVIDEO
// in case "VideoOutputClass" was used as a control in a dialog...
#define GetVideoHandle( hWndDialog, nControl ) ((PRENDERER)(GetWindowLong( GetDlgItem( hWndDialog, nControl ), 0 )))


BOOL CALLBACK SigmoidProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   float *pk;
   switch( uMsg )
   {
   case WM_INITDIALOG:
      pk = (float*)lParam;
      SetWindowLong( hWnd, GWL_USERDATA, (DWORD)pk );
      SendDlgItemMessage( hWnd, SLD_K, TBM_SETRANGE, TRUE, MAKELONG(1 , 100) );
      SendDlgItemMessage( hWnd, SLD_K, TBM_SETPOS, TRUE, (long)(*pk * 100) );
      DrawGraph( GetVideoHandle( hWnd, VID_GRAPH ), *pk );
      break;
   case WM_HSCROLL:
      switch( LOWORD( wParam ) )
      {
      case TB_ENDTRACK:
      case TB_THUMBTRACK:
         {
            pk = (float *)GetWindowLong( hWnd, GWL_USERDATA );
            *pk = (float)SendDlgItemMessage( hWnd, SLD_K, TBM_GETPOS, 0, 0  ) / 100.0f;
            DrawGraph( GetVideoHandle( hWnd, VID_GRAPH ), *pk );
         }
         break;
      }
      break;
   case WM_COMMAND:
      switch( LOWORD(wParam ) )
      {
      case IDCANCEL:
      case BTN_DONE:
         EndDialog( hWnd, 0 );
         break;
      }
      break;
   }
   return FALSE;
}
