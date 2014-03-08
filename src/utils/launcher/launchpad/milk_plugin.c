#define USES_MILK_INTERFACE
#define DEFINES_MILK_INTERFACE
//#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <psi.h>
#include <milk_export.h>
#include <milk_registry.h>
#include <filesys.h> // use to compare using file masking ? and * wildcards

#include "launchpad.h"
#include "pad.h"

extern PLIST class_names;

typedef struct system_class_tag
{
	CTEXTSTR classname;
   CTEXTSTR system;
};

static struct {
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
};

PRELOAD(RegisterMyControl)
{
   EasyRegisterResource( "Milk/Launchpad", TXT_RESPOND_TO, EDIT_FIELD_NAME );
   EasyRegisterResource( "Milk/Launchpad", TXT_SYSTEM_NAME, EDIT_FIELD_NAME );
   EasyRegisterResource( "Milk/Launchpad", BTN_DELETE_PAIR, NORMAL_BUTTON_NAME );
   EasyRegisterResource( "Milk/Launchpad", BTN_ADD_PAIR, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "Milk/Launchpad", LISTBOX_SYSTEM_CLASS_NAMES, LISTBOX_CONTROL_NAME );
   l.system = MILK_GetSystemName();
	l.image = DecodeMemoryToImage( icon_image, sizeof( icon_image ) );
   SetTaskLogOutput();
   DoRegisterControl( &launchpad );
}

//-------------------------------------------------------------------------

static int OnDrawCommon( "Launcher Launchpad" )( PSI_CONTROL pc )
{
	BlotScaledImageAlpha( GetControlSurface( pc ), l.image, ALPHA_TRANSPARENT );
   return 1;
}


//-------------------------------------------------------------------------

static int OnMouseCommon( "Launcher Launchpad" )( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
   //if( b & MK_LBUTTON )
	//	Report();
   return 1;
}

//-------------------------------------------------------------------------

OnCreateControl( "Launcher Launchpad" )( PSI_CONTROL frame, S_32 x, S_32 y, _32 w, _32 h )
{
	PSI_CONTROL pc;
	pc = MakeNamedControl( frame
								, WIDE("Launcher Launchpad")
								, x
								, y
								, w
								, h
								, -1
								);
   AddLink( &l.icons, pc );
   return (PTRSZVAL) pc;
}

OnDestroyControl( "Launcher Launchpad" )( PTRSZVAL psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	DeleteLink( &l.icons, pc );
   DestroyCommon( &pc );
}

OnGetControl( "Launcher Launchpad" )( PTRSZVAL psv )
{
   return (PSI_CONTROL)psv;
}

static void CPROC AddPair( PTRSZVAL psv, PSI_CONTROL button )
{
	PSI_CONTROL system = GetNearControl( button, TXT_SYSTEM_NAME );
	PSI_CONTROL classname = GetNearControl( button, TXT_RESPOND_TO );
	char sysbuf[256];
	char classbuf[256];
	struct system_class_tag *newsystemclass;
	GetControlText( system, sysbuf, sizeof( sysbuf ) );
	GetControlText( classname, classbuf, sizeof( classbuf ) );
	newsystemclass = New( struct system_class_tag );
	newsystemclass->classname = StrDup( classbuf );
	newsystemclass->system = StrDup( sysbuf );
   snprintf( sysbuf, sizeof( sysbuf ), "%s\t%s", newsystemclass->classname
			 , 	newsystemclass->system );
	AddListItem( GetNearControl( button, LISTBOX_SYSTEM_CLASS_NAMES ), sysbuf );
}

static void AddSystemClass( CTEXTSTR classname, CTEXTSTR system )
{
	INDEX idx;
	struct system_class_tag *system_class_name;
	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		if( ( StrCaseCmp( classname,system_class_name->classname ) == 0 )
			&& ( StrCaseCmp( system, system_class_name->system ) == 0 ) )
		{
         break;
		}
	}
	if( !system_class_name )
	{
		system_class_name = New( struct system_class_tag );
		system_class_name->classname = StrDup( classname );
		system_class_name->system = StrDup( system );
		AddLink( &l.system_classnames, system_class_name );
		if( CompareMask( system_class_name->system, l.system, FALSE ) )
		{
         // this class didn't exist before, so remove it form this list...
			AddLink( &class_names, StrDup( classname ) );
		}
	}
}

static void DeleteSystemClass( CTEXTSTR classname, CTEXTSTR system )
{
	INDEX idx;
	struct system_class_tag *system_class_name;
	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		if( ( StrCaseCmp( classname,system_class_name->classname ) == 0 )
			&& ( StrCaseCmp( system, system_class_name->system ) == 0 ) )
		{
         break;
		}
	}
	if( system_class_name )
	{
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
		SetLink( &l.system_classnames, idx, NULL );
      Release( (POINTER)system_class_name->classname );
		Release( (POINTER)system_class_name->system );
		Release( system_class_name );
	}
}

static void CPROC DeletePair( PTRSZVAL psv, PSI_CONTROL button )
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


OnGlobalPropertyEdit( "Launcher Launchpad" )( PSI_CONTROL parent )
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
			int tabs[2] = { 5, 130 };
			if( list )
			{
				SetListBoxTabStops( list, 2, tabs );
				LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
				{
					char buffer[256];
					snprintf( buffer, sizeof( buffer ), "%s\t%s", system_class_name->classname, system_class_name->system );
					AddListItem( list, buffer );
				}
			}
         SetButtonPushMethod( GetControl( frame, BTN_ADD_PAIR ), AddPair, (PTRSZVAL)frame );
         SetButtonPushMethod( GetControl( frame, BTN_DELETE_PAIR ), DeletePair, (PTRSZVAL)list );
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
				GetListItemText( pli, buffer, sizeof( buffer ) );
				// results in a pointer within buffer, which is not const char, therefore is safe cast
				split = (TEXTSTR)StrChr( buffer, '\t' );
				split[0] = 0;
            AddSystemClass( buffer, split + 1 );
			}
			//GetControlText( GetControl( frame, TXT_RESPOND_TO ), buffer, sizeof( buffer ) );
         //if( class_name )
			//	Release( (POINTER)class_name );
         //class_name = StrDup( buffer );
		}
      DestroyFrame( &frame );
	}
}

OnSaveControl( "Launcher Launchpad" )( FILE *file, PTRSZVAL psv )
{
   struct system_class_tag *system_class_name;
	INDEX idx;
	LIST_FORALL( l.system_classnames, idx, struct system_class_tag*, system_class_name )
	{
		fprintf( file, "Launchpad class=%s@%s\n", system_class_name->classname, system_class_name->system );
	}
}

static PTRSZVAL CPROC SetLaunchpadClass( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, new_class_name );
	PARAM( args, CTEXTSTR, new_system_name );
	AddSystemClass( new_class_name, new_system_name );
   return psv;
}

static PTRSZVAL CPROC SetLaunchpadInterface( PTRSZVAL psv, arg_list args )
{
	PARAM( args, CTEXTSTR, interface );
	extern CTEXTSTR pInterfaceAddr;
   pInterfaceAddr = StrDup( interface );
   return psv;
}

OnLoadControl( "Launcher Launchpad" )( PCONFIG_HANDLER pch, PTRSZVAL psv )
{
   AddConfigurationMethod( pch, "Launchpad class=%m@%m", SetLaunchpadClass );
   AddConfigurationMethod( pch, "Launchpad Interface=%m", SetLaunchpadInterface );
}

//-------------------------------------------------------------------------

