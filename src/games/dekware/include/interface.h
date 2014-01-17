#ifndef DEKWARE_EXPORT
#define DEKWARE_EXPORT

#define CORE_PROC_PTR(type,name)  type (CPROC* name)


struct dekware_interface {
   int *b95;
	CORE_PROC_PTR( PTEXT, GetName )( void *pe );
	CORE_PROC_PTR( PTEXT, GetDescription )( void *pe );
	CORE_PROC_PTR( void, SetDescription )( void *pe, PTEXT desc );
	CORE_PROC_PTR( PTEXT, GetParam )( PSENTIENT pEntity, PTEXT *from );
	CORE_PROC_PTR( PTEXT, GetFileName )( PSENTIENT ps, PTEXT *parameters );
	CORE_PROC_PTR( PTEXT, GetVariable )( PSENTIENT ps, CTEXTSTR text );
   CORE_PROC_PTR( PTEXT, MacroDuplicateExx )( BLOBTYPE sentient_tag *pEntity, PTEXT pText, int bKeepEOL, int bSubst, PTEXT args DBG_PASS);
	CORE_PROC_PTR( POINTER, FindThingEx )( PSENTIENT ps, PTEXT *params, PENTITY Around, int type, int *foundtype
												, PTEXT *pObject, PTEXT *pResult DBG_PASS );
	CORE_PROC_PTR( PTEXT, SubstTokenEx )( PSENTIENT ps, PTEXT *token, int IsVar, int IsLen, PENTITY pe );


   CORE_PROC_PTR( PTEXT, MakeNumberText )(size_t val );
	CORE_PROC_PTR( PDATAPATH, CreateDataPath   )( PDATAPATH *ppWhere, int nExtra );

	CORE_PROC_PTR( PMACROSTATE, InvokeBehavior )( TEXTCHAR *name, PENTITY peActor, PSENTIENT psInvokeOn, PTEXT parameters );
	CORE_PROC_PTR( void, AddCommonBehavior )( TEXTCHAR *name, TEXTCHAR *description );
	CORE_PROC_PTR( void, AddBehavior )( PENTITY pe, TEXTCHAR *name, TEXTCHAR *desc );

	CORE_PROC_PTR( PSENTIENT, CreateAwareness  )( PENTITY pEntity );
	CORE_PROC_PTR( PENTITY, CreateEntityIn     )( PENTITY Location, PTEXT pName );
	CORE_PROC_PTR( PENTITY, Duplicate          )( PENTITY object );
	CORE_PROC_PTR( PSHADOW_OBJECT, CreateShadowIn     )( PENTITY pContainer, PENTITY pe );
	CORE_PROC_PTR( void   , DestroyEntityEx    )( PENTITY pe DBG_PASS );
	CORE_PROC_PTR( PDATAPATH, DestroyDataPathEx)( PDATAPATH pdp DBG_PASS);
   CORE_PROC_PTR( void, SetDatapathType )( PDATAPATH pdp, int nType );


	CORE_PROC_PTR( void, WakeAThreadEx )( PSENTIENT ps DBG_PASS );

