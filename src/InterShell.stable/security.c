
#include <sack_types.h>
#include <deadstart.h>
#include <pssql.h>
#include <filesys.h>
#include "intershell_local.h"
#include "intershell_registry.h"
#include "intershell_security.h"
#include "menu_real_button.h"
#include "resource.h"
#include "fonts.h"


/*
 * Menu Buttons extended security attributes.
 *
 *  When a button is pressed the flag bSecurity is check, and if set,
 *  then a security context by permission_context table index.
 *  The program_id will be set for
 *
 */



/* result with either INVALID_INDEX - permission denied
 or 0 - no token created
 or some other value that is passed to CloseSecurityContext when the button press is complete
 */
uintptr_t CreateSecurityContext( uintptr_t button )
{
      // add additional security plugin stuff...
      if( button )
		{
         uintptr_t psv_context;
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			for( name = GetFirstRegisteredName( TASK_PREFIX "/common/security/Test Security", &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				uintptr_t (CPROC*f)(uintptr_t);
				//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/security/save common/%s", name );
				f = GetRegisteredProcedure2( (CTEXTSTR)data, uintptr_t, name, (uintptr_t) );
				if( f )
				{
					psv_context = f( button );
					if( psv_context )
					{
						/* should probably record on per-plugin basis this value...... */
						return psv_context;
					}
				}
			}
		}
   return 0;
}

void CloseSecurityContext( uintptr_t button, uintptr_t psvSecurity )
{
      if( button )
		{
			//uintptr_t psv_context;
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			for( name = GetFirstRegisteredName( TASK_PREFIX "/common/security/Close Security", &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				void (CPROC*f)(uintptr_t,uintptr_t);
				//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/security/save common/%s", name );
				f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (uintptr_t,uintptr_t) );
				if( f )
				{
					f( button, psvSecurity );
				}
			}
		}
}

void AddSecurityContextToken( uintptr_t button, CTEXTSTR module, CTEXTSTR token )
{
	TEXTCHAR tmpname[256];
	void (CPROC*f)(uintptr_t,CTEXTSTR);
	snprintf( tmpname, 256, "intershell/common/security/Add Security Token/%s", module );
	f = GetRegisteredProcedure2( tmpname, void, "add_token", (uintptr_t,CTEXTSTR) );
	if( f )
		f( button, token );
}

void GetSecurityContextTokens( uintptr_t button, CTEXTSTR module, PLIST *ppList )
{
	TEXTCHAR tmpname[256];
	void (CPROC*f)(uintptr_t,PLIST*);
	snprintf( tmpname, 256, "intershell/common/security/Get Security Tokens/%s", module );
	f = GetRegisteredProcedure2( tmpname, void, "get_tokens", (uintptr_t,PLIST*) );
	if( f )
		f( button, ppList );
}

void GetSecurityModules( PLIST *ppList )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	EmptyList( ppList );
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/security/Get Security Tokens", &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		AddLink( ppList, name );
	}
}

void CPROC SelectEditSecurity( uintptr_t psv, PSI_CONTROL listbox, PLISTITEM pli )
{
	{
		CTEXTSTR name;
		//TEXTCHAR invoke[256];
		void (CPROC*f)(uintptr_t);
		name = (CTEXTSTR)GetItemData( pli );
		f = GetRegisteredProcedure2( (CTEXTSTR)"intershell/common/security/Edit Security", void, name, (uintptr_t) );
		if( f )
			f( (uintptr_t)psv );
	}
}


void CPROC EditSecurity( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL pc_list;
   	PLISTITEM pli;
	pc_list = GetNearControl( button, LISTBOX_SECURITY_MODULE );
	pli = GetSelectedItem( pc_list );
	if( pli )
	{
      		SelectEditSecurity( psv, NULL, pli );
	}
	else
	{
      		SimpleMessageBox( button, "No selected security module", "No Selection" );
	}
}


void CPROC EditSecurityNoList( uintptr_t psv, PSI_CONTROL button )
{
	SimpleMessageBox( button, "No listbox to select security module", "NO SECURITY LIST" );

}

void SetupSecurityEdit( PSI_CONTROL frame, uintptr_t object_to_secure )
		{
			PSI_CONTROL listbox;
         //lprintf( "Setting up security edit for %p", object_to_secure );
			listbox = GetControl( frame, LISTBOX_SECURITY_MODULE );
			if( listbox )
			{
				INDEX idx;
				CTEXTSTR name;
				ResetList( listbox );
				LIST_FORALL( g.security_property_names, idx, CTEXTSTR, name )
				{
					SetItemData( AddListItem( listbox, name ), (uintptr_t)name );
				}
				SetDoubleClickHandler( listbox, SelectEditSecurity, object_to_secure );
				SetButtonPushMethod( GetControl( frame, EDIT_SECURITY ), EditSecurity, object_to_secure );
			}
			else
			{
				SetButtonPushMethod( GetControl( frame, EDIT_SECURITY ), EditSecurityNoList, 0 );
			}
		}


static void OnGlobalPropertyEdit( "Set Edit Permissions" )( PSI_CONTROL parent )
{
	PSI_CONTROL pc = LoadXMLFrameOver( parent, "SetEditPermissions.isFrame" );
	if( pc )
	{
		int okay = 0;
		int done = 0;
		SetupSecurityEdit( pc, (uintptr_t)parent );
		SetCommonButtons( pc, &done, &okay );
		DisplayFrameOver( pc, parent );
		CommonWait( pc );
		DestroyFrame( &pc );
	}
}

void InterShell_ReloadSecurityInformation( PCONFIG_HANDLER pch )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	//lprintf( "Gave control a chance to register additional security methods on current config..." );
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/security/Load Security", &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(PCONFIG_HANDLER);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/security/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (PCONFIG_HANDLER) );
		if( f )
			f( pch );
	}
}

void InterShell_SaveSecurityInformation( FILE *file, uintptr_t psv )
{
	// add additional security plugin stuff...
	CTEXTSTR name;
	PCLASSROOT data = NULL;
   //lprintf( "Save existing config for %p", psv );
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/security/Save Security", &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(FILE*,uintptr_t);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/security/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (FILE*,uintptr_t) );
		if( f )
			f( file, psv );
	}
}



static void OnSaveCommon( "@10 EditSecurity" )( FILE *file )
{
	sack_fprintf( file, "Begin Edit Permissions\n" );
	InterShell_SaveSecurityInformation( file, (uintptr_t)InterShell_GetCurrentSavingCanvas() );
}

static uintptr_t CPROC BeginGlobalEditPerms( uintptr_t psv, arg_list args )
{
	//lprintf( "Setting psv to single_frame %p, adding security plugin rules", InterShell_GetCurrentLoadingCanvas() );
	InterShell_ReloadSecurityInformation( InterShell_GetCurrentConfigHandler() );
	// change/set what the psv is for global paramters.
	return (uintptr_t)InterShell_GetCurrentLoadingCanvas();
}

static void OnLoadCommon( "@10 EditSecurity" )( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, "Begin Edit Permissions", BeginGlobalEditPerms );
}

