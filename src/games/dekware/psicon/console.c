#include <stdhdrs.h>
#ifdef __LCC__
//#include <wincon.h>
#endif

#include <timers.h>
#ifdef __DEKWARE_PLUGIN__
#define PLUGIN_MODULE
#include "plugin.h"
#endif

#include "regaccess.h"
PTHREAD ghInputThread;

#include "consolestruc.h"
#include "WinLogic.h"

void Lock( PCONSOLE_INFO pmdp )
{
	EnterCriticalSec( &pmdp->Lock );
}

void Unlock( PCONSOLE_INFO pmdp )
{
   LeaveCriticalSec( &pmdp->Lock );
}

#include "keydefs.h"
// only one master copy of this is really needed...

// NOTE: ONLY one(1) console window ever
static int nActiveConsoles;

extern int myTypeID;
#ifdef _WIN32
//__declspec(dllimport) int b95;
#endif

void CPROC RenderSeparator( PCONSOLE_INFO pdp, int nStart )
{
	int c;
   _32 dwSize;
#define SetCurrentColor(f,b) do { /*lprintf( "setting attrbute %d:%d", f, b );*/ SetConsoleTextAttribute( pdp->consolecon.hStdout, f|((b)<<4) ); }while(0)
#define move(y,x) do { COORD pos; /*lprintf( "setting position: %d,%d", x, y );*/ pos.X = x; pos.Y = y; SetConsoleCursorPosition( pdp->consolecon.hStdout, pos ); }while(0)
	if( nStart == -1 )
      return;
	//  asm("int $3\n");
   //lprintf( "Rendering some separator at %d", nStart );
   SetCurrentColor( 15, 0 );
    move( nStart-1, 0 );
    WriteConsole( pdp->consolecon.hStdout, ">", 1, &dwSize, NULL );
    for( c = 0; c < pdp->nWidth-2; c++ )
		 WriteConsole( pdp->consolecon.hStdout, "-", 1, &dwSize, NULL );
	 WriteConsole( pdp->consolecon.hStdout, "<", 1, &dwSize, NULL );
#undef SetCurrentColor
   return;
}

//typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
// COORD dwSize;
// COORD dwCursorPosition;
// WORD wAttributes;
// SMALL_RECT srWindow;
// COORD dwMaximumWindowSize;
//} CONSOLE_SCREEN_BUFFER_INFO,*PCONSOLE_SCREEN_BUFFER_INFO;


//----------------------------------------------------------------------------

PTEXT win_get_line(HANDLE hFile)
{
//   extern HANDLE hStdin;
   #define WORKSPACE 1024  // character for workspace
   PTEXT workline=(PTEXT)NULL;
   _32 length = 0;
   do
   {
      // create a workspace to read input from the file.
      workline=SegAppend(workline,SegCreate(WORKSPACE));
      SetEnd( workline );
      // read a line of input from the file.
      if( !ReadConsole( hFile
                           , GetText(workline)
                           , WORKSPACE
                           , &length
                           , NULL) ) // if no input read.
      {
         if (PRIORLINE(workline)) // if we've read some.
         {
            PTEXT t;
            workline=PRIORLINE(workline); // go back one.
            SegBreak(t = NEXTLINE(workline));
            LineRelease(t);  // destroy the current segment.
         }
         else
         {
            LineRelease(workline);            // destory only segment.
            workline = NULL;
         }
         break;  // get out of the loop- there is no more to read.
      }
   }
   while (GetText(workline)[length-1]!='\n'); //while not at the end of the line.
   if (workline&&length)  // if I got a line, and there was some length to it.
      SetStart(workline);   // set workline to the beginning.
   return(workline);      // return the line read from the file.
}

void UnregisterWindows( void )
{
	// not sure - probably just FreeConsole();

}

