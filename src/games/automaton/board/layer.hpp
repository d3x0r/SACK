#ifndef BOARD_STRUCTURES_DEFINED
#define BOARD_STRUCTURES_DEFINED

//#define MAX_LAYERS 16

//#include "neuron.h"
#include <pssql.h>
#include <image.h>
#include <sharemem.h>

#include "peice.hpp"

typedef class LAYER *PLAYER;

#undef new
#undef delete
typedef struct layer_flags_tag
{
	// Index into PEICE array is stored...
	// board contains an array of peices....
   // as a bitfield these do not expand correctly if signed.
	signed int BackDir : 4;
	signed int ForeDir : 4;
	// forced set on first node cannot be cleared!
	// other nodes other than the first may have forced.
	// which indicates that the foredir MUST be matched.
	// these nodes do not unlay either.  Need to compute
   // the NEXT layer with a LeftOrRight correction factor
	signed int bForced : 1;
	// if bLeft, and bForced
	// UnlayerPath sets bRight and results with this layer intact.
	// if bLeft AND bRight AND bForced
	// UnlayerPath removes this node (if BackDir != NOWHERE)
	signed int bFlopped : 1; // starts at 0.  Moves ForDir +/- 1
	int bTry : 1; // set if a hard direction tendancy was set ...
   // repeat above with right, setting left, moving left...
	signed int bRight : 1; // starts at 0.  Moves ForDir +/- 1
	// foredir, backdir are unused if the peice is a filler
	// x, y of the data node will be an offset from the current
	// at which place the filler from viaset->GetViaFill1() and 2 will be done
	// actually x, y are unused, since the offset is resulted from the before
	// actually it looks like fillers don't need to be tracked, just drawn
	//int bFiller : 4;
} LAYER_FLAGS;


typedef void (CPROC *UpdateProc)( PTRSZVAL psv, CDATA colors[3] );

typedef struct LAYER_PATH_NODE_tag{
	LAYER_FLAGS flags; // 8 bits need 9 values (-1 being nowhere) 0-7, -1 ( 0xF )
	S_32 x, y;
} LAYER_PATH_NODE, *PLAYER_PATH_NODE;

typedef class LAYER_DATA *PLAYER_DATA;
class LAYER_DATA
{
// common content of a layer....
	//_32 ref; // reference count...
   //_32 _cycle;
   //CDATA cData[3]; // current colors...
	struct LAYER_DATAset_tag **pool;
   // especially for things like via peices...
   //PDATASTACK pds_path;
   // keep a copy of this from peice getsize...
	//_32 rows, cols;
public:
   // master peice archtype
	PIPEICE peice;
   // psv will be PNEURON or PSYNAPSE;
   // the instance of peice which this relates to
	PTRSZVAL psvInstance;

public:
	void Init( void );
	PEICE_PROC(,LAYER_DATA)();
	PEICE_PROC(,LAYER_DATA)( PIPEICE peice );
	PEICE_PROC(,~LAYER_DATA)();
	void SetPeice( PIPEICE peice );
	void *operator new( size_t sz );
	void *operator new( size_t sz, struct LAYER_DATAset_tag **frompool );
	void operator delete( void *ptr);
#ifndef __WATCOMC__
	void operator delete( void *ptr, struct LAYER_DATAset_tag **frompool );
#endif
	void Update( _32 cycle );
	// image and position to draw this layer onto...
	void Draw( PIBOARD, Image, S_32 x, S_32 y );
	friend class LAYER;
};

#define MAXLAYER_DATASPERSET 128
DeclareSet( LAYER_DATA );

class LAYER
{

	//LAYER_FLAGS flags;
	// many route may be linked to a layer
	// but, as a route, only the start and end
	// may contain intelligable information.
	// routes linked to routes, the dest knows there is
	// a route linked, and hmm tributaries/distributaries...
	PLIST linked;
	// okay this becomes non-basic extensions
	// where layers moved will move other attached
	// layers automagically.
public: // ended up exposing this so that save could work correctly...
   // maybe save should be a property of layer.. but then calling it ?
	struct {
		PLAYER layer;
		S_32 x, y; // where this is linked to the other layer
	}route_start_layer;
	struct {
		PLAYER layer;
		S_32 x, y; // where this is linked to the other layer
	}route_end_layer;
private:

