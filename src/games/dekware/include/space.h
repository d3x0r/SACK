#ifndef NEXUS_CORE_SPACE_DEFINITIONS
#define NEXUS_CORE_SPACE_DEFINITIONS


#include <sack_types.h>
#include <timers.h>
//#include "links.h"
#include "text.h"

#ifndef __cplusplus
typedef LOGICAL bool;
#else
#define bool LOGICAL
#endif

#ifdef CORE_SOURCE
#define CORE_PROC(type,name) EXPORT_METHOD type CPROC name
#define CORE_CPROC(type,name) EXPORT_METHOD type CPROC name
#else
#define CORE_PROC(type,name) IMPORT_METHOD type CPROC name
#define CORE_CPROC(type,name) IMPORT_METHOD type CPROC name
#endif

#ifndef BLOBTYPE
#define BLOBTYPE struct
// used to reference the type directly.. struct <sname> ...;
#define BLOBREF  struct
#endif
typedef BLOBTYPE sentient_tag *PSENTIENT;
typedef BLOBTYPE entity_tag *PENTITY;
typedef BLOBTYPE macro_state_tag *PMACROSTATE;
typedef BLOBTYPE shadow_tag *PSHADOW_OBJECT;
typedef BLOBTYPE structure_tag *PSTRUCTURE;
typedef BLOBTYPE datapath_tag *PDATAPATH;
typedef BLOBTYPE macro_tag *PMACRO;

// MUST INCLUDE COMMANDS FIRST...
// this defines entity flags... which merge macro/entity distinction

// heck I thought this should be part of space.h... but it's just all such a 
// mess right now... has been always in fact but well...

//#define 

BLOBTYPE macro_flags {
	bool bDelete;  // when used is cleared, delete if delete set
	bool bUsed;    // macro is running, must set delete and leave.
	bool bRunLock; // provided only for trigger.nex...
};

typedef BLOBTYPE entity_flag_tag {
	bool bShadow;  // this is a shadow entity reference...
	bool bMacro; // is a macro reference.... *frown*
	bool bStructure; // structure reference...
	bool bText;
	bool bDestroy; // destroyed/destroying.  allow certain failures during destruction.
	union {
		BLOBREF macro_flags macro;
	}un;
} ENTITY_FLAGS;

typedef BLOBTYPE macro_tag
{
	ENTITY_FLAGS flags; // must be first for structure type designation
   // struct macro_tag **ppReference; // pointer to the entry referencing? 
   // PENTITY pWithin; // to know what to remove...
   // name must be first - to be LIKE and entity...
   PTEXT pName;     // additional name to reference this macro by.
   PTEXT pDescription; // text description - listed in HELP
   S_32  nArgs;  // count of required arguments... if negative %... present
   PTEXT pArgs;  // names of paremters...
   PLIST pCommands; // list of PTEXT lines to process 
   _32   nCommands;
   _32 Used; // number of times this macro is referenced... duplicate becomes cheap.
   //struct macro_tag *pPriorRecording; // layered macro recording info...
} MACRO;

typedef struct foreach_state_tag
{
	struct {
		_32 bTopLevelStep : 1; // else follows indirects to genuine tokens
	} flags;
	INDEX nForEachTopMacroCmd;
	// list of entities (or tokens)
	// entities will be indirect TF_BINARY|TF_ENTITY segment
	// entities will be indirect TF_BINARY|TF_SENTIENTS segment
	// or else is text tokens
	//   different token types may potentially step differently...
	// foreach phrase ... foreach token ...
	PTEXT list_next, list_prior; // prior list, next list after and before item...
	PTEXT item; // will step through tokens in list at a top-only fashion
	PTEXT after;
	PTEXT variablename; // pointer to a segment which is the name of the variable to set at start and at /step
} FOREACH_STATE, *PFOREACH_STATE;

BLOBTYPE macro_state_flag_data {
           int  levels; // counter for /IF searching (nested)
            PTEXT pLabel; // what lebel to find (for goto)
            _32 delay_end;

};

BLOBTYPE macro_state_state_flags {
	bool bSuccess; 
	bool bFindElse;
	bool bFindEndIf;
	bool bRecord; // set if a macro is begun within a macro.
	bool bFindLabel;
	bool bTrace; // print out macros as they execute...
	bool bInputWait; // /wait command issued...
	bool macro_delay; // /DELAY command... delay_end is next resume time.
	bool forced_run; // will run preempting natural command entry.
	BLOBREF macro_state_flag_data data;
	bool macro_suspend; // lock flag to prevent macro processing...
};
BLOBTYPE macro_state_state{
	BLOBREF macro_state_state_flags flags;
};

