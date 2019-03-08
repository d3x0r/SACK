//#define DO_LOGGING
#if defined( WIN32 ) || defined( SACK_BAG_EXPORTS )
#define _INCLUDE_CLIPBOARD
#endif
//#define NO_LOGGING
#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <stdio.h> // sprintf ?
#include <string.h> // strchr
#include <logging.h>


#include "../global.h" 

#include <psi.h>
/*
//#define USE_IMAGE_INTERFACE ImageInterface
//#define USE_RENDER_INTERFACE RenderInterface
#define USE_IMAGE_INTERFACE (g.MyImageInterface?g.MyImageInterface:(g.MyImageInterface=GetImageInterface() ))
#define USE_RENDER_INTERFACE (g.MyDisplayInterface?g.MyDisplayInterface:(g.MyDisplayInterface=GetDisplayInterface() ))

#include <image.h>
//PIMAGE_INTERFACE ImageInterface;
#include <render.h>
//PRENDER_INTERFACE RenderInterface;
#include <controls.h>
#include <keybrd.h>
*/
#include <psi/console.h>
#include "consolestruc.h"
#include "interface.h"
#include "WinLogic.h"

//#define PutStringEx(i,x,y,f,b,s,l) { Log6( "Putting string: %ld,%ld %ld %*.*s", x,y,l,l,l,s); PutStringFontEx( i,x,y,f,b,s,l,NULL); }
//#define BlatColor(i,x,y,w,h,c)	  { Log5( "BlatColor: %ld,%ld %ld,%ld %08lx", x, y, w, h, c ); BlatColor( i,x,y,w,h,c ); }

PSI_CONSOLE_NAMESPACE


#define text_alpha 255
#define back_alpha 255

static CDATA crColorTableText[16];
#if 0
= { AColor( 0,0,1, text_alpha ), AColor( 0, 0, 128, text_alpha ), AColor( 0, 128, 0, text_alpha )
									 , AColor( 0, 128, 128, text_alpha ), AColor( 192, 32, 32, text_alpha ), AColor( 140, 0, 140, text_alpha )
									 , AColor( 160, 160, 0, text_alpha ), AColor( 192, 192, 192, text_alpha )
									 , AColor( 128, 128, 128, text_alpha ), AColor( 0, 0, 255, text_alpha ), AColor( 0, 255, 0, text_alpha )
									 , AColor( 0, 255, 255, text_alpha ), AColor( 255, 0, 0, text_alpha ), AColor( 255, 0, 255, text_alpha )
									 , AColor( 255, 255, 0, text_alpha ), AColor( 255, 255, 255, text_alpha ) };
#endif
static CDATA crColorTableBack[16];
#if 0
] = { AColor( 0,0,1, back_alpha ), AColor( 0, 0, 128, back_alpha ), AColor( 0, 128, 0, back_alpha )
									 , AColor( 0, 128, 128, back_alpha ), AColor( 192, 32, 32, back_alpha ), AColor( 140, 0, 140, back_alpha )
									 , AColor( 160, 160, 0, back_alpha ), AColor( 192, 192, 192, back_alpha )
									 , AColor( 128, 128, 128, back_alpha ), AColor( 0, 0, 255, back_alpha ), AColor( 0, 255, 0, back_alpha )
									 , AColor( 0, 255, 255, back_alpha ), AColor( 255, 0, 0, back_alpha ), AColor( 255, 0, 255, back_alpha )
									 , AColor( 255, 255, 0, back_alpha ), AColor( 255, 255, 255, back_alpha ) };
#endif



#define MNU_FONT 100
#define MNU_HISTORYSIZE25 101
#define MNU_HISTORYSIZE50 102
#define MNU_HISTORYSIZE75 103
#define MNU_HISTORYSIZE100 104
#define MNU_DIRECT			105
#define MNU_COMMAND_COLOR 106
#define MNU_COMMAND_BACK  107

#define MNU_BLACK	 115
#define MNU_BLUE	  116
#define MNU_GREEN	 117
#define MNU_CYAN	  118
#define MNU_RED		119
#define MNU_MAGENTA  120
#define MNU_DKYEL	 121
#define MNU_GREY	  122
#define MNU_DKGREY	123
#define MNU_LTBLUE	124
#define MNU_LTGREEN  125
#define MNU_LTCYAN	126
#define MNU_LTRED	 127
#define MNU_LTMAG	 128
#define MNU_YELLOW	129
#define MNU_WHITE	 130

