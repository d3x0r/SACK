#ifndef CORE_SOURCE
#define CORE_SOURCE
#endif
//#define __STATIC__ // defined so that we link to static dll ref...
#include <stdhdrs.h>
#define DO_LOGGING
#include <logging.h>
#include <procreg.h>
#ifdef _WIN32
#if !defined( __TURBOC__ ) && ! defined( __CYGWIN__ ) && !defined( __LCC__ ) && !defined( _MSC_VER ) && !defined( __WATCOMC__ )
#include <dirent.h>
#endif
#include <io.h>  // findfirst,findnext, fileinfo
#else
#include <errno.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#endif
//#include <types.h>
#include <filesys.h>
#include <sharemem.h>

#define PLUGIN_LOADER
#include "local.h"
#include "input.h"
#include "commands.h"

#include "plugin.h" // export table structure definition...

CORE_PROC( void, AddVolatileVariables )( PENTITY pe, CTEXTSTR root );
//CORE_PROC( int, RegisterDevice )( TEXTCHAR *pName, TEXTCHAR *pDescription, PDATAPATH (CPROC *Open)( PDATAPATH *ppChannel, PSENTIENT ps, PTEXT params ) );
typedef void (CPROC *MainFunction )( int argc, TEXTCHAR **argv, int bConsole );

//--------------------------------------------------------------------------

static PPLUGIN pPlugins;
PPLUGIN pPluginLoading; // list of modules we loaded....

//static PDEVICE pDeviceRoot;

//--------------------------------------------------------------------------

void DumpLoadedPluginList( PSENTIENT ps )
{
	PPLUGIN pPlugin = pPlugins;
	PVARTEXT vt;
	vt = VarTextCreate();
	while( pPlugin )
	{
		vtprintf( vt, "%s", pPlugin->pName );
		EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		pPlugin = pPlugin->pNext;
	}
	VarTextDestroy( &vt );
}

//--------------------------------------------------------------------------

PPLUGIN AddPlugin( CTEXTSTR pName )
{
	PPLUGIN pPlugin;
	pPlugin = NewPlus( struct plugin_tag, StrLen( pName ) + 1 );
	MemSet( pPlugin, 0, sizeof(*pPlugin) );
	StrCpy( pPlugin->pName, pName );

	pPluginLoading = pPlugin;
	return pPlugin;
}

//--------------------------------------------------------------------------

void CommitPlugin( void )
{
	if( pPluginLoading )
	{
		if( pPlugins )
			pPlugins->pPrior = pPluginLoading;
		pPluginLoading->pNext = pPlugins;
		pPlugins = pPluginLoading;
		pPluginLoading = NULL;
	}
}

//--------------------------------------------------------------------------
void AbortPlugin( void )
{
	UnloadFunction( (generic_function*)&pPluginLoading->RegisterRoutines );
	//UnloadFunction( &pPluginLoading->Unload );
	Release( pPluginLoading );
	pPluginLoading = NULL;
}
//--------------------------------------------------------------------------

void UnloadPlugin( PPLUGIN pPlugin )
{
	PPLUGIN pFind;
	//if( pPlugin->Unload )
	//	pPlugin->Unload();
	UnloadFunction( (generic_function*)&pPlugin->RegisterRoutines );
	//UnloadFunction( &pPlugin->Unload );

	pFind = pPlugins;
	while( pFind )
	{
		if( pFind == pPlugin )
			break;
		pFind = pFind->pNext;
	}
	if( pFind )
	{
		if( pFind->pNext )
			pFind->pNext->pPrior = pFind->pPrior;
		if( pFind->pPrior )
			pFind->pPrior->pNext = pFind->pNext;
		else
			pPlugins = pFind->pNext;
	}
	else if( pPlugin == pPluginLoading )
	{
		pPluginLoading = NULL;
	}

	Release( pPlugin );
}
//--------------------------------------------------------------------------

static CTEXTSTR gpFile;


void LoadPlugin( CTEXTSTR pFile, PSENTIENT ps, PTEXT parameters )
{
	gpFile = pFile;

	//Log1( "Loading Plugin %s", pFile );
	if( pFile )
	{
		PPLUGIN pPlugin = AddPlugin( pFile );
		pPlugin->RegisterRoutines = (RegisterRoutinesProc)LoadPrivateFunctionEx( pFile, NULL DBG_SRC );
		//Unload = (UnloadPluginProc)LoadPrivateFunctionEx( pFile, "UnloadPlugin" DBG_SRC );
		if( pPlugin->RegisterRoutines )
		{
		}
		else
		{
			if( ps && parameters )
			{
				MainFunction Main;
				Main = (MainFunction)LoadPrivateFunction( pFile, "Main" );
				if( Main )
				{
					PTEXT line = SegAppend( SegCreateFromText( "DekwarePlugin " ), MacroDuplicateEx( ps, parameters, TRUE, TRUE ) );
					PTEXT flat = BuildLine( line );
					int argc;
					TEXTCHAR **argv;
					ParseIntoArgs( GetText( flat ), &argc, &argv );
					AddPlugin( pFile );
					LineRelease( flat );
					LineRelease( line );
					Main( argc, argv, 0 );
					CommitPlugin();
					return;
				}
			}
		}
	}
	else
	{
		//TEXTCHAR byMsg[256];
		//sprintf( byMsg, "Failed to resolve plugin module routine in %s", pFile );
#ifdef _WIN32
		//MessageBox( NULL, byMsg, "Plugin Failure", MB_OK );
#else
		//fprintf( stderr, "%s\n", byMsg );
#endif
		//			AbortPlugin();
	}
	CommitPlugin();
}

void CPROC LoadAPlugin( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags )
{
	lprintf( "Plugin:%s", pathrchr( name )+1 );
	LoadPlugin( name, NULL, NULL );
}

