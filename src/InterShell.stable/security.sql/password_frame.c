//
//	login_monitor.c
//	(C) Copyright 2009 
//	Crafted by d3x0r
//						 
////////////////////////////////////////////////////////////////////////////
#define USES_INTERSHELL_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pri

#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <network.h>
#include <controls.h>
#include <sha1.h>
#include "../intershell_export.h"
#include "../intershell_registry.h"
#include "../widgets/include/banner.h"
#include "../widgets/include/keypad.h"
#include "../widgets/keypad/keypad/keypad.h"
//#include "sack/include/sqlgetoption.h"
//#include "sqlgetoption.h"
#include "comn_util.h"
#include "global.h"

#define MESSAGE_SIZE 100
#define NAME_BUFFER_SIZE 100
#define PASSCODE_BUFFER_SIZE 64
#define PASSWORD_ACCESS_LEVELS 16

//--------------------------------------------------------------------------------
struct user_list_user_tracker
{
	PUSER user;
	struct {
		BIT_FIELD bHasLogin : 1;
		BIT_FIELD bHasOverride : 1;
	} flags;
	TEXTSTR required_login_id;
	TEXTSTR required_login_shift;
};

//--------------------------------------------------------------------------------
// Local Struct
static struct local_password_frame {	

	PRENDER_INTERFACE pri;
	PIMAGE_INTERFACE pii;
	uint32_t width, height;
	int32_t x, y;

	PSI_CONTROL frame;	
	PRENDERER	renderer;
	int displays_wide;
	int displays_high;
	PSI_CONTROL keyboard;
	PSI_CONTROL keyboard2;	
	PMENU_BUTTON keyboard_button;
	PMENU_BUTTON keyboard_button2;	
	
	int done;
	int okay;
	int stage;

	TEXTSTR logee_id;
	TEXTSTR logee_name;

	PSI_CONTROL user_list;
	PLISTITEM selected_user;
	PUSER user;
	struct user_list_user_tracker *selected_user_item;

	PSI_CONTROL user_list2;
	PLIST user_list2_controls;
	PLISTITEM selected_user2;
	PUSER user2;

	PSI_CONTROL unlock_user_list;
	PLIST unlock_user_list_controls;
	PLISTITEM unlock_selected_user;
	PUSER unlock_user;

	PSI_CONTROL perm_group_list;
	PLIST group_list_controls;
	PLISTITEM selected_perm_group;
	PGROUP group;

	PSI_CONTROL perm_group_list2;
	PLISTITEM selected_perm_group2;
	PGROUP group2;

	PSI_CONTROL token_list;
	PLIST token_list_controls;
	PLISTITEM selected_token;
	PTOKEN token;	

	PSI_CONTROL token_list2;
	PLISTITEM selected_token2;
	PTOKEN token2;

	PODBC odbc;	
	CTEXTSTR sys_name;
	CTEXTSTR program_id;
	CTEXTSTR prog_name;
	CTEXTSTR default_room_id;

	uint8_t  bad_login_limit;			 // Login attempt limit for locking a user account
	CTEXTSTR bad_login_interval;  // Time interval to keep user locked out ( minutes )	
	// moved to global.h  //int pass_expr_interval;		 // Time interval for password expiration ( days )
	uint8_t  pass_check_num;			  // Number of passwords to check new against
	uint8_t  pass_check_days; 
	uint8_t  pass_min_length;			 // Minimum Length a Password can be 

	int password_len;
	TEXTCHAR password[PASSCODE_BUFFER_SIZE];
	int password_len2;
	TEXTCHAR password2[PASSCODE_BUFFER_SIZE];	
	TEXTCHAR out[SHA1HashSize * 2 + 1];

	TEXTCHAR	 cPassPop[MESSAGE_SIZE];
	CTEXTSTR	 sPassPop;	
	PVARIABLE	lvPassPop;

	TEXTCHAR	 cPassPop2[MESSAGE_SIZE];
	CTEXTSTR	 sPassPop2;	
	PVARIABLE	lvPassPop2;
	
	
	TEXTCHAR	 cChangePassword[MESSAGE_SIZE];
	CTEXTSTR	 sChangePassword;	
	PVARIABLE	lvChangePassword; 

	TEXTCHAR	 cUnlockAccount[MESSAGE_SIZE];
	CTEXTSTR	 sUnlockAccount;	
	PVARIABLE	lvUnlockAccount; 

	TEXTCHAR	 cTermAccount[MESSAGE_SIZE];
	CTEXTSTR	 sTermAccount;	
	PVARIABLE	lvTermAccount; 

	TEXTCHAR	 cExpPassword[MESSAGE_SIZE];
	CTEXTSTR	 sExpPassword;	
	PVARIABLE	lvExpPassword; 

	TEXTCHAR	 cAddPermGroup[MESSAGE_SIZE];
	CTEXTSTR	 sAddPermGroup;	
	PVARIABLE	lvAddPermGroup;

	TEXTCHAR	 cRemPermGroup[MESSAGE_SIZE];
	CTEXTSTR	 sRemPermGroup;	
	PVARIABLE	lvRemPermGroup;

	TEXTCHAR	 cAddToken[MESSAGE_SIZE];
	CTEXTSTR	 sAddToken;	
	PVARIABLE	lvAddToken;

	TEXTCHAR	 cRemToken[MESSAGE_SIZE];
	CTEXTSTR	 sRemToken;	
	PVARIABLE	lvRemToken;	

	TEXTCHAR	 cCreateGroup[MESSAGE_SIZE];
	CTEXTSTR	 sCreateGroup;	
	PVARIABLE	lvCreateGroup;

	TEXTCHAR	 group_name[NAME_BUFFER_SIZE];
	TEXTCHAR	 group_name2[NAME_BUFFER_SIZE];
	TEXTCHAR	 group_description[255];

	TEXTCHAR	 cDeleteGroup[MESSAGE_SIZE];
	CTEXTSTR	 sDeleteGroup;	
	PVARIABLE	lvDeleteGroup;	

	TEXTCHAR	 cCreateToken[MESSAGE_SIZE];
	CTEXTSTR	 sCreateToken;	
	PVARIABLE	lvCreateToken;

	TEXTCHAR	 token_name[NAME_BUFFER_SIZE];
	TEXTCHAR	 token_name2[NAME_BUFFER_SIZE];
	TEXTCHAR	 token_description[255];

	TEXTCHAR	 cDeleteToken[MESSAGE_SIZE];
	CTEXTSTR	 sDeleteToken;	
	PVARIABLE	lvDeleteToken;	

	TEXTCHAR	 cCreateUser[MESSAGE_SIZE];
	CTEXTSTR	 sCreateUser;	
	PVARIABLE	lvCreateUser;

	TEXTCHAR	 cCreateUser2[MESSAGE_SIZE];
	CTEXTSTR	 sCreateUser2;	
	PVARIABLE	lvCreateUser2;
	
	TEXTCHAR staff_id[NAME_BUFFER_SIZE];
	TEXTCHAR staff_id2[NAME_BUFFER_SIZE];
	TEXTCHAR first_name[NAME_BUFFER_SIZE];
	TEXTCHAR first_name2[NAME_BUFFER_SIZE];
	TEXTCHAR last_name[NAME_BUFFER_SIZE];
	TEXTCHAR last_name2[NAME_BUFFER_SIZE];
	TEXTCHAR user_name[NAME_BUFFER_SIZE];
	TEXTCHAR user_name2[NAME_BUFFER_SIZE];
	
	PSI_CONTROL pc_staff_id;
	PSI_CONTROL pc_first_name;
	PSI_CONTROL pc_last_name;
	PSI_CONTROL pc_user_name;
	PSI_CONTROL pc_password;
	PSI_CONTROL pc_password2;
	

	struct local_password_frame_flags 
	{
		BIT_FIELD init					 : 1; // Indicates preload has run	
		BIT_FIELD does_pass_expr		: 1; //
		BIT_FIELD does_pass_req_upper : 1; //
		BIT_FIELD does_pass_req_lower : 1; //
		BIT_FIELD does_pass_req_spec  : 1; // 
		BIT_FIELD does_pass_req_num	: 1; //
		BIT_FIELD matched				 : 1; // Indicates passwords matched on update
		BIT_FIELD first_loop			 : 1; // Indicates if first password has been entered		
		BIT_FIELD exit					 : 1; // Indicates to exit process
		BIT_FIELD bCreateSystemLogin  : 1; // At startup, create a login based on current system user
		BIT_FIELD cover_entire_canvas : 1;
	} flags;

	PUSER test_user;
	INDEX test_login_id;

	PUSER program_login_user;
	INDEX program_login_id;

	PLIST selectable_users; // list of struct user_list_user_tracker for resource managment
#define l password_frame_local
} l;

//--------------------------------------------------------------------------------
// List Box section
//--------------------------------------------------------------------------------
// Selection Change Handler
void CPROC selection( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM user_list_item )
{
	//l.user_list = pc;
	//l.selected_user = GetSelectedItem( pc );
	//l.selected_user = user_list_item;
	//l.user = (PUSER)GetItemData( user_list_item );

	snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "Please enter your password and press enter." );
	LabelVariableChanged( l.lvPassPop );

	return;
}

//--------------------------------------------------------------------------------
// User List Box
static uintptr_t OnCreateListbox( "SQL Users/User Selection Control List" )( PSI_CONTROL listbox )
{
	int tmp[5] = { 0, 100, 200, 250, 320 };
	l.user_list = listbox;
	SetSelChangeHandler( l.user_list, selection, 0 );
	SetListBoxTabStops( l.user_list, 5, tmp );
	SetCommonFont( l.user_list, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}

void CreateToken( CTEXTSTR token, CTEXTSTR description )
{
	TEXTCHAR buf[256];
	CTEXTSTR token_id;

	if( description == NULL )
		description = "<auto token create>";
	if( DoSQLCommandf( "insert into permission_tokens (name,log,description) values ('%s',%d,'%s')", token, 0, description ) )
	{
		token_id = GetLastInsertKey( "permission_tokens", "permission_id" );
	}
	else
	{
		CTEXTSTR result;
		if( DoSQLQueryf( &result, "select permission_id from permission_tokens where name='%s'", EscapeString( token ) ) && result )
		{
			token_id = StrDup( result );
			FindToken( token_id, token );
		}
		else
			token_id = NULL;
	}
	if( token_id == NULL )
	{
		lprintf( "Failed to create token." );
		SimpleMessageBox( NULL, "Create Token Failed", "Failed to create token." );
	}
	else
	{
		snprintf( buf, 256, " Token: %s was created by User: %s Name: %s", token, l.logee_id, l.logee_name );
		DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.logee_id, g.system_id, l.program_id, EscapeString( buf ) );
		FindToken( token_id, token );
	}
}

//--------------------------------------------------------------------------------


static LOGICAL HasPermission( PUSER user, CTEXTSTR *tokens, int nTokens )
{
	uint8_t user_added = 0;
	uint8_t user_terminated = 0;
	CTEXTSTR *result;
	PushSQLQuery();
	DoSQLRecordQueryf( NULL, &result, NULL, "select terminate from permission_user_info where user_id=%d", user->id );
	if( result && atoi( result[0] ) )
	{
		PopODBC();
		// user is terminted, has no permissions.
		return FALSE; 
	}

	PopODBC();
	if( tokens && nTokens )
	{		
		{
			// first, validate that tokens exist at all.
			INDEX idx3;
			PTOKEN group_token;
			int token;
			for( token = 0; token < nTokens; token++ )
			{
				LIST_FORALL( g.tokens, idx3, PTOKEN, group_token )
				{
					if( StrCaseCmp( group_token->name, tokens[token] ) == 0 )
						break;
				}
				if( !group_token )
				{
					CreateToken( tokens[token], NULL );
				}
			}
		}

		{
			INDEX idx2;
			PGROUP group;
			CTEXTSTR token;	
			
			LIST_FORALL( user->groups, idx2, PGROUP, group )
			{
				INDEX idx3;
				PTOKEN group_token;		
				int n = 0;
				LIST_FORALL( group->tokens, idx3, PTOKEN, group_token )
				{
					for( token = tokens[n = 0]; n < nTokens; token = tokens[++n] )
					{
						if( StrCaseCmp( group_token->name, token ) == 0 )
						{
							return TRUE;
						}
					}
				}
			}			
		}		
	}
	return FALSE;
}