typedef BLOBTYPE macro_state_tag
{
	PMACRO pMacro;     // Macro to run
	PTEXT pArgs;       // current arguments for macro
	PLIST pVars;       // variables which this macro has. runtime state.
	INDEX nCommand;    // current command index (within macro)
	BLOBREF macro_state_state state;
	//union state_tag{        // current machine state...
	//};
	//   _32 dwFlags; 
	//}state;
	PSENTIENT pInvokedBy; 
	// this routine must PopData from ps->MacroStack
	// if it is used.... otherwise default processing removes 
   // the data from the macrostack...
	void (*MacroEnd)( PSENTIENT ps// sentient processing macro
	               , PMACROSTATE pMacState ); // saves extra peek...
	void *Data; // used by macroEnd to find the wrapping structure....
	PDATASTACK pdsForEachState; // sizeof( FOREACH_STATE )
	PENTITY peInvokedOn; // entity which this was originally invoked on... NULL is self.
	//----------- To Be Implemented
	// struct { INDEX nLabelCmd+1; PTEXT name } Label;
	//   PDATASTACK pdsLabelStack; // as labels are encountered, save this... otherwise we have to runt he macro to get there
	//-----------
	void (CPROC*StopEvent)(PTRSZVAL psvUser, PMACROSTATE pms );
	PTRSZVAL psv_StopEvent;
} MACROSTATE;


typedef BLOBTYPE shadow_tag
{
	ENTITY_FLAGS flags;
	// this does not have the name members
	// a shadow uses the actual name of what it shadows
	// the only information which a shadow has is
	//   1> it's location, it's contents, and unique relationship with all entities
	//   2> a reference of the entity it shadows, which in turn
	//      has a list containing each shadow.
	PENTITY pCreatedBy;
	PENTITY pWithin;    // single entity this one is in.
	PENTITY pShadowOf;

	PLIST pContains;    // list of entities contained within
	PLIST pAttached;    // mutual attachment links peer stuff
	// end comment...
	PENTITY pForm; // actual object...

	// a shadow may have a unique awareness, or lack any awareness
	PSENTIENT pControlledBy;

	// all behaviors, methods, variables, etc are all shared.
	// a unique macro state exists so each macro will have its own
	// local space.

	// plugin references which are passed shadow objects...
	// for instance /make deck cards
	//    /shadow deck shadow_deck
	//    /shadow_deck /dealto bob
	//    /dest deck # hmm well of course the shadow will go away...
	// would seem that shadows will be the same deck
	// if a shadow deck is used at another table, the cards will be the same
	// between the two seemingly different decks.  A shuffle of a shadow and
	// a shuffle of the real will gather from all hands at both tables....
	// the correct solution for this would be to
	//    /duplicate deck another_deck
} SHADOW_OBJECT;


typedef BLOBTYPE structure_tag
{
   ENTITY_FLAGS flags;
   PTEXT pName;        // name data...
	PTEXT pDescription; // description data...
	PLIST pVars;        // variables common to any macro state....
	PENTITY pInstanceOf;

} STRUCTURE;

