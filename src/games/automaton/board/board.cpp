#define BOARD_MAIN_SOURCE
#include "interface.h"
#include <render.h>
#include <controls.h>
#include <timers.h>
#include <psi.h>
#include "board.hpp"
#include "global.h"

#ifdef __WINDOWS__
#define IMPORT __declspec(dllexport)
#else
#define IMPORT
#endif

// should be 8 pixels on each and every side
// these will be the default color (black?)
#define SCREEN_PAD 8

// c++ compiler is really kinda fucked isn't it
// this is the next issue in a large chain of them...
//   struct { struct { int a, b; }; int c, d; } cannot be initialized
//   defining a array, and then initializing it later cannot be done.
//   // maybe that's not the real issue.
extern DIR_DELTA DirDeltaMap[8];

// I really do hate having circular dependancies....
void CPROC BoardRefreshExtern( PTRSZVAL dwUser, PRENDERER renderer );
void CPROC BoardWindowClose( _32 dwUser );
void CPROC BoardRefreshExtern( PTRSZVAL dwUser, PRENDERER renderer );

//----------------------------------------------------------------------
typedef class UPDATE *PUPDATE;
class UPDATE {
	S_32 _x, _y;
	_32 _wd, _ht;
	PRENDERER pDisplay;
	PSI_CONTROL pControl;
public:
	UPDATE( PRENDERER display )
	{
		pDisplay = display;
		pControl = NULL;
		_wd = 0;
		_ht = 0;
	}
	UPDATE( PSI_CONTROL pc )
	{
		pDisplay = NULL;
		pControl = pc;
		_wd = 0;
		_ht = 0;
	}
	~UPDATE()
	{
	}
	void add( S_32 x, S_32 y, _32 w, _32 h )
	{
		//Log4( WIDE("Adding update region %d,%d -%d,%d"), x, y, w, h );
		if( _wd == 0 && _ht == 0 )
		{
			_x = x;
			_y = y;
		}
		if( x < _x )
		{
			_wd += _x - x;
			_x = x;
		}
		if( y < _y )
		{
			_ht += _y - y;
			_y = y;
		}
		if( w > _wd )
			_wd = w;
		if( h > _ht )
			_ht = h;
	}
	void flush( void )
	{
		if( _wd && _ht )
		{
			//Log4( WIDE("Flushing update to display... %d,%d - %d,%d"), _x, _y, _wd, _ht );
         if( pDisplay )
				UpdateDisplayPortion( pDisplay, _x, _y, _wd, _ht );
			else
            SmudgeCommon( pControl );
		}
		_wd = 0;
		_ht = 0;
	}
};


PTRSZVAL CPROC faisIsLayerAt( void *_layer, PTRSZVAL psv )
{
	struct xy{
		S_32 x, y;
      PLAYER not_layer;
	} *pxy = (struct xy*)psv;
	PLAYER layer = (PLAYER)_layer;
	if( !layer->IsLayerAt( &pxy->x, &pxy->y ) )
		return 0;
	if( layer == pxy->not_layer )
      return 0; // lie, we need to return another...
   return (PTRSZVAL)layer;
}

class BOARD:public IBOARD
{
	_32 cell_width, cell_height;
	// original cell width/height
	// cell_width, height are updated to reflect scale
	_32 _cell_width, _cell_height;
	PTRSZVAL default_peice_instance;
	PIPEICE default_peice;
	void (CPROC*OnClose)(PTRSZVAL,class IBOARD*);
	PTRSZVAL psvClose;
public:
	INDEX Save( PODBC odbc, CTEXTSTR name );
	LOGICAL Load( PODBC odbc, CTEXTSTR name );
	void reset( void );
	CRITICALSECTION cs;
private:
	PLIST peices;
	INDEX iTimer; // this is the timer used for board refreshes...
	PUPDATE update;
	//{
		PRENDERER pDisplay;
		PSI_CONTROL pControl;
		Image pImage;  // this is implied to be pDipslay->surface or pControl->surface
	//}
	PLAYERSET LayerPool;
	PLAYER_DATASET LayerDataPool;
   PLAYER RootLayer;

