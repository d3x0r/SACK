#define BRAINSHELL_SOURCE
#define BUILD_TEST_SHELL

// this is the library for the application brainboard to use....
// since the board itself knows nothing about neurons and brains we
// have to define the peices to load, and the methods to do upon
// connection/interaction with other peices...
// 
#include <stdhdrs.h>

#include <configscript.h>
#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <controls.h>

#include "board.hpp"
#include "toolbin.hpp"
#include "../brain/brain.hpp"
#include "bs_dialog.h"
#include "brainshell.hpp"

#define MNU_ADDCOMPONENT 2048
#define MNU_MAXCOMPONENT MNU_ADDCOMPONENT + 256

#define MNU_SIGMOID    1021

#define MNU_ZOOM       1010 // 0, 1, 2 used...
#define MNU_ENDZOOM    1020

#define MNU_CLOSE      1030
#define MNU_SYNAPSE    1009 
#define MNU_NEURON     1008
#define MNU_PROPERTIES 1007
#define MNU_DELETE     1006
#define MNU_LOAD       1005
#define MNU_SAVE       1004        
#define MNU_RESET      1003               
#define MNU_RUN        1002
#define MNU_ADDNEURON  1001

#define MNU_ADD_INPUT_START 5000
#define MNU_ADD_OUTPUT_START 6000
#define MNU_ADD_OUTPUT_LAST  6999

//#include WIDE("brainshell.hpp")

//typedef class local_tag BRAINBOARD;

typedef struct output_input_type {
	struct {
		_32 bOutput : 1; // else is an input...
	} flags;
	class BRAINBOARD *brainboard;
	//PBRAIN_STEM pbs;
	PBRAIN brain;
	connector *conn;
	PNEURON neuron; // this is what we really need... this particular peice's neural peice 
} *POUTPUT_INPUT;
typedef struct output_input_type OUTPUT_INPUT;

class BOARDPEICE:public IPEICE
{
	class IPEICE *ipeice;
	class BRAINBOARD *brainboard;
};


//static class BRAINBOARD *l;

class BRAINBOARD {
	struct {
		BIT_FIELD bOwnBrain : 1; // allocated its own brain...
	} flags;

	POINTER create_input_type,create_output_type;

	// these are the peice sets which will be loaded
	// from the file at the moemnt...
	PIPEICE NeuronPeice, InputPeice, OutputPeice, BackgroundPeice;
	PIVIA NervePeice;

	PBRAIN brain;
	PSYNAPSE DefaultSynapse;
	PNEURON DefaultNeuron;
	PMENU hMenu, hMenuComponents;
	// although a PIVIA is-a PIPEICE, a via type peice is required
	// for certain operations such as beginpath.
	// this also results in certain interactions between peice instances
	// (peice methods such as connect, disconnect are performed)

	class BACKGROUND_METHODS *background_methods; // these are created with board ID
	class NEURON_METHODS *neuron_methods;
	class INPUT_METHODS *input_methods;
	class OUTPUT_METHODS *output_methods;
	class NERVE_METHODS *nerve_methods;

	PLIST connectors;
	PLIST menus;
	friend class NERVE_METHODS;
	friend class OUTPUT_METHODS;
	friend class INPUT_METHODS;
	friend class NEURON_METHODS;
	friend class BACKGROUND_METHODS;
public:
	PIBOARD board;

	void Init( void );
	BRAINBOARD();
	BRAINBOARD(PBRAIN brain);
	~BRAINBOARD();

	PPEICE_METHODS FindPeiceMethods( TEXTCHAR *type );
	void InitMenus( void );
	void LoadPeices( TEXTCHAR *file );

	void SetCellSize( arg_list args );
	void DefinePeiceColors( arg_list args );
	void DefineABlock( arg_list args );
	void DefineABlockNoOpt( arg_list args );
	void RebuildComponentPopups( void );

};

class NERVE_METHODS:public VIA_METHODS
{
	BRAINBOARD * brainboard;
public:
	NERVE_METHODS( BRAINBOARD *newbrainboard ) { brainboard = newbrainboard; }
private:
	PSYNAPSE synapse;
public:
	PTRSZVAL Create(PTRSZVAL psvExtra )
	{
		return (PTRSZVAL)(brainboard->brain->DupSynapse( brainboard->DefaultSynapse ));
	}
	void Destroy( PTRSZVAL psv )
	{
		brainboard->brain->ReleaseSynapse( (PSYNAPSE)psv );
	}
	int Disconnect( PTRSZVAL psv )
	{
		brainboard->brain->UnLinkSynapseTo( (PSYNAPSE)psv );
		return TRUE;
	}
	int OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		ShowSynapseDialog( (PSYNAPSE)psv );
		return 1;
	}
//	PEICE_PROC( int, OnClick )( PTRSZVAL psv, int x, int y )
//	{
//		lprintf( WIDE("syn") );
 //     return 0;
 //  }
	void SaveBegin( PODBC odbc, PTRSZVAL psvInstance )
	{
		//SQLCommandf( odbc, WIDE("delete from board_layer_neuron where board_layer_id=%lu"), iParent );
		PSYNAPSE synapse = (PSYNAPSE)psvInstance;
		synapse->SaveBegin( odbc );
	}
	INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance )
	{
		//SQLCommandf( odbc, WIDE("delete from board_layer_synapse where board_layer_id=%lu"), iParent );
		//INDEX iBrainSynapse = 
		return ((PSYNAPSE)psvInstance)->Save( odbc, iParent );
		//SQLInsert( odbc, WIDE("board_layer_synapse")
		//			, WIDE("board_layer_id"), 2, iParent
		//			, WIDE("brain_synapse_id"), 2, iBrainSynapse
		//			, NULL, 0, NULL );
		//return FetchLastInsertID( odbc, NULL, NULL );
	}
	PTRSZVAL Load( PODBC odbc, INDEX iInstance )
	{
		PSYNAPSE synapse = brainboard->brain->GetSynapse();
		synapse->Load( odbc, 0, iInstance );
		return (PTRSZVAL)synapse;
	}
};

