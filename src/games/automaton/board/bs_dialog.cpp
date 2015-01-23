#define DEFINE_DEFAULT_IMAGE_INTERFACE
#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <controls.h>
#include <timers.h>
#include "bs_dialog.h"
#include "../brain/brain.hpp"

#define TEXT_VALUE1 1000
#define TEXT_VALUE2 1001

typedef struct neuron_dialog_data_tag
{
	PCOMMON frame;
	PNEURON neuron;
	PSYNAPSE synapse;
} DIALOG_DATA, *PDIALOG_DATA;

typedef struct local_tag
{
	struct { 
		BIT_FIELD bInit : 1;
	} flags;
	CRITICALSECTION cs;
	PLIST add_neurons; // PNEURON
	PLIST done_neurons; // PDIALOG_DATA
	PLIST neurons; // PDIALOG_DATA
	PLIST add_synapses; // PNEURON
	PLIST done_synapses; // PDIALOG_DATA
	PLIST synapses; // PDIALOG_DATA
	_32 neuron_timer;
	PTHREAD prop_thread;
} LOCAL;

static LOCAL l;

int ActiveNeurons( void )
{
	INDEX idx;
	int cnt = 0;
	PTRSZVAL psv;
	LIST_FORALL( l.neurons, idx, PTRSZVAL, psv )
		cnt++;
   return cnt;
}

int ActiveSynapses( void )
{
	INDEX idx;
	int cnt = 0;
	PTRSZVAL psv;
	LIST_FORALL( l.synapses, idx, PTRSZVAL, psv )
		cnt++;
	return cnt;
}

void CPROC DonePushed( PTRSZVAL psv, PCOMMON pc )
{
	PDIALOG_DATA pndd = (PDIALOG_DATA)psv;
	EnterCriticalSec( &l.cs );
	if( pndd->neuron )
		AddLink( &l.done_neurons, pndd );
	else
		AddLink( &l.done_synapses, pndd );
	WakeThread( l.prop_thread );
	LeaveCriticalSec( &l.cs );
}

void CPROC RefreshFrame( PTRSZVAL psv )
{
	INDEX idx;
	PDIALOG_DATA pndd;



	EnterCriticalSec( &l.cs );
	LIST_FORALL( l.neurons, idx, PDIALOG_DATA, pndd )
	{
		TEXTCHAR number[18];
		NATIVE output;
		output = pndd->neuron->get();// &base, &range, &level );
		snprintf( number, sizeof( number ), WIDE("%g"), output );
		SetCommonText( GetControl( pndd->frame, TEXT_VALUE1 ), number );
		snprintf( number, sizeof( number ), WIDE("%g"), pndd->neuron->threshold() );
		SetCommonText( GetControl( pndd->frame, TEXT_VALUE2 ), number );
	}
	LIST_FORALL( l.synapses, idx, PDIALOG_DATA, pndd )
	{
		TEXTCHAR number[18];
		NATIVE gain;
		pndd->synapse->get( &gain, NULL, NULL );// &base, &range, &level );
		snprintf( number, sizeof( number ), WIDE("%g"), gain );
		SetCommonText( GetControl( pndd->frame, TEXT_VALUE1 ), number );
	}
	LeaveCriticalSec( &l.cs );
}

void CPROC SliderChanged( PTRSZVAL psv, PCOMMON pc, int val )
{
	PDIALOG_DATA pndd = (PDIALOG_DATA)psv;
	if( pndd->neuron )
		pndd->neuron->set( (NATIVE)val );
	if( pndd->synapse )
		pndd->synapse->set( (NATIVE)val );
	return;
}

void CPROC SetNeuronTypeButton( PTRSZVAL psv, PSI_CONTROL pc )
{
	PSI_CONTROL frame = GetFrame( pc );
	PDIALOG_DATA pndd = (PDIALOG_DATA)GetCommonUserData( frame );
	pndd->neuron->type( psv );
}

static void Init( void )
{
	if( !l.flags.bInit )
	{
		InitializeCriticalSec( &l.cs );
		l.flags.bInit = 1;
	}
}

