#define _INCLUDE_CLIPBOARD
#include <stdhdrs.h>
#include <stdio.h> // sprintf ?
#include <string.h> // strchr
#include <logging.h>

#include "resource.h"

#include "consolestruc.h"
#include "interface.h"
#include "WinLogic.h"
#include "regaccess.h"
extern HINSTANCE hInstMe;

//----------------------------------------------------------------------------
// only one master copy of this is really needed...
static COLORREF crColorTable[16] = { RGB( 0,0,0 ), RGB( 0, 0, 128 ), RGB( 0, 128, 0 )
                            , RGB( 0, 128, 128 ), RGB( 192, 32, 32 ), RGB( 140, 0, 140 )
                            , RGB( 160, 160, 0 ), RGB( 192, 192, 192 )
                            , RGB( 128, 128, 128 ), RGB( 0, 0, 255 ), RGB( 0, 255, 0 )
                            , RGB( 0, 255, 255 ), RGB( 255, 0, 0 ), RGB( 255, 0, 255 )
                            , RGB( 255, 255, 0 ), RGB( 255, 255, 255 ) };


extern int myTypeID;

static HBRUSH hbrColorTable[16];
static ATOM aClassFrame, aClassChild;
static HWND hWndFrame, hWndMDI;
static HPEN hPenNormal, hPenHighlight, hPenShadow, hPenDkShadow;
HPEN hPenCursor;
static HMENU hChildMenu;
static HMENU hHistoryMenu;
static LOGFONT lfDefaultFont;

static CHOOSEFONT cfDefaultFont;
static HFONT        hFontDefault; // fixed...

//static HFONT hFontStatBar; // constant - cannot change?!
static HHOOK hKeyHook;

#define WM_CREATECHILD ( WM_USER + 100 )

#define WD_CONSOLE_INFO  0 // define for WINDOW DATA

//----------------------------------------------------------------------------

static void CPROC RenderCursor( PCONSOLE_INFO pmdp, RECT *r, int column )
{
   // render the cursor....
	// render cursor ... \_/ or /\ type shape....
   //lprintf( WIDE("Rendering the cursor... at ") );
   {
      extern HPEN hPenCursor;
		if( pmdp->
			common.
			CommandInfo->CollectionInsert )
      {
         int y, x;
         SelectObject( pmdp->wincon.hDC, hPenCursor );
         x = ( column ) * pmdp->nFontWidth + pmdp->nXPad;
         if( pmdp->flags.bDirect )
            y = r->top;
         else
            y = r->top;
         MoveToEx( pmdp->wincon.hDC, x - 3, y, NULL );
         LineTo( pmdp->wincon.hDC, x, y + 2 );
         LineTo( pmdp->wincon.hDC, x + 4, y );

         if( pmdp->flags.bDirect )
            y = r->bottom - 2;
         else
            y = r->bottom - pmdp->nYPad;
         MoveToEx( pmdp->wincon.hDC, x - 3, y, NULL );
         LineTo( pmdp->wincon.hDC, x, y - 2 );
         LineTo( pmdp->wincon.hDC, x + 4, y );
      }
      else
      {
         int y, x;
         SelectObject( pmdp->wincon.hDC, hPenCursor );
         x = ( column ) * pmdp->nFontWidth + pmdp->nXPad;
         if( pmdp->flags.bDirect )
            y = r->top;
         else
            y = r->top + 4;
         MoveToEx( pmdp->wincon.hDC, x, y+2, NULL );
         LineTo( pmdp->wincon.hDC, x, y );
         LineTo( pmdp->wincon.hDC, x + pmdp->nFontWidth, y );
         LineTo( pmdp->wincon.hDC, x + pmdp->nFontWidth, y+3 );

         if( pmdp->flags.bDirect )
            y = r->bottom - 2;
         else
            y = r->bottom - 5;
         MoveToEx( pmdp->wincon.hDC, x, y-2, NULL );
         LineTo( pmdp->wincon.hDC, x, y );
         LineTo( pmdp->wincon.hDC, x + pmdp->nFontWidth, y );
         LineTo( pmdp->wincon.hDC, x + pmdp->nFontWidth, y - 3 );
      }
	}
}


//----------------------------------------------------------------------------

static void CPROC RenderSeparator( PCONSOLE_INFO pdp, int nStart )
{
   //lprintf( WIDE("Rendering separator at %d"), nStart );
   // Render Command Line Separator
   SelectObject( pdp->wincon.hDC, hPenHighlight );
   MoveToEx( pdp->wincon.hDC, 0, nStart, NULL );
   LineTo( pdp->wincon.hDC, pdp->nWidth, nStart );

   SelectObject( pdp->wincon.hDC, hPenNormal );
   MoveToEx( pdp->wincon.hDC, 0, nStart+1, NULL );
   LineTo( pdp->wincon.hDC, pdp->nWidth, nStart+1 );

   SelectObject( pdp->wincon.hDC, hPenShadow );
   MoveToEx( pdp->wincon.hDC, 0, nStart+2, NULL );
   LineTo( pdp->wincon.hDC, pdp->nWidth, nStart+2 );

   SelectObject( pdp->wincon.hDC, hPenDkShadow );
   MoveToEx( pdp->wincon.hDC, 0, nStart+3, NULL );
   LineTo( pdp->wincon.hDC, pdp->nWidth, nStart+3 );
}

//----------------------------------------------------------------------------

void CPROC KeystrokePaste( PCONSOLE_INFO pdp )
{
    if( OpenClipboard(NULL) )
    {
        _32 format;
        // successful open...
        format = EnumClipboardFormats( 0 );
        while( format )
        {
            //DECLTEXT( msg, WIDE("                                     ") );
            //msg.data.size = sprintf( msg.data.data, WIDE("Format: %d"), format );
            //EnqueLink( pdp->ps->Command->ppOutput, SegDuplicate( (PTEXT)&msg ) );
            if( format == CF_TEXT )
            {
                HANDLE hData = GetClipboardData( CF_TEXT );
                LPVOID pData = GlobalLock( hData );
                PTEXT pStroke = SegCreateFromText( (CTEXTSTR)pData );
                int ofs, n;
                GlobalUnlock( hData );
                n = ofs = 0;
                while( pStroke->data.data[n] )
                {
                    pStroke->data.data[ofs] = pStroke->data.data[n];
                    if( pStroke->data.data[n] == '\r' ) // trash extra returns... keep newlines
                    {
                        n++;
                        continue;
                    }
                    else
                    {
                        ofs++;
                        n++;
                    }           
                }
                pStroke->data.size = ofs;
                pStroke->data.data[ofs] = pStroke->data.data[n];
                WinLogicDoStroke( pdp, pStroke );
                //EnqueLink( pdp->ps->Command->ppOutput, SegDuplicate(pStroke) );
                LineRelease( pStroke );
                break;
            }
            format = EnumClipboardFormats( format );
        }
        CloseClipboard();
    }
    else
    {
        DECLTEXT( msg, WIDE("Clipboard was not available") );
#ifdef __DEKWARE_PLUGIN__
		  EnqueLink( &pdp->common.Owner->Command->Output, &msg );
#endif
    }
    return;

}

//----------------------------------------------------------------------------