#define MNU_BKBLACK	 ( 115 + 16 )
#define MNU_BKBLUE	  ( 116 + 16 )
#define MNU_BKGREEN	 ( 117 + 16 )
#define MNU_BKCYAN	  ( 118 + 16 )
#define MNU_BKRED		( 119 + 16 )
#define MNU_BKMAGENTA  ( 120 + 16 )
#define MNU_BKDKYEL	 ( 121 + 16 )
#define MNU_BKGREY	  ( 122 + 16 )
#define MNU_BKDKGREY	( 123 + 16 )
#define MNU_BKLTBLUE	( 124 + 16 )
#define MNU_BKLTGREEN  ( 125 + 16 )
#define MNU_BKLTCYAN	( 126 + 16 )
#define MNU_BKLTRED	 ( 127 + 16 )
#define MNU_BKLTMAG	 ( 128 + 16 )
#define MNU_BKYELLOW	( 129 + 16 )
#define MNU_BKWHITE	 ( 130 + 16 )

//----------------------------------------------------------------------------
// only one master copy of this is really needed...

CDATA cPenNormal, cPenHighlight, cPenShadow, cPenDkShadow, cPenCursor;
PMENU hChildMenu;
PMENU hHistoryMenu;
//cpg27dec2006 console\psicon.c(96): Warning! W202: Symbol 'bCreatingControl' has been defined, but not referenced
//cpg27dec2006 static int bCreatingControl; // set when this is creating a control as opposed to someone externally creating one

//----------------------------------------------------------------------------

static int CPROC InitPSIConsole( PSI_CONTROL pc );
//static int CPROC RenderChildWindow( PSI_CONTROL pc );
static int CPROC MouseHandler( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b );
static int CPROC KeyEventProc( PSI_CONTROL pc, uint32_t key );

CONTROL_REGISTRATION ConsoleClass = { WIDE("PSI Console"), { { 640, 480 }, sizeof( CONSOLE_INFO ), BORDER_NORMAL|BORDER_RESIZABLE|BORDER_FIXED }
												, InitPSIConsole
												, NULL
												, NULL//RenderChildWindow
												, MouseHandler
												, KeyEventProc
};
//----------------------------------------------------------------------------

int CPROC RenderSeparator( PCONSOLE_INFO console, int nStart )
{
	//lprintf( WIDE("Render separator %d-%d %d"), 0, console->nWidth, nStart );
	if( nStart > 0 && (int64_t)nStart < console->nHeight )
	{
		// Render Command Line Separator
		do_hline( console->psicon.image, nStart, 0, console->nWidth, cPenHighlight );
		do_hline( console->psicon.image, nStart+1, 0, console->nWidth, cPenNormal );
		do_hline( console->psicon.image, nStart+2, 0, console->nWidth, cPenShadow );
		do_hline( console->psicon.image, nStart+3, 0, console->nWidth, cPenDkShadow );
		//SmudgeCommon( console->psicon.image );
	}
	return 4;
}

//----------------------------------------------------------------------------

void CPROC PSI_Console_KeystrokePaste( PCONSOLE_INFO console )
{
#if defined( WIN32 ) && !defined( __NO_WIN32API__ )
	if( OpenClipboard(NULL) )
	{
		uint32_t format;
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
				PSI_WinLogicDoStroke( console, pStroke );
				//EnqueLink( console->ps->Command->ppOutput, SegDuplicate(pStroke) );
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
		  DECLTEXT( msg, WIDE( "Clipboard was not available" ) );
		  EnqueLink( &console->common.Output, (PTEXT)&msg );
#endif
	 }
#endif
	 return;

}

//----------------------------------------------------------------------------

static int OnDrawCommon( WIDE("PSI Console") )( PSI_CONTROL pc )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	//PCONSOLE_INFO console = (PCONSOLE_INFO)GetCommonUserData(pc);
	//lprintf( WIDE("Rendering window.") );
	if( !console )
	{
		Log( WIDE("How could we have gotten here without a console??") );
		return 0;
	}
	console->psicon.image = GetFrameSurface( pc );
	//ClearImage( console->psicon.image );
	BlatColor( console->psicon.image, 0, 0, console->psicon.image->width, console->psicon.image->height, AColor( 5, 5, 5, 64 ) );

	// this is the only place which size is changed.
	if( console->nWidth != console->psicon.image->width ||
		console->nHeight != console->psicon.image->height )
	{
		// nWidth/nHeight are set in child calculate
		// and acknowledge processing a new rarea rect.
		console->rArea.left = 0;
		console->rArea.right = console->psicon.image->width;
		console->rArea.top = 0;
		console->rArea.bottom = console->psicon.image->height;
		//lprintf( WIDE("Updating child propportions...") );
		
		PSI_ConsoleCalculate( console, GetCommonFont( pc ) ); // this includes doing a render.
	}
	else
		PSI_RenderConsole( console, GetCommonFont( pc ) );
	//lprintf( WIDE( "Done rendering child." ) );
	return TRUE;
}