static PTRSZVAL CPROC _ShowPropertyDialog( PTHREAD thread )
{
	PNEURON neuron;
	PSYNAPSE synapse;
	PDIALOG_DATA pndd;
	TEXTCHAR title[64];
	INDEX idx;
	Init();
	EnterCriticalSec( &l.cs );
	l.neuron_timer = AddTimer( 166, RefreshFrame, 0 );
	do
	{
		S_32 x, y;
		GetMouseState( &x, &y, NULL );
		LIST_FORALL( l.add_neurons, idx, PNEURON, neuron )
		{
			// remove from list of neurons to add
			SetLink( &l.add_neurons, idx, 0 );
			pndd = (PDIALOG_DATA)Allocate( sizeof( DIALOG_DATA ) );
			AddLink( &l.neurons, pndd );
			pndd->neuron = neuron;
			pndd->synapse = NULL;
			snprintf( title, sizeof( title ), WIDE("%s Properties"), neuron->name() );
			pndd->frame = CreateFrame( title, x, y, 256, 128, BORDER_NORMAL, NULL );
			SetCommonUserData( pndd->frame, (PTRSZVAL)pndd );
			MakeCaptionedControl( pndd->frame, STATIC_TEXT, 5, 33, 73, 18, TEXT_VALUE1, WIDE("00") );
			MakeCaptionedControl( pndd->frame, STATIC_TEXT, 5, 56, 73, 18, TEXT_VALUE2, WIDE("00") );
			SetSliderValues( MakeSlider( pndd->frame, 5, 5, 246, 24, -1
												, SLIDER_HORIZ
												, SliderChanged, (PTRSZVAL)pndd
												)
								, -256, (int)pndd->neuron->threshold(), 256 );

			{
				PSI_CONTROL pc;
				pc = MakeRadioButton( pndd->frame,  83, 33, 73, 18, -1, WIDE("Digital"), 1, 1, SetNeuronTypeButton, NT_DIGITAL );
				if( pndd->neuron->type() == NT_DIGITAL )
					SetCheckState( pc, TRUE );
				pc = MakeRadioButton( pndd->frame,  83, 46, 73, 18, -1, WIDE("Analog"), 1, 1, SetNeuronTypeButton, NT_ANALOG );
				if( pndd->neuron->type() == NT_ANALOG )
					SetCheckState( pc, TRUE );
				pc = MakeRadioButton( pndd->frame,  83, 59, 73, 18, -1, WIDE("Sigmoid"), 1, 1, SetNeuronTypeButton, NT_SIGMOID );
				if( pndd->neuron->type() == NT_SIGMOID )
					SetCheckState( pc, TRUE );
			}
			MakeButton( pndd->frame, 170, 46, 80, 20, IDOK, WIDE("Delete")
									  , 0, NULL, 0 );
			MakeButton( pndd->frame, 170, 71, 80, 20, IDOK, WIDE("Done")
						 , 0, DonePushed, (PTRSZVAL)pndd );
			DisplayFrame( pndd->frame );
		}
		LIST_FORALL( l.add_synapses, idx, PSYNAPSE, synapse )
		{
			// remove from list of synapses to add
			SetLink( &l.add_synapses, idx, 0 );
			pndd = (PDIALOG_DATA)Allocate( sizeof( DIALOG_DATA ) );
			AddLink( &l.synapses, pndd );
			pndd->neuron = NULL;
			pndd->synapse = synapse;
			snprintf( title, sizeof( title ), WIDE("%s Properties"), synapse->name() );
			pndd->frame = CreateFrame( title, x, y, 256, 128, BORDER_NORMAL, NULL );
			MakeCaptionedControl( pndd->frame, STATIC_TEXT, 5, 33, 73, 18, TEXT_VALUE1, WIDE("00") );
			SetSliderValues( MakeSlider( pndd->frame, 5, 5, 246, 24, -1
												, SLIDER_HORIZ
												, SliderChanged, (PTRSZVAL)pndd
												)
								, -256, (int)pndd->synapse->gain(), 256 );

			//MakeCaptionedControl( pndd->frame, RADIO_BUTTON,  83, 33, 73, 18, -1, WIDE("Digital"), 0, 1, NULL, 0 );
			//MakeCaptionedControl( pndd->frame, RADIO_BUTTON,  83, 46, 73, 18, -1, WIDE("Analog"), 0, 1, NULL, 0 );
			//MakeCaptionedControl( pndd->frame, RADIO_BUTTON,  83, 59, 73, 18, -1, WIDE("Sigmoid"), 0, 1, NULL, 0 );

			MakeButton( pndd->frame, 170, 46, 80, 20, IDOK, WIDE("Delete")
						 , 0, NULL, 0 );
			MakeButton( pndd->frame, 170, 71, 80, 20, IDOK, WIDE("Done")
						 ,0 , DonePushed, (PTRSZVAL)pndd );
			//SetCommonUserData( pndd->frame, pndd );

			DisplayFrame( pndd->frame );
		}
		LeaveCriticalSec( &l.cs );
		WakeableSleep( SLEEP_FOREVER );
		EnterCriticalSec( &l.cs );
		LIST_FORALL( l.done_neurons, idx, PDIALOG_DATA, pndd )
		{
			SetLink( &l.done_neurons, idx, 0 );
			DeleteLink( &l.neurons, pndd );
			DestroyCommon( &pndd->frame );
			Release( pndd );
		}
		LIST_FORALL( l.done_synapses, idx, PDIALOG_DATA, pndd )
		{
			SetLink( &l.done_synapses, idx, 0 );
			DeleteLink( &l.synapses, pndd );
			DestroyCommon( &pndd->frame );
			Release( pndd );
		}

	} while( ActiveNeurons() || ActiveSynapses() );
	RemoveTimer( l.neuron_timer );
	l.prop_thread = 0;
	LeaveCriticalSec( &l.cs );
	return 0;
}

