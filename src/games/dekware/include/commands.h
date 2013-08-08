#ifndef COMMANDS_DEFINED
#define COMMANDS_DEFINED

#include <sack_types.h>
#include <procreg.h>
//#include "links.h"  // PDATA
#include "text.h"

#define byOutput Buffer.data.data
#define nOutput &Buffer.data.size
#define DECLOUTBUF(size) DECLTEXTSZ( Buffer, (size) )

#include "space.h"

typedef int (CPROC *Function)( PSENTIENT ps, PTEXT parameters );
typedef int (CPROC FunctionProto)( PSENTIENT ps, PTEXT parameters );

typedef PTEXT (CPROC *GetVariableFunc)( PENTITY pe
                                 , PTEXT *lastvalue );
typedef PTEXT (CPROC GetVariableProto)( PENTITY pe
                                 , PTEXT *lastvalue );
typedef PTEXT (CPROC *SetVariableFunc)( PENTITY pe
                                 , PTEXT newvalue );
typedef PTEXT (CPROC SetVariableProto)( PENTITY pe
                                 , PTEXT newvalue );

FunctionProto  UNIMPLEMENTED;

typedef int (CPROC *RoutineAddress)( PSENTIENT ps, PTEXT params );

typedef struct command_entry
{
   DECLTEXTSZTYPE(name, 32);
   S_8 significant; // minimum match
   S_8 maxlen;      // maximum usable characters
   DECLTEXTSZTYPE(description, 128);
   Function function;
   TEXTCHAR *funcname;
//   DECLTEXTSZ(extended_help, 256);
}command_entry;

#define HandleCommand( name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),HandleExtendedCommand,WIDE("commands"),name,desc,int,(PSENTIENT,PTEXT),__LINE__)

#define OnCreateObject( name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),HandleCreateObject,WIDE("objects"),name,desc,int,(PSENTIENT,PENTITY,PTEXT),__LINE__)

#define ObjectMethod( object, name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),HandleObjectMethod,WIDE("objects/")object WIDE("/methods"),name,desc,int,(PSENTIENT,PTEXT),__LINE__)

#define DeviceMethod( object, name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),HandleObjectMethod,WIDE("devices/")object WIDE("/methods"),name,desc,int,(PSENTIENT,PTEXT),__LINE__)

#define ObjectVolatileVariableGet( object, name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),ObjectVolatileVariableGet,WIDE("objects/")object WIDE("/variables/")name,WIDE("get"),desc,PTEXT,(PENTITY,PTEXT*),__LINE__)

#define DeviceVolatileVariableGet( device, name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),DeviceVolatileVariableGet,WIDE("devices/")device WIDE("/variables/")name,WIDE("get"),desc,PTEXT,(PENTITY,PTEXT*),__LINE__)

#define ObjectVolatileVariableSet( object, name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),ObjectVolatileVariableSet,WIDE("objects/")object WIDE("/variables/")name,WIDE("set"),desc,PTEXT,(PENTITY,PTEXT),__LINE__)

#define DeviceVolatileVariableSet( device, name, desc )   \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),DeviceVolatileVariableSet,WIDE("devices/")devic WIDE("/variables/")name,WIDE("set"),desc,PTEXT,(PENTITY,PTEXT),__LINE__)

#define OnInitDevice( device, desc ) \
	__DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, WIDE("Dekware"),HandleObjectMethod,WIDE("devices"),device,desc,PDATAPATH,(PDATAPATH*,PSENTIENT,PTEXT),__LINE__)

typedef struct volatile_variable_entry
{
	// this needs to be much bigger for sql volatile vars...
   // need to change this to a PTEXT someday
	//DECLTEXTSZTYPE(name, 64);
   CTEXTSTR pName;
   GetVariableFunc get;
   SetVariableFunc set;
   PTEXT pLastValue;
   //PTRSZVAL psv; // this is the first param passed to set/get
}volatile_variable_entry;

#include "plugin.h"

// -------------------- Exported Commands.... these were previosuly only script reachable
CORE_CPROC( int, DefineOnBehavior )( PSENTIENT ps, PTEXT parameters );



int Process_Command(PSENTIENT  pEntity, PTEXT *Command);

#define AddMacro(l,m) AddLink(l,m)

#define CreateMacroList() (PMACROLIST)CreateList()

S_32 CountArguments( PSENTIENT ps, PTEXT args );
CORE_PROC( PMACRO, CreateMacro )( PENTITY pEnt, PLINKQUEUE *ppOutput, PTEXT name );
CORE_CPROC( void, AddMacroCommand )( PMACRO pMacro, PTEXT pText );
CORE_PROC( void, DestroyMacro )( PENTITY pe, PMACRO pm );
CORE_PROC( PMACRO, DuplicateMacro )( PMACRO pm );

#ifdef CORE_SOURCE
CORE_CPROC( PTEXT, GetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed );
CORE_CPROC( PTEXT, SetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed, PTEXT newval );
// is now an external - heading towards more generic usage...
// need to break out test for default variable names....
// also might be good to have the % and # characters definable via
// a command.

CORE_PROC( PTEXT, GetEntityParam )( PENTITY pEntity, PTEXT *from );
CORE_CPROC( PTEXT, GetParam )( PSENTIENT pEntity, PTEXT *from );
//void LackingParam(PSENTIENT ps, INDEX command);
CORE_CPROC( PTEXT, GetFileName )( PSENTIENT ps, PTEXT *parameters );