static LOGICAL AddTmpPath( TEXTCHAR *tmp )
{
	// if loading a plugin from a path, add that path to the PATH
	CTEXTSTR old_environ = OSALOT_GetEnvironmentVariable( "PATH" );
#ifdef WIN32
#define PATH_SEP_CHAR ';'
#else
#define PATH_SEP_CHAR ':'
#endif
	// safe conversion.
	size_t len = StrLen( tmp );
	CTEXTSTR result;
	if( !( result = StrCaseStr( old_environ, tmp ) ) )
	{
		OSALOT_PrependEnvironmentVariable( "PATH", tmp );
		return TRUE;
	}
	else
	{
		if( result == old_environ )
		{
			if( result[len] != PATH_SEP_CHAR )
			{
				OSALOT_PrependEnvironmentVariable( "PATH", tmp );
				return TRUE;
			}
		}
		else if( result[-1] == PATH_SEP_CHAR )
		{
			// is not at the end of the string, and it's not a ;
			// otherwise end of string is OK.
			if( result[len] && ( result[len] != PATH_SEP_CHAR ) )
			{
				OSALOT_PrependEnvironmentVariable( "PATH", tmp );
				return TRUE;
			}
		}
		else
		{
			OSALOT_PrependEnvironmentVariable( "PATH", tmp );
			return TRUE;
		}
	}
	return FALSE;
}
//--------------------------------------------------------------------------
static int iObjectDevice;

void LoadPlugins( CTEXTSTR base )
{
	TEXTCHAR savepath[256];
	void *pInfo;
	FILE *pluginfile;
	CTEXTSTR old_environ = StrDup( OSALOT_GetEnvironmentVariable( "PATH" ) );
	pInfo = NULL;
	// this *.nex is ignored except in windows pattern search support...
#ifndef __ANDROID__
	GetCurrentPath( savepath, 256 );
	SetCurrentPath( base );
#endif

	pluginfile = sack_fopen( 0, "plugins.list", "rt" );
	if( !pluginfile )
	{
#ifdef __ANDROID__
		while( ScanFiles( base, "lib*.nex.so", &pInfo, LoadAPlugin, SFF_DEFAULT, 0 ) );
#else
		while( ScanFiles( base, "*.nex", &pInfo, LoadAPlugin, SFF_DEFAULT, 0 ) );
#endif
	}
	else
	{
		TEXTCHAR buf[256];
		while( fgets( buf, sizeof( buf ), pluginfile ) )
		{
			CTEXTSTR r;
			TEXTCHAR filename[256];
			buf[strlen( buf)-1] = 0; // kill \n from end of line
			if( r = pathrchr( buf ) )
			{
				LOGICAL bAppended = FALSE;
				{
					// use length of r-buf to print into a buffer for the base path of the plugin to set as path (if missing)
					PVARTEXT pvt;
					PTEXT result;

					pvt = VarTextCreate();
					vtprintf( pvt,	"%s/%*.*s", base, r-buf, r-buf, buf );
					result = VarTextGet( pvt );
					VarTextDestroy( &pvt );

					bAppended = AddTmpPath( GetText( result ) );

					LineRelease( result );
				}

				while( ScanFiles( filename, r+1, &pInfo, LoadAPlugin, SFF_DEFAULT, 0 ) );
				if( bAppended )
					OSALOT_SetEnvironmentVariable( "PATH", old_environ );
			}
			else
			{
				while( ScanFiles( base, buf, &pInfo, LoadAPlugin, SFF_DEFAULT, 0 ) );
			}
		}
		fclose( pluginfile );
	}

#ifndef __ANDROID__
	SetCurrentPath( savepath );
#endif
	OSALOT_SetEnvironmentVariable( "PATH", old_environ );
	Release( (POINTER)old_environ );

	pPluginLoading = (PPLUGIN)1;
	{
		//		 extern PDATAPATH (CPROC OpenObject)( PDATAPATH *pChannel, PSENTIENT ps, PTEXT params );
		//		 iObjectDevice = RegisterDevice( "object", "Datapath designed to accept input from other objects", OpenObject );
	}
	pPluginLoading = NULL;
}

//--------------------------------------------------------------------------

void UnloadPlugins( void )
{
	while( pPlugins )
	{
		UnloadPlugin( pPlugins );
	}
//	UnregisterDevice( "object" );
}

//--------------------------------------------------------------------------
// Unload is called from internal command interface....
void Unload( PTEXT pCommandName )
{
	PPLUGIN pPlugin;
	TEXTCHAR *filename;
	pPlugin = pPlugins;
	while( pPlugin )
	{
		if( TextLike( pCommandName, pPlugin->pName ) )
			break;
		filename = (TEXTCHAR*)pathrchr( pPlugin->pName );
		if( filename )
		{
			filename++;
			if( TextLike( pCommandName, filename ) )
				break;
		}
		pPlugin = pPlugin->pNext;
	}
	if( pPlugin )
		UnloadPlugin( pPlugin );
}

//--------------------------------------------------------------------------


CORE_PROC( void, RegisterRoutine )( CTEXTSTR pClassname, CTEXTSTR pName, CTEXTSTR pDescription, RoutineAddress Routine )
{
	//PREGROUTINE prr;
	TEXTCHAR tmp[256];
	if( !pPluginLoading )
	{
		Log1( "Cannot Register routines except when plugin is loading...%p\n", pPluginLoading );
		//return;
	}
	snprintf( tmp, sizeof( tmp ), "Dekware/commands/%s", pName );
	SimpleRegisterMethod( "Dekware/commands", Routine
							  , "int", pName, "(PSENTIENT,PTEXT)" );
	RegisterValue( tmp, "Description", pDescription );
	RegisterValue( tmp, "Command Class", pClassname );
}

