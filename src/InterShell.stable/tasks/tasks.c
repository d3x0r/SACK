#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define DEFINE_DEFAULT_RENDER_INTERFACE
// define the render interface first.

#include <stdhdrs.h>
#include <idle.h>
#ifdef WIN32
// need PVIDEO internals, so we can generate kind-close keystrokes
#include <vidlib/vidstruc.h>
#endif
#include <sqlgetoption.h>
#include <psi.h>
#include <psi/shadewell.h>
#include "widgets/include/banner.h"
#include <text_label.h> // InterShell substitution proc
#include "../intershell_export.h"
#include "../intershell_registry.h"
#include "tasks.h"

static struct {
	PLOAD_TASK tasklist;
	PLIST autoload;
	PLOAD_TASK shell;
	PLOAD_TASK power_shell;
	PLOAD_TASK power_shell_ise;
	PLOAD_TASK windows_shell;
	//PSI_CONTROL frame; // this should be the same as the global frame (to hide when launching task)
	struct {
		uint32_t bExit : 4; // this needs to be set someday... it comes from intershell_main
		uint32_t wait_for_network_drive : 1;
		BIT_FIELD bSentLaunchComplete : 1;
	} flags;
	PLIST tasks_that_hid_main_canvas;
	CTEXTSTR more_path, less_path; // append and prepend path values... (append at end, insert at start of PATH environment)
#ifdef WIN32
	DEVMODE original_devmode; // the original mode that this as called with
#endif
} l;

enum {
	EDIT_TASK_LAUNCH_X = 2000
	  , EDIT_TASK_LAUNCH_Y
	  , LISTBOX_AUTO_TASKS
	  , BUTTON_EDIT_TASK_PROPERTIES
	  , BUTTON_CREATE_AUTO_TASK
	  , CHECKBOX_RESTART
	  , CHECKBOX_ONE_TIME_LAUNCH
	  , CHECKBOX_EXCLUSIVE
	  , CHECKBOX_LAUNCH_NETWORK_READY
	  , CHECKBOX_NETWORK_WAIT
	  , CHECKBOX_CAPTURE_OUTPUT // dos prompt, get the input and do something with it...
	  , LISTBOX_ALLOW_RUN_ON
	  , LISTBOX_DISALLOW_RUN_ON
	  , EDIT_SYSTEM_NAME
	  , BTN_ADD_SYSTEM
	  , BTN_REMOVE_SYSTEM
	  , TXT_TASK_NAME
	  , TXT_TASK_PATH
	  , TXT_TASK_ARGS
	  , EDIT_TASK_FRIENDLY_NAME
	  , BUTTON_DESTROY_AUTO_TASK
	  , CHECKBOX_BACKGROUND
	  , CHECKBOX_HIDE_CANVAS
	  , BTN_ADD_DISALLOW_SYSTEM
	  , BTN_REMOVE_DISALLOW_SYSTEM
	  , CHECKBOX_LAUNCH_AT_LEAST
	  , CHECKBOX_WAIT_FOR_TASK
	  , LABEL_TEXT_COLOR
	  , LABEL_BACKGROUND_COLOR
	  , LABEL_RING_COLOR
	  , LABEL_RING_HIGHLIGHT_COLOR
	  , CHECKBOX_ONE_TIME_CLICK_STOP
	  , TXT_SHUTDOWN_TASK_NAME
	  , TXT_SHUTDOWN_TASK_PATH
	  , TXT_SHUTDOWN_TASK_ARGS
};

LOGICAL MainCanvasStillHidden( void )
{
	LOGICAL yes = FALSE;
	INDEX idx;
	PLOAD_TASK task;
	LIST_FORALL( l.tasks_that_hid_main_canvas, idx, PLOAD_TASK, task )
	{
		yes = TRUE;
	}
	lprintf( WIDE("Still hidden is %s"), yes?WIDE("yes"):WIDE("no") );
	return yes;
}


#ifdef WIN32
#ifndef UNDER_CE
DEVMODE devmode; // the original mode that this as called with
/* Utility routine for SetResolution */
static void SetWithFindMode( LPDEVMODE mode, int bRestoreOnCrash )
{
	DEVMODE current;
	DEVMODE check;
	DEVMODE best;
	INDEX idx;
	// going to code something that compilers hate
	// will have usage of an undefined thing.
	// it is defined by the time it is read. Assured.
	int best_set = 0;
	for( idx = 0;
		 EnumDisplaySettings( NULL /*EnumDisplaySettings */
								  , (DWORD)idx
									//ENUM_REGISTRY_SETTINGS
								  , &check );
		  idx++
		)
	{
		if( !idx )
			current = check;
		if( idx )
		{
			// current and check should both be valid
			if( ( check.dmPelsWidth == mode->dmPelsWidth )
				&& (check.dmPelsHeight == mode->dmPelsHeight ) )
			{
				if( best_set )
				{
					if( best.dmBitsPerPel < check.dmBitsPerPel )
					{
						if( check.dmDisplayFrequency == mode->dmDisplayFrequency ||
							check.dmDisplayFrequency == 0 ||
							check.dmDisplayFrequency == 1
						  )
						{
							lprintf( WIDE(" ---- Updating best to current ---- ") );
							lprintf( WIDE("Found mode: %d %dx%d %d @%d")
									 , idx
									 , check.dmPelsWidth
									 , check.dmPelsHeight
									 , check.dmBitsPerPel
									 , check.dmDisplayFrequency
									 );
							best = check;
						}
					}
				}
				else
				{
					lprintf( WIDE(" ---- Updating best to ccheck ---- ") );
					lprintf( WIDE("Found mode: %d %dx%d %d @%d")
							 , idx
							 , check.dmPelsWidth
							 , check.dmPelsHeight
							 , check.dmBitsPerPel
							 , check.dmDisplayFrequency
							 );
					best = check;
					best_set = 1;
				}
			}
		}
	}
	{
		int n;
		for( n = 0; n < 3; n++ )
		{
			uint32_t flags;
			uint32_t result;
			switch( n )
			{
			case 0:
				flags = bRestoreOnCrash?CDS_FULLSCREEN:0;
				break;
			case 1:
				flags = 0;
				break;
			case 2:
				flags = CDS_UPDATEREGISTRY | CDS_GLOBAL;
				break;


			}
			if ( !best_set || (result=ChangeDisplaySettings(&best
																		  , flags // on program exit/original mode is restored.
																		  )) != DISP_CHANGE_SUCCESSFUL ) 
			{
				if( best_set && ( result == DISP_CHANGE_RESTART ) )
				{
					//system( WIDE("rebootnow.exe") );
					//SimpleMessageBox( NULL, WIDE("Restart?"), WIDE("Must RESTART for Resolution change to apply :(") );
					lprintf( WIDE("Result indicates Forced Restart to change modes.") );
					break;
				}
				mode->dmBitsPerPel = 32;
				if( best_set )
					lprintf( WIDE("Last failure %d %d"), result, GetLastError() );
				else
					;//lprintf( WIDE( "no best set" ) );
				if ( (result=ChangeDisplaySettings(mode
															 , flags // on program exit/original mode is restored.
															 )) != DISP_CHANGE_SUCCESSFUL ) {
					if( result == DISP_CHANGE_RESTART )
					{
						//SimpleMessageBox( NULL, WIDE("Restart?"), WIDE("Must RESTART for Resolution change to apply :(") );
						lprintf( WIDE("Result indicates Forced Restart to change modes.") );
						break;
					}
					mode->dmBitsPerPel = 24;
					lprintf( WIDE("Last failure %d %d"), result, GetLastError() );
					if ( (result=ChangeDisplaySettings(mode
																 , flags // on program exit/original mode is restored.
																 )) != DISP_CHANGE_SUCCESSFUL ) {
						if( result == DISP_CHANGE_RESTART )
						{
							//SimpleMessageBox( NULL, WIDE("Restart?"), WIDE("Must RESTART for Resolution change to apply :(") );
							lprintf( WIDE("Result indicates Forced Restart to change modes.") );
							break;
						}
						mode->dmBitsPerPel = 16;
						lprintf( WIDE("Last failure %d %d"), result, GetLastError() );
						if ( (result=ChangeDisplaySettings(mode
														, flags // on program exit/original mode is restored.
																	 )) != DISP_CHANGE_SUCCESSFUL ) {

							if( result == DISP_CHANGE_RESTART )
							{
								//SimpleMessageBox( NULL, WIDE("Restart?"), WIDE("Must RESTART for Resolution change to apply :(") );
								lprintf( WIDE("Result indicates Forced Restart to change modes.") );
								break;
							}
							//char msg[256];
							lprintf( WIDE("Failed to change resolution to %d by %d (16,24 or 32 bit) %d %d")
									 , mode->dmPelsWidth
									 , mode->dmPelsHeight
									 , result
									 , GetLastError() );
							//MessageBox( NULL, msg
							//			 , WIDE(WIDE("Resolution Failed")), MB_OK );
						}
						else
						{
							lprintf( WIDE("Success setting 16 bit %d %d")
									 , mode->dmPelsWidth
									 , mode->dmPelsHeight );
							break;
						}
					}
					else
					{
						lprintf( WIDE("Success setting 24 bit %d %d")
								 , mode->dmPelsWidth
								 , mode->dmPelsHeight );
						break;
					}
				}
				else
				{
					//lprintf( WIDE("Success setting 32 bit %d %d")
					//		 , mode->dmPelsWidth
					//		 , mode->dmPelsHeight );
					break;
				}
			}
			else
			{
				//lprintf( WIDE("Success setting enumerated bestfit %d %d")
				//		 , mode->dmPelsWidth
				//		 , mode->dmPelsHeight );
				break;
			}
		}
	}
}
#endif
#endif

void SetResolution( PLOAD_TASK task, uint32_t w, uint32_t h, LOGICAL bAtLeast )
{
#ifndef UNDER_CE
#ifdef WIN32
	DEVMODE settings;

	devmode.dmSize = sizeof( devmode );
	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &devmode );

	// if the screen is already at least as big as required....
	if( task )
		task->flags.bNoChangeResolution = 0;

	settings = devmode;
	if( bAtLeast )
	{
		if( ( settings.dmPelsWidth >= w ) && ( settings.dmPelsHeight >= h ) )
		{
			task->flags.bNoChangeResolution = 1;
			return;
		}
	}

	settings.dmPelsWidth = w;
	settings.dmPelsHeight = h;
	settings.dmBitsPerPel = 32; //video->format->BitsPerPixel;
	settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

	SetWithFindMode( &settings, TRUE /* we do want to restore on rpogram crahs */);

#endif
#endif
}

