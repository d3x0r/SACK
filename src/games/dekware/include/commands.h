#ifndef COMMANDS_DEFINED
#define COMMANDS_DEFINED

#include <sack_types.h>
#include <procreg.h>
#include <filesys.h>
//#include "links.h"  // PDATA
#include "text.h"

#define DECLOUTBUF(size) DECLTEXTSZ( Buffer, (size) )

#include "space.h"

typedef int (CPROC *Function)( PSENTIENT ps, PTEXT parameters );
typedef int (CPROC *ObjectFunction)( PSENTIENT ps, PENTITY pe, PTEXT parameters );
typedef void (CPROC *MacroCreateFunction)( PENTITY pe, PMACRO macro ); // event when a new macro is created
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

// this is depricated, use appropriate inline declarators instead...
typedef struct command_entry
{
   DECLTEXTSZTYPE(name, 32);
   DECLTEXTSZTYPE(classname, 25);   
   int8_t significant; // minimum match
   int8_t maxlen;      // maximum usable characters
   DECLTEXTSZTYPE(description, 128);
   Function function;
   TEXTCHAR *funcname;
//   DECLTEXTSZ(extended_help, 256);
}command_entry;


#define ___DefineRegistryMethod3P(priority,task,name,classtype,classname,methodname,desc,returntype,argtypes,line)   \
	CPROC paste(name,line)argtypes;       \
	PRIORITY_PRELOAD( paste(paste(paste(Register,name),Method),line), priority ) {  \
	SimpleRegisterMethod( task "/" classtype, paste(name,line)  \
	, #returntype, methodname, #argtypes ); \
   RegisterValue( task "/" classtype "/" methodname, "Description", desc ); \
   RegisterValue( task "/" classtype "/" methodname, "Command Class", classname ); \
}                                                                          \
	static returntype CPROC paste(name,line)

#define __DefineRegistryMethod3P(priority,task,name,classtype,classname,methodname,desc,returntype,argtypes,line)   \
	___DefineRegistryMethod3P(priority,task,name,classtype,classname,methodname,desc,returntype,argtypes,line)



// static int HandleCommand( "Command Text", "..." )(PSENTIENT ps,PTEXT parameters)
#define HandleCommand( classname, name, desc )   \
	__DefineRegistryMethod3P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleExtendedCommand,"commands",classname,name,desc,int,(PSENTIENT,PTEXT),__LINE__)

// static int OnCreateObject( "object_type", "Friendly description" )(PSENTIENT ps,PENTITY pe_created,PTEXT parameters)
#define OnCreateObject( name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleCreateObject,"objects",name,desc,int,(PSENTIENT,PENTITY,PTEXT),__LINE__)

// static int ObjectMethod( "object_type", "command", "Friendly command description" )(PSENTIENT ps, PENTITY pe_object, PTEXT parameters)
#define ObjectMethod( object, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleObjectMethod,"objects/" object "/methods",name,desc,int,(PSENTIENT,PENTITY,PTEXT),__LINE__)

#define ObjectMacroCreated( object, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleObjectMacroCreate,"objects/" object "/macro/create",name,desc,void,(PENTITY,PMACRO),__LINE__)

#define ObjectMacroDestroyed( object, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleObjectMacroDestroy,"objects/" object "/macro/destroy",name,desc,void,(PENTITY,PMACRO),__LINE__)

#define DeviceMethod( object, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleObjectMethod,"devices/" object "/methods",name,desc,int,(PSENTIENT,PTEXT),__LINE__)

#define ObjectVolatileVariableGet( object, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",ObjectVolatileVariableGet,"objects/" object "/variables/" name,"get",desc,PTEXT,(PENTITY,PTEXT*),__LINE__)

#define DeviceVolatileVariableGet( device, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",DeviceVolatileVariableGet,"devices/" device "/variables/" name,"get",desc,PTEXT,(PENTITY,PTEXT*),__LINE__)

#define ObjectVolatileVariableSet( object, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",ObjectVolatileVariableSet,"objects/" object "/variables/" name,"set",desc,PTEXT,(PENTITY,PTEXT),__LINE__)

#define DeviceVolatileVariableSet( device, name, desc )   \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",DeviceVolatileVariableSet,"devices/" devic "/variables/" name,"set",desc,PTEXT,(PENTITY,PTEXT),__LINE__)

#define OnInitDevice( device, desc ) \
	DefineRegistryMethod2P(DEFAULT_PRELOAD_PRIORITY-5, "Dekware",HandleObjectMethod,"devices",device,desc,PDATAPATH,(PDATAPATH*,PSENTIENT,PTEXT),__LINE__)

