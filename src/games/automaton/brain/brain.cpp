#ifdef __WINDOWS__
//#include <afxwin.h>
//#include "stdafx.h"
#include <stdhdrs.h>
#include <commctrl.h>
#endif
//#include <windows.h>
#include <math.h>  // exp()
//#include <process.h>
#include <typelib.h>
#include "global.h"
#include "brain.hpp"

#include <sharemem.h>
#include <deadstart.h>
// brain.cpp : Defines the entry point for the DLL application.
//

#define AliasVoidProc( type,name ) type CPROC name( PBRAIN brain ) { return brain->name(); }  \
	type BRAIN::name(void)

#ifdef _MSC_VER
#define AliasProc( ret, name, args, call_args ) \
	ret BRAIN::name args
#define AliasProcVoid( name, args, call_args )  \
	void BRAIN::name args
#else
#define f(...) __VA_ARGS__

#define AliasProc( ret, name, args, call_args ) ret CPROC name( PBRAIN brain, f/**/args ) { return brain->name call_args; } \
	ret BRAIN::name args
#define AliasProcVoid( name, args, call_args ) void CPROC name( PBRAIN brain, f/**/args ) { brain->name call_args; } \
	void BRAIN::name args
#endif

//HINSTANCE ghInstDLL;
static _32 brain_timer; // one timer for all brains.
static PLIST brains;

//----------------------------------------------------------------------

void CPROC BrainThread( PTRSZVAL brain )
{
   PBRAIN b = (PBRAIN)(brain);
	if( b->bExit )
		b->bReleased = true;
	else
		b->Process();  // always process....
}


static PTRSZVAL CPROC DoBrainTick(PTRSZVAL psv, INDEX idx, POINTER *item )
{
	PBRAIN brain = ((PBRAIN*)item)[0];
	{
		if( brain->bExit )
		{
			brain->bReleased = true;
			(*item) = NULL;
		}
		else if( brain->bHalt )
			return 0;
		else
			brain->Process();
		return 0;
	}
}

void CPROC BrainTick( PTRSZVAL unused )
{
	INDEX idx;
	PBRAIN brain;
	LIST_FORALL( brains, idx, PBRAIN, brain )
	{
		DoBrainTick( 0, idx, (POINTER*)&brain );
	}
	//ForAllLinks( &brains, DoBrainTick, 0 );

}

//----------------------------------------------------------------------

void BRAIN_STEM::AddInput( PANYVALUE pv, CTEXTSTR name )
{
	connector *newcon = new connector( name, pv );
	newcon->pbs = this;
	Inputs.add( newcon );
}

//----------------------------------------------------------------------

void BRAIN_STEM::AddInput( connector *connector )
{
	connector->pbs = this;
	Inputs.add( connector );
}

//----------------------------------------------------------------------

void BRAIN_STEM::AddOutput( PANYVALUE pv, CTEXTSTR name )
{
	connector *newcon = new connector( name, pv );
	newcon->pbs = this;
	Outputs.add( newcon );
}

//----------------------------------------------------------------------

void BRAIN_STEM::AddOutput( connector *connector )
{
	connector->pbs = this;
	Outputs.add( connector );
}

void BRAIN_STEM::AddModule( PBRAIN_STEM pbs )
{
	pbs->parent = this;
	Modules.add( pbs );
}

PBRAIN_STEM BRAIN::GetBrainStem( INDEX id_stem )
{
	PBRAIN_STEM pbs = (PBRAIN_STEM)BrainStems.first();
	if( id_stem == INVALID_INDEX )
		return NULL;
	// uses id_stem as a countdown-counter to get stem by index....
	// using first, next,k next next and stopping when id_stem == 0 
	for( ;id_stem; id_stem-- )
	{
		pbs = (PBRAIN_STEM)BrainStems.next();
	}
	if( id_stem != INVALID_INDEX )
		return pbs;
	return NULL;
}

PBRAIN_STEM BRAIN::GetBrainStem( CTEXTSTR stem_name )
{
	PBRAIN_STEM pbs = (PBRAIN_STEM)BrainStems.first();
	if( !stem_name )
		return NULL;
	while( pbs )
	{
		if( StrCmp( pbs->name(), stem_name ) == 0 )
			break;
		pbs = (PBRAIN_STEM)BrainStems.next();
	}
	return pbs;
}