//--------------------------------------------------------------------------------
// Fill User List 
void FillList( CTEXTSTR *tokens, int nTokens, PSQL_PASSWORD pls )
{	
	uint8_t n = 0;
	uint8_t user_added = 0;
	uint8_t user_terminated = 0;

	if( l.user_list )
		ResetList( l.user_list );	

	if( l.selectable_users )
	{
		INDEX idx;
		struct user_list_user_tracker *user;
		LIST_FORALL( l.selectable_users, idx, struct user_list_user_tracker *, user )
		{
			if( user->required_login_id )
				Release( user->required_login_id );
			if( user->required_login_shift )
				Release( user->required_login_shift );
			Release( user );
		}
		EmptyList( &l.selectable_users );
	}


	if( pls && pls->required_token )
	{
		PVARTEXT pvt_query = VarTextCreate();
		CTEXTSTR *results;
		vtprintf( pvt_query, "select user_id,login_id,shift,fiscalday from login_history "
					"join program_identifiers using (program_id) "
					"where logout_whenstamp=11111111111111 "
					"and program_name='%s'",
					pls->required_token
				  );

		for( DoSQLRecordQueryf( NULL, &results, NULL, GetText( VarTextPeek( pvt_query ) ) );
			 results;
			  GetSQLRecord( &results ) )
		{
			CTEXTSTR user_id =  results[0];
			PUSER user;
			INDEX idx;
			LIST_FORALL( g.users, idx, PUSER, user )
			{
				if( StrCmp( user->id, user_id ) == 0 )
				{
					if( HasPermission( user, tokens, nTokens ) )
					{
						struct user_list_user_tracker *user_entry = New( struct user_list_user_tracker );
						AddLink( &l.selectable_users, user_entry );
						user_entry->user = user;
						user_entry->flags.bHasOverride = 0;
						user_entry->flags.bHasLogin = 1;
						user_entry->required_login_id = StrDup( results[1] );
						user_entry->required_login_shift = StrDup( results[2] );
						{
							TEXTCHAR tmp[128];
							snprintf( tmp, 128, "%s\t%s\tS:%s", user->full_name, results[3], results[2] );
							SetItemData( AddListItem( l.user_list, tmp ), (uintptr_t)user_entry );
						}
						break;
					}
				}
			}
		}
		VarTextEmpty( pvt_query );
		if( pls->override_required_token_token )
		{
			vtprintf( pvt_query, "select distinct user_id from permission_user_info "
						"join permission_user using(user_id) "
						"join permission_set using(permission_group_id) "
						"where permission_id in (%d)",
						pls->override_required_token_token->id
				  );
			for( DoSQLRecordQueryf( NULL, &results, NULL, GetText( VarTextPeek( pvt_query ) ) );
				 results;
				  GetSQLRecord( &results ) )
			{
				TEXTSTR user_id =  results[0];
				PUSER user;
				INDEX idx;
				LIST_FORALL( g.users, idx, PUSER, user )
				{
					if( StrCmp( user->id, user_id ) ==0 )
					{
						if( HasPermission( user, tokens, nTokens ) )
						{
							struct user_list_user_tracker *user_entry = New( struct user_list_user_tracker );
							AddLink( &l.selectable_users, user_entry );
							user_entry->user = user;
							user_entry->flags.bHasOverride = 1;
							user_entry->flags.bHasLogin = 0;
							user_entry->required_login_id = NULL;
							user_entry->required_login_shift = NULL;
							SetItemData( AddListItem( l.user_list, user->full_name ), (uintptr_t)user_entry );
							break;
						}
					}
				}
			}
		}
		VarTextDestroy( &pvt_query );
		return;
	}

	if( tokens && nTokens )
	{		
		PUSER user;
		INDEX idx;
		{
			// first, validate that tokens exist at all.
			INDEX idx3;
			PTOKEN group_token;
			int token;
			for( token = 0; token < nTokens; token++ )
			{
				LIST_FORALL( g.tokens, idx3, PTOKEN, group_token )
				{
					if( StrCaseCmp( group_token->name, tokens[token] ) == 0 )
						break;
				}
				if( !group_token )
				{
					CreateToken( tokens[token], NULL );
				}
			}
		}
		LIST_FORALL( g.users, idx, PUSER, user )
		{
			if( HasPermission( user, tokens, nTokens ) )
			{
				struct user_list_user_tracker *user_entry = New( struct user_list_user_tracker );
				AddLink( &l.selectable_users, user_entry );
				user_entry->user = user;
				user_entry->flags.bHasOverride = 0;
				user_entry->flags.bHasLogin = 0;
				user_entry->required_login_id = NULL;
				user_entry->required_login_shift = NULL;
				SetItemData( AddListItem( l.user_list, user->full_name ), (uintptr_t)user_entry );	
			}
		}		
	}
	else
	{
		PUSER user;
		INDEX idx;		
		LIST_FORALL( g.users, idx, PUSER, user )
		{						
			struct user_list_user_tracker *user_entry = New( struct user_list_user_tracker );
			AddLink( &l.selectable_users, user_entry );
			user_entry->user = user;
			user_entry->flags.bHasOverride = 0;
			user_entry->flags.bHasLogin = 0;
			user_entry->required_login_id = NULL;
			user_entry->required_login_shift = NULL;
			SetItemData( AddListItem( l.user_list, user->full_name ), (uintptr_t)user_entry );				
		}	

	}
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Selection Change Handler
void CPROC selection2( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM user_list2_item )
{
	l.user_list2 = pc;
	//l.selected_user2 = GetSelectedItem( pc );
	l.selected_user2 = user_list2_item;
	return;
}
//--------------------------------------------------------------------------------
// User List Box 2
static uintptr_t OnCreateListbox( "SQL Users/User Selection Control List 2" )( PSI_CONTROL listbox )
{	
	int tmp[3] = { 0, 100, 200 };
	AddLink( &l.user_list2_controls, listbox );
	SetSelChangeHandler( listbox, selection2, 0 );
	SetListBoxTabStops( listbox, 3, tmp );
	SetCommonFont( listbox, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}


//--------------------------------------------------------------------------------
// Fill List Box 2
void FillList2( PSI_CONTROL listbox )
{
	if( listbox )
		ResetList( listbox );	

	if( listbox )
	{
		PUSER user;
		INDEX idx;		
		LIST_FORALL( g.users, idx, PUSER, user )
		{						
			SetItemData( AddListItem( listbox, user->full_name ), (uintptr_t)user );				
		}		
	}	
}

//--------------------------------------------------------------------------------
// Fill List Box 2 on show control
static void OnShowControl( "SQL Users/User Selection Control List 2" )( uintptr_t psv )
{	
	INDEX idx;
	PSI_CONTROL listbox;
	LIST_FORALL( l.user_list2_controls, idx, PSI_CONTROL,  listbox )
	{
		if( psv == (uintptr_t)listbox )
			FillList2( listbox );
	}
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Selection Change Handler
void CPROC unlock_selection( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM unlock_list_item )
{
	l.unlock_user_list = pc;
	//l.selected_user2 = GetSelectedItem( pc );
	l.unlock_selected_user = unlock_list_item;
	return;
}
//--------------------------------------------------------------------------------
// Unlock User List Box
static uintptr_t OnCreateListbox( "SQL Users/Unlock User Selection Control List" )( PSI_CONTROL listbox )
{	
	int tmp[3] = { 0, 100, 200 };
	AddLink( &l.unlock_user_list_controls, listbox );
	SetSelChangeHandler( listbox, unlock_selection, 0 );
	SetListBoxTabStops( listbox, 3, tmp );
	SetCommonFont( listbox, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}

//--------------------------------------------------------------------------------
// Fill Unlock List Box 
void FillUnlockList( PSI_CONTROL listbox )
{
	if( listbox )
		ResetList( listbox );	

	if( listbox )
	{
		PUSER user;
		INDEX idx;		
		LIST_FORALL( g.users, idx, PUSER, user )
		{
			//If locks are even possible
			if( *l.bad_login_interval != '0' )
			{
				CTEXTSTR *result;
				DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=4 and log_whenstamp >= now() - interval %s", user->id, l.bad_login_interval );
				if( result && atoi( result[0] ) > 0 )
				{
					DoSQLRecordQueryf( NULL, &result, NULL, "select system_exception_type_id,log_whenstamp from system_exceptions where user_id=%d and system_exception_type_id in ( 4, 6 ) and log_whenstamp >= now() - interval %s order by system_exception_id desc limit 1", user->id, l.bad_login_interval );				
					if( result && ( atoi( result[0] ) != 6 )  )	
						SetItemData( AddListItem( listbox, user->full_name ), (uintptr_t)user );	
				}
			}
		}		
	}	
}

//--------------------------------------------------------------------------------
// Fill Unlock List Box on show control
static void OnShowControl( "SQL Users/Unlock User Selection Control List" )( uintptr_t psv )
{	
	INDEX idx;
	PSI_CONTROL listbox;
	LIST_FORALL( l.unlock_user_list_controls, idx, PSI_CONTROL, listbox )
	{
		if( psv == (uintptr_t)listbox )
			FillUnlockList( listbox );
	}
}

//--------------------------------------------------------------------------------
// Permission Group List Box 
//--------------------------------------------------------------------------------
//

//--------------------------------------------------------------------------------
// Selection Change Handler
// Set information on selection to be used with (Add Group Token)
void CPROC group_selection( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM group_list_item )
{	
	l.perm_group_list = pc;
	//l.selected_perm_group = GetSelectedItem( pc );	
	l.selected_perm_group = group_list_item;
	return;
}

//--------------------------------------------------------------------------------
// Permission Group List Box 
static uintptr_t OnCreateListbox( "SQL Users/Permission Group Selection Control List" )( PSI_CONTROL listbox )
{	
	int tmp[3] = { 0, 100, 200 };	
	AddLink( &l.group_list_controls, listbox );	
	SetSelChangeHandler( listbox, group_selection, 0 );
	SetListBoxTabStops( listbox, 3, tmp );
	SetCommonFont( listbox, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}

//--------------------------------------------------------------------------------
//
void FillGroupList( PSI_CONTROL listbox )
{
	if( listbox )
		ResetList( listbox );	

	if( listbox )
	{
		INDEX idx;
		PGROUP group;
		LIST_FORALL( g.groups, idx, PGROUP, group )
		{
			SetItemData( AddListItem( listbox, group->name ), (uintptr_t)group );
		}
	}
}

//--------------------------------------------------------------------------------
// 
static void OnShowControl( "SQL Users/Permission Group Selection Control List" )( uintptr_t psv )
{
	INDEX idx;
	PSI_CONTROL listbox;
	LIST_FORALL( l.group_list_controls, idx, PSI_CONTROL, listbox )
	{
		if( psv == (uintptr_t)listbox )
			FillGroupList( listbox );			
	}	  	
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Permission Group List Box 2
static uintptr_t OnCreateListbox( "SQL Users/Permission Group Selection Control List2" )( PSI_CONTROL listbox )
{	
	int tmp[3] = { 0, 100, 200 };
	l.perm_group_list2 = listbox;
	SetListBoxTabStops( l.perm_group_list2, 3, tmp );
	SetCommonFont( l.perm_group_list2, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}

//--------------------------------------------------------------------------------
//
void FillGroupList2( void )
{
	if( l.perm_group_list2 )
		ResetList( l.perm_group_list2 );	

	if( l.perm_group_list2 )
	{
		INDEX idx;
		PGROUP group;
		LIST_FORALL( l.user2->groups, idx, PGROUP, group )
		{
			SetItemData( AddListItem( l.perm_group_list2, group->name ), (uintptr_t)group );
		}
	}
}

//--------------------------------------------------------------------------------
// Token List Box 
//--------------------------------------------------------------------------------
//

//--------------------------------------------------------------------------------
// Selection Change Handler
void CPROC token_selection( uintptr_t psvUser, PSI_CONTROL pc, PLISTITEM token_list_item )
{	
	l.token_list = pc;
	//l.selected_perm_group = GetSelectedItem( pc );	
	l.selected_token = token_list_item;
	return;
}

//--------------------------------------------------------------------------------
// Token List Box 
static uintptr_t OnCreateListbox( "SQL Users/Token Selection Control List" )( PSI_CONTROL listbox )
{	
	int tmp[3] = { 0, 100, 200 };	
	AddLink( &l.token_list_controls, listbox );	
	SetSelChangeHandler( listbox, token_selection, 0 );
	SetListBoxTabStops( listbox, 3, tmp );
	SetCommonFont( listbox, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}

//--------------------------------------------------------------------------------
//
void FillTokenList( PSI_CONTROL listbox )
{
	if( listbox )
		ResetList( listbox );

	if( listbox )
	{
		INDEX idx;
		PTOKEN group_token;
		LIST_FORALL( g.tokens, idx, PTOKEN, group_token )
		{
			lprintf( "Added token %s", group_token->name );
			SetItemData( AddListItem( listbox, group_token->name ), (uintptr_t)group_token );
		}
	}
}

//--------------------------------------------------------------------------------
// 
static void OnShowControl( "SQL Users/Token Selection Control List" )( uintptr_t psv )
{
	INDEX idx;
	PSI_CONTROL listbox;
	LIST_FORALL( l.token_list_controls, idx, PSI_CONTROL, listbox )
	{
		if( psv == (uintptr_t)listbox )
			FillTokenList( listbox );			
	}				
}

//--------------------------------------------------------------------------------
// Token List Box 2
//--------------------------------------------------------------------------------
// 
static uintptr_t OnCreateListbox( "SQL Users/Token Selection Control List2" )( PSI_CONTROL listbox )
{	
	int tmp[3] = { 0, 100, 200 };
	l.token_list2 = listbox;
	SetListBoxTabStops( l.token_list2, 3, tmp );
	SetCommonFont( l.token_list2, *( UseACanvasFont( GetCommonParent( listbox ), "User Select List Font" ) ) );
	return (uintptr_t)listbox;
}

//--------------------------------------------------------------------------------
//
void FillTokenList2( void )
{
	if( l.token_list2 )
		ResetList( l.token_list2 );	

	if( l.token_list2 )
	{
		INDEX idx;
		PTOKEN token;
		LIST_FORALL( l.group->tokens, idx, PTOKEN, token )
		{
			SetItemData( AddListItem( l.token_list2, token->name ), (uintptr_t)token );
		}
	}
}

//--------------------------------------------------------------------------------
// PromptForPassword Called from Test Security Context ( Handles everything for checking security )
// Another way to get token info
// PMENU_BUTTON button = (PMENU_BUTTON)psv;	
// PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
// FillUserList( l.user_list2, pls->pTokens, pls->nTokens );

//--------------------------------------------------------------------------------
// Update Local Coordinates
void UpdatePasswordFrame( void )
{
	uint32_t w, h;
	int32_t x, y;
	GetDisplaySizeEx( 0, &x, &y, &w, &h );
	lprintf( "Control is a new size old %d,%d new %d,%d", l.width, l.height, w, h );
	if( w != l.width || h != l.height )
	{
		lprintf( "Control is a new size old %d,%d new %d,%d", l.width, l.height, w, h );
		l.width = w;
		l.height = h;
		l.x = x;
		l.y = y;
		SizeCommon( l.frame
					 , l.width * (l.flags.cover_entire_canvas?l.displays_wide:1)
					 , l.height * (l.flags.cover_entire_canvas?l.displays_high:1) );
	}
}

//--------------------------------------------------------------------------------
// Create Password Frame and different pages to attach to the frames 
void CreatePasswordFrame( void )
{
	if( !l.renderer )
	{
		uint32_t w, h;
		int32_t x, y;
		GetDisplaySizeEx( 0, &x, &y, &w, &h );
		l.width = w;
		l.height = h;
		l.x = x;
		l.y = y;

		l.renderer = OpenDisplaySizedAt( 0, l.width * (l.flags.cover_entire_canvas?l.displays_wide:1)
												 , l.height * (l.flags.cover_entire_canvas?l.displays_high:1)
												 , l.x, l.y );
		SetRendererTitle( l.renderer, "User Login Frame" );
		MakeTopmost( l.renderer );
	}

	if( !l.frame )
	{
		PMENU_BUTTON button;
		// Default Password Page
		l.frame = MakeNamedControl( NULL, "Menu Canvas", 0, 0
										  , l.width * (l.flags.cover_entire_canvas?l.displays_wide:1)
										  , l.height * (l.flags.cover_entire_canvas?l.displays_high:1)
										  , 0 );
		InterShell_SetPageLayout( l.frame, 40, 40 );
		AttachFrameToRenderer( l.frame, l.renderer ); // need renderer built so we can attach key events to it.
		ShellSetCurrentPage( l.frame, "first" );
		InterShell_SetPageColor( ShellGetNamedPage( l.frame, "first" ), BASE_COLOR_BLACK );
		InterShell_CreateSomeControl( l.frame, 3, 2, 34, 20, "SQL Users/User Selection Control List" );
		l.keyboard = InterShell_GetButtonControl( l.keyboard_button = InterShell_CreateSomeControl( l.frame, 2, 23, 36, 16, "keyboard 2" ) );
		SetKeypadType( l.keyboard, "Enter Password Keypad" );
		SetKeypadStyle( l.keyboard, KEYPAD_STYLE_PASSWORD|KEYPAD_DISPLAY_ALIGN_CENTER );
		//InterShell_CreateSomeControl( l.frame, 30, 26, 5, 5, "SQL Users/Accept Password" );		
		//InterShell_CreateSomeControl( l.frame, 30, 32, 5, 5, "SQL Users/Cancel Generic" );
		button = InterShell_CreateSomeControl( l.frame, 2, 22, 36, 1, "Text Label" );
		InterShell_SetButtonText( button, "%<Pop Pass Message>" );
		InterShell_SetTextLabelOptions( button, TRUE, FALSE, FALSE, FALSE );
		//InterShell_SetButtonFont( button, "Small Txt Status" );		

		// Create Expired Password Page ( Used when a password is expired )
		InterShell_CreateNamedPage( l.frame, "Expired Password" );
		InterShell_SetPageColor( ShellGetNamedPage( l.frame, "Expired Password" ), BASE_COLOR_BLACK );
		l.keyboard2 = InterShell_GetButtonControl( l.keyboard_button2 = InterShell_CreateSomeControl( l.frame, 2, 23, 36, 16, "keyboard 2" ) );
		SetKeypadType( l.keyboard2, "Expire Password Keypad" );
		SetKeypadStyle( l.keyboard2, KEYPAD_STYLE_PASSWORD|KEYPAD_DISPLAY_ALIGN_CENTER );
		//InterShell_CreateSomeControl( l.frame, 30, 20, 5, 5, "SQL Users/Accept Expired Password" );
		//InterShell_CreateSomeControl( l.frame, 30, 26, 5, 5, "SQL Users/Cancel Generic" );
		button = InterShell_CreateSomeControl( l.frame, 2, 22, 36, 1, "Text Label" );
		InterShell_SetButtonText( button, "%<Pop Pass2 Message>" );
		InterShell_SetTextLabelOptions( button, TRUE, FALSE, FALSE, FALSE );
	}
}

//--------------------------------------------------------------------------------
//
struct password_info *PromptForPassword( PUSER *result_user, INDEX *result_login_id, CTEXTSTR program, CTEXTSTR *tokens, int ntokens, PSQL_PASSWORD pls )
{	
	CTEXTSTR *result;				  // Points to results read from the database
	TEXTSTR  time = NULL;
	TEXTSTR  time2 = NULL;
	struct password_info *password_info = New( struct password_info );
	uint8_t unlock = 0;					  // Flag to indicate if the user account unlocked was performed
	uint8_t unlocked = 0;					// Flag to indicate if the user account is locked				 
	uint8_t expired = 0;					 // Flag to indicate if the user account has expired	

	password_info->login_id = INVALID_INDEX;
	password_info->actual_login_id = INVALID_INDEX;
	CreatePasswordFrame();			// Create Renderer & pages
	UpdatePasswordFrame();			// Resize

	FillList( tokens, ntokens, pls );	// Fill List box control with users
	
	// Display Canvas ( will always reset page to first page )
	// other parameters don't matter, already have a renderer
	DisplayMenuCanvas( l.frame, NULL, 0, 0, 0, 0 );
	if( result_user )
		(*result_user) = NULL;
	if( result_login_id )
		(*result_login_id) = INVALID_INDEX;

	while( !l.flags.matched )
	{
		time = NULL;		
		unlock = 0;					  
		unlocked = 0;	

		l.done = l.okay = 0;	
		CommonLoop( &l.done, &l.okay );	 // Wait until until l.done or l.okay is true	
	
		// If okay was pressed ( else return invalid index )
		if( l.okay )
		{	
			//Check if terminated
			DoSQLRecordQueryf( NULL, &result, NULL, "select terminate from permission_user_info where user_id=%d", l.user->id );
			if( result && atoi( result[0] ) > 0 )
			{			
				snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "This user has been terminated." );
				LabelVariableChanged( l.lvPassPop );
			}

			else
			{
				//Check if locked, if locked, check unlock, if not unlocked pop up banner message and return invalid index
				if( *l.bad_login_interval != '0' )
				{
					DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=4 and log_whenstamp >= now() - interval %s", l.user->id, l.bad_login_interval );
					if( result && atoi( result[0] ) > 0 )
					{
						DoSQLRecordQueryf( NULL, &result, NULL, "select system_exception_type_id,log_whenstamp from system_exceptions where user_id=%d and system_exception_type_id in ( 4, 6 ) and log_whenstamp >= now() - interval %s order by system_exception_id desc limit 1", l.user->id, l.bad_login_interval );				
						if( result && result[0] && atoi( result[0] ) == 6  )	
						{
							unlocked = unlock = 1;	
							time = StrDup( result[1] );
						}
						else
						{
							snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "The selected account has exceeded the maximum number of failed login attempts." );
							LabelVariableChanged( l.lvPassPop );
						}
					}

					else
						unlocked = 1;
				}

				else
					unlocked = 1;
			}
		
			if( unlocked )
			{
				// Check Password
				DoSQLRecordQueryf( NULL, &result, NULL, "select 1 from permission_user_info where user_id=%d and password='%s'"
									  , l.user->id, l.out );
				if( result )
				{
					// Log good password
					if( l.selected_user_item->flags.bHasLogin )
					{
						password_info->actual_login_id = atoi( l.selected_user_item->required_login_id );
						DoSQLCommandf( "insert into login_history (actual_login_id,shift,fiscalday,system_id,program_id,user_id,login_whenstamp) values (%s,%s,%s,%s,%s,%s,now())"
							, l.selected_user_item->required_login_id
							, l.selected_user_item->required_login_shift
							, GetSQLOffsetDate( NULL, "Fiscal", 5 )
							, g.system_id
							, program?GetProgramID( program ):l.program_id
							, l.user->id );

					}
					else
					{
						// override, and normal login, use this, not a chain login
						DoSQLCommandf( "insert into login_history (fiscalday,system_id,program_id,user_id,login_whenstamp) values (%s,%s,%s,%s,now())"
										 , GetSQLOffsetDate( NULL, "Fiscal", 5 ), g.system_id, program?GetProgramID( program ):l.program_id, l.user->id );
					}
					// Get Login ID
					password_info->login_id = GetLastInsertID( NULL, NULL );

					if( result_user )
						(*result_user) = l.user;
					if( result_login_id )
						(*result_login_id) = password_info->login_id;

					lprintf( "Doing login... %p %p %s %s", g.current_user, g.temp_user, l.user->name, password_info->login_id );
					// Check if already current user if so

					snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "Success." );
					LabelVariableChanged( l.lvPassPop );

					// Check expiration
					if( l.flags.does_pass_expr && g.pass_expr_interval )
					{
						DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_user_info where user_id=%d and password_creation_datestamp <= now() - interval %d day", l.user->id, g.pass_expr_interval );
						if( result && atoi( result[0] ) > 0 )
							expired = 1;
					}

					l.flags.matched = 1;
				}

				// Log Bad Password Entry	
				if( password_info->login_id == INVALID_INDEX )
				{					
					DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (1,1,%d,%d,%d,'%s',now())", l.user->id, g.system_id, l.program_id, "failed login" );	
				
					if( *l.bad_login_interval != '0' )
					{
						//Check to see if successful login in the last 30 min						
						DoSQLRecordQueryf( NULL, &result, NULL, "select login_whenstamp from login_history where user_id=%d and login_whenstamp >= now() - interval %s order by login_id desc limit 1", l.user->id, l.bad_login_interval );
						if( result && result[0] )
							time2 = StrDup( result[0] );

						// Check if I should set lock ( if unlock has been performed
						if( unlock )
						{
							DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=1 and log_whenstamp >= IF(cast('%s', as datetime) < cast('%s', as datetime), '%s', '%s' )", l.user->id, time, time2, time2, time );  
							//if( time2 && ( time2 > time ) )
								//DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=1 and log_whenstamp >= '%s'", l.user->id, time2 );

							//else
								//DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=1 and log_whenstamp >= '%s'", l.user->id, time );
						}
				
						else
						{							
							if( time2 )
								DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=1 and log_whenstamp >= '%s'", l.user->id, time2 );

							else
								DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from system_exceptions where user_id=%d and system_exception_type_id=1 and log_whenstamp >= now() - interval %s", l.user->id, l.bad_login_interval );		
						}

						if( result && ( atoi( result[0] ) >= l.bad_login_limit ) )				
								DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (1,4,%d,%d,%d,'%s',now())", l.user->id, g.system_id, l.program_id, "user locked out" );	

						if( atoi( result[0] ) > 0 )
						{
							snprintf( l.cPassPop, MESSAGE_SIZE, "Invalid Password. %d attempts left.", ( l.bad_login_limit - atoi( result[0] ) ) );
							LabelVariableChanged( l.lvPassPop );
						}

						else
						{
							snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "Invalid Password." );
							LabelVariableChanged( l.lvPassPop );
						}
					}

					else
					{
						snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "Invalid Password." );
						LabelVariableChanged( l.lvPassPop );
					}
				}				
			}		
		}	

		else
			l.flags.matched = 1;
			
		if( time )
		{
			Release( time );
			time = NULL;
		}

		if( time2 )
		{
			Release( time2 );
			time2 = NULL;
		}

		unlocked = unlock = 0;
	}	

	// Hide Canvas
	HideControl( l.frame );
	l.flags.matched = 0;

	//Reset Message
	snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "Please enter your password and press enter." );
	LabelVariableChanged( l.lvPassPop );

	// If reset password call necessary page
	if( expired )
	{	
		// Set and display page to be shown
		ShellSetCurrentPage( l.frame, "Expired Password" );
		RevealCommon( l.frame );		
		SetCommonFocus( l.frame );
	
		// clearn done and okay first, otherwise it passes the wait, and hides as it shows which leaves it showing.
		l.done = l.okay = 0;			
		// Loop until user enters 2 matching		
		while( !l.done && !l.okay )
		{
			CommonLoop( &l.done, &l.okay );	
			//if( !l.okay )
				//l.flags.matched = 1;
		}

		// Reset Flag & Hide Canvas
		l.flags.first_loop = 0;
		l.flags.matched = 0;
		HideControl( l.frame );

		//Reset Message
		snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "Your password has expired! Please enter a new password and press enter." );
		LabelVariableChanged( l.lvPassPop2 );

		// If cancel issued
		if( l.done == TRUE )
		{
			l.done = l.okay = 0;
			return NULL;
		}

		l.done = l.okay = 0;
	}		

	// Return status if not 0 execute button
	return password_info;
}

//--------------------------------------------------------------------------------
//
void LogOutPassword( INDEX login_id, INDEX actual_login_id, LOGICAL skip_logout )
{
	CTEXTSTR *results;
	if( !actual_login_id )
		actual_login_id = login_id;
	PushSQLQuery();
	for( DoSQLRecordQueryf( NULL, &results, NULL
								 , (actual_login_id != INVALID_INDEX)
								  ?"select login_id,logout_whenstamp from login_history where actual_login_id=%ld or actual_login_id=%ld"
								  :"select login_id,logout_whenstamp from login_history where actual_login_id=%ld"
								 , login_id, actual_login_id );
		 results;
		  GetSQLRecord( &results ) )
	{
		if( StrCmp( results[1], "1111-11-11 11:11:11" ) == 0 )
			LogOutPassword( atoi( results[0] ), INVALID_INDEX, FALSE );
		else
			LogOutPassword( atoi( results[0] ), INVALID_INDEX, TRUE );
	}
	PopODBC();
	if( !skip_logout )
		DoSQLCommandf( "update login_history set logout_whenstamp=now() where login_id=%d", login_id );
}
//--------------------------------------------------------------------------------
// Accept Generic
static uintptr_t OnCreateMenuButton( "SQL Users/Accept Generic" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Okay" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Accept Generic" )( uintptr_t psv )
{	
	l.okay = TRUE;
	l.done = TRUE;
}

//--------------------------------------------------------------------------------
// Cancel Generic
static uintptr_t OnCreateMenuButton( "SQL Users/Cancel Generic" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Cancel" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Cancel Generic" )( uintptr_t psv )
{	
	l.done = TRUE;
}

//--------------------------------------------------------------------------------
// Test Password Dialog
static uintptr_t OnCreateMenuButton( "SQL Users/Test Password Dialog" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Test" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Test Password Dialog" )( uintptr_t psv )
{	
	PromptForPassword( &l.test_user, &l.test_login_id, NULL, NULL, 0, NULL );
}

//--------------------------------------------------------------------------------
// Accept Password for first Page
void promptOkay( void )
{
	TEXTCHAR sha1[SHA1HashSize];
	SHA1Context sc;

	l.password_len = 0;
	l.selected_user = NULL;

	l.password_len = GetKeyedText( l.keyboard, l.password, 64 );	
	l.selected_user = GetSelectedItem( l.user_list );

	if( l.selected_user && l.password_len )
	{
		l.selected_user_item = (struct user_list_user_tracker*)GetItemData( l.selected_user );
		l.user = l.selected_user_item->user;		
		SHA1Reset( &sc );
		SHA1Input( &sc, (uint8_t*)l.password, strlen( l.password ) );
		SHA1Result( &sc, (uint8_t*)sha1 );
		ConvertFromBinary( l.out, (uint8_t*)sha1, SHA1HashSize );

		//ClearSelectedItems( l.user_list );
		ClearKeyedEntry( l.keyboard );

		l.okay = TRUE;
		l.done = TRUE;
	}

	else if( !l.selected_user && !l.password_len )
	{
		snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "No User Selected or Password Entered. Please try again." );
		LabelVariableChanged( l.lvPassPop );
	}

	else if( !l.selected_user )
	{		
		snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
		LabelVariableChanged( l.lvPassPop );
	}

	else
	{
		snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
		LabelVariableChanged( l.lvPassPop );
	}
}