VOID KeyEventProc(PCONSOLE_INFO pdp, KEY_EVENT_RECORD event)
{
   // this must here gather keystrokes and pass them forward into the
   // opened sentience...
//   PTEXT temp;
   int bOutput = FALSE;
   int mod = KEYMOD_NORMAL;
   if( !pdp ) // not a valid window handle/device path
        return;
   //Log( WIDE("Entering keyproc...") );
   EnterCriticalSec( &pdp->Lock );
   //while( LockedExchange( &pdp->common.CommandInfo->CollectionBufferLock, 1 ) )
   //   Sleep(0);
   //Log1( WIDE("mod = %x"), pdp->dwControlKeyState );
   mod = pdp->dwControlKeyState;

   if( event.bKeyDown )
   {
      // here is where we evaluate the curent keystroke....
      for( ; event.wRepeatCount; event.wRepeatCount-- )
		{
			BYTE KeyState[256];
			DECLTEXT( key, WIDE("                   ") );
			GetKeyboardState( KeyState );
			SetTextSize( &key, ToAscii( event.wVirtualKeyCode
											  , event.wVirtualScanCode
											  , KeyState
											  , (unsigned short*)key.data.data
											  , 0 ) );
			KeyPressHandler( pdp, event.wVirtualKeyCode, mod, (PTEXT)&key );
		}
	}
   else
   {
		// flag is redundant for CONTOLKEY type...
		// key up's only matter if key is upaction flaged...
		switch( KeyDefs[event.wVirtualKeyCode].op[mod].bFunction )
		{
		case CONTROLKEY:
			KeyDefs[event.wVirtualKeyCode].op[mod].data.ControlKey( &pdp->dwControlKeyState, FALSE );
			break;
		}
	}
	LeaveCriticalSec( &pdp->Lock );
	//lprintf( WIDE("Left critical section on wincon.") );
}

//----------------------------------------------------------------------------

HWND hWndFocused;

//----------------------------------------------------------------------------

int CALLBACK NameDialog( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   static TEXTCHAR NameBuffer[256];
   switch( uMsg )
   {
   case WM_INITDIALOG:
      SetFocus( GetDlgItem( hWnd, EDT_NAME ) );
      return FALSE;
   case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case IDOK:
         GetDlgItemText( hWnd, EDT_NAME, NameBuffer, 256 );
         if( strlen( NameBuffer ) )
            EndDialog( hWnd, (int)NameBuffer );
         else
            EndDialog( hWnd, 0 );
         return TRUE;
      case IDCANCEL:
         EndDialog( hWnd, 0 );
         return TRUE;
      }
   }
   return FALSE;
}

//----------------------------------------------------------------------------