//----------------------------------------------------------------------------



//----------------------------------------------------------------------------

int CPROC KeyEventProc( PSI_CONTROL pc, uint32_t key )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	//PCONSOLE_INFO console = (PCONSOLE_INFO)GetCommonUserData( pc );
	// this must here gather keystrokes and pass them forward into the
	// opened sentience...
	if( console )
	{
		const TEXTCHAR *character = GetKeyText( key );
		DECLTEXT( stroke, WIDE("                                       ") ); // single character ...
		if( !console ) // not a valid window handle/device path
			return 0;
		EnterCriticalSec( &console->Lock );
		console->lockCount++;

		// here is where we evaluate the curent keystroke....
		if( character )
		{
			int n;
			for( n = 0; character[n]; n++ )
				stroke.data.data[n] = character[n];
			stroke.data.size = n;
		}
		else
			stroke.data.size = 0;

		if( key & KEY_PRESSED )
		{
			PSI_KeyPressHandler( console, (uint8_t)(KEY_CODE(key)&0xFF), (uint8_t)KEY_MOD(key), (PTEXT)&stroke, GetCommonFont( pc ) );
			//SmudgeCommon( console->psicon.frame );
		}
		console->lockCount--;
		LeaveCriticalSec( &console->Lock );
	}
	return 1;
}

//----------------------------------------------------------------------------

