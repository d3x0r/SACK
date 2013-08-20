#include <stdhdrs.h>
#include <configscript.h>
#include <systray.h>

// process api
// make sure it links against psapi, and not kernel32 (windows7++)
#define PSAPI_VERSION 1
#include <psapi.h>

// order is tracked from top to bottom.
// so order 0 is the top window to organize.

#define l window_stacker_local

typedef struct window_tracker *PWINDOW_TRACKER;
typedef struct window_tracker WINDOW_TRACKER;
struct window_tracker
{
	DeclareLink( struct window_tracker );
	TEXTSTR name;

   // applications can have multiple windows.
	PLIST windows;

	int order;
	int order_found;
	INDEX pass_id; // last pass this was updated on
};

typedef struct enum_state *PENUM_STATE;
typedef struct enum_state ENUM_STATE;
struct enum_state {
	INDEX pass_id;  // which pass we're on
	INDEX index;
	PWINDOW_TRACKER prior_tracker;
};

static struct window_stacker_local {
	struct {
		BIT_FIELD bLog : 1;
	} flags;
	CTEXTSTR window_list;

	PWINDOW_TRACKER window_tracker_root;
	int trackers;

} window_stacker_local;


static void CheckWindow( PENUM_STATE enum_state, HWND hWnd )
{
	DWORD dwProcessID;
	TCHAR szEXEName[MAX_PATH];

	GetWindowThreadProcessId (hWnd, &dwProcessID);

	//lprintf( "%p %d", hWnd, dwProcessID );

	{
		// Get a handle to the process
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
												PROCESS_VM_READ, FALSE, dwProcessID);

		// Get the process name

		if (NULL != hProcess)
		{
			HMODULE hMod[128];
			DWORD cbNeeded = 0;
			//lprintf( "..." );
			if(EnumProcessModules(hProcess, hMod,
										 sizeof(hMod), &cbNeeded))
			{
				// only need the first handle, that's the program.
				int n = 0;
				//lprintf( "Got back %d", cbNeeded );
				//for( n = 0; n < cbNeeded / sizeof( HMODULE ); n++ )
				{
					//lprintf( "check %d", n );
					//Get the name of the exe file
					GetModuleBaseName(hProcess, hMod[n], szEXEName,
											sizeof(szEXEName)/sizeof(TCHAR));

					{
						TEXTCHAR *extension = StrRChr( szEXEName, WIDE('.') );
						PWINDOW_TRACKER tracker;
						if( extension )
							extension[0] = 0;

						if( l.flags.bLog )
							lprintf( "%s", szEXEName );

						for( tracker = l.window_tracker_root; tracker; tracker = NextThing( tracker ) )
						{
							//lprintf( "is %s==%s", tracker->name, szEXEName );
							if( StrCaseCmp( tracker->name, szEXEName ) == 0 )
							{
								INDEX idx2;
								HWND hWndCheck;
								if( l.flags.bLog )
									lprintf( "Found Process in my list... %s", tracker->name );
								LIST_FORALL( tracker->windows, idx2, HWND, hWndCheck )
								{
									if( hWndCheck == hWnd )
										break;
								}
								if( !hWndCheck )
								{
									if( l.flags.bLog )
										lprintf( "Adding window %p to %s", hWnd, tracker->name );
									AddLink( &tracker->windows, hWnd );
								}

								if( tracker->pass_id != enum_state->pass_id )
								{
									tracker->pass_id = enum_state->pass_id;
									// subtract 1... if there's 1 tracker then 0 is the order...
									tracker->order_found = ( l.trackers - 1 ) - enum_state->index;
									enum_state->index++;
								}
								else
								{
									if( enum_state->prior_tracker != tracker )
									{
										lprintf( "Windows are inter-mixed." );
									}
								}
								enum_state->prior_tracker = tracker;
							}
						}
					}
				}
			}
			else
				lprintf( "failed unermate process handles? %d(need %d)", GetLastError(), cbNeeded );
		}
		else
		{
			lprintf( "Failed to get handle?" );
		}
	}
}

static BOOL CALLBACK EnumWindowsProc (HWND hWnd, LPARAM lParam)
{
	PENUM_STATE enum_state = (PENUM_STATE)lParam;

	int nItem = 0;

	//Make sure that the window is visible

	DWORD dwProcessID;

	if (!IsWindowVisible (hWnd))
		return TRUE;

	GetWindowThreadProcessId (hWnd, &dwProcessID);

	//Get the name of the executable file
	CheckWindow( enum_state, hWnd );

	// return false if ending enumeration.
	return TRUE;
}