CORE_PROC( void, AddVariableExxx )( PSENTIENT ps, PENTITY pe /*FOLD00*/
											 , PTEXT pName, PTEXT parameters
											 , int bBinary, int bForceEnt, int bNoSubst
											  DBG_PASS );
CORE_PROC( PTEXT, SubstTokenEx )( PSENTIENT ps, PTEXT *token, int IsVar, int IsLen, PENTITY pe );

//void AddBinary( PSENTIENT ps, PENTITY pe, PTEXT pName, PTEXT binary );
// lookup a variable, returns volatile variable, and local entity variables
CORE_CPROC( PTEXT, GetVariable )( PSENTIENT ps, CTEXTSTR text );

// if you need to duplicate a macro which is not within
// your object but in someone else
// you CAN - set ps->Current to the new object (and restore it when done!)
CORE_PROC( PTEXT, MacroDuplicateExx )( BLOBTYPE sentient_tag *pEntity, PTEXT pText, int bKeepEOL, int bSubst, PTEXT args DBG_PASS);
CORE_PROC( PMACRO, LocateMacro )( PENTITY pe, TEXTCHAR *name );
CORE_PROC( PMACROSTATE, InvokeMacro )( BLOBTYPE sentient_tag *ps, PMACRO pMacro, PTEXT pArgs );
void EnqueCommandProcess( PTEXT pName, PLINKQUEUE *ppOutput, PTEXT pCommand );
 /*FOLD00*/
//-------------- PLUGINS Support -----------------------
CORE_PROC(void, WriteCommandList )( PLINKQUEUE *Output, command_entry *commands /*FOLD00*/
                         , INDEX nCommands, PTEXT pMatch );
CORE_CPROC( void, WriteCommandList2 )( PLINKQUEUE *Output, CTEXTSTR root
                         , PTEXT pMatch );
CORE_CPROC( void, WriteOptionList )( PLINKQUEUE *Output, option_entry *commands
                         , INDEX nCommands
                         , PTEXT pMatch );
void HandleExternalDefaults( PSENTIENT ps, PTEXT Command );

CORE_PROC( struct datapath_tag *, OpenDevice )( struct datapath_tag **pChannel /*FOLD00*/
                               , BLOBTYPE sentient_tag *ps
                               , PTEXT pName, PTEXT parameters );

void PrintRegisteredDevices( PLINKQUEUE *ppOutput );
PTEXT FindDeviceName( int Type );

// returns a text segment containing the list specified.
//   if output is passed, and is non-null - the segment is
//   enqueued into output, and not returned, otherwise it is returned (appended to leader)
PTEXT WriteListNot( PLIST pSource
                  , PENTITY pNot
                  , PTEXT pLeader
                  , PLINKQUEUE *Output );

CORE_PROC( void, prompt )( BLOBTYPE sentient_tag *ps );
//--------------- Text Value Converstions ----------------
//CORE_PROC( double, FltNumber )( PTEXT pText );
//CORE_PROC( S_64, IntNumber )( PTEXT pText );
CORE_PROC( PTEXT, MakeNumberText )( size_t val );

CORE_PROC( PMACRO, GetMacro )( PENTITY pe, TEXTCHAR *pNamed );
CORE_PROC( void, QueueCommand )( PSENTIENT ps, TEXTCHAR *Command );
#endif
#define SubstToken(ps,tok,var,len) SubstTokenEx(ps,tok,var,len,NULL)
#define AddVariableExx(ps,pe,pName,params, bBinary,forcent) AddVariableExxx((ps), (pe), (pName), (params), (bBinary), (forcent), FALSE DBG_SRC )
#define AddVariableEx(ps,pe,pName,params, bBinary)          AddVariableExxx((ps), (pe), (pName), (params), (bBinary), FALSE    , FALSE DBG_SRC )
#define AddVariable( ps, pe, pName, params )                AddVariableExxx((ps), (pe), (pName), (params), FALSE    , FALSE    , FALSE DBG_SRC )

#define AddBinary( ps, pe, pName, params )                  AddVariableEx( (ps), (pe), (pName), (params), TRUE  DBG_SRC)
#define MacroDuplicateEx(e,t,eol,subst) MacroDuplicateExx( e,t,eol,subst,NULL DBG_SRC )
#define MacroDuplicate(e,t) MacroDuplicateEx(e,t,FALSE,TRUE)
#define WriteList(s, leader, Output) WriteListNot( s, NULL, leader, Output )
#define IsNumber(p) IsSegAnyNumberEx( &(p), NULL, NULL, NULL, 0 )
// if IsNumber returns true, text token is updated
#define IsNumber(p) IsSegAnyNumberEx( &(p), NULL, NULL, NULL, 0 )
//CORE_PROC( int, IsNumber )( PTEXT *pText, double *dNumber, S_64 *iNumber, int *bIntNumber );
//CORE_PROC( int, IsIntNumber )( PTEXT *pText, S_64 *iNumber );
#define IntNumber IntCreateFromSeg
#define FltNumber FloatCreateFromSeg
//----------------------------------------------------------------------


#endif