int CPROC MouseHandler( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	static int32_t _x, _y;
	static uint32_t _b;
	//lprintf( WIDE("Mouse thing...%ld,%ld %08lx"), x, y, b );
	{
		int xPos, yPos, row, col;
		//PCONSOLE_INFO console;
		if( (b & MK_LBUTTON) && !(_b & MK_LBUTTON) )
		  { // mouse down.
			  ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
			  //console = (PCONSOLE_INFO)GetCommonUserData( pc );
			  if( !console )
				  return 0;
			  xPos = x; 
			  yPos = y;
			  if( PSI_ConvertXYToLineCol( console, xPos, yPos
											, &row, &col ) )
			  {
				  //lprintf( "converted is %d,%d", row, col );
				  console->mark_start.row = row;
				  console->mark_start.col = col;
				  console->flags.bUpdatingEnd = 1;
				  console->flags.bMarking = 1;
				  console->CurrentMarkInfo = console->CurrentLineInfo;
			  }
		  }
		else if( ( !(b & MK_LBUTTON) && (_b & MK_LBUTTON) )
				  ||( b & MK_LBUTTON ) )
		{ // mouse up
			ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
			//console = (PCONSOLE_INFO)GetCommonUserData( pc );
			if( !console )
				return 0;
			xPos = x;
			yPos = y;
			if( PSI_ConvertXYToLineCol( console, xPos, yPos
										 , &row, &col ) )
			{
				//lprintf( "converted is %d,%d", row, col );
				if( console->CurrentMarkInfo == console->CurrentLineInfo )
				{
					if( console->flags.bUpdatingEnd )
					{
						console->mark_end.row = row;
						console->mark_end.col = col;
						if( console->mark_end.row > console->mark_start.row )
						{
							int tmp = console->mark_end.row;
							console->mark_end.row = console->mark_start.row;
							console->mark_start.row = tmp;
							tmp = console->mark_end.col;
							console->mark_end.col = console->mark_start.col;
							console->mark_start.col = tmp;
							console->flags.bUpdatingEnd = 0;
						}
						else if( console->mark_end.row == console->mark_start.row
								  && ( console->mark_end.col < console->mark_start.col ) )
						{
							int tmp = console->mark_end.col;
							console->mark_end.col = console->mark_start.col;
							console->mark_start.col = tmp;
							console->flags.bUpdatingEnd = 0;
						}
					}
					else
					{
						console->mark_start.col = col;
						console->mark_start.row = row;
						if( console->mark_end.row > console->mark_start.row )
						{
							int tmp = console->mark_end.row;
							console->mark_end.row = console->mark_start.row;
							console->mark_start.row = tmp;
							tmp = console->mark_end.col;
							console->mark_end.col = console->mark_start.col;
							console->mark_start.col = tmp;
							console->flags.bUpdatingEnd = 1;
						}
						else if( console->mark_end.row == console->mark_start.row
								  && console->mark_end.col < console->mark_start.col )
						{
							int tmp = console->mark_end.col;
							console->mark_end.col = console->mark_start.col;
							console->mark_start.col = tmp;
							console->flags.bUpdatingEnd = 1;
						}
					}
					if( !(b & MK_LBUTTON) && (_b & MK_LBUTTON) )
					{
						if( console->mark_start.row != console->mark_end.row
							|| console->mark_start.col != console->mark_end.col )
						{
#ifndef __NO_WIN32API__
#ifdef _WIN32
							TEXTCHAR *data = PSI_GetDataFromBlock( console );
							if( data && OpenClipboard(NULL) )
							{
								size_t nLen = StrLen( data ) + 1;
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
#endif
						}
						//Log( WIDE( "Ending mark." ) );
						console->flags.bMarking = 0;
						console->flags.bUpdatingEnd = 0;
					}
					SmudgeCommon( console->psicon.frame );
				}
			}
			if( !(b & MK_LBUTTON) && (_b & MK_LBUTTON) )
			{
				//Log( WIDE( "Ending mark(2)." ) );
				console->flags.bMarking = 0;
				console->flags.bUpdatingEnd = 0;
				SmudgeCommon( console->psicon.frame );
			}
		}
	}

	if( (b & MK_RBUTTON) && !(_b & MK_RBUTTON ) )
	{
		ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
		//PCONSOLE_INFO console = (PCONSOLE_INFO)GetCommonUserData( pc );
		int cmd;
		if( !console )
			return 0;
		CheckPopupItem( hHistoryMenu
						  , MNU_HISTORYSIZE25+console->nHistoryPercent
						  , MF_BYCOMMAND|MF_CHECKED	 );
		if( console->flags.bDirect )
			CheckPopupItem( hChildMenu
							  , MNU_DIRECT
							  , MF_BYCOMMAND|MF_CHECKED );
		cmd = TrackPopup( hChildMenu, console->psicon.frame );
		if( ( cmd >= MNU_BKBLACK ) &&
			( cmd <= MNU_BKWHITE ) )
		{
			PSI_SetHistoryDefaultBackground( console->pCursor, cmd - MNU_BKBLACK );
			SACK_WriteProfileInt( "sack/PSI/console", "background", cmd - MNU_BKBLACK );
		}
		else if( ( cmd >= MNU_BLACK ) &&
				  ( cmd <= MNU_WHITE ) )
		{
			PSI_SetHistoryDefaultForeground( console->pCursor, cmd - MNU_BLACK );
			SACK_WriteProfileInt( "sack/PSI/console", "foreground", cmd - MNU_BLACK );
		}
		else switch( cmd )
		{
		case MNU_DIRECT:
			{
				console->flags.bDirect ^= 1;
				SACK_WriteProfileInt( "sack/PSI/console", "direct", console->flags.bDirect );

				EnterCriticalSec( &console->Lock );
				console->lockCount++;
				PSI_ConsoleCalculate( console, GetCommonFont( pc ) );
				console->lockCount--;
				LeaveCriticalSec( &console->Lock );
			}
			break;
		case MNU_HISTORYSIZE25:
		case MNU_HISTORYSIZE50:
		case MNU_HISTORYSIZE75:
		case MNU_HISTORYSIZE100:
			{
				console->nHistoryPercent =  cmd - MNU_HISTORYSIZE25;
				SACK_WriteProfileInt( "sack/PSI/console", "direct", console->nHistoryPercent );
				if( console->flags.bHistoryShow ) // currently showing history
				{
					EnterCriticalSec( &console->Lock );
					console->lockCount++;
					PSI_ConsoleCalculate( console, GetCommonFont( pc ) ); // changed history display...
					console->lockCount--;
					LeaveCriticalSec( &console->Lock );
				}
			}
			break;
		case MNU_FONT:
			{
				//console->cfFont.hwndOwner = hWnd;
				size_t size;
				SFTFont font;
				POINTER info = NULL;
				if( font = PickFont( -1, -1, &size, &info, NULL ) )
				{
					//POINTER data;
					//size_t datalen;
					//GetFontRenderData( font, &data, &datalen );
					SACK_WriteProfileString( "sack/PSI/console", "font", (CTEXTSTR)info );
					//console->psicon.hFont = (SFTFont)font;
					SetCommonFont( console->psicon.frame, (SFTFont)font );
					//GetDefaultFont();
					//GetStringSizeFont( WIDE(" "), &console->nFontWidth, &console->nFontHeight, (SFTFont)font );
					PSI_ConsoleCalculate( console, GetCommonFont( pc ) );
				}
			}
				break;
		  case MNU_COMMAND_COLOR:
				{
				CDATA color;
					 if( PickColor( &color, console->psicon.crCommand, console->psicon.frame ) )
						  console->psicon.crCommand = color;
					 else
						  Log2( WIDE("Colors %08x %08x"), color, console->psicon.crCommand );
				}
			break;
		  case MNU_COMMAND_BACK:
				{
				CDATA color;
					 if( PickColor( &color, console->psicon.crCommandBackground, console->psicon.frame ) )
						  console->psicon.crCommandBackground = color;
					 else
						  Log2( WIDE("Colors %08x %08x"), color, console->psicon.crCommandBackground );
				}
			break;
		}
		  CheckPopupItem( hHistoryMenu
								 , MNU_HISTORYSIZE25+console->nHistoryPercent
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
						  , crColorTableText[ pdi->psvUser ] );
	 }
}

//----------------------------------------------------------------------------
static int WindowRegistered;
int RegisterWindows( void )
{
	if( WindowRegistered )
		return TRUE;
	//Log( "Done with psi interfaces.." );
	hChildMenu = CreatePopup();
	//Log( WIDE( "Created menu..." ) );
	AppendPopupItem( hChildMenu, MF_STRING, MNU_FONT, WIDE( "Set Font" ) );
	//Log( WIDE( "Added an ittem..." ) );
	{
		hHistoryMenu = CreatePopup();
		AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE25, WIDE( "25%" ) );
		AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE50, WIDE( "50%" ) );
		AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE75, WIDE( "75%" ) );
		AppendPopupItem( hHistoryMenu, MF_STRING, MNU_HISTORYSIZE100, WIDE( "100%" ) );
		AppendPopupItem( hChildMenu, MF_STRING|MF_POPUP, (uintptr_t)hHistoryMenu, WIDE( "History Display Size" ) );
	}
	{
		PMENU hColorMenu, hColorMenu2;
		hColorMenu = CreatePopup();
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_BLACK, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_BLUE, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_GREEN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_CYAN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_RED, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_MAGENTA, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_DKYEL, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_GREY, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_DKGREY, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTBLUE, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTGREEN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTCYAN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTRED, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_LTMAG, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_YELLOW, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu, MF_OWNERDRAW, MNU_WHITE, (POINTER)DrawMenuItem );
		AppendPopupItem( hChildMenu, MF_STRING|MF_POPUP, (uintptr_t)hColorMenu, WIDE( "Text Color" ) );
		hColorMenu2 = CreatePopup();
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKBLACK, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKBLUE, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKGREEN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKCYAN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKRED, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKMAGENTA, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKDKYEL, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKGREY,  (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKDKGREY, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTBLUE, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTGREEN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTCYAN, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTRED, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKLTMAG, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKYELLOW, (POINTER)DrawMenuItem );
		AppendPopupItem( hColorMenu2, MF_OWNERDRAW, MNU_BKWHITE, (POINTER)DrawMenuItem );
		AppendPopupItem( hChildMenu, MF_STRING|MF_POPUP, (uintptr_t)hColorMenu2, WIDE( "Background Color" ) );
	}
	AppendPopupItem( hChildMenu, MF_STRING, MNU_COMMAND_COLOR, WIDE( "Command Color" ) );
	AppendPopupItem( hChildMenu, MF_STRING, MNU_COMMAND_BACK, WIDE( "Command Background Color" ) );
	AppendPopupItem( hChildMenu, MF_STRING, MNU_DIRECT, WIDE( "Direct Mode" ) );
	//Log( WIDE( "Menus created..." ) );
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
	//SetControlImageInterface( NULL );
	DropDisplayInterface( NULL );
	DropImageInterface( NULL );
	WindowRegistered = FALSE;
}

