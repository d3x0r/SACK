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
#include <sqlgetoption.h>
#include <pssql.h>
#include <sha1.h>
#include "../widgets/include/banner.h"
#include "../intershell_registry.h"
#include "../intershell_export.h"

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
	CTEXTSTR permission_tokens = WIDE("CREATE TABLE `permission_tokens` (")
	WIDE("`permission_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`log` int(11) NOT NULL DEFAULT '0',")
	WIDE("`description` varchar(255) DEFAULT '',")
	WIDE("PRIMARY KEY (`permission_id`),")
	WIDE("KEY `token` (`name`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=827 DEFAULT CHARSET=latin1 COMMENT='Name detail of permission_id'");

	CTEXTSTR permission_set = WIDE("CREATE TABLE `permission_set` (")
	WIDE("`permission_group_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`permission_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("PRIMARY KEY (`permission_group_id`,`permission_id`),")
	WIDE("KEY `group` (`permission_group_id`),")
	WIDE("KEY `token` (`permission_id`)")
	WIDE(") ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='relate permission_group_id to permission_id'");

	CTEXTSTR permission_group = WIDE("CREATE TABLE `permission_group` (")
	WIDE("`permission_group_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`hall_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`dummy_timestamp` timestamp,")
	WIDE("`charity_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`description` varchar(255) DEFAULT '',")
	WIDE("PRIMARY KEY (`permission_group_id`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=39 DEFAULT CHARSET=latin1 COMMENT='USAGE: Groups in which tokens are assigned to & then groups '");

	CTEXTSTR permission_user = WIDE("CREATE TABLE `permission_user` (")
	WIDE("`permission_group_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`user_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("PRIMARY KEY (`permission_group_id`,`user_id`),")
	WIDE("KEY `user` (`user_id`),")
	WIDE("KEY `group` (`permission_group_id`)")
	WIDE(") ENGINE=MyISAM DEFAULT CHARSET=latin1 COMMENT='relate permission_group_id to user_id'");

	CTEXTSTR permission_user_info = WIDE("CREATE TABLE `permission_user_info` (")
	WIDE("`user_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`hall_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`charity_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`default_room_id` int(11) unsigned NOT NULL DEFAULT '0',")
	WIDE("`first_name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`last_name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`system_user_name` varchar(255) DEFAULT '',")
	WIDE("`staff_id` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`pin` varchar(42) DEFAULT NULL,")
	WIDE("`password` varchar(42) NOT NULL DEFAULT '',")
	WIDE("`password_creation_datestamp` date DEFAULT NULL,")
	WIDE("`key_code` varchar(100) DEFAULT NULL,")
	WIDE("`terminate` int(11) DEFAULT '0',")
	WIDE("`lot_container` tinyint(4) unsigned NOT NULL DEFAULT '0',")
	WIDE("`locale_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`card` varchar(25) DEFAULT NULL,")
	WIDE("PRIMARY KEY (`user_id`),")
	WIDE("KEY `PINsearch` (`pin`),")
	WIDE("KEY `permission_user_info_terminate_idx` (`terminate`),")
	WIDE("KEY `permuserinfo_locale_idx` (`locale_id`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=22 DEFAULT CHARSET=latin1 COMMENT='USAGE: Stores all users in all systems, there passwords, ids'");

	CTEXTSTR permission_user_password = WIDE("CREATE TABLE `permission_user_password` (")
	WIDE("`user_password_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`user_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`password` varchar(42) NOT NULL DEFAULT '',")
	WIDE("`description` varchar(255) NOT NULL DEFAULT '',")
	WIDE("`creation_datestamp` timestamp,")
	WIDE("PRIMARY KEY (`user_password_id`),")
	WIDE("KEY `user_password_key` (`password`,`user_id`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=23 DEFAULT CHARSET=latin1");

	CTEXTSTR permission_user_type = WIDE("CREATE TABLE `permission_user_type` (")
	WIDE("`permission_user_type_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`permission_user_type_name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("`permission_token_name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("PRIMARY KEY (`permission_user_type_id`)")
	WIDE(") ENGINE=MyISAM DEFAULT CHARSET=latin1");

	CTEXTSTR system_exception_type = WIDE("CREATE TABLE `system_exception_type` (")
	WIDE("`system_exception_type_id` int(11) NOT NULL,")
	WIDE("`system_exception_type_name` varchar(255) NOT NULL,")
	WIDE("PRIMARY KEY (`system_exception_type_id`)")
	WIDE(") ENGINE=InnoDB DEFAULT CHARSET=latin1");

	CTEXTSTR system_exceptions = WIDE("CREATE TABLE `system_exceptions` (")
	WIDE("`system_exception_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`system_exception_category_id` int(11) NOT NULL,")
	WIDE("`system_exception_type_id` int(11) NOT NULL,")
	WIDE("`user_id` int(11) DEFAULT NULL,")
	WIDE("`system_id` int(11) DEFAULT NULL,")
	WIDE("`program_id` int(11) DEFAULT NULL,")
	WIDE("`initial_value` tinytext,")
	WIDE("`new_value` tinytext,")
	WIDE("`description` tinytext,")
	WIDE("`log_whenstamp` timestamp,")
	WIDE("PRIMARY KEY (`system_exception_id`)")
	WIDE(") ENGINE=InnoDB AUTO_INCREMENT=283 DEFAULT CHARSET=latin1");

	CTEXTSTR system_exception_category = WIDE("CREATE TABLE `system_exception_category` (")
	WIDE("`system_exception_category_id` int(11) NOT NULL,")
	WIDE("`system_exception_category_name` varchar(255) NOT NULL,")
	WIDE("PRIMARY KEY (`system_exception_category_id`)")
	WIDE(") ENGINE=InnoDB DEFAULT CHARSET=latin1");

	CTEXTSTR program_identifiers = WIDE("CREATE TABLE `program_identifiers` (")
	WIDE("`program_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`program_name` varchar(100) NOT NULL DEFAULT '',")
	WIDE("PRIMARY KEY (`program_id`),")
	WIDE("KEY `prog_name` (`program_name`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=61 DEFAULT CHARSET=latin1");

	CTEXTSTR login_history = WIDE("CREATE TABLE `login_history` (")
	WIDE("`login_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`actual_login_id` int(11) DEFAULT '0',")
	WIDE("`fiscalday` date DEFAULT NULL,")
	WIDE("`shift` int(11) DEFAULT '0',")
	WIDE("`system_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`program_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`user_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`group_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`hall_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`charity_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`login_whenstamp` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',")
	WIDE("`logout_whenstamp` datetime NOT NULL DEFAULT '1111-11-11 11:11:11',")
	WIDE("PRIMARY KEY (`login_id`),")
	WIDE("KEY `shiftkey` (`shift`),")
	WIDE("KEY `fiscalday` (`fiscalday`,`shift`,`user_id`),")
	WIDE("KEY `loginkey` (`login_id`),")
	WIDE("KEY `prog` (`program_id`),")
	WIDE("KEY `logout` (`logout_whenstamp`),")
	WIDE("KEY `3columns` (`system_id`,`program_id`,`logout_whenstamp`),")
	WIDE("KEY `login_hist_idx` (`logout_whenstamp`,`program_id`,`system_id`),")
	WIDE("KEY `login_history_user_id_idx` (`user_id`),")
	WIDE("KEY `login_history_actual_login_id_idx` (`actual_login_id`),")
	WIDE("KEY `login_history_actual_login_whenstamp_idx` (`login_whenstamp`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=3830 DEFAULT CHARSET=latin1 COMMENT='USAGE: keeps track of all logins from all systems and progam'");

	CTEXTSTR systems = WIDE("CREATE TABLE `systems` (")
	WIDE("`system_id` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`name` varchar(64) NOT NULL DEFAULT '',")
	WIDE("`address` varchar(64) NOT NULL DEFAULT '',")
	WIDE("`login_failure_count` int(11) NOT NULL DEFAULT '0',")
	WIDE("`login_failure_lockout_until` datetime DEFAULT NULL,")
	WIDE("PRIMARY KEY (`system_id`),")
	WIDE("KEY `namekey` (`name`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=32 DEFAULT CHARSET=latin1 COMMENT='address must be determined by running the User()'");

	CTEXTSTR permission_user_log = WIDE("CREATE TABLE `permission_user_log` (")
	WIDE("`log_ID` int(11) NOT NULL AUTO_INCREMENT,")
	WIDE("`login_id` int(11) DEFAULT '0',")
	WIDE("`description` varchar(255) DEFAULT NULL,")
	WIDE("`permission_id` int(11) NOT NULL DEFAULT '0',")
	WIDE("`logtype` char(48) NOT NULL DEFAULT '0',")
	WIDE("`log_whenstamp` datetime DEFAULT NULL,")
	WIDE("PRIMARY KEY (`log_ID`),")
	WIDE("KEY `permission_user_log_login_id_idx` (`login_id`)")
	WIDE(") ENGINE=MyISAM AUTO_INCREMENT=11031 DEFAULT CHARSET=latin1 COMMENT='USAGE: Logs systems access'");
	
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
		if( DoSQLRecordQueryf( NULL, &result, NULL, WIDE("show create table permission_user_log") ) && result && result[1] )
			if( StrStr( result[1], WIDE("enum") ) )
			{
				DoSQLCommandf( WIDE("ALTER TABLE `permission_user_log` MODIFY COLUMN `logtype` CHAR(48) CHARACTER SET latin1 COLLATE latin1_swedish_ci NOT NULL DEFAULT 0") );
			}
	}

	EasyRegisterResourceRange( WIDE("User Keypad"), BTN_PASSKEY, 36, NORMAL_BUTTON_NAME );
	//EasyRegisterResourceRange( WIDE("User Keypad"), BTN_PASSKEY, 36, CUSTOM_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), PERMISSIONS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("User Keypad"), PASSCODE, STATIC_TEXT_NAME );
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), REQUIRED_PERMISSIONS, LISTBOX_CONTROL_NAME );

   EasyRegisterResource( WIDE("InterShell/Security/SQL"), CHECKBOX_REQUIRE_PARENT_LOGIN, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), CHECKBOX_OVERRIDE_PARENT_REQUIRED, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), TEXT_EDIT_REQUIRED_PERMISSION, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), TEXT_EDIT_OVERRIDE_REQUIRED_PERMISSION, EDIT_FIELD_NAME );

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
			Release( (POINTER)*target );
      lprintf( WIDE("New permission is %s"), permission );
		if( permission && permission[0] )
		{
			(*target) = StrDup( permission );
			LIST_FORALL( g.tokens, n, PTOKEN , token )
			{
            lprintf( WIDE("is [%s]==[%s]"), permission, token->name );
				if( StrCaseCmp( permission, token->name ) == 0 )
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
	//lprintf( WIDE("load context %p(%p)"), pls, last_loading );
	if( pls )
	{
		int n;
		PTOKEN token;		
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( StrCaseCmp( permission, token->name ) == 0 )
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
	lprintf( WIDE("load context %p(%p)"), pls, last_loading );
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
	//lprintf( WIDE("load context %p(%p)"), pls, last_loading );
	if( pls )
	{
      ResolveToken( &pls->override_required_token_token, &pls->override_required_token, permission );
	}	
	return psv;
}

static void OnAddSecurityContextToken( WIDE("SQL Password") )( PTRSZVAL context, CTEXTSTR permission )
{
	PSQL_PASSWORD pls = GetButtonSecurity( context, TRUE );
	//lprintf( WIDE("load context %p(%p)"), pls, context );
	if( pls )
	{
		int n;
		PTOKEN token;		
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( StrCaseCmp( permission, token->name ) == 0 )
			{				
				pls->pTokens = Reallocate( pls->pTokens, sizeof( TEXTSTR ) * (++pls->nTokens) );				
				pls->pTokens[pls->nTokens-1] = StrDup( token->name );				
				SETFLAG( pls->permissions, n );
				break;
			}
		}
	}	
}

static void OnGetSecurityContextTokens( WIDE("SQL Password") )( PTRSZVAL context, PLIST *list )
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


static void OnLoadSecurityContext( WIDE("SQL Password") )( PCONFIG_HANDLER pch )
{
   AddConfigurationMethod( pch, WIDE("SQL password security=%m"), AddButtonSecurity );
   AddConfigurationMethod( pch, WIDE("SQL password required login=%m"), AddButtonSecurityRequire );
   AddConfigurationMethod( pch, WIDE("SQL password required login override=%m"), AddButtonSecurityOverride );
}

static void OnSaveSecurityContext( WIDE("SQL Password") )( FILE *file, PTRSZVAL button )
{
	PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
   //lprintf( WIDE("save context %p"), pls );
	if( pls )
	{
		int n;
		PTOKEN token;
		LIST_FORALL( g.tokens, n, PTOKEN , token )
		{
			if( TESTFLAG( pls->permissions, n ) )
            fprintf( file, WIDE("%sSQL password security=%s\n"), InterShell_GetSaveIndent(), token->name );
		}
		if( pls->required_token && pls->required_token[0] )
		{
			fprintf( file, WIDE("%sSQL password required login=%s\n"), InterShell_GetSaveIndent(), pls->required_token );
			if( pls->override_required_token && pls->override_required_token[0] )
				fprintf( file, WIDE("%sSQL password required login override=%s\n"), InterShell_GetSaveIndent(), pls->override_required_token );
		}
	}
}

//--------------------------------------------------------------------------------

static PTRSZVAL TestSecurityContext( WIDE("SQL Password") )( PTRSZVAL button )
{	
	TEXTSTR current_user;
	PTRSZVAL result;
	PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
	if( pls )
	{
   		//lprintf( WIDE("load context %p(%p)"), pls, button );
   	
		g.current_user = NULL;
		if( current_user = getCurrentUser() )
		{
			PUSER user;
			INDEX idx;	
			lprintf( WIDE("Have a current user?!") );
			LIST_FORALL( g.users, idx, PUSER, user )
			{
				if( StrCmp( user->name, current_user ) == 0 )
				{
					lprintf( WIDE("Setting current user to ....?") );
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

static void  EndSecurityContext( WIDE("SQL Password") ) ( PTRSZVAL button, PTRSZVAL psv )
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

static void OnEditSecurityContext( WIDE("SQL Password") )( PTRSZVAL button )
{
	PSQL_PASSWORD pls = GetButtonSecurity( button, TRUE );
	if( pls )
	{
		PSI_CONTROL frame = LoadXMLFrameOver( NULL
														, WIDE("EditSQLButtonSecurity.Frame") );
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
				lprintf( WIDE("Token ID does not match (multiple definition?!") );
			}
			break;
		}
	}
	if( !token )
	{
		lprintf( WIDE("Added new token %d(%s)"), id, name );
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
				lprintf( WIDE("Group ID does not match (multiple definition?!") );
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
	lprintf(WIDE(" UnloadUserCache"));
	UnloadUserCache();
	lprintf(WIDE(" ReloadUserCache"));

	for( SQLRecordQueryf( odbc, NULL, &result_group, NULL, WIDE("select permission_group_id,name from permission_group")  )
		; result_group
		; FetchSQLRecord( odbc, &result_group ) )
	{
		INDEX group_id = IntCreateFromText( result_group[0] );
		PGROUP group = FindGroup( group_id, result_group[1] );
	}


	for( SQLRecordQueryf( odbc, NULL, &result_user, NULL
							  , WIDE("select user_id,first_name,last_name,name,password_creation_datestamp,date_add(password_creation_datestamp,interval %d day),staff_id from permission_user_info order by first_name")
							  , g.pass_expr_interval )
		 ; result_user
		  ; FetchSQLRecord( odbc, &result_user ) )
	{
		INDEX user_id = IntCreateFromText( result_user[0] );
		PUSER user = FindUser( user_id );
		PushSQLQueryEx( odbc );

		//
		if( result_user[6] && result_user[6][0] != '\0' )	
			user->staff = StrDup( result_user[6] );			

		else
			user->staff = WIDE(" ");
			

		//
		if( result_user[1] && result_user[1][0] != '\0' )	
			user->first_name = StrDup( result_user[1] );

		else
			user->first_name = WIDE(" ");

		//
		if( result_user[2] && result_user[2][0] != '\0' )
			user->last_name = StrDup( result_user[2] );			

		else
			user->last_name = WIDE(" ");
		
		//
		if( result_user[3] && result_user[3][0] != '\0' )
			user->name = StrDup( result_user[3] );
			
		else
			user->name = WIDE(" ");
				
		// first, last [1], [2]
		{
			int len;
			len = StrLen( user->first_name ) + StrLen( user->last_name ) + StrLen( user->staff ) + 5;
			user->full_name = NewArray( TEXTCHAR, len );
			snprintf( user->full_name, len, WIDE("%s\t%s\t(%s)"), user->first_name, user->last_name, user->staff );
		}
		
		{
			int yr, mo, dy, hr, mn, sc;			
			if( result_user[4] )
			{
				ConvertDBTimeString( result_user[4], NULL, &yr, &mo, &dy, &hr, &mn, &sc );
				if( yr == 0 || mo == 0 || dy == 0 )
					user->dwFutTime_Updated_Password = 0;
				else
				{
               lprintf(WIDE(" not saving time result...") );
					user->dwFutTime_Updated_Password = 1;
				}
			}
			else
				user->dwFutTime_Updated_Password = 0;
			if( result_user[5] )
			{
				ConvertDBTimeString( result_user[5], NULL, &yr, &mo, &dy, &hr, &mn, &sc );
				if( yr == 0 || mo == 0 || dy == 0 )
					user->dwFutTime = 0;
				else
				{
               lprintf(WIDE(" not saving time result...") );
					user->dwFutTime = 1;
				}
			}
			else
				user->dwFutTime = 0;
			user->dwFutTime_Created = 0;
		}
		for( SQLRecordQueryf( odbc, NULL, &result_group, NULL, WIDE("select permission_group_id,name from permission_user join permission_group using(permission_group_id) where user_id=%s"), result_user[0]  )
			 ; result_group
			  ; FetchSQLRecord( odbc, &result_group ) )
		{
			INDEX group_id = IntCreateFromText( result_group[0] );
			PGROUP group = FindGroup( group_id, result_group[1] );
			if( !group->tokens )
			{
				PushSQLQueryEx( odbc );
				for( SQLRecordQueryf( odbc, NULL, &result_token, NULL, WIDE("select permission_id,name from permission_set join permission_tokens using(permission_id) where permission_group_id=%s order by permission_id"), result_group[0]  )
					 ; result_token
					  ; FetchSQLRecord( odbc, &result_token ) )
				{
					INDEX token_id = IntCreateFromText( result_token[0] );
					PTOKEN token = FindToken( token_id, result_token[1] );
					//lprintf( WIDE("Adding token %s to group %s"), token->name, group->name );
					AddLink( &group->tokens, token );
				}
				PopODBCEx( odbc );
			}
			AddLink( &user->groups, group );
		}
		PopODBCEx( odbc );
		//lprintf( WIDE("Adding group %s to user %s"), group->name, user->name );
	}	
}

PRELOAD( InitSQLPassword )
{
	CTEXTSTR *result;
	for( DoSQLRecordQueryf( NULL, &result, NULL
								 , WIDE("select permission_id, name from permission_tokens order by permission_id")
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
	//ReadNameTableExx( WIDE(WIDE("names")), WIDE(WIDE("program_identifiers")), WIDE(WIDE("program_id")), WIDE(WIDE("menu login")), NULL );
	
	g.flags.bPrintAccountCreated = SACK_GetProfileInt( WIDE("PASSWORD"), WIDE("Report Account Creation"), 0 );

}

ATEXIT( CloseLogins )
{
   // is responsible for root logins, therefore close all on system.
   if( g.flags.bInitializeLogins )
		DoSQLCommandf( WIDE("update login_history set logout_whenstamp=now() where system_id=%d and logout_whenstamp=11111111111111"), g.system_id );
}

#if ( __WATCOMC__ < 1291 )
PUBLIC( void, ExportThis )( void )
{
}
#endif