class INPUT_METHODS:public PEICE_METHODS
{
	BRAINBOARD * brainboard;
public:
	INPUT_METHODS( BRAINBOARD *newbrainboard ) { brainboard = newbrainboard; }
private:
	CDATA level_colors[3];
public:
	PTRSZVAL Create( PTRSZVAL psvExtra )
	{
		//brainboard->create_input_type = (POUTPUT_INPUT)psvExtra;
		//brainboard->create_input_type->flags.bOutput = 0;
		lprintf( WIDE("Creating a new input (peice instance)") );
		//this->brainboard->brain->GetInputNeuron( ((POUTPUT_INPUT)psvExtra)->pbs, ((POUTPUT_INPUT)psvExtra)->conn
		return (PTRSZVAL)((POUTPUT_INPUT)psvExtra); // still not the real create...  but this is psviNstance...
	}
	void SetColors( CDATA c1, CDATA c2, CDATA c3 )
	{
		level_colors[0] = c1;
		level_colors[1] = c2;
		level_colors[2] = c3;
	}
	void Draw( PTRSZVAL psvInstance, Image image, Image cell, S_32 x, S_32 y )
	{
		CDATA cPrimary;
		POUTPUT_INPUT input = (POUTPUT_INPUT)psvInstance;
		NATIVE value = input->conn->get();
		lprintf( WIDE("input value is %g"), value );
		if( value < 0 )
			cPrimary = ColorAverage( level_colors[1]
										  , level_colors[0]
											, (int)-(value * 1200), 1000 );
		else
			cPrimary = ColorAverage( level_colors[1]
											, level_colors[2]
									  , (int)(value*1200), 1000 );

		BlotImageShaded( image
		               , cell //master->getcell(cellx, celly)
		               , x, y
		               , cPrimary );
	}
	int ConnectEnd( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
		return FALSE;
	}
	int ConnectBegin( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
		int n;
		//if( peice_from == brainboard->NerveMethods )
		// maybe...
		PSYNAPSE synapse = (PSYNAPSE)psv_from_instance;
		POUTPUT_INPUT neuron = (POUTPUT_INPUT)psv_to_instance;
		// validate that peice_from is a nerve_method type
		for( n = 0; n < 8; n++ )
			if( DirDeltaMap[n].x == x && DirDeltaMap[n].y == y )
				break;
		if( n < 8 )
			return brainboard->brain->LinkSynapseFrom( synapse, neuron->neuron, n );
		return FALSE;
	}
	int OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
      //ShowInputDialog( (PNEURON)psv );
      return 1;
	}
	int OnClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		lprintf( WIDE("click on input! at %d,%d"), x, y );
		if( x == 0 && y == 0 )
		{
			// this is implied to be the current peice that
			// has been clicked on...
			// will receive further OnMove events...
			brainboard->board->LockPeiceDrag();
			return TRUE;
		}
		else
		{
			if( !brainboard->board->BeginPath( brainboard->NervePeice, (PTRSZVAL)brainboard ) )
			{
				// attempt to grab existing path...
				// current position, and current layer
				// will already be known by the grab path method
				//brainboard->board->GrabPath();
			}
		}
		// so far there's nothing on this cell to do....
		return FALSE;
	}
	INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psv )
	{
		POUTPUT_INPUT neuron = (POUTPUT_INPUT)psv;
		INDEX fake_parent_id = neuron->neuron->Save( odbc, iParent ); //io_node->Save( odbc, neuron->pbs->name(), iParent, TRUE );
		DoSQLCommandf( WIDE("insert into brain_connectors (connector_name,input,parent_id) values ('%s/%s',1,%d)")
			, neuron->conn->pbs->fullname()
			, neuron->conn->name()
			//, iParent 
			, fake_parent_id
			);
		return fake_parent_id;
	}
	PTRSZVAL Load( PODBC odbc, INDEX iInstance )
	{
		// result with psvInstance....
		//PNEURON neuron = (PNEURON)psv;
		CTEXTSTR *result;
		
		PushSQLQueryEx( odbc );
//		DebugBreak();
		if( SQLRecordQueryf( odbc, NULL, &result, NULL, WIDE("select connector_name from brain_connectors where parent_id=%ld")
								, iInstance )
			&& result ) 
		{
			POUTPUT_INPUT io_thing = New( OUTPUT_INPUT );
			io_thing->brainboard = brainboard;
			io_thing->brain = io_thing->brainboard->brain;
			//io_thing->pbs = io_thing->brain->GetBrainStem(result[1]);
			brainboard->brain->GetConnector( result[0], &io_thing->conn, 1 );  
			io_thing->neuron = io_thing->brain->GetInputNeuron(io_thing->conn );
			io_thing->neuron->Load( odbc, iInstance );
			//PNEURON neuron = brainboard->brain->GetOutputNeuron( name );
			//return neuron->Load( odbc, 0, iInstance, brainboard->brain );
			SQLEndQuery( odbc );
			return (PTRSZVAL)io_thing;
		}
		PopODBCEx( odbc );
		return 0;
	}
};
class OUTPUT_METHODS:public PEICE_METHODS
{
	BRAINBOARD * brainboard;
public:
	OUTPUT_METHODS( BRAINBOARD *newbrainboard ) { brainboard = newbrainboard; }
private:
	CDATA level_colors[3];
	PTRSZVAL Create( PTRSZVAL psvExtra )
	{
		//brainboard->create_output_type = (POUTPUT_INPUT)psv;
		//brainboard->create_output_type->flags.bOutput = 1;
        lprintf( WIDE("Creating a new output (peice instance)") );

		POUTPUT_INPUT poi = (POUTPUT_INPUT)psvExtra;

		poi->neuron = brainboard->brain->GetOutputNeuron( poi->conn );
		//DupNeuron( brainboard->DefaultNeuron ))

		return (PTRSZVAL)((POUTPUT_INPUT)psvExtra); // still not the real create...  but this is psviNstance...
		//return (PTRSZVAL)poi->neuron; // still not the real create...  but this is psviNstance...
		//return (PTRSZVAL)(((POUTPUT_INPUT)psvExtra)->conn); // still not the real create...  but this is psviNstance...
		//return (PTRSZVAL)(brainboard->create_output_type);
	}
	void Draw( PTRSZVAL psvInstance, Image image, Image cell, S_32 x, S_32 y )
	{
		CDATA cPrimary;
		PNEURON neuron = (PNEURON)psvInstance;
		POUTPUT_INPUT poi = (POUTPUT_INPUT)psvInstance;
		//PANYVALUE output = neuron->Output; //(connector*)psvInstance;
		NATIVE value = neuron->get(); //output->get();
		if( value < 0 )
			cPrimary = ColorAverage( level_colors[1]
										  , level_colors[0]
											, (int)-(value * 1200), 1000 );
		else
			cPrimary = ColorAverage( level_colors[1]
											, level_colors[2]
									  , (int)(value*1200), 1000 );

		BlotImageShaded( image
								  , cell //master->getcell(cellx, celly)
								  , x, y
								  , cPrimary );
	}
public:
	void SetColors( CDATA c1, CDATA c2, CDATA c3 )
	{
		level_colors[0] = c1;
		level_colors[1] = c2;
		level_colors[2] = c3;
	}
	int ConnectEnd( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
		int n;
		int success = FALSE;
		//POUTPUT_INPUT poi = (POUTPUT_INPUT)psv_to_instance;
		//POUTPUT_INPUT poi = (POUTPUT_INPUT);
		//connector *output = (connector*)psv_to_instance;
		//if( peice_from == brainboard->NerveMethods )
		// maybe...
		PSYNAPSE synapse = (PSYNAPSE)psv_from_instance;
		POUTPUT_INPUT neuron = (POUTPUT_INPUT)psv_to_instance;
		// validate that peice_from is a nerve_method type
		//poi->
		for( n = 0; n < 8; n++ )
			if( DirDeltaMap[n].x == x && DirDeltaMap[n].y == y )
				break;
		if( n < 8 )
		{
			int success = brainboard->brain->LinkSynapseTo( synapse, neuron->neuron, n );
			return success;
		}
		return FALSE;
	}
	int ConnectBegin( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
		return FALSE;
	}
	int OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		//ShowOutputDialog( (PNEURON)psv );
		return 1;
	}
	int OnClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		lprintf( WIDE("click on neuron! at %d,%d"), x, y );
		if( x == 0 && y == 0 )
		{
			// this is implied to be the current peice that
			// has been clicked on...
			// will receive further OnMove events...
			brainboard->board->LockPeiceDrag();
			return TRUE;
		}
		else
		{
			if( !brainboard->board->BeginPath( brainboard->NervePeice, (PTRSZVAL)brainboard ) )
			{
				// attempt to grab existing path...
				// current position, and current layer
				// will already be known by the grab path method
				//brainboard->board->GrabPath();
			}
		}
		// so far there's nothing on this cell to do....
		return FALSE;
	}
	INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psv )
	{
		POUTPUT_INPUT neuron = (POUTPUT_INPUT)psv;
		INDEX fake_parent_id = neuron->neuron->Save( odbc, iParent ); //io_node->Save( odbc, neuron->pbs->name(), iParent, TRUE );
		DoSQLCommandf( WIDE("insert into brain_connectors (connector_name,input,parent_id) values ('%s/%s',0,%d)")
			, neuron->conn->pbs->fullname()
			, neuron->conn->name()
			//, iParent 
			, fake_parent_id
			);
		return fake_parent_id;
	}
	PTRSZVAL Load( PODBC odbc, INDEX iInstance )
	{
		// result with psvInstance....
		//PNEURON neuron = (PNEURON)psv;
		CTEXTSTR *result;
		if( SQLRecordQueryf( odbc, NULL, &result, NULL, WIDE("select connector_name from brain_connectors where parent_id=%ld")
								, iInstance )
			&& result ) 
		{
			POUTPUT_INPUT io_thing = New( OUTPUT_INPUT );
			io_thing->brainboard = brainboard;
			io_thing->brain = io_thing->brainboard->brain;
			//io_thing->pbs = io_thing->brain->GetBrainStem(result[1]);
			brainboard->brain->GetConnector( result[0], &io_thing->conn, 0 );  
			io_thing->neuron = io_thing->brain->GetOutputNeuron(io_thing->conn );
			io_thing->neuron->Load( odbc, iInstance );
			//PNEURON neuron = brainboard->brain->GetOutputNeuron( name );
			//return neuron->Load( odbc, 0, iInstance, brainboard->brain );
			SQLEndQuery( odbc );
			return (PTRSZVAL)io_thing;
		}
		return 0;
	}
};

