#ifndef BRAIN_DEFINED
#define BRAIN_DEFINED

#include <sack_types.h>
#include <idle.h>
#include <configscript.h>
//#include <controls.h>
#include <pssql.h>
#include <timers.h>
#include <sharemem.h>

typedef void *PBRAIN_INPUT;
typedef void *PBRAIN_OUTPUT;

#include "braintypes.h"
#include "neuron.h"

//#include "3dlib.h"
   // max nerons fills a board of around 128x128

#define MAX_NEURONS   1024 // then start radio connection
#define MAX_SYNAPSES  4*MAX_NEURONS // 2 spots per neuron consumed

//#define MAX_INPUTS 32
//#define MAX_OUTPUTS 32

#define MAX_NAME_LEN 256 // can be a whole path of names...
#pragma pack(1)

#ifdef __cplusplus
typedef class BRAIN_STEM *PBRAIN_STEM;
#endif

typedef class connector *PCONNECTOR, CONNECTOR;
class connector:public value{
public:
   TEXTCHAR c_name[MAX_NAME_LEN]; // always make this last
	IMPORT CTEXTSTR name(void);
	IMPORT connector()
#ifdef BRAIN_SOURCE
   :value()
	{ strcpy( c_name, WIDE("unnamed") ); }
#else
   ;
#endif
	connector(CTEXTSTR conn_name, value *pvalue )
		:value(pvalue)
	{ strcpy( c_name, conn_name ); }
	INDEX idx;
	PBRAIN_STEM pbs;
	IMPORT INDEX Save( PODBC odbc, CTEXTSTR name_prefix, INDEX iParent, int bInput )
#ifdef BRAIN_SOURCE
	{
		TEXTCHAR name[MAX_NAME_LEN + 256];
		snprintf( name, sizeof( name ), WIDE("%s/%s"), name_prefix, c_name );
		SQLInsert( odbc
						, WIDE("brain_connectors") 
						, WIDE("connector_name"),1,name
						, WIDE("parent_id"),2,iParent
						, WIDE("input"),2,bInput
						, NULL, 0, NULL);
						return FetchLastInsertID(odbc,NULL,NULL); }
	//IMPORT INDEX Load( PODBC odbc, INDEX iParent ) { 
		//CTEXTSTR *results;
		//DoSQLRecordQueryf( NULL, &results, NULL
		//	, WIDE("select connector_name,input from brain_connectors where parent_id=%d"), iParent );
		// uhmm how do I recreate myself from here?
		// it's like all the save/load code is skewed... I vaguely remember something like that...
	//};
#else
   ;
#endif
//#ifdef BRAIN_SOURCE
#define CONNECTOR_CONSTRUCTOR( conn_type, type_name ) 	connector( CTEXTSTR conn_name, conn_type *type_name ):value(type_name) { strcpy( connector::c_name, conn_name ); }
//#else
//#define CONNECTOR_CONSTRUCTOR( conn_type, type_name ) 	IMPORT connector( CTEXTSTR conn_name, conn_type *type_name );
//#endif
	CONNECTOR_CONSTRUCTOR( float           ,   pf );
	CONNECTOR_CONSTRUCTOR( double          ,   pd );
	CONNECTOR_CONSTRUCTOR( char            ,   pc );
	CONNECTOR_CONSTRUCTOR( short           ,   ps );
	CONNECTOR_CONSTRUCTOR( long            ,   pl );
	CONNECTOR_CONSTRUCTOR( S_64            ,  pll );
	CONNECTOR_CONSTRUCTOR( unsigned char   ,  puc );
	CONNECTOR_CONSTRUCTOR( unsigned short  ,  pus );
	CONNECTOR_CONSTRUCTOR( unsigned long   ,  pul );
	CONNECTOR_CONSTRUCTOR( _64             , pull );
	CONNECTOR_CONSTRUCTOR( bool            , pb );
};
#pragma pack()

#ifdef __cplusplus
class BRAIN_STEM // allocated by the body.
{
private:
   // when cycle updates, set outputs to 0.
	_32 nCycle;
	iList Inputs; // list of PCONNECTORs
	iList Outputs;  // list of PCONNECTORs
	iList Modules;  // brainstems provide a good way to glob things...
	int nGroups;
	PBRAIN_STEM *group; // array of connector groups... 
	PBRAIN_STEM parent; // is a module within another stem.

	TEXTCHAR FullName[MAX_NAME_LEN];  // omittable?
	TEXTSTR Name;
	int nComponent;  // shrug - some sort of index number from brainboard
public:
	IMPORT BRAIN_STEM( CTEXTSTR name );
	IMPORT BRAIN_STEM( CTEXTSTR name, connector **inputs, int nInputs, connector **outputs, int nOutputs );
	IMPORT BRAIN_STEM();
	IMPORT ~BRAIN_STEM();
	//_BRAIN_STEM();

