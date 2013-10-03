//
//   login_monitor.c
//   (C) Copyright 2009 
//   Crafted by d3x0r
//                   
////////////////////////////////////////////////////////////////////////////
#define DEFINE_DEFAULT_RENDER_INTERFACE

#define SQL_PASSWORD_MAIN
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE

#include <stdhdrs.h>
#include <pssql.h>
#include <sha1.h>
#include <futgetpr.h>
#include <futcal.h>
//#include <getoption.h>
#include <InterShell/widgets/banner.h>
#include <InterShell/intershell_registry.h>
#include <InterShell/intershell_export.h>
#include "comn_util.h"

/* get permisison creation time option... */
#include "global.h"

enum {
   BTN_PASSKEY = 10000
	  , PERMISSIONS = BTN_PASSKEY + 50
	  , REQUIRED_PERMISSIONS
	  , PASSCODE
	  , CHECKBOX_REQUIRE_PARENT_LOGIN
	  , CHECKBOX_OVERRIDE_PARENT_REQUIRED
     , TEXT_EDIT_REQUIRED_PERMISSION
     , TEXT_EDIT_OVERRIDE_REQUIRED_PERMISSION
};

PRIORITY_PRELOAD( RegisterUserPasswordControls, DEFAULT_PRELOAD_PRIORITY - 2 )
{
	CTEXTSTR permission_tokens = "CREATE TABLE `permission_tokens` ("
	"`permission_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`name` varchar(100) NOT NULL DEFAULT '',"
	"`log` int(11) NOT NULL DEFAULT '0',"
	"`description` varchar(255) DEFAULT '',"
	"PRIMARY KEY (`permission_id`),"
	"KEY `token` (`name`)"
	") ENGINE=MyISAM AUTO_INCREMENT=827 DEFAULT CHARSET=latin1 COMMENT='Name detail of permission_id'";

	CTEXTSTR permission_set = "CREATE TABLE `permission_set` ("
	"`permission_group_id` int(11) NOT NULL DEFAULT '0',"
	"`permission_id` int(11) NOT NULL DEFAULT '0',"
	"PRIMARY KEY (`permission_group_id`,`permission_id`),"
	"KEY `group` (`permission_group_id`),"
	"KEY `token` (`permission_id`)"
	") ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='relate permission_group_id to permission_id'";

	CTEXTSTR permission_group = "CREATE TABLE `permission_group` ("
	"`permission_group_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`name` varchar(100) NOT NULL DEFAULT '',"
	"`hall_id` int(11) NOT NULL DEFAULT '0',"
	"`dummy_timestamp` timestamp,"
	"`charity_id` int(11) NOT NULL DEFAULT '0',"
	"`description` varchar(255) DEFAULT '',"
	"PRIMARY KEY (`permission_group_id`)"
	") ENGINE=MyISAM AUTO_INCREMENT=39 DEFAULT CHARSET=latin1 COMMENT='USAGE: Groups in which tokens are assigned to & then groups '";

	CTEXTSTR permission_user = "CREATE TABLE `permission_user` ("
	"`permission_group_id` int(11) NOT NULL DEFAULT '0',"
	"`user_id` int(11) NOT NULL DEFAULT '0',"
	"PRIMARY KEY (`permission_group_id`,`user_id`),"
	"KEY `user` (`user_id`),"
	"KEY `group` (`permission_group_id`)"
	") ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='relate permission_group_id to user_id'";

	CTEXTSTR permission_user_info = "CREATE TABLE `permission_user_info` ("
	"`user_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`hall_id` int(11) NOT NULL DEFAULT '0',"
	"`charity_id` int(11) NOT NULL DEFAULT '0',"
	"`default_room_id` int(11) unsigned NOT NULL DEFAULT '0',"
	"`first_name` varchar(100) NOT NULL DEFAULT '',"
	"`last_name` varchar(100) NOT NULL DEFAULT '',"
	"`name` varchar(100) NOT NULL DEFAULT '',"
	"`system_user_name` varchar(255) DEFAULT '',"
	"`staff_id` varchar(100) NOT NULL DEFAULT '',"
	"`pin` varchar(42) DEFAULT NULL,"
	"`password` varchar(42) NOT NULL DEFAULT '',"
	"`password_creation_datestamp` date DEFAULT NULL,"
	"`key_code` varchar(100) DEFAULT NULL,"
	"`terminate` int(11) DEFAULT '0',"
	"`lot_container` tinyint(4) unsigned NOT NULL DEFAULT '0',"
	"`locale_id` int(11) NOT NULL DEFAULT '0',"
	"`card` varchar(25) DEFAULT NULL,"
	"PRIMARY KEY (`user_id`),"
	"KEY `PINsearch` (`pin`),"
	"KEY `permission_user_info_terminate_idx` (`terminate`),"
	"KEY `permuserinfo_locale_idx` (`locale_id`)"
	") ENGINE=MyISAM AUTO_INCREMENT=22 DEFAULT CHARSET=latin1 COMMENT='USAGE: Stores all users in all systems, there passwords, ids'";

	CTEXTSTR permission_user_password = "CREATE TABLE `permission_user_password` ("
	"`user_password_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`user_id` int(11) NOT NULL DEFAULT '0',"
	"`password` varchar(42) NOT NULL DEFAULT '',"
	"`description` varchar(255) NOT NULL DEFAULT '',"
	"`creation_datestamp` timestamp,"
	"PRIMARY KEY (`user_password_id`),"
	"KEY `user_password_key` (`password`,`user_id`)"
	") ENGINE=MyISAM AUTO_INCREMENT=23 DEFAULT CHARSET=latin1";

	CTEXTSTR permission_user_type = "CREATE TABLE `permission_user_type` ("
	"`permission_user_type_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`permission_user_type_name` varchar(100) NOT NULL DEFAULT '',"
	"`permission_token_name` varchar(100) NOT NULL DEFAULT '',"
	"PRIMARY KEY (`permission_user_type_id`)"
	") ENGINE=MyISAM DEFAULT CHARSET=latin1";

	CTEXTSTR system_exception_type = "CREATE TABLE `system_exception_type` ("
	"`system_exception_type_id` int(11) NOT NULL,"
	"`system_exception_type_name` varchar(255) NOT NULL,"
	"PRIMARY KEY (`system_exception_type_id`)"
	") ENGINE=InnoDB DEFAULT CHARSET=latin1";

	CTEXTSTR system_exceptions = "CREATE TABLE `system_exceptions` ("
	"`system_exception_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`system_exception_category_id` int(11) NOT NULL,"
	"`system_exception_type_id` int(11) NOT NULL,"
	"`user_id` int(11) DEFAULT NULL,"
	"`system_id` int(11) DEFAULT NULL,"
	"`program_id` int(11) DEFAULT NULL,"
	"`initial_value` tinytext,"
	"`new_value` tinytext,"
	"`description` tinytext,"
	"`log_whenstamp` timestamp,"
	"PRIMARY KEY (`system_exception_id`)"
	") ENGINE=InnoDB AUTO_INCREMENT=283 DEFAULT CHARSET=latin1";

	CTEXTSTR system_exception_category = "CREATE TABLE `system_exception_category` ("
	"`system_exception_category_id` int(11) NOT NULL,"
	"`system_exception_category_name` varchar(255) NOT NULL,"
	"PRIMARY KEY (`system_exception_category_id`)"
	") ENGINE=InnoDB DEFAULT CHARSET=latin1";

	CTEXTSTR program_identifiers = "CREATE TABLE `program_identifiers` ("
	"`program_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`program_name` varchar(100) NOT NULL DEFAULT '',"
	"PRIMARY KEY (`program_id`),"
	"KEY `prog_name` (`program_name`)"
	") ENGINE=MyISAM AUTO_INCREMENT=61 DEFAULT CHARSET=latin1";

	CTEXTSTR login_history = "CREATE TABLE `login_history` ("
	"`login_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`actual_login_id` int(11) DEFAULT '0',"
	"`bingoday` date DEFAULT NULL,"
	"`session` int(11) DEFAULT '0',"
	"`system_id` int(11) NOT NULL DEFAULT '0',"
	"`program_id` int(11) NOT NULL DEFAULT '0',"
	"`user_id` int(11) NOT NULL DEFAULT '0',"
	"`group_id` int(11) NOT NULL DEFAULT '0',"
	"`hall_id` int(11) NOT NULL DEFAULT '0',"
	"`charity_id` int(11) NOT NULL DEFAULT '0',"
	"`login_whenstamp` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',"
	"`logout_whenstamp` datetime NOT NULL DEFAULT '1111-11-11 11:11:11',"
	"PRIMARY KEY (`login_id`),"
	"KEY `sesskey` (`session`),"
	"KEY `bingoday` (`bingoday`,`session`,`user_id`),"
	"KEY `loginkey` (`login_id`),"
	"KEY `prog` (`program_id`),"
	"KEY `logout` (`logout_whenstamp`),"
	"KEY `3columns` (`system_id`,`program_id`,`logout_whenstamp`),"
	"KEY `login_hist_idx` (`logout_whenstamp`,`program_id`,`system_id`),"
	"KEY `login_history_user_id_idx` (`user_id`),"
	"KEY `login_history_actual_login_id_idx` (`actual_login_id`),"
	"KEY `login_history_actual_login_whenstamp_idx` (`login_whenstamp`)"
	") ENGINE=MyISAM AUTO_INCREMENT=3830 DEFAULT CHARSET=latin1 COMMENT='USAGE: keeps track of all logins from all systems and progam'";

	CTEXTSTR systems = "CREATE TABLE `systems` ("
	"`system_id` int(11) NOT NULL AUTO_INCREMENT,"
	"`name` varchar(64) NOT NULL DEFAULT '',"
	"`address` varchar(64) NOT NULL DEFAULT '',"
	"`login_failure_count` int(11) NOT NULL DEFAULT '0',"
	"`login_failure_lockout_until` datetime DEFAULT NULL,"
	"PRIMARY KEY (`system_id`),"
	"KEY `namekey` (`name`)"
	") ENGINE=MyISAM AUTO_INCREMENT=32 DEFAULT CHARSET=latin1 COMMENT='address must be determined by running the User()'";

	CTEXTSTR permission_user_log = "CREATE TABLE `permission_user_log` ("
	"`log_ID` int(11) NOT NULL AUTO_INCREMENT,"
	"`login_id` int(11) DEFAULT '0',"
	"`description` varchar(255) DEFAULT NULL,"
	"`permission_id` int(11) NOT NULL DEFAULT '0',"
	"`logtype` char(48) NOT NULL DEFAULT '0',"
	"`log_whenstamp` datetime DEFAULT NULL,"
	"PRIMARY KEY (`log_ID`),"
	"KEY `permission_user_log_login_id_idx` (`login_id`)"
	") ENGINE=MyISAM AUTO_INCREMENT=11031 DEFAULT CHARSET=latin1 COMMENT='USAGE: Logs systems access'";
	
	PTABLE table;

	table = GetFieldsInSQL( permission_tokens, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_set, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_group, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_user, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_user_password, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_user_info, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_user_type, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( system_exception_type, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( system_exceptions, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( system_exception_category, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( program_identifiers, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( login_history, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( systems, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	table = GetFieldsInSQL( permission_user_log, FALSE );
	CheckODBCTable( NULL, table, CTO_MERGE );
	DestroySQLTable( table );

	{
		CTEXTSTR *result;
      // code to fix OLD permission_user_log
		if( DoSQLRecordQueryf( NULL, &result, NULL, "show create table permission_user_log" ) && result && result[1] )
			if( StrStr( result[1], "enum" ) )
			{
				DoSQLCommandf( "ALTER TABLE `permission_user_log` MODIFY COLUMN `logtype` CHAR(48) CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL DEFAULT 0" );
			}
	}

	EasyRegisterResourceRange( "User Keypad", BTN_PASSKEY, 36, NORMAL_BUTTON_NAME );
	//EasyRegisterResourceRange( "User Keypad", BTN_PASSKEY, 36, CUSTOM_BUTTON_NAME );
	EasyRegisterResource( "MILK/Security/SQL", PERMISSIONS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "User Keypad", PASSCODE, STATIC_TEXT_NAME );
	EasyRegisterResource( "MILK/Security/SQL", REQUIRED_PERMISSIONS, LISTBOX_CONTROL_NAME );

   EasyRegisterResource( "MILK/Security/SQL", CHECKBOX_REQUIRE_PARENT_LOGIN, RADIO_BUTTON_NAME );
	EasyRegisterResource( "MILK/Security/SQL", CHECKBOX_OVERRIDE_PARENT_REQUIRED, RADIO_BUTTON_NAME );
	EasyRegisterResource( "MILK/Security/SQL", TEXT_EDIT_REQUIRED_PERMISSION, EDIT_FIELD_NAME );
	EasyRegisterResource( "MILK/Security/SQL", TEXT_EDIT_OVERRIDE_REQUIRED_PERMISSION, EDIT_FIELD_NAME );

}

PSQL_PASSWORD GetButtonSecurity( PTRSZVAL button, int bCreate )
{
	PSQL_PASSWORD pls;
	INDEX idx;
	LIST_FORALL( g.secure_buttons, idx, PSQL_PASSWORD, pls )
	{
		if( pls->button == button )
			break;
	}
	if( !pls && bCreate )
	{
		pls = New( SQL_PASSWORD );
      MemSet( pls, 0, sizeof( SQL_PASSWORD ) );
		pls->button = button;
		pls->permissions = NewArray( FLAGSETTYPE, FLAGSETSIZE( FLAGSETTYPE, g.permission_count ) );
		MemSet( pls->permissions, 0, FLAGSETSIZE( FLAGSETTYPE, g.permission_count ) );
		AddLink( &g.secure_buttons, pls );
	}
   return pls;
}

void ResolveToken( PTOKEN *ppToken, CTEXTSTR *target, CTEXTSTR permission )
{
	{
		int n;
		PTOKEN token;
      if( (*target) )
			Release( *target );
      lprintf( "New permission is %s", permission );
		if( permission && permission[0] )
		{
			(*target) = StrDup( permission );
			LIST_FORALL( g.tokens, n, PTOKEN , token )
			{
            lprintf( "is [%s]==[%s]", permission, token->name );
				if( stricmp( permission, token->name ) == 0 )
				{
					(*ppToken) = token;
					break;
				}
			}
		}
		else
		{
			(*target) = NULL;
			(*ppToken) = NULL;
		}
	}
}

static PTRSZVAL CPROC AddButtonSecurity( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, permission );
	PTRSZVAL last_loading = psv;
	PSQL_PASSWORD pls = GetButtonSecurity( last_loading, TRUE );
	//lprintf( "load context %p(%p)", pls, last_loading );
	if( pls )
	{
		int n;
		PTOKEN token;		
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( stricmp( permission, token->name ) == 0 )
			{				
				pls->pTokens = Reallocate( pls->pTokens, sizeof( TEXTSTR ) * (++pls->nTokens) );				
				pls->pTokens[pls->nTokens-1] = StrDup( token->name );				
				SETFLAG( pls->permissions, n );
				break;
			}
		}
	}	
	return psv;
}

static PTRSZVAL CPROC AddButtonSecurityRequire( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, permission );
	PTRSZVAL last_loading = psv;
	PSQL_PASSWORD pls = GetButtonSecurity( last_loading, TRUE );
	lprintf( "load context %p(%p)", pls, last_loading );
	if( pls )
	{
      ResolveToken( &pls->required_token_token, &pls->required_token, permission );
	}	
	return psv;
}

static PTRSZVAL CPROC AddButtonSecurityOverride( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, permission );
	PTRSZVAL last_loading = psv;
	PSQL_PASSWORD pls = GetButtonSecurity( last_loading, TRUE );
	//lprintf( "load context %p(%p)", pls, last_loading );
	if( pls )
	{
      ResolveToken( &pls->override_required_token_token, &pls->override_required_token, permission );
	}	
	return psv;
}

static void OnAddSecurityContextToken( "SQL Password" )( PTRSZVAL context, CTEXTSTR permission )
{
	PSQL_PASSWORD pls = GetButtonSecurity( context, TRUE );
	//lprintf( "load context %p(%p)", pls, context );
	if( pls )
	{
		int n;
		PTOKEN token;		
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( stricmp( permission, token->name ) == 0 )
			{				
				pls->pTokens = Reallocate( pls->pTokens, sizeof( TEXTSTR ) * (++pls->nTokens) );				
				pls->pTokens[pls->nTokens-1] = StrDup( token->name );				
				SETFLAG( pls->permissions, n );
				break;
			}
		}
	}	
}

static void OnGetSecurityContextTokens( "SQL Password" )( PTRSZVAL context, PLIST *list )
{
	PSQL_PASSWORD pls = GetButtonSecurity( context, TRUE );
	EmptyList( list );
	{
		int n;
		PTOKEN token;
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( TESTFLAG( pls->permissions, n ) )
				AddLink( list, token->name );
		}
	}
}


static void OnLoadSecurityContext( "SQL Password" )( PCONFIG_HANDLER pch )
{
   AddConfigurationMethod( pch, "SQL password security=%m", AddButtonSecurity );
   AddConfigurationMethod( pch, "SQL password required login=%m", AddButtonSecurityRequire );
   AddConfigurationMethod( pch, "SQL password required login override=%m", AddButtonSecurityOverride );
}

static void OnSaveSecurityContext( "SQL Password" )( FILE *file, PTRSZVAL button )
{
	PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
   //lprintf( "save context %p", pls );
	if( pls )
	{
		int n;
		PTOKEN token;
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( TESTFLAG( pls->permissions, n ) )
            fprintf( file, "%sSQL password security=%s\n", InterShell_GetSaveIndent(), token->name );
		}
		if( pls->required_token && pls->required_token[0] )
		{
			fprintf( file, "%sSQL password required login=%s\n", InterShell_GetSaveIndent(), pls->required_token );
			if( pls->override_required_token && pls->override_required_token[0] )
				fprintf( file, "%sSQL password required login override=%s\n", InterShell_GetSaveIndent(), pls->override_required_token );
		}
	}
}