typedef struct volatile_variable_entry
{
	// this needs to be much bigger for sql volatile vars...
   // need to change this to a PTEXT someday
	//DECLTEXTSZTYPE(name, 64);
   CTEXTSTR pName;
   GetVariableFunc get;
   SetVariableFunc set;
   PTEXT pLastValue;
   //uintptr_t psv; // this is the first param passed to set/get
}volatile_variable_entry;

#include "plugin.h"

// -------------------- Exported Commands.... these were previosuly only script reachable
CORE_CPROC( int, DefineOnBehavior )( PSENTIENT ps, PTEXT parameters );



int Process_Command(PSENTIENT  pEntity, PTEXT *Command);

#define AddMacro(l,m) AddLink(l,m)

#define CreateMacroList() (PMACROLIST)CreateList()

int32_t CountArguments( PSENTIENT ps, PTEXT args );
CORE_PROC( PMACRO, CreateMacro )( PENTITY pEnt, PLINKQUEUE *ppOutput, PTEXT name );
CORE_CPROC( void, AddMacroCommand )( PMACRO pMacro, PTEXT pText );
CORE_PROC( void, DestroyMacro )( PENTITY pe, PMACRO pm );
CORE_PROC( PMACRO, DuplicateMacro )( PMACRO pm );

#ifdef CORE_SOURCE
void CPROC TerminateMacro( PMACROSTATE pms );
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
CORE_PROC( PMACRO, LocateMacro )( PENTITY pe, CTEXTSTR name );
CORE_PROC( PMACROSTATE, InvokeMacroEx )( PSENTIENT ps, PMACRO pMacro, PTEXT pArgs, void (CPROC*StopEvent)(uintptr_t psvUser, PMACROSTATE pms ), uintptr_t psv );
CORE_PROC( PMACROSTATE, InvokeMacro )( BLOBTYPE sentient_tag *ps, PMACRO pMacro, PTEXT pArgs );
#define InvokeMacro(ps,pm,arg) InvokeMacroEx( ps,pm,arg,NULL,0 )

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

LOGICAL ExtraParse( PTEXT *token );
LOGICAL CommandExtraParse( PTEXT *pReturn );

//PENTITY ResolveEntity( PSENTIENT ps_out, PENTITY focus, enum FindWhere type, PTEXT *varname, PTEXT *tokens );
PENTITY ResolveEntity( PSENTIENT ps_out, PENTITY focus, enum FindWhere type, PTEXT *tokens, LOGICAL bKeepVarName );


CORE_PROC( void, prompt )( BLOBTYPE sentient_tag *ps );
//--------------- Text Value Converstions ----------------
//CORE_PROC( double, FltNumber )( PTEXT pText );
//CORE_PROC( int64_t, IntNumber )( PTEXT pText );
CORE_PROC( PTEXT, MakeNumberText )( size_t val );

CORE_PROC( PMACRO, GetMacro )( PENTITY pe, CTEXTSTR pNamed );
CORE_PROC( void, QueueCommand )( PSENTIENT ps, CTEXTSTR Command );
#endif
#define SubstToken(ps,tok,var,len) SubstTokenEx(ps,tok,var,len,NULL)
#define AddVariableExx(ps,pe,pName,params, bBinary,forcent) AddVariableExxx((ps), (pe), (pName), (params), (bBinary), (forcent), FALSE DBG_SRC )
#define AddVariableEx(ps,pe,pName,params, bBinary)          AddVariableExxx((ps), (pe), (pName), (params), (bBinary), FALSE    , FALSE DBG_SRC )
#define AddVariable( ps, pe, pName, params )                AddVariableExxx((ps), (pe), (pName), (params), FALSE    , FALSE    , FALSE DBG_SRC )

#define AddBinary( ps, pe, pName, params )                  AddVariableEx( (ps), (pe), (pName), (params), TRUE  DBG_SRC)
#define MacroDuplicateEx(e,t,eol,subst) MacroDuplicateExx( e,t,eol,subst,NULL DBG_SRC )
#define MacroDuplicate(e,t) MacroDuplicateEx(e,t,FALSE,TRUE)
#define WriteList(s, leader, Output) WriteListNot( s, NULL, leader, Output )
// if IsNumber returns true, text token is updated
#define IsNumber(p) IsSegAnyNumberEx( &(p), NULL, NULL, NULL, 0 )
//CORE_PROC( int, IsNumber )( PTEXT *pText, double *dNumber, int64_t *iNumber, int *bIntNumber );
//CORE_PROC( int, IsIntNumber )( PTEXT *pText, int64_t *iNumber );
#define IntNumber IntCreateFromSeg
#define FltNumber FloatCreateFromSeg
//----------------------------------------------------------------------


#endif