	IMPORT void AddInput( PANYVALUE pv, CTEXTSTR name );
	IMPORT void AddOutput( PANYVALUE pv, CTEXTSTR name );
	IMPORT void AddModule( PBRAIN_STEM );
	IMPORT void AddInput( connector * );
	IMPORT void AddOutput( connector * );
	IMPORT connector *first_input( void );
	IMPORT connector *first_output( void );
	IMPORT BRAIN_STEM *first_module( void );
	IMPORT connector *next_input( void );
	IMPORT connector *next_output( void );
	IMPORT BRAIN_STEM *next_module( void );
	//IMPORT PCONNECTOR FirstInput( void );
	//IMPORT PCONNECTOR NextInput( void );
	//IMPORT PCONNECTOR FirstOutput( void );
	//IMPORT PCONNECTOR NextOutput( void );
	IMPORT CTEXTSTR fullname();
	IMPORT CTEXTSTR name();
	IMPORT void update( _32 cycle )
#ifdef BRAIN_SOURCE
	{
		if( cycle != nCycle )
		{
			nCycle = cycle;
			PCONNECTOR Output;
			for( Output = (PCONNECTOR)Outputs.first(); Output; Output = (PCONNECTOR)Outputs.next() )
			{
				Output->set( (NATIVE)0 );
			}
		}
	}
#else
	;
#endif
	inline NATIVE get( int idx ) { return ((connector*)Inputs.get(idx))->get(); }
	inline void setget( int idx, NATIVE n ) { ((connector*)Inputs.get(idx))->set(n); }
	inline void set( int idx, NATIVE n ) { ((connector*)Outputs.get(idx))->set(n); }
	inline NATIVE getset( int idx ) { return ((connector*)Outputs.get(idx))->get(); }
	connector *getoutput( int idx ) { return ((connector*)Outputs.get(idx)); }
	connector *getinput( int idx ) { return ((connector*)Outputs.get(idx)); }

   friend class BRAIN;
};
#endif
// definitions for passing to Component...
#define INPUT 0
#define OUTPUT 1


//class BRAIN_INTERFACE {
	// these are the methods availabe for the outside world...
//};
#ifdef __cplusplus
typedef class BRAIN *PBRAIN;
class BRAIN {
public:
   struct {
      int bExit:1;    // set to end brain thread
      int bReleased:1;// set by thread when exited.
      int bHalt:1; // run/stop state for brain
   };
   //long nLock; // count of locks...
   float k; // sigmoid global K for brain...
private:
   _32 dwThreadId;  // perhaps this should be a handle...

   iList BrainStems;  // contained by body.
	PNEURONSET NeuronPool;
	PSYNAPSESET SynapsePool;

   int nCycle;

   int nVersion; // current version of this brain...

   void GatherValues( PNEURON pn ); // get sum of previous nOutputValues
   void ProcessInput( PNEURON pn ); // get value, set first level..
   void DeleteComponents( void );

	CRITICALSECTION cs;
	inline void Lock( void ) { EnterCriticalSec( &cs ); }
	inline void Unlock( void ) { LeaveCriticalSec( &cs ); }
   inline void End( void ) {
      bExit = TRUE;
      while( !bReleased ) // wait for brain to end...
         Idle(); 
	}
public:
	IMPORT void Run( void );
	IMPORT void Stop( void );

   void Process( void );
   IMPORT void Reset( void );
   IMPORT void Init(void);
	IMPORT BRAIN(); // must have a body in a 'known' configuration....
	IMPORT BRAIN( PBRAIN_STEM pbs ); // must have a body in a 'known' configuration....
   IMPORT ~BRAIN();

	IMPORT PBRAIN_STEM first_stem( void );
	IMPORT PBRAIN_STEM next_stem( void );
	IMPORT PNEURON GetNeuron( void );
	IMPORT PNEURON DupNeuron( PNEURON default_neuron );
	IMPORT PNEURON GetInputNeuron( PBRAIN_STEM pbs, int nInput );
	IMPORT PNEURON GetInputNeuron( connector *input );
	IMPORT PNEURON GetOutputNeuron( PBRAIN_STEM pbs, int nOutput );
	IMPORT PNEURON GetOutputNeuron( connector *output );

	IMPORT PSYNAPSE GetSynapse( void );
	IMPORT PSYNAPSE DupSynapse( PSYNAPSE default_synapse );

	IMPORT void ReleaseNeuron( PNEURON pn );
	IMPORT void ReleaseSynapse( PSYNAPSE ps );

   IMPORT int LinkSynapseFrom( PSYNAPSE pSyn, PNEURON pn,  int n );
   IMPORT int LinkSynapseTo( PSYNAPSE pSyn, PNEURON pn,  int n );
   IMPORT int LinkSynapseFrom( PSYNAPSE pSyn, PNEURON pn );
   IMPORT int LinkSynapseTo( PSYNAPSE pSyn, PNEURON pn );
   IMPORT void UnLinkSynapseTo( PSYNAPSE pSyn );
   IMPORT void UnLinkSynapseFrom( PSYNAPSE pSyn );

