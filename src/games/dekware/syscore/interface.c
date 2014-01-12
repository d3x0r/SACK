
#include <stdhdrs.h>
#include <space.h>
#include <interface.h>

extern int b95;

#define f(...)
#undef CORE_PROC_PTR
#define CORE_PROC_PTR(a,b)  b f

struct dekware_interface RealDekwareInterface =
{
	&b95,
	CORE_PROC_PTR( PTEXT, GetName )( void *pe ),
	CORE_PROC_PTR( PTEXT, GetDescription )( void *pe ),
	CORE_PROC_PTR( void, SetDescription )( void *pe, PTEXT desc ),
	CORE_PROC_PTR( PTEXT, GetParam )( PSENTIENT pEntity, PTEXT *from ),
	CORE_PROC_PTR( PTEXT, GetFileName )( PSENTIENT ps, PTEXT *parameters ),
	CORE_PROC_PTR( PTEXT, GetVariable )( PSENTIENT ps, CTEXTSTR text ),
   CORE_PROC_PTR( PTEXT, MacroDuplicateExx )( BLOBTYPE sentient_tag *pEntity, PTEXT pText, int bKeepEOL, int bSubst, PTEXT args DBG_PASS),
	CORE_PROC_PTR( POINTER, FindThingEx )( PSENTIENT ps, PTEXT *params, PENTITY Around, int type, int *foundtype
												, PTEXT *pObject, PTEXT *pResult DBG_PASS ),
	CORE_PROC_PTR( PTEXT, SubstTokenEx )( PSENTIENT ps, PTEXT *token, int IsVar, int IsLen, PENTITY pe ),


   CORE_PROC_PTR( PTEXT, MakeNumberText )( size_t val ),
	CORE_PROC_PTR( PDATAPATH, CreateDataPath   )( PDATAPATH *ppWhere, int nExtra ),

	CORE_PROC_PTR( PMACROSTATE, InvokeBehavior )( CTEXTSTR name, PENTITY peActor, PSENTIENT psInvokeOn, PTEXT parameters ),
	CORE_PROC_PTR( void, AddCommonBehavior )( CTEXTSTR name, CTEXTSTR description ),
	CORE_PROC_PTR( void, AddBehavior )( PENTITY pe, CTEXTSTR name, CTEXTSTR desc ),

	CORE_PROC_PTR( PSENTIENT, CreateAwareness  )( PENTITY pEntity ),
	CORE_PROC_PTR( PENTITY, CreateEntityIn     )( PENTITY Location, PTEXT pName ),
	CORE_PROC_PTR( PENTITY, Duplicate          )( PENTITY object ),
	CORE_PROC_PTR( PSHADOW_OBJECT, CreateShadowIn     )( PENTITY pContainer, PENTITY pe ),
	CORE_PROC_PTR( void   , DestroyEntityEx    )( PENTITY pe DBG_PASS ),
	CORE_PROC_PTR( PDATAPATH, DestroyDataPathEx)( PDATAPATH pdp DBG_PASS),
   CORE_PROC_PTR( void, SetDatapathType )( PDATAPATH pdp, int nType ),


	CORE_PROC_PTR( void, WakeAThreadEx )( PSENTIENT ps DBG_PASS ),

