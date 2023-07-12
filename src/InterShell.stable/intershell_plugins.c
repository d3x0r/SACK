#include <stdhdrs.h>
#include <filesys.h>
#include <deadstart.h>
#include "intershell_local.h"
#include "intershell_registry.h"

/*
 * Common template of a open configuration frame
 *
 */
struct configured_plugin {
	struct {
		BIT_FIELD bDelete : 1;
		BIT_FIELD bLoaded : 1;
		BIT_FIELD bNoLoad : 1;
	} flags;
	PLIST pLoadOn; // systems to load this plugin on (with wildcard support, don't need disallow as much
	PLIST pNoLoadOn; // systems to load this plugin on (with wildcard support, don't need disallow as much
	CTEXTSTR plugin_full_name;
	CTEXTSTR plugin_mask;
	CTEXTSTR plugin_extra_path;
	PLISTITEM pli; // the list item this is...
	generic_function function;
};

static struct {
	PLIST plugins; // list of plugins StrDup()s
	struct configured_plugin *current_plugin;
} local_intershell_common;


//-------------------------------------------------------------------------------

enum {
	LISTBOX_SYSTEMS = 21222
	  , LISTBOX_PLUGINS
	  , SYSTEM_NAME
	  , PLUGIN_NAME
	  , ADD_PLUGIN
	  , REMOVE_PLUGIN
	  , ADD_SYSTEM
	  , REMOVE_SYSTEM
	  , LISTBOX_NO_SYSTEMS
	  , ADD_NO_SYSTEM
	  , REMOVE_NO_SYSTEM
};