static void OnKeypadEnterType( "Password Entry", "Enter Password Keypad" )( PSI_CONTROL pc_keypad )
{
	promptOkay();
}

static void OnKeypadCancelType( "Password Entry", "Enter Password Keypad" )( PSI_CONTROL pc_keypad )
{
	l.done = TRUE;
}

static uintptr_t OnCreateMenuButton( "SQL Users/Accept Password" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Okay" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Accept Password" )( uintptr_t psv )
{	
	promptOkay();
}

//--------------------------------------------------------------------------------
// Accept Password for Expired Password Page

void expirePromptOkay( void )
{
	// Get First Entry
	if( !l.flags.first_loop )
	{	
		TEXTCHAR *temp;
		TEXTCHAR bad = 0;
		TEXTCHAR uppercase_used = 0;
		TEXTCHAR lowercase_used = 0;
		TEXTCHAR num_used = 0;
		TEXTCHAR spec_used = 0;

		l.password_len = 0;
		l.password_len = GetKeyedText( l.keyboard2, l.password, 64 );
		ClearKeyedEntry( l.keyboard2 );

		if( l.password_len )
		{
			if( l.password_len >= l.pass_min_length )
			{
				if( l.flags.does_pass_req_upper )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 64 && *temp < 91 )
							uppercase_used = 1;
					if( !uppercase_used )
					{
						bad = 1;					
						snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "An upper case character is required! Please try again." );
						LabelVariableChanged( l.lvPassPop2 );
					}
				}

				if( l.flags.does_pass_req_lower && !bad )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 96 && *temp < 123 )
							lowercase_used = 1;
					if( !lowercase_used )
					{
						bad = 1;					
						snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "A lower case character is required! Please try again." );
						LabelVariableChanged( l.lvPassPop2 );
					}
				}

				if( l.flags.does_pass_req_num && !bad )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 47 && *temp < 58 )
							num_used = 1;
					if( !num_used )
					{
						bad = 1;					
						snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "A numeric character is required! Please try again." );
						LabelVariableChanged( l.lvPassPop2 );
					}
				}

				if( l.flags.does_pass_req_spec && !bad )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( ( *temp > 32 && *temp < 48 ) || ( *temp > 57 && *temp < 65 ) || ( *temp > 90 && *temp < 97 ) || ( *temp > 122 && *temp < 127 ) )
							spec_used = 1;
					if( !spec_used )
					{
						bad = 1;					
						snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "A special character is required! Please try again." );
						LabelVariableChanged( l.lvPassPop2 );
					}
				}

				if( !bad )
				{
					l.flags.first_loop = 1;
					snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "Please repeat password and press enter again." );
					LabelVariableChanged( l.lvPassPop2 );
				}
			
			}

			else
			{			
				snprintf( l.cPassPop2, MESSAGE_SIZE, "Passwords must be at least %d characters in length.", l.pass_min_length );
				LabelVariableChanged( l.lvPassPop2 );
			}
		}

		else
		{
			snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
			LabelVariableChanged( l.lvPassPop2 );
		}
	}

	// Get Second Entry
	else
	{
		l.flags.first_loop = 0;
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( l.keyboard2, l.password2, 64 );
		ClearKeyedEntry( l.keyboard2 );

		if( l.password_len2 )
		{
		
			// If passwords match sha1, check it hasnt been used and write to the database else pop up banner and repeat
			if( StrCmp( l.password, l.password2 ) == 0 )
			{
				TEXTCHAR buf[256];	
				CTEXTSTR *result; 
				TEXTCHAR password_used = 0;
				SHA1Context sc;					
				TEXTCHAR sha1[SHA1HashSize];
				SHA1Reset( &sc );
				SHA1Input( &sc, (uint8_t*)l.password, strlen( l.password ) );
				SHA1Result( &sc, (uint8_t*)sha1 );
				ConvertFromBinary( l.out, (uint8_t*)sha1, SHA1HashSize );

				// Check password hasn't been used
				if( l.pass_check_num )
				{
					for( DoSQLRecordQueryf( NULL, &result, NULL, "select password from permission_user_password where user_id=%d order by creation_datestamp desc limit %d", l.user->id, l.pass_check_num )
						; result
						; FetchSQLRecord( NULL, &result ) )
					{
						if( StrCmp( result[0], l.out ) == 0 )
							password_used = 1;
					}
				}

				if( l.pass_check_days )
				{
					for( DoSQLRecordQueryf( NULL, &result, NULL, "select password from permission_user_password where user_id=%d and creation_datestamp >= now() - interval %d day order by creation_datestamp desc", l.user->id, l.pass_check_days )
						; result
						; FetchSQLRecord( NULL, &result ) )
					{
						if( StrCmp( result[0], l.out ) == 0 )
							password_used = 1;
					}
				}

				//If password hasn't been used update database else pop up banner message
				if( !password_used )
				{
					l.flags.matched = 1;				
					snprintf( buf, 256, " User: %s Name: %s had expired",  l.user->id, l.user->name );
					DoSQLCommandf( "insert into permission_user_password (user_id,password,description,creation_datestamp) values ('%s','%s','%s',now())", l.user->id, l.out, buf );
					DoSQLCommandf( "update permission_user_info set password='%s',password_creation_datestamp=now() where user_id=%d", l.out, l.user->id );	
					snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "The password has been succesfully updated." );
					LabelVariableChanged( l.lvPassPop2 );
					l.okay = TRUE;
				}

				else
				{	
					if( l.pass_check_num && l.pass_check_days )
					{
						snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "The password entered matches one of the previously used passwords! Please try again." );
						LabelVariableChanged( l.lvPassPop2 );
					}

					else if( l.pass_check_num )
					{
						snprintf( l.cPassPop2, MESSAGE_SIZE, "The password entered matches one of the %d previously used passwords! Please try again.", l.pass_check_num );
						LabelVariableChanged( l.lvPassPop2 );
					}

					else if( l.pass_check_days )
					{
						snprintf( l.cPassPop2, MESSAGE_SIZE, "The password entered matches one of the passwords used in the last %d days! Please try again.", l.pass_check_days );
						LabelVariableChanged( l.lvPassPop2 );
					}
				}
			}
		
			else
			{			
				snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "The passwords entered do not match! Please try again." );
				LabelVariableChanged( l.lvPassPop2 );
			}
		}

		else
		{
			snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
			LabelVariableChanged( l.lvPassPop2 );
		}
	}		
				
	//l.okay = TRUE;
	//l.done = TRUE;
}

