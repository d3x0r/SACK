#ifndef NULL_PASSWORD_GLOBAL_DEFINED
#define NULL_PASSWORD_GLOBAL_DEFINED

#include <InterShell/intershell_export.h>
#include <pssql.h>

typedef struct null_password NULL_PASSWORD;
typedef struct null_password *PNULL_PASSWORD;

struct null_password
{
	PTRSZVAL button; // object to secure, may not be a button.
	PTRSZVAL psv;
	FLAGSETTYPE *permissions;
	int nTokens;
	TEXTSTR *pTokens;

};

typedef struct sql_token *PTOKEN;
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

struct _global_null_password {
	PLIST secure_buttons;
	int permission_count;
	PLIST permissions; // list of struct sql_token *'s (might be old list)
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

	PUSER current_user;
   INDEX current_user_login_id;
	PUSER temp_user;
   INDEX temp_user_login_id;

   INDEX system_id;
};

#ifndef NULL_PASSWORD_MAIN
extern
#endif
struct _global_null_password global_null_password;
#define g global_null_password

TEXTSTR getCurrentUser( void );

INDEX getCurrSystemID( void );

PNULL_PASSWORD GetButtonSecurity( PTRSZVAL button, int bCreate );

void ReloadUserCache( PODBC odbc );

// password_frame.c
INDEX PromptForPassword( PUSER *result_user, INDEX *result_login_id, CTEXTSTR program, CTEXTSTR *tokens, int ntokens );
void LogOutPassword( PTRSZVAL psv );

#endif