class NEURON_METHODS:public PEICE_METHODS
{
	// these methods are passed a psvInstance
	// which is the current neuron instance these are to wokr on
	// this valud is retrieved and stored (by other portions) by the create() method
	BRAINBOARD * brainboard;
public:
	NEURON_METHODS( BRAINBOARD *newbrainboard ) { brainboard = newbrainboard; }
private:
	CDATA c_input[3]; // 0=min,1=mid,2=max
	CDATA c_threshold[3]; // 0=min,1=mid,2=max
	PLAYER connected[8];
public:
	void SetColors( int bInput, CDATA c1, CDATA c2, CDATA c3 )
	{
		if( bInput )
		{
			c_input[0] = c1;
			c_input[1] = c2;
			c_input[2] = c3;
		}
      else
		{
			c_threshold[0] = c1;
			c_threshold[1] = c2;
			c_threshold[2] = c3;
		}
	}
	PTRSZVAL Create( PTRSZVAL psvExtra )
	{
		lprintf( WIDE("Creating a new neuron (peice instance)") );
		return (PTRSZVAL)(brainboard->brain->DupNeuron( brainboard->DefaultNeuron ));
	}
	void Destroy( PTRSZVAL psv )
	{
      brainboard->brain->ReleaseNeuron( (PNEURON)psv );
	}
	