	struct {
		// if this is a route, then this marks
		// whether the first node on the route
		// must remain in this direction
		// of if it may be changed.
		_32 bForced : 1;
		_32 bEnded : 1; // layed an end peice.
		// is a set of vias, and pds_path is used
		// to hold how, what, and where to draw.
		// all segments represent a single object (psvInstance)
		_32 bRoute : 1;
		_32 bRoot : 1; // member 0 of pool is 'root'
	} flags;
public: // exposed to saveLayer
	PDATASTACK pds_path; // PLAYER_PATH_NODE list
private:
	//S_32 row, col; // which row/col of peice this is
	struct LAYERset_tag **pool; // same member - different classes...
	//wonder how to union inheritance...
public:
	void move( S_32 del_x, S_32 del_y );
	void isolate( void );
	void link_top(void );
	// link via to another (via or peice) at x, y
	// x, y save where this is linked to 
	void Link( PLAYER via, int linktype, S_32 x = 0, S_32 y = 0 );
	// unlinks a layer, first, if the end isl inked
	// it unlinks that, then if the start is linked it
	// unlinks that (probably resulting in distruction? )
	void Unlink( void );
	PLAYER next;
	PLAYER prior;
	//what_i_am_over, what_i_am_under;
	PLAYER_DATA pLayerData;
	S_32 x, y;
	// for via sets, the minimum until the width, height
	// is the whole span.
	S_32 min_x, min_y;
	_32 w, h;

public:
	PEICE_PROC(,LAYER)();
	PEICE_PROC(,LAYER)( PODBC odbc, PLIST peices, INDEX iLayer );
	PEICE_PROC(,LAYER)( PIPEICE peice, int x, int y, int w, int h );
	// ofs x, y apply as x - ofsx = min_x y - ofsy = miny
	PEICE_PROC(,LAYER)( PIPEICE peice, int x, int y, int ofsx, int ofsy, int w, int h );
	PEICE_PROC(,LAYER)( PIPEICE peice, int x, int y );
	PEICE_PROC(,LAYER)( PIPEICE peice );
	// does it realy matter wehter I was started with a peice or a viaset?
	PEICE_PROC(,LAYER)( PIVIA via );
	PEICE_PROC(,~LAYER)();
	// ability to set start x, y of this layer
	// it covers an area from here to
	// w, h
	// this allows things like wires to have bounds
	// checking to see if they should be displayed at all...

	//void SetStart( int x, int y );
	//void SetSpan( int w, int h );
	void Init( void );
	void *operator new( size_t sz );
	void *operator new( size_t sz, struct LAYERset_tag **frompool, struct LAYER_DATAset_tag **fromdatapool );
#ifdef _MSC_VER
	void operator delete( void *ptr, struct LAYERset_tag **frompool, struct LAYER_DATAset_tag **fromdatapool );
#endif
	void operator delete( void *ptr );
	void Draw( PIBOARD, Image, S_32 x, S_32 y );
	PIPEICE GetPeice(void );
	PLAYER FindLayer( INDEX iLayer );
	// this begin path just sets the point, and any direction is valid
	// origin point rotates to accomodate.
	void BeginPath( S_32 x, S_32 y );
	// this path must begin from NOWHERE to direction.
	void BeginPath( S_32 x, S_32 y, int direction );
	int IsLayerAt( S_32 *x, S_32 *y );
	// end at this point.
	// final node will be foredir=NOWHERE
	void LayPath( S_32 x, S_32 y );
	//void LayPath( int x, int y );
	int GetLastBackDirection( void );
	int GetLastForeDirection( void );
	int Overlaps( int x, int y ); // returns true if there is a node at x,y (even a partial via fill)
	// UnlayPath 0 layers unlocks a forced node
	// UnlayPath 1 backs up 1 step
	// Intent for use with Overlap which results in nSteps back to the overlap.
	// due to node flags, UnlayPath may choose to not undo all layers.
	// it may even perform internal modifications to the node
	// oh - and it retuns a free PeekStack on the path data
	PLAYER_PATH_NODE UnlayPath( int nLayers );
	// unlay until the loop spot is found...
	//int UnlayPath( int bLoop );
	// destination x, y to extend the path to...
	// unwinding loops, and auto extension is
	// done.
	// (should also modify the region information in the layer itself)

public: //-- this section used by SaveLayer
	// iLayer is the board_layer_id in board_layer table
	// when loaded, or saved, this is updated, and is valid
	// until saved again (will change on save)
	INDEX iLayer; // during the process of saving, this is kept
	INDEX Save( PODBC odbc );
	LOGICAL Load( PODBC odbc, INDEX iItem );
};

enum {
	LINK_VIA_START
	  , LINK_VIA_END
};

#define MAXLAYERSPERSET 128
DeclareSet( LAYER );

typedef struct layer_module_tag {
	PLAYERSET        LayerPool;
	//PSHADOW_LAYERSET ShadowPool;
	PLAYER_DATASET   DataPool;
	// this will be the known point of layers...
	//PBOARD           board;
	//void Draw( PCELL cell );
} LAYER_MODULE;

// can include the bacground texturing on the board....

PTRSZVAL CPROC CheckIsLayer( POINTER p, PTRSZVAL psv );

extern "C"
{
int CPROC FindDirection( int _x, int _y, int wX, int wY ); // From, To
}

#endif