static void OnKeypadEnterType( "Password Update", "Expire Password Keypad" )( PSI_CONTROL pc_keypad )
{
	expirePromptOkay();
}

static void OnKeypadCancelType( "Password Update", "Expire Password Keypad" )( PSI_CONTROL pc_keypad )
{
	l.done = TRUE;
}

static uintptr_t OnCreateMenuButton( "SQL Users/Accept Expired Password" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Okay" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Accept Expired Password" )( uintptr_t psv )
{	
	expirePromptOkay();	
}

//--------------------------------------------------------------------------------
// Static Pages
//--------------------------------------------------------------------------------
// Change Password Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Change Password" )( PSI_CONTROL pc_canvas )
{
	 l.flags.first_loop = 0;

	if( l.lvChangePassword )
	{		
		snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "Please enter a new password and press enter." );
		l.sChangePassword = l.cChangePassword;	
		LabelVariableChanged( l.lvChangePassword );
	}

	if( l.user_list2 )
		ClearSelectedItems( l.user_list2 );	

	if( l.selected_user2 )
		l.selected_user2 = NULL;

	 return 1;
}

//--------------------------------------------------------------------------------
// Change Password Keypad
static void OnKeypadEnterType( "change password", "Change Password Keypad" )( PSI_CONTROL pc_keypad )
{
	if( !l.selected_user2 )
	{		
		snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
		l.sChangePassword = l.cChangePassword;	
		LabelVariableChanged( l.lvChangePassword );
	}
	
	else
	{
		l.user2 = (PUSER)GetItemData( l.selected_user2 );

		if( !l.flags.first_loop )
		{
			TEXTCHAR *temp;						
			TEXTCHAR bad = 0;
			TEXTCHAR uppercase_used = 0;
			TEXTCHAR lowercase_used = 0;
			TEXTCHAR num_used = 0;
			TEXTCHAR spec_used = 0;		
			
			l.password_len = 0;
			l.password_len = GetKeyedText( pc_keypad, l.password, 64 );
			ClearKeyedEntry( pc_keypad );

			if( l.password_len )
			{
		
				if( l.password_len >= l.pass_min_length )
				{
					if( l.flags.does_pass_req_upper )
					{
						for( temp = l.password; *temp != '\0'; temp++ )
							if( *temp > 64 && *temp < 91 )
								uppercase_used = 1;
						if( !uppercase_used )
						{
							bad = 1;						
							snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "An upper case character is required. Please try again." );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}
					}

					if( l.flags.does_pass_req_lower && !bad )
					{
						for( temp = l.password; *temp != '\0'; temp++ )
							if( *temp > 96 && *temp < 123 )
								lowercase_used = 1;
						if( !lowercase_used )
						{
							bad = 1;						
							snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "A lower case character is required. Please try again." );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}
					}

					if( l.flags.does_pass_req_num && !bad )
					{
						for( temp = l.password; *temp != '\0'; temp++ )
							if( *temp > 47 && *temp < 58 )
								num_used = 1;
						if( !num_used )
						{
							bad = 1;						
							snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "A numeric character is required. Please try again." );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}
					}

					if( l.flags.does_pass_req_spec && !bad )
					{
						for( temp = l.password; *temp != '\0'; temp++ )
							if( ( *temp > 32 && *temp < 48 ) || ( *temp > 57 && *temp < 65 ) || ( *temp > 90 && *temp < 97 ) || ( *temp > 122 && *temp < 127 ) )
								spec_used = 1;
						if( !spec_used )
						{
							bad = 1;						
							snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "A special character is required. Please try again." );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}
					}

					if( !bad )
					{
						l.flags.first_loop = 1;
						snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "Please repeat password and press enter again." );
						l.sChangePassword = l.cChangePassword;	
						LabelVariableChanged( l.lvChangePassword );
					}
			
				}

				else
				{									
					snprintf( l.cChangePassword, MESSAGE_SIZE, "Passwords must be at least %d characters in length.", l.pass_min_length );
					l.sChangePassword = l.cChangePassword;	
					LabelVariableChanged( l.lvChangePassword );
				}				
			}

			else
			{
				snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
				l.sChangePassword = l.cChangePassword;	
				LabelVariableChanged( l.lvChangePassword );
			}
		}

		// Get Second Entry
		else
		{			
			l.flags.first_loop = 0;
			l.password_len2 = 0;
			l.password_len2 = GetKeyedText( pc_keypad, l.password2, 64 );
			ClearKeyedEntry( pc_keypad );

			if( l.password_len2 )
			{			
				// If passwords match sha1, check it hasnt been used and write to the database else pop up banner and repeat
				if( StrCmp( l.password, l.password2 ) == 0 )
				{
					TEXTCHAR buf[256];
					CTEXTSTR *result; 
					TEXTCHAR password_used = 0;
					SHA1Context sc;					
					TEXTCHAR sha1[SHA1HashSize];
					SHA1Reset( &sc );
					SHA1Input( &sc, (uint8_t*)l.password, strlen( l.password ) );
					SHA1Result( &sc, (uint8_t*)sha1 );
					ConvertFromBinary( l.out, (uint8_t*)sha1, SHA1HashSize );

					// Check password hasn't been used
					if( l.pass_check_num )
					{
						for( DoSQLRecordQueryf( NULL, &result, NULL, "select password from permission_user_password where user_id=%d order by creation_datestamp desc limit %d", l.user2->id, l.pass_check_num )
							; result
							; FetchSQLRecord( NULL, &result ) )
						{
							if( StrCmp( result[0], l.out ) == 0 )
								password_used = 1;
						}
					}

					if( l.pass_check_days )
					{
						for( DoSQLRecordQueryf( NULL, &result, NULL, "select password from permission_user_password where user_id=%d and creation_datestamp >= now() - interval %d day order by creation_datestamp desc", l.user2->id, l.pass_check_days )
							; result
							; FetchSQLRecord( NULL, &result ) )
						{
							if( StrCmp( result[0], l.out ) == 0 )
								password_used = 1;
						}
					}
									

					//If password hasn't been used update database else pop up banner message
					if( !password_used )
					{				
						snprintf( buf, 256, " User: %s Name: %s password was changed by User: %s Name: %s",  l.user2->id, l.user2->name, l.logee_id, l.logee_name );
						DoSQLCommandf( "insert into permission_user_password (user_id,password,description,creation_datestamp) values (%d,'%s','%s',now())", l.user2->id, l.out, buf );
						DoSQLCommandf( "update permission_user_info set password='%s',password_creation_datestamp=now() where user_id=%d", l.out, l.user2->id );	
						snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "The password has been succesfully updated." );
						l.sChangePassword = l.cChangePassword;	
						LabelVariableChanged( l.lvChangePassword );
						ReloadUserCache( NULL );
					}

					else
					{	
						if( l.pass_check_num && l.pass_check_days )
						{
							snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "The password entered matches one of the previously used passwords! Please try again." );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}

						else if( l.pass_check_num )
						{
							snprintf( l.cChangePassword, MESSAGE_SIZE, "The password entered matches one of the %d previously used passwords! Please try again.", l.pass_check_num );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}

						else if( l.pass_check_days )
						{
							snprintf( l.cChangePassword, MESSAGE_SIZE, "The password entered matches one of the passwords used in the last %d days! Please try again.", l.pass_check_days );
							l.sChangePassword = l.cChangePassword;	
							LabelVariableChanged( l.lvChangePassword );
						}						
					}
				}
		
				else
				{				
					snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "The passwords entered do not match! Please try again." );
					l.sChangePassword = l.cChangePassword;	
					LabelVariableChanged( l.lvChangePassword );
				}
			}

			else
			{
				snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
				l.sChangePassword = l.cChangePassword;	
				LabelVariableChanged( l.lvChangePassword );
			}
		}
	}		
}

//--------------------------------------------------------------------------------
//Unlock Account Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Unlock Account" )( PSI_CONTROL pc_canvas )
{ 
	if( l.lvUnlockAccount )
	{
		snprintf( l.cUnlockAccount, MESSAGE_SIZE, "%s", "Please select a user to unlock and press unlock user account." );
		l.sUnlockAccount = l.cUnlockAccount;	
		LabelVariableChanged( l.lvUnlockAccount );
	}

	if( l.unlock_user_list )
		ClearSelectedItems( l.unlock_user_list );

	if( l.unlock_selected_user )
		l.unlock_selected_user = NULL;

	 return 1;
}

//--------------------------------------------------------------------------------
// Unlock Account
static uintptr_t OnCreateMenuButton( "SQL Users/Unlock Account" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Unlock User Account" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Unlock Account" )( uintptr_t psv )
{ 	
	TEXTCHAR buf[256];		

	if( l.unlock_selected_user )
	{
		l.unlock_user = (PUSER)GetItemData( l.unlock_selected_user );		
		snprintf( buf, 256, " User: %s Name: %s was unlocked by User: %s Name: %s",  l.unlock_user->id, l.unlock_user->name, l.logee_id, l.logee_name ); 		
		DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (1,6,%d,%d,%d,'%s',now())", l.unlock_user->id, g.system_id, l.program_id, buf );	
		snprintf( l.cUnlockAccount, MESSAGE_SIZE, "%s's user account is now unlocked.", l.unlock_user->name  );
		l.sUnlockAccount = l.cUnlockAccount;	
		LabelVariableChanged( l.lvUnlockAccount );
	}

	else
	{
		snprintf( l.cUnlockAccount, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
		l.sUnlockAccount = l.cUnlockAccount;	
		LabelVariableChanged( l.lvUnlockAccount );
	}
}

//--------------------------------------------------------------------------------
//Terminate Account Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Terminate Account" )( PSI_CONTROL pc_canvas )
{ 
	if( l.lvTermAccount )
	{
		snprintf( l.cTermAccount, MESSAGE_SIZE, "%s", "Please select a user to terminate and press terminate user account." );
		l.sTermAccount = l.cTermAccount;	
		LabelVariableChanged( l.lvTermAccount );
	}

	if( l.user_list2 )
		ClearSelectedItems( l.user_list2 );

	if( l.selected_user2 )
		l.selected_user2 = NULL;

	 return 1;
}
//--------------------------------------------------------------------------------
// Terminate Account
static uintptr_t OnCreateMenuButton( "SQL Users/Terminate Account" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Terminate User Account" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Terminate Account" )( uintptr_t psv )
{ 	
	int terminate;
	TEXTCHAR buf[256];
	CTEXTSTR result;	

	if( l.selected_user2 )
	{
		l.user2 = (PUSER)GetItemData( l.selected_user2 );	

		if( DoSQLQueryf( &result, "select terminate from permission_user_info where user_id=%d", l.user2->id ) && result )
		terminate = atoi( result );		

		if( terminate )
		{
			snprintf( buf, 256, " User: %s Name: %s was unterminated by User: %s Name: %s",  l.user2->id, l.user2->name, l.logee_id, l.logee_name ); 			  
			DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (1,9,%d,%d,%d,'%s',now())", l.user2->id, g.system_id, l.program_id, buf );
			DoSQLCommandf( "update permission_user_info set terminate=0 where user_id=%d", l.user2->id  );

			snprintf( l.cTermAccount, MESSAGE_SIZE, "%s's user account is now renewed.", l.user2->name  );
			l.sTermAccount = l.cTermAccount;	
			LabelVariableChanged( l.lvTermAccount );
		}

		else
		{		
			snprintf( buf, 256, " User: %s Name: %s was terminated by User: %s Name: %s",  l.user2->id, l.user2->name, l.logee_id, l.logee_name ); 			  
			DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (1,9,%d,%d,%d,'%s',now())", l.user2->id, g.system_id, l.program_id, buf );
			DoSQLCommandf( "update permission_user_info set terminate=1 where user_id=%d", l.user2->id  );

			snprintf( l.cTermAccount, MESSAGE_SIZE, "%s's user account is now terminated.", l.user2->name  );
			l.sTermAccount = l.cTermAccount;	
			LabelVariableChanged( l.lvTermAccount );		
		}		
	}

	else
	{
		snprintf( l.cTermAccount, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
		l.sTermAccount = l.cTermAccount;	
		LabelVariableChanged( l.lvTermAccount );
	}
		
}

//--------------------------------------------------------------------------------
//Expire Password Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Expire Password" )( PSI_CONTROL pc_canvas )
{ 
	if( l.lvTermAccount )
	{
		snprintf( l.cExpPassword, MESSAGE_SIZE, "%s", "Please select the user whose password is to be expired and press expire user password." );
		l.sExpPassword = l.cExpPassword;	
		LabelVariableChanged( l.lvExpPassword );
	}

	if( l.user_list2 )
		ClearSelectedItems( l.user_list2 );

	if( l.selected_user2 )
		l.selected_user2 = NULL;

	 return 1;
}

//--------------------------------------------------------------------------------
// Expire Account
static uintptr_t OnCreateMenuButton( "SQL Users/Expire Password" )( PMENU_BUTTON button )
{
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Expire User Password" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Expire Password" )( uintptr_t psv )
{ 	
	TEXTCHAR buf[256];	

	if( l.selected_user2 )
	{
		l.user2 = (PUSER)GetItemData( l.selected_user2 );	

		snprintf( buf, 256, " User: %s Name: %s was expired by User: %s Name: %s",  l.user2->id, l.user2->name, l.logee_id, l.logee_name );			  
		DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (1,17,%d,%d,%d,'%s',now())", l.user2->id, g.system_id, l.program_id, buf );
		DoSQLCommandf( "update permission_user_info set password_creation_datestamp = now() - interval %d day where user_id=%d", g.pass_expr_interval, l.user2->id );
		
		snprintf( l.cExpPassword, MESSAGE_SIZE, "%s's password is now expired.", l.user2->name  );
		l.sExpPassword = l.cExpPassword;	
		LabelVariableChanged( l.lvExpPassword );
	}

	else
	{
		snprintf( l.cExpPassword, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
		l.sExpPassword = l.cExpPassword;	
		LabelVariableChanged( l.lvExpPassword );
	}	
}

//--------------------------------------------------------------------------------
// Add User Group Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Add User Group" )( PSI_CONTROL pc_canvas )
{ 
	if( l.lvAddPermGroup )
	{
		snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "Please select a user and a group to add the user to and press add user group." );
		l.sAddPermGroup = l.cAddPermGroup;	
		LabelVariableChanged( l.lvAddPermGroup );
	}	

	if( l.user_list2 )	
		ClearSelectedItems( l.user_list2 );

	if( l.selected_user2 )
		l.selected_user2 = NULL;

	if( l.perm_group_list )
		ClearSelectedItems( l.perm_group_list );

	if( l.selected_perm_group )
		l.selected_perm_group = NULL;

	 return 1;
}


//--------------------------------------------------------------------------------
// Add User Permission Group
static uintptr_t OnCreateMenuButton( "SQL Users/Add User Permission Group" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Add User Group" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Add User Permission Group" )( uintptr_t psv )
{
	TEXTCHAR buf[256];
	int group_id;
	CTEXTSTR *result;	

	if( l.selected_user2 && l.selected_perm_group )
	{
		l.user2 = (PUSER)GetItemData( l.selected_user2 );	
		l.group = (PGROUP)GetItemData( l.selected_perm_group );

		DoSQLRecordQueryf( NULL, &result, NULL, "select permission_group_id from permission_group where name='%s'", l.group->name );
		if( result && atoi( result[0] ) > 0 )
		{
			group_id = atoi( result[0] );

			DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_user where permission_group_id=%d and user_id=%d", group_id, l.user2->id );
			if( result && atoi( result[0] ) > 0 )
			{
				snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "The user is already part of this group." );
				l.sAddPermGroup = l.cAddPermGroup;	
				LabelVariableChanged( l.lvAddPermGroup );
			}

			else
			{
				DoSQLCommandf( "insert into permission_user (permission_group_id,user_id) values (%d,%d)", group_id, l.user2->id );
				
				snprintf( buf, 256, " User: %s Name: %s was added to Group: %s by User: %s Name: %s",  l.user2->id, l.user2->name, l.group->name, l.logee_id, l.logee_name ); 		
				DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.user2->id, g.system_id, l.program_id, buf );

				snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "The user was successfully added to the group." );
				l.sAddPermGroup = l.cAddPermGroup;	
				LabelVariableChanged( l.lvAddPermGroup );

				ReloadUserCache( NULL );
			}			
		}

		else
		{
			snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "Could not find permission group id." );
			l.sAddPermGroup = l.cAddPermGroup;	
			LabelVariableChanged( l.lvAddPermGroup );
		}
	}

	else if( l.selected_perm_group )
	{
		snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
		l.sAddPermGroup = l.cAddPermGroup;	
		LabelVariableChanged( l.lvAddPermGroup );
	}

	else if( l.selected_user2 )
	{
		snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
		l.sAddPermGroup = l.cAddPermGroup;	
		LabelVariableChanged( l.lvAddPermGroup );
	}
	else
	{
		snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "No User or Group Selected. Please try again." );
		l.sAddPermGroup = l.cAddPermGroup;	
		LabelVariableChanged( l.lvAddPermGroup );
	}
}