int CALLBACK ChildWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    PCONSOLE_INFO pdp;
    static _32 mouse_buttons;
    switch( uMsg )
   {
   case WM_SETFOCUS:
      hWndFocused = hWnd;
        FlashWindow( hWnd, FALSE );
      break;
   case WM_KILLFOCUS:
    {
         pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
         if( pdp )
            pdp->dwControlKeyState = 0;
       }
      hWndFocused = NULL;
      break;
   case WM_DESTROY:
      {
         pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
         if( pdp ) // check this to avoid double destruction
         {
             pdp->wincon.hWnd = NULL; // clear this so it's not double destroyed...
          	// if I do this - then I'm not sure which device should be left open...
#ifdef __DEKWARE_PLUGIN__
// magic happens here cause common is first thing in structure.
           	DestroyDataPath( (PDATAPATH)&(pdp->common) );
#endif
           	SetWindowLong( pdp->wincon.hWnd, WD_CONSOLE_INFO, 0 );
        }
      }
      break;
   case WM_MEASUREITEM: // received from menu drawing...
      {
         LPMEASUREITEMSTRUCT pmis = (LPMEASUREITEMSTRUCT) lParam;
         pmis->itemWidth = 80;
         pmis->itemHeight = 15;
      }
      break;
   case WM_DRAWITEM: // received from menu drawing
      {
         LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT) lParam;
         if( pdis->itemState & ODS_SELECTED )
            FillRect( pdis->hDC, &pdis->rcItem, hbrColorTable[15] );
         else
            FillRect( pdis->hDC, &pdis->rcItem, hbrColorTable[0] );
         pdis->rcItem.left += 2;
         pdis->rcItem.top ++;
         pdis->rcItem.right-= 2;
         pdis->rcItem.bottom--;
         FillRect( pdis->hDC, &pdis->rcItem, hbrColorTable[pdis->itemData] );
      }
      break;
   case WM_COMMAND:
      if( ( LOWORD( wParam ) >= MNU_BKBLACK ) &&
          ( LOWORD( wParam ) <= MNU_BKWHITE ) )
      {
         pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
         lprintf( WIDE("Set Default COlor background") );
         SetHistoryDefaultBackground( pdp->pCursor, LOWORD( wParam ) - MNU_BKBLACK );
         {
#ifdef __DEKWARE_PLUGIN__
            SetRegistryInt( WIDE("Dekware\\Wincon\\Background")
                          , GetText( GetName( pdp->common.Owner->Current ) )
								  , LOWORD( wParam ) - MNU_BKBLACK );
#endif
         }
      }
      else if( ( LOWORD( wParam ) >= MNU_BLACK ) &&
               ( LOWORD( wParam ) <= MNU_WHITE ) )
      {
         pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
         lprintf( WIDE("Set Default COlor Foreground") );
         SetHistoryDefaultForeground( pdp->pCursor, LOWORD( wParam ) - MNU_BKBLACK );
         {
#ifdef __DEKWARE_PLUGIN__
            SetRegistryInt( WIDE("Dekware\\Wincon\\Foreground")
                          , GetText( GetName( pdp->common.Owner->Current ) )
								  , LOWORD( wParam ) - MNU_BKBLACK );
#endif
         }
      }
      else switch( LOWORD( wParam ) )
      {
      case MNU_DIRECT:
         {
            pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
            pdp->flags.bDirect ^= 1;
            {
#ifdef __DEKWARE_PLUGIN__
               SetRegistryInt( WIDE("Dekware\\Wincon\\Direct")
                             , GetText( GetName( pdp->common.Owner->Current ) )
									  , pdp->flags.bDirect );
#endif
            }
            EnterCriticalSec( &pdp->Lock );
            ChildCalculate( pdp );
            LeaveCriticalSec( &pdp->Lock );
         }
         break;
      case MNU_HISTORYSIZE25:
      case MNU_HISTORYSIZE50:
      case MNU_HISTORYSIZE75:
      case MNU_HISTORYSIZE100:
         {
            pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
				pdp->nHistoryPercent =  LOWORD( wParam ) - MNU_HISTORYSIZE25;
            EnterCriticalSec( &pdp->Lock );
            WinLogicCalculateHistory( pdp );
            LeaveCriticalSec( &pdp->Lock );
				{
#ifdef __DEKWARE_PLUGIN__
					SetRegistryInt( WIDE("Dekware\\Wincon\\History")
									  , GetText( GetName( pdp->common.Owner->Current ) )
									  , pdp->nHistoryPercent );
#endif
				}
         }
         break;
      case MNU_FONT:
         {
            pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
            pdp->wincon.cfFont.hwndOwner = hWnd;
            if( ChooseFont( &pdp->wincon.cfFont ) )
            {
#ifdef __DEKWARE_PLUGIN__
               SetRegistryBinary( WIDE("Dekware\\Wincon\\Font")
                                , GetText( GetName( pdp->common.Owner->Current ) )
										  , &pdp->wincon.lfFont, sizeof( LOGFONT ) );
#endif
               DeleteObject( pdp->wincon.hFont );
               pdp->wincon.hFont = CreateFontIndirect( pdp->wincon.cfFont.lpLogFont );
               SelectObject( pdp->wincon.hDC, pdp->wincon.hFont );
               {
                  TEXTMETRIC tm;
                  GetTextMetrics( pdp->wincon.hDC, &tm );
                  pdp->nFontHeight = tm.tmHeight;
                  pdp->nFontWidth = tm.tmAveCharWidth;
               }
               ChildCalculate( pdp );
               //InvalidateRect( hWnd, NULL, TRUE );
            }
         }
         break;
      case MNU_NEW:
         {
            TEXTCHAR *Result;
            //PENTITY pe;
            //PSENTIENT ps;
            Result = (TEXTCHAR*)DialogBox( hInstMe, WIDE("NameDialog"), hWnd, NameDialog );
#ifdef __DEKWARE_PLUGIN__
            if( Result )
            {
               PVARTEXT vt;
               vt = VarTextCreate();
               vtprintf( vt, WIDE("/NewWindow NewWindow"), Result );
               EnqueLink( &(GetTheVoid()->pControlledBy->Command->Input), burst( VarTextGet( vt ) ) );
               VarTextEmpty( vt );
               WakeAThread( GetTheVoid()->pControlledBy );
				}
#endif
         }
         break;
      }
      break;
   case WM_RBUTTONDOWN:
      {
         int cmd;
         POINT mpos;
         pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
         GetCursorPos( &mpos );
         CheckMenuItem( hHistoryMenu
                       , MNU_HISTORYSIZE25+pdp->nHistoryPercent
                       , MF_BYCOMMAND|MF_CHECKED    );
            if( pdp->flags.bDirect )
                CheckMenuItem( hChildMenu
                             , MNU_DIRECT
                             , MF_BYCOMMAND|MF_CHECKED );
         cmd = TrackPopupMenu( hChildMenu, TPM_LEFTALIGN|TPM_TOPALIGN
                             , mpos.x, mpos.y, 0
                             , hWnd, NULL );
         CheckMenuItem( hHistoryMenu
                       , MNU_HISTORYSIZE25+pdp->nHistoryPercent
                       , MF_BYCOMMAND|MF_UNCHECKED  );
            CheckMenuItem( hChildMenu
                         , MNU_DIRECT
                         , MF_BYCOMMAND|MF_UNCHECKED );
      }
      break;
    case WM_NCLBUTTONDOWN:
        {   
            RECT r;
            int row, col, xPos, yPos;
          pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
            GetWindowRect( hWnd, &r );
            xPos = LOWORD(lParam) - r.left; 
            yPos = HIWORD(lParam) - r.top; 
            mouse_buttons |= MK_LBUTTON;
            if( 0 )
   case WM_LBUTTONDOWN:
            {   
              pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
                xPos = LOWORD(lParam); 
                yPos = HIWORD(lParam); 
            }
            if( ConvertXYToLineCol( pdp, xPos, yPos
                                   , &row, &col ) )
            {
                pdp->mark_start.row = row;
                pdp->mark_start.col = col;
                pdp->flags.bUpdatingEnd = 1;
                pdp->flags.bMarking = 1;
                pdp->CurrentMarkInfo = pdp->CurrentLineInfo;
            }
        }
       break;
        {   
            int row, col, xPos, yPos;
            RECT r;
    case WM_NCLBUTTONUP:
            mouse_buttons &= ~MK_LBUTTON;
            GetWindowRect( hWnd, &r );
            pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
            xPos = LOWORD(lParam) - r.left; 
            yPos = HIWORD(lParam) - r.top; 
            if( 0 )
            {
    case WM_LBUTTONUP:
                pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
                mouse_buttons = wParam;
                xPos = LOWORD(lParam); 
                yPos = HIWORD(lParam); 
            }
            if( ConvertXYToLineCol( pdp, xPos, yPos
                                   , &row, &col ) )
            {
                if( pdp->CurrentMarkInfo != pdp->CurrentLineInfo )
                    break;
                if( pdp->flags.bUpdatingEnd )
                {
                    pdp->mark_end.row = row;
                    pdp->mark_end.col = col;
                    if( pdp->mark_end.row > pdp->mark_start.row )
                    {
                        int tmp = pdp->mark_end.row;
                        pdp->mark_end.row = pdp->mark_start.row;
                        pdp->mark_start.row = tmp;
                        tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 0;
                    }
                    else if( pdp->mark_end.row == pdp->mark_start.row
                             && ( pdp->mark_end.col < pdp->mark_start.col ) )
                    {
                        int tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 0;
                    }
                }
                else
                {
                    pdp->mark_start.col = col;
                    pdp->mark_start.row = row;
                    if( pdp->mark_end.row > pdp->mark_start.row )
                    {
                        int tmp = pdp->mark_end.row;
                        pdp->mark_end.row = pdp->mark_start.row;
                        pdp->mark_start.row = tmp;
                        tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 1;
                    }
                    else if( pdp->mark_end.row == pdp->mark_start.row
                                 && pdp->mark_end.col < pdp->mark_start.col )
                    {
                        int tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 1;
                    }
                }
do_mark_copy:
                if( pdp->mark_start.row != pdp->mark_end.row 
                    || pdp->mark_start.col != pdp->mark_end.col )
                {
                    TEXTCHAR *data = GetDataFromBlock( pdp );
                    if( data && OpenClipboard(NULL) )
                    {
                        int nLen = strlen( data ) + 1;
                        HGLOBAL mem = GlobalAlloc( GMEM_MOVEABLE, nLen );
                        MemCpy( GlobalLock( mem ), data, nLen );
                        GlobalUnlock( mem );
                        EmptyClipboard();
                        SetClipboardData( CF_TEXT, mem );
                        CloseClipboard();
                        GlobalFree( mem );
                        //Log( data );
                        Release( data );
                    }
                }   
                pdp->flags.bMarking = 0;
					 InvalidateRect( hWnd, NULL, FALSE );
            }
        }
        break;
    case WM_RBUTTONUP:
        break;
    case WM_NCMOUSEMOVE:
        {
            int row, col, xPos, yPos;
            RECT r;
            //static int _row, _col;
            GetWindowRect( hWnd, &r );
				pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
				xPos = ( LOWORD(lParam) - r.left ) - ( GetSystemMetrics( SM_CXSIZE ) +
															 GetSystemMetrics( SM_CXBORDER ) +
																  GetSystemMetrics( SM_CXFRAME ) );
				//lprintf( WIDE("Adjusting size by %d %d %d %d %d")
				//									, GetSystemMetrics( SM_CYSIZEFRAME )
				//									, GetSystemMetrics( SM_CYCAPTION )
				//									, GetSystemMetrics( SM_CYMENU )
				//									, GetSystemMetrics( SM_CYBORDER )
				//									, GetSystemMetrics( SM_CYEDGE ) );

				yPos = ( HIWORD(lParam) - ( r.top
													//+ GetSystemMetrics( SM_CYSIZEFRAME )
													+ GetSystemMetrics( SM_CYCAPTION )
													+ GetSystemMetrics( SM_CYMENU )
													//+ GetSystemMetrics( SM_CYBORDER )
													+ GetSystemMetrics( SM_CYEDGE )
												  ) );
            if( 0 )
            {
    case WM_MOUSEMOVE:
              pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
                xPos = LOWORD(lParam); 
                yPos = HIWORD(lParam); 
                mouse_buttons = wParam;
            }
            if( pdp->flags.bMarking &&
                 ConvertXYToLineCol( pdp, xPos, yPos
                                   , &row, &col ) )
            {
                if( pdp->CurrentMarkInfo != pdp->CurrentLineInfo )
                    break;
                if( pdp->flags.bUpdatingEnd )
                {
                    pdp->mark_end.col = col;
                    pdp->mark_end.row = row;
                    if( pdp->mark_end.row > pdp->mark_start.row )
                    {
                        int tmp = pdp->mark_end.row;
                        pdp->mark_end.row = pdp->mark_start.row;
                        pdp->mark_start.row = tmp;

                        tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;

                        pdp->flags.bUpdatingEnd = 0;
                    }
                    else if( pdp->mark_end.row == pdp->mark_start.row &&
                              pdp->mark_end.col < pdp->mark_start.col )
                    {
                        int tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 0;
                    }
                }
                else
                {
                    pdp->mark_start.col = col;
                    pdp->mark_start.row = row;
                    if( pdp->mark_end.row > pdp->mark_start.row )
                    {
                        int tmp = pdp->mark_end.row;
                        pdp->mark_end.row = pdp->mark_start.row;
                        pdp->mark_start.row = tmp;
                        tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 1;
                    }
                    else if( pdp->mark_end.row == pdp->mark_start.row &&
                              pdp->mark_end.col < pdp->mark_start.col )
                    {
                        int tmp = pdp->mark_end.col;
                        pdp->mark_end.col = pdp->mark_start.col;
                        pdp->mark_start.col = tmp;
                        pdp->flags.bUpdatingEnd = 1;
                    }
                }
					 InvalidateRect( hWnd, NULL, FALSE );
                if( !(mouse_buttons & MK_LBUTTON) )
                {
                    goto do_mark_copy;
                }
            }
        }
        break;
   case WM_WINDOWPOSCHANGED:
		{
			//LPWINDOWPOS pos = (LPWINDOWPOS)lParam;
			// can get pos changed when destroyed.
			pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
			if( pdp )
			{
				GetClientRect( pdp->wincon.hWnd, &pdp->rArea );
				ChildCalculate( pdp );
				InvalidateRect( hWnd, NULL, FALSE );
			}
		}
		break;
   case WM_PAINT:
		{
			pdp = (PCONSOLE_INFO)GetWindowLong( hWnd, WD_CONSOLE_INFO );
			//Log( WIDE("Attempt enter painting...") );
			EnterCriticalSec( &pdp->Lock );
			RenderConsole( pdp );
			LeaveCriticalSec( &pdp->Lock );
			//Log( WIDE("Left critical section pdp") );
		}
		break;

   case WM_CREATE:
      {
         LPCREATESTRUCT pcs;
         pcs = (LPCREATESTRUCT)lParam;
         if( pcs )
         {
            pdp = (PCONSOLE_INFO)((LPMDICREATESTRUCT)pcs->lpCreateParams)->lParam; // user passed param...
#ifdef __DEKWARE_PLUGIN__
				SetWindowText( hWnd, GetText( GetName( pdp->common.Owner->Current ) ) );
#endif
            SetWindowLong( hWnd, WD_CONSOLE_INFO, (LONG)pdp );
            {
               pdp->wincon.hbrBackground = CreateSolidBrush( pdp->wincon.crBackground );
               pdp->wincon.hbrCommandBackground = CreateSolidBrush( pdp->wincon.crCommandBackground );
            }
            pdp->wincon.hDC = GetDC( hWnd );
            pdp->wincon.cfFont = cfDefaultFont;
#ifdef __DEKWARE_PLUGIN__
            if( !GetRegistryBinary( WIDE("Dekware\\Wincon\\Font")
                                  , GetText( GetName( pdp->common.Owner->Current ) )
                                  , &pdp->wincon.lfFont, sizeof( LOGFONT ) ) )
					pdp->wincon.lfFont = lfDefaultFont;
#endif
            pdp->wincon.cfFont.lpLogFont = &pdp->wincon.lfFont;
            pdp->wincon.hFont = CreateFontIndirect( pdp->wincon.cfFont.lpLogFont );
            SelectObject( pdp->wincon.hDC, pdp->wincon.hFont );
            {
               TEXTMETRIC tm;
               GetTextMetrics( pdp->wincon.hDC, &tm );
               pdp->nFontHeight = tm.tmHeight;
               pdp->nFontWidth = tm.tmAveCharWidth;
            }
            if( !hWndFocused )
                hWndFocused = hWnd;
                // hold off till here to set this - GetTextMetric
                // can apparently relinquish while getting itself.
            pdp->wincon.hWnd = hWnd;
         }
      }
      return 0;
   }

   return DefMDIChildProc( hWnd, uMsg, wParam, lParam );
}