	CORE_PROC_PTR( PMACRO, GetMacro )( PENTITY pe, CTEXTSTR pNamed ),
	CORE_PROC_PTR( void, QueueCommand )( PSENTIENT ps, CTEXTSTR Command ),
	CORE_PROC_PTR( int, 		RelayInput			)( PDATAPATH pdp, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) ),
	CORE_PROC_PTR( int, 		RelayOutput			)( PDATAPATH pdp, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) ),
	CORE_PROC_PTR( PTEXT, GatherLineEx )( PTEXT *pOutput, INDEX *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput ),
	CORE_PROC_PTR( PMACRO, LocateMacro )( PENTITY pe, CTEXTSTR name ),
	CORE_PROC_PTR( PMACROSTATE, InvokeMacro )( BLOBTYPE sentient_tag *ps, PMACRO pMacro, PTEXT pArgs ),
	CORE_PROC_PTR( void, UnlockAwareness       )( PSENTIENT ps ),
   CORE_PROC_PTR( INDEX, RegisterExtension    )( CTEXTSTR pName ),
	CORE_PROC_PTR( void,  RegisterObjectEx       )( CTEXTSTR pName, CTEXTSTR pDescription, ObjectInit Init DBG_PASS ),

	CORE_PROC_PTR( void, RegisterCommands )(CTEXTSTR device, command_entry *cmds, INDEX nCommands),
	CORE_PROC_PTR( void, RegisterOptions )(CTEXTSTR device, option_entry *cmds, INDEX nCommands),


	CORE_PROC_PTR( void,  UnregisterRoutine    )( CTEXTSTR pName ),
	CORE_PROC_PTR( void,  UnregisterObject     )( CTEXTSTR pName ),

	CORE_PROC_PTR( void,  RegisterRoutine      )( CTEXTSTR *pClassname, CTEXTSTR pName, CTEXTSTR pDescription, RoutineAddress Routine ),
	CORE_PROC_PTR( int,   RegisterDevice       )( CTEXTSTR pNext, CTEXTSTR pDescription, DeviceOpenDevice Open ),
	CORE_PROC_PTR( int,   RegisterDeviceOpts       )( CTEXTSTR pNext, CTEXTSTR pDescription, DeviceOpenDevice Open, option_entry *pOptions, _32 nOptions ),

	CORE_PROC_PTR( void, AddVariableExxx )( PSENTIENT ps, PENTITY pe
													  , PTEXT pName, PTEXT parameters
													  , int bBinary, int bForceEnt, int bNoSubst
														DBG_PASS ),
	CORE_PROC_PTR( PTEXT, GetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed ),
	CORE_PROC_PTR( PTEXT, SetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed, PTEXT newval ),

	CORE_PROC_PTR( void,  UnregisterDevice     )( CTEXTSTR pName ),
	CORE_PROC_PTR( void, WriteCommandList2 )( PLINKQUEUE *Output, CTEXTSTR root
														 , PTEXT pMatch ),
	CORE_PROC_PTR( void, WriteOptionList )( PLINKQUEUE *Output, option_entry *commands
													  , INDEX nCommands
													  , PTEXT pMatch ),

   CORE_PROC_PTR( Function, GetRoutineRegistered )( TEXTSTR prefix, PTEXT Command ),

	CORE_PROC_PTR( PDATAPATH, FindOpenDevice )( PSENTIENT ps, PTEXT pName ),
	CORE_PROC_PTR( PDATAPATH, FindDataDatapath )( PSENTIENT ps, int type ),
	CORE_PROC_PTR( PDATAPATH, FindCommandDatapath)( PSENTIENT ps, int type ),
	CORE_PROC_PTR( PDATAPATH, FindDatapath     )( PSENTIENT ps, int type ),

	CORE_PROC_PTR( PCOMMAND_INFO, CreateCommandHistoryEx )( PromptCallback prompt ),
	CORE_PROC_PTR( void, RecallCommand )( PCOMMAND_INFO pci, int bUp ),
	//CORE_PROC_PTR( void, EnqueCommandHistory )( PCOMMAND_INFO pci, PTEXT pHistory ),
	CORE_PROC_PTR( int, SetCommandPosition )( PCOMMAND_INFO pci, int nPos, int whence ),
	CORE_PROC_PTR( void, SetCommandInsert )( PCOMMAND_INFO pci, int bInsert ),
	CORE_PROC_PTR( PTEXT, GatherCommand )( PCOMMAND_INFO pci, PTEXT stroke ),

	CORE_PROC_PTR( void, prompt )( BLOBTYPE sentient_tag *ps ),


	CORE_PROC_PTR( int, Assimilate )( PENTITY pe, PSENTIENT ps, CTEXTSTR object_type, PTEXT parameters ),

	CORE_PROC_PTR( void, S_MSG )( PSENTIENT ps, CTEXTSTR msg, ... ),
	CORE_PROC_PTR( void, D_MSG )( PDATAPATH pd, CTEXTSTR msg, ... ),
	CORE_PROC_PTR( void, Q_MSG )( PLINKQUEUE *po, CTEXTSTR msg, ... ),
	CORE_PROC_PTR( PENTITY, FindContainerEx )( PENTITY source
														  , PENTITY *mount_point ),

	CORE_PROC_PTR( int, CreateRegisteredObject )( PSENTIENT ps, PTEXT parameters ),
	GetTheVoid,
	ExitNexus,
   DoCommandf,
   DestroyAwarenessEx,
};


POINTER CPROC LoadDekwareInterface( void )
{
	return (POINTER)&RealDekwareInterface;
}

void CPROC UnloadDekwareInterface( POINTER p )
{
}

// have to regsiter this before pscards; which may have to be loaded at same time
PRIORITY_PRELOAD( RegisterDekwareInterface, DEFAULT_PRELOAD_PRIORITY-4 )
{
	RegisterInterface( WIDE("Dekware"), LoadDekwareInterface, UnloadDekwareInterface );
}