void ResetResolution( PLOAD_TASK task )
{
	lprintf( WIDE("RESET RESOLUTION") );
	if( task )
	{
		INDEX idx;
		while( ( idx = FindLink( &l.tasks_that_hid_main_canvas, task ) ) != INVALID_INDEX )
			SetLink( &l.tasks_that_hid_main_canvas, idx, NULL );
	}
	if( MainCanvasStillHidden() )
		return;
	// this task did not actually change the resolution
	if( task && task->flags.bNoChangeResolution )
	{
		lprintf( WIDE("didn't change the resolution; therefore don't reset resolution") );
	}
	else
	{
#ifndef UNDER_CE
#ifdef WIN32
		if( !task || task->flags.bLaunchAt || task->flags.bLaunchAtLeast )
		{
			if ( ChangeDisplaySettings( &l.original_devmode
											  , 0 //CDS_FULLSCREEN // on program exit/original mode is restored.
											  ) == DISP_CHANGE_SUCCESSFUL )
			{
				lprintf( WIDE("Success Reset Resolution") );
			}
			else
				lprintf( WIDE("Fail reset resolution") );
			//Sleep( 250 ); // give resolution a little time to settle...
		}
#endif
#endif
	}
	if( task )
	{
		if( task->flags.bExclusive || task->flags.bHideCanvas )
		{
			if( !task->flags.bCaptureOutput || task->flags.bHideCanvas )
			{
				InterShell_DisablePageUpdate( InterShell_GetButtonCanvas( task->button ), FALSE );
				lprintf( WIDE("Calling InterShell_Reveal...") );
				InterShell_Reveal( InterShell_GetButtonCanvas( task->button ) );
			}
		}
	}
}


PRELOAD( RegisterTaskControls )
{

	EasyRegisterResource( WIDE("InterShell/tasks"), LABEL_TEXT_COLOR			  , STATIC_TEXT_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), LABEL_BACKGROUND_COLOR	  , STATIC_TEXT_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), LABEL_RING_COLOR			  , STATIC_TEXT_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), LABEL_RING_HIGHLIGHT_COLOR , STATIC_TEXT_NAME );

	EasyRegisterResource( WIDE("InterShell/tasks"), TXT_TASK_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), TXT_TASK_PATH, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), TXT_TASK_ARGS, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), TXT_SHUTDOWN_TASK_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), TXT_SHUTDOWN_TASK_PATH, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), TXT_SHUTDOWN_TASK_ARGS, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), EDIT_TASK_LAUNCH_X, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), EDIT_TASK_LAUNCH_Y, EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), LISTBOX_AUTO_TASKS			 , LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BUTTON_EDIT_TASK_PROPERTIES , NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BUTTON_CREATE_AUTO_TASK	  , NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BUTTON_DESTROY_AUTO_TASK	  , NORMAL_BUTTON_NAME );

	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_RESTART				, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_EXCLUSIVE			 , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_WAIT_FOR_TASK		 , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_BACKGROUND			, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_LAUNCH_NETWORK_READY, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_NETWORK_WAIT		  , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_ONE_TIME_LAUNCH	 , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_ONE_TIME_CLICK_STOP, RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_CAPTURE_OUTPUT	  , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_HIDE_CANVAS		  , RADIO_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), CHECKBOX_LAUNCH_AT_LEAST	 , RADIO_BUTTON_NAME );

	EasyRegisterResource( WIDE("InterShell/tasks"), LISTBOX_ALLOW_RUN_ON	 , LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), LISTBOX_DISALLOW_RUN_ON	 , LISTBOX_CONTROL_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), EDIT_SYSTEM_NAME	 , EDIT_FIELD_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BTN_ADD_SYSTEM	  , NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BTN_REMOVE_SYSTEM	  , NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BTN_ADD_DISALLOW_SYSTEM	  , NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), BTN_REMOVE_DISALLOW_SYSTEM	  , NORMAL_BUTTON_NAME );
	EasyRegisterResource( WIDE("InterShell/tasks"), EDIT_TASK_FRIENDLY_NAME, EDIT_FIELD_NAME );

	{
		l.shell = CreateTask( NULL );
		l.shell->flags.bExclusive = 0;
		StrCpyEx( l.shell->pName, WIDE("Command Shell"), sizeof( l.shell->pName )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.shell->pTask, WIDE("cmd.exe"), sizeof( l.shell->pTask )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.shell->pPath, WIDE("."), sizeof( l.shell->pPath )/sizeof( TEXTCHAR ) );
	}
	{
		l.power_shell = CreateTask( NULL );
		l.power_shell->flags.bExclusive = 0;
		StrCpyEx( l.power_shell->pName, WIDE("Power Shell"), sizeof( l.shell->pName )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.power_shell->pTask, WIDE("%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\PowerShell.exe"), sizeof( l.shell->pTask )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.power_shell->pPath, WIDE("."), sizeof( l.shell->pPath )/sizeof( TEXTCHAR ) );
	}
	{
		l.power_shell_ise = CreateTask( NULL );
		l.power_shell_ise->flags.bExclusive = 0;
		StrCpyEx( l.power_shell_ise->pName, WIDE("Power Shell ISE"), sizeof( l.shell->pName )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.power_shell_ise->pTask, WIDE("%SystemRoot%\\System32\\WindowsPowerShell\\v1.0\\PowerShell_ise.exe"), sizeof( l.shell->pTask )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.power_shell_ise->pPath, WIDE("."), sizeof( l.shell->pPath )/sizeof( TEXTCHAR ) );
	}
	{
		l.windows_shell = CreateTask( NULL );
		l.windows_shell->flags.bExclusive = 0;
		StrCpyEx( l.windows_shell->pName, WIDE("Explorer"), sizeof( l.shell->pName )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.windows_shell->pTask, WIDE("explorer.exe"), sizeof( l.shell->pTask )/sizeof( TEXTCHAR ) );
		StrCpyEx( l.windows_shell->pPath, WIDE("."), sizeof( l.shell->pPath )/sizeof( TEXTCHAR ) );
	}


#ifdef WIN32
	l.original_devmode.dmSize = sizeof( l.original_devmode );
	EnumDisplaySettings( NULL, ENUM_CURRENT_SETTINGS, &l.original_devmode );
#endif

}

//---------------------------------------------------------------------------

PLOAD_TASK CPROC CreateTask( PMENU_BUTTON button )
{
	PLOAD_TASK task = New( LOAD_TASK );
	MemSet( task, 0, sizeof( LOAD_TASK ) );
	StrCpyEx( task->pPath, WIDE("."), sizeof( task->pPath )/sizeof(TEXTCHAR) );
	task->spawns = CreateList();
	{
		PLIST tmp = NULL;
		INDEX idx;
		CTEXTSTR module;
		GetSecurityModules( &tmp );
		LIST_FORALL( tmp, idx, CTEXTSTR, module )
		{
			struct task_security_module *task_security = New( struct task_security_module );
			task_security->name = module;
			task_security->tokens = NULL;
			AddLink( &task->security_modules, task_security );
		}
		DeleteList( &tmp );
	}
	LinkThing( l.tasklist, task );
	return task;
}

static uintptr_t OnCreateMenuButton( WIDE("Task") )( PMENU_BUTTON button )
{
	PLOAD_TASK task = CreateTask( button );
	task->flags.bButton = 1;
	task->button = button;
	InterShell_SetButtonStyle( button, WIDE("bicolor square") );
	return (uintptr_t)task;
}
//---------------------------------------------------------------------------

static void OnDestroyMenuButton( WIDE("Task") )( uintptr_t psv )
{
	// destory button... destroy associated task information...
}

//---------------------------------------------------------------------------

void SetTaskArguments( PLOAD_TASK pTask, LOGICAL bShutdown, TEXTCHAR *args )
{
	int argc;
	TEXTCHAR **pp;

	if( bShutdown )
	{
		pp = pTask->pShutdownArgs;
		while( pp && pp[0] )
		{
			Release( pp[0] );
			pp++;
		}
		Release( pTask->pShutdownArgs );
		ParseIntoArgs( args, &argc, &pTask->pShutdownArgs );
		// insert a TEXTSTR pointer so we can include the task name in the args... prebuilt for launching.
		pTask->pShutdownArgs = (TEXTSTR*)Preallocate( pTask->pShutdownArgs, SizeOfMemBlock( pTask->pShutdownArgs ) + sizeof( TEXTSTR ) );
		pTask->pShutdownArgs[0] = StrDup( pTask->pShutdownTask );
	}
	else
	{
		pp = pTask->pArgs;
		while( pp && pp[0] )
		{
			Release( pp[0] );
			pp++;
		}
		Release( pTask->pArgs );

		ParseIntoArgs( args, &argc, &pTask->pArgs );
		// insert a TEXTSTR pointer so we can include the task name in the args... prebuilt for launching.
		pTask->pArgs = (TEXTSTR*)Preallocate( pTask->pArgs, SizeOfMemBlock( pTask->pArgs ) + sizeof( TEXTSTR ) );
		pTask->pArgs[0] = StrDup( pTask->pTask );
	}
	return;
}

//---------------------------------------------------------------------------

TEXTCHAR *GetTaskArgs( PLOAD_TASK pTask, LOGICAL bShutdown )
{
	static TEXTCHAR args[4096];
	int len = 0, n;
	args[0] = 0;
	// arg[0] should be the same as program name...
	for( n = 1; bShutdown?(pTask->pShutdownArgs && pTask->pShutdownArgs[n]):(pTask->pArgs && pTask->pArgs[n]); n++ )
	{
		if( (bShutdown?pTask->pShutdownArgs[n][0]:pTask->pArgs[n][0]) == 0 )
			len += snprintf( args + len, sizeof( args ) - len * sizeof( TEXTCHAR ), WIDE("%s\"\""), n>1?WIDE(" "):WIDE("") );
		else if( StrChr( bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n], ' ' ) )
		{
			if( StrChr( bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n], '\"' ) )
				len += snprintf( args + len, sizeof( args ) - len * sizeof( TEXTCHAR ), WIDE("%s\'%s\'"), n>1?WIDE(" "):WIDE("")
									, (bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n]) );
			else
				len += snprintf( args + len, sizeof( args ) - len * sizeof( TEXTCHAR ), WIDE("%s\'%s\'"), n>1?WIDE(" "):WIDE("")
									, (bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n]) );
		}
		else if( StrChr(bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n], '\"' ) )
		{
			len += snprintf( args + len, sizeof( args ) - len * sizeof( TEXTCHAR ), WIDE("%s\'%s\'"), n>1?WIDE(" "):WIDE("")
								, (bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n]) );
		}
		else
			len += snprintf( args + len, sizeof( args ) - len * sizeof( TEXTCHAR ), WIDE("%s%s"), n>1?WIDE(" "):WIDE("")
								, (bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n]) );
	}
	return args;
}

//---------------------------------------------------------------------------

static void CPROC AddSystemAllow( uintptr_t psv, PSI_CONTROL pc_button )
{
	TEXTCHAR buffer[256];
	GetControlText( GetNearControl( pc_button, EDIT_SYSTEM_NAME ), buffer, sizeof( buffer ) );
	AddListItem( GetNearControl( pc_button, LISTBOX_ALLOW_RUN_ON ), buffer );
}

static void CPROC AddSystemDisallow( uintptr_t psv, PSI_CONTROL pc_button )
{
	TEXTCHAR buffer[256];
	GetControlText( GetNearControl( pc_button, EDIT_SYSTEM_NAME ), buffer, sizeof( buffer ) );
	AddListItem( GetNearControl( pc_button, LISTBOX_DISALLOW_RUN_ON ), buffer );

}

//---------------------------------------------------------------------------

static void CPROC RemoveSystemAllow( uintptr_t psv, PSI_CONTROL pc_button )
{
	PLISTITEM pli;
	PSI_CONTROL list;
	list = GetNearControl( pc_button, LISTBOX_ALLOW_RUN_ON );
	pli = GetSelectedItem( list );
	if( pli )
	{
		DeleteListItem( list, pli );
	}
}

