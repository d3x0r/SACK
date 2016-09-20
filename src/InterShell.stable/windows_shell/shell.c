
#include <stdhdrs.h>
#define DEFINES_INTERSHELL_INTERFACE
#define USES_INTERSHELL_INTERFACE
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <sqlgetoption.h>
#include "../widgets/include/banner.h"
#include "../intershell_registry.h"
#include "../intershell_export.h"
#include "../widgets/include/buttons.h"

static struct {
	TEXTCHAR shell[280];
	PLIST buttons; // when we update the shell update all buttons
} l;

struct SetShellButton {
	TEXTCHAR shell[280];
	PMENU_BUTTON button;
};


// My Resources
enum {
	EDIT_SHELL_NAME = 1500
};

// file associations can be done via
//	administrative privilege cmd.exe
//
//C:\Windows\system32>assoc .autoconfigbackup2=intershellbackup
//.autoconfigbackup2=intershellbackup
//
//C:\Windows\system32>assoc .autoconfigbackup3=intershellbackup
//.autoconfigbackup3=intershellbackup
//
//C:\Windows\system32>ftype intershellbackup=intershell.exe -restore -SQL %1
//intershellbackup=c:\full\path\to\intershell\intershell.exe -restore -SQL %1


//C:\Windows\system32>ftype /?
//Displays or modifies file types used in file extension associations
//
//FTYPE [fileType[=[openCommandString]]]
//
//  fileType  Specifies the file type to examine or change
//  openCommandString Specifies the open command to use when launching files
//						  of this type.
//
//Type FTYPE without parameters to display the current file types that
//have open command strings defined.  FTYPE is invoked with just a file
//type, it displays the current open command string for that file type.
//Specify nothing for the open command string and the FTYPE command will
//delete the open command string for the file type.  Within an open
//command string %0 or %1 are substituted with the file name being
//launched through the assocation.  %* gets all the parameters and %2
//gets the 1st parameter, %3 the second, etc.  %~n gets all the remaining
//parameters starting with the nth parameter, where n may be between 2 and 9,
//inclusive.  For example:
//
//	 ASSOC .pl=PerlScript
//	 FTYPE PerlScript=perl.exe %1 %*
//
//would allow you to invoke a Perl script as follows:
//
//	 script.pl 1 2 3
//
//If you want to eliminate the need to type the extensions, then do the
//following:
//
//	 set PATHEXT=.pl;%PATHEXT%
//
//and the script could be invoked as follows:
//
//	 script 1 2 3
//
//
//
//C:\Windows\system32>ftype intershellbackup=c:\bin32\intershell\intershell.exe -restore -SQL %1
//intershellbackup=c:\bin32\intershell\intershell.exe -restore -SQL %1
//





PRELOAD( AllowWindowsShell )
{
	// Credit to Nicolas Escuder for figuring out how to *Hide the pretteh XP Logon screen*!
	// Credit also to GeoShell 4.11.8 changelog entry
	lprintf( WIDE("Credit to Nicolas Escuder for figuring out how to *Hide the pretteh XP Logon screen*!") );
	lprintf( WIDE("Credit also to GeoShell version 4.11.8 changelog entry...") );
	{
		GetModuleFileName( NULL, l.shell, sizeof( l.shell ) );
		SACK_GetProfileString( GetProgramName(), WIDE("Windows Shell/Set Shell To"), l.shell, l.shell, sizeof( l.shell ) );
	}
	// Code borrowed from LiteStep (give it back when we're done using it ^.^).
	{
		HANDLE hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, WIDE("Global\\msgina: ShellReadyEvent") );	 // also: "Global\msgina: ReturnToWelcome"

		if( !hLogonEvent )
		{
			//lprintf( WIDE("Error : %d"), GetLastError() );
			hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, WIDE("msgina: ShellReadyEvent") );	 // also: "Global\msgina: ReturnToWelcome"
			if( !hLogonEvent )
			{
				//lprintf( WIDE("Error : %d"), GetLastError() );
				hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, WIDE("Global\\ShellDesktopSwitchEvent") );	 // also: "Global\msgina: ReturnToWelcome"

				if( !hLogonEvent )
				{
					//lprintf( WIDE("Error : %d"), GetLastError() );
					hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, WIDE("ShellDesktopSwitchEvent") );	 // also: "Global\msgina: ReturnToWelcome"
					if( !hLogonEvent )
						lprintf( WIDE("Error finding event to set for windows login") );
				}
			}
		}
		SetEvent( hLogonEvent );
		CloseHandle( hLogonEvent );
	}

	{
		/*
		 * configure InterShell as a sendto handler for certain file types...
		 * configure InterShell as the handler for .config.# files
		 *
		 *  HKEY_CLASSES_ROOT/.config.1
		 *	 Content Type=text/plain text/xml application/xml
		 *	 PerceivedType
		 *	~/OpenWithList
		 *	~/OpenWithProgids
		 *	~/PersisentHandler

		 */
	}

	EasyRegisterResource( WIDE("InterShell/Windows Shell"), EDIT_SHELL_NAME, EDIT_FIELD_NAME );
}