//----------------------------------------------------------------------
AliasProc( PBRAIN_STEM,AddComponent,(int n, CTEXTSTR pCompName
                        , arg_list vl ), (n,pCompName,vl))
{
	int c;
	PBRAIN_STEM pNew = new BRAIN_STEM( pCompName );

	for( c = 0; c < n; c++ )
	{
		PARAM( vl, _32, Type );
		PARAM( vl, PANYVALUE, pValue );
		PARAM( vl, CTEXTSTR, pName );
		if( Type == INPUT )
		{
			pNew->AddInput( pValue, pName );
		}
		else if( Type == OUTPUT )
		{
			pNew->AddOutput( pValue, pName );
		}
	}
	BrainStems.add( (POINTER)pNew );
	return pNew;
}

//----------------------------------------------------------------------

PBRAIN_STEM BRAIN::AddComponent(int n, CTEXTSTR pCompName
										 , ...)
{
   va_list vl;
   va_start( vl, pCompName );
   DebugBreak();
	//return BRAIN::AddComponent( n, pCompName, vl );
   return NULL;
}

//----------------------------------------------------------------------

PBRAIN_STEM CPROC AddComponent(PBRAIN brain, int n, CTEXTSTR pCompName
										 , ...)
{
   va_list vl;
   va_start( vl, pCompName );
   DebugBreak();
   //return brain->AddComponent( n, pCompName, vl );
   return NULL;
}

//----------------------------------------------------------------------

AliasProcVoid( AddBrainStem, ( PBRAIN_STEM pbs ), (pbs) )
{
   BrainStems.add( (POINTER)pbs );
}


BRAIN_STEM::BRAIN_STEM( CTEXTSTR name)
{
	//strcpy( FullName, name );
	parent = NULL;
	group = NULL;
	Name = StrDup( name );
	// hmm not sure what to do here...
}

BRAIN_STEM::~BRAIN_STEM() 
{
	PBRAIN_STEM node;
	for( node = (PBRAIN_STEM)Modules.first();
		node;
		node = (PBRAIN_STEM)Modules.next() )
		delete node;
	PCONNECTOR conn;
	for( conn = (PCONNECTOR)Inputs.first();
		conn;
		conn = (PCONNECTOR)Inputs.next() )
		delete conn;
	for( conn = (PCONNECTOR)Outputs.first();
		conn;
		conn = (PCONNECTOR)Outputs.next() )
		delete conn;
	//Release(  );
	Release( this->Name );
}


BRAIN_STEM::BRAIN_STEM( CTEXTSTR name
							  , connector **inputs, int nInputs
							  , connector **outputs, int nOutputs )
{
	//strcpy( FullName, name );
	Name = StrDup( name );
	// hmm not sure what to do here...
	int n;
	for( n = 0; n < nInputs; n++ )
	   AddInput( inputs[n] );
	for( n = 0; n < nOutputs; n++ )
	   AddOutput( outputs[n] );
}

//----------------------------------------------------------------------

BRAIN_STEM::BRAIN_STEM()
{
	// hmm not sure what to do here...

}

//----------------------------------------------------------------------

void BRAIN::DeleteComponents( void )
{
    PBRAIN_STEM pnext;
    for( pnext = (PBRAIN_STEM)BrainStems.first();
          pnext;
          pnext = (PBRAIN_STEM)BrainStems.next() )
    {
        delete pnext;
    }
}

//----------------------------------------------------------------------

static PTRSZVAL CPROC  cReleaseNeuron( PNEURON neuron,  PTRSZVAL psv )
{
	PBRAIN brain = (PBRAIN)psv;
	brain->ReleaseNeuron( neuron );
	return 0;
}

static PTRSZVAL CPROC  cReleaseSynapse( PSYNAPSE neuron,  PTRSZVAL psv )
{
	PBRAIN brain = (PBRAIN)psv;
	brain->ReleaseSynapse( neuron );
	return 0;
}

BRAIN::~BRAIN()
{
	// should check, if last brain, remove timer...
	// brain_timer is started by the brain creator...
   DeleteComponents();
   ForAllInSet( NEURON, this->NeuronPool, (FAISCallback)cReleaseNeuron, (PTRSZVAL)this );
   ForAllInSet( SYNAPSE, this->SynapsePool, (FAISCallback)cReleaseSynapse, (PTRSZVAL)this );
   DeleteSetEx( NEURON, &this->NeuronPool );
   DeleteSetEx( SYNAPSE, &this->SynapsePool );
   End();
}