	void Draw( PTRSZVAL psvInstance, Image image, Image cell, S_32 x, S_32 y )
	{
      //lprintf( WIDE("---------- DRAW NEURON ------------") );

		CDATA cPrimary, cSecondary, cTertiary;
      NATIVE base,range,value,input,threshold;
		PNEURON neuron = (PNEURON)psvInstance;

		neuron->get( &input, &base, &range, &threshold, &value );

		//     |(base)   --> range
		//        0 // 0 origin may not be the center of a neuron...
		// operational parameters of neurons may be done in such a way
      // that they are centered around some arbitrary number other than 0

		// base -100 range 500
		// center is 200
		// threshold of 100 is
      //    100-(-100) (200/range)

		threshold = (2*(threshold - base)) / range;
		threshold -= 1.0;
		// threshold is now -1.0 to 1.0 biased.

		value = ( 2 * ( value - base ) ) / range;
		value -= 1.0;
		// value is now -1.0 to 1.0 biased.

		if( value > 1.0 )
			value = 1.0;
		else if( value < -1.0 )
			value = -1.0;

		if( input < 0 )
			cPrimary = ColorAverage( c_input[1]
										  , c_input[0]
											, (int)-(value * 1000), 1000 );
		else
			cPrimary = ColorAverage( c_input[1]
											, c_input[2]
									  , (int)(value*1000), 1000 );

		if( threshold )
			cSecondary = ColorAverage( c_threshold[1],
									  c_threshold[0],
										 -(int)(threshold*1000), 1000 );
		else
			cSecondary = ColorAverage( c_threshold[1],
										 c_threshold[2],
										 (int)(threshold*1000), 1000 );
		cTertiary = 0; // no other value...

		BlotImageMultiShaded( image
								  , cell //master->getcell(cellx, celly)
								  , x, y
								  , cTertiary, cSecondary, cPrimary );
	}

	void Update( PTRSZVAL psv, _32 cycle )
	{
		lprintf( WIDE("updating color information for a neuron...") );
	}

	int ConnectEnd( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
		int n;
		//if( peice_from == brainboard->NerveMethods )
		// maybe...
		PSYNAPSE synapse = (PSYNAPSE)psv_from_instance;
		PNEURON neuron = (PNEURON)psv_to_instance;
		// validate that peice_from is a nerve_method type
		for( n = 0; n < 8; n++ )
			if( DirDeltaMap[n].x == x && DirDeltaMap[n].y == y )
				break;
		if( n < 8 )
		{
			int success = brainboard->brain->LinkSynapseTo( synapse, neuron, n );
			return success;
		}
		return FALSE;
	}

	int ConnectBegin( PTRSZVAL psv_to_instance, S_32 x, S_32 y
									  , PIPEICE peice_from, PTRSZVAL psv_from_instance )
	{
		int n;
		//if( peice_from == brainboard->NerveMethods )
		// maybe...
		PSYNAPSE synapse = (PSYNAPSE)psv_from_instance;
		PNEURON neuron = (PNEURON)psv_to_instance;
		// validate that peice_from is a nerve_method type
		for( n = 0; n < 8; n++ )
			if( DirDeltaMap[n].x == x && DirDeltaMap[n].y == y )
				break;
		if( n < 8 )
			return brainboard->brain->LinkSynapseFrom( synapse, neuron, n );
		return FALSE;
	}
	int OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		ShowNeuronDialog( (PNEURON)psv );
		return 1;
	}

	int OnClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		lprintf( WIDE("click on neuron! at %d,%d"), x, y );
		if( x == 0 && y == 0 )
		{
			// this is implied to be the current peice that
			// has been clicked on...
			// will receive further OnMove events...
			brainboard->board->LockPeiceDrag();
			return TRUE;
		}
		else
		{
			if( !brainboard->board->BeginPath( brainboard->NervePeice, (PTRSZVAL)brainboard ) )
			{
				// attempt to grab existing path...
				// current position, and current layer
				// will already be known by the grab path method
				//brainboard->board->GrabPath();
			}
		}
		// so far there's nothing on this cell to do....
		return FALSE;
	}
	void SaveBegin( PODBC odbc, PTRSZVAL psvInstance )
	{
		//SQLCommandf( odbc, WIDE("delete from board_layer_neuron where board_layer_id=%lu"), iParent );
		PNEURON neuron = (PNEURON)psvInstance;
		neuron->SaveBegin( odbc );
	}

	INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance )
	{
		PNEURON neuron = (PNEURON)psvInstance;
		return neuron->Save( odbc, iParent );
	}
	PTRSZVAL Load( PODBC odbc, INDEX iInstance )
	{
		PNEURON neuron = brainboard->brain->GetNeuron();
		neuron->Load( odbc, iInstance );
		return (PTRSZVAL)neuron;
	}
};

							//------------------------------------------
/*
static LOGICAL SelectNewFile( HWND hParent, PSTR szFile )
{
   
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
   szFile[0] = 0;
   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = WIDE("Bodies\0*.Body\0");
   ofn.nFilterIndex = 1;
   ofn.Flags = OFN_NOTESTFILECREATE
              | OFN_NOREADONLYRETURN ;// Display the Open dialog box. 

   return GetOpenFileName(&ofn);
}
*/


void CPROC CreateNewBoardName( PTRSZVAL psv, PSI_CONTROL button )
{
	TEXTCHAR *newname = (TEXTCHAR*)psv;
	SimpleUserQuery( newname, 256, WIDE("Enter a new board name"), button );
}

void CPROC SelectedItem( PTRSZVAL psv, PSI_CONTROL list, PLISTITEM item )
{
	TEXTCHAR *newname = (TEXTCHAR*)psv;
	sack::PSI::listbox::GetItemText( item, 256, newname );
}

void CPROC SelectedItemLoad( PTRSZVAL psv, PSI_CONTROL list, PLISTITEM item )
{
	TEXTCHAR *newname = (TEXTCHAR*)psv;
	GetItemText( item, 256, newname );
	InvokeButton( GetNearControl( list, IDOK ) );
}

enum {
LST_NEWBOARD = 1000
, BTN_CREATENAME
};

PRELOAD( register_control_ids )
{
	SimpleRegisterResource( LST_NEWBOARD, LISTBOX_CONTROL_NAME );
	SimpleRegisterResource( BTN_CREATENAME, NORMAL_BUTTON_NAME );
}