	CORE_PROC_PTR( PMACRO, GetMacro )( PENTITY pe, TEXTCHAR *pNamed );
	CORE_PROC_PTR( void, QueueCommand )( PSENTIENT ps, TEXTCHAR *Command );
	CORE_PROC_PTR( int, 		RelayInput			)( PDATAPATH pdp, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) );
	CORE_PROC_PTR( int, 		RelayOutput			)( PDATAPATH pdp, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) );
	CORE_PROC_PTR( PTEXT, GatherLineEx )( PTEXT *pOutput, INDEX *pIndex, int bInsert, int bSaveCR, int bData, PTEXT pInput );
	CORE_PROC_PTR( PMACRO, LocateMacro )( PENTITY pe, TEXTCHAR *name );
	CORE_PROC_PTR( PMACROSTATE, InvokeMacro )( BLOBTYPE sentient_tag *ps, PMACRO pMacro, PTEXT pArgs );
	CORE_PROC_PTR( void, UnlockAwareness       )( PSENTIENT ps );
   CORE_PROC_PTR( INDEX, RegisterExtension    )( TEXTCHAR *pName );
	CORE_PROC_PTR( void,  RegisterObjectEx       )( TEXTCHAR *pName, TEXTCHAR *pDescription, ObjectInit Init DBG_PASS );

	CORE_PROC_PTR( void, RegisterCommands )(CTEXTSTR device, command_entry *cmds, INDEX nCommands);
	CORE_PROC_PTR( void, RegisterOptions )(CTEXTSTR device, option_entry *cmds, INDEX nCommands);


	CORE_PROC_PTR( void,  UnregisterRoutine    )( TEXTCHAR *pName );
	CORE_PROC_PTR( void,  UnregisterObject     )( TEXTCHAR *pName );

	CORE_PROC_PTR( void,  RegisterRoutine      )( TEXTCHAR *pClassname, TEXTCHAR *pName, TEXTCHAR *pDescription, RoutineAddress Routine );
	CORE_PROC_PTR( int,   RegisterDevice       )( TEXTCHAR *pNext, TEXTCHAR *pDescription, DeviceOpenDevice Open );
	CORE_PROC_PTR( int,   RegisterDeviceOpts       )( TEXTCHAR *pNext, TEXTCHAR *pDescription, DeviceOpenDevice Open, option_entry *pOptions, _32 nOptions );

	CORE_PROC_PTR( void, AddVariableExxx )( PSENTIENT ps, PENTITY pe
													  , PTEXT pName, PTEXT parameters
													  , int bBinary, int bForceEnt, int bNoSubst
														DBG_PASS );
	CORE_PROC_PTR( PTEXT, GetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed );
	CORE_PROC_PTR( PTEXT, SetVolatileVariable )( PENTITY pEnt, CTEXTSTR pNamed, PTEXT newval );

	CORE_PROC_PTR( void,  UnregisterDevice     )( TEXTCHAR *pName );
	CORE_PROC_PTR( void, WriteCommandList2 )( PLINKQUEUE *Output, CTEXTSTR root
														 , PTEXT pMatch );
	CORE_PROC_PTR( void, WriteOptionList )( PLINKQUEUE *Output, option_entry *commands
													  , INDEX nCommands
													  , PTEXT pMatch );

   CORE_PROC_PTR( Function, GetRoutineRegistered )( TEXTSTR prefix, PTEXT Command );

	CORE_PROC_PTR( PDATAPATH, FindOpenDevice )( PSENTIENT ps, PTEXT pName );
	CORE_PROC_PTR( PDATAPATH, FindDataDatapath )( PSENTIENT ps, int type );
	CORE_PROC_PTR( PDATAPATH, FindCommandDatapath)( PSENTIENT ps, int type );
	CORE_PROC_PTR( PDATAPATH, FindDatapath     )( PSENTIENT ps, int type );

	CORE_PROC_PTR( PCOMMAND_INFO, CreateCommandHistoryEx )( PromptCallback prompt );
	CORE_PROC_PTR( void, RecallCommand )( PCOMMAND_INFO pci, int bUp );
	//CORE_PROC_PTR( void, EnqueCommandHistory )( PCOMMAND_INFO pci, PTEXT pHistory );
	CORE_PROC_PTR( int, SetCommandPosition )( PCOMMAND_INFO pci, int nPos, int whence );
	CORE_PROC_PTR( void, SetCommandInsert )( PCOMMAND_INFO pci, int bInsert );
	CORE_PROC_PTR( PTEXT, GatherCommand )( PCOMMAND_INFO pci, PTEXT stroke );

	CORE_PROC_PTR( void, prompt )( BLOBTYPE sentient_tag *ps );


	CORE_PROC_PTR( int, Assimilate )( PENTITY pe, PSENTIENT ps, CTEXTSTR object_type, PTEXT parameters );

	CORE_PROC_PTR( void, S_MSG )( PSENTIENT ps, CTEXTSTR msg, ... );
	CORE_PROC_PTR( void, D_MSG )( PDATAPATH pd, CTEXTSTR msg, ... );
	CORE_PROC_PTR( void, Q_MSG )( PLINKQUEUE *po, CTEXTSTR msg, ... );
	CORE_PROC_PTR( PENTITY, FindContainerEx )( PENTITY source
														  , PENTITY *mount_point );

	CORE_PROC_PTR( int, CreateRegisteredObject )( PSENTIENT ps, PTEXT parameters );
	CORE_PROC_PTR( PENTITY, GetTheVoid         )( void );
	CORE_PROC_PTR( void, ExitNexus )( void );

	CORE_PROC_PTR( void, DoCommandf )( PSENTIENT ps, CTEXTSTR f, ... );
	CORE_PROC_PTR( int, DestroyAwarenessEx      )( PSENTIENT ps DBG_PASS );
	CORE_PROC_PTR( ObjectInit, ScanRegisteredObjects )( PENTITY pe, CTEXTSTR for_name );
	CORE_PROC_PTR( PMACROSTATE, InvokeMacroEx )( PSENTIENT ps, PMACRO pMacro, PTEXT pArgs, void (CPROC*StopEvent)(PTRSZVAL psvUser, PMACROSTATE pms ), PTRSZVAL psv );

};


