#ifndef SQL_PASSWORD_GLOBAL_DEFINED
#define SQL_PASSWORD_GLOBAL_DEFINED

#include <InterShell/intershell_export.h>
#include <pssql.h>

typedef struct sql_password SQL_PASSWORD;
typedef struct sql_password *PSQL_PASSWORD;
typedef struct sql_token *PTOKEN;

struct sql_password
{
	PTRSZVAL button; // object to secure, may not be a button.
	PTRSZVAL psv;
	FLAGSETTYPE *permissions;
	int nTokens;
	TEXTSTR *pTokens;
	CTEXTSTR required_token;
   PTOKEN required_token_token;
   CTEXTSTR override_required_token;
   PTOKEN override_required_token_token;
};

struct sql_token
{
	INDEX id;
	CTEXTSTR name;
};

typedef struct sql_group *PGROUP;
struct sql_group
{
	INDEX id;
	TEXTSTR name;
	PLIST tokens;
};

typedef struct sql_user *PUSER;
struct sql_user
{
	INDEX id;
	TEXTSTR full_name;
	TEXTSTR name;
	TEXTSTR first_name;
	TEXTSTR last_name;
    TEXTSTR staff;
	PLIST groups;
	_32 dwFutTime; // expiration time...
	_32 dwFutTime_Created; // account creation
	_32 dwFutTime_Updated_Password; // last password update
};

struct _global_sql_password {
	PLIST secure_buttons;
	int permission_count;
	//PLIST permissions; // list of struct sql_token *'s (might be old list)
	PLIST tokens; // list of struct sql_token *'s
	PLIST groups; // list of groups
	struct {
      // set group when selecting... otherwise just select group.
		BIT_FIELD bSetGroup : 1;
		BIT_FIELD bPrintAccountCreated : 1;
		BIT_FIELD bSetTokens : 1;
		BIT_FIELD bUseSQL : 1;
		BIT_FIELD bInitializeLogins   : 1; // Closes all logins on this sytem.
	} flags;

	PLIST users; // cached list of users
	_32  prior_user_load; // prevent from reloading an already filled cache, but allow refreshing
	int fDays;

	PTHREAD pWindowThread;
	HWND hWndProxy;

	PUSER root_user;    // eventually this should be filled by the initial login (permarun from windows)
   INDEX root_user_login_id;

	PUSER current_user;
   INDEX current_user_login_id;
	PUSER temp_user;
   INDEX temp_user_login_id;

	INDEX system_id;

	int pass_expr_interval;       // Time interval for password expiration ( days )

};

#ifndef SQL_PASSWORD_MAIN
extern
#endif
struct _global_sql_password global_sql_password;
#define g global_sql_password

TEXTSTR getCurrentUser( void );

INDEX getCurrSystemID( void );

PSQL_PASSWORD GetButtonSecurity( PTRSZVAL button, int bCreate );

void ReloadUserCache( PODBC odbc );

PTOKEN FindToken( INDEX id, CTEXTSTR name );

// password_frame.c
struct password_info
{
	INDEX login_id;
   INDEX actual_login_id;
};

struct password_info * PromptForPassword( PUSER *result_user, INDEX *result_login_id, CTEXTSTR program, CTEXTSTR *tokens, int ntokens, PSQL_PASSWORD pls );
void LogOutPassword( INDEX login_id, INDEX actual_login_id, LOGICAL skip_logout );
void ResolveToken( PTOKEN *ppToken, CTEXTSTR *target, CTEXTSTR permission );

#endif