	// current layer which has a mouse event dispatched to it.
	PLAYER mouse_current_layer;
	PLAYER route_current_layer;
	PLAYER move_current_layer;
	S_32 xStart, yStart, wX, wY;
	_32 board_width, board_height;
	// cached based on current layer definitions...
	// when layers are updated, this is also updated
	// when board is scrolled...
	// it's a fairly tedious process so please keep these
	// updates down to as little as possible...
	// I suppose I should even hook into the update of the layer
	// module to update the current top-level cells on the board.
	// hmm drawing however is bottom-up, and hmm actually this
	// is a mouse phenomenon - display is a different basis.
	PCELL *board; //[64*64]; // manually have to compute offset from board_width
	S_32 board_origin_x, board_origin_y; // [0][0] == this coordinate.

	struct {
		_32 bSliding : 1;
		_32 bDragging : 1;
		_32 bLockLeft : 1;
		_32 bLockRight : 1;
		_32 bLeft : 1;
		_32 bRight : 1;
		// left changed happend both when a button is clicked
		// and when it is unclicked.
		_32 bLeftChanged : 1;
		_32 bRightChanged : 1;
	} flags;
	int scale;
	struct {
		PIPEICE viaset;
		S_32 _x, _y;
	} current_path;
public:
	  int GetScale( void )
	{
		return scale;
	}
public:
	void SetScale( int scale )
	{
		if( scale < 0 || scale > 2 )
			return;
		cell_width = _cell_width >> scale;
		cell_height = _cell_height >> scale;
		BOARD::scale = scale;
	}
	void Close( void );
private:
	void SetCloseHandler( void (CPROC*)(PTRSZVAL,class IBOARD*), PTRSZVAL );
	void SetBackground( PIPEICE peice )
	{
		default_peice = peice;
		default_peice_instance = default_peice->methods->Create(peice->psvCreate);
		SmudgeCommon( pControl );
	}

	void SetCellSize( _32 cx, _32 cy )
	{
		cell_width = _cell_width = cx;
		cell_height = _cell_height = cy;
	}

