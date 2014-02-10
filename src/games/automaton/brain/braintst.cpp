#include <stdhdrs.h>
#include <procreg.h>
#include <configscript.h>
#include "brain.hpp"


int main( void )
{
   DumpRegisteredNames();
	{
   //SetAllocateLogging( TRUE );
	PBRAIN_INTERFACE piBrain = (PBRAIN_INTERFACE)GetRegisteredInterface( "brain" );
	PBRAIN pBrain = GET_PUBLIC_DATA( "brain", BRAIN, NULL );
	ANYVALUE a(VAL_NATIVE),b(VAL_NATIVE),c(VAL_NATIVE);
	a.set( 0.001f );
	b.set( 0.0001f );
	c.set( 4.26f );
      printf( "Uhmm values are... %g %g %g\n\n", a.get(), b.get(), c.get() );

	PBRAIN_STEM pbs;
	va_args args;
	init_args( args );
#define AddComp(type,var,name) PushArgument( args, char *, name ); \
	PushArgument( args, PANYVALUE, var );  \
	PushArgument( args, _32, type );

	AddComp( INPUT, &a, "a value" );
	AddComp( INPUT, &b, "b value");
	AddComp( OUTPUT, &c, "c value" );

	pbs = piBrain->_AddComponent( pBrain, 3, "Radio"
										 , pass_args( args ) );
	piBrain->_AddBrainStem( pBrain, pbs );
	PSYNAPSE ps = piBrain->_GetSynapse( pBrain );
	PNEURON pn1 = piBrain->_GetInputNeuron( pBrain, pbs, 0 );
	PNEURON pn2 = piBrain->_GetOutputNeuron( pBrain, pbs, 0 );

  piBrain->_LinkSynapseFrom( pBrain, ps, pn1 );
	piBrain->_LinkSynapseTo( pBrain, ps, pn2 );

	ps = piBrain->_GetSynapse( pBrain );
	pn1 = piBrain->_GetInputNeuron( pBrain, pbs, 1 );
	pn2 = piBrain->_GetOutputNeuron( pBrain, pbs, 0 );

	PNEURON n1 = piBrain->_GetNeuron( pBrain );
	PNEURON n2 = piBrain->_GetNeuron( pBrain );
	ps = piBrain->_GetSynapse( pBrain );
   piBrain->_LinkSynapseFrom( pBrain, ps, n1 );
	piBrain->_LinkSynapseTo( pBrain, ps, n2 );

	ps = piBrain->_GetSynapse( pBrain );
   piBrain->_LinkSynapseFrom( pBrain, ps, n2 );
   piBrain->_LinkSynapseTo( pBrain, ps, n1 );

	ps = piBrain->_GetSynapse( pBrain );
   piBrain->_LinkSynapseFrom( pBrain, ps, pn1 );
   piBrain->_LinkSynapseTo( pBrain, ps, n2 );

	ps = piBrain->_GetSynapse( pBrain );
   piBrain->_LinkSynapseFrom( pBrain, ps, n1 );
   piBrain->_LinkSynapseTo( pBrain, ps, pn2 );




	while( 1 )
	{
      printf( "Uhmm values are... %g %g %g\r", a.get(), b.get(), c.get() );
	}
	}
}
