#define USES_INTERSHELL_INTERFACE
#define DEFINES_INTERSHELL_INTERFACE
#define USE_IMAGE_INTERFACE l.pii
#include <psi.h>
#include <sack_system.h> // GetProgramName()
#include <sqlgetoption.h>
#include "../../../InterShell/intershell_export.h"
#include "../../../InterShell/intershell_registry.h"
#include <filesys.h> // use to compare using file masking ? and * wildcards

#include "launchpad.h"
#include "pad.h"

extern PLIST class_names;

struct system_class_system_tag
{
	struct
	{
		BIT_FIELD bNot : 1; // opposite logic test for class
	} flags;
	CTEXTSTR system;
};

struct system_class_tag
{
	CTEXTSTR classname;
	PLIST systems;  // list of struct system_class_system_tag*
};

static struct {
	PIMAGE_INTERFACE pii;
	Image image;
	CTEXTSTR system; // my system name
	PLIST system_classnames;
	PLIST icons;
} l;

CONTROL_REGISTRATION launchpad = { "Launcher Launchpad", {{ 32, 32 }, 0, BORDER_NONE|BORDER_FIXED }
};

enum {
	TXT_RESPOND_TO = 5000
	  , TXT_SYSTEM_NAME
	  , BTN_DELETE_PAIR
	  , BTN_ADD_PAIR
	  , LISTBOX_SYSTEM_CLASS_NAMES
	  , BTN_DELETE_NOT_PAIR
	  , BTN_ADD_NOT_PAIR
};

PRELOAD(RegisterMyControl)
{
	l.pii = GetImageInterface();
	EasyRegisterResource( "InterShell/Launchpad", TXT_RESPOND_TO, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/Launchpad", TXT_SYSTEM_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/Launchpad", BTN_DELETE_PAIR, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Launchpad", BTN_ADD_PAIR, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Launchpad", BTN_DELETE_NOT_PAIR, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Launchpad", BTN_ADD_NOT_PAIR, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/Launchpad", LISTBOX_SYSTEM_CLASS_NAMES, LISTBOX_CONTROL_NAME );
	l.system = InterShell_GetSystemName();
	l.image = DecodeMemoryToImage( icon_image, sizeof( icon_image ) );
	SetTaskLogOutput();
	DoRegisterControl( &launchpad );
   if( SACK_GetProfileIntEx( GetProgramName(), "Launchpad/Log Packet Receive", 0, TRUE ) )
	{
		extern int bLogPacketReceive;
		bLogPacketReceive = 1;
	}
}

//-------------------------------------------------------------------------

static int OnDrawCommon( "Launcher Launchpad" )( PSI_CONTROL pc )
{
	BlotScaledImageAlpha( GetControlSurface( pc ), l.image, ALPHA_TRANSPARENT );
	return 1;
}


//-------------------------------------------------------------------------

static int OnMouseCommon( "Launcher Launchpad" )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	return 1;
}

//-------------------------------------------------------------------------

static uintptr_t OnCreateControl( "Launcher Launchpad" )( PSI_CONTROL frame, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( frame
								, "Launcher Launchpad"
								, x
								, y
								, w
								, h
								, -1
								);
   AddLink( &l.icons, pc );
   return (uintptr_t) pc;
}

static void OnDestroyControl( "Launcher Launchpad" )( uintptr_t psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	DeleteLink( &l.icons, pc );
	DestroyCommon( &pc );
}

static PSI_CONTROL OnGetControl( "Launcher Launchpad" )( uintptr_t psv )
{
	return (PSI_CONTROL)psv;
}

static struct system_class_tag *GetSystemClass( CTEXTSTR classname )
{
	INDEX idx;
	struct system_class_tag *system_class_name;
	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		if( ( StrCaseCmp( classname, system_class_name->classname ) == 0 ) )
		{
			break;
		}
	}
	if( !system_class_name )
	{
		system_class_name = New( struct system_class_tag );
		system_class_name->classname = StrDup( classname );
		system_class_name->systems = NULL;
		AddLink( &l.system_classnames, system_class_name );
	}
	return system_class_name;
}