//----------------------------------------------------------------------

void BRAIN::Stop( void )
{
	bHalt = 1;
}

void BRAIN::Run( void )
{
	bHalt = 0;
}

void BRAIN::Init( void )
{
	// nothing to do here I think...
	//_32 dwThreadId;
	//pBrainStem = pbs;
	bHalt = 0;
	bExit = 0;
	bReleased = FALSE;
	k = 0.05f;
	NeuronPool = NULL; //new NEURONSET();
	SynapsePool = NULL; //new SYNAPSESET();
#define zero(n) /*Log( "Zero of pools and active list needs to be upated!" );*/
	zero( NeuronPool );
	zero( SynapsePool );
	zero( ActiveList );
	//nActive = 0;
	nCycle = 0;
	InitializeCriticalSec( &cs );
	AddLink( &brains, this );
	//zero( Inputs );
	//zero( Outputs );
	// otherwise mount oh board....
	//   Board = new BOARD( this ); // pass this brain to create...
	 //Board = NULL;
	 if( !brain_timer )
		brain_timer = sack::timers::AddTimer( 5, BrainTick, 0 );
}

void CPROC InitBrain( PBRAIN brain )
{
	brain->Init();
}

PBRAIN CPROC CreateBrain( void )
{
	PBRAIN brain = new BRAIN;
   return brain;
}

void CPROC DestroyBrain( PBRAIN brain )
{
	Release( brain );
}

//----------------------------------------------------------------------

BRAIN::BRAIN( PBRAIN_STEM pbs )
{
   	InitBrain( (PBRAIN)this );
	AddBrainStem( pbs );
}

BRAIN::BRAIN()
{
	InitBrain( (PBRAIN)this );
}

//----------------------------------------------------------------------
//PUBLIC_DATA( WIDE("brain"), class BRAIN, InitBrain, DestroyBrain );

PRELOAD( RegisterBrain )
{
   // this will be invoked by requesting the interface.
}

//----------------------------------------------------------------------

//----------------------------------------------------------------------

PTRSZVAL CPROC DoCollect( void *_this, PTRSZVAL cycle )
{
    ((PNEURON)_this)->Collect( (_32)cycle );
   return 0;
}

//----------------------------------------------------------------------

void BRAIN::Process( void )
{
	if( bHalt )
		return;
	EnterCriticalSec( &cs );
	nCycle++;  // starting a new cycle....
	//lprintf( WIDE("Tick...") );
	{
		PBRAIN_STEM pbs;
		for( pbs = (PBRAIN_STEM)BrainStems.first(); pbs; pbs = (PBRAIN_STEM)BrainStems.next() )
		{
			pbs->update( nCycle );
		}
	}
	ForAllInSet( NEURON, NeuronPool, DoCollect, nCycle );
	//NeuronPool->forall( DoCollect, nCycle );
	//ForAllInSet( NEURON, NeuronPool->pool, DoCollect, nCycle );
	LeaveCriticalSec( &cs );
}

//----------------------------------------------------------------------

void NEURON::Reset( void )
{
    switch( nType ) // if allocated
    {
    case NT_INPUT:
    break;
    case NT_OUTPUT:
        // should probably reset output states....
        break;
    case NT_DIGITAL:
    case NT_ANALOG:
    case NT_SIGMOID:
      Input.set((NATIVE)0);
       Output.set((NATIVE)0);
        break;
    }
}

//----------------------------------------------------------------------

PTRSZVAL CPROC DoReset( POINTER _this, PTRSZVAL psv )
{
    ((PNEURON)_this)->Reset();
   return 0;
}

//----------------------------------------------------------------------

void BRAIN::Reset( void )
{
	ForAllInSet( NEURON, NeuronPool, DoReset, 0 );
	//NeuronPool->forall( DoReset, 0 );
    //ForAllInSet( NEURON, NeuronPool, DoReset, 0 );
}

//----------------------------------------------------------------------


AliasVoidProc( PNEURON, GetNeuron )
{
	Lock();
	PNEURON neuron = new( &NeuronPool ) NEURON;
	Unlock();
	return neuron;
}

