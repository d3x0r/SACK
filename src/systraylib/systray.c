#ifndef NO_OPEN_MACRO
#  define UNDEF_NO_OPEN_MACRO
#  define NO_OPEN_MACRO
#endif
#define FIX_RELEASE_COM_COLLISION
#include <stdhdrs.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <sharemem.h>
#include <logging.h>
#include <controls.h>
#include <idle.h>
//#ifndef __cplusplus_cli
#ifdef WIN32
#include	<windowsx.h>
#endif
#include <sqlgetoption.h>
//#endif

#include <systray.h>
#include <timers.h>
#include "../systraylib/resource.h"
//----------------------------------------------------------------------

#define MNU_EXIT 1000

#ifdef WIN32
HWND ghWndIcon;
#define WM_USERICONMSG (WM_USER + 212)
static HMENU hMainMenu;
HICON hLastIcon;
static NOTIFYICONDATA nid;

static HINSTANCE hInstMe;
static UINT WM_TASKBARCREATED;

CTEXTSTR icon;
LOGICAL thread_ready;

#else

#define __NO_WIN32API__
static PMENU hMainMenu;

#endif

static void (*DblClkCallback)(void);
typedef struct addition {
	CTEXTSTR text;
	void (CPROC*f2)(uintptr_t);
	uintptr_t param;
	void (CPROC*f)(void);
   int id;
} ADDITION, *PADDITION;
static int additions = 0;
static PLIST Functions;


static struct systray_local {
	struct {
		BIT_FIELD bLog : 1;
	} flags;
} localSystrayState;

//----------------------------------------------------------------------

#ifndef __NO_WIN32API__ 