typedef BLOBTYPE entity_tag
{
#ifdef __cplusplus
public:
#endif
   // this is now what is common - FLAGS only... ???
   ENTITY_FLAGS flags;
   PTEXT pName;        // name data...
	PTEXT pDescription; // description data...
   PENTITY pCreatedBy;
   PENTITY pWithin;    // single entity this one is in.
   PENTITY pShadowOf;
   PLIST pContains;    // list of entities contained within
   PLIST pAttached;    // mutual attachment links peer stuff


	// an entity has a structure which describes its own
   // data set...  a variable may also be a structure.
   //STRUCTURE;
   PLIST pMacros;      // methods this object may perform....
   PLIST pVars;        // variables common to any macro state....

   //--- shadows of this object which exist...
	PLIST pShadows;     // pointers to remove shadows on destruction

   //---- object hide/show keys....
   PLIST pDarkens;     // list of light keys which in certain combinations
                       // may provide visibility.  If this element is emtpy
                       // then it is visible always. multi-keys are linked
                       // lines of keys... a single key is a single entry...
   //---- 
       // to be impelmented - but then the action has to be delayed until the
       // macro associated is completed...
   PLIST behaviors; // local behaviors listed here (TEXTCHAR*)
	PLIST pBehaviors; // set once any single behavior is scripted... local behaviors(PMACRO)
   PLIST pGlobalBehaviors; // (uses global.behaviors...) (PMACRO)
	//---- Plugin extensions
	//PLIST(struct
	//{
		PLIST pPlugin;   // plugin extention... Call RegisterExtension for
		// index(iPluginID) assignment then must always use that index
		// to access the data for the plugin...
		// one and only one pointer per plugin are allowed...
		PLIST pDestroy; // list of void CPROC Destroy( PENTITY pe ) methods...
		PLIST pDuplicate; // list of void CPROC Duplicate( PENTITY peSource, PENTITY peNew )
		//---- Created object accounting....
	//}) plugin;
   PLIST pCreated;     // list of objects which were created by this....
   // what about multiple sentients controlling same object...
   // object actions are split into...
   //    action - performed -> people who can see, prefixed by Object name...
   //                       -> people who are this - prefixed by 'you'
   //                       -> result from target is him/her/it/xo/xi
   //                       -> action->reply/respond with...
   //                       ->   result to those who can see -   subst 'name'
   //  any action I can do has a name...
   //  this name determines 
   //    (source=me) (target=param/me)
   //    send direct text... to viewers, and provide hide keys....
   //    activate mating action in target..
   //    if target does not have a responce action, action text is
   //          displayed to active, aware in vicinity, target if aware.
   //       if target is unaware, and responce action is present in defined
   //       macro methods, active sentience processes with it's time...
   //       otherwise command is enqueed as input path...
   //       immediate mode commands have priority over any current 
   //       macro, and runs outside of the macro's context...

   // now... objects have standard sets of methods, and to conserve space,
   // must all reference the same sources... I guess some 4billion objects
   // may hold the macro, and of course upon deletion delete the macro...
   // if the macro is destroyed and recreated it is for the current context
   // only - any object created from the original must/may uhmm well...

   // any output is sent to all of these...
	PSENTIENT pControlledBy;
   //PLIST(command_entry*)
	PLIST pMethods; // list member is a pointer to an array of command entries... sorted...
   //PLIST(volatile_variable*)
   PLIST pVariables; // list member is a pointer to an array of volatile_variable entries
} ENTITY;

// getname works for PENTITY or PMACRO types
CORE_CPROC( PTEXT, GetName )( void *pe );
CORE_CPROC( PTEXT, GetDescription )( void *pe );
CORE_CPROC( void, SetDescription )( void *pe, PTEXT desc );

#define GetNameSize(e)  GetTextSize( GetName( e ) ) 

#define NameIs(e,s) ( !strcmp( GetText( GetName(e)), s ) )
#define NameLike(e,s) ( !stricmp( GetText( GetName(e)), s ) )



#include "datapath.h"

BLOBTYPE sentient_flags
{
//		struct {
		bool  macro_input;  // macro command wishes next input line...
		//bool  macro_suspend; // lock flag to prevent macro processing...
		bool  resume_run;    // resume issued while not suspended... resume while running
		//bool  macro_delay;
		bool  commanded;  // set when command was ever issued..,
		bool  prompt;  // input flag whether prompt needed...
		bool  command_recording; // command line is recording not macro...
		//bool  input_wait;  // waiting for Data.Input to be available...
		bool  destroy;     // this is scheduled to be deleted
		bool  force_destroy; // locked, AND can delete....
		bool  destroy_object;
		bool  master_operator; // set if can execute raw commands...
		        // otherwiese, must execute macro aliases of base commands 
		        // if provided... those methods in this or the room...
		        // base object which entity was cloned from determines the 
		        // command set then... and location...
		bool  decker; // this entity may create new scripts... 
		              // using only those methods it may access

		             //must associate a command level
		             // to available command list...

		bool  user; // may use room commands in addtion to self commands...
		bool  bot; // this entity may only execute it's own macros...
		          // macros may not be created
		bool  var_subst;
		bool  var_prefix;
		bool  no_prompt; 
		bool bRelay;
		bool bHoldCommands;
		bool scheduled;
		// the pLastTell assocaited with a posted
		// command should be checked to see if it si waiting
		bool waiting_for_someone;
		PTRSZVAL delay_end;   // number of milli seconds to wait....
//	}flags;

};