//---------------------------------------------------------------------------

static void CPROC RemoveSystemDisallow( uintptr_t psv, PSI_CONTROL pc_button )
{
	PLISTITEM pli;
	PSI_CONTROL list;
	list = GetNearControl( pc_button, LISTBOX_DISALLOW_RUN_ON );
	pli = GetSelectedItem( list );
	if( pli )
	{
		DeleteListItem( list, pli );
	}
}

//---------------------------------------------------------------------------




void EditTaskProperties( uintptr_t psv, PSI_CONTROL parent_frame, LOGICAL bVisual )
{
	PLOAD_TASK pTask = (PLOAD_TASK)psv;
	PSI_CONTROL frame = LoadXMLFrameOver( parent_frame, bVisual?WIDE("menu.task.isframe"):WIDE("task.isframe") );
	int created = 0;
	int okay = 0;
	int done = 0;
	TEXTCHAR menuname[256];
	{
		pTask->button = InterShell_GetCurrentButton();
		SetCommonButtons( frame, &done, &okay );
		if( bVisual )
			SetCommonButtonControls( frame );

		// re-set security module to reference a different place.
		if( pTask->flags.bButton ) // otherwise security will be checked on the non-button task
			SetupSecurityEdit( frame, (uintptr_t)&pTask->security_modules );

		{
			TEXTCHAR buf[256];
			if( pTask->button )
				InterShell_GetButtonText( pTask->button, buf, 256 );
			else
				StrCpyEx( buf, pTask->pName, 256 );
			SetControlText( GetControl( frame, EDIT_TASK_FRIENDLY_NAME ), buf );
		}
		SetControlText( GetControl( frame, TXT_TASK_NAME ), pTask->pTask );
		SetControlText( GetControl( frame, TXT_TASK_PATH ), pTask->pPath );
		SetControlText( GetControl( frame, TXT_TASK_ARGS ), GetTaskArgs( pTask, FALSE ) );
		SetControlText( GetControl( frame, TXT_SHUTDOWN_TASK_NAME ), pTask->pShutdownTask );
		SetControlText( GetControl( frame, TXT_SHUTDOWN_TASK_PATH ), pTask->pShutdownPath );
		SetControlText( GetControl( frame, TXT_SHUTDOWN_TASK_ARGS ), GetTaskArgs( pTask, TRUE ) );
		snprintf( menuname, sizeof(menuname), WIDE("%ld"), pTask->launch_width );
		SetControlText( GetControl( frame, EDIT_TASK_LAUNCH_X ), menuname );
		snprintf( menuname, sizeof(menuname), WIDE("%ld"), pTask->launch_height );
		SetControlText( GetControl( frame, EDIT_TASK_LAUNCH_Y ), menuname );
		SetCheckState( GetControl( frame, CHECKBOX_RESTART ), pTask->flags.bRestart );
		SetCheckState( GetControl( frame, CHECKBOX_EXCLUSIVE ), pTask->flags.bExclusive );
		SetCheckState( GetControl( frame, CHECKBOX_WAIT_FOR_TASK ), pTask->flags.bWaitForTask );
		SetCheckState( GetControl( frame, CHECKBOX_BACKGROUND ), pTask->flags.bBackground );
		SetCheckState( GetControl( frame, CHECKBOX_LAUNCH_NETWORK_READY ), pTask->flags.bLaunchWhenNetworkDriveUp );
		SetCheckState( GetControl( frame, CHECKBOX_ONE_TIME_LAUNCH ), pTask->flags.bOneLaunch );
		SetCheckState( GetControl( frame, CHECKBOX_ONE_TIME_CLICK_STOP ), pTask->flags.bOneLaunchClickStop );
		SetCheckState( GetControl( frame, CHECKBOX_CAPTURE_OUTPUT ), pTask->flags.bCaptureOutput );
		SetCheckState( GetControl( frame, CHECKBOX_HIDE_CANVAS ), pTask->flags.bHideCanvas );
		SetCheckState( GetControl( frame, CHECKBOX_LAUNCH_AT_LEAST ), pTask->flags.bLaunchAtLeast );
		
		{
			PSI_CONTROL list;
			list = GetControl( frame, LISTBOX_ALLOW_RUN_ON );
			if( list )
			{
				INDEX idx;
				CTEXTSTR system;
				LIST_FORALL( pTask->allowed_run_on, idx, CTEXTSTR, system )
				{
					AddListItem( list, system );
				}
			}
			list = GetControl( frame, LISTBOX_DISALLOW_RUN_ON );
			if( list )
			{
				INDEX idx;
				CTEXTSTR system;
				LIST_FORALL( pTask->disallowed_run_on, idx, CTEXTSTR, system )
				{
					AddListItem( list, system );
				}
			}
			SetButtonPushMethod( GetControl( frame, BTN_ADD_SYSTEM ), AddSystemAllow, (uintptr_t)pTask );
			SetButtonPushMethod( GetControl( frame, BTN_REMOVE_SYSTEM ), RemoveSystemAllow, (uintptr_t)pTask );
			SetButtonPushMethod( GetControl( frame, BTN_ADD_DISALLOW_SYSTEM ), AddSystemDisallow, (uintptr_t)pTask );
			SetButtonPushMethod( GetControl( frame, BTN_REMOVE_DISALLOW_SYSTEM ), RemoveSystemDisallow, (uintptr_t)pTask );
		}
	}
	DisplayFrameOver( frame, parent_frame );
	EditFrame( frame, TRUE );
	CommonWait( frame );
	lprintf( WIDE("Wait complete... %d %d"), okay, done );
	if( okay )
	{
		TEXTCHAR args[256];
		// Get info from dialog...
		GetControlText( GetControl( frame, TXT_TASK_NAME ), pTask->pTask, sizeof( pTask->pTask ) );
		GetControlText( GetControl( frame, TXT_TASK_PATH ), pTask->pPath, sizeof( pTask->pPath ) );
		GetControlText( GetControl( frame, TXT_SHUTDOWN_TASK_NAME ), pTask->pShutdownTask, sizeof( pTask->pShutdownTask ) );
		GetControlText( GetControl( frame, TXT_SHUTDOWN_TASK_PATH ), pTask->pShutdownPath, sizeof( pTask->pShutdownPath ) );
		GetControlText( GetControl( frame, EDIT_TASK_FRIENDLY_NAME ), menuname, sizeof( menuname ) );
		InterShell_SetButtonText( pTask->button, menuname );
		GetControlText( GetControl( frame, EDIT_TASK_LAUNCH_X ), args, sizeof( args ) );
		pTask->launch_width = atoi( args );
		GetControlText( GetControl( frame, EDIT_TASK_LAUNCH_Y ), args, sizeof( args ) );
		pTask->launch_height = atoi( args );
		if( pTask->launch_width && pTask->launch_height )
			pTask->flags.bLaunchAt = 1;
		{
			PSI_CONTROL checkbox;
			checkbox = GetControl( frame, CHECKBOX_RESTART );
			if( checkbox ) pTask->flags.bRestart = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_EXCLUSIVE );
			if( checkbox ) pTask->flags.bExclusive = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_WAIT_FOR_TASK );
			if( checkbox ) pTask->flags.bWaitForTask = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_BACKGROUND );
			if( checkbox ) pTask->flags.bBackground = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_LAUNCH_NETWORK_READY );
			if( checkbox ) pTask->flags.bLaunchWhenNetworkDriveUp = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_ONE_TIME_LAUNCH );
			if( checkbox ) pTask->flags.bOneLaunch = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_ONE_TIME_CLICK_STOP );
			if( checkbox ) pTask->flags.bOneLaunchClickStop = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_CAPTURE_OUTPUT );
			if( checkbox ) pTask->flags.bCaptureOutput = GetCheckState( checkbox );
			checkbox = GetControl( frame, CHECKBOX_HIDE_CANVAS );
			if( checkbox ) pTask->flags.bHideCanvas = GetCheckState( checkbox );
			if( pTask->flags.bLaunchAt )
			{
				checkbox = GetControl( frame, CHECKBOX_LAUNCH_AT_LEAST );
				if( checkbox ) pTask->flags.bLaunchAtLeast = GetCheckState( checkbox );
			}
			else
				pTask->flags.bLaunchAtLeast = 0;
			{
				PSI_CONTROL list;
				list = GetControl( frame, LISTBOX_ALLOW_RUN_ON );
				if( list )
				{
					INDEX idx;
					PLISTITEM pli;
					TEXTSTR system;
					LIST_FORALL( pTask->allowed_run_on, idx, TEXTSTR, system )
					{
						Release( system );
						SetLink( &pTask->allowed_run_on, idx, NULL );
					}
					for( idx = 0; pli = GetNthItem( list, (int)idx ); idx++ )
					{
						TEXTCHAR buffer[256];
						INDEX idx2;
						GetListItemText( pli, buffer, sizeof( buffer ) );
						LIST_FORALL( pTask->allowed_run_on, idx2, TEXTSTR, system )
						{
							//lprintf( "Compare [%s] vs [%s] ...", system, buffer );
							if( CompareMask( system, buffer, FALSE ) )
							{
								//lprintf( "success..." );
								break;
							}
							//else
							//	lprintf( "failure..." );
						}
						if( !system )
						{
							AddLink( &pTask->allowed_run_on, StrDup( buffer ) );
						}
					}
				}
				list = GetControl( frame, LISTBOX_DISALLOW_RUN_ON );
				if( list )
				{
					INDEX idx;
					PLISTITEM pli;
					TEXTSTR system;
					LIST_FORALL( pTask->disallowed_run_on, idx, TEXTSTR, system )
					{
						Release( system );
						SetLink( &pTask->disallowed_run_on, idx, NULL );
					}
					for( idx = 0; pli = GetNthItem( list, (int)idx ); idx++ )
					{
						TEXTCHAR buffer[256];
						INDEX idx2;
						GetListItemText( pli, buffer, sizeof( buffer ) );
						LIST_FORALL( pTask->disallowed_run_on, idx2, TEXTSTR, system )
						{
							//lprintf( "Compare [%s] vs [%s] ...", system, buffer );
							if( CompareMask( system, buffer, FALSE ) )
							{
								//lprintf( "success..." );
								break;
							}
							//else
							//	lprintf( "failure..." );
						}
						if( !system )
						{
							AddLink( &pTask->disallowed_run_on, StrDup( buffer ) );
						}
					}
				}
			}
		}
		GetControlText( GetControl( frame, TXT_TASK_ARGS )
						  , args, sizeof( args ) );
		SetTaskArguments( pTask, FALSE, args );
		GetControlText( GetControl( frame, TXT_SHUTDOWN_TASK_ARGS )
						  , args, sizeof( args ) );
		SetTaskArguments( pTask, TRUE, args );
		if( bVisual )
			GetCommonButtonControls( frame );
	}
	else
	{
		if( created )
			DestroyTask( &pTask );
	}
	DestroyFrame( &frame );
}

static uintptr_t OnEditControl( WIDE("Task") )( uintptr_t psv, PSI_CONTROL parent_frame )
{
	EditTaskProperties( psv, parent_frame, TRUE );
	return psv;
}


//---------------------------------------------------------------------------