	int BeginPath( PIVIA viaset/*, S_32 x, S_32 y*/, PTRSZVAL psv )
	{
		if( mouse_current_layer )
		{
			EnterCriticalSec( &cs );
			PLAYER pl = new(&LayerPool,&LayerDataPool) LAYER( viaset );
			int connect_okay = mouse_current_layer
				->pLayerData
				->peice
				->methods
				->ConnectBegin( mouse_current_layer
									->pLayerData
									->psvInstance
								  , (wX - mouse_current_layer->x)
								  , (wY - mouse_current_layer->y)
								  , viaset
								  , pl->pLayerData->psvInstance );
			if( !connect_okay )
			{
				//DebugBreak();
				delete pl;
				LeaveCriticalSec( &cs );
				return FALSE;
			}
			mouse_current_layer->Link( pl, LINK_VIA_START
         									, (wX - mouse_current_layer->x)
         									, (wY - mouse_current_layer->y) );
			route_current_layer = pl;
			// set the via layer type, and direction, and stuff..
			pl->BeginPath( wX, wY );
			LeaveCriticalSec( &cs );

			// otherwise change mode, now we are working on a new current layer...
			// we're working on dragging the connection, and... stuff...
		}
		//current_path.viaset = viaset;
		//current_path._x = x;
		//current_path._y = y;
		// lay peice does something like uhmm add to layer
		// to this point layers are single image entities...
		//LayPeice( viaset, x, y );
		return TRUE;
	}

PLAYER GetLayerAt( S_32 *wX, S_32 *wY, PLAYER notlayer = NULL )
{
	PLAYER layer = GetSetMember( LAYER, &LayerPool, 0 );
	while( layer )
	{
		if( layer != notlayer )
			if( layer->IsLayerAt( wX, wY ) )
			{
				lprintf( WIDE("Okay got a layer to return...") );
				return layer;
			}
		layer = layer->next;
	};

 //	if( ( layer = (PLAYER)LayerPool->forall( faisIsLayerAt, (PTRSZVAL)&xy ) ) )
 //  {
 //     (*wX) = xy.x;
 //  	(*wY) = xy.y;
 //  }
	return layer;
}

PLAYER_DATA GetLayerDataAt( S_32 *wX, S_32 *wY, PLAYER notlayer = NULL )
{
	PLAYER layer = GetLayerAt( wX, wY, notlayer );
	if( layer )
		return layer->pLayerData;
	return NULL;
}


// viaset is implied by route_current_layer
	void EndPath( S_32 x, S_32 y )
	{
		// really this is lay path also...
		// however, this pays attention to mouse states
		// and data and layers and connections and junk like that
		// at this point I should have
		//   mouse flags.bRight, bLeft
		//   route_current_layer AND mouse_current_layer
		//
		PLAYER layer;
		PLAYER_DATA pld;
		// first layer should result and be me... check it.
		EnterCriticalSec( &cs );
		if( layer = GetLayerAt( &x, &y, route_current_layer ) )
		{
			if( flags.bLeftChanged )
			{
				pld = layer->pLayerData;
				int connect_okay = pld->peice->methods->ConnectEnd( pld->psvInstance
																				  , (wX - layer->x)
																				  , (wY - layer->y)
																				  , route_current_layer->pLayerData->peice
																				  , route_current_layer->pLayerData->psvInstance );
				if( connect_okay )
				{
					//DebugBreak();
					lprintf( WIDE("Heh guess we should do something when connect succeeds?") );
					// keep route_current_layer;
					layer->Link( route_current_layer, LINK_VIA_END, (wX-layer->x), (wY-layer->y) );
					route_current_layer = NULL;
					LeaveCriticalSec( &cs );
					return;
				}
				else
				{
					//DebugBreak();
					delete route_current_layer;
					route_current_layer = NULL;
					LeaveCriticalSec( &cs );
					return;
				}
			}
		}
		else
		{
         // right click anywhere to end this thing...
			if( route_current_layer &&
				flags.bRightChanged &&
				!flags.bRight )
			{
            //DebugBreak();
				// also have to delete this layer.
				delete route_current_layer;
				route_current_layer = NULL;
				LeaveCriticalSec( &cs );
				return;
			}
		}
		LayPathTo( wX, wY );
		LeaveCriticalSec( &cs );
	}
void UnendPath( void )
{
	EnterCriticalSec( &cs );
	int disconnect_okay = mouse_current_layer
		->pLayerData
		->peice
		->methods
		->Disconnect( mouse_current_layer
							->pLayerData
							->psvInstance );
						  //, (wX - mouse_current_layer->x)
						  //, (wY - mouse_current_layer->y)
						  //, viaset
						  //, pl->pLayerData->psvInstance );
	if( disconnect_okay )
	{
		mouse_current_layer->Unlink();
		mouse_current_layer->isolate();
		mouse_current_layer->link_top();
		route_current_layer = mouse_current_layer;
	}
	LeaveCriticalSec( &cs );
}
public:
	void timer(void);
	PSI_CONTROL GetControl( void );
	void DrawLayer( PLAYER layer );
	void PutPeice( PIPEICE, S_32 x, S_32 y, PTRSZVAL psv );
	void BoardRefresh( void );  // put current board on screen.
private:

void LayPathTo( int wX, int wY )
{
	route_current_layer->LayPath( wX, wY );
	SmudgeCommon( pControl );
}

public:

 
void DoMouse( int X, int Y, int b )
{
	//static _left, _right;
#define SCRN_TO_GRID_X(x) ((x - SCREEN_PAD)/(signed)cell_width - board_origin_x )
#define SCRN_TO_GRID_Y(y) ((y - SCREEN_PAD)/(signed)cell_height - board_origin_y)

	wX = SCRN_TO_GRID_X( X );
	wY = SCRN_TO_GRID_Y( Y );
	//lprintf( WIDE("mouse at %d,%d"), wX, wY );
	{
		//S_32 x = wX, y = wY;
		//PLAYER_DATA pld = GetLayerDataAt( &x, &y );
		//lprintf( WIDE("%s at %d,%d"), pld?WIDE("something"):WIDE("nothing"), x, y );
	}

#ifdef __WINDOWS__
	SetCursor(LoadCursor(NULL, IDC_ARROW));
#endif
	flags.bLeftChanged = flags.bLeft ^ ( (b & MK_LBUTTON) != 0 );
	flags.bRightChanged = flags.bRight ^ ( (b & MK_RBUTTON) != 0 );
	flags.bLeft = ( (b & MK_LBUTTON) != 0 );
	flags.bRight = ( (b & MK_RBUTTON) != 0 );

	if( flags.bRightChanged && !flags.bRight )
	{
	   if( !route_current_layer )
	   {
			S_32 x = wX, y = wY;
			lprintf( WIDE("right at %d,%d"), wX, wY );
			PLAYER_DATA pld = GetLayerDataAt( &x, &y );
			if( pld )
			{
				lprintf( WIDE("Okay it's on a layer, and it's at %d,%d on the layer"), wX, wY );
				if( !pld->peice->methods->OnRightClick( pld->psvInstance, wX, wY ) )
					return; // routine has done something to abort processing...
			}
			else if( default_peice )
			{
				if( !default_peice->methods->OnRightClick(NULL,wX,wY) )
					return; // routine has done something to abort processing...
			}
		}
	}
	else
	{
		//_right = flags.bRight;
	}


	if( flags.bSliding )
	{
		if( ( flags.bLockLeft && flags.bLeft ) ||
			( flags.bLockRight && flags.bRight ) )
		{
				if( wX != xStart ||
					wY != yStart )
				{
					lprintf( WIDE("updating board origin by %d,%d"), wX-xStart, wY-yStart );
					board_origin_x += wX - xStart;
					board_origin_y += wY - yStart;
					wX = xStart;
					wY = yStart;
					SmudgeCommon( pControl );
				}
			}
			else
			{
				flags.bSliding = FALSE;
				flags.bLockLeft = FALSE;
				flags.bLockRight = FALSE;
			}
		}
   else if( move_current_layer ) // moving a node/neuron/other...
   {
		if( flags.bLeft )
		{
			//DebugBreak();
			move_current_layer->move( wX - xStart, wY - yStart );
			xStart = wX;
			yStart = wY;
			SmudgeCommon( pControl );
			move_current_layer
				->pLayerData
				->peice
				->methods
				->OnMove( move_current_layer
							->pLayerData->psvInstance
						  );
		}
		else
		{
			move_current_layer = NULL;
		}
	}
	else
	{
		if( flags.bLeft )  // not drawing, not doing anything...
		{
			// find neuron center...
			// first find something to do in this cell already
			// this is 'move neuron'
			// or disconnect from...

			S_32 x = wX, y = wY;
			PLAYER layer = GetLayerAt( &x, &y, route_current_layer );
			lprintf( WIDE("event at %d,%d"), wX, wY );
			if( route_current_layer )
			{
				if( flags.bLeftChanged )
				{
					if( !layer )
					{
						// if it was a layer... then lay path to is probably
                  // going to invoke connection procedures.
						default_peice->methods->OnClick(NULL,wX,wY);
					}
				}
				LayPathTo( wX, wY );
			}
			else if( layer )
			{
				PLAYER_DATA pld = layer->pLayerData;
				mouse_current_layer = layer;
				lprintf( WIDE("Generate onclick method to peice.") );
				pld->peice->methods->OnClick( pld->psvInstance, x, y );
				mouse_current_layer = NULL;
			}
			else if( default_peice )
			{
				lprintf( WIDE("Default peice click.") );
				default_peice->methods->OnClick(NULL,wX,wY);
			}
		}
		else
		{
			if( route_current_layer )
			{
				// ignore current layer, and uhmm
				// get Next layer data... so we have something to connect to...
				// okay end path is where all the smarts of this is...
				// handles mouse changes in state, handles linking to the peice on the board under this route...
				EndPath( wX, wY );
			}
		}
	}
}


IMPORT void LockDrag( void )
{
	// this method is for locking the drag on the board...
   // cannot lock if neither button is down...??
	if( flags.bLeft || flags.bRight )
	{
		xStart = wX;
		yStart = wY;
		flags.bSliding = TRUE;
		if( flags.bLeft )
		{
			flags.bLockRight = FALSE;
			flags.bLockLeft = TRUE;
		}
		else
		{
			flags.bLockRight = TRUE;
			flags.bLockLeft = FALSE;
		}
	}
	//Log( WIDE("Based on current OnMouse cell data message, lock that into cursor move...") );
}
IMPORT void LockPeiceDrag( void )
{
	// this method is for locking the drag on the board...
   // cannot lock if neither button is down...??
	if( flags.bLeft || flags.bRight )
	{
		xStart = wX;
		yStart = wY;
		flags.bDragging = TRUE;
		move_current_layer = mouse_current_layer;
		if( flags.bLeft )
		{
			flags.bLockRight = FALSE;
			flags.bLockLeft = TRUE;
		}
		else
		{
			flags.bLockRight = TRUE;
			flags.bLockLeft = FALSE;
		}
	}
	//Log( WIDE("Based on current OnMouse cell data message, lock that into cursor move...") );
}
private:
   void Init( void );

public:

   BOARD();
   BOARD(PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h );
   ~BOARD();
	PIPEICE CreatePeice( CTEXTSTR name //= WIDE("A Peice")
								  , Image image //= NULL
								  , int rows //= 1
								  , int cols //= 1
								  , int hotspot_x
								  , int hotspot_y
								  , PPEICE_METHODS methods //= NULL
								  , PTRSZVAL psv
								  );
	PIVIA CreateVia( CTEXTSTR name //= WIDE("A Peice")
						, Image image //= NULL
						, PVIA_METHODS methods //= NULL
					  , PTRSZVAL psv
						);
	PIPEICE GetFirstPeice( INDEX *idx );
	PIPEICE GetNextPeice( INDEX *idx );
   void GetSize( P_32 cx, P_32 cy )
	{
		// result with the current cell size, so we know
		// how much to multiply row/column counters by.
		// X is always passed correctly?
		if( cx )
			(*cx) = board_width;
		if( cy )
         (*cy) = board_height;
	}
	void GetCellSize( P_32 cx, P_32 cy, int scale )
	{
		// result with the current cell size, so we know
		// how much to multiply row/column counters by.
		// X is always passed correctly?
		if( !scale )
		{
			if( cx )
				(*cx) = cell_width;
			if( cy )
				(*cy) = cell_height;
		}
		else
		{
			if( cx )
				(*cx) = cell_width;
			if( cy )
				(*cy) = cell_height;
		}
	}

};

