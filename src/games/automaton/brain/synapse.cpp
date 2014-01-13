#include "neuron.h"

#include <sharemem.h>
#undef new
#undef delete

int SYNAPSE::DetachSource( void )
{
	if( SourceReference )
	{
		*SourceReference = NULL;
		SourceReference = NULL;
      Source = NULL;
		return TRUE;
	}
	return FALSE;
}

int SYNAPSE::DetachDestination( void )
{
	if( DestinationReference )
	{
		*DestinationReference = NULL;
		DestinationReference = NULL;
      Destination = NULL;
      return TRUE;
	}
   return FALSE;
}

int SYNAPSE::AttachDestination( PNEURON neuron, PSYNAPSE*ppSyn )
{
	if( !ppSyn && neuron )
		for( INDEX n = 0; 
			  !(ppSyn = neuron->AttachSynapse( n ) )
			  && n < MAX_NERVES; 
			n++ );
	if( !Destination && neuron && ppSyn )
	{
		DestinationReference = ppSyn;
		*DestinationReference = this;
		Destination = neuron;
      return TRUE;
	}
	return FALSE;
}
int SYNAPSE::AttachDestination( PNEURON neuron )
{
	return AttachDestination( neuron, NULL );
}


int SYNAPSE::AttachSource( PNEURON neuron, PSYNAPSE*ppSyn )
{
	if( !ppSyn && neuron )
		for( INDEX n = 0; 
			  !(ppSyn = neuron->AttachSynapse( n ) )
			  && n < MAX_NERVES; 
			n++ );
	if( !Source && neuron && ppSyn )
	{
		SourceReference = ppSyn;
		*SourceReference = this;
		Source = neuron;
		return TRUE;
	}
	return FALSE;
}
int SYNAPSE::AttachSource( PNEURON neuron )
{
	return AttachSource( neuron, NULL );
}


void *SYNAPSE::operator new(size_t sz, struct SYNAPSEset_tag **set )
{
	if( sz != sizeof( class SYNAPSE ) )
		return NULL;
	PSYNAPSE synapse = GetFromSet( SYNAPSE, set );
	MemSet( synapse, 0, sz );
	synapse->pool = set;
	return synapse;
}

void *SYNAPSE::operator new(size_t sz)
{
	// impossible to allocate without specifying the set.
	return NULL;
}

void SYNAPSE::operator delete( void *ptr )
{
	DeleteFromSet( SYNAPSE, (((PSYNAPSE)(ptr))->pool), (PSYNAPSE)ptr );
}

#ifdef _MSC_VER
void SYNAPSE::operator delete( void *ptr, struct SYNAPSEset_tag *set )
{
	lprintf( WIDE("DO SOMETHING HERE!") );
//	(((PSYNAPSE)(ptr))->pool)->drop( (PSYNAPSE)ptr );
}
#endif

void SYNAPSE::Init( CTEXTSTR name, NATIVE Gain )
{
	if( name )
	{
		Name = (TEXTSTR)Allocate( strlen( name ) + 1 );
		strcpy( Name, name );
	}
	else
	{
		static int nDefault = 1;
		TEXTCHAR tmpname[32];
		Name = (TEXTSTR)Allocate( snprintf( tmpname, sizeof( tmpname ), WIDE("Synapse %d"), nDefault ) + 1 );
		strcpy( Name, tmpname );
	}
   // set the type...
	SYNAPSE::Gain.set( VAL_NATIVE, (NATIVE)Gain );
	//SYNAPSE::Gain.set( VAL_NATIVE, (NATIVE)0 );
   // then set the value...
	//SYNAPSE::Gain.set( Gain );
}

SYNAPSE::SYNAPSE( NATIVE Gain )
{
	Init( NULL, Gain );
}

SYNAPSE::SYNAPSE( CTEXTSTR name, NATIVE Gain )
{
	Init( name, Gain );
}

SYNAPSE::SYNAPSE( CTEXTSTR name )
{
	SYNAPSE::Init( name, 256 );
}

SYNAPSE::SYNAPSE()
{
	Init( NULL, 256 );
}