static void OnKeyPressEvent( WIDE("Windows Logoff") )( uintptr_t psv )
{
	/*
	HANDLE hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, WIDE("Global\\msgina: ReturnToWelcome") );
	if( !hLogonEvent )
		hLogonEvent = OpenEvent( EVENT_MODIFY_STATE, 1, WIDE("msgina: ReturnToWelcome") );
	SetEvent( hLogonEvent );

	CloseHandle( hLogonEvent );
	*/

	// 0, 0x8000000 | 0x4000000
#ifdef MINGW_SUX
#define SHTDN_REASON_FLAG_PLANNED  0x80000000
#define SHTDN_REASON_FLAG_USER_DEFINED  0x40000000
#endif
	ExitWindowsEx( EWX_LOGOFF, SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_FLAG_USER_DEFINED );
}

static uintptr_t OnCreateMenuButton( WIDE("Windows Logoff") )( PMENU_BUTTON button )
{
	return 1;
}

#if 0
/* this might be nice to have auto populated start menu
 * based on the original window system stuff... should be easy enough to do...
 */
OnKeyPressEvent( WIDE("Windows->Start") )( uintptr_t psv )
{
	PMENU menu;
	menu = CreatePopup();
	/*
	 * if( is98 )
	 * BuildMenuItemsPopup( menu, WIDE("/users/all users/start menu") );
	 * if( isXP )
	 * BuildMenuItemsPopup( menu, WIDE(WIDE("/documents and settings/all users/start menu")) );
	 * if( isVista )
	 * BuildMenuItemsPopup( menu, WIDE("/????") );
	 */
}

OnCreateMenuButton( WIDE("Windows->Start") )( PMENU_BUTTON button )
{
	return 1;
}
#endif


static void OnKeyPressEvent( WIDE("Windows/Set Permashell") )( uintptr_t psv )
{
	struct SetShellButton *my_button = ( struct SetShellButton *)psv;
	DWORD dwStatus;
	HKEY hTemp;

	dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
									 WIDE("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), 0,
									KEY_WRITE, &hTemp );

	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		//lprintf( WIDE("Write shell to: %s"), my_button->shell );
		dwStatus = RegSetValueEx(hTemp, WIDE("Shell"), 0
										  , REG_SZ
										  , (BYTE*)my_button->shell, strlen( my_button->shell ) * sizeof( TEXTCHAR ) );
		RegCloseKey( hTemp );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			Banner2Message( WIDE("Failed to set shell") );
		{
			INDEX idx;
			struct SetShellButton *button;
			LIST_FORALL( l.buttons, idx, struct SetShellButton *, button )
				UpdateButton( button->button );
		}
	}

	dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER,
									 WIDE("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0,
									 KEY_WRITE, &hTemp );
	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
		DWORD dwDispos;
		dwStatus = RegCreateKeyEx( HKEY_CURRENT_USER
										 , WIDE("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0
										 , NULL  //lpclass
										 , 0  // dwOptions
										 , KEY_WRITE //sam desired
										 , NULL // secureity attrib
										 , &hTemp
										 , &dwDispos
										 );

		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
		}
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		DWORD dwOne = 1;
		dwStatus = RegSetValueEx(hTemp, WIDE("DisableTaskMgr"), 0
										  , REG_DWORD
										  , (const BYTE *)&dwOne, sizeof( dwOne ) );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			Banner2Message( WIDE("Failed to set disable task manager") );

		RegCloseKey( hTemp );
	}
}