int CPROC ActiveNeuron( PNEURON neuron )
{
	INDEX idx;
	int cnt = 0;
   PTRSZVAL psv;
	LIST_FORALL( l.neurons, idx, PTRSZVAL, psv )
		if( neuron == (PNEURON)psv )
			return TRUE;
	LIST_FORALL( l.add_neurons, idx, PTRSZVAL, psv )
		if( neuron == (PNEURON)psv )
			return TRUE;
	LIST_FORALL( l.done_neurons, idx, PTRSZVAL, psv )
		if( neuron == (PNEURON)psv )
			return TRUE;
	return FALSE;
}

int CPROC ActiveSynapse( PSYNAPSE synapse )
{
	INDEX idx;
	int cnt = 0;
	PTRSZVAL psv;
	LIST_FORALL( l.synapses, idx, PTRSZVAL, psv )
		if( synapse == (PSYNAPSE)psv )
			return TRUE;
	LIST_FORALL( l.add_synapses, idx, PTRSZVAL, psv )
		if( synapse == (PSYNAPSE)psv )
			return TRUE;
	LIST_FORALL( l.done_synapses, idx, PTRSZVAL, psv )
		if( synapse == (PSYNAPSE)psv )
			return TRUE;
	return FALSE;
}

void CPROC ShowNeuronDialog( PNEURON neuron )
{
	Init();
	EnterCriticalSec( &l.cs );
	if( !ActiveNeuron( neuron ) )
	{
		AddLink( &l.add_neurons, neuron );
		if( !ActiveNeurons() )
		{
			l.prop_thread = ThreadTo( _ShowPropertyDialog, 0 );
		}
		else
			WakeThread( l.prop_thread );
	}
	LeaveCriticalSec( &l.cs );
}

void CPROC ShowSynapseDialog( PSYNAPSE synapse )
{
	Init();
	EnterCriticalSec( &l.cs );
	if( !ActiveSynapse( synapse ) )
	{
		AddLink( &l.add_synapses, synapse );
		if( !ActiveSynapses() )
		{
			l.prop_thread = ThreadTo( _ShowPropertyDialog, 0 );
		}
		else
			WakeThread( l.prop_thread );
	}
	LeaveCriticalSec( &l.cs );
}