static struct system_class_system_tag *GetSystemClassSystem( struct system_class_tag *system_class_name, CTEXTSTR name )
{
	INDEX idx;
	struct system_class_system_tag *system_class_system_name;
	LIST_FORALL( system_class_name->systems, idx, struct system_class_system_tag*, system_class_system_name )
	{
		if( ( StrCaseCmp( name, system_class_system_name->system ) == 0 ) )
			break;
	}
	if( !system_class_system_name )
	{
		system_class_system_name = New( struct system_class_system_tag );
		system_class_system_name->system = StrDup( name );
		AddLink( &system_class_name->systems, system_class_system_name );
	}
	return system_class_system_name;
}

static void CPROC AddPair( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL system = GetNearControl( button, TXT_SYSTEM_NAME );
	PSI_CONTROL classname = GetNearControl( button, TXT_RESPOND_TO );
	TEXTCHAR sysbuf[256];
	TEXTCHAR classbuf[256];
	struct system_class_tag *newsystemclass;
	struct system_class_system_tag *newsystemclasssystem;
	GetControlText( system, sysbuf, sizeof( sysbuf ) );
	GetControlText( classname, classbuf, sizeof( classbuf ) );
	
	newsystemclass = GetSystemClass( classbuf );
	newsystemclasssystem = GetSystemClassSystem( newsystemclass, sysbuf );
	newsystemclasssystem->flags.bNot = 0;
	snprintf( sysbuf, sizeof( sysbuf ), "%s\t%s\t%s"
			, newsystemclass->classname
			, newsystemclasssystem->system
			, newsystemclasssystem->flags.bNot?"NOT":""
			);
	AddListItem( GetNearControl( button, LISTBOX_SYSTEM_CLASS_NAMES ), sysbuf );
}

static void CPROC AddNotPair( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL system = GetNearControl( button, TXT_SYSTEM_NAME );
	PSI_CONTROL classname = GetNearControl( button, TXT_RESPOND_TO );
	TEXTCHAR sysbuf[256];
	TEXTCHAR classbuf[256];
	struct system_class_tag *newsystemclass;
	struct system_class_system_tag *newsystemclasssystem;
	GetControlText( system, sysbuf, sizeof( sysbuf ) );
	GetControlText( classname, classbuf, sizeof( classbuf ) );
	
	newsystemclass = GetSystemClass( classbuf );
	newsystemclasssystem = GetSystemClassSystem( newsystemclass, sysbuf );
	newsystemclasssystem->flags.bNot = 1;
	snprintf( sysbuf, sizeof( sysbuf ), "%s\t%s\t%s"
			, newsystemclass->classname
			, newsystemclasssystem->system
			, newsystemclasssystem->flags.bNot?"NOT":""
			);
	AddListItem( GetNearControl( button, LISTBOX_SYSTEM_CLASS_NAMES ), sysbuf );
}

static void AddSystemClassEx( CTEXTSTR classname, CTEXTSTR system, LOGICAL bExclusion )
{
	struct system_class_tag *system_class_name = GetSystemClass( classname );
	struct system_class_system_tag *system_class_system_name = GetSystemClassSystem( system_class_name, system );
	system_class_system_name->flags.bNot = bExclusion;
}

