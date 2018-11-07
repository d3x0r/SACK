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
#include "../intershell_registry.h"
#include "../intershell_export.h"

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
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), PERMISSIONS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/Security/SQL"), REQUIRED_PERMISSIONS, LISTBOX_CONTROL_NAME );
}

PNULL_PASSWORD GetButtonSecurity( uintptr_t button, int bCreate )
{
	PNULL_PASSWORD pls;
	INDEX idx;
	LIST_FORALL( g.secure_buttons, idx, PNULL_PASSWORD, pls )
	{
		if( pls->button == button )
			break;
	}
	if( !pls && bCreate )
	{
		pls = New( NULL_PASSWORD );
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

static uintptr_t CPROC AddButtonSecurity( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, permission );
	uintptr_t last_loading = psv;
	PNULL_PASSWORD pls = GetButtonSecurity( last_loading, TRUE );
	//lprintf( WIDE("load context %p(%p)"), pls, last_loading );
	if( pls )
	{
		int n;
		struct sql_token *token;		
		LIST_FORALL( g.permissions, n, struct sql_token *, token )
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

static void OnLoadSecurityContext( WIDE("NULL Password") )( PCONFIG_HANDLER pch )
{
   AddConfigurationMethod( pch, WIDE("SQL password security=%m"), AddButtonSecurity );
}

static void OnSaveSecurityContext( WIDE("NULL Password") )( FILE *file, uintptr_t button )
{
	PNULL_PASSWORD pls = GetButtonSecurity( button, FALSE );
   //lprintf( WIDE("save context %p"), pls );
	if( pls )
	{
		int n;
		struct sql_token *token;
		LIST_FORALL( g.permissions, n, struct sql_token *, token )
		{
			if( TESTFLAG( pls->permissions, n ) )
            sack_fprintf( file, WIDE("%sSQL password security=%s\n"), InterShell_GetSaveIndent(), token->name );
		}
	}
}

//--------------------------------------------------------------------------------

static uintptr_t TestSecurityContext( WIDE("NULL Password") )( uintptr_t button )
{	
	TEXTSTR current_user;
	INDEX result;	
	PNULL_PASSWORD pls = GetButtonSecurity( button, FALSE );
   //lprintf( WIDE("load context %p(%p)"), pls, button );

	g.current_user = NULL;
	if( current_user = NULL /*getCurrentUser() */)
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

	if( pls )
	{		
		result = 0;//PromptForPassword( &g.current_user, &g.current_user_login_id, NULL, pls->pTokens, pls->nTokens );
		return result;		
	}

	return 0; /* no security... */
}

static void  EndSecurityContext( WIDE("NULL Password") ) ( uintptr_t button, uintptr_t psv )
{
	//LogOutPassword( psv );
	return;	
}
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

void CPROC OnItemDoubleClickPermission( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
{
	int n;
	int already_picked = 0;
	PLISTITEM pli_picked;
	uintptr_t perm_selected = GetItemData( pli );
	PSI_CONTROL pc_required = GetNearControl( pc, REQUIRED_PERMISSIONS );

   /* check the already required list, see if something is picked...*/
	for( n = 0; pli_picked = GetNthItem( pc_required, n ); n++ )
	{
		uintptr_t nRequired = GetItemData( pli_picked );
		if( nRequired == perm_selected )
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

void CPROC OnItemDoubleClickRequired( uintptr_t psv, PSI_CONTROL pc, PLISTITEM pli )
{
   DeleteListItem( pc, pli );
}

//--------------------------------------------------------------------------------

static void OnEditSecurityContext( WIDE("NULL Password") )( uintptr_t button )
{
	PNULL_PASSWORD pls = GetButtonSecurity( button, TRUE );
	if( pls )
	{
		PSI_CONTROL frame = LoadXMLFrameOver( NULL
														, WIDE("EditSQLButtonSecurity.Frame") );
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


#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