//----------------------------------------------------------------------

AliasProc( PNEURON, DupNeuron,( PNEURON default_neuron ),(default_neuron))
{
	Lock();
	PNEURON neuron = new( &NeuronPool ) NEURON( default_neuron );
	Unlock();
	return neuron;
}

//----------------------------------------------------------------------

AliasProc( PSYNAPSE, DupSynapse,( PSYNAPSE default_synapse ),(default_synapse))
{
	Lock();
	PSYNAPSE synapse = new( &SynapsePool ) SYNAPSE( default_synapse );
	Unlock();
	return synapse;
}

//----------------------------------------------------------------------

AliasVoidProc( PSYNAPSE,GetSynapse )
{
	return new(&SynapsePool) SYNAPSE;
}

//----------------------------------------------------------------------

AliasProc( int, LinkSynapseTo, ( PSYNAPSE ps, PNEURON pn, int n ), (ps, pn, n ) )
{
    Lock();
    if( ps->AttachDestination( pn
                                     , pn->AttachSynapse( n ) ) )
    {
        Unlock();
        return TRUE;
    }
    Unlock();
	return FALSE;
}

//----------------------------------------------------------------------
AliasProc( int, LinkSynapseFrom, ( PSYNAPSE ps, PNEURON pn, int n ), (ps, pn, n ) )
{
    Lock();
    if( ps->AttachSource( pn
                              , pn->AttachSynapse( n ) ) )
    {
        Unlock();
        return TRUE;
    }
    Unlock();
	return FALSE;
}

//----------------------------------------------------------------------
AliasProc( int, LinkSynapseTo, ( PSYNAPSE ps, PNEURON pn ), (ps,pn) )
{
    Lock();
    if( ps->AttachDestination( pn
                                     , pn->AttachSynapse() ) )
    {
        Unlock();
        return TRUE;
    }
    Unlock();
	return FALSE;
}

//----------------------------------------------------------------------
AliasProc( int, LinkSynapseFrom, ( PSYNAPSE ps, PNEURON pn ), (ps,pn) )
{
    Lock();
    if( ps->AttachSource( pn
                              , pn->AttachSynapse() ) )
    {
        Unlock();
        return TRUE;
    }
    Unlock();
	return FALSE;
}

//----------------------------------------------------------------------
 void BRAIN::UnLinkSynapseTo( PSYNAPSE pSyn )
{
    Lock();
    pSyn->DetachDestination();
	Unlock();
}

//----------------------------------------------------------------------

 void BRAIN::UnLinkSynapseFrom( PSYNAPSE pSyn )
{
    Lock();
    pSyn->DetachSource();
	Unlock();
}

//----------------------------------------------------------------------

 void BRAIN::ReleaseNeuron( PNEURON pn )
{
    // should validate that it is unlinked...
	// va_args args;
	//init_args( args );
	//PushArgument( args, PNEURONSET*, &NeuronPool );
	//PushArgument( args, PNEURON, pn );
	delete pn;
	//delete (PNEURON)pass_args(args);
}

//----------------------------------------------------------------------

 void BRAIN::ReleaseSynapse( PSYNAPSE ps )
{
    // should validate it is unlinked...
	// va_args args;
	//init_args( args );
	//PushArgument( args, PSYNAPSESET *, &SynapsePool );
	//PushArgument( args, PSYNAPSE, ps );
	delete ps;
	//delete (PSYNAPSE)pass_args(args);
}

//----------------------------------------------------------------------

PTRSZVAL CPROC SaveNeuron( void *neuron, PTRSZVAL psvFile )
{
    ((PNEURON)neuron)->Save( (PODBC)NULL, INVALID_INDEX );
   return 0;
}

//----------------------------------------------------------------------

struct save_info_tag {
		PODBC odbc;
		INDEX iParent;
	};
    
PTRSZVAL CPROC SaveSynapse( void *synapse, PTRSZVAL args )
{
	struct save_info_tag *info = (struct save_info_tag*)args;

    ((PSYNAPSE)synapse)->Save( info->odbc, info->iParent );
    return 1;
}

//----------------------------------------------------------------------