void DestroyTask( PLOAD_TASK *ppTask )
{
	if( ppTask && *ppTask )
	{
		PLOAD_TASK pTask = *ppTask;
		{
			int tried = 0;
			POINTER spawned;
			INDEX idx;
		retry:
			LIST_FORALL( pTask->spawns, idx, POINTER, spawned )
			{
				if( spawned )
					break;
			}
			if( spawned )
			{
				pTask->flags.bDestroy = TRUE;
				if( !tried )
				{
					INDEX idx;
					PTASK_INFO task;
					LIST_FORALL( pTask->spawns, idx, PTASK_INFO, task )
					{
						// should get the termination callback which will take this
						// instance out of this list... which then
						// will enable us to continue;
						TerminateProgram( task );
					}
					tried = 1;
					goto retry;
				}

				// do still remove the task...
				DeleteLink( &l.autoload, pTask );
				// and unlink it.
				UnlinkThing( pTask );
				return;
			}
		}
		DeleteList( &pTask->spawns );

		if( pTask->pArgs )
		{
			TEXTCHAR **pp;
			pp = pTask->pArgs;
			while( pp && pp[0] )
			{
				Release( pp[0] );
				pp++;
			}
			Release( pTask->pArgs );
			pTask->pArgs = NULL;
		}
		if( pTask->pShutdownArgs )
		{
			TEXTCHAR **pp;
			pp = pTask->pShutdownArgs;
			while( pp && pp[0] )
			{
				Release( pp[0] );
				pp++;
			}
			Release( pTask->pShutdownArgs );
			pTask->pShutdownArgs = NULL;
		}
		DeleteLink( &l.autoload, pTask );
		UnlinkThing( pTask );
		Release( pTask );
		*ppTask = NULL;
	}
}

//---------------------------------------------------------------------------

void CPROC HandleTaskOutput( uintptr_t psvTaskInfo, PTASK_INFO task, CTEXTSTR buffer, size_t size )
{
	PLOAD_TASK pTask = (PLOAD_TASK)psvTaskInfo;
	lprintf( WIDE("%s:%s"), pTask->pTask, buffer );
}

//---------------------------------------------------------------------------
static LOGICAL IsTaskRunning( PLOAD_TASK pTask )
{
	PTASK_INFO task;
	INDEX idx;
	LIST_FORALL( pTask->spawns, idx, PTASK_INFO, task )
	{
		return TRUE;
	}
	return FALSE;
}


//---------------------------------------------------------------------------
// forward declaration, cause the task may re-spawn within task ended
void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task_ended );
//---------------------------------------------------------------------------

void RunATask( PLOAD_TASK pTask, int bWaitInRoutine, LOGICAL bShutdown )
{
	//PLOAD_TASK pTask = (PLOAD_TASK)psv;
	PTASK_INFO task;
	INDEX idx;
	// if task flag set as exclusive...
	if( IsSystemShuttingDown() )
	{
		lprintf( WIDE("Do not start a task, system is shutting down.") );
		return;
	}
	if( pTask->flags.bDisallowedRun )
	{
		if( !pTask->flags.bAllowedRun )
			return;
	}
	{
		uintptr_t task_security;
		if( !pTask->flags.bButton ) // otherwise security will be checked on the button
			task_security = CreateSecurityContext( (uintptr_t)pTask );
		else
			task_security = CreateSecurityContext( (uintptr_t)&pTask->security_modules );
		if( task_security == INVALID_INDEX )
			return;
		//if( task_security )
		//	pTask->flags.bExclusive = 1;
		pTask->psvSecurityToken = task_security;
	}
	// else if allowed, okay
	// if not allowed and not disallowed, okay

	if( (!bShutdown) && pTask->flags.bExclusive )
	{
		LIST_FORALL( pTask->spawns, idx, PTASK_INFO, task )
		{
			if( pTask->flags.bOneLaunch )
			{
				//lprintf( WIDE("Task is already spawned, leave.") );
				return;
			}
			lprintf( WIDE("Re-hide frame - tasks still running.") );
			// shouldn't be showing to launch anything...
				InterShell_Hide( InterShell_GetButtonCanvas( pTask->button ) );
			return;
		}
		//lprintf( WIDE("Launching task %s"), pTask->pTask );
		if( pTask->flags.bLaunchAtLeast )
		{
			SetResolution( pTask, pTask->launch_width, pTask->launch_height, TRUE );
		}
		else if( pTask->flags.bLaunchAt )
		{
			SetResolution( pTask, pTask->launch_width, pTask->launch_height, FALSE );
		}
	}
	xlprintf(LOG_ALWAYS)( WIDE("Launching program... %s[%s] in %s"), pTask->pTask, GetTaskArgs(pTask, bShutdown), pTask->pPath );
	if( !bShutdown )
	{
		pTask->last_lauch_time = timeGetTime();
		pTask->launch_count++;
	}
	{
		TEXTCHAR buffer1[256];
		TEXTCHAR buffer2[256];
		CTEXTSTR taskname = StrDup( InterShell_TranslateLabelText( NULL
																					, buffer1, sizeof( buffer1 )
																					, bShutdown?pTask->pShutdownTask:pTask->pTask ) );
		CTEXTSTR path = StrDup( InterShell_TranslateLabelText( NULL
																			  , buffer2, sizeof( buffer2 )
																			  , bShutdown?pTask->pShutdownPath:pTask->pPath ) );
		TEXTSTR *args;
		if( bShutdown )
		{
			if( pTask->pShutdownArgs )
			{
				int n;
				args = pTask->pShutdownArgs;
				for( n = 0; args && args[n]; n++ ); // just count.
				args = NewArray( TEXTSTR, (n+1) );
				for( n = 0; pTask->pShutdownArgs[n]; n++ )
					args[n] = StrDup( InterShell_TranslateLabelText( NULL
																				  , buffer1, sizeof( buffer1 )
																				  , bShutdown?pTask->pShutdownArgs[n]:pTask->pShutdownArgs[n] ) );
				args[n] = pTask->pShutdownArgs[n]; // copy NULL too.
			}
			else
				args = NULL;
		}
		else
		{
			if( pTask->pArgs )
			{
				int n;
				args = pTask->pArgs;
				for( n = 0; args && args[n]; n++ ); // just count.
				args = NewArray( TEXTSTR, (n+1) );
				for( n = 0; pTask->pArgs[n]; n++ )
					args[n] = StrDup( InterShell_TranslateLabelText( NULL
																				  , buffer1, sizeof( buffer1 )
																				  , bShutdown?pTask->pShutdownArgs[n]:pTask->pArgs[n] ) );
				args[n] = pTask->pArgs[n]; // copy NULL too.
			}
			else
				args = NULL;
		}
		pTask->flags.bStarting = 1;
#ifndef UNDER_CE
		if( pTask->flags.bCaptureOutput )
		{
			task = LaunchPeerProgramExx( taskname
											, path
											, (PCTEXTSTR)args
											, 0
											, HandleTaskOutput
											, TaskEnded
											, (uintptr_t)pTask 
											DBG_SRC
											);
		}
		else
#endif
			task = LaunchProgramEx( taskname, path
					, (PCTEXTSTR)args
					, TaskEnded
					, (uintptr_t)pTask 
					);
		Release( (POINTER)taskname );
		Release( (POINTER)path );
		if( args )
		{
			int n;
			for( n = 0; args[n]; n++ )
				Release( (POINTER)args[n] );
			Release( (POINTER)args );
		}
	}

	//lprintf( WIDE("Result is %p"), task );

	if( task )
	{
		AddLink( &pTask->spawns, (POINTER)task );
		pTask->flags.bStarting = 0; // okay to allow ended to check now...

		if( !bShutdown )
		{
			if( pTask->button )
				InterShell_SetButtonHighlight( pTask->button, TRUE );

			if( pTask->flags.bExclusive )
			{
				if( !pTask->flags.bCaptureOutput || pTask->flags.bHideCanvas )
				{
					AddLink( &l.tasks_that_hid_main_canvas, pTask );
					InterShell_Hide( InterShell_GetButtonCanvas( pTask->button ) );
				}
			}
		}

		if( bShutdown || pTask->flags.bExclusive || pTask->flags.bWaitForTask )
		{
			// Wait here until task ends.
			// no real reason to wait anymore, there is an event thhat happens?
			if( bWaitInRoutine )
			{
				//lprintf( WIDE("Waiting...") );
				do
				{
					LIST_FORALL( pTask->spawns, idx, PTASK_INFO, task )
					{
						// there is a task spawned...
						if( !Idle() )
						{
							pTask->waiting_thread = MakeThread();
							WakeableSleep( 2500 );
						}
						else
							WakeableSleep( 100 );
						break;
					}
				} while( task );
			}
		}
		else
		{
			if( pTask->flags.bHideCanvas )
			{
				AddLink( &l.tasks_that_hid_main_canvas, pTask );
				InterShell_Hide( InterShell_GetButtonCanvas( pTask->button ) );
			}
		}
	}
	else
	{
		pTask->flags.bStarting = 0; // not starting anymore...
		ResetResolution( pTask );
	}
}