CTEXTSTR PickBoardName( PODBC odbc, int bMustExist )
{
	PSI_CONTROL frame = LoadXMLFrame( WIDE("PickBoardName.frame") );
	if( frame )
	{
		static TEXTCHAR newname[256];
		int okay = 0;
		int done = 0;
		{
			CTEXTSTR *results;
			PSI_CONTROL list = GetControl( frame, LST_NEWBOARD );
			if( list )
			{
				for( SQLRecordQuery( odbc, WIDE("select board_name from board_info"), NULL, &results, NULL );
					results;
					FetchSQLRecord( odbc, &results ) )
				{
					AddListItem( list, results[0] );
				}
				SetSelChangeHandler( list, SelectedItem, (PTRSZVAL)newname ); 
				SetDoubleClickHandler( list, SelectedItemLoad, (PTRSZVAL)newname ); 
			}
		}
		{
			PSI_CONTROL pc;
			if( ( pc = GetControl( frame, BTN_CREATENAME) ) != NULL )
			{
				if( bMustExist )
					HideCommon( pc );
				else
					SetButtonPushMethod( pc, CreateNewBoardName, (PTRSZVAL)newname );
			}
		}
		SetCommonButtons( frame, &done, &okay );
		DisplayFrame( frame );
		EditFrame( frame, 1 );
		CommonWait( frame );
		if( okay )
		{
			DestroyFrame( &frame );
			return newname;
		}
		DestroyFrame( &frame );
		return NULL;
	}
	return NULL;
}

class BACKGROUND_METHODS:public PEICE_METHODS
{
	BRAINBOARD * brainboard;
public:
	BACKGROUND_METHODS( BRAINBOARD *newbrainboard ) 
	{ brainboard = newbrainboard; }
private:
	typedef PEICE_METHODS Parent;
public:
	PTRSZVAL Create(PTRSZVAL psvExtra)
	{
		//brainboard = (BRAINBOARD *)psvExtra;
		return 1;
	}

	void Destroy( PTRSZVAL )
	{
		// nothing special to do on destroy background
	}
	PEICE_PROC( void, Properties )( PTRSZVAL psv, PCOMMON parent );
	PEICE_PROC( int, Connect )( PTRSZVAL psvTo
				  , int rowto, int colto
				  , PTRSZVAL psvFrom
				  , int rowfrom, int colfrom )
	{
      return 0;
	}

	void Update( PTRSZVAL psv, _32 cycle )
	{
      lprintf( WIDE("Update background - nothing to do.") );
		Parent::Update(psv,cycle);
	}

	int OnClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		brainboard->board->LockDrag();
		return TRUE;
	}

	int OnRightClick( PTRSZVAL psv, S_32 x, S_32 y )
	{
		brainboard->RebuildComponentPopups();

		_32 result = TrackPopup( brainboard->hMenu, NULL );
		//DebugBreak();
		if( result >= MNU_ADD_INPUT_START && result < MNU_ADD_OUTPUT_START )
		{
			lprintf( WIDE("Put input peice at %d,%d"), x, y );
			POUTPUT_INPUT io_thing = (POUTPUT_INPUT)Allocate( sizeof( *io_thing ) );
			io_thing->brainboard = brainboard;
			io_thing->brain = io_thing->brainboard->brain;
			io_thing->conn = (PCONNECTOR)GetLink( &brainboard->connectors, result-MNU_ADD_INPUT_START );// io_thing->pbs->getinput( (result-MNU_ADD_INPUT_START) %80 );

			io_thing->neuron = io_thing->brain->GetInputNeuron( io_thing->conn );
			//brainboard->create_input_type = GetLink( &brainboard->inputs, result-MNU_ADD_INPUT_START );

			// really this only needs to pass the connector? or do I need to get a Input/Ouptut Neuron?
			brainboard->board->PutPeice( brainboard->InputPeice, x, y, (PTRSZVAL)io_thing );
		}
		else if( result >= MNU_ADD_OUTPUT_START && result <= MNU_ADD_OUTPUT_LAST )
		{
			lprintf( WIDE("Put output peice at %d,%d"), x, y );
			POUTPUT_INPUT io_thing = (POUTPUT_INPUT)Allocate( sizeof( *io_thing ) );
			io_thing->brainboard = brainboard;
			io_thing->brain = io_thing->brainboard->brain;
			io_thing->conn = (PCONNECTOR)GetLink( &brainboard->connectors, result-MNU_ADD_OUTPUT_START );// io_thing->pbs->getoutput( (result-MNU_ADD_OUTPUT_START)%80 );

			// really this only needs to pass the connector? or do I need to get a Input/Ouptut Neuron?
			brainboard->board->PutPeice( brainboard->OutputPeice, x, y, (PTRSZVAL)io_thing );
		}
		else switch( result )
		{
		case MNU_ADDNEURON:
			lprintf( WIDE("Put neuron peice at %d,%d"), x, y );
			brainboard->board->PutPeice( brainboard->NeuronPeice, x, y, 0 );
			return TRUE;
      case MNU_ZOOM:
      case MNU_ZOOM+1:
      case MNU_ZOOM+2:
         brainboard->board->SetScale( result - MNU_ZOOM );
         break;
	  case MNU_SAVE:
         {
            //BYTE byFile[256];
            //BYTE byTemp[256];
            //byFile[0] = 0;
            //if( SelectNewFile( (HWND)NULL, (char*)byFile ) )
            {
               //FILE *file;
               //strcpy( (char*)byTemp, (char*)byFile );
               //strupr( (char*)byTemp );
               //if( !strstr( (char*)byTemp, WIDE(".BOARD") ) )
               //   strcat( (char*)byFile, WIDE(".Board") );

               //file = fopen( (char*)byFile, WIDE("wb") );
				//	if( file  )
					{
						CTEXTSTR name = PickBoardName( NULL, FALSE );
						if( name )
							brainboard->board->Save( NULL, name );
              //    fclose( file );
               }
            }
         }

			break;
		case MNU_LOAD:
			{
				CTEXTSTR name = PickBoardName( NULL, TRUE );
				if( name )
					brainboard->board->Load( NULL, name );
			}
			break;
		case MNU_CLOSE:
			delete brainboard;
			return FALSE;
		}
		return TRUE;
	}
	int OnDoubleClick( PTRSZVAL psv, int x, int y )
	{
		_32 result = TrackPopup( brainboard->hMenu, NULL );
		return TRUE;
	}
};