//#if 0
INDEX BRAIN::Save( PODBC odbc, CTEXTSTR brainname )
{
	struct save_info_tag save_info;
   TEXTCHAR tmpval[32];
	_32  dwWritten, dwZero;
	if( !odbc )
		odbc = ConnectToDatabase( WIDE("game_dev") );
	if( !odbc )
		return INVALID_INDEX;
	Lock();
	save_info.odbc = odbc;
	save_info.iParent = INVALID_INDEX;
	dwZero = 0;
	dwWritten = 0;
   snprintf( tmpval, sizeof( tmpval), WIDE("%g"), k );
	SQLInsert( odbc
				, WIDE("brain_info")
				, WIDE("brain_name"), 1, brainname
				, WIDE("version"), 2, 0
             , WIDE("k"), 0, tmpval );
	save_info.iParent = FetchLastInsertID( odbc, NULL, NULL );

	{
		SYNAPSE_SAVE save;
		save.pSynapses = (PSYNAPSE)GetLinearSetArray( SYNAPSE, SynapsePool, (int*)&save.nSynapses );
		save.pNeurons = (PNEURON)GetLinearSetArray( NEURON, NeuronPool, (int*)&save.nNeurons );
		//save.brain_id = brain_id;
		//save.file = file;
		ForAllInSet( NEURON, NeuronPool, SaveNeuron, (PTRSZVAL)&save_info );
		//NeuronPool->forall( SaveNeuron, (PTRSZVAL)&save_info );
		ForAllInSet( SYNAPSE, SynapsePool, SaveSynapse, (PTRSZVAL)&save_info );
		//SynapsePool->forall( SaveSynapse, (PTRSZVAL)&save_info );
	}
	Unlock();
	return dwWritten;
}
//#endif
#if 0
_32 BRAIN::Load( FILE *file )
{
#if 0
   _32 dwR, dwRead, dwVersion, i, n, idx, j;
   NEURON neuron;
   SYNAPSE synapse;
   dwRead = 0;
   Lock();
    ReadFile( file, &dwVersion, sizeof( _32 ), &dwR, NULL );
    if( dwVersion != *(_32*)WIDE("MIND") )
        return 0; // failed to read any really...

   ReadFile( file, &dwVersion, sizeof( _32 ), &dwR, NULL );

    ReadFile( file, &k, sizeof(k), &dwR, NULL );

    ReadFile( file, &n, sizeof(n), &dwR, NULL);


    for( i = 0; i < n; i++ )
	 {
		 PNEURON pn;
		 pn = GetNeuron();
		 pn->Load( file );
/*
		 ReadFile( file, &neuron, sizeof(NEURON), &dwR, NULL );
		 idx = (int)neuron.Name;
		 pn = GetNeuron();
		 *pn = neuron;
		 for( j = 0; j < MAX_NERVES; j++ )
		 {
			 if( pn->Nerve[j] )
				 pn->Nerve[j] = SynapsePool
					 + ((int)pn->Nerve[j]);
		 }
*/
	 }

	 ReadFile( file, &n, sizeof(n), &dwR, NULL );
	 if( n > MAX_SYNAPSES )
		 DebugBreak();

	 for( i = 0; i < n; i++ )
	 {
		 PSYNAPSE ps = GetSynapse();
		 ps->Load( file );
/*
		 ReadFile( file, &synapse, sizeof(SYNAPSE), &dwR, NULL );

		 idx = (int)synapse.Name;
		 SynapsePool[idx] = synapse;
		 SynapsePool[idx].Name = NULL;

		 SynapsePool[idx].Destination = NeuronPool + (int)SynapsePool[idx].Destination;
		 SynapsePool[idx].Source = NeuronPool + (int)SynapsePool[idx].Source;
*/
	 }
}
   }
   else 
      k = 0.05f;

   if( Board )
      dwRead += Board->Load( file ); // let board attempt to load...

   Unlock();
    return dwRead;
#else
   return 0;
#endif
}

#endif
//----------------------------------------------------------------------

AliasProc( PNEURON, GetInputNeuron, ( PBRAIN_STEM pbs, int nInput ), (pbs,nInput) )
//PNEURON BRAIN::GetInputNeuron( PBRAIN_STEM pbs, int nInput )
{
	Lock();
   //PANYVALUE value = pbs->getinput( nInput );
	PNEURON neuron = new( &NeuronPool ) NEURON( NT_INPUT, pbs->getinput( nInput ) );
	Unlock();
	return neuron;
}

