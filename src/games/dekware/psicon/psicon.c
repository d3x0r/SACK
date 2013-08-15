//#define DO_LOGGING
#ifdef _WIN32
#define _INCLUDE_CLIPBOARD
#endif
#ifndef FORCE_NO_INTERFACES
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#endif
//#define NO_LOGGING
#include <stdhdrs.h>
#include <stdio.h> // sprintf ?
#include <string.h> // strchr
#include <logging.h>

//#define USE_IMAGE_INTERFACE ImageInterface
#include <image.h>
//PIMAGE_INTERFACE ImageInterface;
//#define USE_RENDER_INTERFACE RenderInterface
#include <render.h>
//PRENDER_INTERFACE RenderInterface;
#include <controls.h>
#include <keybrd.h>

static CDATA crColorTable[16];
#if 0
= { Color( 0,0,1 ), Color( 0, 0, 128 ), Color( 0, 128, 0 )
                            , Color( 0, 128, 128 ), Color( 192, 32, 32 ), Color( 140, 0, 140 )
                            , Color( 160, 160, 0 ), Color( 192, 192, 192 )
                            , Color( 128, 128, 128 ), Color( 0, 0, 255 ), Color( 0, 255, 0 )
                            , Color( 0, 255, 255 ), Color( 255, 0, 0 ), Color( 255, 0, 255 )
                            , Color( 255, 255, 0 ), Color( 255, 255, 255 ) };
#endif

//#define PutStringEx(i,x,y,f,b,s,l) { Log6( WIDE("Putting string: %ld,%ld %ld %*.*s"), x,y,l,l,l,s); PutStringFontEx( i,x,y,f,b,s,l,NULL); }
//#define BlatColor(i,x,y,w,h,c)     { Log5( WIDE("BlatColor: %ld,%ld %ld,%ld %08lx"), x, y, w, h, c ); BlatColor( i,x,y,w,h,c ); }

#include "consolestruc.h"
#include "interface.h"
#include "WinLogic.h"

#define MNU_FONT 100
#define MNU_HISTORYSIZE25 101
#define MNU_HISTORYSIZE50 102
#define MNU_HISTORYSIZE75 103
#define MNU_HISTORYSIZE100 104
#define MNU_DIRECT         105
#define MNU_COMMAND_COLOR 106
#define MNU_COMMAND_BACK  107

#define MNU_BLACK    115
#define MNU_BLUE     116
#define MNU_GREEN    117
#define MNU_CYAN     118
#define MNU_RED      119
#define MNU_MAGENTA  120
#define MNU_DKYEL    121
#define MNU_GREY     122
#define MNU_DKGREY   123
#define MNU_LTBLUE   124
#define MNU_LTGREEN  125
#define MNU_LTCYAN   126
#define MNU_LTRED    127
#define MNU_LTMAG    128
#define MNU_YELLOW   129
#define MNU_WHITE    130

#define MNU_BKBLACK    ( 115 + 16 )
#define MNU_BKBLUE     ( 116 + 16 )
#define MNU_BKGREEN    ( 117 + 16 )
#define MNU_BKCYAN     ( 118 + 16 )
#define MNU_BKRED      ( 119 + 16 )
#define MNU_BKMAGENTA  ( 120 + 16 )
#define MNU_BKDKYEL    ( 121 + 16 )
#define MNU_BKGREY     ( 122 + 16 )
#define MNU_BKDKGREY   ( 123 + 16 )
#define MNU_BKLTBLUE   ( 124 + 16 )
#define MNU_BKLTGREEN  ( 125 + 16 )
#define MNU_BKLTCYAN   ( 126 + 16 )
#define MNU_BKLTRED    ( 127 + 16 )
#define MNU_BKLTMAG    ( 128 + 16 )
#define MNU_BKYELLOW   ( 129 + 16 )
#define MNU_BKWHITE    ( 130 + 16 )

//----------------------------------------------------------------------------
// only one master copy of this is really needed...

//extern int myTypeID;
#if 0
#ifdef _WIN32
__declspec(dllimport)
#else
extern
#endif
   int b95;
#endif

static struct psicon_local
{
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
} local_psicon_data;
#define l local_psicon_data
CDATA cPenNormal, cPenHighlight, cPenShadow, cPenDkShadow, cPenCursor;
PMENU hChildMenu;
PMENU hHistoryMenu;
static int bCreatingControl; // set when this is creating a control as opposed to someone externally creating one

//----------------------------------------------------------------------------

void CPROC RenderSeparator( PCONSOLE_INFO pdp, int nStart )
{
   //lprintf( WIDE("Render separator %d-%d %d"), 0, pdp->nWidth, nStart );
	if( nStart > 0 && nStart < pdp->nHeight )
	{
	// Render Command Line Separator
		do_hline( pdp->psicon.image, nStart, 0, pdp->nWidth, cPenHighlight );
		do_hline( pdp->psicon.image, nStart+1, 0, pdp->nWidth, cPenNormal );
		do_hline( pdp->psicon.image, nStart+2, 0, pdp->nWidth, cPenShadow );
		do_hline( pdp->psicon.image, nStart+3, 0, pdp->nWidth, cPenDkShadow );
      //SmudgeCommon( pdp->psicon.image );
	}
}

