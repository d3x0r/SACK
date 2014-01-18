#include "neuron.h"

#include "brain.hpp"
#include <sharemem.h>
#undef new
#undef delete

void NEURON::Detach( class SYNAPSE *synapse )
{
	synapse->DetachDestination();
	synapse->DetachSource();
}

class SYNAPSE **NEURON::AttachSynapse( INDEX position )
{
	if( Nerve[position] )
		return NULL;
	return Nerve + position;
}

class SYNAPSE **NEURON::AttachSynapse( void )
{
	class SYNAPSE **result;

	for( INDEX n = 0; 
		  !(result = AttachSynapse( n ) )
		  && n < MAX_NERVES; 
		  n++ );
	return result;
}

void *NEURON::operator new(size_t sz, PNEURONSET *set )
{
	if( sz != sizeof( class NEURON ) )
		return NULL;
	PNEURON neuron = GetFromSet( NEURON, set );
	MemSet( neuron, 0, sizeof( NEURON ) );
	neuron->pool = set;
	return neuron;
}

void *NEURON::operator new( size_t sz )
{
   return NULL;
}


#ifdef _MSC_VER
void NEURON::operator delete( void *ptr, struct NEURONset_tag **set )
{
   DeleteFromSet( NEURON, set, ptr );
};
#endif

void NEURON::operator delete( void *ptr )
{
   DeleteFromSet( NEURON, ((PNEURON)ptr)->pool, ptr );
}

void NEURON::Init( CTEXTSTR name )
{
	if( !this )
      return;
	if( name )
	{
		Name = (TEXTSTR )Allocate( strlen( name ) + 1 );
		strcpy( Name, name );
	}
	else
	{
		static int nDefault = 1;
		TEXTCHAR tmpname[32];
		Name = NewArray( TEXTCHAR, snprintf( tmpname, sizeof( tmpname ), WIDE("Neuron %d"), nDefault ) + 1);
		nDefault++;
		strcpy( Name, tmpname );
	}
   nType = NT_ANALOG;
   Threshold.set( VAL_NATIVE, (NATIVE)0 );
   Max.set( VAL_NATIVE, (NATIVE)256 );
   Min.set( VAL_NATIVE, (NATIVE)0 );
   Input.set( VAL_NATIVE, (NATIVE)0 );
   Output.set( VAL_NATIVE, (NATIVE)0 );
}

//----------------------------------------------------------------------

NEURON::NEURON( CTEXTSTR name )
{
	Init( name );
}

//----------------------------------------------------------------------

NEURON::NEURON( void )
{
	Init( NULL );
}

//----------------------------------------------------------------------

NEURON::NEURON( PNEURON default_neuron )
{
	Init( NULL );
	nType  = default_neuron->nType;
	Input = default_neuron->Input;
	Output = default_neuron->Output;
	Threshold = default_neuron->Threshold;
	Min = default_neuron->Min;
	Max = default_neuron->Max;
}

//----------------------------------------------------------------------

NEURON::NEURON( int newType, connector *value )
{
	Init( NULL );
	switch( nType = newType )
	{
	case NT_INPUT:
		//this->pbs = value->pbs;
		io_node = value;
		// this is an equate/clone... it makes an indirect to the output for the neuron
		Input.set( VAL_PANYVALUE, value );
		break;
	case NT_OUTPUT:
		//this->pbs = pbs;
		io_node = value;
		// this is an equate/clone... it makes an indirect to the output for the neuron
		Output.set( VAL_PANYVALUE, value );
		// this actually sets the value; initialize to 0
		Output.set( (NATIVE)0 );
		break;
	}
}

//----------------------------------------------------------------------

NEURON::~NEURON( void )
{
	Release( Name );
}

//----------------------------------------------------------------------

NATIVE NEURON::get( PNATIVE input, PNATIVE base, PNATIVE range, PNATIVE threshold, PNATIVE level )
{
	if( input )
	{
      NATIVE tmp;
		*input = Input.get();
		if( *input > ( tmp = Max.get() ) )
			*input = tmp;
		if( *input < ( tmp = Min.get() ) )
         *input = tmp;
	}
   if( base )
		*base = ((NATIVE)-256);
	if( range )
      *range = ((NATIVE)512);
		//*range = Max.get() - Min.get();
   if( threshold )
		*threshold = Threshold.get();
   if( level )
		*level = Output.get() - Min.get();
   return Output.get();
}

//----------------------------------------------------------------------

NATIVE NEURON::get( PNATIVE base, PNATIVE range, PNATIVE level )
{
   return get( (PNATIVE)NULL, base, range, (PNATIVE)NULL, level );
}

//----------------------------------------------------------------------

void NEURON::set( NATIVE threshold )
{
   Threshold.set( threshold );
}

//----------------------------------------------------------------------

CTEXTSTR NEURON::name( void )
{ return Name; }

//----------------------------------------------------------------------

int NEURON::type( void )
{
	return nType;
}

//----------------------------------------------------------------------

void NEURON::type( int type )
{
	nType = type;
}

//----------------------------------------------------------------------

void NEURON::set( NATIVE base, NATIVE range, NATIVE threshold )
{
	Threshold.set( threshold );
	Min.set( base );
	Max.set( range+base );
}

//----------------------------------------------------------------------