//----------------------------------------------------------------------

AliasProc( PNEURON, GetInputNeuron, ( connector *input ), (input) )
//PNEURON BRAIN::GetInputNeuron( PBRAIN_STEM pbs, int nInput )
{
	Lock();
   //PANYVALUE value = pbs->getinput( nInput );
	PNEURON neuron = new( &NeuronPool ) NEURON( NT_INPUT, input);
	Unlock();
	return neuron;
}

//----------------------------------------------------------------------

AliasProc( PNEURON, GetOutputNeuron, (PBRAIN_STEM pbs, int nOutput ), (pbs,nOutput) )
{
	Lock();
	//PANYVALUE value = pbs->getoutput( nOutput );
	PNEURON neuron = new( &NeuronPool ) NEURON( NT_OUTPUT, pbs->getinput( nOutput ) );
	Unlock();
	return neuron;
}

AliasProc( PNEURON, GetOutputNeuron, (connector *output ), (output) )
{
	Lock();
	PNEURON neuron = new( &NeuronPool ) NEURON( NT_OUTPUT, output );
	Unlock();
	return neuron;
}

//----------------------------------------------------------------------

PBRAIN_STEM BRAIN::first( void )
{
	return (PBRAIN_STEM)BrainStems.first();
}

//----------------------------------------------------------------------

PBRAIN_STEM BRAIN::next( void )
{
	return (PBRAIN_STEM)BrainStems.next();
}
//----------------------------------------------------------------------

CTEXTSTR BRAIN_STEM::fullname( void )
{
	if( this->parent )
	{
		snprintf( FullName, sizeof( FullName ), WIDE("%s/%s"), this->parent->fullname(), this->Name );
		return FullName;
	}
	return Name;
}

//----------------------------------------------------------------------

CTEXTSTR BRAIN_STEM::name( void )
{
	return Name;
}

//----------------------------------------------------------------------

PCONNECTOR BRAIN_STEM::next_input( void )
{
	return (PCONNECTOR)Inputs.next();
}


//----------------------------------------------------------------------

PCONNECTOR BRAIN_STEM::first_input( void )
{
	return (PCONNECTOR)Inputs.first();
}

//----------------------------------------------------------------------

PCONNECTOR BRAIN_STEM::next_output( void )
{
	return (PCONNECTOR)Outputs.next();
}

//----------------------------------------------------------------------

BRAIN_STEM * BRAIN_STEM::next_module( void )
{
	return (BRAIN_STEM*)Modules.next();
}


//----------------------------------------------------------------------

PCONNECTOR BRAIN_STEM::first_output( void )
{
	return (PCONNECTOR)Outputs.first();
}

//----------------------------------------------------------------------

BRAIN_STEM * BRAIN_STEM::first_module( void )
{
	return (BRAIN_STEM*)Modules.first();
}

//----------------------------------------------------------------------


void BRAIN::GetConnector( CTEXTSTR name, PCONNECTOR *connector_result, int bInput )
{
	CTEXTSTR start, end;
	start = name;
	end = strchr( start, '/' );
	if( !end )
	{
		lprintf( WIDE("Invalid name, should have at least one slash for brainstem/outputname") );
		return;
	}
	{
		PBRAIN_STEM pbs;
		for( pbs = first(); pbs; pbs = next() )
		{
			if( StrCmpEx( pbs->name(), start, end-start ) == 0 )
			{
				break;
			}
		}
		if( pbs )
		{
			PBRAIN_STEM actual = pbs;
			PCONNECTOR conn;
			//(*stem_result) = pbs;
			start = end+1;
			end = strchr( start, '/' );
			while( end )
			{
				for( actual = pbs->first_module(); actual; actual = pbs->next_module() )
				{
					if( StrCmpEx( actual->name(), start, end-start ) == 0 )
						break;
				}
				if( actual )
					pbs = actual;
				else
				{
					lprintf( WIDE("Failed to find registered IO thing") );
					return;
				}
				start = end+1;
				end = strchr( start, '/' );
			}
			if( !end )
			{
				if( bInput )
				{
					for( conn = pbs->first_input(); conn; conn = pbs->next_input() )
					{
						if( StrCmpEx( conn->name(), start, end-start ) == 0 )
							break;
					}
					(*connector_result)=conn;
				}
				else
				{
					for( conn = pbs->first_output(); conn; conn = pbs->next_output() )
					{
						if( strcmp( conn->name(), start ) == 0 )
							break;
					}
					(*connector_result)=conn;
				}
			}
		}
	}
}