PRELOAD( RegisterPluginResources )
{
	EasyRegisterResource( "InterShell/plugins", LISTBOX_SYSTEMS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "InterShell/plugins", LISTBOX_PLUGINS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "InterShell/plugins", SYSTEM_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/plugins", PLUGIN_NAME, EDIT_FIELD_NAME );
	EasyRegisterResource( "InterShell/plugins", ADD_PLUGIN, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/plugins", REMOVE_PLUGIN, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/plugins", ADD_SYSTEM, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/plugins", REMOVE_SYSTEM, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/plugins", LISTBOX_NO_SYSTEMS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "InterShell/plugins", ADD_NO_SYSTEM, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "InterShell/plugins", REMOVE_NO_SYSTEM, NORMAL_BUTTON_NAME );

}

struct configured_plugin *GetPlugin( CTEXTSTR plugin_mask )
{

	struct configured_plugin *plugin;
	INDEX idx;
	TEXTSTR full_name = StrDup( plugin_mask );
	TEXTSTR extra_path = (TEXTSTR)StrChr( plugin_mask, ';' );
	if( extra_path )
	{
		extra_path[0] = 0;
		extra_path++;
	}
	LIST_FORALL( local_intershell_common.plugins, idx, struct configured_plugin*, plugin )
	{
		if( StrCaseCmp( plugin->plugin_mask, plugin_mask ) == 0 )
			break;
	}

	if( !plugin )
	{
		plugin = New( struct configured_plugin );
		plugin->plugin_full_name = full_name;
		plugin->plugin_mask = StrDup( plugin_mask );
		plugin->plugin_extra_path = StrDup( extra_path );
		plugin->pLoadOn = NULL;
		plugin->pNoLoadOn = NULL;
		plugin->pli = NULL;
		plugin->flags.bDelete = FALSE;
		plugin->flags.bLoaded = FALSE;
		plugin->flags.bNoLoad = FALSE;
		AddLink( &local_intershell_common.plugins, plugin );
	}
	else
		Release( full_name );
	return plugin;
}

//-------------------------------------------------------------------------------

static void CPROC PluginPicked( uintptr_t psv, PSI_CONTROL list, PLISTITEM pli )
{
	struct configured_plugin *plugin = (struct configured_plugin *)GetItemData( pli );
	PSI_CONTROL syslist = GetNearControl( list, LISTBOX_SYSTEMS );
	INDEX idx;
	if( plugin )
	{
		CTEXTSTR system;
		if( local_intershell_common.current_plugin )
		{
			PLISTITEM pli;
			{
				INDEX idx;
				POINTER p;
				LIST_FORALL( local_intershell_common.current_plugin->pNoLoadOn, idx, POINTER, p )
					Release( p );
				LIST_FORALL( local_intershell_common.current_plugin->pLoadOn, idx, POINTER, p )
					Release( p );
				DeleteList( &local_intershell_common.current_plugin->pNoLoadOn );
				DeleteList( &local_intershell_common.current_plugin->pLoadOn );
			}
			syslist = GetNearControl( list, LISTBOX_SYSTEMS );
			if( syslist )
			{
				for( pli = GetNthItem( syslist, (INDEX)(idx = 0) ); pli; pli = GetNthItem( syslist, ( INDEX )(++idx)) )
				{
					TEXTCHAR buffer[256];
					GetListItemText( pli, buffer, sizeof( buffer ) );
					AddLink( &local_intershell_common.current_plugin->pLoadOn, StrDup( buffer ) );
				}
			}

			syslist = GetNearControl( list, LISTBOX_NO_SYSTEMS );
			if( syslist )
			{
				for( pli = GetNthItem( syslist, (INDEX)(idx = 0) ); pli; pli = GetNthItem( syslist, (INDEX)( ++idx ) ) )
				{
					TEXTCHAR buffer[256];
					GetListItemText( pli, buffer, sizeof( buffer ) );
					AddLink( &local_intershell_common.current_plugin->pNoLoadOn, StrDup( buffer ) );
				}
			}
		}

		syslist = GetNearControl( list, LISTBOX_SYSTEMS );
		if( syslist )
		{
			ResetList( syslist );
			LIST_FORALL( plugin->pLoadOn, idx, CTEXTSTR, system )
			{
				SetItemData( AddListItem( syslist, system ), (uintptr_t)system );
			}
		}

		syslist = GetNearControl( list, LISTBOX_NO_SYSTEMS );
		if( syslist )
		{
			ResetList( syslist );
			LIST_FORALL( plugin->pNoLoadOn, idx, CTEXTSTR, system )
			{
				SetItemData( AddListItem( syslist, system ), (uintptr_t)system );
			}
		}
		local_intershell_common.current_plugin = plugin;
	}
}

//-------------------------------------------------------------------------------

static void CPROC AddPlugin( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL plugin_name = GetNearControl( button, PLUGIN_NAME );
	if( plugin_name )
	{
		TEXTCHAR name[256];
		struct configured_plugin *plugin;
		GetControlText( plugin_name, name, sizeof( name ) );
		plugin = GetPlugin( name );
		if( !plugin->pli )
		{
			PSI_CONTROL list;
			SetSelectedItem( list
								, SetItemData( plugin->pli = AddListItem( list = GetNearControl( button, LISTBOX_PLUGINS ), name ), (uintptr_t)plugin )
								);
		}
	}
}

//-------------------------------------------------------------------------------

static void CPROC AddSystem( uintptr_t psv, PSI_CONTROL button )
{
	if( local_intershell_common.current_plugin )
	{
		PSI_CONTROL system_name = GetNearControl( button, SYSTEM_NAME );
		if( system_name )
		{
			TEXTCHAR name[256];
			CTEXTSTR system;
			INDEX idx;
			GetControlText( system_name, name, sizeof( name ) );
			LIST_FORALL( local_intershell_common.current_plugin->pLoadOn, idx, CTEXTSTR, system )
			{
				if( StrCaseCmp( system, name ) == 0 )
					break;
			}
			if( !system )
			{
				AddLink( &local_intershell_common.current_plugin->pLoadOn, StrDup( name ) );
				SetItemData( AddListItem( GetNearControl( button, LISTBOX_SYSTEMS ), name ), 0 );
			}
		}
	}
}


//-------------------------------------------------------------------------------

static void CPROC AddNoSystem( uintptr_t psv, PSI_CONTROL button )
{
	if( local_intershell_common.current_plugin )
	{
		PSI_CONTROL system_name = GetNearControl( button, SYSTEM_NAME );
		if( system_name )
		{
			TEXTCHAR name[256];
			CTEXTSTR system;
			INDEX idx;
			GetControlText( system_name, name, sizeof( name ) );
			LIST_FORALL( local_intershell_common.current_plugin->pLoadOn, idx, CTEXTSTR, system )
			{
				if( StrCaseCmp( system, name ) == 0 )
					break;
			}
			if( !system )
			{
				AddLink( &local_intershell_common.current_plugin->pLoadOn, StrDup( name ) );
				SetItemData( AddListItem( GetNearControl( button, LISTBOX_NO_SYSTEMS ), name ), 0 );
			}
		}
	}
}


//-------------------------------------------------------------------------------
static void CPROC RemovePlugin( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list = GetNearControl( button, LISTBOX_PLUGINS );
	PLISTITEM pli = GetSelectedItem( list );
	if( pli )
	{
		struct configured_plugin *plugin = (struct configured_plugin *)GetItemData( pli );
		DeleteLink( &local_intershell_common.plugins, plugin );
		{
			INDEX idx;
			POINTER p;
			LIST_FORALL( plugin->pLoadOn, idx, POINTER, p )
			{
				Release( p );
			}
		}
		DeleteList( &plugin->pLoadOn );
		Release( (TEXTSTR)plugin->plugin_full_name );
		Release( (TEXTSTR)plugin->plugin_mask );
		Release( (TEXTSTR)plugin->plugin_extra_path );
		Release( plugin );
		DeleteListItem( list, pli );
		local_intershell_common.current_plugin = NULL; // clean this up after the config handlers above...
	}
}
//-------------------------------------------------------------------------------
static void CPROC RemoveSystem( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list = GetNearControl( button, LISTBOX_SYSTEMS );
	PLISTITEM pli = GetSelectedItem( list );
	if( pli )
	{
		DeleteListItem( list, pli );
	}
}
//-------------------------------------------------------------------------------
static void CPROC RemoveNoSystem( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list = GetNearControl( button, LISTBOX_NO_SYSTEMS );
	PLISTITEM pli = GetSelectedItem( list );
	if( pli )
	{
		DeleteListItem( list, pli );
	}
}
//-------------------------------------------------------------------------------


static void InitControls( PSI_CONTROL frame )
{
	PSI_CONTROL list;
	list = GetControl( frame, LISTBOX_PLUGINS );
	if( list )
	{
		INDEX idx;
		struct configured_plugin *plugin;
		LIST_FORALL( local_intershell_common.plugins, idx, struct configured_plugin *, plugin )
		{
			SetItemData( plugin->pli = AddListItem( list, plugin->plugin_full_name ), (uintptr_t)plugin );
		}
		SetSelChangeHandler( list, PluginPicked, 0 );
	}

	SetButtonPushMethod( GetControl( frame, ADD_PLUGIN ), AddPlugin, 0 );
	SetButtonPushMethod( GetControl( frame, REMOVE_PLUGIN ), RemovePlugin, 0 );
	SetButtonPushMethod( GetControl( frame, ADD_SYSTEM ), AddSystem, 0 );
	SetButtonPushMethod( GetControl( frame, REMOVE_SYSTEM ), RemoveSystem, 0 );
	SetButtonPushMethod( GetControl( frame, ADD_NO_SYSTEM ), AddNoSystem, 0 );
	SetButtonPushMethod( GetControl( frame, REMOVE_NO_SYSTEM ), RemoveNoSystem, 0 );
}

static void UpdateFromControls( PSI_CONTROL frame )
{
	INDEX idx;
	struct configured_plugin *plugin;
	PSI_CONTROL list = GetControl( frame, LISTBOX_PLUGINS );
	PLISTITEM pli;

	PluginPicked( 0, list, GetSelectedItem( list ) );

	LIST_FORALL( local_intershell_common.plugins, idx, struct configured_plugin *, plugin )
	{
		plugin->flags.bDelete = 1;
	}

	for( pli = GetNthItem( list, idx = 0 ); pli; pli = GetNthItem( list, ++idx ) )
	{
		plugin = (struct configured_plugin*)GetItemData( pli );
		plugin->flags.bDelete = FALSE;
	}
	LIST_FORALL( local_intershell_common.plugins, idx, struct configured_plugin *, plugin )
	{
		CTEXTSTR p;
		if( plugin->flags.bDelete )
		{
			SetLink( &local_intershell_common.plugins, idx, NULL );
			LIST_FORALL( plugin->pLoadOn, idx, CTEXTSTR, p )
				Release( (POINTER)p );
			LIST_FORALL( plugin->pNoLoadOn, idx, CTEXTSTR, p )
				Release( (POINTER)p );
			DeleteList( &plugin->pLoadOn );
			Release( plugin );
		}
		else
		{
		}
	}
}

void ClearControls( void )
{
	local_intershell_common.current_plugin = NULL;
}

static void OnGlobalPropertyEdit( "Edit Plugins" )( PSI_CONTROL parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( parent, "EditPlugins.isFrame" );
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		InitControls( frame );
		//lprintf( "show frame over parent." );
		DisplayFrameOver( frame, parent );
		//lprintf( "Begin waiting..." );
		CommonWait( frame );
		if( okay )
		{
			UpdateFromControls( frame );
		}
		ClearControls();
		DestroyFrame( &frame );
	}
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------


static uintptr_t CPROC LoadConfigPlugin( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, filemask );
	local_intershell_common.current_plugin = GetPlugin( filemask );
	return psv;
}