//----------------------------------------------------------------------------
#ifdef __DEKWARE_PLUGIN__
static void CPROC WinconPrompt( PDATAPATH console )
{
	//RenderCommandLine( (PCONSOLE_INFO)console );
	prompt( console->Owner ); // will need to call this since it's bypassed
}
#endif
//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
static int CPROC Close( PDATAPATH pPath )
{
	PCONSOLE_INFO console = (PCONSOLE_INFO)pPath;
	RemoveVolatileVariable( console->common.Owner->Current, &vve_rows );
	RemoveVolatileVariable( console->common.Owner->Current, &vve_cols );
	RemoveVolatileVariable( console->common.Owner->Current, &vve_cursorx );
	RemoveVolatileVariable( console->common.Owner->Current, &vve_cursory );
	console->common.Close = NULL;
	console->common.Write = NULL;
	console->common.Read = NULL;
	console->common.Type = 0;

	PSI_DestroyHistoryRegion( console->pHistory );
	PSI_DestroyHistoryCursor( console->pCursor );
	PSI_DestroyHistoryBrowser( console->pCurrentDisplay );
	PSI_DestroyHistoryBrowser( console->pHistoryDisplay );
	PSI_DestroyHistoryBrowser( console->pCommandDisplay );

	DestroyFrame( &console->psicon.frame );

	return 1;
}
#endif