//----------------------------------------------------------------------------

void CPROC KeystrokePaste( PCONSOLE_INFO pdp )
{
#ifdef __WINDOWS__
	if( OpenClipboard(NULL) )
	{
		_32 format;
	// successful open...
		format = EnumClipboardFormats( 0 );
		while( format )
		{
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
#ifdef __DEKWARE_PLUGIN__
        DECLTEXT( msg, WIDE("Clipboard was not available") );
		  EnqueLink( &pdp->common.Output, (PTEXT)&msg );
#endif
    }
#endif
    return;

}

//----------------------------------------------------------------------------

int CPROC RenderChildWindow( PCOMMON pc )
{
	PCONSOLE_INFO pdp = (PCONSOLE_INFO)GetCommonUserData(pc);
	//lprintf( WIDE("Rendering window.") );
	if( !pdp )
	{
		Log( WIDE("How could we have gotten here without a pdp??") );
		return 0;
	}
	pdp->psicon.image = GetFrameSurface( pc );
   // this is the only place which size is changed.
	if( pdp->nWidth != pdp->psicon.image->width ||
		pdp->nHeight != pdp->psicon.image->height )
	{
		// nWidth/nHeight are set in child calculate
      // and acknowledge processing a new rarea rect.
		pdp->rArea.left = 0;
		pdp->rArea.right = pdp->psicon.image->width;
		pdp->rArea.top = 0;
		pdp->rArea.bottom = pdp->psicon.image->height;
		ChildCalculate( pdp );
	}
	else
		RenderConsole( pdp );
   return TRUE;
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------

int CPROC KeyEventProc( PCOMMON pc, _32 key )
{
	PCONSOLE_INFO pdp = (PCONSOLE_INFO)GetCommonUserData( pc );
	// this must here gather keystrokes and pass them forward into the
	// opened sentience...
	if( pdp )
	{
		TEXTCHAR character = GetKeyText( key );
		DECLTEXT( stroke, WIDE(" ") ); // single character ...
		int bOutput = FALSE;
		//Log1( WIDE("Key: %08x"), key );
		int mod = KEYMOD_NORMAL;
		if( !pdp ) // not a valid window handle/device path
			return 0;
		EnterCriticalSec( &pdp->Lock );

		// here is where we evaluate the curent keystroke....
		if( character )
		{
			stroke.data.data[0] = character;
			stroke.data.size = 1;
		}
		else
			stroke.data.size = 0;

		if( key & KEY_PRESSED )
		{
			KeyPressHandler( pdp, KEY_CODE(key), KEY_MOD(key), (PTEXT)&stroke );
			//SmudgeCommon( pdp->psicon.frame );
		}
		LeaveCriticalSec( &pdp->Lock );
	}
   return 1;
}

//----------------------------------------------------------------------------

int CPROC MouseHandler( PCOMMON pc, S_32 x, S_32 y, _32 b )
{
	static S_32 _x, _y;
	static _32 _b;
	//Log3( WIDE("Mouse thing...%ld,%ld %08lx"), x, y, b );
	{
		int xPos, yPos, row, col;
		PCONSOLE_INFO pdp;
		if( (b & MK_LBUTTON) && !(_b & MK_LBUTTON) )
        { // mouse down.
			  pdp = (PCONSOLE_INFO)GetCommonUserData( pc );
			  if( !pdp )
              return 0;
			  xPos = x; 
			  yPos = y;
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
		else if( ( !(b & MK_LBUTTON) && (_b & MK_LBUTTON) )
				  ||( b & MK_LBUTTON ) )
		{ // mouse up
			pdp = (PCONSOLE_INFO)GetCommonUserData( pc );
			if( !pdp )
				return 0;
			xPos = x;
			yPos = y;
			if( ConvertXYToLineCol( pdp, xPos, yPos
										 , &row, &col ) )
			{
				if( pdp->CurrentMarkInfo == pdp->CurrentLineInfo )
				{
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
					if( !(b & MK_LBUTTON) && (_b & MK_LBUTTON) )
					{
						if( pdp->mark_start.row != pdp->mark_end.row
							|| pdp->mark_start.col != pdp->mark_end.col )
						{
#ifdef _WIN32
							TEXTCHAR *data = GetDataFromBlock( pdp );
							if( data && OpenClipboard(NULL) )
							{
								size_t nLen = strlen( data ) + 1;
								HGLOBAL mem = GlobalAlloc( GMEM_MOVEABLE, nLen );
								MemCpy( GlobalLock( mem ), data, nLen );
								GlobalUnlock( mem );
								EmptyClipboard();
#ifndef CF_TEXT 
#define CF_TEXT 1
#endif
								SetClipboardData( CF_TEXT, mem );
                                CloseClipboard();
										  GlobalFree( mem );
                                //Log( data );
										  Release( data );
							}
#endif
						}
						Log( WIDE("Ending mark.") );
						pdp->flags.bMarking = 0;
						pdp->flags.bUpdatingEnd = 0;
					}
					RenderChildWindow( pdp->psicon.frame );
				}
			}
			if( !(b & MK_LBUTTON) && (_b & MK_LBUTTON) )
			{
				Log( WIDE("Ending mark(2).") );
				pdp->flags.bMarking = 0;
				pdp->flags.bUpdatingEnd = 0;
			}
		}
	}

	if( (b & MK_RBUTTON) && !(_b & MK_RBUTTON ) )
	{
		PCONSOLE_INFO pdp = (PCONSOLE_INFO)GetCommonUserData( pc );
		int cmd;
		if( !pdp )
			return 0;
		CheckPopupItem( hHistoryMenu
						  , MNU_HISTORYSIZE25+pdp->nHistoryPercent
						  , MF_BYCOMMAND|MF_CHECKED    );
		if( pdp->flags.bDirect )
			CheckPopupItem( hChildMenu
							  , MNU_DIRECT
							  , MF_BYCOMMAND|MF_CHECKED );
		cmd = TrackPopup( hChildMenu, pdp->psicon.frame );
		if( ( cmd >= MNU_BKBLACK ) &&
			( cmd <= MNU_BKWHITE ) )
		{
         SetHistoryDefaultBackground( pdp->pCursor, cmd - MNU_BKBLACK );
		}
		else if( ( cmd >= MNU_BLACK ) &&
				  ( cmd <= MNU_WHITE ) )
		{
         SetHistoryDefaultForeground( pdp->pCursor, cmd - MNU_BKBLACK );
		}
		else switch( cmd )
		{
		case MNU_DIRECT:
			{
				pdp->flags.bDirect ^= 1;
            /*
				 {
				 SetRegistryInt( WIDE("Dekware\\Wincon\\Direct")
				 , GetText( GetName( pdp->common.Owner->Current ) )
				 , pdp->flags.bDirect );
                                          }
                                */
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
				pdp->nHistoryPercent =  cmd - MNU_HISTORYSIZE25;
				if( pdp->flags.bHistoryShow ) // currently showing history
				{
					EnterCriticalSec( &pdp->Lock );
					ChildCalculate( pdp ); // changed history display...
					LeaveCriticalSec( &pdp->Lock );
				}
			}
         break;
      case MNU_FONT:
         {
				//pdp->cfFont.hwndOwner = hWnd;
				size_t size;
				SFTFont font;
            POINTER info = NULL;
            if( font = PickFont( -1, -1, &size, &info, NULL ) )
            {
					pdp->psicon.hFont = font;
					//GetDefaultFont();
					GetStringSizeFont( WIDE(" "), &pdp->nFontWidth, &pdp->nFontHeight, pdp->psicon.hFont );
               ChildCalculate( pdp );
            }
         }
            break;
        case MNU_COMMAND_COLOR:
            {
            CDATA color;
                if( PickColor( &color, pdp->psicon.crCommand, pdp->psicon.frame ) )
                    pdp->psicon.crCommand = color;
                else
                    Log2( WIDE("Colors %08x %08x"), color, pdp->psicon.crCommand );
            }
         break;
        case MNU_COMMAND_BACK:
            {
            CDATA color;
                if( PickColor( &color, pdp->psicon.crCommandBackground, pdp->psicon.frame ) )
                    pdp->psicon.crCommandBackground = color;
                else
                    Log2( WIDE("Colors %08x %08x"), color, pdp->psicon.crCommandBackground );
            }
         break;
      }
        CheckPopupItem( hHistoryMenu
                         , MNU_HISTORYSIZE25+pdp->nHistoryPercent
                         , MF_BYCOMMAND|MF_UNCHECKED  );
        CheckPopupItem( hChildMenu
                         , MNU_DIRECT
                         , MF_BYCOMMAND|MF_UNCHECKED );
    }

    _x = x;
    _y = y;
	 _b = b;
    return TRUE;
}
//----------------------------------------------------------------------------

// Image is a temporary subimage to draw into...
void DrawMenuItem( LOGICAL measure, PDRAWPOPUPITEM pdi )
{
    if( measure )
    {
      pdi->measure.width  = 80;
      pdi->measure.height = 15;
    }
    else
    {
        if( pdi->psvUser >= MNU_BLACK && pdi->psvUser <= MNU_WHITE )
            pdi->psvUser -= MNU_BLACK;
        else if( pdi->psvUser >= MNU_BKBLACK && pdi->psvUser <= MNU_BKWHITE )
            pdi->psvUser -= MNU_BKBLACK;
        BlatColor( pdi->draw.image
                  , pdi->draw.x + 2, pdi->draw.y + 1
                    , pdi->draw.width - 4, pdi->draw.height - 2
                    , crColorTable[ pdi->psvUser ] );
    }
}

//----------------------------------------------------------------------------
static int WindowRegistered;
int RegisterWindows( void )
{
   if( WindowRegistered )
        return TRUE;
    //Log( WIDE("Setting up PSI render interface...") );
//	 SetControlInterface( RenderInterface = GetDisplayInterface() );
//	 if( !RenderInterface )
//       return 0;
   //Log( WIDE("Setting up PSI image interface...") );
//	SetControlImageInterface( ImageInterface = GetImageInterface() );
//	if( !ImageInterface )
//      return 0;
   //Log( WIDE("Done with psi interfaces..") );
    hChildMenu = CreatePopup();
   Log( WIDE("Created menu...") );
    AppendPopupItem( hChildMenu, MF_STRING, MNU_FONT, WIDE("Set Font") );
    Log( WIDE("Added an ittem...") );
   {
      hHistoryMenu = CreatePopup();
      AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE25, WIDE("25%") );
      AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE50, WIDE("50%") );
      AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE75, WIDE("75%") );
      AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE100, WIDE("100%") );
      AppendPopupItem( hChildMenu, MF_STRING|MF_POPUP, (int)hHistoryMenu, WIDE("History Display Size") );
   }
   {
      PMENU hColorMenu, hColorMenu2;
      hColorMenu = CreatePopup();
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_BLACK, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_BLUE, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_GREEN, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_CYAN, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_RED, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_MAGENTA, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_DKYEL, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_GREY, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_DKGREY, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTBLUE, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTGREEN, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTCYAN, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTRED, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTMAG, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_YELLOW, DrawMenuItem );
      AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_WHITE, DrawMenuItem );
      AppendPopupItem( hChildMenu, MF_STRING|MF_POPUP, (int)hColorMenu, WIDE("Text Color") );
      hColorMenu2 = CreatePopup();
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKBLACK, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKBLUE, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKGREEN, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKCYAN, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKRED, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKMAGENTA, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKDKYEL, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKGREY,  DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKDKGREY, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTBLUE, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTGREEN, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTCYAN, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTRED, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTMAG, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKYELLOW, DrawMenuItem );
      AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKWHITE, DrawMenuItem );
      AppendPopupItem( hChildMenu, MF_STRING|MF_POPUP, (int)hColorMenu2, WIDE("Background Color") );
   }
   AppendPopupItem( hChildMenu, MF_STRING, MNU_COMMAND_COLOR, WIDE("Command Color") );
   AppendPopupItem( hChildMenu, MF_STRING, MNU_COMMAND_BACK, WIDE("Command Background Color") );
    AppendPopupItem( hChildMenu, MF_STRING, MNU_DIRECT, WIDE("Direct Mode") );
   Log( WIDE("Menus created...") );
   cPenNormal = GetBaseColor( NORMAL );
   cPenHighlight = GetBaseColor( HIGHLIGHT );
   cPenShadow = GetBaseColor( SHADE );
   cPenDkShadow = GetBaseColor( SHADOW );
   cPenCursor = Color( 255, 255, 255 );
   WindowRegistered = TRUE;
   return TRUE;
}
//----------------------------------------------------------------------------
void UnregisterWindows( void )
{
	DestroyPopup( hChildMenu );
// first set the old interfaces to NULL then we can drop the interface.
	SetControlInterface( NULL );
	SetControlImageInterface( NULL );
	DropDisplayInterface(NULL);
	DropImageInterface(NULL);
	WindowRegistered = FALSE;
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
static void CPROC WinconPrompt( PDATAPATH pdp )
{
	//RenderCommandLine( (PCONSOLE_INFO)pdp );
	prompt( pdp->Owner ); // will need to call this since it's bypassed
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
static int CPROC Close( PDATAPATH pPath )
{
	PCONSOLE_INFO pdp = (PCONSOLE_INFO)pPath;
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

	DestroyHistoryRegion( pdp->pHistory );
	DestroyHistoryCursor( pdp->pCursor );
	DestroyHistoryBrowser( pdp->pCurrentDisplay );
   DestroyHistoryBrowser( pdp->pHistoryDisplay );

	DestroyFrame( &pdp->psicon.frame );

	return 1;
}
#endif

//----------------------------------------------------------------------------

static void CPROC FillLineLeader( PCONSOLE_INFO pdp, RECT *r, CDATA c )
{
      BlatColor( pdp->psicon.image, r->left, r->top
               , r->right - r->left, r->bottom - ( r->top )
               , pdp->psicon.crCommandBackground );
}

//----------------------------------------------------------------------------

#include <psi.h>
CONTROL_REGISTRATION ConsoleClass = { WIDE("Dekware PSI Console"), { { -1, -1 }, 0, BORDER_NORMAL|BORDER_RESIZABLE|BORDER_FIXED }
												, NULL
												, NULL
												, RenderChildWindow
												, MouseHandler
												, KeyEventProc
};
int CPROC InitDekwareConsole( PSI_CONTROL pc );

PRELOAD(RegisterConsole)
{
	l.pdi = GetDisplayInterface();
	l.pii = GetImageInterface();
	DoRegisterControl( &ConsoleClass );
	SimpleRegisterMethod( WIDE("psi/control/") WIDE("Dekware PSI Console") WIDE("/rtti/extra init")
							  , InitDekwareConsole, WIDE("int"), WIDE("extra init"), WIDE("(PCOMMON)") );

	crColorTable[0] = Color( 0,0,1 ); 
	crColorTable[1] =Color( 0, 0, 128 ); 
	crColorTable[2] =Color( 0, 128, 0 ); 
	crColorTable[3] =Color( 0, 128, 128 ); 
	crColorTable[4] =Color( 192, 32, 32 ); 
	crColorTable[5] =Color( 140, 0, 140 ); 
	crColorTable[6] =Color( 160, 160, 0 ); 
	crColorTable[7] =Color( 192, 192, 192 ); 
	crColorTable[8] =Color( 128, 128, 128 ); 
	crColorTable[9] =Color( 0, 0, 255 ); 
	crColorTable[10] =Color( 0, 255, 0 ); 
	crColorTable[11] =Color( 0, 255, 255 ); 
	crColorTable[12] =Color( 255, 0, 0 ); 
	crColorTable[13] =Color( 255, 0, 255 ); 
	crColorTable[14] =Color( 255, 255, 0 ); 
	crColorTable[15] =Color( 255, 255, 255 );

}

//----------------------------------------------------------------------------
// methods for window logic routines to use as callbacks...
//----------------------------------------------------------------------------

static void CPROC DrawString( PCONSOLE_INFO pdp, int x, int y, RECT *r, TEXTCHAR *s, size_t nShown, size_t nShow )
{
	//lprintf( WIDE("Adding string out : %p %s %d %d at %d,%d #%08lX #%08lX"), pdp, s, nShown, nShow,x,y,r->left,r->top
	//		 , pdp->psicon.crText, pdp->psicon.crBack );
	PutStringFontEx( pdp->psicon.image, x, y
						, pdp->psicon.crText, pdp->psicon.crBack
						, s + nShown
						, nShow, pdp->psicon.hFont );
}

//----------------------------------------------------------------------------

static void CPROC SetCurrentColor( PCONSOLE_INFO pdp, enum current_color_type type, PTEXT segment )
{
	switch( type )
	{
	case COLOR_COMMAND:
		( pdp->psicon.crText = pdp->psicon.crCommand );
		( pdp->psicon.crBack = pdp->psicon.crCommandBackground );
		break;
	case COLOR_MARK:
		( pdp->psicon.crText = pdp->psicon.crMark );
		( pdp->psicon.crBack = pdp->psicon.crMarkBackground );
		break;
	case COLOR_DEFAULT:
		( pdp->psicon.crText = crColorTable[15] );
		( pdp->psicon.crBack = pdp->psicon.crBackground );
		break;
	case COLOR_SEGMENT:
		if( segment )
		{
			( pdp->psicon.crText = crColorTable[segment->format.flags.foreground] );
			( pdp->psicon.crBack = crColorTable[segment->format.flags.background] );
		}
      break;
	}
	//lprintf( WIDE("Set Color :%p %d #%08lX #%08lX"), pdp, type
	//		 , pdp->psicon.crText, pdp->psicon.crBack );
}

//----------------------------------------------------------------------------

static void CPROC FillConsoleRect( PCONSOLE_INFO pdp, RECT *r, enum fill_color_type type )
{
	switch( type )
	{
	case FILL_COMMAND_BACK:
		BlatColor( pdp->psicon.image
					, r->left,r->top
					, r->right - r->left
					, r->bottom - r->top, pdp->psicon.crBackground );
		break;
	case FILL_DISPLAY_BACK:
		BlatColor( pdp->psicon.image, r->left,r->top
					 ,r->right - r->left
					 ,r->bottom - r->top
					, pdp->psicon.crCommandBackground );
      break;
	}
}

//----------------------------------------------------------------------------

static void CPROC RenderCursor( PCONSOLE_INFO pdp, RECT *r, int column )
{
	if( pdp->
#ifdef __DEKWARE_PLUGIN__
		common.
#endif
		CommandInfo->CollectionInsert )
	{
		int y, x;

		x = ( column ) * pdp->nFontWidth + pdp->nXPad;
		if( pdp->flags.bDirect )
			y = r->top;
		else
			y = r->top + 4;
		do_line( pdp->psicon.image, x-3, y, x, y + 2, cPenCursor );
		do_line( pdp->psicon.image, x, y+2, x+4, y, cPenCursor );

		if( pdp->flags.bDirect )
			y = r->bottom - 2;
		else
			y = r->bottom - 5;
		do_line( pdp->psicon.image, x-3, y, x, y - 2, cPenCursor );
		do_line( pdp->psicon.image, x, y-2, x+4, y, cPenCursor );

	}
	else
	{
		int y, x;
		x = ( column ) * pdp->nFontWidth + pdp->nXPad;
		if( pdp->flags.bDirect )
			y = r->top;
		else
			y = r->top + 4;
		do_line( pdp->psicon.image, x, y+3, x, y, cPenCursor );
		do_line( pdp->psicon.image, x, y, x + pdp->nFontWidth, y, cPenCursor );
		do_line( pdp->psicon.image, x + pdp->nFontWidth, y, x+pdp->nFontWidth, y+3, cPenCursor );

		if( pdp->flags.bDirect )
			y = r->bottom - 2;
		else
			y = r->bottom - 5;
		do_line( pdp->psicon.image, x, y-3, x, y, cPenCursor );
		do_line( pdp->psicon.image, x, y, x + pdp->nFontWidth, y, cPenCursor );
		do_line( pdp->psicon.image, x + pdp->nFontWidth, y, x+pdp->nFontWidth, y-3, cPenCursor );
	}
}

//----------------------------------------------------------------------------

static void CPROC Update( PCONSOLE_INFO pmdp, RECT *upd )
{
	// passed region is the region which was updated by drawing
// code.
	//lprintf( WIDE("update some controls... %d,%d - %d,%d"), upd->left, upd->right, upd->top, upd->bottom );
	upd->right -= upd->left;
   upd->bottom -= upd->top;
    UpdateSomeControls( pmdp->psicon.frame, (IMAGE_RECTANGLE*)upd );

   //SmudgeCommonArea( pmdp->psicon.frame, upd );
   //SmudgeCommon( pmdp->psicon.frame );
}

//----------------------------------------------------------------------------

int CPROC InitDekwareConsole( PSI_CONTROL pc )
{
	if( !RegisterWindows() )
	{
		//Log( WIDE("Register windows failed...") );
		return 0; // cancel load, unload library...
	}
	if( !bCreatingControl )
	{
#ifdef __DEKWARE_PLUGIN__
      PTEXT name = SegCreateFromText( WIDE("Auto Console") );
		PENTITY pe = CreateEntityIn( NULL, name );
		PSENTIENT ps = CreateAwareness( pe );
		PCONSOLE_INFO pdp = (PCONSOLE_INFO)CreateDataPath( &ps->Command, CONSOLE_INFO );
      pdp->common.pName = SegCreateFromText( WIDE("Auto Console") );

		pdp->common.Owner = ps;
		//Log( WIDE("Create frame!!") );
      pdp->psicon.frame = pc;
		SetCommonUserData( pc, (PTRSZVAL)pdp );

		pdp->psicon.image = GetFrameSurface( pdp->psicon.frame );
		GetStringSize( WIDE(" "), &pdp->nFontWidth, &pdp->nFontHeight );
#if 0
		AddVolatileVariable( pe, &vve_rows, (PTRSZVAL)pdp );
		AddVolatileVariable( pe, &vve_cols, (PTRSZVAL)pdp );
		AddVolatileVariable( pe, &vve_cursorx, (PTRSZVAL)pdp );
		AddVolatileVariable( pe, &vve_cursory, (PTRSZVAL)pdp );
#endif
#else
		PCONSOLE_INFO pdp;
		pdp = Allocate( sizeof( *pdp ) );
      MemSet( pdp, 0, sizeof( *pdp ) );
#endif
		InitializeCriticalSec( &pdp->Lock );

   // this is destroyed when the common closes...
		pdp->
#ifdef __DEKWARE_PLUGIN__
			common.
#endif
			CommandInfo =
#ifdef __DEKWARE_PLUGIN__
			CreateCommandHistoryEx( WinconPrompt )
#else
			CreateUserInputBuffer()
#endif
         ;

		//pdp->common.Type = myTypeID;
#ifdef __DEKWARE_PLUGIN__
		pdp->common.Close = Close;
		//   pdp->common.Read = NULL;
		pdp->common.Write = (int (CPROC*)(PDATAPATH))WinLogicWrite;
		//pdp->common.Option = HandleOptions;
		pdp->common.flags.Data_Source = 1;
#endif
		pdp->flags.bDirect = 0;
		//pdp->common.flags.Formatted = TRUE;

	pdp->psicon.crCommand = Color( 32, 192, 192 );
	pdp->psicon.crCommandBackground = Color( 0, 0, 1 );
	pdp->psicon.crBackground = Color( 0, 0, 1 );
	pdp->psicon.crMark = Color( 192, 192, 192 );
	pdp->psicon.crMarkBackground = Color( 67, 116, 150 );

	pdp->pHistory = CreateHistoryRegion();
	pdp->pCursor = CreateHistoryCursor( pdp->pHistory );
	pdp->pCurrentDisplay = CreateHistoryBrowser( pdp->pHistory );
	pdp->pHistoryDisplay = CreateHistoryBrowser( pdp->pHistory );
	SetHistoryBrowserNoPageBreak( pdp->pHistoryDisplay );

	pdp->nXPad = 5;
	pdp->nYPad = 5;
	pdp->nCmdLinePad = 2;

	pdp->FillConsoleRect = FillConsoleRect;
	pdp->RenderSeparator = RenderSeparator;
	pdp->DrawString = DrawString;
	pdp->KeystrokePaste = KeystrokePaste;
	pdp->SetCurrentColor = SetCurrentColor;
	pdp->RenderCursor = RenderCursor;
	pdp->Update = Update;

	//EnterCriticalSec( &pdp->Lock );
	//ChildCalculate( pdp );
	//LeaveCriticalSec( &pdp->Lock );
	//DisplayFrame( pdp->psicon.frame );
#ifdef __DEKWARE_PLUGIN__
	{
      PTEXT data;
      PVARTEXT first = VarTextCreate();
      vtprintf( first, WIDE("/script psicon.startup") );
      data = VarTextGet( first );
      VarTextDestroy( &first );

      //EnqueLink( &PLAYER->Command->Input, burst( SegCreateFromText( WIDE("/debug") ) ) );
		//LineRelease( data );
		//Log1( WIDE("Running : %s"), GetText( data) );
      		EnqueLink( &ps->Command->Input, burst( data ) );
		LineRelease( data );
		UnlockAwareness( ps);
	      	WakeAThread( ps );
	}
#endif
	}
   return 1;
}

DECLTEXT( PSIConName, WIDE("psicon") );

//----------------------------------------------------------------------------

static PTEXT DeviceVolatileVariableGet( WIDE("psicon"), WIDE("rows"), WIDE("console row count") )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&PSIConName );
   return GetRows( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( WIDE("psicon"), WIDE("cols"), WIDE("console col count") )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&PSIConName );
   return GetCols( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( WIDE("psicon"), WIDE("cursor_x"), WIDE("console cursor x(col) position") )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&PSIConName );
   return GetCursorX( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( WIDE("psicon"), WIDE("cursor_y"), WIDE("console cursor y(row) position") )(PENTITY pe,PTEXT*ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&PSIConName );
   return GetCursorY( (PTRSZVAL)pdp, pe, ppLastValue );
}

//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
static PDATAPATH OnInitDevice( WIDE("psicon"), WIDE("Windows MDI interactive interface") )( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
//PDATAPATH CPROC CreateConsole( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
	PCONSOLE_INFO pdp;
	//Log( WIDE("Create console!") );
	// get result from send message.... this will be useful data
	if( !RegisterWindows() )
	{
		//Log( WIDE("Register windows failed...") );
		return NULL; // cancel load, unload library...
	}
	//Log( WIDE("--------------------------------------------") );
	pdp = (PCONSOLE_INFO)CreateDataPath( pChannel, CONSOLE_INFO );
	pdp->common.Owner = ps;
	//Log( WIDE("Create frame!!") );
	pdp->psicon.renderer = OpenDisplaySizedAt( 0, -1, -1, -1, -1 );
	if( !pdp->psicon.renderer )
	{
		Log( WIDE("FAILED WINDOW!") );
		DestroyDataPath( (PDATAPATH)pdp );
		return NULL;
	}

	{
		_32 width, height;
		GetDisplayPosition( pdp->psicon.renderer, NULL, NULL, &width, &height );
      bCreatingControl = 1;
		pdp->psicon.frame = MakeCaptionedControl( NULL, ConsoleClass.TypeID, 0, 0
															 , width, height
															 , 0
															 , GetText( GetName( ps->Current ) )
															 );
      bCreatingControl = 0;
		AttachFrameToRenderer( pdp->psicon.frame, pdp->psicon.renderer );
	}
	if( !pdp->psicon.frame )
	{
		Log( WIDE("FAILED FRAME!") );
		DestroyDataPath((PDATAPATH)pdp);
		return NULL;
	}

   SetCommonUserData( pdp->psicon.frame, (PTRSZVAL)pdp );
	//SetFrameDraw( pdp->psicon.frame, RenderChildWindow );
	//SetFrameKey( pdp->psicon.frame, KeyEventProc, (PTRSZVAL)pdp );
	//SetFrameMouse( pdp->psicon.frame, MouseHandler );
	pdp->psicon.image = GetFrameSurface( pdp->psicon.frame );
	if( !pdp->psicon.image )
	{
		Log( WIDE("Failed to get frame image") );
		DestroyDataPath( (PDATAPATH)pdp );
		return NULL;
	}
	GetStringSize( WIDE(" "), &pdp->nFontWidth, &pdp->nFontHeight );

	if( pChannel == &ps->Command )
	{
		// should be added at some point directly here...
		// didn't know before whether opeing a data/command device and
		// now we do?

		// these could potentially result in multiple variables of the
		// same name on an object (psicon input, psicon output...)
#if 0
		AddVolatileVariable( ps->Current, &vve_rows, (PTRSZVAL)pdp );
		AddVolatileVariable( ps->Current, &vve_cols, (PTRSZVAL)pdp );
		AddVolatileVariable( ps->Current, &vve_cursorx, (PTRSZVAL)pdp );
		AddVolatileVariable( ps->Current, &vve_cursory, (PTRSZVAL)pdp );
#endif
	}

	//ps->flags.no_prompt = TRUE;
	InitializeCriticalSec( &pdp->Lock );

   // this is destroyed when the common closes...
	pdp->common.CommandInfo = CreateCommandHistoryEx( WinconPrompt );
	//pdp->common.Type = myTypeID;
	pdp->common.Close = Close;
	//   pdp->common.Read = NULL;
	pdp->common.Write = (int (CPROC*)(PDATAPATH))WinLogicWrite;
	//pdp->common.Option = HandleOptions;
	pdp->common.flags.Data_Source = 1;
	pdp->flags.bDirect = 0;
	//pdp->common.flags.Formatted = TRUE;

   pdp->psicon.crCommand = Color( 32, 192, 192 );
   pdp->psicon.crCommandBackground = Color( 0, 0, 1 );
	pdp->psicon.crBackground = Color( 0, 0, 1 );
	pdp->psicon.crMark = Color( 192, 192, 192 );
	pdp->psicon.crMarkBackground = Color( 67, 116, 150 );

	pdp->pHistory = CreateHistoryRegion();
	pdp->pCursor = CreateHistoryCursor( pdp->pHistory );
	pdp->pCurrentDisplay = CreateHistoryBrowser( pdp->pHistory );
	pdp->pHistoryDisplay = CreateHistoryBrowser( pdp->pHistory );
	SetHistoryBrowserNoPageBreak( pdp->pHistoryDisplay );

	pdp->nXPad = 5;
	pdp->nYPad = 5;
	pdp->nCmdLinePad = 2;

	pdp->FillConsoleRect = FillConsoleRect;
	pdp->RenderSeparator = RenderSeparator;
	pdp->DrawString = DrawString;
	pdp->KeystrokePaste = KeystrokePaste;
	pdp->SetCurrentColor = SetCurrentColor;
	pdp->RenderCursor = RenderCursor;
   pdp->Update = Update;

	//EnterCriticalSec( &pdp->Lock );
	//ChildCalculate( pdp );
	//LeaveCriticalSec( &pdp->Lock );
	DisplayFrame( pdp->psicon.frame );

	return (PDATAPATH)pdp;
}
#endif

//----------------------------------------------------------------------------
// $Log: psicon.c,v $
// Revision 1.79  2005/08/12 21:41:57  d3x0r
// Fix update region...
//
// Revision 1.78  2005/08/08 15:24:12  d3x0r
// Move updated rectangle struct to common space.  Improved curses console handling....
//
// Revision 1.77  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.76  2005/05/30 11:51:42  d3x0r
// Remove many messages...
//
// Revision 1.75  2005/04/22 18:34:09  d3x0r
// Fix CPROC declaration of keystroke paste handler.
//
// Revision 1.74  2005/04/15 07:28:39  d3x0r
// Okay this all seems to work sufficicnetly - disabled logging entirely.
//
// Revision 1.73  2005/02/22 12:28:49  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.72  2005/01/28 09:53:34  d3x0r
// Okay all forms of graphical interface work on windows platform with appropriate updates
//
// Revision 1.71  2005/01/27 17:31:37  d3x0r
// psicon works now, added some locks to protect multiple accesses to datapath (render/update_write)
//
// Revision 1.70  2005/01/26 20:00:01  d3x0r
// Okay - need to do something about partial updates - such as command typing should only update that affected area of the screen...
//
// Revision 1.69  2005/01/23 10:09:57  d3x0r
// cursecon might even work or resemble working..
//
// Revision 1.68  2005/01/23 04:07:57  d3x0r
// Hmm somehow between display rendering stopped working.
//
// Revision 1.67  2005/01/19 15:49:33  d3x0r
// Begin defining external methods so WinLogic and History can become a core-console library
//
// Revision 1.66  2005/01/17 07:58:23  d3x0r
// Rename write to avoid conflict with other libraries