static LOGICAL TaskHasSpawns( PLOAD_TASK task_test )
{
	INDEX idx;
	PTASK_INFO task;
	{
		int other_spawns = 0;
		LIST_FORALL( task_test->spawns, idx, PTASK_INFO, task )
		{
			other_spawns++;
		}
		if( other_spawns )
			return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

static void OnBeginShutdown( WIDE("Intershell Tasks") )( void )
{
	xlprintf(2500)( WIDE("Mark to not start tasks now...") );
	l.flags.bExit = 2;
}

//---------------------------------------------------------------------------

void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task_ended )
{
	INDEX idx;
	int marked = FALSE;
	PTASK_INFO task;
	PLOAD_TASK pTask = (PLOAD_TASK)psv;
	PLOAD_TASK tmp;
	if( !pTask )
		return;
	// don't check ended while starting...
	while( pTask->flags.bStarting )
		Relinquish();
	{
		TEXTCHAR buf[64];
		if( pTask->button )
			InterShell_GetButtonText( pTask->button, buf, 64 );
		else
			StrCpyEx( buf, pTask->pName, 64 );
		lprintf( WIDE("%s ended - refocus menu..."), buf );
	}
	for( tmp = l.tasklist; (!marked) && tmp; tmp = tmp->next )
	{
		//lprintf( WIDE("looking at task %p..."), tmp );
		if( tmp->flags.bLaunchAt || (tmp->flags.bExclusive) )
		{
			LIST_FORALL( tmp->spawns, idx, PTASK_INFO, task )
			{
				//lprintf( WIDE("looking at task instance %p task %p... for %p"), tmp, task, task_ended );
				if( task_ended == task )
					continue;

				marked = TRUE;
				break;
			}
		}
	}

	if( pTask->psvSecurityToken )
	{
		CloseSecurityContext( (uintptr_t)pTask, pTask->psvSecurityToken );
		pTask->psvSecurityToken = 0;
	}

	LIST_FORALL( pTask->spawns, idx, PTASK_INFO, task )
	{
		//lprintf( WIDE("looking at task %p task %p"), pTask, task );
		if( task_ended == task )
		{
			//DebugBreak();
			lprintf( WIDE("Restore frame") );
			SetLink( &pTask->spawns, idx, NULL );
			// reset resolution (if applicable)
			{
				// not marked - no other spawn is on any other button either.
				if( !marked )
					ResetResolution( pTask );
			}
			/* destroy the task... was still running when we destroyed its button */
			if( pTask->flags.bDestroy )
			{
				DestroyTask( &pTask );
				return;
			}

			if( pTask->waiting_thread )
			{
				WakeThread( pTask->waiting_thread );
				pTask->waiting_thread = NULL;
			}
			// get out now, no reason to do much of anything here...
			// especially if we're shutting down.
			if( l.flags.bExit == 2 )
			{
				lprintf( WIDE("Shutting down... why do we crash after this?!") );
				return;
			}
			// exclusive task runs hiding the menu... and only runs once.
			// this task exiting is fair to reveal common, else, don't blink.
			if( pTask &&
				pTask->flags.bRestart &&
				( l.flags.bExit != 2 ) )
			{
				if( ( pTask->last_lauch_time + 2000 ) > timeGetTime() )
				{
					lprintf( WIDE("Task spawning too fast, disabling auto spawn.") );
					pTask->flags.bRestart = 0;
				}
				else
					RunATask( pTask, InterShell_IsButtonVirtual( pTask->button ), FALSE );
			}
			// during exit, no point to updating...
			if( !l.flags.bExit )
			{
				if( pTask->flags.bButton )
				{
					if( ((PLOAD_TASK)psv)->button && !TaskHasSpawns(pTask) )
					{
						InterShell_SetButtonHighlight( ((PLOAD_TASK)psv)->button, FALSE );
						UpdateButton( ((PLOAD_TASK)psv)->button );
					}
				}
			}
			break;
		}
	}
	if( !task )
	{
		lprintf( WIDE("Failed to find task which ended.") );
	}
 
	lprintf( WIDE("and task is done...") );
}

//---------------------------------------------------------------------------
static void KillSpawnedProgram( PLOAD_TASK tasks )
{
	{
		int first_pass = 1;
		INDEX idx;
		PTASK_INFO task;
	retry:
		LIST_FORALL( tasks->spawns, idx, PTASK_INFO, task )
		{
			TEXTCHAR buffer[256];
			LOGICAL closed = FALSE;
			CTEXTSTR fullname = InterShell_TranslateLabelText( NULL, buffer, sizeof( buffer ), tasks->pTask );
			CTEXTSTR filename = pathrchr( fullname );
			if( filename )
				filename++;
			else
				filename = fullname;

			if( first_pass && tasks->pShutdownTask[0] )
			{
				// run and wait there always.  This must complete before continue.
				RunATask( tasks, TRUE, TRUE );
				first_pass = 0;
				goto retry;
			}

			tasks->flags.bRestart = 0;
#ifdef WIN32
			{
				TEXTCHAR progname[256];
				TEXTSTR p;
				LOGICAL bIcon = FALSE;
				HWND hWnd;
				lprintf( WIDE("Terminating %s (by icon)"), filename?filename:WIDE("<noname>") );
				snprintf( progname, sizeof( progname ), WIDE("AlertAgentIcon:%s"), filename );
				if( ( hWnd = FindWindow( WIDE("AlertAgentIcon"), progname ) )
					||( ( p = strrchr( progname, '.' ) )
					  , (p)?p[0]=0:0
					  , hWnd = FindWindow( WIDE("AlertAgentIcon"), progname ) ) )
				{
					uint32_t delay = timeGetTime() + 500;
					bIcon = TRUE;
					lprintf( WIDE("Found by alert tray icon... closing.") );
					PostMessage( hWnd, WM_COMMAND, /*MNU_EXIT*/1000, 0 );
				}
				if( bIcon )
				{
					HWND still_here;
					uint32_t TickDelay = timeGetTime() + 250;
					// give it a little time before just killing it.
					while( ( still_here = FindWindow( WIDE("AlertAgentIcon"), progname ) ) &&
							( TickDelay > timeGetTime() ) )
						Relinquish();
					if( !still_here )
						closed = TRUE;
				}


				if( !closed )
				{

					CTEXTSTR basename = fullname;
					TEXTSTR ext;
					ext = (TEXTSTR)StrCaseStr( basename, WIDE(".exe") );
					if( ext )
						ext[0] = 0;
					ext = (TEXTSTR)pathrchr( basename );
					if( ext )
						basename = ext + 1;
					lprintf( WIDE("Attempting to find [%s]"), basename );
					snprintf( progname, sizeof( progname ), WIDE("%s.instance.lock"), basename );
					{
						POINTER mem_lock;
						uintptr_t size = 0;
						mem_lock = OpenSpace( progname
												  , NULL
													//, WIDE("memory.delete")
												  , &size );
						if( mem_lock )
						{
							PVIDEO video = (PVIDEO)mem_lock;
							ForceDisplayFocus( video );
							lprintf( WIDE("Okay send alt-F4 to it. (and assume it closed)") );
							keybd_event( VK_MENU, 56, 0, 0 );
							keybd_event( VK_F4, 62, 0, 0 );
							keybd_event( VK_F4, 62, KEYEVENTF_KEYUP, 0 );
							keybd_event( VK_MENU, 56, KEYEVENTF_KEYUP, 0 );
							closed = TRUE;
						}
						else
							lprintf( WIDE("Failed to open region.") );
					}
				}
			}
#endif
			if( !closed )
			{
				lprintf( WIDE("Terminating %s (by terminate[console ^c,window WM_QUIT,TerminateProcess)"), filename?filename:WIDE("<noname>") );
				TerminateProgram( task );
			}
		}
	}
}

//---------------------------------------------------------------------------

// should get auto innited to button proc...
static void OnKeyPressEvent(  WIDE("Task") )( uintptr_t psv )
{
	PLOAD_TASK pTask = (PLOAD_TASK)psv;
	if( pTask->flags.bOneLaunch && pTask->flags.bOneLaunchClickStop && TaskHasSpawns( pTask ) )
	{
		KillSpawnedProgram( pTask );
	}
	else
	{
		RunATask( (PLOAD_TASK)psv, InterShell_IsButtonVirtual( ((PLOAD_TASK)psv)->button ), FALSE );
	}
	if( ((PLOAD_TASK)psv)->button )
	{
		UpdateButton( ((PLOAD_TASK)psv)->button );
	}
}

//---------------------------------------------------------------------------

void InvokeTaskLaunchComplete( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	if( l.flags.bSentLaunchComplete )
		return;

	{
		INDEX idx;
		PLOAD_TASK task;
		LIST_FORALL( l.autoload, idx, PLOAD_TASK, task )
		{
			if( !task->spawns )
				break;
		}
		if( task )
			return;
	}
	l.flags.bSentLaunchComplete = 1;
	for( name = GetFirstRegisteredName( TASK_PREFIX WIDE( "/common/task launch complete" ), &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(void);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
}


//---------------------------------------------------------------------------
static void KillSpawnedPrograms( void )
{
	PLOAD_TASK tasks = l.tasklist;
	// need to kill autolaunched things too...
	while( tasks )
	{
		KillSpawnedProgram( tasks );
		tasks = NextLink( tasks );
	}
	// did as good a job as we can...
	l.tasklist = NULL;
}


static void OnInterShellShutdown( WIDE("DOKillSpawnedPrograms") )(void)
{
	Banner2NoWaitAlpha( WIDE("Ending Tasks...") );
	l.flags.bExit = 2; // magic number indicating we're quitting for sure.
	KillSpawnedPrograms();
	Banner2NoWaitAlpha( WIDE("Ended Task....") );
}

//---------------------------------------------------------------------------
uintptr_t CPROC WaitForNetworkThread( PTHREAD thread );

int LaunchNetworkTasks( int bWaitForNetworkDrive )
{
	int launched = 0;
	INDEX idx;
	PLOAD_TASK task;

	if( bWaitForNetworkDrive )
	{
		// if network, but no tasks require network, don't do anything.
		LIST_FORALL( l.autoload, idx, PLOAD_TASK, task )
		{
			if( task->flags.bLaunchWhenNetworkDriveUp )
			{
				break;
			}
		}
		if( !task )
			return 0;
	}

#ifdef WIN32
	if( bWaitForNetworkDrive && !l.flags.wait_for_network_drive )
	{
		FILE *file;
		static TEXTCHAR DefaultFilePath[256];
		if( !DefaultFilePath[0] )
			SACK_GetProfileStringEx( GetProgramName(), WIDE("InterShell/Tasks/Wait File Path"), WIDE("o:/config.sys"), DefaultFilePath, sizeof( DefaultFilePath ), TRUE );
		// we're not waiitng for network in a banner-type mode...
		// therefore we need to launch these ourselves...
		if( ( file = sack_fopen( 0, DefaultFilePath, WIDE("rb") ) ) )
		{
			/* is okay. */
			fclose( file );
		}
		else
		{
			ThreadTo( WaitForNetworkThread, 0 );
			return 0; // bWaitForNetworkDrive mode tasks - and network is not up.
		}
	}
#endif
	LIST_FORALL( l.autoload, idx, PLOAD_TASK, task )
	{
		if( ( bWaitForNetworkDrive && ( task->flags.bLaunchWhenNetworkDriveUp ) )
			|| ( !bWaitForNetworkDrive && !( task->flags.bLaunchWhenNetworkDriveUp ) ) )
		{
			launched = 1;
			RunATask( task, (task->flags.bWaitForTask||task->flags.bExclusive )&&!task->flags.bBackground, FALSE );
		}
	}
	if( launched )
		InvokeTaskLaunchComplete();
	return launched;
}

#ifdef _WIN32
uintptr_t CPROC WaitForNetworkThread( PTHREAD thread )
{
	static int bWaiting;
	PBANNER banner = NULL;
	PLOAD_TASK task;
	INDEX idx;
	// if we don't have any network drive tasks, don't bother with this either.
	LIST_FORALL( l.autoload, idx, PLOAD_TASK, task )
	{
		if( task->flags.bLaunchWhenNetworkDriveUp )
		{
			if( task->flags.bDisallowedRun )
			{
				if( !task->flags.bAllowedRun )
					continue;
			}
			break;
		}
	}
	if( !task )
	{
		lprintf( WIDE("Found no tasks to check drives..."));
		return 0;
	}

	if( bWaiting )
	{
		lprintf( WIDE("already waiting for drives") );
		return 0;
	}
	bWaiting = 1;
	if( GetThreadParam( thread ) )
		CreateBanner2Ex( NULL
							, &banner
							, WIDE("Waiting for network drives...")
							, BANNER_TOP|BANNER_NOWAIT|BANNER_DEAD|BANNER_ALPHA
							, 0 );	//SetBannerOptions( banner, BANNER_TOP, 0 );
	while( 1 )
	{
		FILE *file;
		static TEXTCHAR DefaultFilePath[256];
		if( !DefaultFilePath[0] )
			SACK_GetProfileStringEx( GetProgramName(), WIDE("InterShell/Tasks/Wait File Path"), WIDE("f:/config.sys"), DefaultFilePath, sizeof( DefaultFilePath ), TRUE );
		if( ( file = sack_fopen( 0, DefaultFilePath, WIDE("rb") ) ) )
		{
			fclose( file );
			if( GetThreadParam( thread ) )
				RemoveBanner2Ex( &banner DBG_SRC );
			LaunchNetworkTasks( 1 );
			break;
		}
		WakeableSleep( 1000 );
	}
	if( GetThreadParam( thread ) )
		RemoveBanner2Ex( &banner DBG_SRC );
	bWaiting = 0;
	return 0;
}
#else
uintptr_t CPROC WaitForNetworkDriveThread( PTHREAD thread )
{
	// shrug - in a linux world, how do we know?
	return 0;
}
#endif

static uintptr_t CPROC NetworkTaskStarter( PTHREAD thread )
{
	LaunchNetworkTasks( 0 );
	return 0;
}

static void OnFinishAllInit( WIDE("tasks") )( void )
{
	PLOAD_TASK tmp;
	// for consistancy for all task buttons, set the task name in the PLOAD_TASK
	// to the button text.  Other tasks have their own names (auto task/auto network drive task)
	for( tmp = l.tasklist; tmp; tmp = tmp->next )
	{
	}
	ThreadTo( NetworkTaskStarter, 0 );
	ThreadTo( WaitForNetworkThread, 1 );
}

static LOGICAL CPROC PressDosKey( uintptr_t psv, uint32_t key )
{
	static uint32_t _tick, tick;
	static int reset = 0;
	static int reset2 = 0;
	static int reset3 = 0;
	static int reset4 = 0;
	if( _tick < ( ( tick = timeGetTime() ) - 2000 ) )
	{
		reset4 = 0;
		reset3 = 0;
		reset2 = 0;
		reset = 0;
		_tick = tick;
	}
	//lprintf( WIDE("Got a %c at %ld reset=%d"), psv, tick - _tick, reset );
	switch( reset )
	{
	case 0:
		if( psv == 'D' )
			reset++;
		break;
	case 1:
		if( psv == 'O' )
			reset++;
		break;
	case 2:
		if( psv == 'S' )
		{
			if( l.shell )
				RunATask( l.shell, 0, FALSE );
			reset = 0;
		}
		break;

	}
	switch( reset3 )
	{
	case 0:
		if( psv == 'W' )
			reset3++;
		break;
	case 1:
		if( psv == 'I' )
			reset3++;
		break;
	case 2:
		if( psv == 'N' )
		{
			if( l.windows_shell )
				RunATask( l.windows_shell, 0, FALSE );
			reset3 = 0;
		}
		break;
	}
	switch( reset2 )
	{
	case 0:
		if( psv == 'P' )
			reset2++;
		break;
	case 1:
		if( psv == 'W' )
			reset2++;
		break;
	case 2:
		if( psv == 'S' )
		{
			if( l.power_shell )
				RunATask( l.power_shell, 0, FALSE );
			reset2 = 0;
		}
		break;

	}
	switch( reset4 )
	{
	case 0:
		if( psv == 'P' )
			reset4++;
		break;
	case 1:
		if( psv == 'S' )
			reset4++;
		break;
	case 2:
		if( psv == 'I' )
		{
			if( l.power_shell_ise )
				RunATask( l.power_shell_ise, 0, FALSE );
			reset4 = 0;
		}
		break;

	}
	return 1;
}

static void OnFinishInit( WIDE("TasksShellKeys") )( PSI_CONTROL pc_canvas )
//PRELOAD( SetTaskKeys )
{
	BindEventToKey( NULL, KEY_D, KEY_MOD_ALT, PressDosKey, (uintptr_t)'D' );
	BindEventToKey( NULL, KEY_O, KEY_MOD_ALT, PressDosKey, (uintptr_t)'O' );
	BindEventToKey( NULL, KEY_S, KEY_MOD_ALT, PressDosKey, (uintptr_t)'S' );
	BindEventToKey( NULL, KEY_P, KEY_MOD_ALT, PressDosKey, (uintptr_t)'P' );
	BindEventToKey( NULL, KEY_W, KEY_MOD_ALT, PressDosKey, (uintptr_t)'W' );
	BindEventToKey( NULL, KEY_I, KEY_MOD_ALT, PressDosKey, (uintptr_t)'I' );
	BindEventToKey( NULL, KEY_N, KEY_MOD_ALT, PressDosKey, (uintptr_t)'N' );

}

//---------------------------------------------------------------------------


#define PSV_PARAM	PLOAD_TASK pTask;							  \
		pTask = (PLOAD_TASK)psv;


static uintptr_t CPROC ConfigSetTaskName( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	if( pTask )
	{
		StrCpyEx( pTask->pName, text, sizeof( pTask->pName )/sizeof( TEXTCHAR ) );
	}
	return psv;
}

//---------------------------------------------------------------------------

static uintptr_t CPROC SetTaskPath( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	//lprintf( "Setting path on task %p", psv );
	if( pTask )
		StrCpyEx( pTask->pPath, text, sizeof( pTask->pPath )/sizeof(TEXTCHAR) );
	return psv;
}

//---------------------------------------------------------------------------

static uintptr_t CPROC SetShutdownTaskPath( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	//lprintf( "Setting path on task %p", psv );
	if( pTask )
		StrCpyEx( pTask->pShutdownPath, text, sizeof( pTask->pShutdownPath )/sizeof(TEXTCHAR) );
	return psv;
}

//---------------------------------------------------------------------------

static uintptr_t CPROC SetTaskTask( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	if( pTask )
		StrCpyEx( pTask->pTask, text, sizeof( pTask->pTask ) );
	return psv;
}

//---------------------------------------------------------------------------

static uintptr_t CPROC SetShutdownTaskTask( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	if( pTask )
		StrCpyEx( pTask->pShutdownTask, text, sizeof( pTask->pShutdownTask ) );
	return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetTaskArgs( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	if( pTask )
		SetTaskArguments( pTask, FALSE, text );
	//strcpy( pTask->name, text );
	return psv;
}
//---------------------------------------------------------------------------

uintptr_t CPROC SetShutdownTaskArgs( uintptr_t psv, arg_list args )
{
	PARAM( args, TEXTCHAR *, text );
	PSV_PARAM;
	if( pTask )
		SetTaskArguments( pTask, TRUE, text );
	//strcpy( pTask->name, text );
	return psv;
}

uintptr_t CPROC SetTaskRestart( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bRestart );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bRestart = bRestart;
	return psv;
}

uintptr_t CPROC SetTaskExclusive( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bExclusive );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bExclusive = !bExclusive;
	return psv;
}
uintptr_t CPROC SetTaskWait( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bWait );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bWaitForTask = bWait;
	return psv;
}
uintptr_t CPROC SetTaskBackground( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bBackground );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bBackground = bBackground;
	return psv;
}