//--------------------------------------------------------------------------------
// Remove User Group Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Remove User Group" )( PSI_CONTROL pc_canvas )
{ 
	l.stage = 0;

	if( l.lvRemPermGroup )
	{
		snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "Please select a user who is to be removed from a permission group and press remove user group." );
		l.sRemPermGroup = l.cRemPermGroup;	
		LabelVariableChanged( l.lvRemPermGroup );
	}

	if( l.user_list2 )
		ClearSelectedItems( l.user_list2 );

	if( l.selected_user2 )
		l.selected_user2 = NULL;

	if( l.perm_group_list2 )
		ResetList( l.perm_group_list2 );

	//if( l.perm_group_list2 )
		//ClearSelectedItems( l.perm_group_list2 );

	if( l.selected_perm_group2 )
		l.selected_perm_group2 = NULL;

	 return 1;
}


//--------------------------------------------------------------------------------
// Remove User Permission Group
static uintptr_t OnCreateMenuButton( "SQL Users/Remove User Permission Group" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Remove User Group" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Remove User Permission Group" )( uintptr_t psv )
{
	TEXTCHAR buf[256];
	int group_id;
	CTEXTSTR *result;

	if( l.stage == 0 )
	{
		if( l.selected_user2 )
		{
			l.user2 = (PUSER)GetItemData( l.selected_user2 );
			FillGroupList2();

			snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "Please select a group to be removed and press remove user group again." );
			l.sRemPermGroup = l.cRemPermGroup;	
			LabelVariableChanged( l.lvRemPermGroup );
			l.stage++;
		}

		else
		{
			snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "No User Selected. Please try again." );
			l.sRemPermGroup = l.cRemPermGroup;	
			LabelVariableChanged( l.lvRemPermGroup );
		}

	}
	
	else if( l.stage == 1 )
	{
		l.selected_perm_group2 = GetSelectedItem( l.perm_group_list2 );

		if( l.selected_perm_group2 )
		{				
			l.group2 = (PGROUP)GetItemData( l.selected_perm_group2 );

			DoSQLRecordQueryf( NULL, &result, NULL, "select permission_group_id from permission_group where name='%s'", l.group2->name );
			if( result && atoi( result[0] ) > 0 )
			{
				group_id = atoi( result[0] );
				DoSQLCommandf( "delete from permission_user where user_id=%d and permission_group_id=%d", l.user2->id, group_id );

				snprintf( buf, 256, " User: %s Name: %s was removed from Group: %s by User: %s Name: %s",  l.user2->id, l.user2->name, l.group2->name, l.logee_id, l.logee_name ); 		
				DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.user2->id, g.system_id, l.program_id, buf );

				snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "The user was successfully removed from the group." );
				l.sRemPermGroup = l.cRemPermGroup;	
				LabelVariableChanged( l.lvRemPermGroup );

				ReloadUserCache( NULL );
			}

			else
			{
				snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "Could not find permission group id." );
				l.sRemPermGroup = l.cRemPermGroup;	
				LabelVariableChanged( l.lvRemPermGroup );
			}

			l.stage--;
		}

		else
		{
			snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
			l.sRemPermGroup = l.cRemPermGroup;	
			LabelVariableChanged( l.lvRemPermGroup );
		}
	}	
}

//--------------------------------------------------------------------------------
// Add Group Token Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Add Group Token" )( PSI_CONTROL pc_canvas )
{ 
	if( l.lvAddToken )
	{
		snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "Please select a token and a permission group to give the token to and press add group token." );
		l.sAddToken = l.cAddToken;	
		LabelVariableChanged( l.lvAddToken );
	}

	if( l.token_list )
		ClearSelectedItems( l.token_list );

	if( l.selected_token )
		l.selected_token = NULL;

	if( l.perm_group_list )
		ClearSelectedItems( l.perm_group_list );

	if( l.selected_perm_group )
		l.selected_perm_group = NULL;	

	 return 1;
}


//--------------------------------------------------------------------------------
// Add Group Token
static uintptr_t OnCreateMenuButton( "SQL Users/Add Group Token" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Add Group Token" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Add Group Token" )( uintptr_t psv )
{
	TEXTCHAR buf[256];
	int token_id;
	int group_id;
	CTEXTSTR *result;		

	if( l.selected_token && l.selected_perm_group )
	{
		l.token = (PTOKEN)GetItemData( l.selected_token );	
		l.group = (PGROUP)GetItemData( l.selected_perm_group );

		DoSQLRecordQueryf( NULL, &result, NULL, "select permission_id from permission_tokens where name='%s'", l.token->name );		
		if( result && atoi( result[0] ) > 0 )
		{
			token_id = atoi( result[0] );

			DoSQLRecordQueryf( NULL, &result, NULL, "select permission_group_id from permission_group where name='%s'", l.group->name );
			if( result && atoi( result[0] ) > 0 )
			{
				group_id = atoi( result[0] );

				DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_set where permission_group_id=%d and permission_id=%d", group_id, token_id );
				if( result && atoi( result[0] ) > 0 )
				{
					snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "The token has already been given to this group." );
					l.sAddToken = l.cAddToken;	
					LabelVariableChanged( l.lvAddToken );
				}

				else
				{
					DoSQLCommandf( "insert into permission_set (permission_group_id,permission_id) values (%d,%d)", group_id, token_id );

					snprintf( buf, 256, " Token: %s was added to Group: %s by User: %s Name: %s", l.token->name, l.group->name, l.logee_id, l.logee_name ); 		
					DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.logee_id, g.system_id, l.program_id, buf );

					snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "The token was successfully given to the group." );
					l.sAddToken = l.cAddToken;	
					LabelVariableChanged( l.lvAddToken );

					ReloadUserCache( NULL );
				}
			}

			else
			{
				snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "Could not find permission group id." );
				l.sAddToken = l.cAddToken;	
				LabelVariableChanged( l.lvAddToken );
			}
		}

		else
		{
			snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "Could not find permission id." );
			l.sAddToken = l.cAddToken;	
			LabelVariableChanged( l.lvAddToken );
		}
	}

	else if( l.selected_perm_group )
	{
		snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "No Token Selected. Please try again." );
		l.sAddToken = l.cAddToken;	
		LabelVariableChanged( l.lvAddToken );
	}

	else if( l.selected_token )
	{
		snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
		l.sAddToken = l.cAddToken;	
		LabelVariableChanged( l.lvAddToken );
	}
	else
	{
		snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "No Token or Group Selected. Please try again." );
		l.sAddToken = l.cAddToken;	
		LabelVariableChanged( l.lvAddToken );
	}
}

//--------------------------------------------------------------------------------
// Remove Group Token Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Remove Group Token" )( PSI_CONTROL pc_canvas )
{ 
	l.stage = 0;

	if( l.lvRemToken )
	{
		snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "Please select a permission group and press remove group token." );
		l.sRemToken = l.cRemToken;	
		LabelVariableChanged( l.lvRemToken );
	}
	
	if( l.perm_group_list )
		ClearSelectedItems( l.perm_group_list );

	if( l.selected_perm_group )
		l.selected_perm_group = NULL;

	if( l.token_list2 )
		ResetList( l.token_list2 );

	//if( l.token_list2 )
		//ClearSelectedItems( l.token_list2 );

	if( l.selected_token2 )
		l.selected_token2 = NULL;

	 return 1;
}


//--------------------------------------------------------------------------------
// Remove Group Token
static uintptr_t OnCreateMenuButton( "SQL Users/Remove Group Token" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Remove Group Token" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Remove Group Token" )( uintptr_t psv )
{
	TEXTCHAR buf[256];
	int token_id;
	int group_id;
	CTEXTSTR *result;

	if( l.stage == 0 )
	{
		if( l.selected_perm_group )
		{
			l.group = (PGROUP)GetItemData( l.selected_perm_group );
			FillTokenList2();

			snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "Please select a token to be removed and press remove group token again." );
			l.sRemToken = l.cRemToken;	
			LabelVariableChanged( l.lvRemToken );
			l.stage++;
		}

		else
		{
			snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
			l.sRemToken = l.cRemToken;	
			LabelVariableChanged( l.lvRemToken );
		}

	}
	
	else if( l.stage == 1 )
	{
		l.selected_token2 = GetSelectedItem( l.token_list2 );

		if( l.selected_token2 )
		{				
			l.token2 = (PTOKEN)GetItemData( l.selected_token2 );

			DoSQLRecordQueryf( NULL, &result, NULL, "select permission_id from permission_tokens where name='%s'", l.token2->name );		
			if( result && atoi( result[0] ) > 0 )
			{
				token_id = atoi( result[0] );

				DoSQLRecordQueryf( NULL, &result, NULL, "select permission_group_id from permission_group where name='%s'", l.group->name );
				if( result && atoi( result[0] ) > 0 )
				{
					group_id = atoi( result[0] );
					DoSQLCommandf( "delete from permission_set where permission_group_id=%d and permission_id=%d", group_id, token_id );

					snprintf( buf, 256, " Token: %s was removed from Group: %s by User: %s Name: %s", l.token2->name, l.group->name, l.logee_id, l.logee_name ); 		
					DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.logee_id, g.system_id, l.program_id, buf );

					snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "The token was successfully removed from the group." );
					l.sRemToken = l.cRemToken;	
					LabelVariableChanged( l.lvRemToken );

					ReloadUserCache( NULL );
				}

				else
				{
					snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "Could not find permission group id." );
					l.sRemToken = l.cRemToken;	
					LabelVariableChanged( l.lvRemToken );

				}
			}

			else
			{
				snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "Could not find permission id." );
				l.sRemToken = l.cRemToken;	
				LabelVariableChanged( l.lvRemToken );
			}			

			l.stage--;
		}

		else
		{
			snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "No Token Selected. Please try again." );
			l.sRemToken = l.cRemToken;	
			LabelVariableChanged( l.lvRemToken );
		}
	}	
}

//--------------------------------------------------------------------------------
// Create Group Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Create Group" )( PSI_CONTROL pc_canvas )
{
	l.stage = 0;	

	if( l.lvCreateGroup )
	{		
		snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "Please enter a group name and press enter." );
		l.sCreateGroup = l.cCreateGroup;	
		LabelVariableChanged( l.lvCreateGroup );	
	}	

	 return 1;
}

//--------------------------------------------------------------------------------
// Create Group Keypad
static void OnKeypadEnterType( "create group", "Create Group Keypad" )( PSI_CONTROL pc_keypad )
{	
	TEXTCHAR buf[256];
	CTEXTSTR *result;		  // Used for queries	
	
	if( l.stage == 0 )			
	{	
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.group_name, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len )
		{
			snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "Please enter the group name again and press enter." );
			l.sCreateGroup = l.cCreateGroup;	
			LabelVariableChanged( l.lvCreateGroup );
			l.stage++;
		}

		else
		{							
			snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "No group name was entered. Please try again." );
			l.sCreateGroup = l.cCreateGroup;	
			LabelVariableChanged( l.lvCreateGroup );
		}
	}

	else if( l.stage == 1 )			
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.group_name2, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len2 )
		{
			if( StrCmp( l.group_name, l.group_name2 ) == 0 )
			{
				snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "Please enter a short description for the group and press enter." );
				l.sCreateGroup = l.cCreateGroup;	
				LabelVariableChanged( l.lvCreateGroup );	
				l.stage++;						
			}		

			else
			{			
				snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "The group names entered do not match. Please try again." );
				l.sCreateGroup = l.cCreateGroup;	
				LabelVariableChanged( l.lvCreateGroup );	
				l.stage--;
			}
		}

		else
		{			
			snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "No group name was entered. Please try again." );
			l.sCreateGroup = l.cCreateGroup;	
			LabelVariableChanged( l.lvCreateGroup );	
		}
	}

	else if( l.stage == 2 )	
	{
		TEXTSTR tmp1, tmp2;
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.group_description, 255 );
		ClearKeyedEntry( pc_keypad );
		tmp1 = EscapeSQLString( NULL, l.group_name );
		tmp2 = EscapeSQLString( NULL, l.group_description );
		
		if( l.password_len )
		{
			DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_group where name='%s'", tmp1 );
			if( result && atoi( result[0] ) > 0 )
			{					
				snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "The group name is already in use. Please try again." );
				l.sCreateGroup = l.cCreateGroup;	
				LabelVariableChanged( l.lvCreateGroup );				
			}

			else
			{
				DoSQLCommandf( "insert into permission_group (name,dummy_timestamp,description) values ('%s',now(),'%s')"
					, tmp1
					, tmp2 );
				snprintf( buf, 256, " Group: %s was created by User: %s Name: %s", l.group_name, l.logee_id, l.logee_name ); 		
				DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.logee_id, g.system_id, l.program_id, EscapeString( buf ) );

				snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "The group was successfully created." );
				l.sCreateGroup = l.cCreateGroup;	
				LabelVariableChanged( l.lvCreateGroup );

				ReloadUserCache( NULL );
			}

			l.stage = 0;
		}

		else
		{
			snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "No description was entered. Please try again." );
			l.sCreateGroup = l.cCreateGroup;	
			LabelVariableChanged( l.lvCreateGroup );
		}
		Release( tmp1 );
		Release( tmp2 );
	}
}

//--------------------------------------------------------------------------------
// Delete Group Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Delete Group" )( PSI_CONTROL pc_canvas )
{	

	if( l.lvDeleteGroup )
	{		
		snprintf( l.cDeleteGroup, MESSAGE_SIZE, "%s", "Please select a group and press delete group." );
		l.sDeleteGroup = l.cDeleteGroup;	
		LabelVariableChanged( l.lvDeleteGroup );	
	}	

	if( l.perm_group_list )
		ClearSelectedItems( l.perm_group_list );

	if( l.selected_perm_group )
		l.selected_perm_group = NULL;	

	 return 1;
}

//--------------------------------------------------------------------------------
// Delete Group Button
static uintptr_t OnCreateMenuButton( "SQL Users/Delete Group" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Delete Group" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Delete Group" )( uintptr_t psv ){
		
	TEXTCHAR buf[256];

	if( l.selected_perm_group )
	{
		l.group = (PGROUP)GetItemData( l.selected_perm_group );
		{
			DoSQLCommandf( "delete from permission_group where permission_group_id=%d", l.group->id );
			DoSQLCommandf( "delete from permission_user where permission_group_id=%d", l.group->id );

			snprintf( buf, 256, " Group: %s was deleted by User: %s Name: %s", l.group->name, l.logee_id, l.logee_name ); 		
			DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.logee_id, g.system_id, l.program_id, buf );

			snprintf( l.cDeleteGroup, MESSAGE_SIZE, "%s", "The selected group was deleted successfully." );
			l.sDeleteGroup = l.cDeleteGroup;	
			LabelVariableChanged( l.lvDeleteGroup );

			ReloadUserCache( NULL );
		}
	}

	else
	{		
		snprintf( l.cDeleteGroup, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
		l.sDeleteGroup = l.cDeleteGroup;	
		LabelVariableChanged( l.lvDeleteGroup );
	}	
}

//--------------------------------------------------------------------------------
// Create Token Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Create Token" )( PSI_CONTROL pc_canvas )
{
	l.stage = 0;	

	if( l.lvCreateToken )
	{		
		snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "Please enter a token name and press enter." );
		l.sCreateToken = l.cCreateToken;	
		LabelVariableChanged( l.lvCreateToken );	
	}	

	 return 1;
}