#if 0
int CPROC PSIBoardRefreshExtern( PCOMMON pc )
{
	ValidatedControlData( BOARD*, pb, , pc );
	if( pb )
	{
		pb->BoardRefresh();
	}
	return 1;
}
#endif

void CPROC BoardRefreshExtern( PTRSZVAL dwUser, PRENDERER renderer )
{
	BOARD *pb = (BOARD*)dwUser;
	pb->BoardRefresh();
}


int CPROC DoMouseExtern( PTRSZVAL dwUser, S_32 x, S_32 y, _32 b )
{
   BOARD *pb = (BOARD*)dwUser;
   pb->DoMouse( x, y, b );
	return 0;
}


#if 0
void CPROC BoardWindowClose( _32 dwUser )
{
	BOARD *pb;
	pb = (BOARD*)dwUser;
   //pb->pImage = NULL; // called FROM vidlib....

	if( !pb->bClosing )   // closing from BRAIN side...
		delete pb;  // okay?  maybe?
}
#endif

void BOARD::timer( void )
{
	EnterCriticalSec( &cs );
	if( pControl )
		SmudgeCommon( pControl );
	LeaveCriticalSec( &cs );
}

void CPROC BoardRefreshTimer( PTRSZVAL psv )
{
	BOARD *board = (BOARD*)psv;
	board->timer();
}


PSI_CONTROL BOARD::GetControl( void )
{
   return pControl;
}

void BOARD::Init( void )
{
	if( !g.pii )
		g.pii = GetImageInterface();
	if( !g.pri )
		g.pri = GetDisplayInterface();
	peices = NULL;
	cell_width = 16;
	cell_height = 16;
	board_origin_x = 0;
	board_origin_y = 0;
	scale = 0;
	default_peice = NULL;
	board = NULL;
	mouse_current_layer = NULL;
	route_current_layer = NULL;
	move_current_layer = NULL;
	flags.bSliding = 0;
	flags.bDragging = 0;
	flags.bLockLeft = 0;
	flags.bLeftChanged = 0;
	flags.bLeft = 0;
	flags.bLockRight = 0;
	flags.bRightChanged = 0;
	flags.bRight = NULL;
	LayerPool = NULL;//new LAYERSET;
	LayerDataPool = NULL;//new LAYER_DATASET;
	RootLayer = NULL;
	pDisplay = NULL;
	pControl = NULL;
	pImage = NULL;
	OnClose = NULL;
	InitializeCriticalSec( &cs );
}

BOARD::BOARD()
{
	Init();

	{
		_32 w, h;
		GetDisplaySize( &w, &h );
		pDisplay = OpenDisplaySizedAt( 0, w, h, 0, 0 );
	}
	//PSI_CONTROL frame = CreateFrameFromRenderer( WIDE("Brain Editor"), BORDER_RESIZABLE, pDisplay );
	update = new UPDATE( pDisplay );

	SetMouseHandler( pDisplay, DoMouseExtern, (_32)this );
	//SetCloseHandler( pDisplay, BoardWindowClose, (_32)this );

	SetRedrawHandler( pDisplay, BoardRefreshExtern, (_32)this );
   //AddCommonDraw( frame, PSIBoardRefreshExtern );

	SetBlotMethod( BLOT_MMX );

	AddTimer( 250, BoardRefreshTimer, (PTRSZVAL)this );
	UpdateDisplay( pDisplay );
	//BoardRefresh();
	// may seem redundant but I think that this is
	// needed to unhide the initial window...
	//UpdateDisplay( pDisplay );
	//timerID = AddTimer( 1000, Timer, (PTRSZVAL)this );
}

