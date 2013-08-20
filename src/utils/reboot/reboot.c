#include <stdhdrs.h>
#include <stdio.h>

void DoReboot( char *mode )
{
   int b95;
	HANDLE hToken, hProcess;
	TOKEN_PRIVILEGES tp;
	OSVERSIONINFO osvi;
//cpg27dec2006 c:\work\sack\src\utils\reboot\reboot.c(9): Warning! W202: Symbol 'dwPriority' has been defined, but not referenced
//cpg27dec2006 	DWORD dwPriority;
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	GetVersionEx( &osvi );
	if( osvi.dwPlatformId  == VER_PLATFORM_WIN32_NT )
		b95 = FALSE;
	else
		b95 = TRUE;

	if( DuplicateHandle( GetCurrentProcess(), GetCurrentProcess()
							 , GetCurrentProcess(), &hProcess, 0
							 , FALSE, DUPLICATE_SAME_ACCESS  ) )
	{
		if( b95 || OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
		{
			tp.PrivilegeCount = 1;
			if( b95 || LookupPrivilegeValue( NULL
													 , SE_SHUTDOWN_NAME
													 , &tp.Privileges[0].Luid ) )
			{
				if( !b95 )
				{
					tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
					AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
				}
				//ExitWindowsEx( EWX_LOGOFF|EWX_FORCE, 0 );
				{
					// PTEXT temp;
					// DECLTEXT( msg, WIDE("Initiating system shutdown...") );
					// EnqueLink( &ps->Command->Output, &msg );
					// if( !(temp = GetParam( ps, &param ) ) )
               if( stricmp( mode, "shutdown" ) == 0 )
						ExitWindowsEx( EWX_SHUTDOWN|EWX_FORCE, 0 );
               else if( stricmp( mode, "reboot" ) == 0 )
						ExitWindowsEx( EWX_REBOOT|EWX_FORCE, 0 );
					else
                  printf( "Mode specified invalid! (reboot/shutdown)\n" );
				 //  else
				   {
						//if( TextLike( temp, WIDE("logoff") ) )
						//	ExitWindowsEx( EWX_LOGOFF|EWX_FORCE, 0 );
						//else if( TextLike( temp, WIDE("reboot") ) )
						//	ExitWindowsEx( EWX_REBOOT|EWX_FORCE, 0 );
						//else if( TextLike( temp, WIDE("shutdown") ) )
						//	ExitWindowsEx( EWX_SHUTDOWN|EWX_FORCE, 0 );
					}
				}
			}
			else
			{
				//DECLTEXT( msg, WIDE("Failed to get privledge lookup...#######") );
				//msg.data.size = sprintf( msg.data.data, WIDE("Failed to get privledge lookup...%ld"), GetLastError() );
				//EnqueLink( &ps->Command->Output, &msg );
				GetLastError();

			}
		}
		else
		{
			//DECLTEXT( msg, WIDE("Failed to open process token... ########") );
			//msg.data.size = sprintf( msg.data.data, WIDE("Failed to open process token...%ld"), GetLastError() );
			//EnqueLink( &ps->Command->Output, &msg );
			GetLastError();
		}
	}
	else
		GetLastError();
	CloseHandle( hProcess );
	CloseHandle( hToken );
}

#ifndef DEDICATE_REBOOT
#include "systray.h"
#include "resource.h"
#include <network.h>

void CPROC Connection( PCLIENT whatever, PCLIENT pNew )
{
	RemoveClient( pNew );
	OpenTCPClientEx( WIDE("127.0.0.1"), 16662, NULL,NULL,NULL );
	Sleep(20);
   DoReboot( "reboot" );
}
#endif

#ifndef DEDICATE_REBOOT
SaneWinMain( argc, argv )
#else
int main( int argc, char **argv )
#endif
{
#ifndef DEDICATE_REBOOT
	RegisterIcon( (char*)ICO_REBOOT );
	NetworkStart();
   while( !OpenTCPServerEx( 16661, Connection ) )
   	Sleep( 2 );
	{
		MSG msg;
		while( GetMessage( &msg, NULL, 0, 0 ) )
		{
			DispatchMessage( &msg );
		}
	}
	UnregisterIcon();
#else
	DoReboot( argc > 1 ? argv[1] : "reboot" );
#endif
   return 0;
}
#ifndef DEDICATE_REBOOT
EndSaneWinMain()
#endif