SYNAPSE::SYNAPSE( PSYNAPSE default_synapse )
{
	Init( NULL, 0 );
   Gain = default_synapse->Gain;
}

SYNAPSE::~SYNAPSE()
{
	DetachSource();
	DetachDestination();
	Release( Name );
	Name = NULL;
}

//----------------------------------------------------------------------

CTEXTSTR SYNAPSE::name( void )
{
   return Name;
}

//----------------------------------------------------------------------

void SYNAPSE::set( NATIVE gain )
{
   Gain.set( gain );
}

//----------------------------------------------------------------------

void SYNAPSE::get( PNATIVE gain, PNATIVE level, PNATIVE range )
{
   if( gain )
		*gain = Gain.get();
	Source->get( NULL, range, level );
}

//----------------------------------------------------------------------

NATIVE SYNAPSE::Collect( _32 cycle )
{
	return ( (NATIVE)Source->Collect(cycle) * Gain.get() ) / ((NATIVE)256);
}

//----------------------------------------------------------------------

NATIVE SYNAPSE::gain( void )
{
   return Gain.get();
}

//----------------------------------------------------------------------

void SYNAPSE::SaveBegin( PODBC odbc )
{
	if( iSynapse && iSynapse != INVALID_INDEX  )
	{
		SQLCommandf( odbc, WIDE("delete from brain_synapse where brain_synapse_id=%lu"), iSynapse );
	}
	iSynapse = 0;
}

//----------------------------------------------------------------------

INDEX SYNAPSE::Save( PODBC odbc, INDEX iParent )
{
	INDEX iSource = Source->Save( odbc, iParent );
	INDEX iDestination = Destination->Save( odbc, iParent );
	if( !iSynapse )
	{
		TEXTCHAR szGain[32];
		snprintf( szGain, sizeof( szGain ), WIDE("%g"), Gain.get() );
		if( SQLInsert( odbc, WIDE("brain_synapse")
						 , WIDE("synapse_gain"), 0, szGain
						 , WIDE("brain_neuron_id_from"), 2, iSource
						 , WIDE("brain_neuron_id_to"), 2, iDestination
						 , NULL, 0, NULL ) )
		{
			iSynapse = FetchLastInsertID(odbc, NULL, NULL);
		}
	}
	else
	{
		SQLCommandf( odbc, WIDE("update brain_neuron set synapse_gain=%g,brain_neuron_id_to=%lu,brain_neuron_id_from=%lu where brain_synapse_id=%lu")
					  , Gain.get()
					  , iDestination
					  , iSource
					  , iSynapse
					  );
	}
	return iSynapse;
}

//----------------------------------------------------------------------

PTRSZVAL SYNAPSE::Load( PODBC odbc, INDEX iParent, INDEX iLoad )
{
	if( iSynapse )
	{
		if( iSynapse != iLoad )
		{
			lprintf( WIDE("We're fatally confused.") );
			DebugBreak();
		}
	}
	else
		iSynapse = iLoad;

	{
		CTEXTSTR *result;
		PushSQLQueryEx( odbc );
		if( SQLRecordQueryf( odbc, NULL, &result, NULL, WIDE("select synapse_gain,brain_neuron_id_from,brain_neuron_id_to from brain_synapse where brain_synapse_id=%lu")
								, iSynapse )
			&& result )
		{
			//INDEX iSource = atoi( result[1] );
			//INDEX iDestination = atoi( result[2] );

			// linkage is recovered from outside sources....
			Gain.set( atof( result[0] ) );
			//if( iSource != Source->Save( odbc, iParent ) )
			{
				//UnlinkSynapseFrom();
				//this will leave synapses connected in indexes
				// that the board may care about?
				// or just linking to a neuron is enough withou
				// minding direction anymore?
				//LinkSynapseFrom( this, FindNeuronByID( iSource ) );
			}
			//if( iDestination != Destination->Save( odbc, iParent ) )
			{
            //UnlinkSynapseTo();
            //LinkSynapseTo( this, FindNeuronByID( iDestination ) );
			}
			PopODBCEx( odbc );
	        return TRUE;
		}
		PopODBCEx( odbc );
		return FALSE;
	}
}