extern CONTROL_REGISTRATION board_control; // forward declaration so we have the control ID
//BOARD *creating_board;


int CPROC DrawBrainEditorControl( PSI_CONTROL pc )
{
	ValidatedControlData( BOARD **, board_control.TypeID, ppBoard, pc );
	if( ppBoard )
	{
		(*ppBoard)->BoardRefresh();
	}
	return 1;
}

int CPROC MouseBrainEditorControl( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( class BOARD * *, board_control.TypeID, ppBoard, pc );
	if( ppBoard )
	{
		(*ppBoard)->DoMouse( x, y, b );
		return 1;
	}
	return 0;
}

CONTROL_REGISTRATION board_control = { WIDE("Brain Edit Control"), { { 256, 256 }, sizeof( class BOARD * ), BORDER_RESIZABLE }
												 , NULL /* InitBrainEditorControl /* int CPROC init(PSI_CONTROL) */
												 , NULL /* load*/
												 , DrawBrainEditorControl
												 , MouseBrainEditorControl
												 , NULL // key
												 , NULL // destroy
												 , NULL // prop_page
												 , NULL // apply_page
												 , NULL // save
												 , NULL // Added a control
												 , NULL // changed caption
												 , NULL // focuschanged
};
PRELOAD( RegisterBoardControl )
{
   DoRegisterControl( &board_control );
}

BOARD::BOARD(PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
	BOARD::Init();
	pControl = MakeCaptionedControl( parent, board_control.TypeID
		, x, y, w, h
		, -1/*ID*/, WIDE("Brain Editor") );
	{
		ValidatedControlData( BOARD **, board_control.TypeID, ppBoard, pControl );
		if( ppBoard )
			(*ppBoard) = this;
	}
	update = new UPDATE( pControl );
	iTimer = AddTimer( 250, BoardRefreshTimer, (PTRSZVAL)this );
	DisplayFrame( pControl );
}

BOARD::~BOARD()
{
	if( OnClose )
		OnClose( psvClose, this );
	RemoveTimer( iTimer );
	delete update;
	DestroyFrame( &pControl );
}

void BOARD::SetCloseHandler( void (CPROC*f)(PTRSZVAL,class IBOARD*)
								, PTRSZVAL psv )
{
	this->OnClose = f;
	this->psvClose = psv;
}
	


void BOARD::DrawLayer( PLAYER layer )
{
	layer->Draw( this
				  , pDisplay?GetDisplayImage( pDisplay ):pControl?GetControlSurface( pControl ):NULL
				  , SCREEN_PAD + ( board_origin_x + (layer->x) ) * cell_width
				  , SCREEN_PAD + ( board_origin_y + (layer->y) ) * cell_height
				  );
	update->add( SCREEN_PAD + ( board_origin_x + (layer->x) ) * cell_width
		, SCREEN_PAD + ( board_origin_y + (layer->y) ) * cell_height
		, cell_width, cell_height );
}

void BOARD::BoardRefresh( void )  // put current board on screen.
	{
		int x,y;
		EnterCriticalSec( &cs );
		pImage= pDisplay?GetDisplayImage( pDisplay ):pControl?GetControlSurface( pControl ):NULL;
		ClearImage( pImage );
		// 8 border top, bottom(16),left,right(16)
		{
			_32 old_width = board_width;
			_32 old_height = board_height;
			board_width = ( pImage->width - (2*SCREEN_PAD) + ( cell_width-1) ) / cell_width;
			board_height = ( pImage->height - (2*SCREEN_PAD) + (cell_height-1) ) / cell_height;
			if( old_width != board_width || old_height != board_height )
			{
				if( board )
					Release( board );
				board = (PCELL*)Allocate( sizeof( CELL ) * board_width*board_height );
			}
		}
		if( default_peice )
		{
			_32 rows,cols;
			S_32 sx, sy;
			default_peice->getsize( &rows, &cols );

			if( board_origin_x >= 0 )
				sx = board_origin_x % cols;
			else
				sx = -(-board_origin_x % (S_32)cols);

			if( sx >= 0 )
				sx -= cols;

			if( board_origin_y >= 0 )
				sy = board_origin_y % rows;
			else
				sy = -(-board_origin_y % (S_32)rows);

			if( sy >= 0 )
				sy -= rows;

			update->add( SCREEN_PAD
						  , SCREEN_PAD
						  , (board_width+1) * cell_width, (board_height+1) * cell_height );
			for( x = sx; x < (signed)board_width; x += cols )
				for( y = sy; y < (signed)board_height; y += rows )
				{
					default_peice->methods->Draw( default_peice_instance
														 , pImage
														 , default_peice->getimage(scale)
														 , x * cell_width + SCREEN_PAD
														 , y * cell_height + SCREEN_PAD
														 );
				}
		}
		if( LayerPool )
		{
			PLAYER layer = GetSetMember( LAYER, &LayerPool, 0 );
			while( layer && (layer = layer->prior) )
			{
				DrawLayer( (PLAYER)layer );
			}
		}
		//LayerPool->forall( faisDrawLayer, (PTRSZVAL)this );
		//ForAllInSet( LAYER, LayerPool, faisDrawLayer, (PTRSZVAL)this );
		update->flush();
		LeaveCriticalSec( &cs );
	}