static void DeleteSystemClass( CTEXTSTR classname, CTEXTSTR system )
{
	INDEX idx;
	INDEX idx2;
	struct system_class_tag *system_class_name;
	struct system_class_system_tag *system_class_system_name;
	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		LIST_FORALL( system_class_name->systems, idx2, struct system_class_system_tag*, system_class_system_name )
		if( ( StrCaseCmp( classname, system_class_name->classname ) == 0 )
			&&( StrCaseCmp( system, system_class_system_name->system ) == 0 )  )
		{
			break;
		}
		if( system_class_system_name )
			break;
	}
	if( system_class_name )
	{
		/* should fix this loop to remove the active classname from launchpad */
#if 0
		if( CompareMask( system_class_name->system, l.system, FALSE ) )
		{
			INDEX idx;
			// remove this from list of active classes being processed...
			LIST_FORALL( class_names, idx, CTEXTSTR, classname )
			{
				if( StrCaseCmp( classname, system_class_name->classname ) == 0 )
					break;
			}
			if( classname )
			{
				SetLink( &class_names, idx, NULL );
				Release( (POINTER)classname );
			}
		}
#endif
		{
			int used = 0;
			INDEX idx3;
			struct system_class_system_tag *system_class_system_name2;
			LIST_FORALL( system_class_name->systems, idx3, struct system_class_system_tag*, system_class_system_name2 )
			{
				if( system_class_system_name2 == system_class_system_name )
				{
					Release( (POINTER)system_class_system_name2->system );
					Release( system_class_system_name2 );
					SetLink( &system_class_name->systems, idx3, NULL );
				}
				else
					used++;
			}
			if( used == 0 )
			{
				SetLink( &l.system_classnames, idx, NULL );
				Release( (POINTER)system_class_name->classname );
				DeleteList( &system_class_name->systems );
				Release( system_class_name );
			}
		}
	}
}

static void CPROC DeletePair( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL listbox = GetNearControl( button, LISTBOX_SYSTEM_CLASS_NAMES );
	if( listbox )
	{
		PLISTITEM pli = GetSelectedItem( listbox );
		if( pli )
		{
			TEXTCHAR buffer[256];
			TEXTSTR split;
			GetListItemText( pli, buffer, sizeof( buffer ) );
			// results in a pointer within buffer, which is not const char, therefore is safe cast
			split = (TEXTSTR)StrChr( buffer, '\t' );
			split[0] = 0;
			DeleteSystemClass( buffer, split + 1 ); // class, ssytem
			DeleteListItem( listbox, pli );
		}
	}
}

static void CPROC DeleteNotPair( uintptr_t psv, PSI_CONTROL button )
{
}

static void OnFinishInit( "@Launchpad" )( PCanvasData canvas )
{
	INDEX idx;
	struct system_class_tag *system_class_name;

	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		LOGICAL bNoAdd = FALSE;
		LOGICAL bAdd = FALSE;
		INDEX idx2;
		struct system_class_system_tag *system_class_system_name;
		LIST_FORALL( system_class_name->systems, idx2, struct system_class_system_tag*, system_class_system_name )
		{
			if( system_class_system_name->flags.bNot )
			{
				if( CompareMask( system_class_system_name->system, l.system, FALSE ) )
				{
					bNoAdd = TRUE;
				}
				else
					bAdd = TRUE;
			}
			else
			{
				if( CompareMask( system_class_system_name->system, l.system, FALSE ) )
				{
					bAdd = TRUE;
				}
			}
		}
		if( bAdd && !bNoAdd )
		{
			TEXTCHAR translated_class[256];
			InterShell_TranslateLabelText( NULL, translated_class, sizeof( translated_class ), system_class_name->classname );
			lprintf( "This will respond to class [%s]", system_class_name->classname );

			AddLink( &class_names, StrDup( translated_class ) );
		}
	}

}