uintptr_t CPROC SetTaskCapture( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bCaptureOutput );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bCaptureOutput = bCaptureOutput;
	return psv;
}

uintptr_t CPROC SetTaskHide( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bHideCanvas );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bHideCanvas = bHideCanvas;
	return psv;
}

uintptr_t CPROC SetTaskOneTime( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bOneLaunch );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bOneLaunch = bOneLaunch;
	return psv;
}

uintptr_t CPROC SetTaskOneTimeClickStop( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bOneLaunchClickStop );
	PSV_PARAM;
	if( pTask )
		pTask->flags.bOneLaunchClickStop = bOneLaunchClickStop;
	return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetLaunchResolution( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, width );
	PARAM( args, int64_t, height );
	PSV_PARAM;
	if( pTask )
	{
		if( width && height )
		{
			pTask->launch_width = (int)width;
			pTask->launch_height = (int)height;
			pTask->flags.bLaunchAt = 1;
		}
	}
	return psv;
}

//---------------------------------------------------------------------------

uintptr_t CPROC SetLeastLaunchResolution( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, width );
	PARAM( args, int64_t, height );
	PSV_PARAM;
	if( pTask )
	{
		if( width && height )
		{
			pTask->launch_width = (int)width;
			pTask->launch_height = (int)height;
			pTask->flags.bLaunchAtLeast = 1;
		}
	}
	return psv;
}

//---------------------------------------------------------------------------

static uintptr_t CPROC SetTaskSecurity( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, module );
	PARAM( args, CTEXTSTR, token );
	PSV_PARAM;
	AddSecurityContextToken( (uintptr_t)&pTask->security_modules, module, token );	
	return psv;
}

static uintptr_t CPROC SetTaskRunOn( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, system );
	CTEXTSTR my_system = InterShell_GetSystemName();
	PSV_PARAM;
	AddLink( &pTask->allowed_run_on, StrDup( system ) );
	//lprintf( "Compare %s vs %s", system, my_system );
	if( CompareMask( system, my_system, FALSE ) )
	{
		//lprintf( "Task allowed..." );
		// at least one matched...
		pTask->flags.bAllowedRun = 1;
	}
	else
	{
		//lprintf( "task disallowed..." );
		// at least one didn't match... if not AllowedRun && Disallowed, don't run
		// else if( !allowed and !disallowed) run
		pTask->flags.bDisallowedRun = 1;
	}
	return psv;
}

static uintptr_t CPROC SetTaskNoRunOn( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, system );
	CTEXTSTR my_system = InterShell_GetSystemName();
	PSV_PARAM;
	AddLink( &pTask->disallowed_run_on, StrDup( system ) );
	//lprintf( "Compare %s vs %s", system, my_system );
	if( CompareMask( system, my_system, FALSE ) )
	{
		pTask->flags.bDisallowedRun = 1;
	}
	return psv;
}

uintptr_t CPROC BeginButtonTaskInfo( uintptr_t psv, arg_list args )
{
	//BeginSubConfiguration( NULL, WIDE("Task Done") );
	return psv;
}


void AddTaskConfigs( PCONFIG_HANDLER pch )
{
	//lprintf( WIDE("Adding configuration handling for a task....") );
	AddConfigurationMethod( pch, WIDE("name=%m"), ConfigSetTaskName );
	AddConfigurationMethod( pch, WIDE("path=%m"), SetTaskPath );
	AddConfigurationMethod( pch, WIDE("program=%m"), SetTaskTask );
	AddConfigurationMethod( pch, WIDE("args=%m"), SetTaskArgs );
	AddConfigurationMethod( pch, WIDE("shutdown path=%m"), SetShutdownTaskPath );
	AddConfigurationMethod( pch, WIDE("shutdown program=%m"), SetShutdownTaskTask );
	AddConfigurationMethod( pch, WIDE("shutdown args=%m"), SetShutdownTaskArgs );
	AddConfigurationMethod( pch, WIDE("Security Token for [%m]%m"), SetTaskSecurity );
	AddConfigurationMethod( pch, WIDE("Launch at %i by %i"), SetLaunchResolution );
	AddConfigurationMethod( pch, WIDE("Launch at least %i by %i"), SetLeastLaunchResolution );
	AddConfigurationMethod( pch, WIDE("restart %b"), SetTaskRestart );
	AddConfigurationMethod( pch, WIDE("one time %b"), SetTaskOneTime );
	AddConfigurationMethod( pch, WIDE("click to stop %b"), SetTaskOneTimeClickStop );
	AddConfigurationMethod( pch, WIDE("non-exclusive %b"), SetTaskExclusive );
	AddConfigurationMethod( pch, WIDE("wait for task %b"), SetTaskWait );
	AddConfigurationMethod( pch, WIDE("background %b"), SetTaskBackground );
	AddConfigurationMethod( pch, WIDE("Capture task output?%b" ), SetTaskCapture );
	AddConfigurationMethod( pch, WIDE("Force Hide Display?%b" ), SetTaskHide );
	AddConfigurationMethod( pch, WIDE("Run task On %m" ), SetTaskRunOn );
	AddConfigurationMethod( pch, WIDE("Disallow task On %m" ), SetTaskNoRunOn );

}