//----------------------------------------------------------------------------

int CALLBACK FrameWindowProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
//   HVIDEO hVideo;

   switch( uMsg )
   {
   case WM_DESTROY:
      {
         PostQuitMessage( 0xD1E );
      }
      break;
   case WM_SYSCOLORCHANGE:
      DeleteObject( hPenNormal );
      DeleteObject( hPenHighlight );
      DeleteObject( hPenShadow );
      DeleteObject( hPenDkShadow );
      hPenNormal = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DFACE ) );
      hPenHighlight = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DHILIGHT ) );
      hPenShadow = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DSHADOW ) );
      hPenDkShadow = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DDKSHADOW ) );
      break;
   case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
    case MNU_PASTE:
        {
          PCONSOLE_INFO pdp = (PCONSOLE_INFO)GetWindowLong( hWndFocused, WD_CONSOLE_INFO );
            KeystrokePaste( pdp );
        }
        break;
      case MNU_CASCADE:
         SendMessage( hWndMDI, WM_MDICASCADE, 0, 0 );
         break;
      case MNU_TILEVERT:
         SendMessage( hWndMDI, WM_MDITILE, MDITILE_VERTICAL, 0 );
         break;
      case MNU_TILEHORZ:
         SendMessage( hWndMDI, WM_MDITILE, MDITILE_HORIZONTAL, 0 );
         break;
      case MNU_FONT:
         {
            cfDefaultFont.hwndOwner = hWnd;
            if( ChooseFont( &cfDefaultFont ) )
            {
               SetRegistryBinary( WIDE("Dekware\\Wincon"), WIDE("DefaultFont"), &lfDefaultFont, sizeof( LOGFONT ) );
               DeleteObject( hFontDefault );
               hFontDefault = CreateFontIndirect( cfDefaultFont.lpLogFont );
            }
         }
         break;
      case MNU_NEW:
         {
            TEXTCHAR *Result;
            //PENTITY pe;
            //PSENTIENT ps;
            Result = (TEXTCHAR*)DialogBox( hInstMe, WIDE("NameDialog"), hWnd, NameDialog );
            if( Result )
            {
#ifdef __DEKWARE_PLUGIN__
               PVARTEXT vt;
               vt = VarTextCreate();
               vtprintf( vt, WIDE("/NewWindow Window"), Result );
					EnqueLink( &(GetTheVoid()->pControlledBy->Command->Input), burst( VarTextGet( vt ) ) );
               VarTextEmpty( vt );
               WakeAThread( GetTheVoid()->pControlledBy );
#endif
            }
         }
         break;
      }
      break;
   case WM_CREATECHILD:
      {
         PCONSOLE_INFO pmdp = (PCONSOLE_INFO) lParam;
         MDICREATESTRUCT mcs;
         mcs.szClass = WIDE("DekwareChildClass");
         mcs.szTitle = WIDE("Object Command Console");
         mcs.hOwner = hInstMe;
         mcs.cy =
         mcs.cx =
         mcs.y =
         mcs.x = CW_USEDEFAULT;
         mcs.style = WS_VISIBLE;
         mcs.lParam = lParam;
         SendMessage( hWndMDI, WM_MDICREATE, 0, (LPARAM)&mcs );
         if( !pmdp->wincon.hWnd )
            pmdp->wincon.hWnd = (HWND)INVALID_HANDLE_VALUE;
         return TRUE;
      }
      break;
   case WM_CREATE:
      {
         HDC hDC;
         hDC = GetDC( hWnd );
         // check registry to see if prior font setting exists....
         if( !GetRegistryBinary( WIDE("Dekware\\Wincon"), WIDE("DefaultFont"), &lfDefaultFont, sizeof( LOGFONT ) ) )
         {
            static const LOGFONT lfDefaultFontVals= { -13
                             , 0, 0, 0
                             , 0x100
                             , FALSE, FALSE, FALSE
                             , ANSI_CHARSET
                             , OUT_DEFAULT_PRECIS
                             , CLIP_DEFAULT_PRECIS
                             , DEFAULT_QUALITY
                             , DEFAULT_PITCH | FF_DONTCARE
                             , WIDE("Lucida Console") }; // information about the font...
            MemCpy( &lfDefaultFont, (POINTER)&lfDefaultFontVals, sizeof( LOGFONT ) );
         }
         hFontDefault = CreateFontIndirect( &lfDefaultFont );
         SelectObject( hDC, hFontDefault );
         {
            TEXTMETRIC tm;
            GetTextMetrics( hDC, &tm );
         }
         ReleaseDC( hWnd, hDC );
         {
             CLIENTCREATESTRUCT ccs;
             /* Find window menu where children will be listed */
             ccs.hWindowMenu = GetSubMenu( GetMenu(hWnd),2);
             ccs.idFirstChild = 4100;//IDM_WINDOWCHILD;
             hWndMDI = CreateWindow( WIDE("MDICLIENT"), NULL
                     , WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_VSCROLL|WS_HSCROLL
                     , 0, 0, 0, 0, hWnd, (HMENU)0xcac/* ????*/
                     , hInstMe, (void*)&ccs );
         }
      }
      break;
   }

   return DefFrameProc( hWnd, hWndMDI, uMsg, wParam, lParam );
}

