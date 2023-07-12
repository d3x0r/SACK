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

enum { BTN_ENV_VARNAME = 1322
	  , BTN_ENV_VARVAL
	  , CHECKBOX_USER_VARIABLE
};


struct env_button_set_text {
	TEXTSTR varname;
	TEXTSTR newval;
	LOGICAL bUser;
};
typedef struct env_button_set_text SETENV;
typedef struct env_button_set_text *PSETENV;

PRELOAD( EnvironmentVariableSetup )
{
	EasyRegisterResource( "intershell/Set Environment", BTN_ENV_VARNAME, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/Set Environment", BTN_ENV_VARVAL, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/Set Environment", CHECKBOX_USER_VARIABLE, RADIO_BUTTON_NAME );
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
								"Environment", 0,
										KEY_WRITE, &hTemp );
	else
		dwStatus = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
		 						"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0,
										KEY_WRITE, &hTemp );

	if( dwStatus == ERROR_FILE_NOT_FOUND )
	{
	}
	if( (dwStatus == ERROR_SUCCESS) && hTemp )
	{
		//lprintf( "Write shell to: %s", my_button->shell );
		dwStatus = RegSetValueEx(hTemp, name, 0
                                , REG_SZ
                                , (BYTE*)value, (DWORD)strlen( value ) );
		RegCloseKey( hTemp );
		if( dwStatus == ERROR_SUCCESS )
		{
		}
		else
			Banner2Message( "Failed to set environment variable" );
	}
	else
	{
		if( bUser )
			Banner2Message( "Failed to open User environment registry" );
		else
			Banner2Message( "Failed to open System environment registry" );
	}
#endif
}

static void OnKeyPressEvent( "InterShell/Set Environment" )( uintptr_t psv )
{
	PSETENV pSetVar = (PSETENV)psv;
	SetEnvVariable( pSetVar->varname, pSetVar->newval, pSetVar->bUser );

	//return 1;
}

static uintptr_t OnCreateMenuButton( "InterShell/Set Environment" )( PMENU_BUTTON button )
{
	PSETENV pSetVar = New( SETENV );
	pSetVar->varname = NULL;
	pSetVar->newval = NULL;
	InterShell_SetButtonStyle( button, "bicolor square" );

	return (uintptr_t)pSetVar;
}

static uintptr_t OnConfigureControl( "InterShell/Set Environment" )( uintptr_t psv, PSI_CONTROL parent )
{
	PSETENV pSetVar = (PSETENV)psv;
	PSI_CONTROL frame;
	frame = LoadXMLFrameOver( parent, "configure_environment_setvar_button.isframe" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetControlText( GetControl( frame, BTN_ENV_VARNAME ), pSetVar->varname );
		SetControlText( GetControl( frame, BTN_ENV_VARVAL ), pSetVar->newval );
		SetCheckState( GetControl( frame, CHECKBOX_USER_VARIABLE ), pSetVar->bUser );
		SetCommonButtons( frame, &done, &okay );
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR buffer[256];
			GetControlText( GetControl( frame, BTN_ENV_VARNAME ), buffer, sizeof( buffer )  );
			if( ( pSetVar->varname && strcmp( pSetVar->varname, buffer ) ) || ( !pSetVar->varname && buffer[0] ) )
			{
				Release( pSetVar->varname );
				pSetVar->varname = StrDup( buffer );
			}
			GetControlText( GetControl( frame, BTN_ENV_VARVAL ), buffer, sizeof( buffer ) );
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

static void OnSaveControl( "InterShell/Set Environment" )( FILE *file, uintptr_t psv )
{
	PSETENV pSetVar = (PSETENV)psv;
	sack_fprintf( file, "Set Environment User=%s\n", pSetVar->bUser?"yes":"no" );
	sack_fprintf( file, "Set Environment text name=%s\n", EscapeMenuString( pSetVar->varname ) );
	sack_fprintf( file, "Set Environment text value=%s\n", EscapeMenuString( pSetVar->newval ) );
}

static void OnCloneControl( "InterShell/Set Environment" )( uintptr_t psvNew, uintptr_t psvOld )
{
	PSETENV pSetVarNew = (PSETENV)psvNew;
	PSETENV pSetVarOld = (PSETENV)psvOld;
	pSetVarNew->varname = StrDup( pSetVarOld->varname );
	pSetVarNew->newval = StrDup( pSetVarOld->newval );
	pSetVarNew->bUser = pSetVarOld->bUser;
}

static uintptr_t CPROC SetEnvVariableVariableName( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PSETENV pSetVar = (PSETENV)psv;
	pSetVar->varname = StrDup( name );
	return psv;
}

static uintptr_t CPROC SetEnvVariableVariableValue( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, name );
	PSETENV pSetVar = (PSETENV)psv;
	pSetVar->newval = StrDup( name );
	return psv;
}

static uintptr_t CPROC SetVariableUser( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bUser );
	PSETENV pSetVar = (PSETENV)psv;
	pSetVar->bUser = bUser;
	return psv;
}

static void OnLoadControl( "InterShell/Set Environment" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch,  "Set Environment user=%b", SetVariableUser );
	AddConfigurationMethod( pch,  "Set Environment text name=%m", SetEnvVariableVariableName );
	AddConfigurationMethod( pch,  "Set Environment text value=%m", SetEnvVariableVariableValue );
}
