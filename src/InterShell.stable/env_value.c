#include <filesys.h>
#include "widgets/include/banner.h"
#include "intershell_export.h"
#include "intershell_registry.h"



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Begin button method that can Set Environment text...
//  useful for setting in macros to indicate current task mode? or perhaps
//  status messages to indicate what needs to be done now?
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

enum { BTN_VARNAME = 1322
	  , BTN_VARVAL
	  , CHECKBOX_USER_VARIABLE
};


struct button_set_text {
	TEXTSTR varname;
	TEXTSTR newval;
	LOGICAL bUser;
};
typedef struct button_set_text SETVAR;
typedef struct button_set_text *PSETVAR;

PRELOAD( AliasPageTitle )
{
	EasyRegisterResource( WIDE( "intershell/Set Environment" ), BTN_VARNAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE( "intershell/Set Environment" ), BTN_VARVAL, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE( "intershell/Set Environment" ), CHECKBOX_USER_VARIABLE, RADIO_BUTTON_NAME );
}

static void SetEnvVariable( CTEXTSTR name, CTEXTSTR value, LOGICAL bUser )
{
#ifdef _WIN32
	DWORD dwStatus;
	HKEY hTemp;

	// user environment
	//HKEY_CURRENT_USER\Environment
	// system environment
	//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
	if( bUser )
		dwStatus = RegOpenKeyEx( HKEY_CURRENT_USER,
								WIDE("Environment"), 0,
										KEY_WRITE, &hTemp );
	else
		dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		 						WIDE("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), 0,
										KEY_WRITE, &hTemp );

	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		//lprintf( WIDE("Write shell to: %s"), my_button->shell );
		dwStatus = RegSetValueEx(hTemp, name, 0
                                , REG_SZ
                                , (BYTE*)value, strlen( value ) );
		RegCloseKey( hTemp );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			Banner2Message( WIDE("Failed to set environment variable") );
	}
	else
	{
		if( bUser )
			Banner2Message( WIDE("Failed to open User environment registry") );
		else
			Banner2Message( WIDE("Failed to open System environment registry") );
	}
#endif
}

static void OnKeyPressEvent( WIDE( "InterShell/Set Environment" ) )( uintptr_t psv )
{
	PSETVAR pSetVar = (PSETVAR)psv;
	SetEnvVariable( pSetVar->varname, pSetVar->newval, pSetVar->bUser );

	//return 1;
}

static uintptr_t OnCreateMenuButton( WIDE( "InterShell/Set Environment" ) )( PMENU_BUTTON button )
{
	PSETVAR pSetVar = New( SETVAR );
	pSetVar->varname = NULL;
	pSetVar->newval = NULL;
	InterShell_SetButtonStyle( button, WIDE( "bicolor square" ) );

	return (uintptr_t)pSetVar;
}

static uintptr_t OnConfigureControl( WIDE( "InterShell/Set Environment" ) )( uintptr_t psv, PSI_CONTROL parent )
{
	PSETVAR pSetVar = (PSETVAR)psv;
	PSI_CONTROL frame;
	frame = LoadXMLFrameOver( parent, WIDE( "configure_environment_setvar_button.isframe" ) );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetControlText( GetControl( frame, BTN_VARNAME ), pSetVar->varname );
		SetControlText( GetControl( frame, BTN_VARVAL ), pSetVar->newval );
		SetCheckState( GetControl( frame, CHECKBOX_USER_VARIABLE ), pSetVar->bUser );
		SetCommonButtons( frame, &done, &okay );
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR buffer[256];
			GetControlText( GetControl( frame, BTN_VARNAME ), buffer, sizeof( buffer )  );
			if( ( pSetVar->varname && strcmp( pSetVar->varname, buffer ) ) || ( !pSetVar->varname && buffer[0] ) )
			{
				Release( pSetVar->varname );
				pSetVar->varname = StrDup( buffer );
			}
			GetControlText( GetControl( frame, BTN_VARVAL ), buffer, sizeof( buffer ) );
			if( ( pSetVar->newval && strcmp( pSetVar->newval, buffer ) ) || ( !pSetVar->newval && buffer[0] ) )
			{
				Release( pSetVar->newval );
				pSetVar->newval = StrDup( buffer );
			}
			pSetVar->bUser = GetCheckState( GetControl( frame, CHECKBOX_USER_VARIABLE ) );
		}
		DestroyFrame( &frame );
	}
	return psv;
}

static void OnSaveControl( WIDE( "InterShell/Set Environment" ) )( FILE *file, uintptr_t psv )
{
	PSETVAR pSetVar = (PSETVAR)psv;
	sack_fprintf( file, WIDE( "Set Environment User=%s\n" ), pSetVar->bUser?WIDE("yes"):WIDE("no") );
	sack_fprintf( file, WIDE( "Set Environment text name=%s\n" ), EscapeMenuString( pSetVar->varname ) );
	sack_fprintf( file, WIDE( "Set Environment text value=%s\n" ), EscapeMenuString( pSetVar->newval ) );
}

static void OnCloneControl( WIDE( "InterShell/Set Environment" ) )( uintptr_t psvNew, uintptr_t psvOld )
{
	PSETVAR pSetVarNew = (PSETVAR)psvNew;
	PSETVAR pSetVarOld = (PSETVAR)psvOld;
	pSetVarNew->varname = StrDup( pSetVarOld->varname );
	pSetVarNew->newval = StrDup( pSetVarOld->newval );
	pSetVarNew->bUser = pSetVarOld->bUser;
}

static uintptr_t CPROC SetVariableVariableName( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PSETVAR pSetVar = (PSETVAR)psv;
	pSetVar->varname = StrDup( name );
	return psv;
}

static uintptr_t CPROC SetVariableVariableValue( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PSETVAR pSetVar = (PSETVAR)psv;
	pSetVar->newval = StrDup( name );
	return psv;
}

static uintptr_t CPROC SetVariableUser( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bUser );
	PSETVAR pSetVar = (PSETVAR)psv;
	pSetVar->bUser = bUser;
	return psv;
}

static void OnLoadControl( WIDE( "InterShell/Set Environment" ) )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch,  WIDE( "Set Environment user=%b" ), SetVariableUser );
	AddConfigurationMethod( pch,  WIDE( "Set Environment text name=%m" ), SetVariableVariableName );
	AddConfigurationMethod( pch,  WIDE( "Set Environment text value=%m" ), SetVariableVariableValue );
}
