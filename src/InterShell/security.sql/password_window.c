// stub window proxy handler for communication with POS
//
#ifndef __LINUX__
#include <stdhdrs.h>
#include <deadstart.h>
#include <idle.h>

#include "global.h"

enum password_message_IDs{
   UWM_PASSWORD_LOGIN = WM_USER + 200,
   UWM_PASSWORD_LOGOUT,
   UWM_PASSWORD_LOGOUT_TEMP,
	UWM_PASSWORD_LOGIN_AUTO,
	UWM_PASSWORD_CHECK_TOKEN,
   UWM_PASSWORD_LOG_ACTION,
};

static int CPROC ProcessMessages( PTRSZVAL unused )
{
	MSG Msg;
   //lprintf( WIDE("Check messages.") );
	if( IsThisThread( g.pWindowThread ) )
	{
		if( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) )
		{
			DispatchMessage( &Msg );
			return 1;
		}
		return 0;
	}
	return -1;
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CREATE:
		return TRUE;
	case WM_TIMER:
		return TRUE;
	case UWM_PASSWORD_CHECK_TOKEN:
		{
			char atom_buf[256];
			static CTEXTSTR *token_list;
         PUSER user;
         CTEXTSTR token_start;
         int token_count;
			GlobalGetAtomName( (ATOM)lParam, atom_buf, sizeof( atom_buf ) );
			lprintf( "Token check for : %d %s", lParam, atom_buf );
			GlobalDeleteAtom( (ATOM)lParam );
			token_start = atom_buf;
			if( token_start )
			{
				ParseStringVector( token_start, &token_list, &token_count );
			}
			else
			{
				token_list = NULL;
				token_count = 0;
			}
			for( user = (g.temp_user?g.temp_user:g.current_user);
				 user;
				  user = ( (user == g.temp_user)?g.current_user:NULL ) )
			{
				int nToken;
				INDEX idx;
				PGROUP group;
            lprintf( "Checking current user..." );
				LIST_FORALL( user->groups, idx, PGROUP, group )
				{
					INDEX idx2;
					PTOKEN token;
					lprintf( "User has group : %s", group->name );
					LIST_FORALL( group->tokens, idx2, PTOKEN, token )
					{
						lprintf( "which has token : %s", token->name );
						for( nToken = 0; nToken < token_count; nToken++ )
						{
                     lprintf( "COmpare %s vs %s", token_list[nToken], token->name );
							if( StrCaseCmp( token_list[nToken], token->name ) == 0 )
                        return 1;
						}
					}
				}
			}
		}
      return 0;

	case UWM_PASSWORD_LOGIN:
		{
			TEXTCHAR atom_buf[256];
			static CTEXTSTR *token_list;
			CTEXTSTR program;	
			TEXTSTR token_start;
			int token_count;
			GlobalGetAtomName( (ATOM)lParam, atom_buf, sizeof( atom_buf ) );
			lprintf( "Login request for : %d %s", lParam, atom_buf );
			GlobalDeleteAtom( (ATOM)lParam );
			// safe to cast to textstr, it's actually a char buffer above...
			token_start = (TEXTSTR)StrChr( atom_buf, ':' );
			if( token_start )
			{
				token_start[0] = 0;
				program = atom_buf;
				token_start++;
				ParseStringVector( token_start, &token_list, &token_count );
			}
			else
			{
				program = NULL;
				token_list = NULL;
				token_count = 0;
			}

			if( g.current_user )
			{
				int nToken;
				INDEX idx;
				PGROUP group;
				lprintf( "Had a current user... %s", g.current_user->name );
				LIST_FORALL( g.current_user->groups, idx, PGROUP, group )
				{
					INDEX idx2;
					PTOKEN token;
					lprintf( "User has group : %s", group->name );
					LIST_FORALL( group->tokens, idx2, PTOKEN, token )
					{
						lprintf( "which has token : %s", token->name );
						for( nToken = 0; nToken < token_count; nToken++ )
						{
							lprintf( "Compare %s vs %s", token_list[nToken], token->name );
							if( StrCaseCmp( token_list[nToken], token->name ) == 0 )
								break;
						}
						if( nToken < token_count )
							break;
					}
					if( token )
						break;
				}
				if( group )
				{
					return 1;
				}
			}
			{
				struct password_info *pi;
				INDEX login_id;
				PUSER external_user;
				INDEX external_login_id;
				if( !( pi = PromptForPassword( &external_user, &external_login_id, program, token_list, token_count, NULL ) ) 
					|| pi->login_id == INVALID_INDEX )
				{
					return 0;
				}
				else
				{
					CTEXTSTR *results;
					login_id = pi->login_id;
					if( g.current_user )
					{
						g.temp_user = external_user;
						g.temp_user_login_id = external_login_id;
					}
					else
					{
						lprintf( "This should NEVER happen; something asking for a password, but there is no current user for the button launch" );
						g.current_user = external_user;
						g.current_user_login_id = external_login_id;
					}

					if( g.current_user_login_id )
					{
						DoSQLRecordQueryf( NULL, &results, NULL, "select login_id from login_history where actual_login_id=%d and system_id=%ld order by login_id desc limit 1"
											  , g.current_user_login_id
											  , g.system_id );
						if( results )
							DoSQLCommandf( "update login_history set actual_login_id=%s where login_id=%ld", results[0], login_id );
					}
					else
					{
						DoSQLCommandf( "update login_history set actual_login_id=%ld where login_id=%ld", g.current_user_login_id, login_id );
					}

					g.temp_user_login_id = login_id;

					results = NULL;
					if( DoSQLRecordQueryf( NULL, &results, NULL, "select name from permission_user_info join login_history using(user_id) where login_id=%ld", login_id ) )
					{
						ATOM aResult = GlobalAddAtom( results[0] );
						SQLEndQuery( NULL );
						return aResult;
					}
				}
			}
		}

      // unreachable code, just put in for clarity.
      return 0;
	case UWM_PASSWORD_LOGOUT:
		if( g.current_user_login_id )
		{
			lprintf( "Logout %d", lParam );
			DoSQLCommandf( "udpate login_history set logout_whenstamp=now() where login_id=%ld", g.current_user_login_id );
		}
		return TRUE;
	case UWM_PASSWORD_LOG_ACTION:
		{
			TEXTCHAR tmp1[256];
			TEXTCHAR tmp2[256];
         GlobalGetAtomName( (ATOM)wParam, tmp1, 256 );
			GlobalGetAtomName( (ATOM)lParam, tmp2, 256 );
			DoSQLCommandf( "insert into permission_user_log (login_id,description,logtype,log_whenstamp) values (%lu,\'%s\',\'%s\',now())"
							 , g.temp_user
							  ? g.temp_user_login_id
							  : g.current_user
							  ? g.current_user_login_id
							  : 0L
							 , tmp1, tmp2 );
         GlobalDeleteAtom( (ATOM)wParam );
         GlobalDeleteAtom( (ATOM)lParam );
         return TRUE;
		}
      break;

	case UWM_PASSWORD_LOGOUT_TEMP:
		if( g.temp_user_login_id )
		{
			lprintf( "Logout temporary login (override login)" );
			DoSQLCommandf( "update login_history set logout_whenstamp=now() where login_id=%ld", g.temp_user_login_id );
			g.temp_user_login_id = 0;
         g.temp_user = NULL;
		}
      return 0;
	}
	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