//--------------------------------------------------------------------------------

static PTRSZVAL TestSecurityContext( "SQL Password" )( PTRSZVAL button )
{	
	TEXTSTR current_user;
	PTRSZVAL result;
	PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
	if( pls )
	{
   		//lprintf( "load context %p(%p)", pls, button );
   	
		g.current_user = NULL;
		if( current_user = getCurrentUser() )
		{
			PUSER user;
			INDEX idx;	
			lprintf( "Have a current user?!" );
			LIST_FORALL( g.users, idx, PUSER, user )
			{
				if( strcmp( user->name, current_user ) == 0 )
				{
					lprintf( "Setting current user to ....?" );
					g.current_user = user;
				}
			}
		}

		if( pls && pls->nTokens )
		{
			struct password_info *pi;
			pi = PromptForPassword( &g.current_user, &g.current_user_login_id, NULL, pls->pTokens, pls->nTokens, pls );
			if( !pi || pi->login_id == INVALID_INDEX )
				return INVALID_INDEX;

			return (PTRSZVAL)pi;
		}
	}
	return 0; /* no security... */
}

static void  EndSecurityContext( "SQL Password" ) ( PTRSZVAL button, PTRSZVAL psv )
{
	struct password_info *pi = (struct password_info *)psv;
	if( pi )
	{
		LogOutPassword( pi->login_id, pi->actual_login_id, FALSE );
		Release( pi );
	}
	return;	
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CPROC OnItemDoubleClickPermission( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
	int n;
	int already_picked = 0;
	PLISTITEM pli_picked;
	int perm_selected = GetItemData( pli );
	PSI_CONTROL pc_required = GetNearControl( pc, REQUIRED_PERMISSIONS );

   /* check the already required list, see if something is picked...*/
	for( n = 0; pli_picked = GetNthItem( pc_required, n ); n++ )
	{
		int nRequuired = GetItemData( pli_picked );
		if( nRequuired == perm_selected )
		{
			already_picked = TRUE;
			break;
		}
	}
	if( !already_picked )
	{
		SetItemData( AddListItem( pc_required
										, ((struct sql_token*)GetLink( &g.tokens, perm_selected ))->name ), perm_selected );
	}

}

//--------------------------------------------------------------------------------

void CPROC OnItemDoubleClickRequired( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
   DeleteListItem( pc, pli );
}

//--------------------------------------------------------------------------------

static void OnEditSecurityContext( "SQL Password" )( PTRSZVAL button )
{
	PSQL_PASSWORD pls = GetButtonSecurity( button, TRUE );
	if( pls )
	{
		PSI_CONTROL frame = LoadXMLFrameOver( NULL
														, "EditSQLButtonSecurity.Frame" );
		if( frame )
		{
			int okay = 0;
			int done = 0;

         SetControlText( GetControl( frame, TEXT_EDIT_REQUIRED_PERMISSION ), pls->required_token );
         SetControlText( GetControl( frame, TEXT_EDIT_OVERRIDE_REQUIRED_PERMISSION ), pls->override_required_token );
			{
				PSI_CONTROL list;
				list = GetControl( frame, PERMISSIONS );
				if( list )
				{
					int n;
					PTOKEN token;
					SetDoubleClickHandler( list, OnItemDoubleClickPermission, 0 );
					LIST_FORALL( g.tokens, n, PTOKEN , token )
					{
						SetItemData( AddListItem( list, token->name ), n );
					}
				}
				list = GetControl( frame, REQUIRED_PERMISSIONS );
				if( list )
				{
					int n;
					PTOKEN token;
					SetDoubleClickHandler( list, OnItemDoubleClickRequired, 0 );

					LIST_FORALL( g.tokens, n, PTOKEN , token )
					{
						if( TESTFLAG( pls->permissions, n ) )
							SetItemData( AddListItem( list, token->name ), n );
					}
				}
			}
			SetCommonButtons( frame, &done, &okay );
			DisplayFrame( frame );
			CommonWait( frame );
			if( okay )
			{
            TEXTCHAR buf[256];
				GetControlText( GetControl( frame, TEXT_EDIT_REQUIRED_PERMISSION ), buf, 256 );
            ResolveToken( &pls->required_token_token, &pls->required_token, buf );

				GetControlText( GetControl( frame, TEXT_EDIT_OVERRIDE_REQUIRED_PERMISSION ), buf, 256 );
            ResolveToken( &pls->override_required_token_token, &pls->override_required_token, buf );

				{
					PSI_CONTROL list;
					list = GetControl( frame, REQUIRED_PERMISSIONS );
					if( list )
					{
						int n;
						PTOKEN token;
						PLISTITEM pli;
						LIST_FORALL( g.tokens, n, PTOKEN , token )
						{
							// reset all flags
							RESETFLAG( pls->permissions, n );
						}
						/* for all items in list, set required flag... */
						for( n = 0; pli = GetNthItem( list, n ); n++ )
						{
							SETFLAG( pls->permissions, GetItemData( pli ) );
						}
					}
				}
			}
			DestroyFrame( &frame );
		}
	}
}

PTOKEN FindToken( INDEX id, CTEXTSTR name )
{
	INDEX idx;
	PTOKEN token;
	LIST_FORALL( g.tokens, idx, PTOKEN, token )
	{
		if( StrCaseCmp( name, token->name ) == 0 )
		{
			if( id != token->id )
			{
				lprintf( "Token ID does not match (multiple definition?!" );
			}
			break;
		}
	}
	if( !token )
	{
		lprintf( "Added new token %d(%s)", id, name );
		token = New( struct sql_token );
		token->name = StrDup( name );
		token->id = id;
		AddLink( &g.tokens, token );
	}
	return token;
}

PGROUP FindGroup( INDEX id, CTEXTSTR name )
{
	INDEX idx;
	PGROUP group;
	LIST_FORALL( g.groups, idx, PGROUP, group )
	{
		if( StrCaseCmp( name, group->name ) == 0 )
		{
			if( id != group->id )
			{
				lprintf( "Group ID does not match (multiple definition?!" );
			}
         break;
		}
	}
	if( !group )
	{
		group = New( struct sql_group );
		group->name = StrDup( name );
		group->id = id;
		group->tokens = NULL;
		AddLink( &g.groups, group );
	}
   return group;
}

PUSER FindUser( INDEX id )
{
	INDEX idx;
	PUSER user;
	LIST_FORALL( g.users, idx, PUSER, user )
	{
		if( id == user->id )
			break;
	}
	if( !user )
	{
		user = New( struct sql_user );
		MemSet( user, 0, sizeof( struct sql_user ) );
		user->id = id;
		user->groups = NULL;
		AddLink( &g.users, user );
	}
   return user;
}

void UnloadUserCache( void )
{
	PUSER user;
	PGROUP group;
	PTOKEN token;
	INDEX idx, idx2, idx3;

   //
	// don't unload tokens, they are not reloaded.
   //

	LIST_FORALL( g.groups, idx2, PGROUP, group )
	{
		DeleteList( &group->tokens );
		Release( group );
		//DeleteLink( &g.groups, group );
	}

	LIST_FORALL( g.users, idx3, PUSER, user )
	{
		DeleteList( &user->groups );
		Release( user );
		//DeleteLink( &g.users, user );		
	}
		
	g.users = NULL;
	g.groups = NULL;
}


void ReloadUserCache( PODBC odbc )
{
	CTEXTSTR *result_user;
	CTEXTSTR *result_group;
	CTEXTSTR *result_token;
	lprintf(" UnloadUserCache");
	UnloadUserCache();
	lprintf(" ReloadUserCache");

	for( SQLRecordQueryf( odbc, NULL, &result_group, NULL, "select permission_group_id,name from permission_group"  )
		; result_group
		; FetchSQLRecord( odbc, &result_group ) )
	{
		INDEX group_id = strtoul( result_group[0], NULL, 10 );
		PGROUP group = FindGroup( group_id, result_group[1] );
	}


	for( SQLRecordQueryf( odbc, NULL, &result_user, NULL
							  , "select user_id,first_name,last_name,name,password_creation_datestamp,date_add(password_creation_datestamp,interval %d day),staff_id from permission_user_info order by first_name"
							  , g.pass_expr_interval )
		 ; result_user
		  ; FetchSQLRecord( odbc, &result_user ) )
	{
		INDEX user_id = strtoul( result_user[0], NULL, 10 );
		PUSER user = FindUser( user_id );
		PushSQLQueryEx( odbc );

		//
		if( result_user[6] && result_user[6][0] != '\0' )	
			user->staff = StrDup( result_user[6] );			

		else
			user->staff = " ";
			

		//
		if( result_user[1] && result_user[1][0] != '\0' )	
			user->first_name = StrDup( result_user[1] );

		else
			user->first_name = " ";

		//
		if( result_user[2] && result_user[2][0] != '\0' )
			user->last_name = StrDup( result_user[2] );			

		else
			user->last_name = " ";
		
		//
		if( result_user[3] && result_user[3][0] != '\0' )
			user->name = StrDup( result_user[3] );
			
		else
			user->name = " ";
				
		// first, last [1], [2]
		{
			int len;
			len = StrLen( user->first_name ) + StrLen( user->last_name ) + StrLen( user->staff ) + 5;
			user->full_name = NewArray( TEXTCHAR, len );
			snprintf( user->full_name, len, "%s\t%s\t(%s)", user->first_name, user->last_name, user->staff );
		}
		
		{
			int yr, mo, dy, hr, mn, sc;			
			if( result_user[4] )
			{
				ConvertDBTimeString( result_user[4], NULL, &yr, &mo, &dy, &hr, &mn, &sc );
				if( yr == 0 || mo == 0 || dy == 0 )
					user->dwFutTime_Updated_Password = 0;
				else
					user->dwFutTime_Updated_Password = CAL_FDATETIME_OF_YMDHMS( yr,mo,dy,hr,mn,sc) ;
			}
			else
				user->dwFutTime_Updated_Password = 0;
			if( result_user[5] )
			{
				ConvertDBTimeString( result_user[5], NULL, &yr, &mo, &dy, &hr, &mn, &sc );
				if( yr == 0 || mo == 0 || dy == 0 )
					user->dwFutTime = 0;
				else
					user->dwFutTime = CAL_FDATETIME_OF_YMDHMS( yr,mo,dy,hr,mn,sc) ;
			}
			else
				user->dwFutTime = 0;
			user->dwFutTime_Created = 0;
		}
		for( SQLRecordQueryf( odbc, NULL, &result_group, NULL, "select permission_group_id,name from permission_user join permission_group using(permission_group_id) where user_id=%s", result_user[0]  )
			 ; result_group
			  ; FetchSQLRecord( odbc, &result_group ) )
		{
			INDEX group_id = strtoul( result_group[0], NULL, 10 );
			PGROUP group = FindGroup( group_id, result_group[1] );
			if( !group->tokens )
			{
				PushSQLQueryEx( odbc );
				for( SQLRecordQueryf( odbc, NULL, &result_token, NULL, "select permission_id,name from permission_set join permission_tokens using(permission_id) where permission_group_id=%s order by permission_id", result_group[0]  )
					 ; result_token
					  ; FetchSQLRecord( odbc, &result_token ) )
				{
					INDEX token_id = strtoul( result_token[0], NULL, 10 );
					PTOKEN token = FindToken( token_id, result_token[1] );
					//lprintf( "Adding token %s to group %s", token->name, group->name );
					AddLink( &group->tokens, token );
				}
				PopODBCEx( odbc );
			}
			AddLink( &user->groups, group );
		}
		PopODBCEx( odbc );
		//lprintf( "Adding group %s to user %s", group->name, user->name );
	}	
}

PRELOAD( InitSQLPassword )
{
	CTEXTSTR *result;
	for( DoSQLRecordQueryf( NULL, &result, NULL
								 , "select permission_id, name from permission_tokens order by permission_id"
								 );
		  result;
		  GetSQLRecord( &result ) )
	{
		PTOKEN token = New( struct sql_token );
		token->name = StrDup( result[1] );
		token->id = atoi( result[0] );
		AddLink( &g.tokens, token );
		g.permission_count++; // should stay in sync with list...
	}
	ReloadUserCache( NULL );
	//ReadNameTableExx( WIDE("names"), WIDE("program_identifiers"), WIDE("program_id"), WIDE("menu login"), NULL );
	
	g.flags.bPrintAccountCreated = FutGetProfileInt( "PASSWORD", "Report Account Creation", 0 );

}

ATEXIT( CloseLogins )
{
   // is responsible for root logins, therefore close all on system.
   if( g.flags.bInitializeLogins )
		DoSQLCommandf( "update login_history set logout_whenstamp=now() where system_id=%d and logout_whenstamp=11111111111111", g.system_id );
}

#if ( __WATCOMC__ < 1291 )
PUBLIC( void, ExportThis )( void )
{
}
#endif