NATIVE NEURON::Collect( _32 cycle )
{
	int bAdd = FALSE;
	if( ( nCycle != cycle ) && ( nType != NT_FREE ) ) // from brain...
	{
		nCycle = cycle;
		//lprintf( WIDE("collect for %s %p"), Name, this );
		if( nType == NT_INPUT )
		{
			//lprintf(WIDE("Input : ") NATIVE_FORMAT WIDE("\n"), Input.get() );
			Output.set( Input.get() );
		}
		else
		{
			//lprintf( WIDE("Gathering inputs...") );
			Input.set(NATIVE(0));

			bAdd = TRUE;
			for( int d = 0; d < MAX_NERVES; d++ )
			{
				PSYNAPSE ps;
				ps = Nerve[d];
				if( ps &&
					ps->DestinationIs(this) ) // ending on this neuron...
				{
					// synapse timing....
					if( ps->SourceIs(NULL) )
						DebugBreak(); // incomplete synaptical linkage...
					else
					{
                  //lprintf( WIDE("Gather from chain...") );
						//lprintf( WIDE("Collecting from an attached neuron. %g + %g = %g")
						//		 , Input.get()
                  //       , ps->Collect( cycle )
						//		 , Input.get() + ps->Collect( cycle ) );
						Input.set( Input.get() + ps->Collect( cycle ) );
					}
				}
			}
			// after sum of all inputs has been gathered
			// produce the output value.
			Emit();
		}
	}
	return Output.get();
}

void NEURON::Emit( void )
{
   //lprintf( WIDE("Emitting a value... %d"), nType );
    switch( nType )
    {
    case NT_OUTPUT:
        {
           Output.set( Input.get() );
           //lprintf( WIDE("emit value into output ") NATIVE_FORMAT, Input.get() );
        }
        break;
	 case NT_DIGITAL:
		 //lprintf( WIDE("emit digital %g %g %g %g"), Threshold.get(), Input.get(), Min.get(), Max.get() );
		if( Input.get() <= Threshold.get() )
			Output.set( Min.get() );
		else
			Output.set( Max.get() );
		break;
	 case NT_ANALOG:
		 {
			 NATIVE tmp;
			 NATIVE n = Input.get() - Threshold.get();
			 //lprintf( WIDE("emit analog") );

			 if( n < (tmp=Min.get()) )
				 Output.set( tmp );
			 else if( n > (tmp=Max.get()) )
				 Output.set( tmp );
			 else
				 Output.set( n );
		 }
		 break;
	 case NT_SIGMOID:
		 //         pn->nOutput = (int)(MAX_OUTPUT_VALUE/(1+exp( -k * (pn->nInput-pn->nThreshold))));
        //         if( pn->nOutput <= 0 )
        //            pn->nOutput = 0;  // trim bottom portion...
        //         else 
        //            if( pn->nOutput > MAX_OUTPUT_VALUE )
        //               pn->nOutput = MAX_OUTPUT_VALUE; 
        break;
    }
}

void NEURON::SaveBegin( PODBC odbc )
{
	if( iNeuron && iNeuron != INVALID_INDEX  )
	{
		SQLCommandf( odbc, WIDE("delete from brain_neuron where brain_neuron_id=%lu"), iNeuron );
	}
	iNeuron = 0;
}

INDEX NEURON::Save( PODBC odbc, INDEX iParent )
{
	NEURON nout;
	_32 dwW;
	int nSaveType;
	if( !iNeuron )
	{
		TEXTCHAR szThreshold[32];
		snprintf( szThreshold, sizeof( szThreshold ), WIDE("%g"), Threshold.get() );
		if( SQLInsert( odbc, WIDE("brain_neuron")
						 , WIDE("threshold"), 0, szThreshold
						 , WIDE("parent_id"), 2, iParent
						 , WIDE("type"), 2, nType
						 , WIDE("min_output"), 0, WIDE("0")
						 , WIDE("max_output"), 0, WIDE("0")
						 , NULL, 0, NULL ) )
		{
			iNeuron = FetchLastInsertID( odbc, NULL, NULL );
			switch( nType )
			{
			case NT_INPUT:
				//io_node->Save( odbc, pbs->fullname(), iParent, TRUE );
				break;
			case NT_OUTPUT:
				//((connector*)Output)
				//SQLCommandf( odbc, "delete from brain_connectors where parent_id=%d", iNeuron );
				//io_node->Save( odbc, pbs->fullname(), iParent, FALSE );
				//((connector*)Output)
				break;
			}
		}
	}
	else
	{
      //  lprintf( "What about updates? and we don't want to SAVE all the time... neurons can octuple load this" );
		//SQLCommandf( odbc, "update brain_neuron set threshold=%g where brain_neuron_id=%lu", Threshold.get(), iNeuron );
	}
	return iNeuron;
}

LOGICAL NEURON::Load( PODBC odbc, INDEX iInstance )
{
	CTEXTSTR *result;
	if( SQLRecordQueryf( odbc, NULL, &result, NULL, WIDE("select threshold,type,min_output,max_output,brain_neuron_id from brain_neuron where brain_neuron_id=%lu")
		, iInstance )
		&& result )
	{
		INDEX neuron_idx = atoi( result[2] );
		Threshold.set( atof( result[0] ) );
		iNeuron = neuron_idx;
		int load_type = atoi(result[1]);
		switch( load_type )
		{
		case 0:
			load_type = NT_ANALOG;
			break;
		case NT_INPUT:
			break;
		case NT_OUTPUT:
			break;
		}
		type(load_type);
		//min.set( atof( result[0] ) );
		//max.set( atof( result[0] ) );
		SQLEndQuery( odbc );
		return TRUE;
	}
	return FALSE;	
}