#ifndef NO_DEKWARE_INTERFACE
#  ifndef DEFINES_DEKWARE_INTERFACE
extern 
#  endif
	 struct dekware_interface *DekwareInterface
#ifdef __GCC__
	 __attribute__((visibility("hidden")))
#endif
;

#  ifdef DEFINES_DEKWARE_INTERFACE
// this needs to be done before most modules can run their PRELOADS...so just move this one.
// somehow this ended up as 69 and 69 was also PRELOAD() priority... bad.
PRIORITY_PRELOAD( InitDekwareInterface, DEFAULT_PRELOAD_PRIORITY - 3)
{
	DekwareInterface = (struct dekware_interface*)GetInterface( WIDE("dekware") );
}

#  endif

#endif


#ifndef CORE_SOURCE
#if 0
#define a                                ( !DekwareInterface )?NULL:DekwareInterface->a
#define a                                if( DekwareInterface ) DekwareInterface->a
#endif
#define CreateRegisteredObject                                ( !DekwareInterface )?0:DekwareInterface->CreateRegisteredObject
#define DestroyAwarenessEx                                ( !DekwareInterface )?0:DekwareInterface->DestroyAwarenessEx
#define S_MSG                                if( DekwareInterface ) DekwareInterface->S_MSG
#define FindContainerEx                                ( !DekwareInterface )?NULL:DekwareInterface->FindContainerEx
#define D_MSG                                if( DekwareInterface ) DekwareInterface->D_MSG
#define Q_MSG                                if( DekwareInterface ) DekwareInterface->Q_MSG
#define Assimilate                                ( !DekwareInterface )?0:DekwareInterface->Assimilate
#define CreateCommandHistoryEx                                ( !DekwareInterface )?NULL:DekwareInterface->CreateCommandHistoryEx
#define CreateEntityIn                                ( !DekwareInterface )?NULL:DekwareInterface->CreateEntityIn
#define prompt                                if( DekwareInterface ) DekwareInterface->prompt
#define GetName                                ( !DekwareInterface )?NULL:DekwareInterface->GetName
#define GetDescription                                ( !DekwareInterface )?NULL:DekwareInterface->GetDescription
#define SetDescription                                if( DekwareInterface ) DekwareInterface->SetDescription
#define GatherCommand                                ( !DekwareInterface )?NULL:DekwareInterface->GatherCommand
#define SetCommandInsert                                if( DekwareInterface ) DekwareInterface->SetCommandInsert
#define SetCommandPosition                                ( !DekwareInterface )?0:DekwareInterface->SetCommandPosition
#define RecallCommand                                if( DekwareInterface ) DekwareInterface->RecallCommand
#define SubstTokenEx                                 ( !DekwareInterface )?NULL:DekwareInterface->SubstTokenEx 
#define FindThingEx                                ( !DekwareInterface )?NULL:DekwareInterface->FindThingEx
#define GetVolatileVariable                                ( !DekwareInterface )?NULL:DekwareInterface->GetVolatileVariable
#define SetVolatileVariable                                ( !DekwareInterface )?NULL:DekwareInterface->SetVolatileVariable
#define MacroDuplicateExx                                ( !DekwareInterface )?NULL:DekwareInterface->MacroDuplicateExx
#define RegisterObjectEx                                if( DekwareInterface ) DekwareInterface->RegisterObjectEx
#define FindOpenDevice                   ( !DekwareInterface )?NULL:DekwareInterface->FindOpenDevice
#define FindDataDatapath                 ( !DekwareInterface )?NULL:DekwareInterface->FindDataDatapath
#define FindCommandDatapath            ( !DekwareInterface )?NULL:DekwareInterface->FindCommandDatapath
#define FindDatapath                     (( !DekwareInterface )?NULL:DekwareInterface->FindDatapath)
#define RegisterDevice                                ( !DekwareInterface )?0:DekwareInterface->RegisterDevice
#define RegisterDeviceOpts                                ( !DekwareInterface )?0:DekwareInterface->RegisterDeviceOpts
#define RegisterOptions                                if( DekwareInterface ) DekwareInterface->RegisterOptions
#define RegisterCommands                                if( DekwareInterface ) DekwareInterface->RegisterCommands
#define GetRoutineRegistered                                ( !DekwareInterface )?NULL:DekwareInterface->GetRoutineRegistered
#define WriteCommandList2                                if( DekwareInterface ) DekwareInterface->WriteCommandList2
#define WriteOptionList                                if( DekwareInterface ) DekwareInterface->WriteOptionList