uintptr_t  CPROC FinishConfigTask( uintptr_t psv, arg_list args )
{
	// just return NULL here, so there's no object to process
	// EndConfig?  how does the pushed state recover?
	return 0;
}

/* place holder for common subconfiguration start. */
static void OnLoadControl( WIDE("TaskInfo") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	lprintf( WIDE("Begin sub for task...") );
	AddTaskConfigs( pch );
}

static void OnLoadControl( WIDE("Task") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	//lprintf( "Begin sub for task..." );
	AddTaskConfigs( pch );
	//BeginSubConfiguration( WIDE( "TaskInfo" ), WIDE("Task Done") );
}

static void OnInterShellShutdown( WIDE("Task") )( void )
{
	KillSpawnedPrograms();
}

//---------------------------------------------------------------------------

uintptr_t CPROC CreateNewNetworkTask( uintptr_t psv, arg_list args )
{
	PLOAD_TASK pTask = CreateTask(NULL);
	pTask->flags.bExclusive = 0;
	pTask->flags.bAutoLaunch = 1;
	AddLink( &l.autoload, pTask );
	//l.flags.bTask = 1;
	BeginSubConfiguration( WIDE("TaskInfo"), WIDE("Task Done") );
	return (uintptr_t)pTask;
}

//---------------------------------------------------------------------------

uintptr_t CPROC CreateShellCommand( uintptr_t psv, arg_list args )
{
	PLOAD_TASK pTask = CreateTask(NULL);
	pTask->flags.bExclusive = 0;
	l.shell = pTask;
	//l.flags.bTask = 1;
	BeginSubConfiguration( WIDE("TaskInfo"), WIDE("Task Done") );
	return (uintptr_t)pTask;
}

//---------------------------------------------------------------------------

uintptr_t CPROC CreateWinShellCommand( uintptr_t psv, arg_list args )
{
	PLOAD_TASK pTask = CreateTask(NULL);
	pTask->flags.bExclusive = 0;
	l.windows_shell = pTask;
	//l.flags.bTask = 1;
	BeginSubConfiguration( WIDE("TaskInfo"), WIDE("Task Done") );
	return (uintptr_t)pTask;
}

//---------------------------------------------------------------------------

uintptr_t CPROC CreatePowerShellCommand( uintptr_t psv, arg_list args )
{
	PLOAD_TASK pTask = CreateTask(NULL);
	pTask->flags.bExclusive = 0;
	l.power_shell = pTask;
	//l.flags.bTask = 1;
	BeginSubConfiguration( WIDE("TaskInfo"), WIDE("Task Done") );
	return (uintptr_t)pTask;
}

//---------------------------------------------------------------------------

uintptr_t CPROC CreatePowerShellISECommand( uintptr_t psv, arg_list args )
{
	PLOAD_TASK pTask = CreateTask(NULL);
	pTask->flags.bExclusive = 0;
	l.power_shell_ise = pTask;
	//l.flags.bTask = 1;
	BeginSubConfiguration( WIDE("TaskInfo"), WIDE("Task Done") );
	return (uintptr_t)pTask;
}

//---------------------------------------------------------------------------

uintptr_t CPROC CreateNewWaitTask( uintptr_t psv, arg_list args )
{
	PLOAD_TASK pTask = CreateTask(NULL);
	pTask->flags.bLaunchWhenNetworkDriveUp = 1;
	pTask->flags.bAutoLaunch = 1;
	pTask->flags.bExclusive = 0;
	AddLink( &l.autoload, pTask );
	//l.flags.bTask = 1;
	// at this point how do I get the thing?
	//AddTaskConfigs( ... );
	BeginSubConfiguration( WIDE("TaskInfo"), WIDE("Task Done") );
	return (uintptr_t)pTask;
}

uintptr_t CPROC SetWaitForNetwork( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, yes_no );
	l.flags.wait_for_network_drive = yes_no;
	return psv;
}

uintptr_t CPROC AddAdditionalPath( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
#ifdef HAVE_ENVIRONMENT
	OSALOT_AppendEnvironmentVariable( WIDE( "PATH" ), path );
#endif
	l.more_path = StrDup( path );
	return psv;
}

uintptr_t CPROC AddPrependPath( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, path );
#ifdef HAVE_ENVIRONMENT
	OSALOT_PrependEnvironmentVariable( WIDE( "PATH" ), path );
#endif
	l.less_path = StrDup( path );
	return psv;
}

static void OnLoadCommon( WIDE("Tasks") )( PCONFIG_HANDLER pch )
{
	/* standard tasks, these will get task_info prefix to compliment task done suffix */
	AddConfigurationMethod( pch, WIDE("auto network task"), CreateNewNetworkTask );
	AddConfigurationMethod( pch, WIDE("auto task"), CreateNewNetworkTask );
	AddConfigurationMethod( pch, WIDE("Command Shell"), CreateShellCommand );
	AddConfigurationMethod( pch, WIDE("Windows Shell"), CreateWinShellCommand );
	AddConfigurationMethod( pch, WIDE("Power Shell"), CreatePowerShellCommand );
	AddConfigurationMethod( pch, WIDE("Power Shell ISE"), CreatePowerShellISECommand );
	//AddConfigurationMethod( pch, WIDE("{task_info}" ), BeginButtonTaskInfo );

	AddConfigurationMethod( pch, WIDE("<path more=\"%m\"}"), AddAdditionalPath );
	AddConfigurationMethod( pch, WIDE("<path less=\"%m\">"), AddPrependPath );
	AddConfigurationMethod( pch, WIDE("wait for network? %b"), SetWaitForNetwork );
}


//---------------------------------------------------------------------------

TEXTCHAR *GetTaskSecurity( PLOAD_TASK pTask )
{
	static TEXTCHAR args[256];
	snprintf( args, sizeof( args ), WIDE("none") );
	return args;
}