//--------------------------------------------------------------------------

CORE_PROC( void, UnregisterRoutine )( CTEXTSTR pName )
{
	Log( "Plugin is attempting to Unregister an unknown routine..." );
}
//--------------------------------------------------------------------------

extern int gbTrace;


Function GetRoutineRegistered( CTEXTSTR prefix, PTEXT Command )
{
	Function f;
	TEXTCHAR tmp[256];
	if( prefix )
		snprintf( tmp, sizeof( tmp ), "dekware/commands/%s", prefix );
	else
		StrCpy( tmp, "dekware/commands" );

	f = GetRegisteredProcedure2( tmp, int, GetText(Command), (PSENTIENT,PTEXT) );
	return f;
}

OptionHandler GetOptionRegistered( CTEXTSTR prefix, PTEXT Command )
{
	OptionHandler f;
	TEXTCHAR tmp[256];
	if( prefix )
		snprintf( tmp, sizeof( tmp ), "dekware/devices/%s/options", prefix );
	else
		snprintf( tmp, sizeof( tmp ), "dekware/options" );

	f = GetRegisteredProcedure2( tmp, int, GetText(Command), (PDATAPATH,PSENTIENT,PTEXT) );
	return f;
}


int RoutineRegistered( PSENTIENT ps, PTEXT Command )
{
	int (CPROC *f)(PSENTIENT ps, PTEXT params );

	f = GetRoutineRegistered( NULL, Command );
	if( f )
	{
		extern int CanProcess( PSENTIENT ps, Function function );
		if( CanProcess( ps, f ) )
		{
			f(ps, NEXTLINE( Command ));
			return 1;
		}
		return -1;
	}
	return 0;
}

//--------------------------------------------------------------------------

static int nDevice;
static int nTypeID;

CORE_PROC( int, RegisterDeviceOpts )( CTEXTSTR pName
											  , CTEXTSTR pDescription
											  , PDATAPATH (CPROC *Open)( PDATAPATH *ppChannel, PSENTIENT ps, PTEXT params )
											  , option_entry *pOptions
											  , uint32_t nOptions )
{
	nTypeID = ++nDevice;
	if( pName && pDescription )
	{
		PCLASSROOT root;

		SimpleRegisterMethod( "dekware/devices", Open, "PDATAPATH", pName, "(PDATAPATH*,PSENTIENT,PTEXT)" );
		root = GetClassRootEx( (PCLASSROOT)"dekware/devices", pName );
		RegisterValue( (CTEXTSTR)root, "Description", pDescription );
		RegisterValue( (CTEXTSTR)root, "Name", pName );
		RegisterIntValue( (CTEXTSTR)root, "TypeID", nTypeID );

		{
			TEXTCHAR tmp[64];
			snprintf( tmp, sizeof( tmp ), "dekware/devices/%d", nTypeID );
			RegisterClassAlias( (CTEXTSTR)root, tmp );
		}
		
		if( pOptions && nOptions )
		{
			uint32_t i;
			for( i = 0; i < nOptions; i++ )
			{
				TEXTCHAR tmp[256];
				TEXTCHAR tmp2[256];
				CTEXTSTR name;
				snprintf( tmp, sizeof( tmp ), "Dekware/devices/%s/options/%s"
						  , pName
						  , name = GetText( (PTEXT)&pOptions[i].name ) );
				snprintf( tmp2, sizeof( tmp2 ), "Dekware/devices/%s/options"
						  , pName );
				if( CheckClassRoot( tmp ) )
				{
					lprintf( "%s already registered", tmp );
					continue;
				}
				//lprintf( "regsiter %s", tmp );
				SimpleRegisterMethod( tmp2, pOptions[i].function
										  , "int", name, "(PDATAPATH,PSENTIENT,PTEXT)" );
				RegisterValue( tmp, "Description", GetText( (PTEXT)&pOptions[i].description ) );
			}
			//RegisterOptions( pName, pOptions, nOptions );
		}
		return nTypeID;
	}
	return 0;
}

CORE_PROC( int, RegisterDevice )( CTEXTSTR pName, CTEXTSTR pDescription, PDATAPATH (CPROC *Open)( PDATAPATH *ppChannel, PSENTIENT ps, PTEXT params ) )
{
	return RegisterDeviceOpts( pName, pDescription, Open, NULL, 0 );
}
//--------------------------------------------------------------------------

CORE_PROC( void, UnregisterDevice )( CTEXTSTR pName )
{
#if 0
	PDEVICE pdev, prior;
	PDATAPATH pdp;
	prior = NULL;
	pdev = pDeviceRoot;
	while( pdev )
	{
		if( !strcmp( GetText( pdev->name ), pName ) )
		{
			INDEX idx;
			LIST_FORALL( pdev->pOpenPaths, idx, PDATAPATH, pdp ) 
			{
				lprintf( "Unregister is destroying path: %s(%d) %p (%p:%p)", pName, pdev->TypeID, pdp, pDeviceRoot, pdev );
				DestroyDataPath( pdp );
			}

			if( prior )
			{
				prior->pNext = pdev->pNext;
			}
			else
				pDeviceRoot = pdev->pNext;
			//Log1( "Unlinked from possible datapaths:%s", pName );
			LineRelease( pdev->name );
			LineRelease( pdev->description );
			DeleteList( &pdev->pOpenPaths );
			Release( pdev );
			break;
		}
		prior = pdev;
		pdev = pdev->pNext;
	}
#endif
}

//--------------------------------------------------------------------------

void PrintRegisteredDevices( PLINKQUEUE *ppOutput )
{
	WriteCommandList2( ppOutput, "dekware/devices", NULL );
}

//--------------------------------------------------------------------------