static void OnGlobalPropertyEdit( "Launcher Launchpad" )( PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "ConfigureLaunchpad.Frame" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );

		{
			struct system_class_tag *system_class_name;
			PSI_CONTROL list = GetControl( frame, LISTBOX_SYSTEM_CLASS_NAMES );
			INDEX idx;
			int tabs[3] = { 5, 130, 230 };
			if( list )
			{
				SetListBoxTabStops( list, 3, tabs );
				LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
				{
					INDEX idx2;
					struct system_class_system_tag *system_class_system_name;
					TEXTCHAR buffer[256];
					LIST_FORALL( system_class_name->systems, idx2, struct system_class_system_tag *, system_class_system_name )
					{
						snprintf( buffer, sizeof( buffer ), "%s\t%s\t%s"
							, system_class_name->classname
							, system_class_system_name->system
							, system_class_system_name->flags.bNot?"NOT":"" );
						AddListItem( list, buffer );
					}
				}
			}
			SetButtonPushMethod( GetControl( frame, BTN_ADD_PAIR ), AddPair, (uintptr_t)frame );
			SetButtonPushMethod( GetControl( frame, BTN_DELETE_PAIR ), DeletePair, (uintptr_t)list );
			SetButtonPushMethod( GetControl( frame, BTN_ADD_NOT_PAIR ), AddNotPair, (uintptr_t)frame );
			SetButtonPushMethod( GetControl( frame, BTN_DELETE_NOT_PAIR ), DeleteNotPair, (uintptr_t)list );
		}
		//SetControlText( GetControl( frame, TXT_RESPOND_TO ), class_name );

		DisplayFrame( frame );
		CommonWait( frame );
		if( okay )
		{
			TEXTCHAR buffer[256];
			int n;
			PLISTITEM pli;
			PSI_CONTROL list = GetControl( frame, LISTBOX_SYSTEM_CLASS_NAMES );
			for( n = 0; pli = GetNthItem( list, n ); n++ )
			{
				TEXTSTR split;
				TEXTSTR split2;
				GetListItemText( pli, buffer, sizeof( buffer ) );
				// results in a pointer within buffer, which is not const char, therefore is safe cast
				split = (TEXTSTR)StrChr( buffer, '\t' );
				split[0] = 0;
				split2 = (TEXTSTR)StrChr( split+1, '\t' );
				split2[0] = 0;
				AddSystemClassEx( buffer, split + 1, split2[1]=='N'?TRUE:FALSE );
			}
			//GetControlText( GetControl( frame, TXT_RESPOND_TO ), buffer, sizeof( buffer ) );
			//if( class_name )
			//	Release( (POINTER)class_name );
			//class_name = StrDup( buffer );
		}
		DestroyFrame( &frame );
	}
}

static void OnSaveCommon( "Launcher Launchpad" )( FILE *file )
{
	struct system_class_tag *system_class_name;
	INDEX idx;
	// MAKE SURE exclusions are written before inclusions?
	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		struct system_class_system_tag *system_class_system_name;
		INDEX idx2;
		LIST_FORALL( system_class_name->systems, idx2, struct system_class_system_tag*, system_class_system_name )
		{
			if( system_class_system_name->flags.bNot )
				fprintf( file, "Launchpad no class=%s@%s\n", system_class_name->classname, system_class_system_name->system );
			else
				fprintf( file, "Launchpad class=%s@%s\n", system_class_name->classname, system_class_system_name->system );
		}
	}
}

static uintptr_t CPROC SetLaunchpadClass( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, new_class_name );
	PARAM( args, CTEXTSTR, new_system_name );
	lprintf( "Add '%s'@'%s'", new_class_name, new_system_name );
	AddSystemClassEx( new_class_name, new_system_name, FALSE );
	return psv;
}

static uintptr_t CPROC SetLaunchpadNotClass( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, new_class_name );
	PARAM( args, CTEXTSTR, new_system_name );
	lprintf( "Add '%s'^'%s'", new_class_name, new_system_name );
	AddSystemClassEx( new_class_name, new_system_name, TRUE );
	return psv;
}

static uintptr_t CPROC SetLaunchpadInterface( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, interface_address );
	extern CTEXTSTR pInterfaceAddr;
	pInterfaceAddr = StrDup( interface_address );
	return psv;
}

static void OnLoadCommon( "Launcher Launchpad" )( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, "Launchpad class=%m@%m", SetLaunchpadClass );
	AddConfigurationMethod( pch, "Launchpad no class=%m@%m", SetLaunchpadNotClass );
	AddConfigurationMethod( pch, "Launchpad Interface=%m", SetLaunchpadInterface );
}

static void OnLoadControl( "Launcher Launchpad" )( PCONFIG_HANDLER pch, uintptr_t psv )
{
	AddConfigurationMethod( pch, "Launchpad class=%m@%m", SetLaunchpadClass );
}

//-------------------------------------------------------------------------

#ifdef __WATCOMC__
PUBLIC( void, AtLeastOneExport )( void )
{
}
#endif