static void FillDefaultColors( void )
{
	if( !crColorTableText[0] )
	{
		crColorTableText[0] = AColor( 0,0,1, text_alpha );
		crColorTableText[1] = AColor( 0, 0, 128, text_alpha );
		crColorTableText[2] = AColor( 0, 128, 0, text_alpha );
		crColorTableText[3] = AColor( 0, 128, 128, text_alpha );
		crColorTableText[4] = AColor( 192, 32, 32, text_alpha );
		crColorTableText[5] = AColor( 140, 0, 140, text_alpha );
		crColorTableText[6] = AColor( 160, 160, 0, text_alpha );
		crColorTableText[7] = AColor( 192, 192, 192, text_alpha );
		crColorTableText[8] = AColor( 128, 128, 128, text_alpha );
		crColorTableText[9] = AColor( 0, 0, 255, text_alpha );
		crColorTableText[10] = AColor( 0, 255, 0, text_alpha );
		crColorTableText[11] = AColor( 0, 255, 255, text_alpha );
		crColorTableText[12] = AColor( 255, 0, 0, text_alpha );
		crColorTableText[13] = AColor( 255, 0, 255, text_alpha );
		crColorTableText[14] = AColor( 255, 255, 0, text_alpha );
		crColorTableText[15] = AColor( 255, 255, 255, text_alpha );

		crColorTableBack[0] = AColor( 0,0,1, back_alpha );
		crColorTableBack[1] = AColor( 0, 0, 128, back_alpha );
		crColorTableBack[2] = AColor( 0, 128, 0, back_alpha );
		crColorTableBack[3] = AColor( 0, 128, 128, back_alpha );
		crColorTableBack[4] = AColor( 192, 32, 32, back_alpha );
		crColorTableBack[5] = AColor( 140, 0, 140, back_alpha );
		crColorTableBack[6] = AColor( 160, 160, 0, back_alpha );
		crColorTableBack[7] = AColor( 192, 192, 192, back_alpha );
		crColorTableBack[8] = AColor( 128, 128, 128, back_alpha );
		crColorTableBack[9] = AColor( 0, 0, 255, back_alpha );
		crColorTableBack[10] = AColor( 0, 255, 0, back_alpha );
		crColorTableBack[11] = AColor( 0, 255, 255, back_alpha );
		crColorTableBack[12] = AColor( 255, 0, 0, back_alpha ) ;
		crColorTableBack[13] = AColor( 255, 0, 255, back_alpha );
		crColorTableBack[14] = AColor( 255, 255, 0, back_alpha );
		crColorTableBack[15] = AColor( 255, 255, 255, back_alpha );
	}
}

//----------------------------------------------------------------------------

PRELOAD(RegisterConsole)
{
	//ImageInterface = GetImageInterface();
	//RenderInterface = GetDisplayInterface();
	DoRegisterControl( &ConsoleClass );
	//SimpleRegisterMethod( WIDE( "psi/control/" ) WIDE( "Dekware PSI Console" ) WIDE( "/rtti/extra init" )
	//						  , InitDekwareConsole, WIDE( "int" ), WIDE( "extra init" ), WIDE( "(PSI_CONTROL)" ) );

}


void CPROC PSIMeasureString( uintptr_t psvConsole, CTEXTSTR s, int nShow, uint32_t *w, uint32_t *h, SFTFont font )
{
	PCONSOLE_INFO console = (PCONSOLE_INFO)psvConsole;
	GetStringSizeFontEx( s, nShow, w, h, font?font:GetCommonFont( console->psicon.frame ) );
}

//----------------------------------------------------------------------------
// methods for window logic routines to use as callbacks...
//----------------------------------------------------------------------------

static void CPROC DrawString( PCONSOLE_INFO console, int x, int y, RECT *r, CTEXTSTR s, int nShown, int nShow )
{
	//uint32_t width;
	//lprintf( WIDE( "Adding string out : %p %s start:%d len:%d at %d,%d #%08lX #%08lX" ), console, s, nShown, nShow,x,y,r->left,r->top
	//		 , console->psicon.crText, console->psicon.crBack );

	if(0)  // strings are measured way before drawstring happens now...
	{
		uint32_t w, h;
		GetStringSizeFontEx( s + nShown, nShow, &w, &h, GetCommonFont( console->psicon.frame ) );
		r->right = r->left + w;
		r->bottom = r->top + h;
	}
#ifdef DEBUG_OUTPUT
	lprintf( WIDE("Output string  xy(%d,%d)   lr(%d-%d)  tb(%d-%d) %*.*s  %08x %08x")
		, x, y
		, (*r).left, (*r).right, (*r).top, (*r).bottom
		, nShow, nShow, s + nShown, console->psicon.crText, console->psicon.crBack );
#endif 
	PutStringFontEx( console->psicon.image, x, y
						, console->psicon.crText, console->psicon.crBack
						, s + nShown
						, nShow
						, GetCommonFont( console->psicon.frame ) );
}

