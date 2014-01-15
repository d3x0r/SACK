#ifndef PLUGIN_DEFINITION
#define PLUGIN_DEFINITION
// plugin export table defined here...

// include standard headers...
#include <stdhdrs.h>
#include "sharemem.h"

// include nexus core header
#include "space.h" // space.h, and all private headers...

#include "my_ver.h"

#include "commands.h"
#include "input.h"

typedef struct registered_device_tag 
{   
	PTEXT name;   
	PTEXT description;   
	PDATAPATH (CPROC *Open)( PDATAPATH *ppChannel, PSENTIENT ps, PTEXT params );
	option_entry *pOptions;
	INDEX nOptions;
	PLIST pOpenPaths;   
	int   TypeID;   
	struct registered_device_tag *pNext;
} DEVICE, *PDEVICE;

typedef PDATAPATH (CPROC *DeviceOpenDevice)( PDATAPATH *ppChannel, PSENTIENT ps, PTEXT params );


#ifndef PLUGIN_LOADER
CORE_PROC( DeviceOpenDevice, FindDevice )( PTEXT pName );
#endif
CORE_CPROC( PDATAPATH, CreateDataPath   )( PDATAPATH *ppWhere, int nExtra );
#define CreateDataPath(where, pathname) (P##pathname)CreateDataPath( where, sizeof(pathname) - sizeof( DATAPATH ) )
CORE_CPROC( PDATAPATH, DestroyDataPathEx)( PDATAPATH pdp DBG_PASS);
#define DestroyDataPath(pdp) DestroyDataPathEx( (pdp) DBG_SRC )
CORE_CPROC( void, SetDatapathType )( PDATAPATH pdp, int nType );

void LoadPlugin( CTEXTSTR pFile, PSENTIENT ps, PTEXT parameters );
// callback for ScanFiles; calls LoadPlugin with ps=NULL and parameters=NULL
// called from scanfiles in syscmds too right now...
void CPROC LoadAPlugin( PTRSZVAL psv, CTEXTSTR name, int flags );
void Unload    ( PTEXT pCommandName );
void DumpLoadedPluginList( PSENTIENT ps );

// uses one paramter, and assimlates as an object
CORE_PROC( int, Assimilate )( PENTITY pe, PSENTIENT ps, CTEXTSTR object_type, PTEXT parameters );

CORE_PROC( int, CloseDevice			)( PDATAPATH pdp );
CORE_PROC( void, AddMethodEx )( PENTITY pe, command_entry *pce DBG_PASS );
CORE_PROC( void, AddMethodsEx )( PENTITY pe, PCLASSROOT root DBG_PASS );
#define AddMethod(pe,pce) AddMethodEx( pe,pce DBG_SRC )
#define AddMethods(pe,pce) AddMethodsEx( pe,pce DBG_SRC )
//CORE_PROC( void, AddMethod				)( PENTITY pe, command_entry *pce );
CORE_PROC( void, RemoveMethod      	)( PENTITY pe, command_entry *pce );
//CORE_PROC( void, AddVolatileVariable)( PENTITY pe, volatile_variable_entry *pve );
//CORE_PROC( void, RemoveVolatileVariable)( PENTITY pe, volatile_variable_entry *pve );

CORE_CPROC( void, RegisterRoutine  )( TEXTCHAR *pName, TEXTCHAR *pDescription, RoutineAddress Routine );
CORE_CPROC( void, RegisterCommands )(CTEXTSTR device, command_entry *cmds, INDEX nCommands);
CORE_CPROC( void, RegisterOptions )(CTEXTSTR device, option_entry *cmds, INDEX nCommands);
CORE_PROC( Function, GetRoutineRegistered )( TEXTSTR prefix, PTEXT Command );
CORE_PROC( OptionHandler, GetOptionRegistered )( TEXTSTR prefix, PTEXT Command );