//----------------------------------------------------------------------------
HANDLE hMessageThread;
DWORD dwMessageThread;
_32 exiting;
PTRSZVAL CPROC FrameWindowThread( PTHREAD thread )
{
	MSG msg;
	// create message queue - make sure we're able to handle incoming messages...
	PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE );
	hWndFrame = CreateWindow( WIDE("DekwareFrameClass"),
                 WIDE("Dekware Interface"),
                 (WS_VISIBLE|WS_OVERLAPPEDWINDOW),
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 CW_USEDEFAULT,
                 NULL, // Parent
                 NULL, // Menu
                 hInstMe, // GetModuleHandle(NULL),
                 NULL );

	{
		while( GetMessage( &msg, NULL, 0, 0 ) )
		{
			if( !TranslateMDISysAccel( hWndMDI, &msg ) )
			{
				//TranslateAccelerator( &msg );
				DispatchMessage( &msg );
			}
		}
	}
#ifdef __DEKWARE_PLUGIN__
	if( !exiting )
		ExitNexus();
#endif
	//DestroyEntity( THE_VOID );
	hMessageThread = NULL;
	return 0;

}

//----------------------------------------------------------------------------
static LRESULT CALLBACK KeyHook(
    int code,   // hook code
    WPARAM wParam,  // virtual-key code
    LPARAM lParam   // keystroke-message information
   )
{
         KEY_EVENT_RECORD ker;
         /*
             BOOL bKeyDown;
             WORD wRepeatCount;
             WORD wVirtualKeyCode;
             WORD wVirtualScanCode;
             union {
                 WCHAR UnicodeChar;
                 CHAR  AsciiChar;
             } uChar;
             DWORD dwControlKeyState;
                 */
   if( hWndFocused )
   {
         ker.bKeyDown = !(lParam & 0x80000000);
         ker.wRepeatCount = LOWORD( lParam );
         ker.wVirtualKeyCode = wParam;
         ker.wVirtualScanCode = (WORD)((lParam & 0xFF0000 ) >> 16);
         ker.uChar.UnicodeChar = 0;
         ker.dwControlKeyState = 0;

         KeyEventProc( (PCONSOLE_INFO)GetWindowLong( GetFocus(), WD_CONSOLE_INFO ), ker );
   }
   return CallNextHookEx( hKeyHook, code, wParam, lParam );
}


//----------------------------------------------------------------------------

void CreateFrameWindow( void )
{
	if( !hWndFrame && !hWndMDI )
	{
		PTHREAD thread = ThreadTo( FrameWindowThread, 0 );
		/*
		 hMessageThread = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)
		 , NULL, 0, (DWORD*)&dwMessageThread );
		 */
		while( !hWndFrame || !hWndMDI )
		{
			Log( WIDE("Waiting for windows to be made.") );
			Sleep( 10 );
		}
		hKeyHook = SetWindowsHookEx( WH_KEYBOARD, (HOOKPROC)KeyHook
											, NULL, GetThreadID( thread ) & 0xFFFFFFFF );
		//Log1( WIDE("Done creating window... %d"), hKeyHook );
		GetLastError();
	}
}