	IMPORT void AddBrainStem( PBRAIN_STEM pStem );
   IMPORT PBRAIN_STEM AddComponent(int n, CTEXTSTR name, ...);
   IMPORT PBRAIN_STEM AddComponent(int n, CTEXTSTR name, arg_list  args);

   IMPORT LOGICAL Load( PODBC odbc, INDEX idx );
   //IMPORT INDEX Save( PODBC odbc );
   IMPORT INDEX Save( PODBC odbc, CTEXTSTR brainname ); // SQL Save
   //IMPORT _32 Load( FILE *File );
	IMPORT PBRAIN_STEM first( void );
	IMPORT PBRAIN_STEM next( void );
	IMPORT PBRAIN_STEM GetBrainStem( INDEX id_stem );
	IMPORT PBRAIN_STEM GetBrainStem( CTEXTSTR stem_name );
	IMPORT void GetConnector( CTEXTSTR connector, class connector **connector_result, int bInput );
};
#else
typedef void BRAIN;
typedef struct BRAIN *PBRAIN;
#endif
#ifdef INTERFACE_USED
# ifndef BRAIN_POINTERS_DEFINED
//typedef class BRAIN *PBRAIN;
# endif
# include <msgprotocol.h> // INTERFACE_METHOD definition
typedef struct brain_interface {
# define BRAIN_PROC INTERFACE_METHOD
#else
# ifdef BRAIN_SOURCE
#  define BRAIN_PROC(type,name) EXPORT_METHOD type CPROC name
# else
#  define BRAIN_PROC(type,name) IMPORT_METHOD type CPROC name
# endif
#endif
	BRAIN_PROC( PBRAIN, CreateBrain )(void); // must have a body in a 'known' configuration....
	BRAIN_PROC( void, DestroyBrain )(PBRAIN); // must have a body in a 'known' configuration....
   BRAIN_PROC( void, Reset )( PBRAIN );

	BRAIN_PROC( PNEURON, GetNeuron )( PBRAIN );
	BRAIN_PROC( PNEURON, DupNeuron )( PBRAIN,PNEURON default_neuron );
	BRAIN_PROC( PNEURON, GetInputNeuron )( PBRAIN,PBRAIN_STEM pbs, int nInput );
	BRAIN_PROC( PNEURON, GetOutputNeuron )( PBRAIN,PBRAIN_STEM pbs, int nOutput );
	//BRAIN_PROC( PNEURON, GetOutputNeuron )( connector *nOutput );
   BRAIN_PROC( PSYNAPSE, GetSynapse )( PBRAIN );
   BRAIN_PROC( PSYNAPSE, DupSynapse )( PBRAIN, PSYNAPSE default_synapse );
	BRAIN_PROC( void, ReleaseNeuron )( PBRAIN,PNEURON pn );
	BRAIN_PROC( void, ReleaseSynapse )( PBRAIN,PSYNAPSE ps );

   BRAIN_PROC( int, LinkSynapseFromPos )( PBRAIN,PSYNAPSE pSyn, PNEURON pn,  int n );
   BRAIN_PROC( int, LinkSynapseToPos )( PBRAIN,PSYNAPSE pSyn, PNEURON pn,  int n );
   BRAIN_PROC( int, LinkSynapseFrom )( PBRAIN,PSYNAPSE pSyn, PNEURON pn );
   BRAIN_PROC( int, LinkSynapseTo )( PBRAIN,PSYNAPSE pSyn, PNEURON pn );
   BRAIN_PROC( void, UnLinkSynapseTo )( PBRAIN,PSYNAPSE pSyn );
   BRAIN_PROC( void, UnLinkSynapseFrom )( PBRAIN,PSYNAPSE pSyn );

	BRAIN_PROC( void, AddBrainStem )( PBRAIN,PBRAIN_STEM pStem );
   BRAIN_PROC( PBRAIN_STEM, AddComponent )(PBRAIN,int n, CTEXTSTR name, arg_list args );
	//BRAIN_PROC( PBRAIN_STEM, AddComponent )(int n, CTEXTSTR name, va_list args );

   //BRAIN_PROC( _32, Save )( PBRAIN,HANDLE hFile );
   //BRAIN_PROC( _32, Load )( PBRAIN,HANDLE hFile );
	BRAIN_PROC( PBRAIN_STEM, first )( PBRAIN );
	BRAIN_PROC( PBRAIN_STEM, next )( PBRAIN );
	// once used in menus, the index is knowable... 
#ifdef INTERFACE_USED
} BRAIN_INTERFACE, *PBRAIN_INTERFACE;

#include <msgprotocol.h>

#endif


#endif
