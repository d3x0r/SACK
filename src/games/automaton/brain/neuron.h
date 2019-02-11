// this should be privately included by the brain
// any access to these things must be either from the brain
// or from their interface....

#ifndef _NEURON_DEFINED
#define _NEURON_DEFINED
#define INCLUDE_SAMPLE_CPLUSPLUS_WRAPPERS

#include <stdhdrs.h> // handle
#include <sharemem.h>
#include <sack_types.h>
#include <pssql.h>
#include "braintypes.h"
//#include "global.h"

//#define MAX_INPUT_VALUE  256 // used between 'world' values and 'neural' values
//#define MAX_OUTPUT_VALUE 256 // used to limit output - and determine value
#define MAX_GAIN         ((NATIVE)100)
//#define MAX_DECAY        256 // / 8 = +/-63 max per cycle
//#define MAX_THRESHOLD    256

#define NT_FREE    0  // allocated flag....
#define NT_ANALOG  1  // output is 0 -> 100 
#define NT_DIGITAL 2  // output is 0 or 100
#define NT_INPUT   3  // input comes from 'real' value
#define NT_OUTPUT  4  // output goes to 'real' value
#define NT_SIGMOID 5  // output is MAX_OUTPUT/(1*exp(-k*(Input-Threshold)))

#define MAX_NERVES  9  // for now limited by connection

// Value is an amount from 0 to 0.9999
// represneted by 0 - 0xFFFFFFFF
// All summations will cap at 1.

#define VALUE_CAP 0xFFFFFFFFUL
typedef uint32_t VALUE;


typedef class NEURON *PNEURON;
class NEURON {
public:
   struct NEURONset_tag **pool; // where this came from...
private:
   struct {
      int nType : 4;  // type of this neuron...
      int bOn   : 1;
      int bLock : 1;  // neuron is being READ or SET
   };
   int nCycle; // last cycle this neuron updated...
               // prevention of recursive loops....
               // this is globally set to the current
               // process cycle of processor machine.....
	class connector *io_node; // input/output goes here, a neuron is one or the other so...
	ANYVALUE Input; // current sum of all inputs... (or input index...)
	ANYVALUE Threshold; // mark point of sigmoid value also...
	//ANYVALUE ResetValue; // set output to this value on reset
	ANYVALUE Output;  // current level output... (or output index)

	class SYNAPSE *Nerve[MAX_NERVES]; // index with ORIDNAL DIRECTIONS

	ANYVALUE Min, Max; // uhmm these are brain params?
	TEXTSTR Name;

	INDEX iNeuron; // index of this neuron in a database...

	void Init( CTEXTSTR name );
public:
	void Detach( class SYNAPSE *what );
	class SYNAPSE **AttachSynapse( void );
	class SYNAPSE **AttachSynapse( INDEX position );

	IMPORT NEURON(CTEXTSTR );
	//IMPORT NEURON( int Type, PANYVALUE value );
	IMPORT NEURON( int Type, class connector *value );
	IMPORT NEURON( PNEURON );
	IMPORT NEURON();
	IMPORT ~NEURON();
	IMPORT NATIVE Collect( uint32_t cycle );
	void Emit( void );
	void Reset( void );
#undef new
	void *operator new(size_t sz, struct NEURONset_tag **ppSet );
	void *operator new(size_t sz);
#ifdef _MSC_VER
	void operator delete( void *ptr, struct NEURONset_tag **set );
#endif
	void operator delete( void *ptr );
	NATIVE get( void ) { return Output.get(); }
	NATIVE threshold( void ) { return Threshold.get(); }
	uint32_t Save( FILE *file, struct synapse_neuron_save_tag *saveinfo  );
	uint32_t Load( FILE *file /*va_list *args*/ );
	IMPORT NATIVE get( PNATIVE input, PNATIVE base, PNATIVE range, PNATIVE threshold, PNATIVE level );
	IMPORT NATIVE get( PNATIVE base, PNATIVE range, PNATIVE level );
	IMPORT void set( NATIVE threshold );
	IMPORT void set( NATIVE base, NATIVE range, NATIVE threshold );
	IMPORT CTEXTSTR name( void );
	IMPORT int type( void );
	IMPORT void type( int type );
	void *operator new( size_t sz, struct NEURONset_tag *frompool );
	IMPORT void SaveBegin( PODBC odbc );
	IMPORT INDEX Save( PODBC odbc, INDEX iParent );