static void LocateWindows( void )
{
	static ENUM_STATE enum_state;

	enum_state.pass_id++;
	enum_state.index = 0;

	{
		PWINDOW_TRACKER tracker;
		for( tracker = l.window_tracker_root; tracker; tracker = NextThing( tracker ) )
		{
			EmptyList( &tracker->windows );
		}
	}
	EnumWindows( EnumWindowsProc, (LPARAM)&enum_state );
}

static LOGICAL UpdateWindows( void )
{
	LOGICAL updated = FALSE;
	HWND last_window = NULL;
	PWINDOW_TRACKER tracker;

	for( tracker = l.window_tracker_root; tracker; tracker = NextThing( tracker ) )
	{
		if( tracker->order_found != tracker->order )
		{
			INDEX idx2;
			HWND hWnd;
			LIST_FORALL( tracker->windows, idx2, HWND, hWnd )
			{
            updated = TRUE;
				if( last_window )
					SetWindowPos( hWnd
									, last_window
									, 0, 0, 0, 0
									, SWP_NOMOVE|SWP_NOSIZE|SWP_DEFERERASE
									 | SWP_NOACTIVATE
									 | SWP_NOCOPYBITS
									 | SWP_NOREDRAW
									);
				else
					SetWindowPos( hWnd
									, HWND_BOTTOM
									, 0, 0, 0, 0
									, SWP_NOMOVE|SWP_NOSIZE|SWP_DEFERERASE
									 | SWP_NOACTIVATE
									 | SWP_NOCOPYBITS
									 | SWP_NOREDRAW
									);
				last_window = hWnd;
			}
		}
		else
		{
			INDEX idx2;
			HWND hWnd;
			LIST_FORALL( tracker->windows, idx2, HWND, hWnd )
			{
				// the top window is the first in the list....
				last_window = hWnd;
				break;
			}
		}
	}
   return updated;
}

static PTRSZVAL CPROC MonitorWindows( PTHREAD thread )
{
	while( 1 )
	{
      if( l.flags.bLog )
			lprintf( "-------- BEGIN WINDOW SCAN -------" );
		LocateWindows();
		// if there was an update, then scan immediate, otherwise wait a couple seconds
		if( l.flags.bLog )
			lprintf( "-------- BEGIN WINDOW UPDATE(?) -------" );
		if( !UpdateWindows() )
			WakeableSleep( 2000 );
	}
	return 0;
}

static PTRSZVAL CPROC AddWindow( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PWINDOW_TRACKER wint = New( WINDOW_TRACKER );
	wint->name = StrDup( name );
	wint->windows = NULL;
	wint->order = l.trackers;
	LinkThing( l.window_tracker_root, wint );
	l.trackers++;
	return psv;
}

static void ProcessOrderList( void )
{
	PCONFIG_HANDLER pch;
	pch = CreateConfigurationHandler();
	AddConfigurationMethod( pch, "process name <%m>", AddWindow );
	ProcessConfigurationFile( pch, l.window_list, 0 );
}

static void Usage( void )
{
	static char buf[256];
	snprintf( buf, 256, "%s [-l] @<window_list.txt>\n"
				"-l  : enable logging\n"
				"Play list lines look like \n"
				"process name <name>\n"
				"\n"
				"the first in the list is the program to keep bottom most;\n"
				"each subsequent process is above the prior..." 
			  , GetProgramName() );
   MessageBox( NULL, buf, "blah", MB_OK );
}

SaneWinMain( argc, argv )
{
	int arg;

	TerminateIcon();
	RegisterIcon( NULL );

	for( arg = 1; arg < argc; arg++ )
	{
		if( argv[arg][0] == '@' )
		{
			l.window_list = DupCharToText( argv[arg]+1 );
		}
		if( argv[arg][0] == '-' )
		{
			switch( argv[arg][1] )
			{
			case 'l':
			case 'L':
            l.flags.bLog = 1;
            break;
			}
		}
	}

	if( !l.window_list )
	{
		Usage();
		return 0;
	}

	ProcessOrderList();
	ThreadTo( MonitorWindows, 0 );

	while( 1 )
		WakeableSleep( 10000 );

	return 0;
}
EndSaneWinMain()