CORE_PROC( PDATAPATH, FindOpenDevice )( PSENTIENT ps, PTEXT pName )
{
	PDATAPATH pdp;
	int n;
	for( n = 0; n < 2; n++ )
	{
		if( n )
			pdp = ps->Command;
		else
			pdp = ps->Data;
		while( pdp )
		{
			CTEXTSTR devname = GetRegisteredValue( (CTEXTSTR)pdp->pDeviceRoot, "Name" );

			if( SameText( pName, pdp->pName ) == 0 )
			{
				return pdp;
			}
			if( devname && TextIs( pName, devname ) )
			{
				return pdp;
			}
			pdp = pdp->pPrior;
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------

CORE_PROC( DeviceOpenDevice, FindDevice )( PTEXT pName )
{
	DeviceOpenDevice f = GetRegisteredProcedure2( "dekware/devices", PDATAPATH, GetText( pName ), (PDATAPATH*,PSENTIENT,PTEXT));
	return f;
}

//--------------------------------------------------------------------------

int CPROC OptionDevice( PSENTIENT ps, PTEXT params )
{
	PTEXT devname = GetParam( ps, &params );
	if( devname )
	{
		PTEXT saveparm = params;
		PDATAPATH pdp = FindOpenDevice( ps, devname );
		if( pdp )
		{
			PCLASSROOT option_root = GetClassRootEx( pdp->pDeviceRoot, "options" );
			//int32_t idx = -1;
			if( params )
			{
				PTEXT temp;
				temp = GetParam( ps, &params );
				//if( pdp->pDevice->pOptions && pdp->pDevice->nOptions )
				{
					OptionHandler fOptionHandler;
					fOptionHandler = GetRegisteredProcedure2( option_root, int, GetText(temp), (PDATAPATH,PSENTIENT,PTEXT) );
					if( fOptionHandler )
					{
						if( ps->CurrentMacro )
						{
							ps->CurrentMacro->state.flags.bSuccess = !fOptionHandler(pdp,ps,params);
						}
						else
							fOptionHandler(pdp, ps, params );
						return 0;
					}
					else
					{
						DumpRegisteredNamesFrom( pdp->pDeviceRoot );
						DumpRegisteredNamesFrom( option_root );
						}
				}
				//else
				{
					if( pdp->Option )
					{
						pdp->Option( pdp, ps, saveparm );
						//idx = 0;
						return 0;
					}
					else
					{
						DECLTEXT( msg, "No options for device." );
						EnqueLink( &ps->Command->Output, &msg );
					}
				}
			}
			//if( idx < 0 )
			WriteCommandList2( &ps->Command->Output
								  , (CTEXTSTR)option_root
								  , NULL );
		}
		else
		{
			if( ps->CurrentMacro )
			{
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
			}
			else
			{
				DECLTEXT( msg, "Unknown device specified for options" );
				EnqueLink( &ps->Command->Output, &msg );
			}
		}
	}
	else
	{
		if( ps->CurrentMacro )
		{
			ps->CurrentMacro->state.flags.bSuccess = FALSE;
		}
		else
		{
			DECLTEXT( msg, "Must supply a device name to set options for." );
			EnqueLink( &ps->Command->Output, &msg );
		}
	}
	return 0;
}

//--------------------------------------------------------------------------

void SetDatapathType( PDATAPATH pdp, int nType )
{
	if( pdp )
	{
		// not sure why this would happen
		// to change from one to another... really this
		// is intended to just set the id...
		//PDEVICE pdev = FindDeviceByID( nType );
		//PDEVICE olddev = pdp->Type?FindDeviceByID( pdp->Type ):NULL;
		//if( olddev )
		//{
			//lprintf( "deleting device %p from list on %p", pdp, pdev );
		//	DeleteLink( &olddev->pOpenPaths, pdp );
		//}
		//if( pdev )

		{
			TEXTCHAR tmp[20];
			snprintf( tmp, sizeof( tmp ), "%d", nType );
			pdp->pDeviceRoot = GetClassRootEx( (PCLASSROOT)"dekware/devices", tmp );
			pdp->Type = nType;
		}
	}
}

//--------------------------------------------------------------------------

PDATAPATH OpenDevice( PDATAPATH *pChannel, PSENTIENT ps, PTEXT pName, PTEXT parameters )
{
	DeviceOpenDevice dev_open;
	//PDEVICE pdev;
	PDATAPATH pdp = NULL;
	dev_open = FindDevice( pName );
	if( !dev_open )
	{
		if( !PeekData( &ps->MacroStack ) )
			S_MSG( ps, "No device type named %s", GetText( pName ) );
	}
	else
	{
		//lprintf( "Opening a device named %s", GetText( pName ) );
		pdp = dev_open( pChannel, ps, parameters );
		if( pdp )
		{
			pdp->pDeviceRoot = GetClassRootEx( (PCLASSROOT)"dekware/devices", GetText( pName ) );
			if( !pdp->pDeviceRoot )
				DumpRegisteredNamesFrom( (PCLASSROOT)"dekware/devices" );
			{
				uintptr_t id = GetRegisteredIntValue( pdp->pDeviceRoot, "TypeID" );
				if( !id )
				{
					TEXTCHAR tmp[20];
					nTypeID = ++nDevice;
					id = nTypeID;
					RegisterIntValue( (CTEXTSTR)pdp->pDeviceRoot, "TypeID", nTypeID );
					RegisterValue( (CTEXTSTR)pdp->pDeviceRoot, "Name", GetText( pName  ) );
					snprintf( tmp, sizeof( tmp ), "%d", nTypeID );
					RegisterClassAlias( (CTEXTSTR)GetClassRootEx( (PCLASSROOT)"dekware/devices", (PCLASSROOT)tmp ), (CTEXTSTR)pdp->pDeviceRoot );
				}
				pdp->Type = nTypeID;
				//SetDatapathType( pdp, (int)id );
			}
			AddVolatileVariables( ps->Current, (CTEXTSTR)pdp->pDeviceRoot );
			//lprintf( "Adding device %p to list on %p", pdp, pdev );
			// if it's previously set, this will delete and re-add
			// maintaining a single reference count
			pdp->Owner = ps;
		}
	}
	return pdp;
}

//--------------------------------------------------------------------------

int CloseDevice( PDATAPATH pdp )
{
	PDEVICE pDevice = NULL;
	if( pdp->Close )
	{
		int nType = pdp->Type;
		if( pdp->Close( pdp ) ) // non zero result is error..
		{
			if( pdp->Type != nType )
			{
				//lprintf( "Lost the type - restoring" );
				pdp->Type = nType;
			}
			return FALSE;
		}
		if( pdp->Type != nType )
		{
			//lprintf( "Lost the type - restoring" );
			pdp->Type = nType;
		}
	}
	//else
	//	lprintf( "No close to call on %s", pDevice?GetText( pDevice->name ):"UNREGISTERED DEVICE" );

	if( pdp->Type )
	{
#if 0
		// previously, device tracked what datapaths it had opened...
		// need like a PLIST registered type associated with devices to track some of this
		// performance issue counters (basically)

		pDevice = pDeviceRoot;
		while( pDevice )
		{
			//Log3( "Close Device: is %d(%s) == %d? "
			//		, pDevice->TypeID
			//		, GetText( pDevice->name )
			//		, pdp->Type );
			if( pDevice->TypeID == pdp->Type )
			{
				break;
			}
			pDevice = pDevice->pNext;
		}
		if( pDevice )
		{
			//lprintf( "Deleting device %p from list on %p", pdp, pDevice );
			if( !DeleteLink( &pDevice->pOpenPaths, pdp ) )
			{
				lprintf( "Failed to find link to delete device %s", GetText( pDevice->name ) );
				DebugBreak();
			}
		}
		else
		{
			DebugBreak();
			Log( "Failed to find device to remove datapath from." );
		}
#endif
	}
	else
	{
		//lprintf( "Device called %s had type 0 (sysinternal)", GetText( pdp->pName ) );
		//DebugBreak();
		// only device that this should be is uhmm user interface...
	}
	return TRUE;
}

//--------------------------------------------------------------------------

CORE_PROC( INDEX, RegisterExtension )( CTEXTSTR pName )
{
	SetLink( &global.ExtensionNames, global.ExtensionCount, StrDup( pName ) );
	return global.ExtensionCount++;
}

//--------------------------------------------------------------------------

CORE_PROC( void, AddVolatileVariables )( PENTITY pe, CTEXTSTR root )
{
	if( pe )
	{
		PCLASSROOT data;
		CTEXTSTR name;
		PCLASSROOT tmproot = GetClassRootEx( (PCLASSROOT)root, "variables" );
		for( name = GetFirstRegisteredName( (CTEXTSTR)tmproot, &data );
			  name;
			  name = GetNextRegisteredName( &data ) )
		{
			// foreach root/variables create a volatile variable reference on the object
			volatile_variable_entry *pvve_add = New( volatile_variable_entry );
			pvve_add->pName = name;
			pvve_add->get = GetRegisteredProcedure2( GetCurrentRegisteredTree(&data), PTEXT, "get", (PENTITY,PTEXT*));
			pvve_add->set = GetRegisteredProcedure2( GetCurrentRegisteredTree(&data), PTEXT, "set", (PENTITY,PTEXT));
			pvve_add->pLastValue = NULL; // no last value.
			AddLink( &pe->pVariables, pvve_add );
		}
	 }
}

//--------------------------------------------------------------------------

CORE_PROC( void,  UnregisterObject )( CTEXTSTR pName )
{
	// find the named object archtype and remove it from the list of knowns...
	static uint32_t unregistering;
	while( LockedExchange( &unregistering, 1 ) )
	{
		Relinquish();
	}
#if 0
	if( pCurrent )
	{
		if( (*pCurrent->me) = pCurrent->pNext )
			pCurrent->pNext->me = pCurrent->me;
		{
			INDEX idx;
			PENTITY pe;
			LIST_FORALL( pCurrent->CreatedObjects, idx, PENTITY, pe )
				DestroyEntity( pe );
			DeleteList( &pCurrent->CreatedObjects );
		}
		LineRelease( pCurrent->name );
		LineRelease( pCurrent->description );
		Release( pCurrent );
	}
	else
	{
		Log1( "Failed to find registered object : %s", pName );
	}
#endif
	unregistering = 0;
}

//--------------------------------------------------------------------------

CORE_PROC( void, RegisterObjectEx )( CTEXTSTR pName
											  , CTEXTSTR pDescription
											  , ObjectInit Init
												DBG_PASS )
{
	// should confirm the names... and delete duplicates...
	SimpleRegisterMethod( "dekware/objects", Init, "int", pName, "(PSENTIENT,PENTITY,PTEXT)");
	RegisterValueExx( (PCLASSROOT)"dekware/objects", pName, "Description", FALSE, pDescription );
}

ObjectInit ScanRegisteredObjects( PENTITY pe, CTEXTSTR for_name )
{
	PCLASSROOT current_obj;
	CTEXTSTR name;
	//DebugBreak();
	if( !for_name )
		return NULL;
	for( name = GetFirstRegisteredName( (CTEXTSTR)"dekware/objects", &current_obj );
					 name;
		  name = GetNextRegisteredName( &current_obj ) )
	{
		if( StrCaseCmp( for_name, name ) == 0 )
		{
			if( pe )
			{
				AddLink( &pe->pMethods, GetCurrentRegisteredTree(&current_obj));
				AddVolatileVariables( pe, (CTEXTSTR)GetCurrentRegisteredTree(&current_obj) );
				return ReadRegisteredProcedure( current_obj, int, (PSENTIENT,PENTITY,PTEXT ) );
			}
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------

int ListRegisteredObjects( PSENTIENT ps, PTEXT parameters )
{
	// parameters are ignored
	// enques the list of registered objects and descriptions to the sentient
	PCLASSROOT current_obj;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( (CTEXTSTR)"dekware/objects", &current_obj );
					 name;
		  name = GetNextRegisteredName( &current_obj ) )
//	while( pCurrent )
	{
		PTEXT out;
		out = SegCreate( 256 );
		out->data.size = snprintf(out->data.data, 256 * sizeof( TEXTCHAR ), "%-20s %s"
										, name
										, GetRegisteredValue( (CTEXTSTR)GetCurrentRegisteredTree(&current_obj), "Description" )
										);
		EnqueLink( &ps->Command->Output, out );
	}
	return 1;
}

//--------------------------------------------------------------------------

void DestroyRegisteredObject( PENTITY pe )
{
	Log( "Could not find entity to destroy?!" );
}

//--------------------------------------------------------------------------


// ps is used with parameters
// pe is the object actually assimilatin
int Assimilate( PENTITY pe, PSENTIENT ps, CTEXTSTR name, PTEXT parameters )
{
	ObjectInit Init;
	// this is for an exisitng object to become an extended object
	// problem with this is... if it's already aware ...
	//	especially if it's a temporary sentience... such as executing
	//	a command as an object...

	// some of the real work of this routine is done in ScanRegisteredObjects
	if( Init = ScanRegisteredObjects( pe, name ) )
	{
		if( !Init( ps, pe, parameters ) )
		{
			if( ps && ps->CurrentMacro )
				ps->CurrentMacro->state.flags.bSuccess = TRUE;
		}
		else
		{
			if( ps )
			{
				DECLTEXT( msg, "Initialzation method failed." );
				// if in a macro, should not log just set result...
				EnqueLink( &ps->Command->Output, &msg );
			}
			return 0;
		}
	}
	else
	{
		if( ps )
		{
			DECLTEXT( msg, "No such object type has been registered." );
			EnqueLink( &ps->Command->Output, &msg );
		}
		return 0;
	}
	return 0;
}

static int HandleCommand( "Object", "Assimilate", "This object inherits being an objec to some type" )( PSENTIENT ps, PTEXT parameters )
{
	PTEXT type = GetParam( ps, &parameters );
	if( !type )
	{
		DECLTEXT( msg, "Must specify type of object to make..." );
		EnqueLink( &ps->Command->Output, &msg );
		ListRegisteredObjects( ps, NULL );
		return 0;	
	}
	return Assimilate( ps->Current, ps, GetText( type ), parameters );
}
//--------------------------------------------------------------------------

int CPROC CreateRegisteredObject( PSENTIENT ps, PTEXT parameters )
{
	// handles the command MAKE <type> <named>
	PTEXT type, name;
	ObjectInit Init;
	PENTITY pe;
	name = GetParam( ps, &parameters );
	if( !name )
	{
		DECLTEXT( msg, "Must specify name and type of object to make..." );
		EnqueLink( &ps->Command->Output, &msg );
		ListRegisteredObjects( ps, NULL );
		return 0;
	}
	type = GetParam( ps, &parameters );
	if( !type )
	{
		DECLTEXT( msg, "Must specify type of object to make..." );
		EnqueLink( &ps->Command->Output, &msg );
		ListRegisteredObjects( ps, NULL );
		return 0;
	}
	pe = CreateEntityIn( ps->Current, SegDuplicate( name ) );
	if( Init = ScanRegisteredObjects( pe, GetText( type ) ) )
	{
		{
			if( !Init( ps, pe, parameters ) )
			{
				//AddLink( &arch->CreatedObjects, pe );
				//AddLink( &pe->pDestroy, DestroyRegisteredObject );
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
			}
			else
			{
				DECLTEXT( msg, "Initialzation method failed." );
				// if in a macro, should not log just set result...
				EnqueLink( &ps->Command->Output, &msg );
				DestroyEntity( pe );
				return 0;
			}
		}
	}
	else
	{
		DECLTEXT( msg, "No such object type has been registered." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	return 1;
}

//--------------------------------------------------------------------------

int IsObjectTypeOf( PSENTIENT ps, PTEXT entity, PTEXT form )
{
	PENTITY pe;
	pe = (PENTITY)FindThing( ps, &entity, ps->Current, FIND_VISIBLE, NULL );
	if( pe )
	{
		//PARCHTYPE arch = FindArchtype( form );
		//if( arch )
		{
			INDEX idx;
			CTEXTSTR name;
			LIST_FORALL( global.ExtensionNames, idx, CTEXTSTR, name )
			{
				if( StrCaseCmp( name, GetText( form ) ) == 0 )
				{
					if( GetLink( &pe->pPlugin, idx ) )
						return TRUE;
					// else object does not have one of these extensions
				}
			}
			S_MSG( ps, "Object %s is not a type of %s", GetText( GetName( pe ) ), GetText( form ) );
		}
		//else
		//{
		//	if( !ps->CurrentMacro )
		//		S_MSG( ps, "Type %s is not a registered type", GetText( form ) );
		//}
	}
	else
	{
		if( !ps->CurrentMacro )
			S_MSG( ps, "Failed to find object named %s", GetText( entity ) );
	}
	return FALSE;
}

int CPROC ListExtensions( PSENTIENT ps, PTEXT parameters )
{
	PVARTEXT pvt = VarTextCreate();
	INDEX idx;
	int first = 1;
	CTEXTSTR name;
	LIST_FORALL( global.ExtensionNames, idx, CTEXTSTR, name )
	{
		vtprintf(  pvt, "%s%s"
				  , first?"":", "
				  , name );
		first = 0;
	}
	if( first )
		vtprintf( pvt, "none." );
	else
		vtprintf( pvt, "." );
	{
		PTEXT out = VarTextGet( pvt );
		EnqueLink( &ps->Command->Output, out );
	}
	VarTextDestroy( &pvt );
	return 1;
}

//--------------------------------------------------------------------------

static PDATAPATH OnInitDevice( "object", "Datapath designed to accept input from other objects" )
//		 iObjectDevice = RegisterDevice( "object", "Datapath designed to accept input from other objects", OpenObject );
//PDATAPATH OpenObject
( PDATAPATH *pChannel, PSENTIENT ps, PTEXT params )
{
	PDATAPATH pdp = CreateDataPath( pChannel, DATAPATH );
	pdp->Type = iObjectDevice; 
	return pdp;
}

//--------------------------------------------------------------------------
int SendToObjectEx( PSENTIENT ps, PTEXT parameters, int bInput )
{
	PTEXT pName, pMsg;
	PENTITY pe;
	pe = (PENTITY)FindThing( ps, &parameters, ps->Current, FIND_VISIBLE, NULL );
	if( !pe )
	{
		pName = GetParam( ps, &parameters );
		if( !pName )
		{
			DECLTEXT( msg, "must specify a name of an entity to send data to..." );
			EnqueLink( &ps->Command->Output, &msg );
			return 0;
		}
		if( pName == ps->Current->pName )
		{
			pe = ps->Current;
		}
		else
		{
			DECLTEXT( msg, "Could not see entity to send data to..." );
			EnqueLink( &ps->Command->Output, &msg );
			return 0;
		}
	}
	if( !pe->pControlledBy )
	{
		DECLTEXT( msg, "Entity is not aware and cannot process data." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;
	}
	if( !pe->pControlledBy->Data )
	{
		DECLTEXT( msg, "Entity to send to has not opened its datapath." );
		EnqueLink( &ps->Command->Output, &msg );
		return 0;

	}
	pMsg = MacroDuplicateEx( ps, parameters, FALSE, TRUE );

	// these enqueues to input SHOULD be to the internal device....
		  // NOT the first device...
	if( bInput )
		EnqueLink( &pe->pControlledBy->Data->Input, pMsg );
	else
	{
		lprintf( "Enqueying output on %s", GetText( GetName( pe ) ) );
		EnqueLink( &pe->pControlledBy->Data->Output, pMsg );
	}

	return 1;
}

//--------------------------------------------------------------------------
int CPROC SendToObject( PSENTIENT ps, PTEXT parameters )
{
	return SendToObjectEx( ps, parameters, TRUE );
}
//--------------------------------------------------------------------------
int CPROC WriteToObject( PSENTIENT ps, PTEXT parameters )
{
	return SendToObjectEx( ps, parameters, FALSE );
}

//--------------------------------------------------------------------------

// choice
//	1> make a behavior struct { CTEXTSTR name }

PMACROSTATE InvokeBehavior( CTEXTSTR name, PENTITY peActor, PSENTIENT psInvokeOn, PTEXT parameters )
{
	INDEX idx;
	PTEXT testname;
	PLIST *ppBehaviors;
	PMACROSTATE pms = NULL;
	if( !psInvokeOn )
		return NULL;
	LIST_FORALL( global.behaviors, idx, PTEXT, testname )
	{
		if( !StrCaseCmp( name, GetText( testname ) ) )
		{
			ppBehaviors = &psInvokeOn->Current->pGlobalBehaviors;
			break;
		}
	}
	if( !testname )
	{
		LIST_FORALL( psInvokeOn->Current->behaviors, idx, PTEXT, testname )
		{
			if( !StrCaseCmp( name, GetText( testname ) ) )
			{
				ppBehaviors = &psInvokeOn->Current->pBehaviors;
				break;
			}
		}
	}
	if( testname )
	{
		PMACRO macro = (PMACRO)GetLink( ppBehaviors, idx );
		if( macro )
		{
			//if( peActor->pControlledBy )
			//	psInvokeOn->pToldBy = peActor->pControlledBy;
			//ele
			psInvokeOn->pInactedBy = peActor;
			pms = InvokeMacro( psInvokeOn, macro, parameters );
			WakeAThread( psInvokeOn );
		}
	}
	return pms;
}

void AddBehavior( PENTITY pe, CTEXTSTR name, CTEXTSTR description )
{
	AddLink( &pe->behaviors
			 , SegAppend( SegCreateFromText( name )
							, SegCreateFromText( description ) ) );
}

void AddCommonBehavior( CTEXTSTR name, CTEXTSTR description )
{
	AddLink( &global.behaviors
			 , SegAppend( SegCreateFromText( name )
							, SegCreateFromText( description ) ) );
}


//$Log: plugins.c,v $
//Revision 1.59  2005/08/08 15:26:59  d3x0r
//Define protect filenames... also uses extended loadfunction which loads plugins with private namespaces... otherwise silly things like Read() overlap
//
//Revision 1.58  2005/08/08 09:30:24  d3x0r
//Fixed listing and handling of ON behaviors.  Protected against some common null spots.  Fixed databath destruction a bit....
//
//Revision 1.57  2005/05/30 11:51:42  d3x0r
//Remove many messages...
//
//Revision 1.56  2005/04/25 07:56:55  d3x0r
//Fix lingering data on NULL path datapaths.  Also, return find_self as a result of searching... there is only one object that will match %me... and that object, although not really 'grabbable' or 'visilbe' is both.
//
//Revision 1.55  2005/04/20 06:20:25  d3x0r
//
//Revision 1.54  2005/03/02 19:13:59  d3x0r
//Add local behaviors to correct list.
//
//Revision 1.53  2005/02/26 23:26:33  d3x0r
//Show behaviors which have been defined on the /list command.  Show list of available behaviors on /on with no parameters
//
//Revision 1.52  2005/02/24 03:10:51  d3x0r
//Fix behaviors to work better... now need to register terminal close and a couple other behaviors...
//
//Revision 1.51  2005/02/23 11:38:59  d3x0r
//Modifications/improvements to get MSVC to build.
//
//Revision 1.50  2005/02/21 12:08:57  d3x0r
//Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
//Revision 1.49  2005/01/17 09:01:31  d3x0r
//checkpoint ...
//
//Revision 1.48  2005/01/10 09:20:09  d3x0r
//Protect against developer state method table with NULL enthries.
//
//Revision 1.47  2004/09/27 16:06:56  d3x0r
//Checkpoint - all seems well.
//
//Revision 1.46  2004/09/21 00:27:48  d3x0r
//Fix plugins proc def... now we will blow up..
//
//Revision 1.45  2004/05/06 08:10:04  d3x0r
//Cleaned all warning from core code...
//
//Revision 1.44  2004/05/04 19:37:56  d3x0r
//Destroy behaviors on destruction, checking for memory loss...
//
//Revision 1.43  2004/05/04 07:29:50  d3x0r
//Massive reformatting.  Fixed looping, removed some gotos. Implement the relay for TF_RELAY segments.
//
//Revision 1.42  2004/04/06 01:50:31  d3x0r
//Update to standardize device options and the processing thereof.
//
//Revision 1.41  2004/04/05 22:57:15  d3x0r
//Revert plugin loading to standard library
//
//Revision 1.40  2003/12/10 21:59:51  panther
//Remove all LIST_ENDFORALL
//
//Revision 1.39  2003/11/07 23:45:28  panther
//Port to new vartext abstraction
//
//Revision 1.38  2003/08/20 08:05:49  panther
//Fix for watcom build, file usage, stuff...
//
//Revision 1.37  2003/08/15 13:19:12  panther
//Device subsystem much more robust?
//
//Revision 1.36  2003/08/01 02:36:18  panther
//Updates for watcom...
//
//Revision 1.35  2003/07/28 08:45:16  panther
//Cleanup exports, and use cproc type for threadding
//
//Revision 1.34  2003/04/20 23:06:30  panther
//Okay multi seg wraps fixed
//
//Revision 1.33  2003/04/02 06:43:27  panther
//Continued development on ANSI position handling.  PSICON receptor.
//
//Revision 1.32  2003/03/24 12:42:21  panther
//Very nearly working psicon now.
//
//Revision 1.31  2003/03/24 04:18:10  panther
//cleanup registered objects more properly
//
//Revision 1.30  2003/01/30 04:09:16  panther
//Minor fixes regarding destruction of objects/volatile variables.  Also UDP/ansi/file
//
//Revision 1.29  2003/01/29 06:59:45  panther
//Remove createobject list in plugins, empty vartext in bigbang
//
//Revision 1.28  2003/01/29 05:56:29  panther
//Fixed memory leakage - volatile variables
//
//Revision 1.27  2003/01/28 16:39:20  panther
//Fixed bug with multiple objects simultaneously ready to process
//
//Revision 1.26  2003/01/27 09:09:52  panther
//Updates to projects, minor fixes across the board?
//
//Revision 1.25  2003/01/22 11:09:50  panther
//Cleaned up warnings issued by Visual Studio
//
//Revision 1.24  2003/01/21 11:20:24  panther
//cleaned out some logging messages
//
//Revision 1.23  2003/01/20 12:52:02  panther
//Cleanup exit on linux - use common exit routine
//
//Revision 1.22  2003/01/13 00:37:50  panther
//Added visual studio projects, removed old msvc projects.
//Minor macro mods
//Mods to compile cleanly under visual studio.
//
//Revision 1.21  2002/10/09 13:14:57  panther
//Changes and tweaks - show volatile variables now...
//cards nearly evaluates all hands - straight is still iffy.
//Fixed support for macros to have descriptions.
//
//Revision 1.20  2002/10/07 08:29:58  panther
//Makefile changes across the board.
//
//Revision 1.19  2002/09/27 07:19:58  panther
//Unsure mods... macro updates.
//
//Revision 1.18  2002/09/16 02:24:14  panther
//Updated wincon support for closing as an unregister operation.  Removed
//(FINEALLY!) the double deallocate...
//
//Revision 1.17  2002/09/16 01:13:57  panther
//Removed the ppInput/ppOutput things - handle device closes on
//plugin unload.  Attempting to make sure that we can restore to a
//clean state on exit.... this lets us track lost memory...
//
//Revision 1.16  2002/09/09 02:58:13  panther
//Conversion to new make system - includes explicit coding of exports.
//
//Revision 1.15  2002/09/09 01:28:10  panther
//Cleaned up logging options, cleaned device ownership, line duplication.
//
//Revision 1.14  2002/09/09 00:19:58  panther
//Modified makefile system - use Makefile instead of makefile.cm1.
//Any code mods were no differntial.
//
//Revision 1.13  2002/07/14 04:18:28  panther
//Changes to how METHODS are handled
//additional ability for volitile_variable types (result of a function call
//is the value of the variable - therefore not static possibly)
//Unification of method_entry and command_entry since these are static
//things.
//
//Revision 1.12  2002/07/08 02:40:33  panther
//Added /option command.
//Begin to modify /script to use more standard components
//(/command file, /command token, /option file close )
//
//Revision 1.11  2002/04/25 18:34:19  panther
//Added check for file called "plugins.list" to specify which plugins should
//be loaded.  If this file is absent, normal scan directory method is done.
//
//Revision 1.10  2002/04/15 18:23:57  panther
//Well - autoversion almost works... but not quite
//