//----------------------------------------------------------------------

void CPROC ResetBrain( PBRAIN brain )
{
   brain->Reset();
}

//----------------------------------------------------------------------
#ifdef INTERFACE_USED
static BRAIN_INTERFACE Interface
	= { CreateBrain, DestroyBrain, ResetBrain
	  , GetNeuron
	  , DupNeuron
	  , GetInputNeuron
	  , GetOutputNeuron
	  , GetSynapse
     , DupSynapse
	  , NULL
	  , NULL
	  , NULL
     , NULL
	  , LinkSynapseFrom
	  , LinkSynapseTo
	  , NULL
     , NULL
	  , AddBrainStem
	  , AddComponent
	};

static int ConnectToServer( void )
{
	return 1;
}

static int references;
PUBLIC( PBRAIN_INTERFACE, GetBrainInterface )( void )
{
	if( ConnectToServer() )
	{
      references++;
		return &Interface;
	}
   return NULL;
}
static void DisconnectFromServer( void )
{
}

ATEXIT( DropInterface )
{
   Log1( WIDE("Dropping 1 of %d display connections.."), references );
	references--;
	if( !references )
      DisconnectFromServer();
}
#endif


TEXTCHAR *brain_info_table =
WIDE("CREATE TABLE `brain_info` (                             ")
WIDE("  `brain_info_id` int(11) NOT NULL auto_increment,		 ")
WIDE("  `brain_name` varchar(100) NOT NULL default '',		 ")
WIDE("  `version` int(11) NOT NULL default '0',				 ")
WIDE("  `k` double NOT NULL default '0',						 ")
WIDE("  PRIMARY KEY  (`brain_info_id`)						 ")
WIDE(") TYPE=MyISAM;											 ")
;														 

TEXTCHAR *brain_neuron_table =								 
WIDE("CREATE TABLE `brain_neuron` (                          ")
WIDE("  `brain_neuron_id` int(11) NOT NULL auto_increment,	")
WIDE("  `brain_info_id` int(11) NOT NULL default '0',		")
WIDE("  `parent_id` int(11) NOT NULL default '0',			")
WIDE("  `type` int(11) NOT NULL default '0',					")
WIDE("  `threshold` double NOT NULL default '0',				")
WIDE("  `min_output` double NOT NULL default '0',			")
WIDE("  `max_output` double NOT NULL default '0',			")
WIDE("  PRIMARY KEY  (`brain_neuron_id`)						")
WIDE(") TYPE=MyISAM;											")
;

TEXTCHAR *brain_synapse_table = 
WIDE("CREATE TABLE `brain_synapse` (                         ")
WIDE("  `brain_synapse_id` int(11) NOT NULL auto_increment,	")
WIDE("  `brain_info_id` int(11) NOT NULL default '0',		")
WIDE("  `brain_neuron_id_from` int(11) NOT NULL default '0',	")
WIDE("  `brain_neuron_id_to` int(11) NOT NULL default '0',	")
WIDE("  `synapse_gain` double NOT NULL default '0',			")
WIDE("  PRIMARY KEY  (`brain_synapse_id`)					")
WIDE(") TYPE=MyISAM;											")
;

TEXTCHAR *brain_connectors = 
WIDE("CREATE TABLE `brain_connectors` (                         ")
WIDE("  `brain_connector_id` int(11) NOT NULL auto_increment,	")
WIDE("  `connector_name` varchar(100) NOT NULL default '0',		")
WIDE("  `input` int(11) NOT NULL default '0',	")
WIDE("  `parent_id` int(11) NOT NULL default '0',	")
WIDE("  PRIMARY KEY  (`brain_connector_id`)					")
WIDE(") TYPE=MyISAM;											")
;


PRELOAD( CreateTables )
{
	PTABLE table;

	table = GetFieldsInSQL( brain_info_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( brain_neuron_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( brain_synapse_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( brain_connectors, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );
}


CTEXTSTR connector::name(void)
{ return c_name;
}