//---------------------------------------------------

PTRSZVAL DefineAColor( PTRSZVAL psv, arg_list args )
{
	PARAM( args, char *, pName );
	PARAM( args, CDATA, color );
	return psv;
}

#if 0
{
#define Set(color)  else if( !stricmp( pName, #color ) ) *color = RGB( r, g, b );

	if(0) {}
   Set( GAIN_MID     )
   Set( GAIN_HIGH    )
   Set( GAIN_LOW     )

   Set( LEVEL_MID    )
   Set( LEVEL_LOW    )
   Set( LEVEL_HIGH   )

   Set( INPUT_MID    )
   Set( INPUT_HIGH   )
   Set( INPUT_LOW    )

   Set( THRESH_MID   )
   Set( THRESH_LOW   )
   Set( THRESH_HIGH  )

   Set( INPUT_LEVEL_LOW    )
   Set( INPUT_LEVEL_MID    )
   Set( INPUT_LEVEL_HIGH   )

   Set( OUTPUT_LEVEL_LOW   )
   Set( OUTPUT_LEVEL_MID   )
   Set( OUTPUT_LEVEL_HIGH  )
}
#endif


//---------------------------------------------------

PPEICE_METHODS BRAINBOARD::FindPeiceMethods( TEXTCHAR *type )
{
	PPEICE_METHODS methods = NULL;
	if( StrCaseCmp( type, WIDE("neuron") ) == 0 )
		methods =neuron_methods;
	else if( StrCaseCmp( type, WIDE("background") ) == 0 )
		methods =background_methods;
	else if( StrCaseCmp( type, WIDE("nerve") ) == 0 )
		methods =nerve_methods;
	else if( StrCaseCmp( type, WIDE("input") ) == 0 )
		methods =input_methods;
	else if( StrCaseCmp( type, WIDE("output") ) == 0 )
		methods =output_methods;
	if( methods && methods->master )
	{
		lprintf( WIDE("Peice for methods is already defined... there is a tight relationship between a single graphic and these methods") );
		//return NULL;
	}
	return methods;
}

//---------------------------------------------------
void BRAINBOARD::SetCellSize( arg_list args )
{
	PARAM( args, S_64, x );
	PARAM( args, S_64, y );
	board->SetCellSize( (int)x, (int)y );
}
PTRSZVAL CPROC SetCellSize( PTRSZVAL psv, arg_list args )
{
	((BRAINBOARD*)psv)->SetCellSize( args );
	return psv;
}

//---------------------------------------------------
void BRAINBOARD::DefinePeiceColors( arg_list args )
{
	PARAM( args, TEXTCHAR *, type );
	PARAM( args, _64, input_or_threshold );
	PARAM( args, CDATA, c1 );
	PARAM( args, CDATA, c2 );
	PARAM( args, CDATA, c3 );
	PPEICE_METHODS methods = FindPeiceMethods( type );
	if( methods == neuron_methods )
	{
		neuron_methods->SetColors( (int)input_or_threshold, c1, c2, c3 );
	}
	else if( methods == nerve_methods )
	{
      //nerve_methods.SetColors( c1, c2, c3 );
	}
	else if( methods == input_methods )
	{
		input_methods->SetColors( c1,c2,c3 );
	}
	else if( methods == output_methods )
	{
		output_methods->SetColors( c1,c2,c3 );
	}
}
PTRSZVAL CPROC DefinePeiceColors( PTRSZVAL psv, arg_list args )
{
	BRAINBOARD *brainboard = (BRAINBOARD*)psv;
	brainboard->DefinePeiceColors( args );
	return psv;
}
//---------------------------------------------------
void BRAINBOARD::DefineABlock( arg_list args )
{
	PARAM( args, TEXTCHAR *, type );
	PARAM( args, _64, cx );
	PARAM( args, _64, cy );
	PARAM( args, TEXTCHAR *, filename );
	PPEICE_METHODS methods = FindPeiceMethods( type );
	Image image = LoadImageFile( filename );
	if( image )
	{
		lprintf( WIDE("Make block %s : %s"), type, filename );
		PIPEICE pip = board->CreatePeice( type, image
										, (int)cx, (int)cy
										, ((int)cx-1)/2, ((int)cy-1)/2
										, methods 
										, (PTRSZVAL)this /* brainboard*/
										 );
		lprintf( WIDE("Make block %s : %s"), type, filename );
		if( methods == background_methods )
		{
			board->SetBackground( pip );
		lprintf( WIDE("Make block %s : %s"), type, filename );
		}
		else if( methods == neuron_methods )
		{
			NeuronPeice = pip;
		lprintf( WIDE("Make block %s : %s"), type, filename );
		}
		else if( methods == input_methods )
		{
			InputPeice = pip;
		lprintf( WIDE("Make block %s : %s"), type, filename );
		}
		else if( methods == output_methods )
		{
			OutputPeice = pip;
		lprintf( WIDE("Make block %s : %s"), type, filename );
		}
	}
	else
		lprintf( WIDE("Failed to open %s"), filename );
}
PTRSZVAL CPROC DefineABlock( PTRSZVAL psv, arg_list args )
{
	BRAINBOARD *brainboard = (BRAINBOARD*)psv;
	brainboard->DefineABlock( args );
	return psv;
}

//---------------------------------------------------
void BRAINBOARD::DefineABlockNoOpt( arg_list args )
{
	PARAM( args, TEXTCHAR *, filename );
	//PPEICE_METHODS methods = FindPeiceMethods( type );
	Image image = LoadImageFile( filename );
	lprintf( WIDE("Attempt to define via with %s"), filename );
	if( image )
		NervePeice = board->CreateVia( WIDE("nerve"), image, nerve_methods, (PTRSZVAL)this );
}
PTRSZVAL CPROC DefineABlockNoOpt( PTRSZVAL psv, arg_list args )
{
	BRAINBOARD *brainboard = (BRAINBOARD*)psv;
	brainboard->DefineABlockNoOpt( args );
	return psv;
}