LRESULT APIENTRY IconMessageHandler( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{

	if( uMsg == WM_TASKBARCREATED )
	{
		Shell_NotifyIcon( NIM_ADD, &nid );
	}
	else switch( uMsg )
	{
	case WM_MOUSEMOVE:
		//printf( "Mouse: %d, %d", HIWORD( wParam ), LOWORD( wParam ) );
		break;
	case WM_USERICONMSG:
		//lprintf( "anythign %08x", lParam );
		switch( lParam & 0xFF )
		{
		case 6: // double right.
			break;
		case 4:  // right button down
			//printf( "RightDown" );
			break;
		case 5: // right button up
			{
				POINT p;
				GetCursorPos( &p );
#ifdef WIN32
				TrackPopupMenu( hMainMenu
								  , TPM_LEFTALIGN //| TPM_LEFTBUTTON|TPM_RIGHTBUTTON
								  , p.x, p.y
								  , 0
								  , hWnd
								  , NULL );
#else
				if( TrackPopup( hMainMenu, NULL ) == MNU_EXIT )
					SendMessage( hWnd, WM_COMMAND, MNU_EXIT, 0 );
#endif
			}
			//printf( "RightUp" );
			break;
		case 3: // double left
			if( DblClkCallback )
				DblClkCallback();
			break;
		case 2: // left button down
			//printf( "LeftUp" );
			break;
		case 1: // left button up
			//printf( "LeftDown" );
			break;
		default:
			//Log3( "Mouse: %d, %d %08x", HIWORD( wParam ), LOWORD( wParam ), lParam );
			break;
		}
		break;
	case WM_COMMAND:
//IJ		switch( LOWORD( wParam )) 
		switch( GET_WM_COMMAND_ID (wParam, lParam) ) 
		{
		case MNU_EXIT:
			if( localSystrayState.flags.bLog )
				lprintf( "Posting quit Message" );
			UnregisterIcon();
			PostQuitMessage( 0 );
			exit(0);
		case 0xFFFF:
			break;
		default:
			{
				int fidx = LOWORD(wParam) - (MNU_EXIT + 1);
				PADDITION addon = ((PADDITION)GetLink( &Functions, fidx ));
				if( addon ) {
					void (CPROC * func)(void) = addon->f;
					if( func )
				               func();
					else if( addon->f2 )
						addon->f2( addon->param );
				}
			}
		}
		break;
	case WM_TIMER:
		break;
	case WM_CREATE:
		break;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

//----------------------------------------------------------------------

static PTHREAD pMyThread;
static int CPROC systrayidle( uintptr_t unused )
{
	MSG msg;
	if( IsThisThread( pMyThread ) )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			if( localSystrayState.flags.bLog )
				lprintf( "dispatch %d ...", msg.message );
			DispatchMessage( &msg );
			return 1;
		}
		return 0;
	}
	return -1;
}

#endif

//----------------------------------------------------------------------
uintptr_t CPROC RegisterAndCreate( PTHREAD thread )
{
#ifndef __NO_WIN32API__ 

	static WNDCLASS wc;  // zero init.
	static ATOM ac;
#ifndef __NO_OPTIONS__
	localSystrayState.flags.bLog = SACK_GetProfileIntEx( GetProgramName(), "SACK/System Tray/Logging Enable", 0, TRUE );
#endif
	if( !ac )
	{
		WM_TASKBARCREATED = RegisterWindowMessage("TaskbarCreated");
		memset( &wc, 0, sizeof( WNDCLASS ) );
   		wc.lpfnWndProc   = (WNDPROC)IconMessageHandler;
		wc.hInstance     = GetModuleHandle( NULL ) ;
		wc.lpszClassName = "AlertAgentIcon";
		if( !( ac = RegisterClass(&wc) ) )
		{
			TEXTCHAR byBuf[256];
			if( GetLastError() != ERROR_CLASS_ALREADY_EXISTS )
			{
				tnprintf( byBuf, sizeof( byBuf ), "RegisterClassError: %p %d", GetModuleHandle( NULL ), GetLastError() );
				MessageBox( NULL, byBuf, "BAD", MB_OK );
				return FALSE;   // stop thread
			}
		}
		pMyThread = MakeThread();
		AddIdleProc( systrayidle, 0 );
	}
	{
		TEXTCHAR wndname[256];
		//if( (uintptr_t)icon < 0x10000)
		//	snprintf( wndname, sizeof( wndname ), "AlertAgentIcon:%d", icon );
		//else
		tnprintf( wndname, sizeof( wndname ), "AlertAgentIcon:%s", GetProgramName() );
		/*
		{
			HWND prior = NULL;
			HWND test = NULL;
			while( (test = FindWindow( wc.lpszClassName, wndname )) && test != prior )
			{
				//lprintf( "Sending paint to prior window?" );
				SendMessage( test, WM_PAINT, 0, 0 );
				prior = test;
			}
			}
		*/
		ghWndIcon = CreateWindow( MAKEINTATOM(ac),
										 wndname,
										 0,0,0,0,0,NULL,NULL,NULL,NULL);
	}
	if( !ghWndIcon )
	{
		MessageBox( NULL, "System Tray icon cannot load (no window)", "Exiting now", MB_OK );
		return FALSE;
	}

	if( thread )
	{
		MSG msg;
		thread_ready = TRUE;
		//icon = NULL;
		while( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
		return 0;
	}
	else
#endif
		return TRUE;
}

//----------------------------------------------------------------------

int RegisterIconHandler( CTEXTSTR param_icon )
{
	//if( param_icon )
	//{
	//   icon = param_icon;
	//	return RegisterAndCreate( NULL );
	//}
	//else
#ifdef WIN32
	{
		// start as a thread... which creates the window and registers the class..
		// and continues to receive messages... since the thread parameter is NOT NULL
		// when a threadproc is invoked by ThreadTo();
		if( !param_icon )
			icon = "default";
		else
			icon = param_icon;
		ThreadTo( RegisterAndCreate, 0 );
		// have to wait for a completion event....
		// so we know when we can actually result and allow
		// the registration of the icon itself with a valid window handle
		// with a valid message loop.
		while( !thread_ready )
		{
			// blah..
			Relinquish();
		}
		return TRUE;
	}
#endif
	return 0;
}

//----------------------------------------------------------------------

void SetIconDoubleClick( void (*DoubleClick)(void ) )
{
	DblClkCallback = DoubleClick;
}

//----------------------------------------------------------------------

void BasicExitMenu( void )
{
	CTEXTSTR filepath;
	filepath = GetProgramName();
#ifdef WIN32
	hInstMe = GetModuleHandle( TARGETNAME );
	hMainMenu = CreatePopupMenu();
	AppendMenu( hMainMenu, MF_STRING, TXT_STATIC, filepath );
	AppendMenu( hMainMenu, MF_STRING, MNU_EXIT, "&Exit" );
#else
	hMainMenu = CreatePopup();
	AppendPopupItem( hMainMenu, MF_STRING, TXT_STATIC, filepath );
	AppendPopupItem( hMainMenu, MF_STRING, MNU_EXIT, "&Exit" );
#endif
	{
		INDEX idx;
		PADDITION addition;
		LIST_FORALL( Functions, idx, PADDITION, addition )
		{
#ifdef WIN32
			AppendMenu( hMainMenu, MF_STRING, MNU_EXIT+1+addition->id, addition->text );
#else
			AppendPopupItem( hMainMenu, MF_STRING, MNU_EXIT+1+addition->id, addition->text );
#endif
		}
	}
}


void AddSystrayMenuFunction( CTEXTSTR text, void (CPROC*function)(void) )
{
	if( hMainMenu )
	{
#ifdef WIN32
		AppendMenu( hMainMenu, MF_STRING, MNU_EXIT+1+additions, text );
#else
		AppendPopupItem( hMainMenu, MF_STRING, MNU_EXIT+1+additions, text );
#endif
	}
	{
		PADDITION addition = New( ADDITION );
		addition->text = StrDupEx( text DBG_SRC );
		addition->f = function;
		addition->id = additions;
		SetLink( &Functions, additions, addition );
		additions++;
	}
}

void AddSystrayMenuFunction_v2( CTEXTSTR text, void (CPROC* function)(uintptr_t), uintptr_t param ){
	if( hMainMenu )
	{
#ifdef WIN32
		AppendMenu( hMainMenu, MF_STRING, MNU_EXIT+1+additions, text );
#else
		AppendPopupItem( hMainMenu, MF_STRING, MNU_EXIT+1+additions, text );
#endif
	}
	{
		PADDITION addition = New( ADDITION );
		addition->text = StrDupEx( text DBG_SRC );
		addition->f = NULL;
		addition->param = param;
		addition->f2 = function;
		addition->id = additions;
		SetLink( &Functions, additions, addition );
		additions++;
	}
}

//----------------------------------------------------------------------

int RegisterIconEx( CTEXTSTR user_icon DBG_PASS )
{
	int status;
	int tried_default = 0;
	//NOTIFYICONDATA nid;
	if( !RegisterIconHandler( user_icon ) )
		return 0;
	if( !hMainMenu )
		BasicExitMenu();
#ifdef WIN32

	nid.cbSize = sizeof( NOTIFYICONDATA );
	nid.hWnd = ghWndIcon;
	nid.uID = 0;
	nid.uFlags = NIF_ICON|NIF_MESSAGE;
	nid.uCallbackMessage = WM_USERICONMSG;


	if( (uintptr_t)icon & 0xFFFF0000 )
	{
		nid.hIcon = (HICON)LoadImage( NULL, icon, IMAGE_ICON
									, 0, 0
									, 
#ifndef _ARM_
									LR_LOADFROMFILE| 
									LR_DEFAULTSIZE
#else
									0
#endif
									);
		if( !nid.hIcon )
				nid.hIcon = (HICON)LoadImage( GetModuleHandle(NULL), icon, IMAGE_ICON
											, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE
#else
											, 0
#endif
											);
		if( !nid.hIcon )
				nid.hIcon = (HICON)LoadImage( hInstMe, icon, IMAGE_ICON
											, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE
#else
											, 0
#endif
											);
	}
	else
	{
		if( icon )
		{
			nid.hIcon = (HICON)LoadImage( GetModuleHandle(NULL), icon, IMAGE_ICON
										, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE 
#else
											, 0
#endif
										);
			if( !nid.hIcon )
				nid.hIcon = (HICON)LoadImage( hInstMe, icon, IMAGE_ICON
											, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE 
#else
											, 0
#endif
											);
			if( !nid.hIcon )
				nid.hIcon = (HICON)LoadImage( NULL, icon, IMAGE_ICON
											, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE 
#else
											, 0
#endif
											);
		}
		else
		{
			SetDefault:
			{
				nid.hIcon = (HICON)LoadImage( hInstMe
											, (TEXTCHAR*)ICO_DEFAULT, IMAGE_ICON
											, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE 
#else
											, 0
#endif
											);
				if( !nid.hIcon )
					nid.hIcon = (HICON)LoadImage( hInstMe, (TEXTCHAR*)"ICO_DEFAULT", IMAGE_ICON
												, 0, 0
#ifndef _ARM_
												,  LR_DEFAULTSIZE 
#else
												, 0
#endif
												);
			}
		}
	}
	if( !nid.hIcon && !tried_default )
	{
		//char msg[128];
		tried_default++;
		//sprintf( msg, DBG_FILELINEFMT "Failed to load icon" DBG_RELAY );
		//MessageBox( NULL, msg, "Systray Library", MB_OK );
      goto SetDefault;
	}
	status = Shell_NotifyIcon( NIM_ADD, &nid );
	DeleteObject( hLastIcon );
	hLastIcon = nid.hIcon;
#endif
	return status;
}


//----------------------------------------------------------------------

void ChangeIconEx( CTEXTSTR icon DBG_PASS )
{
#ifdef WIN32
	if( !ghWndIcon ) {
		RegisterIcon( icon );
		return;
	}
	nid.cbSize = sizeof( NOTIFYICONDATA );
	nid.hWnd = ghWndIcon;
	nid.uID = 0;
	nid.uFlags = NIF_ICON|NIF_MESSAGE;
	nid.uCallbackMessage = WM_USERICONMSG;
	if( (uintptr_t)icon & 0xFFFF0000 )
	{
		nid.hIcon = (HICON)LoadImage( GetModuleHandle(NULL), icon, IMAGE_ICON
								, 0, 0
#ifndef _ARM_
											,  LR_DEFAULTSIZE 
#else
											, 0
#endif
								);
		if( !nid.hIcon )
		nid.hIcon = (HICON)LoadImage( NULL, icon, IMAGE_ICON
								, 0, 0
#ifndef _ARM_
								, LR_LOADFROMFILE| LR_DEFAULTSIZE 
#else
											, 0
#endif
								);
	}
	else
	{
		nid.hIcon = (HICON)LoadImage( GetModuleHandle(NULL), icon, IMAGE_ICON
								, 0, 0
#ifndef _ARM_
								,  LR_DEFAULTSIZE 
#else
											, 0
#endif
								);
	}
	if( !nid.hIcon )
	{
		TEXTCHAR msg[128];
		tnprintf( msg, sizeof( msg ), DBG_FILELINEFMT "Failed to load icon" DBG_RELAY );
		MessageBox( NULL, msg, "Systray Library", MB_OK );
	}
	Shell_NotifyIcon( NIM_MODIFY, &nid );
	DeleteObject( hLastIcon );
	hLastIcon = nid.hIcon;
#endif
}

//----------------------------------------------------------------------

void UnregisterIcon( void )
{
#ifdef WIN32
	if( nid.cbSize )
	{
		nid.cbSize = sizeof( NOTIFYICONDATA );
		nid.hWnd = ghWndIcon;
		nid.uID = 0;
		nid.uFlags = NIF_ICON|NIF_MESSAGE;
		nid.uCallbackMessage = WM_USERICONMSG;
		Shell_NotifyIcon( NIM_DELETE, &nid );
	}
#endif
}

void TerminateIcon( void )
{
#if WIN32
	int attempt = 0;
	HWND hWndOld;
	TEXTCHAR iconwindow[256];
	tnprintf( iconwindow, sizeof( iconwindow ), "AlertAgentIcon:%s", GetProgramName() );
	while( ( hWndOld = FindWindow( "AlertAgentIcon", iconwindow ) ) && ( attempt < 5 ) )
	{
		if( localSystrayState.flags.bLog )
			Log( "Telling previous instance to exit." );
		SendMessage( hWndOld, WM_COMMAND, /*MNU_EXIT*/1000, 0 );
		Sleep( 100 );
		attempt++;
	}
	if( attempt == 5 )
	{
		DWORD dwProcess;
		//DWORD dwThread = 
		GetWindowThreadProcessId( hWndOld, &dwProcess );
		HANDLE hProc;
		if( localSystrayState.flags.bLog )
			lprintf( "posting didn't cause process to exit... attempting to terminate." );
		hProc = OpenProcess( SYNCHRONIZE | PROCESS_TERMINATE, FALSE, dwProcess );
		if( hProc == NULL )
		{
			if( localSystrayState.flags.bLog )
				lprintf( "Failed to open process handle..." );
			return;
		}
		TerminateProcess( hProc, 0xFEED );
		CloseHandle( hProc );
	}
#endif
}

ATEXIT( DoUnregisterIcon )
{
   UnregisterIcon();
}

#ifdef UNDEF_NO_OPEN_MACRO
#  undef UNDEF_NO_OPEN_MACRO
#  undef NO_OPEN_MACRO
#endif

// $Log: systray.c,v $
// Revision 1.15  2005/07/25 21:43:15  jim
// Fix regisrered name of non-default icons
//
// Revision 1.18  2005/07/25 21:41:49  d3x0r
// Didn't pass icon name new newly threaded - easy to use systray thing.
//
// Revision 1.17  2005/06/06 09:27:41  d3x0r
// Fix loading of the default icon.
//
// Revision 1.16  2005/06/05 05:23:43  d3x0r
// Add module filename to the menu so we know which default icon is which.
//
// Revision 1.15  2005/06/05 05:07:20  d3x0r
// Add default thread to accompany default icon and default behvaior of exit(0)
//
// Revision 1.14  2005/06/05 04:53:42  d3x0r
// Default exit method also calls exit().  THis is a termination exit, and does not allow the application to exit gracefully.  There is also a PostQuitMessage... which may or may not be received by the main thread (this thread??)
//
// Revision 1.13  2005/06/05 04:52:02  d3x0r
// Add default icon to systray handler... so we can registericon...
//
// Revision 1.12  2005/05/25 16:50:30  d3x0r
// Synch with working repository.
//
// Revision 1.13  2005/01/10 21:43:42  panther
// Unix-centralize makefiles, also modify set container handling of getmember index
//
// Revision 1.12  2004/12/20 22:32:49  panther
// Modifications to make idle proc check for thread instance
//
// Revision 1.11  2004/10/03 02:14:03  d3x0r
// Auto unregister icon at exit...
//
// Revision 1.10  2004/06/24 03:33:02  d3x0r
// Register idle proc for systray's ldle of peek/dispatch message which eveyrone else doesn't need to know.
//
// Revision 1.9  2004/06/15 21:33:36  d3x0r
// Define libmain to make compilation happier...
//
// Revision 1.8  2004/05/27 20:57:40  d3x0r
// Use PSI menus instead of windows menus...
//
// Revision 1.8  2004/05/21 00:57:49  jim
// Fix some mouse issues, track focus issues, fix some soft cursor issues...
//
// Revision 1.7  2003/11/09 22:33:13  panther
// Fix name of wndname if token is int resource
//
// Revision 1.6  2003/10/21 16:23:04  panther
// Append icon name to window title to produce 'unique' names to wake
//
// Revision 1.5  2002/10/16 10:22:12  panther
// Modified places to check for a named icon from.  - file/resource, resource
//
// Revision 1.4  2002/04/25 00:05:04  panther
// Added logging of PostQuitMessages/WM_QUIT...
//
// Revision 1.3  2002/04/18 20:42:52  panther
// minor cleanup.
//
// Revision 1.2  2002/04/18 17:48:24  panther
// Added doublclick icon method callback...
//