void BOARD::Close( void )
{
	// Implementation of IBOARD::Close virtual function
	delete this;
}

void BOARD::PutPeice( PIPEICE peice, S_32 x, S_32 y, PTRSZVAL psv )
{
	//PTRSZVAL psv = peice->Create();
   // at some point I have to instance the peice to have a neuron...
	_32 rows, cols;
	S_32 hotx, hoty;
	if( !peice ) {
		lprintf( WIDE("PEICE IS NULL!") );
		return;
	}
	EnterCriticalSec( &cs );
	peice->getsize( &rows, &cols );
	peice->gethotspot( &hotx, &hoty );
	lprintf( WIDE("hotspot offset of created cell is %d,%d so layer covers from %d,%d to %d,%d,")
			 , hotx, hoty
			 , x-hotx, y-hoty
			 , x-hotx+cols, y-hoty+rows );
	peice->psvCreate = psv; // kinda the wrong place for this but we abused this once upon a time.
	PLAYER pl = new(&LayerPool,&LayerDataPool) LAYER( peice, x, y, hotx, hoty, cols, rows );
	//pl->pLayerData = new(&LayerDataPool) LAYER_DATA(peice);
	// should be portioned...
	LeaveCriticalSec( &cs );
	SmudgeCommon( pControl );
	//BoardRefresh();
	//UpdateDisplay( pDisplay );
}

PIBOARD CreateBoard( void )
{
   return new BOARD();
}

PIBOARD CreateBoardControl( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
   return new BOARD(parent, x, y, w, h);
}


PIPEICE BOARD::CreatePeice( CTEXTSTR name //= WIDE("A Peice")
								  , Image image //= NULL
								  , int rows //= 1
								  , int cols //= 1
								  , int hotspot_x
								  , int hotspot_y
								  , PPEICE_METHODS methods //= NULL
								  , PTRSZVAL psv
								  )
{
	PIPEICE peice = DoCreatePeice( this, name, image, rows, cols, hotspot_x, hotspot_y, methods, psv );
	AddLink( &peices, peice );
	return peice; // should be able to auto cast this...
}

PIVIA BOARD::CreateVia( CTEXTSTR  name //= WIDE("A Peice")
											 , Image image //= NULL
											 , PVIA_METHODS methods //= NULL
											 , PTRSZVAL psv
											 )
{
	PIVIA via = DoCreateVia( this, name, image, methods, psv );
	AddLink( &peices, (PIPEICE)via );
	return via;
}

PIPEICE BOARD::GetFirstPeice( INDEX *idx )
{
	PIPEICE peice;
	if( !idx )
		return NULL;
	( *idx ) = 0;
	LIST_FORALL( peices, (*idx), PIPEICE, peice )
		return peice;
	return NULL;
}

PIPEICE BOARD::GetNextPeice( INDEX *idx )
{
	PIPEICE peice;
	if( !idx )
		return NULL;
	LIST_NEXTALL( peices, (*idx), PIPEICE, peice )
		return peice;
	return NULL;
}


struct save_struct
{
	PODBC odbc;
   INDEX iBoard;
};