//--------------------------------------------------------------------------------
// Create Token Keypad
static void OnKeypadEnterType( "create token", "Create Token Keypad" )( PSI_CONTROL pc_keypad )
{
	CTEXTSTR *result;		  // Used for queries								  		
	
	if( l.stage == 0 )			
	{	
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.token_name, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len )
		{
			snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "Please enter the token name again and press enter." );
			l.sCreateToken = l.cCreateToken;	
			LabelVariableChanged( l.lvCreateToken );
			l.stage++;
		}

		else
		{							
			snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "No token name was entered. Please try again." );
			l.sCreateToken = l.cCreateToken;	
			LabelVariableChanged( l.lvCreateToken );
		}
	}

	else if( l.stage == 1 )			
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.token_name2, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len2 )
		{
			if( StrCmp( l.token_name, l.token_name2 ) == 0 )
			{
				snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "Please enter a short description for the token and press enter." );
				l.sCreateToken = l.cCreateToken;	
				LabelVariableChanged( l.lvCreateToken );	
				l.stage++;							
			}		

			else
			{			
				snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "The token names entered do not match. Please try again." );
				l.sCreateToken = l.cCreateToken;	
				LabelVariableChanged( l.lvCreateToken );	
				l.stage--;
			}
		}

		else
		{			
			snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "No token name was entered. Please try again." );
			l.sCreateToken = l.cCreateToken;	
			LabelVariableChanged( l.lvCreateToken );	
		}
	}

	else if( l.stage == 2 )	
	{
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.token_description, 255 );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len )
		{
			DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_tokens where name='%s'", l.token_name );
			if( result && atoi( result[0] ) > 0 )
			{					
				snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "The token name is already in use. Please try again." );
				l.sCreateToken = l.cCreateToken;	
				LabelVariableChanged( l.lvCreateToken );
				l.stage = 0;
			}

			else
			{
				CreateToken( l.token_name, l.token_description );

				snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "The token was successfully created." );
				l.sCreateToken = l.cCreateToken;	
				LabelVariableChanged( l.lvCreateToken );

				l.stage = 0;
				//ReloadUserCache( NULL );
			}
		}
		
		else
		{			
			snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "No description was entered. Please try again." );
			l.sCreateToken = l.cCreateToken;	
			LabelVariableChanged( l.lvCreateToken );	
		}
	}
}

//--------------------------------------------------------------------------------
// Delete Token Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Delete Token" )( PSI_CONTROL pc_canvas )
{
	l.stage = 0;	

	if( l.lvDeleteToken )
	{		
		snprintf( l.cDeleteToken, MESSAGE_SIZE, "%s", "Please select a token and press delete token." );
		l.sDeleteToken = l.cDeleteToken;	
		LabelVariableChanged( l.lvDeleteToken );	
	}

	if( l.token_list )
		ClearSelectedItems( l.token_list );

	if( l.selected_token )
		l.selected_token = NULL;	

	 return 1;
}

//--------------------------------------------------------------------------------
// Delete Token Button
static uintptr_t OnCreateMenuButton( "SQL Users/Delete Token" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Delete Token" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Delete Token" )( uintptr_t psv ){
	
	TEXTCHAR buf[256];
	int token_id;
	CTEXTSTR *result;	

	if( l.selected_token )
	{
		l.token = (PTOKEN)GetItemData( l.selected_token );

		DoSQLRecordQueryf( NULL, &result, NULL, "select permission_id from permission_tokens where name='%s'", l.token->name );
		if( result && atoi( result[0] ) > 0 )
		{
			token_id = atoi( result[0] );
			DoSQLCommandf( "delete from permission_tokens where name='%s'", l.token->name );
			DoSQLCommandf( "delete from permission_set where permission_id=%d", token_id );

			snprintf( buf, 256, " Token: %s was deleted by User: %s Name: %s", l.token->name, l.logee_id, l.logee_name ); 		
			DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", l.logee_id, g.system_id, l.program_id, buf );

			snprintf( l.cDeleteToken, MESSAGE_SIZE, "%s", "The selected token was deleted successfully." );
			l.sDeleteToken = l.cDeleteToken;	
			LabelVariableChanged( l.lvDeleteToken );

			ReloadUserCache( NULL );
		}

		else
		{
			snprintf( l.cDeleteToken, MESSAGE_SIZE, "%s", "Could not find permission id." );
			l.sDeleteToken = l.cDeleteToken;	
			LabelVariableChanged( l.lvDeleteToken );
		}		
	}

	else
	{		
		snprintf( l.cDeleteToken, MESSAGE_SIZE, "%s", "No Token Selected. Please try again." );
		l.sDeleteToken = l.cDeleteToken;	
		LabelVariableChanged( l.lvDeleteToken );
	}	
}

//--------------------------------------------------------------------------------
// Create User Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Create User" )( PSI_CONTROL pc_canvas )
{
	l.stage = 0;	

	if( l.lvCreateUser )
	{		
		snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter a staff id and press enter." );
		l.sCreateUser = l.cCreateUser;	
		LabelVariableChanged( l.lvCreateUser );	
	}

	if( l.perm_group_list )
		ClearSelectedItems( l.perm_group_list );

	if( l.selected_perm_group )
		l.selected_perm_group = NULL;	

	 return 1;
}

//--------------------------------------------------------------------------------
// Create User Keypad
static void OnKeypadEnterType( "create user", "Create User Keypad" )( PSI_CONTROL pc_keypad )
{		
	TEXTCHAR buf[256];			  // Used for making descriptions	
	CTEXTSTR *result;		  // Used for queries
	int user_id = 0;			// Need to get on creation for other entries
	int group_id = 0;		  // Need to get on creation for other entries	
	
	if( l.stage == 0 )			
	{	
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.staff_id, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len )
		{
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter a staff id again and press enter." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
			l.stage++;
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No staff id was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 1 )			
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.staff_id2, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len2 )
		{
			if( StrCmp( l.staff_id, l.staff_id2 ) == 0 )
			{
				DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_user_info where staff_id='%s'", l.staff_id );
				if( result && atoi( result[0] ) > 0 )
				{
					snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The staff id is already in use. Please try again." );
					l.sCreateUser = l.cCreateUser;	
					LabelVariableChanged( l.lvCreateUser );	
					l.stage--;
				}

				else
				{
					snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter the user's first name and press enter." );
					l.sCreateUser = l.cCreateUser;	
					LabelVariableChanged( l.lvCreateUser );
					l.stage++;
				}			
			}		

			else
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The staff ids entered do not match. Please try again." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );	
				l.stage--;
			}
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No staff id was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 2 )
	{
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.first_name, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len )
		{
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter the user's first name again and press enter." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
			l.stage++;
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No first name was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 3 )
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.first_name2, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len2 )
		{
			if( StrCmp( l.first_name, l.first_name2 ) == 0 )
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter the user's last name and press enter." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );
				l.stage++;			
			}		

			else
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The first names entered do not match. Please try again." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );	
				l.stage--;
			}
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No first name was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 4 )
	{
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.last_name, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len )
		{
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter the user's last name again and press enter." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
			l.stage++;
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No last name was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 5 )
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.last_name2, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len2 )
		{
			if( StrCmp( l.last_name, l.last_name2 ) == 0 )
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter a user name and press enter." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );
				l.stage++;			
			}		

			else
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The last names entered do not match. Please try again." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );	
				l.stage--;
			}
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No last name was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}
	
	else if( l.stage == 6 )
	{
		l.password_len = 0;
		l.password_len = GetKeyedText( pc_keypad, l.user_name, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len )
		{
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter the user name again and press enter." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
			l.stage++;
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No user name was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 7 )			
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.user_name2, NAME_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );
		
		if( l.password_len2 )
		{
			if( StrCmp( l.user_name, l.user_name2 ) == 0 )
			{
				DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_user_info where name='%s'", l.user_name );
				if( result && atoi( result[0] ) > 0 )
				{
					snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The user name is already in use. Please try again." );
					l.sCreateUser = l.cCreateUser;	
					LabelVariableChanged( l.lvCreateUser );	
					l.stage--;
				}

				else
				{
					snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please select a user group and press enter." );
					l.sCreateUser = l.cCreateUser;	
					LabelVariableChanged( l.lvCreateUser );
					l.stage++;
				}			
			}		

			else
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The user names entered do not match. Please try again." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );	
				l.stage--;
			}
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No user name was entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 8 )
	{
		if( l.selected_perm_group )
		{				
			l.group = (PGROUP)GetItemData( l.selected_perm_group );

			DoSQLRecordQueryf( NULL, &result, NULL, "select permission_group_id from permission_group where name='%s'", l.group->name );
			if( result && atoi( result[0] ) > 0 )
			{
				group_id = atoi( result[0] );
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter a password and press enter." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );
				l.stage++;
			}

			else
			{
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Permission group id could not be found. Please try again." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );
			}			
		}
		
		else
		{
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}

	else if( l.stage == 9 )
	{	
		TEXTCHAR *temp;						
		TEXTCHAR bad = 0;
		TEXTCHAR uppercase_used = 0;
		TEXTCHAR lowercase_used = 0;
		TEXTCHAR num_used = 0;
		TEXTCHAR spec_used = 0;

		l.password_len = 0;			
		l.password_len = GetKeyedText( pc_keypad, l.password, PASSCODE_BUFFER_SIZE );			
		
		if( l.password_len )
		{
			if( l.password_len >= l.pass_min_length )
			{
				if( l.flags.does_pass_req_upper )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 64 && *temp < 91 )
							uppercase_used = 1;
					if( !uppercase_used )
					{
						bad = 1;						
						snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "An upper case character is required. Please try again." );
						l.sCreateUser = l.cCreateUser;	
						LabelVariableChanged( l.lvCreateUser );
					}
				}

				if( l.flags.does_pass_req_lower && !bad )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 96 && *temp < 123 )
							lowercase_used = 1;
					if( !lowercase_used )
					{
						bad = 1;						
						snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "A lower case character is required. Please try again." );
						l.sCreateUser = l.cCreateUser;	
						LabelVariableChanged( l.lvCreateUser );
					}
				}

				if( l.flags.does_pass_req_num && !bad )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 47 && *temp < 58 )
							num_used = 1;
					if( !num_used )
					{
						bad = 1;						
						snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "A numeric character is required. Please try again." );
						l.sCreateUser = l.cCreateUser;	
						LabelVariableChanged( l.lvCreateUser );
					}
				}

				if( l.flags.does_pass_req_spec && !bad )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( ( *temp > 32 && *temp < 48 ) || ( *temp > 57 && *temp < 65 ) || ( *temp > 90 && *temp < 97 ) || ( *temp > 122 && *temp < 127 ) )
							spec_used = 1;
					if( !spec_used )
					{
						bad = 1;						
						snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "A special character is required. Please try again." );
						l.sCreateUser = l.cCreateUser;	
						LabelVariableChanged( l.lvCreateUser );
					}
				}	

				if( !bad )
				{			
					snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please repeat password and press okay again." );
					l.sCreateUser = l.cCreateUser;	
					LabelVariableChanged( l.lvCreateUser );
					l.stage++;
				}
			
			}

			else
			{							
				snprintf( l.cCreateUser, MESSAGE_SIZE, "Passwords must be at least %d characters in length.", l.pass_min_length );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );
			}
		}

		else
		{							
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
		
	}
	// Get Second Entry
	else if( l.stage == 10 )
	{
		l.password_len2 = 0;
		l.password_len2 = GetKeyedText( pc_keypad, l.password2, PASSCODE_BUFFER_SIZE );
		ClearKeyedEntry( pc_keypad );

		if( l.password_len2 )
		{		
			if( StrCmp( l.password, l.password2 ) == 0 )
			{
				SHA1Context sc;				
				TEXTCHAR sha1[SHA1HashSize]; 	
				SHA1Reset( &sc );
				SHA1Input( &sc, (uint8_t*)l.password, strlen( l.password ) );
				SHA1Result( &sc, (uint8_t*)sha1 );
				ConvertFromBinary( l.out, (uint8_t*)sha1, SHA1HashSize );

				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The user was successfully created." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );
				l.stage++;			
			}		

			else
			{			
				snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "The passwords entered do not match. Please try again." );
				l.sCreateUser = l.cCreateUser;	
				LabelVariableChanged( l.lvCreateUser );	
				l.stage--;
			}
		}

		else
		{
			snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "No Password Entered. Please try again." );
			l.sCreateUser = l.cCreateUser;	
			LabelVariableChanged( l.lvCreateUser );
		}
	}		
	
	if( l.stage == 11 )
	{
		//Insert collected info into permission_user_info
		DoSQLCommandf( "insert into permission_user_info (default_room_id,first_name,last_name,name,staff_id,password,password_creation_datestamp) values (%d,'%s','%s','%s','%s','%s',now())"
			, l.default_room_id, l.first_name, l.last_name, l.user_name, l.staff_id, l.out );

		//Get user id for insertion into 
		DoSQLRecordQueryf( NULL, &result, NULL, "select user_id from permission_user_info where staff_id='%s'", l.staff_id );
		if( result && atoi( result[0] ) > 0 )
			user_id = atoi( result[0] );

		//Insert collected info into permission_user
		DoSQLCommandf( "insert into permission_user (permission_group_id,user_id) values (%d,%d)", group_id, user_id );
				
		//Insert collected info into permission_user_password
		snprintf( buf, 256, " User: %s Name: %s was created by User: %s Name: %s",  user_id, l.user_name, l.logee_id, l.logee_name ); 
		DoSQLCommandf( "insert into permission_user_password (user_id,password,description,creation_datestamp) values (%d,'%s','%s',now())", user_id, l.out, buf );			
		DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", user_id, g.system_id, l.program_id, buf );

		ReloadUserCache( NULL );
	}			
}

//--------------------------------------------------------------------------------
// Create User 2 Page
//--------------------------------------------------------------------------------
static int OnChangePage( "Create User 2" )( PSI_CONTROL pc_canvas )
{
	snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "Please enter all fields, select a permission group and press create user." );
	LabelVariableChanged( l.lvCreateUser2 );	

	if( l.perm_group_list )
		ClearSelectedItems( l.perm_group_list );

	if( l.selected_perm_group )
		l.selected_perm_group = NULL;

	SetControlText( l.pc_staff_id, NULL );
	SetControlText( l.pc_first_name, NULL );
	SetControlText( l.pc_last_name, NULL );
	SetControlText( l.pc_user_name, NULL );
	SetControlText( l.pc_password, NULL );
	SetControlText( l.pc_password2, NULL );	
	
	 return 1;
}


//--------------------------------------------------------------------------------
// Staff ID
static uintptr_t OnCreateControl( "SQL Users/Create User Form/Staff ID" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, EDIT_FIELD_NAME, x, y, w, h, -1 );
	l.pc_staff_id = pc;

	return (uintptr_t) pc;
}

static PSI_CONTROL OnGetControl( "SQL Users/Create User Form/Staff ID" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

//--------------------------------------------------------------------------------
// First Name
static uintptr_t OnCreateControl( "SQL Users/Create User Form/First Name" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, EDIT_FIELD_NAME, x, y, w, h, -1 );
	l.pc_first_name = pc;

	return (uintptr_t) pc;
}

static PSI_CONTROL OnGetControl( "SQL Users/Create User Form/First Name" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

//--------------------------------------------------------------------------------
// Last Name
static uintptr_t OnCreateControl( "SQL Users/Create User Form/Last Name" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, EDIT_FIELD_NAME, x, y, w, h, -1 );
	l.pc_last_name = pc;

	return (uintptr_t) pc;
}

static PSI_CONTROL OnGetControl( "SQL Users/Create User Form/Last Name" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

//--------------------------------------------------------------------------------
// User Name
static uintptr_t OnCreateControl( "SQL Users/Create User Form/User Name" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, EDIT_FIELD_NAME, x, y, w, h, -1 );
	l.pc_user_name = pc;

	return (uintptr_t) pc;
}

static PSI_CONTROL OnGetControl( "SQL Users/Create User Form/User Name" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

//--------------------------------------------------------------------------------
// Password1
static uintptr_t OnCreateControl( "SQL Users/Create User Form/Password 1" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, EDIT_FIELD_NAME, x, y, w, h, -1 );
	l.pc_password = pc;
	SetEditControlPassword( pc, TRUE );

	return (uintptr_t) pc;
}