static int CPROC Close( PCONSOLE_INFO pdp )
{
   EndThread( pdp->consolecon.pThread );
   CloseHandle( pdp->consolecon.hStdout );
   CloseHandle( pdp->consolecon.hStdin );
   FreeConsole();

   pdp->consolecon.hStdout = NULL;
	pdp->consolecon.hStdin = NULL;
#ifdef __DEKWARE_PLUGIN__
   pdp->common.Close = NULL;
   pdp->common.Write = NULL;
   pdp->common.Read = NULL;
	pdp->common.Type = 0;
#endif

   return 1;
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------

static int KeyDelete( PCONSOLE_INFO pdp )
{
	// eh copy code for generat 0x7f delete char...
	return FALSE;
}
//----------------------------------------------------------------------------

unsigned char KeyboardState[256];

VOID KeyEventProc(PCONSOLE_INFO pdp, KEY_EVENT_RECORD event)
{
#ifdef USE_OLD_CODE
   // this must here gather keystrokes and pass them forward into the
   // opened sentience...
//   PTEXT temp;
	int bOutput = FALSE;
   lprintf( "Entering handle a key event..." );
	Lock( pdp );
   lprintf( "Got the lock, waiting for collection buffer..." );
   if( event.bKeyDown )
   {
      PTEXT key;
      // here is where we evaluate the curent keystroke....

      for( ; event.wRepeatCount; event.wRepeatCount-- )
      {
         int mod = KEYMOD_NORMAL;
         extern PSIKEYDEFINE KeyDefs[];
         if(event.dwControlKeyState& (RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED))
            mod |= KEYMOD_CTRL;
         if(event.dwControlKeyState& (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED))
            mod |= KEYMOD_ALT;
         if( KeyDefs[event.wVirtualKeyCode].flags & KDF_CAPSKEY )
         {
            if( event.dwControlKeyState & CAPSLOCK_ON )
            {
               if( !(event.dwControlKeyState& SHIFT_PRESSED) )
                  mod |= KEYMOD_SHIFT;
            }
            else
            {
               if( event.dwControlKeyState& SHIFT_PRESSED )
                  mod |= KEYMOD_SHIFT;
            }
         }
         else
            if( event.dwControlKeyState& SHIFT_PRESSED )
               mod |= KEYMOD_SHIFT;
         if( pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bStroke ||
             pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bMacro )
         {
            if( pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bStroke )
            {
               bOutput |= DoStroke(pdp, pdp->Keyboard[event.wVirtualKeyCode][mod].data.stroke);
            }
            else if( pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bMacro )
            {
               if( pdp->common.Owner->pRecord != pdp->Keyboard[event.wVirtualKeyCode][mod].data.macro )
                  InvokeMacro( pdp->common.Owner
                             , pdp->Keyboard[event.wVirtualKeyCode][mod].data.macro
                             , NULL );
               // do macro!!!!!
            }
         }
         else
         {
				if( key = KeyDefs[event.wVirtualKeyCode].op[mod].data.pStroke )
				{
					if( KeyDefs[event.wVirtualKeyCode].op[mod].bFunction )
						bOutput |= ((KeyFunc)key)(pdp);
					else
					{
						bOutput |= DoStroke( pdp, key );
					}
				}
         }
      }
      // call to clear and re-write the current buffer....
      //if( bOutput ) // so we don't output on shift, control, etc...
         //RenderCommandLine( pdp );
   }
   else
   {
      // key up's don't matter like ever...
   }
	Unlock( pdp );
#else
   // this must here gather keystrokes and pass them forward into the
   // opened sentience...
//   PTEXT temp;
   int bOutput = FALSE;
   int mod = KEYMOD_NORMAL;
   if( !pdp ) // not a valid window handle/device path
        return;
   //Log( "Entering keyproc..." );
   EnterCriticalSec( &pdp->Lock );
   //while( LockedExchange( &pdp->common.CommandInfo->CollectionBufferLock, 1 ) )
   //   Sleep(0);
   Log1( "mod = %x", pdp->dwControlKeyState );
   mod = pdp->dwControlKeyState;

   if( KeyDefs[event.wVirtualKeyCode].flags & KDF_CAPSKEY )
   {
      if( event.dwControlKeyState & CAPSLOCK_ON )
      {
         mod ^= KEYMOD_SHIFT;
      }
   }

   if( event.bKeyDown )
   {
      for( ; event.wRepeatCount; event.wRepeatCount-- )
		{
			unsigned char KeyState[256];
			DECLTEXT( key, "                   " );
			GetKeyboardState( KeyState );
			SetTextSize( &key, ToAscii( event.wVirtualKeyCode
											  , event.wVirtualScanCode
											  , KeyState
											  , (unsigned short*)key.data.data
											  , 0 ) );
			KeyPressHandler( pdp, event.wVirtualKeyCode, mod, (PTEXT)&key );
		}
#if 0
      // here is where we evaluate the curent keystroke....
      for( ; event.wRepeatCount; event.wRepeatCount-- )
		{

         // check current keyboard override...
         if( pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bStroke ||
             pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bMacro )
         {
            if( pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bStroke )
            {
               bOutput |= DoStroke(pdp, pdp->Keyboard[event.wVirtualKeyCode][mod].data.stroke);
            }
            else if( pdp->Keyboard[event.wVirtualKeyCode][mod].flags.bMacro )
            {
               if( pdp->common.Owner->pRecord != pdp->Keyboard[event.wVirtualKeyCode][mod].data.macro )
                  InvokeMacro( pdp->common.Owner
                             , pdp->Keyboard[event.wVirtualKeyCode][mod].data.macro
                             , NULL );
            }
         }
         else // key was not overridden
         {
            int result;
            Log1( "Keyfunc = %d", KeyDefs[event.wVirtualKeyCode].op[mod].bFunction );
            switch( KeyDefs[event.wVirtualKeyCode].op[mod].bFunction )
            {
            case KEYDATA_DEFINED:
               Log( "Key data_defined" );
               bOutput |= DoStroke( pdp, (PTEXT)&KeyDefs[event.wVirtualKeyCode].op[mod].data.pStroke );
                    result = UPDATE_NOTHING; // unsure about this - recently added.
                    // well it would appear that the stroke results in whether to update
                    // the command prompt or not.
                break;
            case KEYDATA:
               {
                  char KeyState[256];
                  DECLTEXT( key, "                   " );
                  GetKeyboardState( KeyState );
                  SetTextSize( &key, ToAscii( event.wVirtualKeyCode
                                            , event.wVirtualScanCode
                                            , KeyState
                                            , (void*)key.data.data
                                            , 0 ) );
                  if( GetTextSize( (PTEXT)&key ) )
                     bOutput |= DoStroke( pdp, (PTEXT)&key );
                  result = UPDATE_NOTHING; // already taken care of?!
               }
               break;
            case COMMANDKEY:
               result = KeyDefs[event.wVirtualKeyCode].op[mod].data.CommandKey( pdp->common.CommandInfo );
               break;
            case HISTORYKEY:
               result = KeyDefs[event.wVirtualKeyCode].op[mod].data.HistoryKey( pdp->pHistoryDisplay );
               break;
            case CONTROLKEY:
               KeyDefs[event.wVirtualKeyCode].op[mod].data.ControlKey( &pdp->dwControlKeyState, TRUE );
               result = UPDATE_NOTHING;
               break;
            case SPECIALKEY:
               result = KeyDefs[event.wVirtualKeyCode].op[mod].data.SpecialKey( pdp );
               break;
            }
            switch( result )
            {
            case UPDATE_COMMAND:
               bOutput = TRUE;
               break;
				case UPDATE_HISTORY:
					if( UpdateHistory( pdp ) )
						DoRenderHistory(pdp, TRUE);
						DoRenderHistory(pdp, FALSE);

					break;
				case UPDATE_DISPLAY:
					ChildCalculate( pdp );
					break;
            }
         }
      }
      //if( bOutput )
		//   RenderCommandLine( pdp );
#endif
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
   //lprintf( "Left critical section on wincon." );
#endif
}

int gbExitThread;
HANDLE hInput;

static PTRSZVAL CPROC ConsoleInputThread( PTHREAD thread )
{
	PCONSOLE_INFO pdp = (PCONSOLE_INFO)GetThreadParam( thread );
   INPUT_RECORD irInBuf[128];
   //PTEXT pCommand;
   DWORD cNumRead, c;
   hInput = pdp->consolecon.hStdin;
      while( !gbExitThread )
      {
         if (! ReadConsoleInput(
                pdp->consolecon.hStdin,      // input buffer handle
                irInBuf,     // buffer to read into
                128,         // size of read buffer
                &cNumRead) ) // number of records read
         {
            MessageBox( NULL, "Bad things happened on console input...", "ERROR", MB_OK );
         }
         for( c = 0; c < cNumRead; c++ )
         {
            switch(irInBuf[c].EventType)
            {
                case KEY_EVENT: // keyboard input
                 //  KeyboardState[irInBuf[c].Event.KeyEvent.wVirtualKeyCode] =
                 //                      irInBuf[c].Event.KeyEvent.bKeyDown;
                   KeyEventProc(pdp, irInBuf[c].Event.KeyEvent);
                   break;
                   /*
                case WINDOW_BUFFER_SIZE_EVENT:
                   irInBuf[c].Event.WindowBufferSizeEvent.dwSize.Y = 50;
                   SetConsoleScreenBufferSize( consolecon.hStdout, irInBuf[c].Event.WindowBufferSizeEvent.dwSize );
                   DebugBreak();
                   break;
                   */
                default:
                   //DebugBreak();
                   break;
            }
         }
      }
   Close( pdp );
   ghInputThread = NULL;
	ExitThread( 0xE0F );
   return 0;
}

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// methods for window logic routines to use as callbacks...
//----------------------------------------------------------------------------

static void CPROC DrawString( PCONSOLE_INFO pdp, int x, int y, RECT *r, char *s, int nShown, int nShow )
{
	_32 dwSize;
	lprintf( "Adding string out : %s %d %d at %d,%d", s, nShown, nShow,x,y,r->left,r->top );
   move( y-1, x );
	WriteConsole( pdp->consolecon.hStdout, s + nShown, nShow, &dwSize, NULL );
}

//----------------------------------------------------------------------------

static void CPROC SetCurrentColor( PCONSOLE_INFO pdp, enum current_color_type type, PTEXT segment )
{
	switch( type )
	{
	case COLOR_COMMAND:
		SetConsoleTextAttribute( pdp->consolecon.hStdout, 11|((0)<<4) );
		break;
	case COLOR_MARK:
		SetConsoleTextAttribute( pdp->consolecon.hStdout, 15|((1)<<4) );
		break;
	case COLOR_DEFAULT:
		SetConsoleTextAttribute( pdp->consolecon.hStdout, 7|((0)<<4) );
		break;
	case COLOR_SEGMENT:
		if( segment )
		{
			SetConsoleTextAttribute( pdp->consolecon.hStdout, segment->format.flags.foreground|((segment->format.flags.background)<<4) );
		}
      break;
	}
}

//----------------------------------------------------------------------------

static void CPROC FillConsoleRect( PCONSOLE_INFO pdp, RECT *r, enum fill_color_type type )
{
	int c; _32 dwSize;
	move( r->top-1, r->left );
	lprintf( "Filling blank line: at %d,%d  %d,%d = %d", r->top, r->left, r->left, r->right, r->right-r->left);
	switch( type )
	{
	case FILL_COMMAND_BACK:
      SetCurrentColor( pdp, COLOR_COMMAND, NULL );
      //FillRect( pdp->wincon.hDC, r, pdp->wincon.hbrBackground );
		break;
	case FILL_DISPLAY_BACK:
      SetCurrentColor( pdp, COLOR_DEFAULT, NULL );
      //FillRect( pdp->wincon.hDC, r, pdp->wincon.hbrCommandBackground );
      break;
	}
	for( c = r->left; c < r->right; c++ )
		WriteConsole( pdp->consolecon.hStdout, " ", 1, &dwSize, NULL );
}

//----------------------------------------------------------------------------

static void CPROC Update( PCONSOLE_INFO pmdp, RECT *r )
{
	// probably for console mode all the characters
   // will already be in all the correct places...
	//DoRenderHistory( pmdp, FALSE );
}

static void CPROC RenderCursor( PCONSOLE_INFO pdp, RECT *r, int column )
{
   if( !column )
		move( r->top-1, 1 );
   else
		move( r->top-1, column );

}
//----------------------------------------------------------------------------


__declspec(dllimport) int IsConsole;
DECLTEXT( ConsoleName, "wincon" );

//----------------------------------------------------------------------------

static PTEXT DeviceVolatileVariableGet( "console", "rows", "console row count" )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&ConsoleName );
   return GetRows( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( "console", "cols", "console col count" )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&ConsoleName );
   return GetCols( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( "console", "cursor_x", "console cursor x(col) position" )(PENTITY pe,PTEXT *ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&ConsoleName );
   return GetCursorX( (PTRSZVAL)pdp, pe, ppLastValue );
}
static PTEXT DeviceVolatileVariableGet( "console", "cursor_y", "console cursor y(row) position" )(PENTITY pe,PTEXT*ppLastValue)
{
	PSENTIENT ps = pe->pControlledBy;
	PDATAPATH pdp = FindOpenDevice( ps, (PTEXT)&ConsoleName );
   return GetCursorY( (PTRSZVAL)pdp, pe, ppLastValue );
}

//----------------------------------------------------------------------------

#ifdef __DEKWARE_PLUGIN__
static PDATAPATH OnInitDevice( "console", "Windows MDI interactive interface" )( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
//PDATAPATH CPROC CreateConsole( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
//PDATAPATH Open( PDATAPATH *pChannel, PSENTIENT ps, PTEXT parameters )
{
   // no parameters needed for this....
   PCONSOLE_INFO pdp;

   DWORD fdwMode, fdwSaveOldMode;

//   if( pdp->consolecon.hStdin || pdp->consolecon.hStdout ) // console device is open here...
//      return NULL;
   if( nActiveConsoles )
   {
      MessageBox( NULL, "Console is already active", "Open Console", MB_OK );
      return NULL;
	}
   lprintf( "Opening console interface..." );
   if( IsConsole || AllocConsole() )
   {

      nActiveConsoles++;
		pdp = CreateDataPath( pChannel, CONSOLE_INFO );
		pdp->common.Owner = ps; // we need this early...
      pdp->consolecon.hStdin = GetStdHandle( STD_INPUT_HANDLE );
      pdp->consolecon.hStdout = GetStdHandle( STD_OUTPUT_HANDLE );
		{
         CONSOLE_SCREEN_BUFFER_INFO csbi;
			COORD buf;
			GetConsoleScreenBufferInfo( pdp->consolecon.hStdout, &csbi );
			csbi.dwSize.X = csbi.srWindow.Right - csbi.srWindow.Left;
			csbi.dwSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top;
         //SetConsoleScreenBufferInfo( pdp->consolecon.hStdout, &csbi );
   	   buf = GetLargestConsoleWindowSize( pdp->consolecon.hStdout );
      	//buf.X = 80;
	      //buf.Y = 37;
   	   SetConsoleScreenBufferSize( pdp->consolecon.hStdout, csbi.dwSize );
		}
      GetLastError();
      SetConsoleCtrlHandler( NULL, TRUE );
      LineRelease( parameters );
      //pdp->nLines = pdp->LastCommandPos.dwSize.Y - 1;
      {
         //if( b95 )
         //{
            if (! GetConsoleMode(pdp->consolecon.hStdin, &fdwSaveOldMode) )
            {
               DebugBreak();
            }
            fdwMode = ENABLE_WINDOW_INPUT;
            if (! SetConsoleMode(pdp->consolecon.hStdin, fdwMode) )
            {
               DebugBreak();
				}

            fdwMode = 0;
            if (! SetConsoleMode(pdp->consolecon.hStdout, fdwMode) )
            {
               DebugBreak();
            }
         //}
      }
      //pdp->common.Type = myTypeID;
      pdp->common.Close = (int (CPROC *)( struct datapath_tag *pdp ))Close;
      pdp->common.Read = NULL;
      pdp->common.Write = (int(CPROC*)(PDATAPATH))WinLogicWrite;
		pdp->common.flags.Data_Source = 1;
	   //pdp->common.flags.Formatted = TRUE;

		pdp->common.CommandInfo = CreateCommandHistoryEx( NULL );

		{
			// fix the buffer to the currently visible window, thereby
			// removing the scrollbar...
			CONSOLE_SCREEN_BUFFER_INFO csbi;
			GetConsoleScreenBufferInfo( pdp->consolecon.hStdout, &csbi );
			csbi.dwSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
			csbi.dwSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
			if( !SetConsoleScreenBufferSize( pdp->consolecon.hStdout, csbi.dwSize ) )
				lprintf( "Failed to set buf size: %d", GetLastError() );
			pdp->rArea.left = 0;
			pdp->rArea.right = csbi.dwSize.X;
			pdp->rArea.top = 0;
			pdp->rArea.bottom = csbi.dwSize.Y;
		}

		pdp->nXPad = 0;
		pdp->nYPad = 0;
		pdp->nCmdLinePad = 0;

		pdp->nFontHeight = 1;
		pdp->nFontWidth = 1;

		pdp->consolecon.crCommand = 7;
		pdp->consolecon.crCommandBackground = 0;
		pdp->consolecon.crMark = 15;
		pdp->consolecon.crMarkBackground = 1;

		pdp->FillConsoleRect = FillConsoleRect;
		pdp->RenderSeparator = RenderSeparator;
		pdp->DrawString = DrawString;
		//pdp->KeystrokePaste = KeystrokePaste;
		pdp->SetCurrentColor = SetCurrentColor;
      // should probably use render cursor to update cursor position.
		pdp->RenderCursor = RenderCursor;
		pdp->Update = Update;

		{
			int nValue;
			if( GetRegistryInt( "Dekware\\Console\\Direct"
									, GetText( GetName( pdp->common.Owner->Current ) )
									, &nValue ) )
				pdp->flags.bDirect = nValue;
			if( GetRegistryInt( "Dekware\\Console\\History"
									, GetText( GetName( pdp->common.Owner->Current ) )
									, &nValue ) )
				pdp->nHistoryPercent = nValue;
			else
            pdp->nHistoryPercent = 1;
		}


	pdp->pHistory = CreateHistoryRegion();
	pdp->pCursor = CreateHistoryCursor( pdp->pHistory );
	pdp->pCurrentDisplay = CreateHistoryBrowser( pdp->pHistory );
	pdp->pHistoryDisplay = CreateHistoryBrowser( pdp->pHistory );
	SetHistoryBrowserNoPageBreak( pdp->pHistoryDisplay );
		//InitHistory( &pdp->History, NULL );
		//pdp->History.pdp = pdp;
#if 0
    AddVolatileVariable( ps->Current, &vve_rows, (PTRSZVAL)pdp );
    AddVolatileVariable( ps->Current, &vve_cols, (PTRSZVAL)pdp );
    AddVolatileVariable( ps->Current, &vve_cursorx, (PTRSZVAL)pdp );
	 AddVolatileVariable( ps->Current, &vve_cursory, (PTRSZVAL)pdp );
#endif
	 ChildCalculate( pdp );


      lprintf( "Starting input thread..." );
      ghInputThread = 
			pdp->consolecon.pThread = ThreadTo( ConsoleInputThread, (PTRSZVAL)pdp );
      lprintf( "And we're done." );
      return (PDATAPATH)pdp;
   }
   return NULL;
}
#endif

//-------------------------------------------------------------------
// $Log: console.c,v $
// Revision 1.23  2005/06/10 10:31:59  d3x0r
// Fix setcolor usage so background is actually set.  Fix loading option table... most of the options are DLLIMPORT which is a void(**f)(...)
//
// Revision 1.22  2005/02/22 12:28:49  d3x0r
// Final bit of sweeping changes to use CPROC instead of undefined proc call type
//
// Revision 1.21  2005/01/27 17:31:37  d3x0r
// psicon works now, added some locks to protect multiple accesses to datapath (render/update_write)
//
// Revision 1.20  2005/01/26 20:00:01  d3x0r
// Okay - need to do something about partial updates - such as command typing should only update that affected area of the screen...
//
// Revision 1.19  2005/01/20 20:04:10  d3x0r
// Wincon nearly functions, consolecon ported to new coreocn lib, psicon is next.
//
// Revision 1.18  2005/01/17 11:21:46  d3x0r
// partial working status for psicon
//
// Revision 1.17  2004/09/29 09:31:32  d3x0r
//  Added support to make page breaks on output.  Fixed up some minor issues with scrollback distances
//
// Revision 1.16  2004/09/27 16:06:16  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.15  2004/09/24 08:05:43  d3x0r
// okay console console seems to work pretty well, psicon also, even wincon all mark and play nice...
//
// Revision 1.14  2004/09/23 09:53:33  d3x0r
// Looks like it's in a usable state for windows... perhaps an alpha/beta to be released soon.
//
// Revision 1.13  2004/09/10 08:48:49  d3x0r
// progress not perfection... a grand dawning on the horizon though.
//
// Revision 1.12  2004/09/10 00:28:21  d3x0r
// okay looks like there's some windows console issues... better check curses also...
//
// Revision 1.11  2004/08/13 09:29:50  d3x0r
// checkpoint
//
// Revision 1.10  2004/07/26 08:40:40  d3x0r
// checkpoint...
//
// Revision 1.9  2004/06/14 09:07:13  d3x0r
// Mods to reduce function depth
//
// Revision 1.8  2004/06/12 08:42:34  d3x0r
// Well it initializes, first character causes failure... all windows targets build...
//
// Revision 1.7  2004/04/06 01:50:31  d3x0r
// Update to standardize device options and the processing thereof.
//
// Revision 1.6  2004/04/05 05:43:04  d3x0r
// Fix command line drawing, untested in console modes, works in gui modes
//
// Revision 1.5  2004/03/24 19:24:55  d3x0r
// Progress on fixing console con...
//
// Revision 1.4  2004/03/08 09:25:42  d3x0r
// Fix history underflow and minor drawing/mouse issues
//
// Revision 1.3  2004/01/21 08:36:24  d3x0r
// Added renderseparator to console, supply fix for cursecon
//
// Revision 1.2  2004/01/21 06:45:23  d3x0r
// Compiles okay - windows.  Test point
//
// Revision 1.1  2004/01/20 08:19:15  d3x0r
// Finally merging console into common interface....
//
// Revision 1.17  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.16  2003/10/27 17:34:36  panther
// Port to common ThreadTo api
//
// Revision 1.15  2003/10/26 12:38:16  panther
// Updates for newest scheduler
//
// Revision 1.14  2003/02/03 08:20:13  panther
// Modified to support new WakeThreadID timer library call
//
// Revision 1.13  2003/01/27 14:37:25  panther
// Updated projects from old to new, included missing projects
//
// Revision 1.12  2003/01/22 11:09:50  panther
// Cleaned up warnings issued by Visual Studio
//
// Revision 1.11  2003/01/19 20:15:23  panther
// Use wakeable sleep, and therefore wake objects
//
// Revision 1.10  2002/09/16 01:13:56  panther
// Removed the ppInput/ppOutput things - handle device closes on
// plugin unload.  Attempting to make sure that we can restore to a
// clean state on exit.... this lets us track lost memory...
//
// Revision 1.9  2002/07/29 01:38:32  panther
// Fixed console to use new filter layers - does no processing other than
// gathering a single line for commands.
//
//