static void DumpTask( FILE *file, PLOAD_TASK pTask, int sub )
{
	//PLOAD_TASK pTask = (PLOAD_TASK)psv;
	if( pTask )
	{
		CTEXTSTR p;
		InterShell_SaveSecurityInformation( file, (uintptr_t)pTask );

		if( sub )
		{
			if( ( p = EscapeMenuString( pTask->pName ) ) )
				sack_fprintf( file, WIDE("%sname=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		}
		if( ( p = EscapeMenuString( pTask->pPath ) ) )
			sack_fprintf( file, WIDE("%spath=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		if( ( p =  EscapeMenuString( pTask->pTask ) ) )
			sack_fprintf( file, WIDE("%sprogram=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		if( ( p =  EscapeMenuString( GetTaskArgs( pTask, FALSE ) ) ) )
			sack_fprintf( file, WIDE("%sargs=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		if( ( p = EscapeMenuString( pTask->pShutdownPath ) ) )
			sack_fprintf( file, WIDE("%sshutdown path=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		if( ( p =  EscapeMenuString( pTask->pShutdownTask ) ) )
			sack_fprintf( file, WIDE("%sshutdown program=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		if( ( p =  EscapeMenuString( GetTaskArgs( pTask, TRUE ) ) ) )
			sack_fprintf( file, WIDE("%sshutdown args=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), p );
		if( pTask->flags.bLaunchAtLeast )
			sack_fprintf( file, WIDE("%slaunch at least %d by %d\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->launch_width, pTask->launch_height );
		if( pTask->flags.bLaunchAt )
			sack_fprintf( file, WIDE("%slaunch at %d by %d\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->launch_width, pTask->launch_height );
		else
			sack_fprintf( file, WIDE("%slaunch at 0 by 0\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->launch_width, pTask->launch_height );

		//if( !pTask->flags.bButton )
		sack_fprintf( file, WIDE("%srestart %s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bRestart?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%snon-exclusive %s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), (!pTask->flags.bExclusive)?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%swait for task %s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bWaitForTask?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%sbackground %s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bBackground?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%sone time %s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bOneLaunch?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%sclick to stop %s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bOneLaunchClickStop?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%sCapture task output?%s\n" ), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bCaptureOutput?WIDE("Yes"):WIDE("No") );
		sack_fprintf( file, WIDE("%sForce Hide Display?%s\n" ), sub?WIDE("\t"):InterShell_GetSaveIndent(), pTask->flags.bHideCanvas?WIDE("Yes"):WIDE("No") );
		{
			INDEX idx;
			INDEX idx2;
			struct task_security_module *module;
			LIST_FORALL( pTask->security_modules, idx, struct task_security_module *, module )
			{
				CTEXTSTR token;
				GetSecurityContextTokens( (uintptr_t)&pTask->security_modules, module->name, &module->tokens );
				LIST_FORALL( module->tokens, idx2, CTEXTSTR, token )
				{
					sack_fprintf( file, WIDE("%sSecurity Token for [%s]%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), module->name, token );
				}
			}
		}
		{
			INDEX idx;
			CTEXTSTR sysname;
			LIST_FORALL( pTask->allowed_run_on, idx, CTEXTSTR, sysname )
				sack_fprintf( file, WIDE("%sRun task on %s\n" ), sub?WIDE("\t"):InterShell_GetSaveIndent(), sysname );
			LIST_FORALL( pTask->disallowed_run_on, idx, CTEXTSTR, sysname )
				sack_fprintf( file, WIDE("%sDisallow task on %s\n" ), sub?WIDE("\t"):InterShell_GetSaveIndent(), sysname );
		}
		//if( pTask->flags.bButton )
		//sack_fprintf( file, WIDE("%ssecurity=%s\n"), sub?WIDE("\t"):InterShell_GetSaveIndent(), GetTaskSecurity( pTask ) );
		//if( pTask->pImage && pTask->pImage[0] )
		//	sack_fprintf( file, WIDE("image=%s\n"), EscapeMenuString( pTask->pImage ) );
		if( sub )
			sack_fprintf( file, WIDE( "%sTask Done\n" ), sub?WIDE(""):(InterShell_GetSaveIndent()+1) ); // drop back one level
	}
}

static void OnSaveControl( WIDE("Task") )( FILE *file, uintptr_t psv )
{
	DumpTask( file, (PLOAD_TASK)psv, 0 );
}

static void OnSaveCommon( WIDE("Tasks") )( FILE *file )
{
		int bWroteAutoNetwork = FALSE;
		int bWroteAuto = FALSE;
		if( l.shell )
		{
			sack_fprintf( file, WIDE("\n\nCommand Shell\n") );
			DumpTask( file, l.shell, 1 );
			sack_fprintf( file, WIDE("\n\n") );
		}
		if( l.power_shell )
		{
			sack_fprintf( file, WIDE("\n\nPower Shell\n") );
			DumpTask( file, l.power_shell, 1 );
			sack_fprintf( file, WIDE("\n\n") );
		}
		if( l.power_shell_ise )
		{
			sack_fprintf( file, WIDE("\n\nPower Shell ISE\n") );
			DumpTask( file, l.power_shell_ise, 1 );
			sack_fprintf( file, WIDE("\n\n") );
		}
		if( l.windows_shell )
		{
			sack_fprintf( file, WIDE("\n\nWindows Shell\n") );
			DumpTask( file, l.windows_shell, 1 );
			sack_fprintf( file, WIDE("\n\n") );
		}
		if( l.autoload )
		{
			INDEX idx;
			PLOAD_TASK pTask;
			LIST_FORALL( l.autoload, idx, PLOAD_TASK, pTask )
			{
				if( pTask->flags.bLaunchWhenNetworkDriveUp )
				{
					bWroteAutoNetwork = TRUE;
					sack_fprintf( file, WIDE("auto network task\n") );
				}
				else
				{
					bWroteAuto = TRUE;
					sack_fprintf( file, WIDE("auto task\n") );
				}
				DumpTask( file, pTask, 1 );
				sack_fprintf( file, WIDE("\n\n") );
			}
		}
		if( l.more_path )
			sack_fprintf( file, WIDE( "<path more=\"%s\">\n" ), l.more_path );
		if( l.less_path )
			sack_fprintf( file, WIDE( "<path less=\"%s\">\n" ), l.less_path );
		sack_fprintf( file, WIDE( "wait for network? %s\n\n" ), l.flags.wait_for_network_drive?WIDE("yes"):WIDE("no") );

}

//-------------------------------------------------------------------------------
// property methods for tasks which are auto load/invisible.


void CPROC EditNetworkTaskProperties( uintptr_t psv, PSI_CONTROL button )
{
	PLISTITEM pli = GetSelectedItem( GetNearControl( button, LISTBOX_AUTO_TASKS ) );
	if( pli )
	{
		TEXTCHAR buf[256];
		PLOAD_TASK task = (PLOAD_TASK)GetItemData( pli );
		EditTaskProperties( (uintptr_t)task, button, FALSE );
		snprintf( buf, sizeof( buf ), WIDE("%s%s%s")
				  , task->pName
				  , task->flags.bLaunchWhenNetworkDriveUp?WIDE("[NETWORK]"):WIDE("")
				  , task->flags.bRestart?WIDE("[RESTART]"):WIDE("") );
		SetItemText( pli, buf );
	}
}

void CPROC CreateNetworkTaskProperties( uintptr_t psv, PSI_CONTROL button )
{
	PLOAD_TASK task;
	task = CreateTask( NULL );
	EditTaskProperties( (uintptr_t)task, button, FALSE );
	// validate task, and perhaps destroy it?
	if( !task->pName[0] )
		StrCpy( task->pName, WIDE("NO PROGRAM") );
	{
		TEXTCHAR buf[256];
		snprintf( buf, sizeof( buf ), WIDE("%s%s%s")
				  , task->pName
				  , task->flags.bLaunchWhenNetworkDriveUp?WIDE("[NETWORK]"):WIDE("")
				  , task->flags.bRestart?WIDE("[RESTART]"):WIDE("") );
		SetItemData( AddListItem( GetNearControl( button, LISTBOX_AUTO_TASKS ), buf ), (uintptr_t)task );
	}
	AddLink( &l.autoload, task );
}

void CPROC DestroyNetworkTaskProperties( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list;
	PLISTITEM pli = GetSelectedItem( list = GetNearControl( button, LISTBOX_AUTO_TASKS ) );
	if( pli )
	{
		PLOAD_TASK task = (PLOAD_TASK)GetItemData( pli );
		DestroyTask( &task );
		DeleteListItem( list, pli );
	}
}


static void OnGlobalPropertyEdit( WIDE("Tasks") )( PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrame( WIDE("CommonTaskProperties.isFrame") );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		SetButtonPushMethod( GetControl( frame, BUTTON_EDIT_TASK_PROPERTIES ), EditNetworkTaskProperties, 0 );
		SetButtonPushMethod( GetControl( frame, BUTTON_CREATE_AUTO_TASK ), CreateNetworkTaskProperties, 0 );
		SetButtonPushMethod( GetControl( frame, BUTTON_DESTROY_AUTO_TASK ), DestroyNetworkTaskProperties, 0 );
		{
			PSI_CONTROL list = GetControl( frame, LISTBOX_AUTO_TASKS );
			if( list )
			{
				PLOAD_TASK task;
				INDEX idx;
				TEXTCHAR buf[256];
				SetItemData( AddListItem( list, WIDE("Command Shell") ), (uintptr_t)l.shell );
#ifdef WIN32
				SetItemData( AddListItem( list, WIDE("Explorer") ), (uintptr_t)l.windows_shell );
				SetItemData( AddListItem( list, WIDE("Power Shell") ), (uintptr_t)l.power_shell );
				SetItemData( AddListItem( list, WIDE("Power Shell ISE") ), (uintptr_t)l.power_shell_ise );
#endif
				LIST_FORALL( l.autoload, idx, PLOAD_TASK, task )
				{
					snprintf( buf, sizeof( buf ), WIDE("%s%s%s")
							  , task->pName
							  , task->flags.bLaunchWhenNetworkDriveUp?WIDE("[NETWORK]"):WIDE("")
							  , task->flags.bRestart?WIDE("[RESTART]"):WIDE("") );
					SetItemData( AddListItem( list, buf ), (uintptr_t)task );
				}
			}
			SetCheckState( GetControl( frame, CHECKBOX_NETWORK_WAIT ), l.flags.wait_for_network_drive );
		}
		DisplayFrameOver( frame, parent );
		CommonWait( frame );
		if( okay )
		{
			// command shell properties
			// wait for network flag
			// auto network tasks
			// auto tasks
			{
				l.flags.wait_for_network_drive = GetCheckState( GetControl( frame, CHECKBOX_NETWORK_WAIT ) );
			}
		}
		DestroyFrame( &frame );
	}
}

static void OnCloneControl( WIDE("Task") )( uintptr_t psvNew, uintptr_t psvOriginal )
{
	PLOAD_TASK pNewTask = (PLOAD_TASK)psvNew;
	PLOAD_TASK pOriginalTask = (PLOAD_TASK)psvOriginal;
	pNewTask[0] = pOriginalTask[0];
	pNewTask->button = NULL; // don't know the button we're cloning...
	pNewTask->spawns = NULL; // this button hasn't launched anything yet.
	pNewTask->allowed_run_on = NULL;
	pNewTask->disallowed_run_on = NULL;
	pNewTask->pArgs = NULL; // don't have args yet... so don't release anything when setting args.
	pNewTask->pShutdownArgs = NULL; // don't have args yet... so don't release anything when setting args.
 	SetTaskArguments( pNewTask, FALSE, GetTaskArgs( pOriginalTask, FALSE ) );
	SetTaskArguments( pNewTask, TRUE, GetTaskArgs( pOriginalTask, TRUE ) );
	{
		POINTER p;
		INDEX idx;
		LIST_FORALL( pOriginalTask->allowed_run_on, idx, POINTER, p )
		{
			AddLink( &pNewTask->allowed_run_on, p );
		}
		LIST_FORALL( pOriginalTask->disallowed_run_on, idx, POINTER, p )
		{
			AddLink( &pNewTask->disallowed_run_on, p );
		}
	}
}

struct resolution_button
{
	int width, height;
	PMENU_BUTTON button;
};

static void OnKeyPressEvent( WIDE("Task Util/Set Resolution") )( uintptr_t psv )
{
	struct resolution_button *resbut = (struct resolution_button *)psv;
	if( resbut->width && resbut->height )
		SetResolution( NULL, resbut->width, resbut->height, FALSE );
	else
		ResetResolution( NULL );
}

static uintptr_t OnCreateMenuButton( WIDE("Task Util/Set Resolution") )( PMENU_BUTTON button )
{
	struct resolution_button *resbut = New( struct resolution_button );
	resbut->width = 0;
	resbut->height = 0;
	resbut->button = button;
	return (uintptr_t)resbut;
}

static uintptr_t OnEditControl( WIDE("Task Util/Set Resolution") )( uintptr_t psv, PSI_CONTROL parent_frame )
{
	PSI_CONTROL frame = LoadXMLFrame( WIDE( "task.resolution.isframe" ) );
	int okay = 0;
	int done = 0;
	struct resolution_button *resbut = (struct resolution_button *)psv;
	if( frame )
	{
		TEXTCHAR buffer[15];
		SetCommonButtons( frame, &done, &okay );
		snprintf( buffer, sizeof( buffer ), WIDE("%ld"), resbut->width );
		SetControlText( GetControl( frame, EDIT_TASK_LAUNCH_X ), buffer );
		snprintf( buffer, sizeof( buffer ), WIDE("%ld"), resbut->height );
		SetControlText( GetControl( frame, EDIT_TASK_LAUNCH_Y ), buffer );
		DisplayFrameOver( frame, parent_frame );
		CommonWait( frame );
		if( okay )
		{
			GetControlText( GetControl( frame, EDIT_TASK_LAUNCH_X ), buffer, sizeof( buffer ) );
			resbut->width = atoi( buffer );
			GetControlText( GetControl( frame, EDIT_TASK_LAUNCH_Y ), buffer, sizeof( buffer ) );
			resbut->height = atoi( buffer );
		}
		DestroyFrame( &frame );
	}
	return psv;
}

static void OnSaveControl( WIDE("Task Util/Set Resolution") )( FILE *file, uintptr_t psv )
{
	struct resolution_button *resbut = (struct resolution_button *)psv;
	sack_fprintf( file, WIDE("launch at %d by %d\n"), resbut->width, resbut->height );
}

uintptr_t CPROC SetLaunchResolution2( uintptr_t psv, arg_list args )
{
	PARAM( args, int64_t, width );
	PARAM( args, int64_t, height );
	struct resolution_button *resbut = (struct resolution_button *)psv;
	if( resbut )
	{
		resbut->width = (int)width;
		resbut->height = (int)height;
	}
	return psv;
}



static void OnLoadControl( WIDE("Task Util/Set Resolution") )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, WIDE("Launch at %i by %i"), SetLaunchResolution2 );
}


static LOGICAL OnDropAccept( WIDE("Add Task Button") )( PSI_CONTROL pc_canvas, CTEXTSTR file, int x, int y )
{
	if( StrCaseStr( file, WIDE(".exe") )
		||StrCaseStr( file, WIDE(".bat") )
		||StrCaseStr( file, WIDE(".com") )
		||StrCaseStr( file, WIDE(".cmd") ) )
	{
		uintptr_t psv = InterShell_CreateControl( pc_canvas, WIDE("Task"), x, y, 5, 3 );
		PLOAD_TASK pTask = (PLOAD_TASK)psv;
		if( pTask )
		{
			//int argc;
			//TEXTCHAR **argv;
			TEXTCHAR *pathend;
			TEXTCHAR *ext;
			//ParseIntoArgs( (TEXTCHAR*)file, &argc, &argv );
			pathend = (TEXTCHAR*)pathrchr( file );
			if( pathend )
				pathend[0] = 0;
			else
				pathend = (TEXTCHAR*)file;
			StrCpyEx( pTask->pTask, pathend+1, 256 );
			ext = strrchr( pathend+1, '.' );
			if( ext )
				ext[0] = 0;

			if( pTask->button )
				InterShell_SetButtonText( pTask->button, pathend+1 );

			StrCpyEx( pTask->pPath, file, 256 );

			pTask->pArgs = NULL;

		}
		return 1;
	}
	return 0;
}


#if defined( __CMAKE_VERSION__ ) && ( __CMAKE_VERSION__ < 2081003 )
// cmake + watcom link failure fix
PUBLIC( void, ExportThis )( void )
{
}
#endif