CORE_CPROC( void,  UnregisterRoutine    )( TEXTCHAR *pName );
CORE_PROC( void,  PrintRegisteredRoutines)( PLINKQUEUE *Output, PSENTIENT ps, PTEXT parameters );
CORE_PROC( int,   RoutineRegistered)( PSENTIENT ps, PTEXT Command );
CORE_CPROC( int,   RegisterDeviceOpts       )( TEXTCHAR *pNext, TEXTCHAR *pDescription, DeviceOpenDevice Open, option_entry *pOptions, _32 nOptions );
CORE_CPROC( int,   RegisterDevice       )( TEXTCHAR *pNext, TEXTCHAR *pDescription, DeviceOpenDevice Open );
CORE_CPROC( void,  UnregisterDevice     )( TEXTCHAR *pName );
CORE_PROC( INDEX, RegisterExtension    )( CTEXTSTR pName );
typedef int (CPROC *ObjectInit)( PSENTIENT ps, PENTITY pe, PTEXT parameters );
CORE_PROC( void,  RegisterObjectEx       )( CTEXTSTR pName, CTEXTSTR pDescription, ObjectInit Init DBG_PASS );
#define RegisterObject( name,desc,init)  RegisterObjectEx(name,desc,init DBG_SRC)
CORE_CPROC( void,  UnregisterObject     )( TEXTCHAR *pName );
CORE_CPROC( int, CreateRegisteredObject )( PSENTIENT ps, PTEXT parameters );
CORE_CPROC( int, IsObjectTypeOf        )( PSENTIENT ps, PTEXT entity, PTEXT form );


CORE_CPROC( PDATAPATH, FindOpenDevice )( PSENTIENT ps, PTEXT pName );
CORE_CPROC( PDATAPATH, FindDataDatapath )( PSENTIENT ps, int type );
CORE_CPROC( PDATAPATH, FindCommandDatapath)( PSENTIENT ps, int type );
CORE_PROC( PDATAPATH, FindDatapath     )( PSENTIENT ps, int type );
// results (in theory) in the number of things enqueued to this pdp's input.
CORE_PROC( int, 		RelayInput			)( PDATAPATH pdp, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) );
// results (in theory) in the number of things enqueued to this pdp's output.
CORE_PROC( int, 		RelayOutput			)( PDATAPATH pdp, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) );

//CORE_PROC( INDEX,  FindDeviceID        )( PTEXT pName );

// sentient to invoke the macro upon
CORE_CPROC( PMACROSTATE, InvokeBehavior )( TEXTCHAR *name, PENTITY peActor, PSENTIENT psInvokeOn, PTEXT parameters );

CORE_CPROC( void, AddCommonBehavior )( TEXTCHAR *name, TEXTCHAR *description );

// a archtype object might define behaviors specific to is object...
// if many of its objects are expected to be created it might use
// the global AddCommonBehavior.....
CORE_PROC( void, AddBehavior )( PENTITY pe, TEXTCHAR *name, TEXTCHAR *desc );

#include "interface.h"

#endif
// $Log: plugin.h,v $
// Revision 1.21  2005/08/08 09:32:50  d3x0r
// Set data path type with a method...Added used reference to macros for quick duplication...
//
// Revision 1.20  2005/02/23 11:38:59  d3x0r
// Modifications/improvements to get MSVC to build.
//
// Revision 1.19  2005/02/21 12:08:42  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.18  2004/08/12 16:47:30  d3x0r
// Export createregisteredobject call
//
// Revision 1.17  2004/04/06 01:50:27  d3x0r
// Update to standardize device options and the processing thereof.
//
// Revision 1.16  2003/10/26 12:38:16  panther
// Updates for newest scheduler
//
// Revision 1.15  2003/08/15 13:16:15  panther
// Add name to datapath - since it's not always a real device
//
// Revision 1.14  2003/04/02 06:43:27  panther
// Continued development on ANSI position handling.  PSICON receptor.
//
// Revision 1.13  2003/03/25 08:59:02  panther
// Added CVS logging
//