//----------------------------------------------------------------------------

static void CPROC SetCurrentColor( PCONSOLE_INFO console, enum current_color_type type, PTEXT segment )
{
	switch( type )
	{
	case COLOR_COMMAND:
		( console->psicon.crText = console->psicon.crCommand );
		( console->psicon.crBack = console->psicon.crCommandBackground );
		break;
	case COLOR_MARK:
		( console->psicon.crText = console->psicon.crMark );
		( console->psicon.crBack = console->psicon.crMarkBackground );
		break;
	case COLOR_DEFAULT:
		( console->psicon.crText = crColorTableText[15] );
		( console->psicon.crBack = console->psicon.crBackground );
		break;
	case COLOR_SEGMENT:
		if( segment )
		{
			( console->psicon.crText = crColorTableText[segment->format.flags.foreground] );
			( console->psicon.crBack = crColorTableBack[segment->format.flags.background] );
		}
		break;
	}
	//lprintf( WIDE( "Set Color :%p %d #%08lX #%08lX" ), console, type
	//		 , console->psicon.crText, console->psicon.crBack );
}

//----------------------------------------------------------------------------

static void CPROC FillConsoleRect( PCONSOLE_INFO console, RECT *r, enum fill_color_type type )
{
	switch( type )
	{
	case FILL_COMMAND_BACK:
		BlatColorAlpha( console->psicon.image
					, r->left,r->top
					, r->right - r->left
					, r->bottom - r->top, console->psicon.crCommandBackground );
		break;
	case FILL_DISPLAY_BACK:
		BlatColorAlpha( console->psicon.image, r->left,r->top
					 ,r->right - r->left
					 ,r->bottom - r->top
					, console->psicon.crBack );
		break;
	}
}

//----------------------------------------------------------------------------

static void CPROC RenderCursor( PCONSOLE_INFO console, RECT *r, int column )
{
	if( console->
#ifdef __DEKWARE
		common.
#endif
		CommandInfo->CollectionInsert )
	{
		int y, x;

		x = ( column ) /* * console->nFontWidth */ + console->nXPad;
		if( console->flags.bDirect )
			y = r->top;
		else
			y = r->top + 4;
		do_lineAlpha( console->psicon.image, x-3, y, x, y + 2, cPenCursor );
		do_lineAlpha( console->psicon.image, x, y+2, x+4, y, cPenCursor );

		if( console->flags.bDirect )
			y = r->bottom - 2;
		else
			y = r->bottom - 5;
		do_lineAlpha( console->psicon.image, x-3, y, x, y - 2, cPenCursor );
		do_lineAlpha( console->psicon.image, x, y-2, x+4, y, cPenCursor );

	}
	else
	{
		uint32_t w, h;
		int y, x;
		x = ( column ) /*  * console->nFontWidth */ + console->nXPad;
		if( console->flags.bDirect )
			y = r->top;
		else
			y = r->top + 4;
		if( console->CommandInfo->CollectionIndex == GetTextSize( console->CommandInfo->CollectionBuffer) )
			GetStringSizeFontEx( " ", 1, &w, &h, GetCommonFont( console->psicon.frame ) );
		else if( console->CommandInfo->CollectionBuffer )
			GetStringSizeFontEx( GetText( console->CommandInfo->CollectionBuffer ) + console->CommandInfo->CollectionIndex, 1
			                   , &w, &h, GetCommonFont( console->psicon.frame ) );
		else {
			w = 0; h = 0;
		}

		do_lineAlpha( console->psicon.image, x, y+3, x, y, cPenCursor );
		do_lineAlpha( console->psicon.image, x, y, x + w, y, cPenCursor );
		do_lineAlpha( console->psicon.image, x + w, y, x+w, y+3, cPenCursor );

		if( console->flags.bDirect )
			y = r->bottom - 2;
		else
			y = r->bottom - 5;
		do_lineAlpha( console->psicon.image, x, y-3, x, y, cPenCursor );
		do_lineAlpha( console->psicon.image, x, y, x + w, y, cPenCursor );
		do_lineAlpha( console->psicon.image, x + w, y, x+w, y-3, cPenCursor );
	}
}

//----------------------------------------------------------------------------