PTRSZVAL CPROC SaveLayer( POINTER p, PTRSZVAL psv )
{
	PLAYER layer = (PLAYER)p;
	if( layer->pLayerData )
	{
		struct save_struct *save_struct = (struct save_struct*)psv;
		INDEX iLayer = layer->Save( save_struct->odbc );
		SQLCommandf( save_struct->odbc, WIDE("insert into board_layer_link (board_info_id,board_layer_id) values (%lu,%lu)")
					  , save_struct->iBoard
					  , iLayer );
	}
	return 0;
}

PTRSZVAL CPROC BeginSaveLayer( POINTER p, PTRSZVAL psv )
{
	PLAYER layer = (PLAYER)p;
	struct save_struct *save_struct = (struct save_struct*)psv;
	if( layer->iLayer && layer->iLayer != INVALID_INDEX )
	{
		SQLCommandf( save_struct->odbc, WIDE("delete from board_layer_path where board_layer_id=%lu"), layer->iLayer );
		layer->pLayerData->peice->SaveBegin( save_struct->odbc, layer->pLayerData->psvInstance );
	}
	layer->iLayer = 0; // or invalid_index
	return 0;
}

INDEX BOARD::Save( PODBC odbc, CTEXTSTR boardname )
{
	struct save_struct save_struct;
	CTEXTSTR result;
	save_struct.odbc = odbc;
	if( SQLQueryf( odbc, &result, WIDE("select board_info_id from board_info where board_name=\'%s\'"), EscapeString( boardname ) )
		&& result )
	{
		save_struct.iBoard = atoi( result );
		SQLEndQuery(odbc);
	}
	else
	{
		SQLCommandf( odbc, WIDE("insert into board_info (board_name) values (\'%s\')"), EscapeString( boardname ) );
		save_struct.iBoard = FetchLastInsertID( odbc, NULL, NULL );
	}
	//SQLCommandf( odbc, WIDE("update board_info ") );
	SQLCommandf( odbc, WIDE("delete from board_layer_link where board_info_id = %lu"), save_struct.iBoard );

	ForAllInSet( LAYER, LayerPool, BeginSaveLayer, (PTRSZVAL)&save_struct );
	ForAllInSet( LAYER, LayerPool, SaveLayer, (PTRSZVAL)&save_struct );

	//LayerPool->forall( BeginSaveLayer, (PTRSZVAL)&save_struct );
	//LayerPool->forall( SaveLayer, (PTRSZVAL)&save_struct );
	return save_struct.iBoard;
}

PTRSZVAL CPROC DeleteSaveLayer( POINTER p, PTRSZVAL psv )
{
	PLAYER l = (PLAYER)p;
	delete l;
	return 0;
}

void BOARD::reset( void )
{
	ForAllInSet( LAYER, LayerPool, DeleteSaveLayer, 0 );
	//LayerPool->forall( DeleteSaveLayer, 0 );

}

PIPEICE GetPeice( PLIST peices, CTEXTSTR peice_name )
{
	PIPEICE peice_type;
	INDEX idx;
	LIST_FORALL( peices, idx, PIPEICE, peice_type )
	{
		if( strcmp( peice_type->name(), peice_name ) == 0 )
			break;
	}
	return peice_type;
}

LOGICAL BOARD::Load( PODBC odbc, CTEXTSTR boardname )
{
	struct save_struct save_struct;
	CTEXTSTR result;
	save_struct.odbc = odbc;
		
	if( SQLQueryf( odbc, &result, WIDE("select board_info_id from board_info where board_name=\'%s\'"), EscapeString( boardname ) )
		&& result )
	{
		EnterCriticalSec( &cs );
		// okay found the board's index... now to load the board itself... 
		// probably need to generate a board reset...
		reset();
		{
			CTEXTSTR *results;
			for( SQLRecordQueryf( odbc, NULL, &results, NULL
							, WIDE("select board_layer_id from board_layer_link where board_info_id=%s order by board_layer_id")
							, result );
				results;
				FetchSQLRecord( odbc, &results ) )
			{
				//PIPEICE peice_type = GetPeice( peices, results[1] );
				INDEX iLayer = IntCreateFromText( results[0] );
				PLAYER pl = (PLAYER)ForAllInSet( LAYER, this->LayerPool, CheckIsLayer, (PTRSZVAL)iLayer );
				//(PLAYER)this->LayerPool->forall( CheckIsLayer, iLayer );
				if( !pl )
				{
					pl = new(&LayerPool,&LayerDataPool) LAYER( odbc, peices, (INDEX)atoi(results[0]) );
				}
			}
		}
		SQLEndQuery( odbc );
		LeaveCriticalSec( &cs );
		return TRUE;
	}
	return FALSE;
}