static void OnShowControl( WIDE("Windows/Set Permashell") )( uintptr_t psv )
{
	struct SetShellButton *my_button = ( struct SetShellButton *)psv;
	{
		DWORD dwStatus;
		HKEY hTemp;

		dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
										WIDE("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"), 0,
										KEY_QUERY_VALUE|KEY_WRITE, &hTemp );
		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
		}
		if( (dwStatus == ERROR_SUCCESS) && hTemp )
		{
			DWORD dwType;
			TEXTCHAR buffer[256];
			DWORD bufsize = sizeof( buffer );
			dwStatus = RegQueryValueEx( hTemp, WIDE("Shell"), 0
										 , &dwType, (LPBYTE)buffer, &bufsize );
			lprintf( WIDE("status %d %d %d %s %s"), dwStatus, dwType, bufsize, buffer, my_button->shell );
			if( StrCaseCmp( buffer, my_button->shell ) == 0 )
			{
				lprintf( WIDE("yes it's permashell...") );
				InterShell_SetButtonHighlight( my_button->button, TRUE );
			}
			else
				InterShell_SetButtonHighlight( my_button->button, FALSE );
			RegCloseKey( hTemp );
		}
		else
			InterShell_SetButtonHighlight( my_button->button, FALSE );
	}
}

static uintptr_t OnCreateMenuButton( WIDE("Windows/Set Permashell") )( PMENU_BUTTON button )
{
	struct SetShellButton *my_button = New( struct SetShellButton );
	my_button->button = button;
	StrCpy( my_button->shell, l.shell );
	AddLink( &l.buttons, my_button );
	InterShell_SetButtonStyle( button, WIDE("square") );
	InterShell_SetButtonText( button, WIDE("Disable_Windows_Shell") );
	InterShell_SetButtonColors( button, BASE_COLOR_LIGHTGREY, BASE_COLOR_BLACK, BASE_COLOR_BLACK, BASE_COLOR_BLACK );
	return (uintptr_t)my_button;
}

static void OnSaveControl( WIDE("Windows/Set Permashell") )( FILE *file, uintptr_t psv )
{
	struct SetShellButton *my_button = (struct SetShellButton*)psv;
	sack_fprintf( file, WIDE("Set Shell To:%s\n"), EscapeMenuString( my_button->shell ) );
}

static uintptr_t CPROC SetButtonShell( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, shell );
	struct SetShellButton *my_button = (struct SetShellButton*)psv;
	StrCpyEx( my_button->shell, shell, sizeof( my_button->shell ) / sizeof( my_button->shell[0] ) );
	return psv;
}

static void OnLoadControl( WIDE("Windows/Set Permashell") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, WIDE("Set Shell To:%m"), SetButtonShell );
}

static uintptr_t OnConfigureControl( WIDE("Windows/Set Permashell") )( uintptr_t psv, PSI_CONTROL parent )
{
	struct SetShellButton *my_button = (struct SetShellButton*)psv;
	PSI_CONTROL frame = LoadXMLFrameOver( parent, WIDE("ConfigurePermaShellButton.isFrame") );
	int ok = 0;
	int done = 0;
	if( frame )
	{
		DisplayFrame( frame );

		SetControlText( GetControl( frame, EDIT_SHELL_NAME ), my_button->shell );

		SetCommonButtons( frame, &done, &ok );
		CommonWait( frame );

		if( ok )
		{
			GetControlText( GetControl( frame, EDIT_SHELL_NAME ), my_button->shell, sizeof( my_button->shell ) / sizeof( my_button->shell[0] ) );
		}

		DestroyFrame( &frame );
	}
	return psv;

}