//----------------------------------------------------------------------------
static int WindowRegistered;
int RegisterWindows( void )
{
   WNDCLASS wc;
//   WindowBorderHeight = ( GetSystemMetrics( SM_CYBORDER ) * 2 )
//                      + GetSystemMetrics( SM_CYCAPTION );
   if( WindowRegistered )
      return TRUE;
   if( !aClassFrame )
   {
      memset( &wc, 0, sizeof(WNDCLASS) );
      wc.style = CS_OWNDC | CS_GLOBALCLASS;

      wc.lpfnWndProc = (WNDPROC)FrameWindowProc;
      wc.hInstance = hInstMe; // GetModuleHandle(NULL);
      wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1);
      wc.hCursor = LoadCursor( NULL, IDC_ARROW );
      wc.lpszClassName = WIDE("DekwareFrameClass");
      wc.lpszMenuName = WIDE("FRAME_MENU");
      wc.cbWndExtra = 4;  // one extra DWORD


      aClassFrame = RegisterClass( &wc );
      if( !aClassFrame )
         return FALSE;
   }
   if( !aClassChild )
   {
      memset( &wc, 0, sizeof(WNDCLASS) );
      wc.style = CS_OWNDC | CS_GLOBALCLASS;

      wc.lpfnWndProc = (WNDPROC)ChildWindowProc;
      wc.hInstance = hInstMe; // GetModuleHandle(NULL);
      wc.hCursor = LoadCursor( NULL, IDC_ARROW );
      {
         LOGBRUSH lbr;
         lbr.lbStyle = BS_SOLID;
         lbr.lbColor = RGB( 0, 0, 0 );
         lbr.lbHatch = 0; // ignored
         wc.hbrBackground = (HBRUSH)CreateBrushIndirect( &lbr );
         //wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
      }
      wc.lpszClassName = WIDE("DekwareChildClass");
      wc.cbWndExtra = 4;  // one extra DWORD

      aClassChild = RegisterClass( &wc );
      if( !aClassChild )
         return FALSE;
   }
   hChildMenu = CreatePopupMenu();
   AppendMenu( hChildMenu, MF_STRING, MNU_NEW, WIDE("New Window") );
   AppendMenu( hChildMenu, MF_STRING, MNU_FONT, WIDE("Set Font") );
   {
      hHistoryMenu = CreatePopupMenu();
      AppendMenu( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE25, WIDE("25%") );
      AppendMenu( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE50, WIDE("50%") );
      AppendMenu( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE75, WIDE("75%") );
      AppendMenu( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE100, WIDE("100%") );
      AppendMenu( hChildMenu, MF_STRING|MF_POPUP, (int)hHistoryMenu, WIDE("History Display Size") );
   }
   {
      HMENU hColorMenu, hColorMenu2;
      hColorMenu = CreatePopupMenu();
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_BLACK, (LPCTSTR)0 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_BLUE, (LPCTSTR)1 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_GREEN, (LPCTSTR)2 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_CYAN, (LPCTSTR)3 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_RED, (LPCTSTR)4 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_MAGENTA, (LPCTSTR)5 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_DKYEL, (LPCTSTR)6 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_GREY, (LPCTSTR)7 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_DKGREY, (LPCTSTR)8 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_LTBLUE, (LPCTSTR)9 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_LTGREEN, (LPCTSTR)10 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_LTCYAN, (LPCTSTR)11 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_LTRED, (LPCTSTR)12 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_LTMAG, (LPCTSTR)13 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_YELLOW, (LPCTSTR)14 );
      AppendMenu( hColorMenu, MF_OWNERDRAW, MNU_WHITE, (LPCTSTR)15 );
      AppendMenu( hChildMenu, MF_STRING|MF_POPUP, (int)hColorMenu, WIDE("Text Color") );
      hColorMenu2 = CreatePopupMenu();
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKBLACK, (LPCTSTR)0 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKBLUE, (LPCTSTR)1 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKGREEN, (LPCTSTR)2 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKCYAN, (LPCTSTR)3 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKRED, (LPCTSTR)4 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKMAGENTA, (LPCTSTR)5 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKDKYEL, (LPCTSTR)6 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKGREY,(LPCTSTR) 7 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKDKGREY, (LPCTSTR)8 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKLTBLUE, (LPCTSTR)9 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKLTGREEN, (LPCTSTR)10 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKLTCYAN, (LPCTSTR)11 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKLTRED, (LPCTSTR)12 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKLTMAG, (LPCTSTR)13 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKYELLOW, (LPCTSTR)14 );
      AppendMenu( hColorMenu2, MF_OWNERDRAW, MNU_BKWHITE, (LPCTSTR)15 );
      AppendMenu( hChildMenu, MF_STRING|MF_POPUP, (int)hColorMenu2, WIDE("Background Color") );
   }
   AppendMenu( hChildMenu, MF_STRING, 0, WIDE("Command Color") );
   AppendMenu( hChildMenu, MF_STRING, 0, WIDE("Command Background Color") );
   AppendMenu( hChildMenu, MF_STRING, MNU_DIRECT, WIDE("Direct Mode") );
   {
        int n;
      for( n = 0; n < 16; n++ )
         hbrColorTable[n] = CreateSolidBrush( crColorTable[n] );
   }

   if( !cfDefaultFont.lStructSize )
   {
      cfDefaultFont.lStructSize = sizeof( CHOOSEFONT );
      cfDefaultFont.hDC = NULL;
      cfDefaultFont.lpLogFont = &lfDefaultFont;
      cfDefaultFont.Flags = CF_FIXEDPITCHONLY
                          | CF_SCREENFONTS
                          | CF_EFFECTS
                          | CF_INITTOLOGFONTSTRUCT ;
      cfDefaultFont.rgbColors = RGB( 0, 0, 0 );
      cfDefaultFont.lpszStyle = (LPSTR)NULL;
      cfDefaultFont.nFontType = SCREEN_FONTTYPE;
   }
   hPenNormal = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DFACE ) );
   hPenHighlight = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DHILIGHT ) );
   hPenShadow = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DSHADOW ) );
   hPenDkShadow = CreatePen( PS_SOLID, 0, GetSysColor( COLOR_3DDKSHADOW ) );
   hPenCursor = CreatePen( PS_SOLID, 0, RGB( 255, 255, 255 ) );
   WindowRegistered = TRUE;
   return TRUE;
}

//----------------------------------------------------------------------------

void UnregisterWindows( void )
{
	int n;
   if( !WindowRegistered )
		return;
   // this is called from external sources...
	exiting = 1;
	PostThreadMessage( dwMessageThread, WM_QUIT, 0, 0 );
	while( hMessageThread )
		Sleep( 1 );
	if( hKeyHook )
	{
		UnhookWindowsHookEx( hKeyHook );
		hKeyHook = NULL;
	}
	if( aClassFrame )
	{
		UnregisterClass( (TEXTCHAR*)aClassFrame, hInstMe );
      aClassFrame = NULL;
	}
	if( aClassChild )
	{
		UnregisterClass( (TEXTCHAR*)aClassChild, hInstMe );
      aClassChild = NULL;
	}
	if( hChildMenu )
	{
		DestroyMenu( hChildMenu );
      hChildMenu = NULL;
	}
	for( n = 0; n < 16; n++ )
		DeleteObject( hbrColorTable[n] );
	WindowRegistered = FALSE;
}
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
int StatusField( PSENTIENT ps, PTEXT parameters )
{
   // /status <size> string....
   // if size is positive then it is the next field from the left
   // if size is negative it is meausred from the right.

   return 1;
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
int UpdateStatusBar( PSENTIENT ps, PTEXT parameters )
{
   // no parameters... just calls render status bar - if status bar active

   return 1;
}
#endif
//void CPROC WinconPrompt( PCONSOLE_INFO pdp )
void CPROC WinconPrompt( PDATAPATH pdp )
{
   PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;
   //lprintf( WIDE("rendering command line - had a prompt issued...") );
    //RenderCommandLine( (PCONSOLE_INFO)pdp );
#ifdef __DEKWARE_PLUGIN__
	prompt( ((PCONSOLE_INFO)pdp)->common.Owner ); // will need to call this since it's bypassed
#endif
}

//----------------------------------------------------------------------------

//static int CPROC Close( PCONSOLE_INFO pPath )
static int CPROC Close( PDATAPATH pPath )
{
	PCONSOLE_INFO pdp = (PCONSOLE_INFO)pPath;
	//PTEXT pHistory;

#ifdef __DEKWARE_PLUGIN__
#if 0
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_rows );
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_cols );
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_cursorx );
	RemoveVolatileVariable( pdp->common.Owner->Current, &vve_cursory );
#endif
	pdp->common.Close = NULL;
	pdp->common.Write = NULL;
	pdp->common.Read = NULL;
	pdp->common.Type = 0;