static void CPROC ConsoleUpdate( PCONSOLE_INFO pmdp, RECT *upd )
{
	// passed region is the region which was updated by drawing
	// code.
	//lprintf( WIDE( "------------------------------------" ) );
	//lprintf( WIDE("update some controls... %d,%d - %d,%d"), upd->left, upd->right, upd->top, upd->bottom );
	upd->right -= upd->left;
	upd->bottom -= upd->top;
	// this causes the parent to update? shoudl be smart and recall the parent's
	// saved original picture here...

	UpdateSomeControls( pmdp->psicon.frame, (IMAGE_RECTANGLE*)upd );
	//lprintf( WIDE("------------------------------------") );
}

//----------------------------------------------------------------------------

static void OnControlFontChanged( "PSI Console" )(PSI_CONTROL pc) {
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	SFTFont font = GetCommonFont( pc );
	console->nFontHeight = GetFontHeight( GetCommonFont( pc ) );
	// it is possible that the new font has never rendered any characters
	if( !console->nFontHeight ) {
		console->nFontHeight = GetStringSizeFont( " ", NULL, NULL, GetCommonFont( pc ) );
	}
	PSI_ConsoleCalculate( console, font );
}

int CPROC InitPSIConsole( PSI_CONTROL pc )
{
	ValidatedControlData( PCONSOLE_INFO, ConsoleClass.TypeID, console, pc );
	if( !RegisterWindows() )
	{
		//Log( WIDE("Register windows failed...") );
		return FALSE; // cancel load, unload library...
	}
	FillDefaultColors();

	SetCommonTransparent( pc, TRUE );

	if( console )
	{
		//console->common.pName = SegCreateFromText( WIDE("Auto Console") );
		//Log( WIDE("Create frame!!") );
		console->psicon.frame = pc;

		InitializeCriticalSec( &console->Lock );

		// this is destroyed when the common closes...
		console->CommandInfo = CreateUserInputBuffer();

		//console->common.Type = myTypeID;
		console->flags.bDirect = 1;
		console->flags.bWrapCommand = 1;
		console->flags.bNoLocalEcho = 1;
		//console->common.flags.Formatted = TRUE;

		console->flags.bDirect = SACK_GetProfileInt( "SACK/PSI/console", "direct", 0 );
		console->nHistoryPercent = SACK_GetProfileInt( "SACK/PSI/console", "history", 1 );

		{
			char fontdata[256];
			SACK_GetProfileString( "sack/PSI/console", "font", "", fontdata, 256 );
			if( fontdata[0] ) {
				FRACTION one = { 1,1 };
				SetControlFont( pc, RenderScaledFontData( (PFONTDATA)fontdata, &one, &one ) );
			}
		}

		console->psicon.crCommand = AColor( 32, 192, 192, text_alpha );
		console->psicon.crCommandBackground = AColor( 0, 0, 1, back_alpha );
		console->psicon.crBackground = AColor( 0, 0, 1, back_alpha );
		console->psicon.crMark = AColor( 192, 192, 192, text_alpha );
		console->psicon.crMarkBackground = AColor( 67, 116, 150, back_alpha );

		console->pHistory = PSI_CreateHistoryRegion();   // this is the history backing buffer; the set of lines that make up the raw history.
		console->pCursor = PSI_CreateHistoryCursor( console->pHistory ); // cursor is where output into the history region happens.
		console->pCommandHistory = PSI_CreateHistoryRegion();   // this is the history backing buffer; the set of lines that make up the raw history.

		console->pCurrentDisplay = PSI_CreateHistoryBrowser( console->pHistory, PSIMeasureString, (uintptr_t)console );
		console->pHistoryDisplay = PSI_CreateHistoryBrowser( console->pHistory, PSIMeasureString, (uintptr_t)console );
		console->pCommandDisplay = PSI_CreateHistoryBrowser( console->pCommandHistory, PSIMeasureString, (uintptr_t)console );

		PSI_SetHistoryBrowserNoPageBreak( console->pHistoryDisplay );

		console->nXPad = 5;
		console->nYPad = 5;
		console->nCmdLinePad = 2;

		if( !(console->nFontHeight = GetFontHeight( GetCommonFont( pc ) ) ) ) {
			console->nFontHeight = GetStringSizeFont( " ", NULL, NULL, GetCommonFont( pc ) );
		}
		console->nSeparatorHeight = RenderSeparator( console, 0 );
		console->FillConsoleRect = FillConsoleRect;
		console->RenderSeparator = RenderSeparator;
		console->DrawString = DrawString;
		console->KeystrokePaste = PSI_Console_KeystrokePaste;
		console->SetCurrentColor = SetCurrentColor;
		console->RenderCursor = RenderCursor;
		console->Update = ConsoleUpdate;

		return 1;
	}
	return 0;
}
PSI_CONSOLE_NAMESPACE_END
//----------------------------------------------------------------------------
