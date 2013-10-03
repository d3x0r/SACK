///////////////////////////////////////////////////////////////////////////
//
//   null_password.c
//   (C) Copyright 2012 Freedom Collective
//   Written by Jim Buckeyne 
//
//                   
////////////////////////////////////////////////////////////////////////////

#define DEFINE_DEFAULT_RENDER_INTERFACE

#define NULL_PASSWORD_MAIN
#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE

#include <stdhdrs.h>
//#include <getoption.h>
#include "InterShell_registry.h"
#include "InterShell_export.h"

/* get permisison creation time option... */
#include "global.h"

enum {
   BTN_PASSKEY = 10000
	  , PERMISSIONS = BTN_PASSKEY + 50
	  , REQUIRED_PERMISSIONS
     , PASSCODE
};

PRELOAD( RegisterUserPasswordControls )
{
	EasyRegisterResource( "InterShell/Security/SQL", PERMISSIONS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "InterShell/Security/SQL", REQUIRED_PERMISSIONS, LISTBOX_CONTROL_NAME );
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
		pls->button = button;
		pls->psv = 0; /* might be used... but not yet */
		pls->pTokens = NULL;		
		pls->nTokens = 0;
		pls->permissions = NewArray( FLAGSETTYPE, FLAGSETSIZE( FLAGSETTYPE, g.permission_count ) );
		MemSet( pls->permissions, 0, FLAGSETSIZE( FLAGSETTYPE, g.permission_count ) );
		AddLink( &g.secure_buttons, pls );
	}
   return pls;
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
		struct sql_token *token;		
		LIST_FORALL( g.permissions, n, struct sql_token *, token )
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

static void OnLoadSecurityContext( "NULL Password" )( PCONFIG_HANDLER pch )
{
   AddConfigurationMethod( pch, "SQL password security=%m", AddButtonSecurity );
}

static void OnSaveSecurityContext( "NULL Password" )( FILE *file, PTRSZVAL button )
{
	PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
   //lprintf( "save context %p", pls );
	if( pls )
	{
		int n;
		struct sql_token *token;
		LIST_FORALL( g.permissions, n, struct sql_token *, token )
		{
			if( TESTFLAG( pls->permissions, n ) )
            fprintf( file, "%sSQL password security=%s\n", InterShell_GetSaveIndent(), token->name );
		}
	}
}

//--------------------------------------------------------------------------------

static PTRSZVAL TestSecurityContext( "NULL Password" )( PTRSZVAL button )
{	
	TEXTSTR current_user;
	INDEX result;	
	PSQL_PASSWORD pls = GetButtonSecurity( button, FALSE );
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

	if( pls )
	{		
		result = PromptForPassword( &g.current_user, &g.current_user_login_id, NULL, pls->pTokens, pls->nTokens );
		return result;		
	}

	return 0; /* no security... */
}

static void  EndSecurityContext( "NULL Password" ) ( PTRSZVAL button, PTRSZVAL psv )
{
	LogOutPassword( psv );
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
										, ((struct sql_token*)GetLink( &g.permissions, perm_selected ))->name ), perm_selected );
	}

}

//--------------------------------------------------------------------------------

void CPROC OnItemDoubleClickRequired( PTRSZVAL psv, PSI_CONTROL pc, PLISTITEM pli )
{
   DeleteListItem( pc, pli );
}

//--------------------------------------------------------------------------------

static void OnEditSecurityContext( "NULL Password" )( PTRSZVAL button )
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
			{
				PSI_CONTROL list;
				list = GetControl( frame, PERMISSIONS );
				if( list )
				{
					int n;
					struct sql_token *token;
					SetDoubleClickHandler( list, OnItemDoubleClickPermission, 0 );
					LIST_FORALL( g.permissions, n, struct sql_token *, token )
					{
						SetItemData( AddListItem( list, token->name ), n );
					}
				}
				list = GetControl( frame, REQUIRED_PERMISSIONS );
				if( list )
				{
					int n;
					struct sql_token *token;
					SetDoubleClickHandler( list, OnItemDoubleClickRequired, 0 );

					LIST_FORALL( g.permissions, n, struct sql_token *, token )
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
				{
					PSI_CONTROL list;
					list = GetControl( frame, REQUIRED_PERMISSIONS );
					if( list )
					{
						int n;
						struct sql_token *token;
						PLISTITEM pli;
						LIST_FORALL( g.permissions, n, struct sql_token *, token )
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


#if ( __WATCOMC__ < 1291 )
PUBLIC( void, ExportThis )( void )
{
}
#endif