#endif
	DestroyHistoryRegion( pdp->pHistory );
	DestroyHistoryCursor( pdp->pCursor );
	DestroyHistoryBrowser( pdp->pCurrentDisplay );
	DestroyHistoryBrowser( pdp->pHistoryDisplay );

   pdp->CurrentMarkInfo = NULL;
	pdp->CurrentLineInfo = NULL;

	SetWindowLong( pdp->wincon.hWnd, WD_CONSOLE_INFO, 0 );
	if( pdp->wincon.hWnd )
	{
		SendMessage( hWndMDI, WM_MDIDESTROY, (WPARAM)pdp->wincon.hWnd, 0 );
	}
	return 1;
}

//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
int CPROC SetFlash( PDATAPATH pdp, PSENTIENT ps, PTEXT parameters )
{
   PCONSOLE_INFO pmdp = (PCONSOLE_INFO)pdp;
   if( pmdp &&
       hWndFocused != pmdp->wincon.hWnd )
       FlashWindow( hWndFrame, TRUE );
    return 0;
}
#endif
//----------------------------------------------------------------------------
// methods for window logic routines to use as callbacks...
//----------------------------------------------------------------------------

static void CPROC DrawString( PCONSOLE_INFO pmdp, int x, int y, RECT *r, TEXTCHAR *s, int nShown, int nShow )
{
	//lprintf( WIDE("Adding string out : %s %d %d at %d,%d or (%d,%d)  (%d,%d)"), s, nShown, nShow,x,y,r->left,r->top, r->right, r->bottom );
   //InvalidateRect( pmdp->wincon.hDC, r, NULL );
	ExtTextOut( pmdp->wincon.hDC, x, y
				 , ETO_OPAQUE, r
				 , s + nShown
				 , nShow
				 , NULL );
   //ValidateRect( pmdp->wincon.hDC, r );
   //GdiFlush();
}

//----------------------------------------------------------------------------

static void CPROC SetCurrentColor( PCONSOLE_INFO pdp, enum current_color_type type, PTEXT segment )
{
	switch( type )
	{
	case COLOR_COMMAND:
		SetTextColor( pdp->wincon.hDC, pdp->wincon.crText = pdp->wincon.crCommand );
		SetBkColor( pdp->wincon.hDC, pdp->wincon.crBack = pdp->wincon.crCommandBackground );
		break;
	case COLOR_MARK:
		SetTextColor( pdp->wincon.hDC, pdp->wincon.crText = pdp->wincon.crMark );
		SetBkColor( pdp->wincon.hDC, pdp->wincon.crBack = pdp->wincon.crMarkBackground );
		break;
	case COLOR_DEFAULT:
		SetTextColor( pdp->wincon.hDC, pdp->wincon.crText = crColorTable[15] );
		SetBkColor( pdp->wincon.hDC, pdp->wincon.crBack = pdp->wincon.crBackground );
		break;
	case COLOR_SEGMENT:
		if( segment )
		{
			SetTextColor( pdp->wincon.hDC, pdp->wincon.crText = crColorTable[segment->format.flags.foreground] );
			SetBkColor( pdp->wincon.hDC, pdp->wincon.crBack = crColorTable[segment->format.flags.background] );
		}
      break;
	}
	//lprintf( WIDE("Set text color to %08lX %08lX")
	//		 , pdp->wincon.crText
	//		 , pdp->wincon.crBack );
}

//----------------------------------------------------------------------------

static void CPROC FillConsoleRect( PCONSOLE_INFO pdp, RECT *r, enum fill_color_type type )
{
	switch( type )
	{
	case FILL_COMMAND_BACK:
      FillRect( pdp->wincon.hDC, r, pdp->wincon.hbrBackground );
		break;
	case FILL_DISPLAY_BACK:
      FillRect( pdp->wincon.hDC, r, pdp->wincon.hbrCommandBackground );
      break;
	}
}

//----------------------------------------------------------------------------

static void CPROC Update( PCONSOLE_INFO pmdp, RECT *update )
{
	// passed region is the region which was updated by drawing
	// code.
   //lprintf( WIDE("... update (should have drawn?)"));
	// there is no action required to flush updates
	// to the display for this interface.
   //ValidateRect( pmdp->wincon.hWnd, update );
   ValidateRect( pmdp->wincon.hWnd, NULL );
	//InvalidateRect( pmdp->wincon.hWnd, NULL, FALSE );
   //GDIFlush();
}

DECLTEXT( WinConName, WIDE("wincon") );