static uintptr_t CPROC NoLoadConfigPluginOn( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, systemmask );
	if( local_intershell_common.current_plugin )
	{
		AddLink( &local_intershell_common.current_plugin->pNoLoadOn, StrDup( systemmask ) );
		if( CompareMask( systemmask, InterShell_GetSystemName(), FALSE ) )
		{
			local_intershell_common.current_plugin->flags.bNoLoad = TRUE;
			// add just this libraries global config stuff....
		}
	}
	return psv;
}

//-------------------------------------------------------------------------------
static uintptr_t CPROC LoadConfigPluginOn( uintptr_t psv, arg_list args )
{
	PARAM( args, CTEXTSTR, systemmask );
	if( local_intershell_common.current_plugin )
	{
		AddLink( &local_intershell_common.current_plugin->pLoadOn, StrDup( systemmask ) );
		if( CompareMask( systemmask, InterShell_GetSystemName(), FALSE ) )
		{
			if( !local_intershell_common.current_plugin->flags.bNoLoad )
			{
				local_intershell_common.current_plugin->flags.bLoaded = TRUE;
				LoadInterShellPlugins( NULL, local_intershell_common.current_plugin->plugin_mask, local_intershell_common.current_plugin->plugin_extra_path );
				// register additional OnLoadCommon methods avaialble now.
				InvokeLoadCommon();
				// add just this libraries global config stuff....
			}
		}
	}
	return psv;
}
//-------------------------------------------------------------------------------