static PSI_CONTROL OnGetControl( "SQL Users/Create User Form/Password 1" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

//--------------------------------------------------------------------------------
// Password2
static uintptr_t OnCreateControl( "SQL Users/Create User Form/Password 2" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( parent, EDIT_FIELD_NAME, x, y, w, h, -1 );
	l.pc_password2 = pc;
	SetEditControlPassword( pc, TRUE );

	return (uintptr_t) pc;
}

static PSI_CONTROL OnGetControl( "SQL Users/Create User Form/Password 2" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

//--------------------------------------------------------------------------------
// Accept Generic
static uintptr_t OnCreateMenuButton( "SQL Users/Create User Form/Create User" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Create User" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Create User Form/Create User" )( uintptr_t psv )
{
	TEXTCHAR buf[256];			  // Used for making descriptions	
	CTEXTSTR *result;		  // Used for queries
	int user_id = 0;			// Need to get on creation for other entries
	//int group_id = 0;		 // Need to get on creation for other entries
	int bad_entry = 0;

	// Handles getting permission group info
	if( l.selected_perm_group )
	{				
		l.group = (PGROUP)GetItemData( l.selected_perm_group );

		/* l.group->id
		DoSQLRecordQueryf( NULL, &result, NULL, "select permission_group_id from permission_group where name='%s'", l.group->name );
		if( result && atoi( result[0] ) > 0 )
		{
			group_id = atoi( result[0] );						
		}

		else
		{
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "Permission group id could not be found. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}	
		*/
	}
		
	else
	{
		snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "No Group Selected. Please try again." );
		LabelVariableChanged( l.lvCreateUser2 );
		bad_entry = 1;
	}	

	// Handles getting staff id
	if( !bad_entry )
	{
		GetControlText( l.pc_staff_id, l.staff_id, NAME_BUFFER_SIZE ); 

		if( strlen( l.staff_id ) )
		{
			DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_user_info where staff_id='%s'", l.staff_id );
			if( result && atoi( result[0] ) > 0 )
			{
				snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "The staff id is already in use. Please try again." );
				LabelVariableChanged( l.lvCreateUser2 );	
				bad_entry = 1;
			}			
		}

		else
		{							
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "No staff id was entered. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}
	}

	// Handles getting first name
	if( !bad_entry )
	{
		GetControlText( l.pc_first_name, l.first_name, NAME_BUFFER_SIZE ); 

		if( !strlen( l.first_name ) )		
		{							
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "No first name was entered. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}
	}

	// Handles getting last name
	if( !bad_entry )
	{
		GetControlText( l.pc_last_name, l.last_name, NAME_BUFFER_SIZE ); 

		if( !strlen( l.last_name ) )		
		{							
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "No last name was entered. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}
	}

	// Handles getting user name
	if( !bad_entry )
	{
		GetControlText( l.pc_user_name, l.user_name, NAME_BUFFER_SIZE ); 

		if( strlen( l.user_name ) )
		{			
			DoSQLRecordQueryf( NULL, &result, NULL, "select count(*) from permission_user_info where name='%s'", l.user_name );
			if( result && atoi( result[0] ) > 0 )
			{
				snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "The user name is already in use. Please try again." );
				LabelVariableChanged( l.lvCreateUser2 );	
				bad_entry = 1;
			}			
		}

		else
		{							
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "No user name was entered. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}
	}

	// Handles getting first password
	if( !bad_entry )
	{
		TEXTCHAR *temp;		
		TEXTCHAR uppercase_used = 0;
		TEXTCHAR lowercase_used = 0;
		TEXTCHAR num_used = 0;
		TEXTCHAR spec_used = 0;

		GetControlText( l.pc_password, l.password, PASSCODE_BUFFER_SIZE ); 

		if( l.password_len = strlen( l.password ) )
		{
			if( l.password_len >= l.pass_min_length )
			{
				if( l.flags.does_pass_req_upper )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 64 && *temp < 91 )
							uppercase_used = 1;

					if( !uppercase_used )
					{												
						snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "An upper case character is required. Please try again." );
						LabelVariableChanged( l.lvCreateUser2 );
						bad_entry = 1;
					}
				}

				if( l.flags.does_pass_req_lower && !bad_entry )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 96 && *temp < 123 )
							lowercase_used = 1;

					if( !lowercase_used )
					{												
						snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "A lower case character is required. Please try again." );
						LabelVariableChanged( l.lvCreateUser2 );
						bad_entry = 1;
					}
				}

				if( l.flags.does_pass_req_num && !bad_entry )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( *temp > 47 && *temp < 58 )
							num_used = 1;

					if( !num_used )
					{												
						snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "A numeric character is required. Please try again." );
						LabelVariableChanged( l.lvCreateUser2 );
						bad_entry = 1;
					}
				}

				if( l.flags.does_pass_req_spec && !bad_entry )
				{
					for( temp = l.password; *temp != '\0'; temp++ )
						if( ( *temp > 32 && *temp < 48 ) || ( *temp > 57 && *temp < 65 ) || ( *temp > 90 && *temp < 97 ) || ( *temp > 122 && *temp < 127 ) )
							spec_used = 1;

					if( !spec_used )
					{												
						snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "A special character is required. Please try again." );
						LabelVariableChanged( l.lvCreateUser2 );
						bad_entry = 1;
					}
				}			
			}

			else
			{							
				snprintf( l.cCreateUser2, MESSAGE_SIZE, "Passwords must be at least %d characters in length.", l.pass_min_length );
				LabelVariableChanged( l.lvCreateUser2 );
				bad_entry = 1;
			}
		}
		
		else
		{							
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "A password was not entered. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}
	}

	// Handles getting first password
	if( !bad_entry )
	{
		GetControlText( l.pc_password2, l.password2, PASSCODE_BUFFER_SIZE ); 

		if( strlen( l.password2 ) )
		{
			if( StrCmp( l.password, l.password2 ) == 0 )
			{
				SHA1Context sc;				
				TEXTCHAR sha1[SHA1HashSize]; 	
				SHA1Reset( &sc );
				SHA1Input( &sc, (uint8_t*)l.password, strlen( l.password ) );
				SHA1Result( &sc, (uint8_t*)sha1 );
				ConvertFromBinary( l.out, (uint8_t*)sha1, SHA1HashSize );

				snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "The user was successfully created." );
				LabelVariableChanged( l.lvCreateUser2 );							
			}		

			else
			{			
				snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "The passwords entered do not match. Please try again." );
				LabelVariableChanged( l.lvCreateUser2 );	
				bad_entry = 1;
			}
		}

		else
		{							
			snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "A password was not entered. Please try again." );
			LabelVariableChanged( l.lvCreateUser2 );
			bad_entry = 1;
		}
	}

	if( !bad_entry )
	{
		//Insert collected info into permission_user_info
		DoSQLCommandf( "insert into permission_user_info (hall_id,charity_id,default_room_id,first_name,last_name,name,staff_id,password,password_creation_datestamp) values (%d,%d,%d,'%s','%s','%s','%s','%s',now())"
			, l.default_room_id, l.first_name, l.last_name, l.user_name, l.staff_id, l.out );

		//Get user id for insertion into 
		DoSQLRecordQueryf( NULL, &result, NULL, "select user_id from permission_user_info where staff_id='%s'", l.staff_id );
		if( result && atoi( result[0] ) > 0 )
			user_id = atoi( result[0] );

		//user_id = GetLastInsertID( "permission_user_info", "system_id" );

		//Insert collected info into permission_user
		DoSQLCommandf( "insert into permission_user (permission_group_id,user_id) values (%d,%d)", l.group->id, user_id );
				
		//Insert collected info into permission_user_password
		snprintf( buf, 256, " User: %s Name: %s was created by User: %s Name: %s",  user_id, l.user_name, l.logee_id, l.logee_name ); 
		DoSQLCommandf( "insert into permission_user_password (user_id,password,description,creation_datestamp) values (%d,'%s','%s',now())", user_id, l.out, buf );			
		DoSQLCommandf( "insert into system_exceptions (system_exception_category_id,system_exception_type_id,user_id,system_id,program_id,description,log_whenstamp) values (2,0,%d,%d,%d,'%s',now())", user_id, g.system_id, l.program_id, buf );

		ReloadUserCache( NULL );
	}	
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// User Report
static uintptr_t OnCreateMenuButton( "SQL Users/User Report" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "User Report" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/User Report" )( uintptr_t psv )
{
#if 0
	HDC printer = GetPrinterDC(1);
	CTEXTSTR result;
	int n;
	TEXTCHAR szString[256];
	struct {
		uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	} g;
	uint32_t now = CAL_GET_FDATETIME();

	// Get User
	ShellSetCurrentPageEx( l.frame, "Select User" );
	RevealCommon( l.frame );
	SetCommonFocus( l.frame );

	l.done = l.okay = 0;			
	CommonLoop( &l.done, &l.okay );	
	HideControl( l.frame );

	FontFromColumns( printer, NULL, NULL, 100, NULL );
	//ReadPasswordFile();
	ClearReportHeaders();
	CAL_P_YMDHMS_OF_FDATETIME( now, &g.wYr, &g.wMo, &g.wDy, &g.wHr, &g.wMn, &g.wSc );
	snprintf( szString, sizeof( szString ), "Active User Report"
			  , g.wMo, g.wDy, g.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), "Printed at %d:%02d:%02d on %02d/%02d/%02d"
			  , g.wHr, g.wMn, g.wSc
			  , g.wMo, g.wDy, g.wYr%100);
	AddReportHeader( szString );
	AddReportHeader( "" );
	
	AddReportHeader( "User Name				Group				 Updated	  Expires	  Created" );
	//					"20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
	AddReportHeader( "____________________ _________________ ___________ ___________ ___________\n" );	
	
	PrintReportHeader( printer, -1, -1 ); // current position, write haeder...
	for( n = 0; n < PASSCODE_BUFFER_SIZE; n++ )
	{
		int m;
		struct {
			uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
		} expire;
		struct { 
			uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
		} created;
		struct { 
			uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
		} password_updated;
		TEXTCHAR buf[256];
		if( l.user2->name )
		{
			INDEX idx;
			PGROUP group;			
			TEXTCHAR *date;
			TEXTCHAR datebuf[13];
			TEXTCHAR *exp_date;
			TEXTCHAR exp_datebuf[13];
			TEXTCHAR *upd_date;
			TEXTCHAR upd_datebuf[13];
			uint32_t now = CAL_GET_FDATETIME();
			if( l.user2->dwFutTime > now )
			{
				//CAL_P_YMDHMS_OF_FDATETIME( l.user2->dwFutTime, &expire.wYr, &expire.wMo, &expire.wDy, &expire.wHr, &expire.wMn, &expire.wSc );
				//snprintf( exp_datebuf, sizeof( exp_datebuf ), "%02d/%02d/%04d" 
				//		, expire.wMo, expire.wDy, expire.wYr );
				DoSQLQueryf( &result, "select now()" );
				snprintf( exp_datebuf, sizeof( exp_datebuf ), "%s" 
						, result );
				exp_date = exp_datebuf;
			}
			else
				exp_date = "Expired	";
			if( l.user2->dwFutTime_Created )
			{
				//CAL_P_YMDHMS_OF_FDATETIME( l.user2->dwFutTime_Created, &created.wYr, &created.wMo, &created.wDy, &created.wHr, &created.wMn, &created.wSc );
				//snprintf( datebuf, sizeof( datebuf ), "%02d/%02d/%04d" 
				//		, created.wMo, created.wDy, created.wYr );
				DoSQLQueryf( &result, "select now()" );
				snprintf( exp_datebuf, sizeof( exp_datebuf ), "%s" 
						, result );
				date = datebuf;
			}
			else
				date = "Before Now";
			if( l.user2->dwFutTime_Updated_Password )
			{
				//CAL_P_YMDHMS_OF_FDATETIME( l.user2->dwFutTime_Updated_Password, &password_updated.wYr, &password_updated.wMo, &password_updated.wDy, &password_updated.wHr, &password_updated.wMn, &password_updated.wSc );
				//snprintf( upd_datebuf, sizeof( upd_datebuf ), "%02d/%02d/%04d"
				//		, password_updated.wMo, password_updated.wDy, password_updated.wYr );
				DoSQLQueryf( &result, "select now()" );
				snprintf( exp_datebuf, sizeof( exp_datebuf ), "%s" 
						, result );
				upd_date = upd_datebuf;
			}
			else
				upd_date = "Before Now";			
			
			LIST_FORALL( l.user2->groups, idx, PGROUP, group )
			{
				INDEX idx2;
				PTOKEN group_token;				
				LIST_FORALL( group->tokens, idx2, PTOKEN, group_token )
				{					
					if( group_token->name )
					{
						snprintf( buf, sizeof( buf ), "%-20.20s %-16.16s  %s  %s  %s\n"
								  , l.user2->name
								  , group_token->name
								  , upd_date
								  , exp_date
								  , date
								  );
						PrintString( buf );

					}
				}
				//if( group_token )
					//break;
			}					
		}
	}

	ClosePrinterDC( printer );
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// User History
static uintptr_t OnCreateMenuButton( "SQL Users/User History" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "User History" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/User History" )( uintptr_t psv )
{
#if 0
	HDC printer = GetPrinterDC(0);
	//int n;
	TEXTCHAR szString[256];
	struct {
		uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	} g;
	struct {
		uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	} report_from;
	//struct {
	//	uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	//} report_to;
	uint32_t now = CAL_GET_FDATETIME();
	FontFromColumns( printer, NULL, NULL, 140, NULL );
	ClearReportHeaders();

	CAL_P_YMDHMS_OF_FDATETIME( now, &g.wYr, &g.wMo, &g.wDy, &g.wHr, &g.wMn, &g.wSc );
	CAL_P_YMDHMS_OF_FDATETIME( now - (100*(24*60*60))
									 , &report_from.wYr, &report_from.wMo, &report_from.wDy, NULL, NULL, NULL );
	snprintf( szString, sizeof( szString ), "User History Report"
			  , g.wMo, g.wDy, g.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), "Printed at %d:%02d:%02d on %02d/%02d/%02d"
			  , g.wHr, g.wMn, g.wSc
			  , g.wMo, g.wDy, g.wYr%100);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), "Report from %02d/%02d/%02d to %02d/%02d/%02d"
			  , report_from.wMo, report_from.wDy, report_from.wYr%100
			  , g.wMo, g.wDy, g.wYr%100
			  );
	AddReportHeader( szString );
	AddReportHeader( "" );

	AddReportHeader( "Time					 UserName				 Event				  Message" );
	//					"00/00/0000 00:00:00 
	//					"20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
	AddReportHeader( "___________________ ____________________ __________________ ______________________________________\n" );
	PrintReportHeader( printer, -1, -1 ); // current position, write haeder...
	
	{
		static TEXTCHAR query[512];
		CTEXTSTR *result;
		snprintf( query, sizeof( query )
				 , "select user_event_log_timestamp,description,event_type,user_name"
				  " from user_event_log"
				  " where user_event_log_timestamp>=%04d%02d%02d and user_event_log_timestamp<=%04d%02d%02d235959"
				" and ( event_type='Password Update'"
				" or event_type='Delete User'"
				" or event_type='Create User'"
				" or event_type='Expire Password'"
				  " or event_type='') order by user_event_log_timestamp"
				  , report_from.wYr
				  , report_from.wMo
				  , report_from.wDy
				  , g.wYr
				  , g.wMo
				  , g.wDy );
		for( DoSQLRecordQuery( query, NULL, &result, NULL )
			; result
			; GetSQLRecord( &result ) )
		{
			TEXTCHAR buf[120];
			sprintf( buf, "%s %-20.20s %-18.18s %s\n", result[0], result[3], result[2], result[1] );
			PrintString( buf );
		}
	}	

	ClosePrinterDC( printer );
#endif
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Permission Report
static uintptr_t OnCreateMenuButton( "SQL Users/Permission Report" )( PMENU_BUTTON button )
{	
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonText( button, "Permission Report" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_GREEN, BASE_COLOR_BLACK, COLOR_IGNORE );
	return (uintptr_t)button;
}