static void OnKeyPressEvent( WIDE("Windows/Set Windows Shell") )( uintptr_t psv )
{
	DWORD dwStatus;
	HKEY hTemp;

	dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
									 WIDE("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"), 0,
									 KEY_WRITE, &hTemp );
	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		dwStatus = RegSetValueEx(hTemp, WIDE("Shell"), 0
										  , REG_SZ
										  , (BYTE*)WIDE("explorer.exe"), strlen( WIDE("explorer.exe") ) * sizeof( WIDE(" ")[0] ) );
		RegCloseKey( hTemp );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			Banner2Message( WIDE("Failed to set shell") );
		{
			INDEX idx;
			struct SetShellButton *button;
			LIST_FORALL( l.buttons, idx, struct SetShellButton *, button )
				UpdateButton( button->button );
		}
	}


	dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER,
									 WIDE("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0,
									 KEY_WRITE, &hTemp );
	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
		DWORD dwDispos;
		dwStatus = RegCreateKeyEx( HKEY_CURRENT_USER
										 , WIDE("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System"), 0
										 , NULL
										 , 0
										 , KEY_WRITE
										 , NULL
										 , &hTemp
										 , &dwDispos
										 );

		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
		}
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		DWORD dwOne = 0;
		dwStatus = RegSetValueEx(hTemp, WIDE("DisableTaskMgr"), 0
										  , REG_DWORD
										  , (const BYTE *)&dwOne, sizeof( dwOne ) );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			Banner2Message( WIDE("Failed to set disable task manager") );

		RegCloseKey( hTemp );
	}

}

static uintptr_t OnCreateMenuButton( WIDE("Windows/Set Windows Shell") )( PMENU_BUTTON button )
{
	struct SetShellButton *my_button = New( struct SetShellButton );
	my_button->button = button;
	AddLink( &l.buttons, my_button );
	InterShell_SetButtonStyle( button, WIDE("square") );
	InterShell_SetButtonText( button, WIDE("Enable_Windows_Shell") );
	InterShell_SetButtonColors( button, BASE_COLOR_LIGHTGREY, BASE_COLOR_BLACK, BASE_COLOR_BLACK, BASE_COLOR_BLACK );
	return (uintptr_t)my_button;
}

static void OnShowControl( WIDE("Windows/Set Windows Shell") )( uintptr_t psv )
{
	struct SetShellButton * my_button = ( struct SetShellButton *)psv;
	{
		DWORD dwStatus;
		HKEY hTemp;

		dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
										WIDE("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"), 0,
										KEY_QUERY_VALUE|KEY_WRITE, &hTemp );
		if( dwStatus == ERROR_FILE_NOT_FOUND )
		{
		}
		if( (dwStatus == ERROR_SUCCESS) && hTemp )
		{
			DWORD dwType = REG_SZ;
			TEXTCHAR buffer[256];
			DWORD bufsize = sizeof( buffer );
			dwStatus = RegQueryValueEx( hTemp, WIDE("Shell"), 0
										 , &dwType, (LPBYTE)buffer, &bufsize );
			//dwStatus = RegGetValue(hTemp, WIDE("Winlogon"), WIDE("Shell"), RRF_RT_REG_SZ
			//							 , &dwType, buffer, &bufsize );
			lprintf( WIDE("status %d %d %d %s"), dwStatus, dwType, bufsize, buffer );
			if( StrCaseCmp( buffer, WIDE("explorer.exe") ) == 0 )
			{
				lprintf( WIDE("yes it's explorer...") );
				InterShell_SetButtonHighlight( my_button->button, TRUE );
			}
			else
				InterShell_SetButtonHighlight( my_button->button, FALSE );
			RegCloseKey( hTemp );
		}
		else
			InterShell_SetButtonHighlight( my_button->button, FALSE );
 	}
}


#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