	IMPORT LOGICAL Load( PODBC odbc, INDEX iParent );

	//void *operator new( size_t sz );
	//void operator delete( void *ptr );
};

#define MAXNEURONSPERSET 256
DeclareSet( NEURON );

typedef class SYNAPSE *PSYNAPSE;
class SYNAPSE {
public: // new needs to set this member...
   struct SYNAPSEset_tag **pool; // where this came from...
private:
   struct {
      int nType  : 4;
      int bLock  : 1; // synapse is being READ or SET
   };
	ANYVALUE Gain; // +/- level multiplier
	// 0 decay is NO decay
	// 1 decay steps the accumulator ...
	//   nGain - nGain * (nDelayAccumulator+1)
	// FFFFFFFF decay is instantaneous off.
	VALUE Decay;  // not sure what 'decay' is...
	VALUE DelayAccumulator; // accumulated decay value... 
	VALUE WorkGain; // modified with decay...
	class SYNAPSE **SourceReference;
	class SYNAPSE **DestinationReference; 
	PNEURON Source;  // end with the GATE
	PNEURON Destination; // end with the SYNAPSE
	TEXTSTR Name;
	INDEX iSynapse; // index of this synapse in a database.
	void Init( CTEXTSTR name, NATIVE Gain );
public:
	IMPORT int AttachSource( PNEURON, PSYNAPSE* );
	IMPORT int AttachDestination( PNEURON, PSYNAPSE* );
	IMPORT int AttachSource( PNEURON );
	IMPORT int AttachDestination( PNEURON );
	IMPORT int DetachSource( void );
	IMPORT int DetachDestination( void );
public:

	IMPORT SYNAPSE( CTEXTSTR name, NATIVE Gain );
	IMPORT SYNAPSE( CTEXTSTR name );
	IMPORT SYNAPSE( NATIVE Gain );
	IMPORT SYNAPSE( PSYNAPSE default_synapse );
	IMPORT SYNAPSE();
	IMPORT ~SYNAPSE();
	void *operator new(size_t sz, struct SYNAPSEset_tag **set );
	void *operator new(size_t sz);
#ifdef _MSC_VER
	void operator delete( void *ptr, struct SYNAPSEset_tag *set );
#endif
	void operator delete( void *ptr );
	IMPORT NATIVE Collect( uint32_t cycle ); //{ return ( Source->Collect(cycle) * Gain.get() ) / ((NATIVE)256); }
	IMPORT CTEXTSTR name( void );
	IMPORT void get( PNATIVE gain, PNATIVE level, PNATIVE range );
	IMPORT NATIVE gain( void );
	IMPORT void set( NATIVE gain );
	inline int DestinationIs( PNEURON neuron ) { return Destination == neuron; }
	inline int SourceIs( PNEURON neuron ) { return Source == neuron; }
	//void *operator new( size_t sz, struct SYNAPSEset_tag **frompool );
	//void *operator new( size_t sz );
	//void operator delete( void *ptr );
	IMPORT void SaveBegin( PODBC odbc );
	IMPORT INDEX Save( PODBC odbc, INDEX iParent );
	//INDEX Save( PODBC odbc );
    IMPORT uintptr_t Load( PODBC odbc, INDEX iParent, INDEX iBrain );

};

#define MAXSYNAPSESPERSET 256
DeclareSet( SYNAPSE );

typedef struct synapse_neuron_save_tag {
	PSYNAPSE pSynapses;
	uint32_t nSynapses;
	PNEURON pNeurons;
	uint32_t nNeurons;
	uint32_t *dwWritten;
	FILE *file;
} SYNAPSE_SAVE, NEURON_SAVE, *PSAVEINFO;

#endif