typedef BLOBTYPE sentient_tag {
#ifdef __cplusplus
public:
#endif
	// no sentience may exist without an entity
	// a body may exist without a sentience
	//
	PENTITY     Current;       /* This is the entity/avatar of this sentience. */

	// NOW: a sentience may never move from one entity to another entity
	// AFTER NOW: if a sentience could change its current entity
	//     should there be a storage of the path it took?
	//     what if that goes away...
	//   sides what would be the harm of becoming something new
	//   all current macros would have to be not runnig
	//   that would imply that the change from one form to antoher
	//    could not be part of the equation.
	//   One could perhaps build entities such that when they lose
	//   their sentience they will self destruct?  what could that
	//   imply?
	//PLINKSTACK  pBecame;    // list/stack of objects player has become

	PMACROSTATE CurrentMacro;
	PTEXT       pLastResult;  // store result from macro here... 
	PDATASTACK  MacroStack; // stack of MACROSTATEs...

	PMACRO      pRecord;        // macro we are recording for this sentient's Entity
	int         nRecord; // levels of recording...
	// this list should be PLIST(PDATAPATH) and its first two
	// entires should be &Command, &Data.  Further data paths may be opened
	//  there has for a time now been problems with having single data paths.
	//  Passing data to other objects on the command channel ... (well it's sorta all
	//     that can/is done now)
	PLIST       pDataPath; // 0 - command
	// 1-n data paths...
	// 1 is the default for current output...
	PDATAPATH   Data;
	PDATAPATH   Command;
//#define S_MSG( ps, msg, ... ) { PVARTEXT pvt; pvt = VarTextCreate(); vtprintf( pvt, msg,##__VA_ARGS__ ); EnqueLink(&(ps)->Command->Output,VarTextGet( pvt )); VarTextDestroy( &pvt ); }
//#define D_MSG( pd, msg, ... ) { PVARTEXT pvt; pvt = VarTextCreate(); vtprintf( pvt, msg,##__VA_ARGS__ ); EnqueLink(&(pd)->Output,VarTextGet( pvt )); VarTextDestroy( &pvt ); }
//#define Q_MSG( po, msg, ... ) { PVARTEXT pvt; pvt = VarTextCreate(); vtprintf( pvt, msg,##__VA_ARGS__ ); EnqueLink(&(po),VarTextGet( pvt )); VarTextDestroy( &pvt ); }
	PENTITY pLastTell; // retain last tell for // tell shortcut...
	// this is parsed from the command received in the input queue.
	// The command handler/dispatcher parses this from the header of
	// the message dequeued... Once the message has been resolved
	// this member is no longer valid.
	PSENTIENT pToldBy;   // used for command level tells
	PENTITY pInactedBy;  // used for behaviors... object which is invoking the macro...
	BLOBREF sentient_flags flags;


	_32  ProcessLock; // Locked from executing.  Unlock sets this...
	_32  ProcessingLock; // currently in use by a process loop...
	_32  StepLock;    // if locked ->Next and/or ->Prior are invalid...
	PSENTIENT Next;
	PSENTIENT Prior;  // next and prior sentients in list...
	// macro variable waiting for input (macrostate?)
	PTEXT MacroInputVar;
	//THREAD_ID LastThreadID; // this is the last thread that tried this and is long sleeping
} SENTIENT;

#include "commands.h"


typedef struct global_vars {
	PENTITY THE_VOID;   /* this is the root of all cookies. */
	PSENTIENT PLAYER;   // first sentient(active) object.
	PSENTIENT AwareEntities; // all sentient entities...
	// this script is written by code
	// it might as well have been implemented in code...
	// but then it might not be as fixable.
	// This script command is really the only 'macro'
	// command which is really based on other commands...
	// all other commands serve their purpose alone...
	PMACRO script;
	PLIST behaviors; // list of behavior names...(CTEXTSTR)
	struct dekware_global_flags
	{
		BIT_FIELD bLogAllCommands : 1;
	} flags;
	INDEX ExtensionCount;
	PLIST ExtensionNames;

} GLOBAL, *PGLOBAL;

#ifndef PLUGIN_MODULE
#ifdef MASTER_CODE
#ifndef __LINUX__
__declspec(dllexport)
#endif
   GLOBAL global;
#else
extern GLOBAL global;
#endif
#else
#ifndef __LINUX__
__declspec(dllimport)
#else
extern
#endif
    GLOBAL global;
#endif

//#ifndef PLUGIN_MODULE
//#define THE_VOID global.THE_VOID
//#define ROOT_PLAYER global.PLAYER
#define AwareEntities global.AwareEntities
//#endif
#ifdef CORE_SOURCE
CORE_CPROC( void, WakeAThreadEx )( PSENTIENT ps DBG_PASS );
#endif
CORE_CPROC( void, TimerWake )( PTRSZVAL ps );