//---------------------------------------------------

BRAINBOARD::BRAINBOARD(PBRAIN _brain )
{
	background_methods=new BACKGROUND_METHODS(this);
	neuron_methods=new NEURON_METHODS(this);
	input_methods=new INPUT_METHODS(this);
	output_methods=new OUTPUT_METHODS(this);
	nerve_methods=new NERVE_METHODS(this);
	brain = _brain;
	Init();
}

//---------------------------------------------------

void BRAINBOARD::LoadPeices( TEXTCHAR *file )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	AddConfigurationMethod( pch, WIDE("cell size %i by %i"), ::SetCellSize );
	//#AddConfigurationMethod( pch, WIDE("color %w %c"), DefineAColor );
	AddConfigurationMethod( pch, WIDE("block %m (%i by %i) %p"), ::DefineABlock );
	AddConfigurationMethod( pch, WIDE("color %m %i %c %c %c"), ::DefinePeiceColors );
	AddConfigurationMethod( pch, WIDE("pathway %p"), ::DefineABlockNoOpt );
	
	if( !ProcessConfigurationFile( pch, file, (PTRSZVAL)this ) )
	{
		TEXTCHAR otherfile[256];
		snprintf( otherfile, 256, WIDE("%%resources%%/%s"), file );
		ProcessConfigurationFile( pch, otherfile, (PTRSZVAL)this );
	}
	DestroyConfigurationHandler( pch );
}

void BuildBrainstemMenus( PMENU hMenuComponents, PBRAIN_STEM pbs
						 , PLIST *menus
						 , PLIST *connectors, INDEX idx )
{
	{
		int n = 0;
		//PBRAIN_STEM pbs;
		PMENU comp_menu, menu;
		//for( pbs = brain->first(); pbs; pbs = brain->next() )
		{
			INDEX idx;
			PCONNECTOR connector;
			PBRAIN_STEM module;
			AppendPopupItem( hMenuComponents
									, MF_STRING|MF_POPUP
									, (PTRSZVAL)(comp_menu = CreatePopup())
									, pbs->name() );
			AddLink( menus, (POINTER)comp_menu );
			AppendPopupItem( comp_menu, MF_STRING|MF_POPUP, (PTRSZVAL)(menu = CreatePopup() ), WIDE("inputs") );
			AddLink( menus, (POINTER)menu );
			
			LIST_FORALL( pbs->Inputs.list, idx, PCONNECTOR, connector )
			{
				AddLink( connectors, (POINTER)connector );
				AppendPopupItem( menu, MF_STRING, MNU_ADD_INPUT_START + FindLink( connectors, (POINTER)connector) + ( n * 80 ), connector->name() );
			}
			AppendPopupItem( comp_menu, MF_STRING|MF_POPUP, (PTRSZVAL)(menu = CreatePopup() ), WIDE("outputs") );
			AddLink( menus, (POINTER)menu );
			
			LIST_FORALL( pbs->Outputs.list, idx, PCONNECTOR, connector )
			{
				AddLink( connectors, (POINTER)connector );
				AppendPopupItem( menu, MF_STRING, MNU_ADD_OUTPUT_START + FindLink( connectors, (POINTER)connector) + ( n * 80 ), connector->name() );
			}
			
			AppendPopupItem( comp_menu, MF_STRING|MF_POPUP, (PTRSZVAL)(menu = CreatePopup() ), WIDE("module") );
			AddLink( menus, (POINTER)menu );
			
			for( module = pbs->first_module(); module; idx++, module = pbs->next_module() )
			{
				BuildBrainstemMenus( menu, module, menus, connectors, idx );
				//AppendPopupItem( menu, MF_STRING, MNU_ADD_OUTPUT_START + idx + ( n * 80 ), module->name() );
				//SetLink( &outputs, idx, (POINTER)connector );
			}
			n++;
		}
	}
}

//---------------------------------------------------

void BRAINBOARD::RebuildComponentPopups()
	{
		{
			int n = 0;
			INDEX idx;
			PBRAIN_STEM pbs;
			ResetPopup( hMenuComponents );
			LIST_FORALL( brain->BrainStems.list, idx, PBRAIN_STEM, pbs )
			{
				BuildBrainstemMenus( hMenuComponents, pbs, &menus, &connectors, 0 );
			}
		}
	}

void BRAINBOARD::InitMenus( void )
{
	hMenu = CreatePopup();
	AppendPopupItem( hMenu, MF_STRING, MNU_ADDNEURON, WIDE("Add &Neuron") );
   
	AppendPopupItem( hMenu,MF_STRING|MF_POPUP, (PTRSZVAL)(hMenuComponents=CreatePopup()), WIDE("Add &Component") );
	{
		int n = 0;
		INDEX idx;
		PBRAIN_STEM pbs;
		LIST_FORALL( brain->BrainStems.list, idx, PBRAIN_STEM, pbs )
		{
			BuildBrainstemMenus( hMenuComponents, pbs, &menus, &connectors, 0 );
		}
	}
	AppendPopupItem( hMenu,MF_SEPARATOR,0,0 );
	AppendPopupItem( hMenu,MF_STRING, MNU_RESET, WIDE("Reset") );
	AppendPopupItem( hMenu,MF_STRING, MNU_RUN, WIDE("RUN") );
	AppendPopupItem( hMenu,MF_SEPARATOR,0,0 );
	{
		PMENU hPopup;
		AppendPopupItem( hMenu,MF_STRING|MF_POPUP, (PTRSZVAL)(hPopup = CreatePopup()), WIDE("Zoom") );
		AddLink( &menus, hPopup );

		//hPopup = (PMENU)GetPopupData( hMenu, 6 );
		AppendPopupItem( hPopup, MF_STRING, MNU_ZOOM + 0, WIDE("x1") );
		AppendPopupItem( hPopup, MF_STRING, MNU_ZOOM + 1, WIDE("x2") );
		AppendPopupItem( hPopup, MF_STRING, MNU_ZOOM + 2, WIDE("x4") );
	}

	AppendPopupItem( hMenu,MF_STRING, MNU_NEURON, WIDE("Default Neuron") );
	AppendPopupItem( hMenu,MF_STRING, MNU_SYNAPSE, WIDE("Default Synapse") );
	AppendPopupItem( hMenu,MF_STRING, MNU_SIGMOID, WIDE("Sigmoid Constant") );
	AppendPopupItem( hMenu,MF_STRING, MNU_SAVE, WIDE("Save...") );
	AppendPopupItem( hMenu,MF_STRING, MNU_LOAD, WIDE("Load...") );
	AppendPopupItem( hMenu,MF_SEPARATOR,0,0 );
	AppendPopupItem( hMenu,MF_STRING, MNU_CLOSE, WIDE("Close") );

}