static int MakeProxyWindow( void )
{
	ATOM aClass;
	{
		WNDCLASS wc;
		memset( &wc, 0, sizeof(WNDCLASS) );
		wc.style = CS_OWNDC | CS_GLOBALCLASS;

		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hbrBackground = 0;
		wc.lpszClassName = "SQLSecurityProxyClass";
		aClass = RegisterClass( &wc );
		if( !aClass )
		{
			MessageBox( NULL, WIDE("Failed to register class to handle Barcode Proxy messagses."), WIDE("INIT FAILURE"), MB_OK );
			return FALSE;
		}
	}

	g.hWndProxy = CreateWindowEx( 0,
								 (char*)aClass,
								 "SQLSecurityProxy",
								 0,
								 0,
								 0,
								 0,
								 0,
								 HWND_MESSAGE, // Parent
								 NULL, // Menu
								 GetModuleHandle(NULL),
								 (void*)1 );
	if( !g.hWndProxy )
	{
		lprintf( WIDE("Failed to create window!?!?!?!") );
		MessageBox( NULL, WIDE("Failed to create window to handle SQL Security Proxy Messages"), WIDE("INIT FAILURE"), MB_OK );
		return FALSE;
	}
	return TRUE;
}

static PTRSZVAL CPROC HandleProxy( PTHREAD thread )
{
	MakeProxyWindow();
	{
		MSG msg;
		g.pWindowThread = MakeThread();
		AddIdleProc( ProcessMessages, 0 );
		while( GetMessage( &msg, NULL, 0, 0 ) )
			DispatchMessage( &msg );
	}
	return 0;
}

PRELOAD( InitWindowProxy )
{
	ThreadTo( HandleProxy, 0 );
}

#endif