// load at a high priority... so that plugins loaded by this might have a chance
// to register their own load common things, before the line happens in the file
static void OnLoadCommon( "@00 Plugins" )( PCONFIG_HANDLER pch )
{
	AddConfigurationMethod( pch, "<plugin filemask='%m'>", LoadConfigPlugin );
	AddConfigurationMethod( pch, "<plugin_load system='%m'>", LoadConfigPluginOn );
	AddConfigurationMethod( pch, "<plugin_no_load system='%m'>", NoLoadConfigPluginOn );

}

//-------------------------------------------------------------------------------
// save at a high priority... so that plugins loaded by this might have a chance
// to register their own load common things...
static void OnSaveCommon( "@00 Plugins" )( FILE *file )
{
	struct configured_plugin *plugin;
	INDEX idx;
	LIST_FORALL( local_intershell_common.plugins, idx, struct configured_plugin*, plugin )
	{
		INDEX idx;
		CTEXTSTR system;
  		sack_fprintf( file, "<plugin filemask=\'%s\'>\n", EscapeMenuString( plugin->plugin_full_name ) );
		LIST_FORALL( plugin->pNoLoadOn, idx, CTEXTSTR, system )
		{
			sack_fprintf( file, "<plugin_no_load system=\'%s\'>\n", EscapeMenuString( system ) );
		}
		LIST_FORALL( plugin->pLoadOn, idx, CTEXTSTR, system )
		{
			sack_fprintf( file, "<plugin_load system=\'%s\'>\n", EscapeMenuString( system ) );
		}
	}
}


//-------------------------------------------------------------------------------
static void OnFinishInit( "Plugins" )( PSI_CONTROL pc_canvas )
{
	local_intershell_common.current_plugin = NULL; // clean this up after the config handlers above...
}