void BRAINBOARD::Init( void )
{
	//brainboard = this;
	//InitCommonControls();

	InputPeice = NULL;
	OutputPeice = NULL;
	NeuronPeice = NULL;
	NervePeice = NULL;
	BackgroundPeice = NULL;

	//PSI_CONTROL frame = CreateFrame( WIDE("Brain Editor"), 0, 0, 640, 480, BORDER_RESIZABLE, NULL );
	board = CreateBoardControl( NULL /*frame*/, 0, 0, 640, 480 );
	//DisplayFrame( frame );
	//CreateToolbin( board );
	board->SetCellSize( 16, 16 );
	if( !brain )
	{
		flags.bOwnBrain = 1;
		brain = new BRAIN();
	}
	else
		flags.bOwnBrain = 0;
	LoadPeices( WIDE("brain.peices.txt") );
	connectors = NULL;
	menus = NULL;

	InitMenus();
	DefaultNeuron = brain->GetNeuron();
	DefaultSynapse = brain->GetSynapse();
}

BRAINBOARD::BRAINBOARD()
{
	background_methods=new BACKGROUND_METHODS(this);
	neuron_methods=new NEURON_METHODS(this);
	input_methods=new INPUT_METHODS(this);
	output_methods=new OUTPUT_METHODS(this);
	nerve_methods=new NERVE_METHODS(this);
	{
		brain = NULL;
		Init();
	}
}

BRAINBOARD::~BRAINBOARD()
{
	// sub menus get destroyed if they are attached...
	DestroyPopup( hMenu );
	//DestroyPopup( hMenuComponents );
	board->Close();
	brain->ReleaseNeuron( DefaultNeuron );
	brain->ReleaseSynapse( DefaultSynapse );
	{
		INDEX idx;
		PCONNECTOR conn;
		// these are a copy of information available in the brain?
		LIST_FORALL( connectors, idx, PCONNECTOR, conn )
		{
			//delete conn;
		}
		DeleteList( &connectors );
	}

	if( 0 )
	{
		INDEX idx;
		PMENU menu;
		LIST_FORALL( menus, idx, PMENU, menu )
		{
			DestroyPopup( menu );
		}
	}
	DeleteList( &menus );
	if( InputPeice )
		InputPeice->Destroy();
	if( OutputPeice )
		OutputPeice->Destroy();
	if( NeuronPeice )
		NeuronPeice->Destroy();
	if( NervePeice )
		NervePeice->Destroy();
	if( BackgroundPeice )
		BackgroundPeice->Destroy();

	delete background_methods;
	delete neuron_methods;
	delete input_methods;
	delete output_methods;
	delete nerve_methods;
}


BRAINBOARD *CreateBrainBoard( PBRAIN brain )
{
	return new BRAINBOARD( brain );
}

PIBOARD GetBoard( PBRAINBOARD board )
{
	return board->board;
}

#ifdef BUILD_TEST_SHELL
float f_values[10];
CONNECTOR *connectors_in[] = { new connector( WIDE("one"), &f_values[0] )
,new connector( WIDE("two"), &f_values[1] )
,new connector( WIDE("three"), &f_values[2] )
,new connector( WIDE("four"), &f_values[3] )
,new connector( WIDE("five"), &f_values[4] )
,new connector( WIDE("six"), &f_values[5] )
,new connector( WIDE("seven"), &f_values[6] )
,new connector( WIDE("eight"), &f_values[7] )
,new connector( WIDE("nine"), &f_values[8] )
,new connector( WIDE("ten"), &f_values[9] )
};

CONNECTOR *connectors_out[] = { new connector( WIDE("one"), &f_values[0] )
,new connector( WIDE("two"), &f_values[1] )
,new connector( WIDE("three"), &f_values[2] )
,new connector( WIDE("four"), &f_values[3] )
,new connector( WIDE("five"), &f_values[4] )
,new connector( WIDE("six"), &f_values[5] )
,new connector( WIDE("seven"), &f_values[6] )
,new connector( WIDE("eight"), &f_values[7] )
,new connector( WIDE("nine"), &f_values[8] )
,new connector( WIDE("ten"), &f_values[9] )
};

BRAIN_STEM clusters[1] = { BRAIN_STEM( WIDE("Basic Structure")
												 , connectors_in, sizeof(connectors_in)/sizeof(connectors_in[0])
												 , connectors_out, sizeof(connectors_out)/sizeof(connectors_out[0]) ) };

// creates a thread, don't do this.
PBRAIN brains[1];// = { BRAIN( &clusters[0] ) };

SaneWinMain( argc, argv )
{

	brains[0] = new BRAIN( &clusters[0] );
	//SetAllocateLogging( TRUE );
	new BRAINBOARD;
	new BRAINBOARD;
	new BRAINBOARD;
	while( 1 )
      Sleep( 1000 );
   return 0;
}
EndSaneWinMain()
#endif

PRELOAD( CreateBoardTables )
{
#if 0
	PTABLE table;
	table = GetFieldsInSQL( board_info_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( board_input_output, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( board_layer_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( board_layer_link, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( board_layer_neuron_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( board_layer_path_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );

	table = GetFieldsInSQL( board_layer_synapse_table, TRUE );
	CheckODBCTable( NULL, table, 0 );
	DestroySQLTable( table );
#endif

}