#define UnregisterDevice                                if( DekwareInterface ) DekwareInterface->UnregisterDevice
#define UnregisterObject                                if( DekwareInterface ) DekwareInterface->UnregisterObject
#define AddVariableExxx                                if( DekwareInterface ) DekwareInterface->AddVariableExxx
#define UnregisterRoutine                                if( DekwareInterface ) DekwareInterface->UnregisterRoutine
#define RegisterRoutine                                if( DekwareInterface ) DekwareInterface->RegisterRoutine
#define RegisterExtension                                ( !DekwareInterface )?0:DekwareInterface->RegisterExtension
#define UnlockAwareness                                if( DekwareInterface ) DekwareInterface->UnlockAwareness
#define SetDatapathType                                if( DekwareInterface ) DekwareInterface->SetDatapathType
#define LocateMacro                                ( !DekwareInterface )?NULL:DekwareInterface->LocateMacro
#define InvokeMacroEx                                ( !DekwareInterface )?NULL:DekwareInterface->InvokeMacroEx
#define InvokeMacro(ps,pm,arg) InvokeMacroEx( ps,pm,arg,NULL,0 )
#define GatherLineEx                                ( !DekwareInterface )?NULL:DekwareInterface->GatherLineEx

#define RelayOutput                                ( !DekwareInterface )?0:DekwareInterface->RelayOutput
#define RelayInput                                ( !DekwareInterface )?0:DekwareInterface->RelayInput
#define ExitNexus                                if( DekwareInterface ) DekwareInterface->ExitNexus

#define WakeAThreadEx                                if( DekwareInterface ) DekwareInterface->WakeAThreadEx
#define GetMacro                                ( !DekwareInterface )?NULL:DekwareInterface->GetMacro
#define QueueCommand                                if( DekwareInterface ) DekwareInterface->QueueCommand
#define DestroyDataPathEx                                ( !DekwareInterface )?NULL:DekwareInterface->DestroyDataPathEx
#define Duplicate                                ( !DekwareInterface )?NULL:DekwareInterface->Duplicate
#define CreateShadowIn                                ( !DekwareInterface )?NULL:DekwareInterface->CreateShadowIn
#define DestroyEntityEx                                if( DekwareInterface ) DekwareInterface->DestroyEntityEx


#define CreateAwareness                                ( !DekwareInterface )?NULL:DekwareInterface->CreateAwareness
#define CreateEntity                                ( !DekwareInterface )?NULL:DekwareInterface->CreateEntity
#define ScanRegisteredObjects                       ( !DekwareInterface )?NULL:DekwareInterface->ScanRegisteredObjects

#define AddBehavior                                if( DekwareInterface ) DekwareInterface->AddBehavior
#define AddCommonBehavior                                ( !DekwareInterface )?NULL:DekwareInterface->AddCommonBehavior
#define InvokeBehavior                                ( !DekwareInterface )?NULL:DekwareInterface->InvokeBehavior
#define GetParam                                ( !DekwareInterface )?NULL:DekwareInterface->GetParam
#define DoCommandf                                if( DekwareInterface ) DekwareInterface->DoCommandf
#define GetVariable                                ( !DekwareInterface )?NULL:DekwareInterface->GetVariable
#define GetFileName                                ( !DekwareInterface )?NULL:DekwareInterface->GetFileName
#undef CreateDataPath
#define CreateDataPath(where, pathname)            (P##pathname)(( !DekwareInterface )?NULL:DekwareInterface->CreateDataPath(where,sizeof(pathname) - sizeof( DATAPATH ) ))

#define MakeNumberText                          ( !DekwareInterface )?NULL:DekwareInterface->MakeNumberText
#define b95                                (!DekwareInterface)?1:(*DekwareInterface->b95)
#define GetTheVoid()                         ((!DekwareInterface)?(PENTITY)NULL:(DekwareInterface->GetTheVoid()))
#endif
#endif