static PTEXT DeviceVolatileVariableGet( WIDE("wincon"), WIDE("rows"), WIDE("console row count") )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&WinConName );
   return GetRows( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( WIDE("wincon"), WIDE("cols"), WIDE("console col count") )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&WinConName );
   return GetCols( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( WIDE("wincon"), WIDE("cursor_x"), WIDE("console cursor x(col) position") )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&WinConName );
   return GetCursorX( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( WIDE("wincon"), WIDE("cursor_y"), WIDE("console cursor y(row) position") )(PENTITY pe,PTEXT*ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&WinConName );
   return GetCursorY( (PTRSZVAL)pdp, pe, ppLastValue );
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
bool short_create;

static PDATAPATH OnInitDevice( WIDE("wincon"), WIDE("Windows MDI interactive interface") )( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
//PDATAPATH CPROC CreateConsole( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   PCONSOLE_INFO pdp;
   // get result from send message.... this will be useful data
   if( !RegisterWindows() )
      return NULL; // cancel load, unload library...

   CreateFrameWindow(); // start main window thread...

   pdp = (PCONSOLE_INFO)CreateDataPath( (PDATAPATH*)pChannel, CONSOLE_INFO );
	pdp->common.Owner = ps; // we need this early...

   pdp->pHistory = CreateHistoryRegion();
   pdp->pCursor = CreateHistoryCursor( pdp->pHistory );
   pdp->pCurrentDisplay = CreateHistoryBrowser( pdp->pHistory );
	pdp->pHistoryDisplay = CreateHistoryBrowser( pdp->pHistory );
	SetHistoryBrowserNoPageBreak( pdp->pHistoryDisplay );

   InitializeCriticalSec( &pdp->Lock );
   //Log( WIDE("Initialized section...") );

#if 0
   AddVolatileVariable( ps->Current, &vve_rows, (PTRSZVAL)pdp );
   AddVolatileVariable( ps->Current, &vve_cols, (PTRSZVAL)pdp );
   AddVolatileVariable( ps->Current, &vve_cursorx, (PTRSZVAL)pdp );
	AddVolatileVariable( ps->Current, &vve_cursory, (PTRSZVAL)pdp );
#endif
    //ps->flags.no_prompt = TRUE;
	pdp->common.CommandInfo = CreateCommandHistoryEx( WinconPrompt );
   pdp->common.Close = Close;
//   pdp->common.Read = NULL;
   pdp->common.Write = WinLogicWrite;
   //pdp->common.CommandInfo = &pdp->CommandInfo;
   pdp->common.flags.Data_Source = 1;
   //pdp->common.flags.Formatted = TRUE;


    {
        int nValue;
        if( GetRegistryInt( WIDE("Dekware\\Wincon\\Direct")
                        , GetText( GetName( pdp->common.Owner->Current ) )
                        , &nValue ) )
			  pdp->flags.bDirect = nValue;
		  if( GetRegistryInt( WIDE("Dekware\\Wincon\\History")
								  , GetText( GetName( pdp->common.Owner->Current ) )
								  , &nValue ) )
			  pdp->nHistoryPercent = nValue;
    }

   pdp->wincon.crCommand = RGB( 32, 192, 192 );
   pdp->wincon.crCommandBackground = RGB( 0, 0, 0 );
   pdp->wincon.crMark = RGB( 192, 192, 192 );
   pdp->wincon.crMarkBackground = RGB( 67, 116, 150 );

	pdp->nXPad = 5;
	pdp->nYPad = 5;
	pdp->nCmdLinePad = 2;

	if( short_create )
		return &pdp->common;

	pdp->FillConsoleRect = FillConsoleRect;
	pdp->RenderSeparator = RenderSeparator;
	pdp->DrawString = DrawString;
	pdp->KeystrokePaste = KeystrokePaste;
	pdp->SetCurrentColor = SetCurrentColor;
	pdp->RenderCursor = RenderCursor;
   pdp->Update = Update;

   PostMessage( hWndFrame, WM_CREATECHILD, 0, (LPARAM)pdp );
   while( !pdp->wincon.hWnd )
      Sleep( 10 );
   if( pdp->wincon.hWnd == INVALID_HANDLE_VALUE )
   {
      DestroyDataPath( (PDATAPATH)pdp );
      return NULL;
   }
   // this should be done with the act of showing the created window...
   //EnterCriticalSec( &pdp->Lock );
	//GetClientRect( pdp->hWnd, &rArea );
   //ChildCalculate( pdp );
   //LeaveCriticalSec( &pdp->Lock );
   return (PDATAPATH)pdp;
}
#endif

#ifdef __cplusplus_cli


using namespace dekware;
using namespace System;
using namespace System::Drawing;
using namespace System::Reflection;

namespace dewkare {
	namespace wincon {

    public ref class DataPathInterface
    {
	public: delegate void FillRect( void );
    }  ;
	


public ref class WinconDataPath//: public DataPath
{
	PSENTIENT ps;
	PENTITY pe;
	PDATAPATH pdp;
	PCONSOLE_INFO pmdp;




public:
	//int Width;
	//int Height;
	int nFontHeight;
	int nFontWidth;
	System::Drawing::Rectangle rArea;
	//dotnetcon::netcon ^console;
	//int nCommandLineStart;
	WinconDataPath( )//dotnetcon::netcon ^con )
	{
		pe = CreateEntityIn( THE_VOID, SegCreateFromText( WIDE("Console") ) );
		ps = CreateAwareness( pe );
	//Sentience ^sent;
	//ps = sent->ps;
		PDATAPATH pdp;
//CORE_PROC( PDATAPATH, CreateDataPath   )( PDATAPATH *ppWhere, int nExtra );
//#define CreateDataPath(where, pathname) (P##pathname)CreateDataPath( where, sizeof(pathname) - sizeof( DATAPATH ) )
		short_create = true;
		pmdp = (PCONSOLE_INFO)CreateConsole( &ps->Command, ps, NULL );
		short_create = false;
		pdp = &pmdp->common;
		pdp->pName = SegCreateFromText( WIDE("interface") );

	//pmdp->FillConsoleRect = FillConsoleRect;
	//pmdp->RenderSeparator = RenderSeparator;
	//pmdp->DrawString = DrawString;
	//pmdp->KeystrokePaste = KeystrokePaste;
	//pmdp->SetCurrentColor = SetCurrentColor;
	//pmdp->RenderCursor = RenderCursor;
	//pmdp->Update = Update;
            
	}

#ifdef __cplusplus_cli
   System::Windows::Forms::PaintEventArgs ^pea;
#endif




	void DoRenderConsole( System::Windows::Forms::PaintEventArgs ^e )
	{
		pea = e;
		RenderConsole( pmdp );
	}

	void Update( void )
	{
		pmdp->nWidth = rArea.Width;
		pmdp->nHeight = rArea.Height;
		pmdp->nFontWidth = nFontWidth;
		pmdp->nFontHeight = nFontHeight;
		pmdp->nDisplayLineStart = 0;
		pmdp->rArea.left = rArea.Left;
		pmdp->rArea.right = rArea.Right;
		pmdp->rArea.top = rArea.Top;
		pmdp->rArea.bottom = rArea.Bottom;
		ChildCalculate( pmdp );
	}
	bool IsInserting( void )
	{
		return ( pmdp->common.CommandInfo->CollectionInsert != 0 );
	}
};

	}
}

#endif


//----------------------------------------------------------------------------
// $Log: wincon.c,v $
// Revision 1.40  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.39  2005/02/24 03:10:49  d3x0r
// Fix behaviors to work better... now need to register terminal close and a couple other behaviors...
//
// Revision 1.38  2005/02/23 12:25:20  d3x0r
// Split consoles into a core, and various front ends... need a couple more visual studio projects for psicon, console
//
// Revision 1.37  2005/02/22 12:28:51  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.36  2005/01/27 17:33:57  d3x0r
// Remove minor logging
//
// Revision 1.35  2005/01/27 17:31:37  d3x0r
// psicon works now, added some locks to protect multiple accesses to datapath (render/update_write)
//
// Revision 1.34  2005/01/26 20:00:02  d3x0r
// Okay - need to do something about partial updates - such as command typing should only update that affected area of the screen...
//
// Revision 1.33  2005/01/23 04:07:58  d3x0r
// Hmm somehow between display rendering stopped working.
//
// Revision 1.32  2005/01/20 20:04:10  d3x0r
// Wincon nearly functions, consolecon ported to new coreocn lib, psicon is next.
//
// Revision 1.31  2005/01/20 06:10:19  d3x0r
// One down, 3 to convert... concore library should serve to encapsulate drawing logic and history code...
//
// Revision 1.30  2005/01/19 15:49:33  d3x0r
// Begin defining external methods so WinLogic and History can become a core-console library
//
// Revision 1.29  2005/01/17 11:21:46  d3x0r
// partial working status for psicon
//
// Revision 1.28  2004/09/29 09:31:32  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.27  2004/09/27 16:06:17  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.26  2004/09/24 17:52:23  d3x0r
// Fix spelling of ConvertXYToLineCol - attempt to fix better the non client mouse position
//
// Revision 1.25  2004/09/24 08:05:43  d3x0r
// okay console console seems to work pretty well, psicon also, even wincon all mark and play nice...
//
// Revision 1.24  2004/09/23 09:53:33  d3x0r
// Looks like it's in a usable state for windows... perhaps an alpha/beta to be released soon.
//
// Revision 1.23  2004/09/20 10:00:17  d3x0r
// Okay line up, wrapped, partial line, line up, history alignment, all sorts of things work very well now.
//
// Revision 1.22  2004/09/10 08:48:50  d3x0r
// progress not perfection... a grand dawning on the horizon though.
//
// Revision 1.21  2004/08/13 09:29:50  d3x0r
// checkpoint
//
// Revision 1.20  2004/06/13 01:23:43  d3x0r
// checkpoint.
//
// Revision 1.19  2004/06/12 09:11:09  d3x0r
// Checkpoint... moving to linux...
//
// Revision 1.18  2004/06/12 08:42:34  d3x0r
// Well it initializes, first character causes failure... all windows targets build...
//
//
// 2004/06/12 Removed prior history - comments obsolete now.
//
// 