static void OnKeyPressEvent( "SQL Users/Permission Report" )( uintptr_t psv )
{	
	/*
	HDC printer = GetPrinterDC(1);
	//int n;
	TEXTCHAR szString[256];
	struct {
		uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	} g;
	struct {
		uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	} report_from;
	//struct {
	//	uint16_t wYr, wMo, wDy, wHr, wMn, wSc;
	//} report_to;
	uint32_t now = CAL_GET_FDATETIME();
	FontFromColumns( printer, NULL, NULL, 112, NULL );
	ClearReportHeaders();

	CAL_P_YMDHMS_OF_FDATETIME( now, &g.wYr, &g.wMo, &g.wDy, &g.wHr, &g.wMn, &g.wSc );
	CAL_P_YMDHMS_OF_FDATETIME( now - (60*(24*60*60))
									 , &report_from.wYr, &report_from.wMo, &report_from.wDy, NULL, NULL, NULL );
	snprintf( szString, sizeof( szString ), "User Permission Report"
			  , g.wMo, g.wDy, g.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), "Printed at %d:%02d:%02d on %02d/%02d/%02d"
			  , g.wHr, g.wMn, g.wSc
			  , g.wMo, g.wDy, g.wYr%100);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), "Report from %02d/%02d/%02d to %02d/%02d/%02d"
			  , report_from.wMo, report_from.wDy, report_from.wYr%100
			  , g.wMo, g.wDy, g.wYr%100
			  );
	AddReportHeader( szString );
	AddReportHeader( "" );


	ClearReportHeaders();

	CAL_P_YMDHMS_OF_FDATETIME( now, &g.wYr, &g.wMo, &g.wDy, &g.wHr, &g.wMn, &g.wSc );
	CAL_P_YMDHMS_OF_FDATETIME( now - (60*(24*60*60))
									 , &report_from.wYr, &report_from.wMo, &report_from.wDy, NULL, NULL, NULL );
	snprintf( szString, sizeof( szString ), "User Permission Report"
			  , g.wMo, g.wDy, g.wYr);
	AddReportHeader( szString );
	snprintf(szString, sizeof( szString ), "Printed at %d:%02d:%02d on %02d/%02d/%02d"
			  , g.wHr, g.wMn, g.wSc
			  , g.wMo, g.wDy, g.wYr%100);
	AddReportHeader( szString );
	AddReportHeader( "" );

	AddReportHeader( "Group					 Permission		" );
	//					"00/00/0000 00:00:00 
	//					"20characternamegoesh Supervisor Group  03/03/1009  02/01/1008
	AddReportHeader( "____________________ __________________\n" );
	PrintReportHeader( printer, -1, -1 ); // current position, write haeder...


	{
		int first;
		int group, n;
		for( group = 0; group < PASSWORD_ACCESS_LEVELS; group++ )
		{
			int invalid = 0;
			first = 1;
			if( l.file[0].AccessDesc[group][0] )
				for( n = 0; permission_names[n]; n++ )
				{
					if( l.file[0].access[group][n].byte )
					{
						TEXTCHAR line[80];
						invalid = 0;
						if( n == 7 )
						{
							if( !SACK_GetProfileInt( "PASSWORDS", "Allow Edit User", 0 ) )
							invalid = 1;
						}
						if( group && first )
						{
							PrintString( "\n" );
							PrintString( "\n" );
						}
						snprintf( line, sizeof( line ), "%-20.20s %s %s\n"
								  , first?l.file[0].AccessDesc[group]:""
								  , permission_names[n]
								  , invalid?"[N/A]":"" );
						PrintString( line );
						first = 0;
					}
				}
		}
		PrintString( "\n" );
		PrintString( "\n" );
		PrintString( "Meanings of Permissions above" );
		PrintString( "___________________________________\n" );
		PrintString( "Cashier			  - General access to the POS for sales.\n" );
		PrintString( "Manager Options	- Allows access to the Manager Options Screen for reporting.\n" );
		PrintString( "Z-out				 - Allows user to close out/Finalize user on the POS.\n" );
		PrintString( "Configure Buttons - Allows the user to configure buttons available to sell.  Add/Delete/Modify.\n" );
		PrintString( "Edit Taxes		  - The POS has support for computing taxes on items.  This option allows the modification\n" );
		PrintString( "						  of these tax rates, if applicable\n" );
		PrintString( "Void				  - User is able to void a transaction.\n" );
		PrintString( "Configure Options - Allows the user to change opens in the Manger Option screen.\n" );
		PrintString( "Edit Groups		 - If group/user editing is enabled, this user may change the permissions for a group.\n" );
		PrintString( "						  (Modify the above report data)\n" );
		PrintString( "Remove Winners	 - Under certain configurations, the payouts are presented in a different format, this\n" );
		PrintString( "						  permission allows the user to no-pay a winner.\n" );
		PrintString( "\n" );
		PrintString( "[N/A] Marks a permission as meaningless based on system configuration.\n" );
	}
	ClosePrinterDC( printer );	
	*/
}

//--------------------------------------------------------------------------------
// Returns Current User if there is one
TEXTSTR getCurrentUser( void )
{	
	return l.logee_name;	
}

//--------------------------------------------------------------------------------
// Returns Current User if there is one
CTEXTSTR getCurrSystemID( void )
{	
	return g.system_id;	
}

//--------------------------------------------------------------------------------
// Create label variables
void CreateLabels( void )
{	
	l.sPassPop = l.cPassPop;	
	l.sPassPop2 = l.cPassPop2;	
	l.sChangePassword = l.cChangePassword;	
	l.sUnlockAccount = l.cUnlockAccount;	
	l.sTermAccount = l.cTermAccount;	
	l.sExpPassword = l.cExpPassword;	
	l.sAddPermGroup = l.cAddPermGroup;	
	l.sRemPermGroup = l.cRemPermGroup;	
	l.sAddToken = l.cAddToken;	
	l.sRemToken = l.cRemToken;	
	l.sCreateGroup = l.cCreateGroup;	
	l.sDeleteGroup = l.cDeleteGroup;	
	l.sCreateToken = l.cCreateToken;	
	l.sDeleteToken = l.cDeleteToken;	
	l.sCreateUser = l.cCreateUser;	
	l.sCreateUser2 = l.cCreateUser2;	
	l.lvPassPop  = CreateLabelVariable( "<Pop Pass Message>", LABEL_TYPE_STRING, &l.sPassPop  );
	l.lvPassPop2  = CreateLabelVariable( "<Pop Pass2 Message>", LABEL_TYPE_STRING, &l.sPassPop2 );
	l.lvChangePassword  = CreateLabelVariable( "<Change Password Message>", LABEL_TYPE_STRING, &l.sChangePassword );
	l.lvUnlockAccount = CreateLabelVariable( "<Unlock Account Message>", LABEL_TYPE_STRING, &l.sUnlockAccount );
	l.lvTermAccount = CreateLabelVariable( "<Terminate Account Message>", LABEL_TYPE_STRING, &l.sTermAccount );
	l.lvExpPassword = CreateLabelVariable( "<Expire Password Message>", LABEL_TYPE_STRING, &l.sExpPassword );
	l.lvAddPermGroup = CreateLabelVariable( "<Add User Group Message>", LABEL_TYPE_STRING, &l.sAddPermGroup );
	l.lvRemPermGroup = CreateLabelVariable( "<Remove User Group Message>", LABEL_TYPE_STRING, &l.sRemPermGroup );
	l.lvAddToken = CreateLabelVariable( "<Add Group Token Message>", LABEL_TYPE_STRING, &l.sAddToken );
	l.lvRemToken = CreateLabelVariable( "<Remove Group Token Message>", LABEL_TYPE_STRING, &l.sRemToken );	
	l.lvCreateGroup = CreateLabelVariable( "<Create Group Message>", LABEL_TYPE_STRING, &l.sCreateGroup );
	l.lvDeleteGroup = CreateLabelVariable( "<Delete Group Message>", LABEL_TYPE_STRING, &l.sDeleteGroup );
	l.lvCreateToken = CreateLabelVariable( "<Create Token Message>", LABEL_TYPE_STRING, &l.sCreateToken );
	l.lvDeleteToken = CreateLabelVariable( "<Delete Token Message>", LABEL_TYPE_STRING, &l.sDeleteToken );
	l.lvCreateUser = CreateLabelVariable( "<Create User Message>", LABEL_TYPE_STRING, &l.sCreateUser );
	l.lvCreateUser2 = CreateLabelVariable( "<Create User2 Message>", LABEL_TYPE_STRING, &l.sCreateUser2 );

	return;
}


//--------------------------------------------------------------------------------
// Preload
PRIORITY_PRELOAD( Init_password_frame, DEFAULT_PRELOAD_PRIORITY-1 )
{	
	PODBC odbc;
	TEXTCHAR buf[10];
	TEXTCHAR buf2[20];	
	CTEXTSTR result;

	// Get Interfaces
	l.pri = GetDisplayInterface();
	l.pii = GetImageInterface();	

	// Set flags ( just in case not compiled to 0 )
	l.flags.init = 1;
	l.flags.matched = 0;
	l.flags.first_loop = 0;		
	l.flags.exit = 0;
	l.stage = 0;

	// Other needed preset ( Will Set From Database )				
	l.default_room_id = 0;

	// Create & Initialize Label Variables
	CreateLabels();	

	snprintf( l.cPassPop, MESSAGE_SIZE, "%s", "Please enter your password and press enter." );

	snprintf( l.cPassPop2, MESSAGE_SIZE, "%s", "Your password has expired! Please enter a new password and press okay." );

	snprintf( l.cChangePassword, MESSAGE_SIZE, "%s", "Please enter a new password and press enter." );

	snprintf( l.cUnlockAccount, MESSAGE_SIZE, "%s", "Please select a user to unlock and press unlock user account." );

	snprintf( l.cTermAccount, MESSAGE_SIZE, "%s", "Please select a user to terminate and press terminate user account." );
	
	snprintf( l.cExpPassword, MESSAGE_SIZE, "%s", "Please select the user whose password is to be expired and press expire user password." );

	snprintf( l.cAddPermGroup, MESSAGE_SIZE, "%s", "Please select a user and a group to add the user to and press add user group." );

	snprintf( l.cRemPermGroup, MESSAGE_SIZE, "%s", "Please select a user who is to be removed from a permission group and press remove user group." );

	snprintf( l.cAddToken, MESSAGE_SIZE, "%s", "Please select a token and a permission group to give the token to and press add group token." );

	snprintf( l.cRemToken, MESSAGE_SIZE, "%s", "Please select a permission group and press remove group token." );

	snprintf( l.cCreateGroup, MESSAGE_SIZE, "%s", "Please enter a group name and press enter." );

	snprintf( l.cDeleteGroup, MESSAGE_SIZE, "%s", "Please select a group and press delete group." );

	snprintf( l.cCreateToken, MESSAGE_SIZE, "%s", "Please enter a token name and press enter." );

	snprintf( l.cDeleteToken, MESSAGE_SIZE, "%s", "Please select a token and press delete token." );

	snprintf( l.cCreateUser, MESSAGE_SIZE, "%s", "Please enter a staff id and press enter." );

	snprintf( l.cCreateUser2, MESSAGE_SIZE, "%s", "Please enter all fields, select a permission group and press create user." );

	//Create Extra Keypads
	CreateKeypadType( "Enter Password Keypad" );
	CreateKeypadType( "Expire Password Keypad" );
	CreateKeypadType( "Change Password Keypad" );
	CreateKeypadType( "Create Group Keypad" );
	CreateKeypadType( "Create Token Keypad" );
	CreateKeypadType( "Create User Keypad" );	

#ifdef _DEBUG
#define DEFAULT_LOGIN_INIT 0
#else
#define DEFAULT_LOGIN_INIT 1
#endif
	g.flags.bInitializeLogins = SACK_GetProfileInt( GetProgramName(), "SECURITY/Initialize Logins", DEFAULT_LOGIN_INIT );
	l.flags.bCreateSystemLogin = SACK_GetProfileInt( GetProgramName(), "SECURITY/Create System Login", 0 );

	l.displays_wide = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Expected displays wide", 1, TRUE );
	l.displays_high = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Expected displays high", 1, TRUE );
	l.flags.cover_entire_canvas = SACK_GetProfileIntEx( GetProgramName(), "SQL Password/Cover Entire Canvas", 0, TRUE );
	{
		// Set Up for pulling options
		TEXTCHAR option_dsn[64];
		SACK_GetProfileString( "SECURITY/SQL Passwords", "password DSN", GetDefaultOptionDatabaseDSN(), option_dsn, sizeof( option_dsn ) );
		odbc = GetOptionODBC( option_dsn );
	}
	 	
	l.bad_login_limit = SACK_GetPrivateOptionInt( odbc, "SECURITY/SYSTEM/Login Failure", "Limit", 5, NULL );

	SACK_GetPrivateOptionString( odbc, "SECURITY/Login/Password", "Lock Interval", "30 MINUTE", buf2, 20, NULL );
	l.bad_login_interval = StrDup(buf2);

	l.flags.does_pass_expr = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Password Expires", "Enabled", 1, NULL );

	g.pass_expr_interval = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Password Expires", "Days", 30, NULL );
	  
	l.pass_check_num = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Password Reusable", "Number", 8, NULL );	
	
	l.pass_check_days = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Password Reusable", "Days", 90, NULL );	

	l.pass_min_length = SACK_GetPrivateOptionInt( odbc, "SECURITY/Login/Password", "MinLength", 8, NULL );		

	SACK_GetPrivateOptionString( odbc, "SECURITY/Login/Password/Upper Case", "Required", "TRUE", buf, 10, NULL );
	l.flags.does_pass_req_upper = ( buf[0]=='t' || buf[0] =='T' || buf[0] == '1' || buf[0] == 'Y' || buf[0] == 'y' ) ? 1 : 0;		

	SACK_GetPrivateOptionString( odbc, "SECURITY/Login/Password/Lower Case", "Required", "TRUE", buf, 10, NULL );	
	l.flags.does_pass_req_lower = ( buf[0]=='t' || buf[0] =='T' || buf[0] == '1' || buf[0] == 'Y' || buf[0] == 'y' ) ? 1 : 0;

	SACK_GetPrivateOptionString( odbc, "SECURITY/Login/Password/Special Characters", "Required", "TRUE", buf, 10, NULL );	
	l.flags.does_pass_req_spec = ( buf[0]=='t' || buf[0] =='T' || buf[0] == '1' || buf[0] == 'Y' || buf[0] == 'y' ) ? 1 : 0;

	SACK_GetPrivateOptionString( odbc, "SECURITY/Login/Password/Numeric Characters", "Required", "TRUE", buf, 10, NULL );	
	l.flags.does_pass_req_num = ( buf[0]=='t' || buf[0] =='T' || buf[0] == '1' || buf[0] == 'Y' || buf[0] == 'y' ) ? 1 : 0;

	DropOptionODBC( odbc );

	// Get system name and system id
	if( DoSQLQueryf( &result, "select user()" ) )
	{
		if( !result )
		{
			GetSQLError( &result );
			lprintf( "Connected to a database, but select user() failed! : %s", result );
			return;
		}	
		l.sys_name = SaveText( result );				
	}
	else
	{		
		  lprintf( "database connection is invalid." );
		return;
	}	
	SQLEndQuery( NULL );
	
	if( DoSQLQueryf( &result, "select system_id from systems where address=user()" ) && result )
	{
		g.system_id = SaveText(result);
	}
	else
	{
		if( DoSQLQueryf( &result, "select system_id from systems where name=\'%s\'",GetSystemName() ) && result )
		{
			DoSQLCommandf( "update systems set address=\'%s\' where system_id=%s", l.sys_name, result );
			g.system_id = SaveText(result);
		}
		else
		{
			DoSQLCommandf( "insert into systems (name,address) values ('%s','%s')", GetSystemName(), l.sys_name );
			g.system_id = GetLastInsertKey( "systems", "system_id" );
		}
		SQLEndQuery( NULL );
	}

	//lprintf( " System ID: %d, System Name: %s", g.system_id, l.sys_name );
	
	// Get program name and program id	
	l.prog_name = GetProgramName();
	l.program_id = GetProgramID( l.prog_name );
 
	//lprintf( " Program ID: %d, Program Name: %s", l.program_id, l.prog_name );

	// Get User Info 
	l.logee_id = 0;
	l.logee_name = NULL;

	if( g.flags.bInitializeLogins )
	{
		DoSQLCommandf( "update login_history set logout_whenstamp=now() where logout_whenstamp=11111111111111 and system_id=%d"
						, g.system_id );
	}

	if( DoSQLQueryf( &result, "select user_id from login_history where system_id=%d and logout_whenstamp='11111111111111' order by login_whenstamp desc limit 1", g.system_id ) && result )
	{
		l.logee_id = StrDup( result );

		if( DoSQLQueryf( &result, "select name from permission_user_info where user_id=%d", l.logee_id ) && result )
			l.logee_name = StrDup( result );
		SQLEndQuery( NULL );

		//lprintf( " User ID: %d, User Name: %s", l.logee_id, l.logee_name );
	}
	SQLEndQuery( NULL );

}
        