#ifdef CORE_SOURCE
CORE_PROC( void, S_MSG )( PSENTIENT ps, CTEXTSTR msg, ... );
CORE_PROC( void, D_MSG )( PDATAPATH pd, CTEXTSTR msg, ... );
CORE_PROC( void, Q_MSG )( PLINKQUEUE *po, CTEXTSTR msg, ... );

CORE_PROC( PENTITY, GetTheVoid         )( void );
// Location is also marked as the creator of the entity
// if the location is destroyed, the created entity is also
// destroyed.
CORE_CPROC( PENTITY, CreateEntityIn     )( PENTITY Location, PTEXT pName );
CORE_CPROC( PENTITY, Duplicate          )( PENTITY object );
CORE_CPROC( PSHADOW_OBJECT, CreateShadowIn     )( PENTITY pContainer, PENTITY pe );
CORE_CPROC( void   , DestroyEntityEx    )( PENTITY pe DBG_PASS );

PENTITY putin              ( PENTITY that, PENTITY _this );
PENTITY pullout            ( PENTITY that, PENTITY _this );
PENTITY attach             ( PENTITY _this, PENTITY that );
PENTITY detach             ( PENTITY _this, PENTITY that );

CORE_PROC( PENTITY, FindContainer      )( PENTITY source );
PENTITY findbynameEx       ( PLIST object, size_t *count, TEXTCHAR *name );

// ptext is updated according to how many toekns were used to find the entity...
// find thing is wise enough to do parameter substitution,
// it therefore needs to know the state (ps->entity->vars, or current_macro_state->vars)
// and therefore should also work itself much like subst param or get param...
CORE_PROC( POINTER, FindThingEx )( PSENTIENT ps, PTEXT *params, PENTITY Around, int type, int *foundtype
											, PTEXT *pObject, PTEXT *pResult DBG_PASS );
CORE_PROC( POINTER, DoFindThing )( PENTITY Around, int type, int *foundtype, size_t *count, TEXTCHAR *t );
CORE_PROC( PLIST, BuildAttachedListEx )( PENTITY also_ignore, PLIST *ppList, PENTITY source, int max_levels );
CORE_PROC( PENTITY, FindContainerEx )( PENTITY source
												 , PENTITY *mount_point );

//void Trace_objects         ( PENTITY start, int level );
PENTITY showall            ( PLINKQUEUE *Output, PENTITY object );

CORE_PROC( PSENTIENT, CreateAwareness  )( PENTITY pEntity );
// at least one method to make a PSENTIENT do something.
CORE_PROC( void, DoCommandf )( PSENTIENT ps, CTEXTSTR f, ... );

CORE_PROC( int, DestroyAwarenessEx      )( PSENTIENT ps DBG_PASS );
CORE_PROC( void, UnlockAwareness       )( PSENTIENT ps );

void Story( PLINKQUEUE *Output );
CORE_PROC( void, ExitNexus )( void );

void LoadPlugins( CTEXTSTR base );
ObjectInit ScanRegisteredObjects( PENTITY pe, CTEXTSTR for_name );

#endif

enum FindWhere {
   FIND_IN,
   FIND_ON,
   FIND_NEAR,
   FIND_AROUND,
	FIND_VISIBLE,
	// cannot grab my current container...
   // also cannot grab things already grabbed
   FIND_GRABBABLE, 
   FIND_MACRO,
	FIND_MACRO_INDEX
	// this is an in-depth near search
	// for all things attached to all things near.
	  ,FIND_ANYTHING_NEAR
	  // FIND_WITHIN only searches objects which are
	  // immediately linked as contents of the container.
	  // this also searches all attached objects to objects
     // within the container.
	  , FIND_WITHIN
     // the object itself has been found...
     , FIND_SELF
};


#define WakeAThread(ps) WakeAThreadEx( ps DBG_SRC )
#define DestroyEntity(pe)  DestroyEntityEx( pe DBG_SRC )
#define findbyname(o,n)    findbynameEx(o,NULL,n)
#define FindThing(ps,param,around,type,foundtype) FindThingEx((ps),(param),(around),(type),(foundtype),NULL,NULL DBG_SRC )
#define BuildAttachedList( source ) BuildAttachedListEx( source, NULL, source, 0 )
#define FindContainer(source) FindContainerEx(source,NULL)
#define DestroyAwareness(ps) DestroyAwarenessEx( ps DBG_SRC )


#endif
