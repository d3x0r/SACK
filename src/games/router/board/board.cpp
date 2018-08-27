#define BOARD_MAIN_SOURCE
#include "interface.h"
#include <render.h>
#include <controls.h>
#include <timers.h>
#include <configscript.h>
#include <psi.h>

#include "board.hpp"
#include "global.h"
#ifdef __WATCOMC__
#ifdef __cplusplus
#pragma inline_depth(0)
#endif
#endif

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
void CPROC BoardRefreshExtern( uintptr_t dwUser, PRENDERER renderer );
void CPROC BoardWindowClose( uint32_t dwUser );
void CPROC BoardRefreshExtern( uintptr_t dwUser, PRENDERER renderer );

//----------------------------------------------------------------------
typedef class UPDATE *PUPDATE;
class UPDATE {
	int32_t _x, _y;
	uint32_t _wd, _ht;
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
	void add( int32_t x, int32_t y, uint32_t w, uint32_t h )
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
      /*
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
      */
	}
};


uintptr_t CPROC faisIsLayerAt( void *_layer, uintptr_t psv )
{
	struct xy{
		int32_t x, y;
      PLAYER not_layer;
	} *pxy = (struct xy*)psv;
	PLAYER layer = (PLAYER)_layer;
	if( !layer->IsLayerAt( &pxy->x, &pxy->y ) )
		return 0;
	if( layer == pxy->not_layer )
      return 0; // lie, we need to return another...
   return (uintptr_t)layer;
}

class BOARD:public IBOARD
{
	uint32_t cell_width, cell_height;
	// original cell width/height
	// cell_width, height are updated to reflect scale
	uint32_t _cell_width, _cell_height;
	uintptr_t default_peice_instance;
	PIPEICE default_peice;
   PIPEICE selected_peice;
	void (CPROC*OnClose)(uintptr_t,class IBOARD*);
	uintptr_t psvClose;
public:
	INDEX Save( PODBC odbc, CTEXTSTR name );
	LOGICAL Load( PODBC odbc, CTEXTSTR name );
	void reset( void );
	CRITICALSECTION cs;

	PSI_CONTROL pControl;
	PUPDATE update;
private:
	PLIST peices;
	INDEX iTimer; // this is the timer used for board refreshes...
	//{
		PRENDERER pDisplay;
		Image pImage;  // this is implied to be pDipslay->surface or pControl->surface
	//}
	PLAYERSET LayerPool;
	PLAYER_DATASET LayerDataPool;

	// current layer which has a mouse event dispatched to it.
	PLAYER mouse_current_layer;
	PLAYER route_current_layer;
	PLAYER move_current_layer;
	PLAYER size_current_layer;
	int32_t xStart, yStart, wX, wY;
	uint32_t board_width, board_height;
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
	int32_t board_origin_x, board_origin_y; // [0][0] == this coordinate.

	struct {
		uint32_t bSliding : 1;
		uint32_t bSlid : 1; // if sliding, and the delta changes, then we're slid.
		uint32_t bDragging : 1;
		uint32_t bSizing : 1;
		uint32_t fSizeCorner : 4; // value from DIR (-1, 0-7)
	   uint32_t bLockLeft : 1;
		uint32_t bLockRight : 1;
		uint32_t bLeft : 1;
		uint32_t bRight : 1;
		// left changed happend both when a button is clicked
		// and when it is unclicked.
		uint32_t bLeftChanged : 1;
		uint32_t bRightChanged : 1;
	} flags;
	int scale;
	struct {
		PIPEICE viaset;
		int32_t _x, _y;
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
   void SetSelectedTool( PIPEICE peice );
	PLAYER CreateActivePeice( int32_t x, int32_t y, uintptr_t psvCreate )
	{
      if( selected_peice )
			return PutPeice( selected_peice, x, y, psvCreate );
      return NULL;
	}
	PLAYER CreatePeice( CTEXTSTR name, int32_t x, int32_t y, uintptr_t psvCreate )
	{
		return PutPeice( GetPeice( name ), x, y, psvCreate );
	}
private:

	void SetCloseHandler( void (CPROC*)(uintptr_t,class IBOARD*), uintptr_t );
	void SetBackground( PIPEICE peice, uintptr_t psv )
	{
		if( peice )
		{
			default_peice = peice;
			// can't pass layer data for background?
			default_peice_instance = default_peice->methods->Create(psv,NULL);
			BoardRedraw();
		}
	}

	void SetCellSize( uint32_t cx, uint32_t cy )
	{
		cell_width = _cell_width = cx;
		cell_height = _cell_height = cy;
	}

	int BeginPath( PIVIA viaset/*, int32_t x, int32_t y*/, uintptr_t psv )
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

	PLAYER GetLayerAt( int32_t *wX, int32_t *wY, PLAYER notlayer = NULL )
	{
		if( LayerPool )
		{
			PLAYER layer = GetSetMember( LAYER, LayerPool, 0 );
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

			//	if( ( layer = (PLAYER)LayerPool->forall( faisIsLayerAt, (uintptr_t)&xy ) ) )
			//  {
			//     (*wX) = xy.x;
			//  	(*wY) = xy.y;
			//  }
			return layer;
		}
		return NULL;
	}

void GetCurrentPeiceSize( int32_t *wX, int32_t *wY )
{
	if( mouse_current_layer )
	{
      if( wX )
			(*wX) = mouse_current_layer->w;
		if( wY )
			(*wY) = mouse_current_layer->h;
	}
}
void GetCurrentPeiceHotSpot( int32_t *wX, int32_t *wY )
{
	if( mouse_current_layer )
	{
      if( wX )
			(*wX) = mouse_current_layer->hotx;
		if( wY )
			(*wY) = mouse_current_layer->hoty;
	}
}

PLAYER_DATA GetLayerDataAt( int32_t *wX, int32_t *wY, PLAYER notlayer = NULL )
{
	PLAYER layer = GetLayerAt( wX, wY, notlayer );
	if( layer )
		return layer->pLayerData;
	return NULL;
}
// viaset is implied by route_current_layer
	void EndPath( int32_t x, int32_t y )
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
	PLAYER PutPeice( PIPEICE, int32_t x, int32_t y, uintptr_t psv );
	void BoardRefresh( void );  // put current board on screen.
	void BoardRedraw( void );  // post desire to draw board
private:

void LayPathTo( int wX, int wY )
{
	route_current_layer->LayPath( wX, wY );
	BoardRedraw();
}

public:


	void InvokeTap( int wX, int wY )
	{
		if( !route_current_layer && !flags.bSlid )
		{
			int32_t x = wX, y = wY;
			PLAYER layer = GetLayerAt( &x, &y );
			if( layer )
			{
				mouse_current_layer = layer;
				layer->pLayerData->peice->methods->OnTap( layer->pLayerData->psvInstance, x, y );
				mouse_current_layer = NULL;
			}
			else
				if( default_peice )
				{
					// this is the click (without drag)
					default_peice->methods->OnTap( default_peice_instance, x, y );

				}
		}
	}

void DoMouse( int X, int Y, int b )
{
	//static _left, _right;
#define SCRN_TO_GRID_X(x) ((x - SCREEN_PAD)/(signed)cell_width - board_origin_x )
#define SCRN_TO_GRID_Y(y) ((y - SCREEN_PAD)/(signed)cell_height - board_origin_y)

	wX = SCRN_TO_GRID_X( X );
	wY = SCRN_TO_GRID_Y( Y );
	//lprintf( WIDE("mouse at %d,%d"), wX, wY );
	{
		int32_t x = wX, y = wY;
		PLAYER_DATA pld = GetLayerDataAt( &x, &y );
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
			int32_t x = wX, y = wY;
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
				if( !default_peice->methods->OnRightClick(default_peice_instance,wX,wY) )
					return; // routine has done something to abort processing...
			}
		}
	}
	else
	{
		//_right = flags.bRight;
	}


	if( flags.bSizing )
	{
		int ok = 0;
		if( flags.bLeft )
		{
			if( wX != xStart || wY != yStart )
			{
				if( size_current_layer )
				{
               //lprintf( WIDE("Delta is like %d,%d"), wX-xStart, wY-yStart );
					switch( flags.fSizeCorner )
					{
					case UP_LEFT:
						ok = size_current_layer->MoveResizeLayer( wX-xStart, wY-yStart, xStart-wX, yStart-wY );
						break;
					case DOWN_LEFT:
						ok = size_current_layer->MoveResizeLayer( wX-xStart, 0, xStart-wX, wY-yStart );
						break;
					case UP_RIGHT:
						ok = size_current_layer->MoveResizeLayer( 0, wY-yStart, wX-xStart, yStart-wY );
						break;
					case DOWN_RIGHT:
						ok = size_current_layer->MoveResizeLayer( 0, 0, wX-xStart, wY-yStart );
						break;
					}
				}
				if( ok )
				{
					xStart = wX;
					yStart = wY;
				}
			}
		}
		else
		{
			flags.bSizing = FALSE;
		}
      return;
	}
	if( flags.bSliding )
	{
		if( ( flags.bLockLeft && flags.bLeft ) ||
			( flags.bLockRight && flags.bRight ) )
		{
			if( wX != xStart ||
				wY != yStart )
			{
				flags.bSlid = TRUE;
				lprintf( WIDE("updating board origin by %d,%d"), wX-xStart, wY-yStart );
				board_origin_x += wX - xStart;
				board_origin_y += wY - yStart;
				wX = xStart;
				wY = yStart;
				BoardRedraw( );
			}
		}
		else
		{
			if( !flags.bSlid )
			{
            InvokeTap( wX, wY );
			}
			flags.bSliding = FALSE;
			flags.bSlid = FALSE;

			flags.bLockLeft = FALSE;
			flags.bLockRight = FALSE;
		}
	}
   if( move_current_layer ) // moving a node/neuron/other...
   {
		if( flags.bLeft )
		{
			//DebugBreak();
			move_current_layer->move( wX - xStart, wY - yStart );
			xStart = wX;
			yStart = wY;
			BoardRedraw();
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
		if( !flags.bLeftChanged && flags.bLeft )
		{
			int32_t x = wX, y = wY;
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
						default_peice->methods->OnClick(default_peice_instance,wX,wY);
					}
					else
					{

					}
				}
				LayPathTo( wX, wY );
			}
			else if( layer )
			{
				PLAYER_DATA pld = layer->pLayerData;
				mouse_current_layer = layer;
				lprintf( WIDE("Generate on Drag Begin method to peice.") );
				pld->peice->methods->OnBeginDrag( pld->psvInstance, x, y );
				mouse_current_layer = NULL;
			}
			else if( default_peice )
			{
				lprintf( WIDE("Default peice begin drag.") );
            // if unhandled...
				if( default_peice->methods->OnBeginDrag(default_peice_instance,wX,wY) == 0 )
				{
					LockDrag();
				}
			}
		}
		else if( flags.bLeft )  // not drawing, not doing anything...
		{
			// find neuron center...
			// first find something to do in this cell already
			// this is 'move neuron'
			// or disconnect from...

			int32_t x = wX, y = wY;
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
						default_peice->methods->OnClick(default_peice_instance,wX,wY);
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
				default_peice->methods->OnClick(default_peice_instance,wX,wY);
			}
		}
		else if( !route_current_layer && flags.bLeftChanged && !flags.bLeft )
		{
			InvokeTap( wX, wY );
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
   BOARD(PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h );
   ~BOARD();
	PIPEICE CreatePeice( CTEXTSTR name //= WIDE("A Peice")
								  , Image image //= NULL
								  , int rows //= 1
								  , int cols //= 1
								  , int hotspot_x
								  , int hotspot_y
								  , PPEICE_METHODS methods //= NULL
								  , uintptr_t psv
								  );
	PIVIA CreateVia( CTEXTSTR name //= WIDE("A Peice")
						, Image image //= NULL
						, PVIA_METHODS methods //= NULL
					  , uintptr_t psv
						);
	PIPEICE GetFirstPeice( INDEX *idx );
	PIPEICE GetPeice( CTEXTSTR );
	PIPEICE GetNextPeice( INDEX *idx );
   void GetSize( uint32_t* cx, uint32_t* cy )
	{
		// result with the current cell size, so we know
		// how much to multiply row/column counters by.
		// X is always passed correctly?
		if( cx )
			(*cx) = board_width;
		if( cy )
         (*cy) = board_height;
	}
	void GetCellSize( uint32_t* cx, uint32_t* cy, int scale )
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

   void ConfigSetCellSize( arg_list args );
	void DefinePeiceColors( arg_list args );
	void DefineABlock( arg_list args );
	void DefineABlockNoOpt( arg_list args );
   LOGICAL LoadPeiceConfiguration( CTEXTSTR file );


	void BeginSize( int dir )
	{
      size_current_layer = mouse_current_layer;
		flags.bSizing = 1;
		flags.fSizeCorner = dir;
		xStart = wX;
		yStart = wY;
	}


};

void CPROC BoardRefreshExtern( uintptr_t dwUser, PRENDERER renderer )
{
   BOARD *pb = (BOARD*)dwUser;
   pb->BoardRefresh();
}


uintptr_t CPROC DoMouseExtern( uintptr_t dwUser, int32_t x, int32_t y, uint32_t b )
{
   BOARD *pb = (BOARD*)dwUser;
   pb->DoMouse( x, y, b );
	return 0;
}


#if 0
void CPROC BoardWindowClose( uint32_t dwUser )
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
   BoardRedraw();
}

void CPROC BoardRefreshTimer( uintptr_t psv )
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
   selected_peice = NULL;
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
   flags.bSlid = 0;
	flags.bDragging = 0;
	flags.bLockLeft = 0;
	flags.bLeftChanged = 0;
	flags.bLeft = 0;
	flags.bLockRight = 0;
	flags.bRightChanged = 0;
	flags.bRight = 0;
	LayerPool = NULL; //new LAYERSET;
	LayerDataPool = NULL; //new LAYER_DATASET;
	pDisplay = NULL;
	pControl = NULL;
	pImage = NULL;
	InitializeCriticalSec( &cs );
}

BOARD::BOARD()
{
	Init();

	{
		uint32_t w, h;
		GetDisplaySize( &w, &h );
		pDisplay = OpenDisplaySizedAt( 0, w, h, 0, 0 );
	}
	//PSI_CONTROL frame = CreateFrameFromRenderer( WIDE("Brain Editor"), BORDER_RESIZABLE, pDisplay );
	update = new UPDATE( pDisplay );

	SetMouseHandler( pDisplay, DoMouseExtern, (uintptr_t)this );
	//SetCloseHandler( pDisplay, BoardWindowClose, (uint32_t)this );

	SetRedrawHandler( pDisplay, BoardRefreshExtern, (uintptr_t)this );
   //AddCommonDraw( frame, PSIBoardRefreshExtern );

	SetBlotMethod( BLOT_MMX );

	AddTimer( 250, BoardRefreshTimer, (uintptr_t)this );
	UpdateDisplay( pDisplay );
	//BoardRefresh();
	// may seem redundant but I think that this is
	// needed to unhide the initial window...
	//UpdateDisplay( pDisplay );
	//timerID = AddTimer( 1000, Timer, (uintptr_t)this );
}

extern CONTROL_REGISTRATION board_control; // forward declaration so we have the control ID
BOARD *creating_board;


static int OnCreateCommon( WIDE("Brain Edit Control") )( PSI_CONTROL pc )
{
	ValidatedControlData( BOARD **, board_control.TypeID, ppBoard, pc );
	if( ppBoard )
	{
		SetCommonTransparent( pc, TRUE );
		if( creating_board )
		{
			creating_board->update = new UPDATE( pc );
			(*ppBoard) = creating_board;
			(*ppBoard)->pControl = pc;
			creating_board = NULL;
		}
		else
		{
			(*ppBoard) = new BOARD();
			(*ppBoard)->pControl = pc;
			(*ppBoard)->update = new UPDATE( pc );
		}
	}
	// hrm how do I set this data ?
   return TRUE;
}

static int OnDrawCommon( WIDE("Brain Edit Control") )( PSI_CONTROL pc )
{
	ValidatedControlData( BOARD **, board_control.TypeID, ppBoard, pc );
	if( ppBoard )
	{
		(*ppBoard)->BoardRefresh();
	}
	return 1;
}

static int OnMouseCommon( WIDE("Brain Edit Control") )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( class BOARD * *, board_control.TypeID, ppBoard, pc );
	if( ppBoard )
	{
		(*ppBoard)->DoMouse( x, y, b );
		return 1;
	}
	return 0;
}

void CPROC BoardPositionChanging( PSI_CONTROL pc, LOGICAL bStart )
{
	ValidatedControlData( class BOARD * *, board_control.TypeID, ppBoard, pc );
	if( ppBoard )
	{
		if( bStart )
			EnterCriticalSec( &(*ppBoard)->cs );
		else
			LeaveCriticalSec( &(*ppBoard)->cs );
	}
}
CONTROL_REGISTRATION board_control = { WIDE("Brain Edit Control"), { { 256, 256 }, sizeof( class BOARD * ), BORDER_RESIZABLE }
												 , NULL //InitBrainEditorControl /* int CPROC init(PSI_CONTROL) */
												 , NULL /* load*/
												 , NULL //DrawBrainEditorControl
												 , NULL //MouseBrainEditorControl
												 , NULL // key
												 , NULL // destroy
												 , NULL // prop_page
												 , NULL // apply_page
												 , NULL // save
												 , NULL // Added a control
												 , NULL // changed caption
												 , NULL // focuschanged
                                     , BoardPositionChanging
                                     , 0 // typeID
};
PRELOAD( RegisterBoardControl )
{
   DoRegisterControl( &board_control );
   g.pii = GetImageInterface();
   g.pri = GetDisplayInterface();
}

BOARD::BOARD(PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
	BOARD::Init();
	creating_board = this;
	pControl = MakeControl( parent, board_control.TypeID
		, x, y, w, h
		, -1/*ID*/ );
 	{
		ValidatedControlData( BOARD **, board_control.TypeID, ppBoard, pControl );
		if( ppBoard )
			(*ppBoard) = this;
	}
	iTimer = AddTimer( 250, BoardRefreshTimer, (uintptr_t)this );
	creating_board = NULL;
	//DisplayFrame( pControl );
}

BOARD::~BOARD()
{
	if( OnClose )
		OnClose( psvClose, this );
	RemoveTimer( iTimer );
	delete update;
	DestroyFrame( &pControl );
}

void BOARD::SetCloseHandler( void (CPROC*f)(uintptr_t,class IBOARD*)
								, uintptr_t psv )
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
	//int32_t hotx, hoty;
	//uint32_t rows, cols;
   //PIPEICE peice = layer->GetPeice();
	//peice->gethotspot( &hotx, &hoty );
	// later, when I get more picky, only draw those cells that changed
   // which may include an offset
	//peice->getsize( &rows, &cols );
   //lprintf( WIDE("Drawing layer at %d,%d (%d,%d) origin at %d,%d"), layer->x, layer->y, hotx, hoty, board_origin_x, board_origin_y );
	//peice->methods->Draw( layer->pLayerData->psvInstance
	//						  , GetDisplayImage( pDisplay )
	//						  , SCREEN_PAD + ( board_origin_x + (layer->x) ) * cell_width
	//						  , SCREEN_PAD + ( board_origin_y + (layer->y) ) * cell_height
	//						  , rows, cols );
}

/*
uintptr_t CPROC faisDrawLayer( void *layer, uintptr_t psv )
{
	BOARD *_this = (BOARD*)psv;
	_this->DrawLayer( (PLAYER)layer );
   return 0;
}
*/
void BOARD::BoardRedraw( void )  // put current board on screen.
{
	if( pControl )
		SmudgeCommon( pControl );
	if( pDisplay )
      Redraw( pDisplay );
}
void BOARD::BoardRefresh( void )  // put current board on screen.
	{
		int x,y;
		EnterCriticalSec( &cs );
		pImage = pDisplay?GetDisplayImage( pDisplay ):pControl?GetControlSurface( pControl ):NULL;
		ClearImageTo( pImage, BASE_COLOR_BLACK );
		//ClearImage( pImage );
		// 8 border top, bottom(16),left,right(16)
		{
			uint32_t old_width = board_width;
			uint32_t old_height = board_height;
			board_width = ( pImage->width - (2*SCREEN_PAD) ) / cell_width;
			board_height = ( pImage->height - (2*SCREEN_PAD) ) / cell_height;
			if( old_width != board_width || old_height != board_height )
			{
				if( board )
					Release( board );
				board = (PCELL*)Allocate( sizeof( CELL ) * board_width*board_height );
			}
		}
		if( default_peice )
		{
			uint32_t rows,cols;
			int32_t sx, sy;
			default_peice->getsize( &rows, &cols );

			if( board_origin_x >= 0 )
				sx = board_origin_x % cols;
			else
				sx = -(-board_origin_x % (int32_t)cols);

			if( sx > 0 )
				sx -= cols;

			if( board_origin_y >= 0 )
				sy = board_origin_y % rows;
			else
				sy = -(-board_origin_y % (int32_t)rows);

			if( sy > 0 )
				sy -= rows;

			update->add( SCREEN_PAD
						  , SCREEN_PAD
						  , (board_width+1) * cell_width, (board_height+1) * cell_height );
			for( x = sx; x < (signed)board_width; x += cols )
				for( y = sy; y < (signed)board_height; y += rows )
				{
					//lprintf( WIDE("background") );
					default_peice->methods->Draw( default_peice_instance
														 , pImage
														 , default_peice->getimage(scale)
														 , x * cell_width + SCREEN_PAD
														 , y * cell_height + SCREEN_PAD
														 );
				}
			//UpdateDisplay( pDisplay );
		}
      if( LayerPool )
		{
			PLAYER layer = GetSetMember( LAYER, LayerPool, 0 );
			while( layer && (layer = layer->prior) )
			{
				DrawLayer( (PLAYER)layer );
			}
		}
		//LayerPool->forall( faisDrawLayer, (uintptr_t)this );
		//ForAllInSet( LAYER, LayerPool, faisDrawLayer, (uintptr_t)this );
		update->flush();
		LeaveCriticalSec( &cs );
      //UpdateDisplay( pDisplay );
	}

void BOARD::Close( void )
{
	// Implementation of IBOARD::Close virtual function
	delete this;
}

PLAYER BOARD::PutPeice( PIPEICE peice, int32_t x, int32_t y, uintptr_t psv )
{
	//uintptr_t psv = peice->Create();
   // at some point I have to instance the peice to have a neuron...
	uint32_t rows, cols;
	int32_t hotx, hoty;
	if( !peice ) {
		lprintf( WIDE("PEICE IS NULL!") );
		return NULL;
	}
	EnterCriticalSec( &cs );
	peice->getsize( &rows, &cols );
	peice->gethotspot( &hotx, &hoty );
	lprintf( WIDE("hotspot offset of created cell is %d,%d so layer covers from %d,%d to %d,%d,")
			 , hotx, hoty
			 , x-hotx, y-hoty
			 , x-hotx+cols, y-hoty+rows );
	peice->psvCreate = psv; // kinda the wrong place for this but we abused this once upon a time.
	PLAYER pl;
	if( mouse_current_layer )
	{
		PLAYER tmp = mouse_current_layer;
		while( tmp )
		{
			x += tmp->x;
			y += tmp->y;
         tmp = tmp->stacked_on;
		}
		pl = new(&LayerPool,&LayerDataPool) LAYER( peice, x, y, hotx, hoty, cols, rows );
		// stack accept?

		pl->stacked_on = mouse_current_layer;
		AddLink( &mouse_current_layer->holding, pl );
	}
	else
	{
		pl = new(&LayerPool,&LayerDataPool) LAYER( peice, x, y, hotx, hoty, cols, rows );
	}
	//pl->pLayerData = new(&LayerDataPool) LAYER_DATA(peice);
	// should be portioned...
	LeaveCriticalSec( &cs );
	BoardRefresh();
   return pl;
}

PIBOARD CreateBoard( void )
{
   return new BOARD();
}

PSI_CONTROL CreateBoardControl( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
   return (new BOARD(parent, x, y, w, h))->GetControl();
}

PIBOARD GetBoardFromControl( PSI_CONTROL pc )
{
	ValidatedControlData( class BOARD * *, board_control.TypeID, ppBoard, pc );
	return (*ppBoard);
}


PIPEICE BOARD::CreatePeice( CTEXTSTR name //= WIDE("A Peice")
								  , Image image //= NULL
								  , int rows //= 1
								  , int cols //= 1
								  , int hotspot_x
								  , int hotspot_y
								  , PPEICE_METHODS methods //= NULL
								  , uintptr_t psv
								  )
{
	PIPEICE peice = DoCreatePeice( this, name, image, rows, cols, hotspot_x, hotspot_y, methods, psv );
	AddLink( &peices, peice );
	return peice; // should be able to auto cast this...
}

PIVIA BOARD::CreateVia( CTEXTSTR name //= WIDE("A Peice")
											 , Image image //= NULL
											 , PVIA_METHODS methods //= NULL
											 , uintptr_t psv
											 )
{
	PIVIA via = DoCreateVia( this, name, image, methods, psv );
	AddLink( &peices, (PIPEICE)via );
	return via;
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


PIPEICE BOARD::GetPeice( CTEXTSTR name )
{
	return ::GetPeice( peices, name );
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


uintptr_t CPROC SaveLayer( POINTER p, uintptr_t psv )
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

uintptr_t CPROC BeginSaveLayer( POINTER p, uintptr_t psv )
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
   CheckTables( odbc );

	if( SQLQueryf( odbc, &result, WIDE("select board_info_id from board_info where board_name=\'%s\'"), EscapeString( boardname ) )
		&& result )
	{
		save_struct.iBoard = atoi( result );
		PopODBCEx(odbc);
	}
	else
	{
		SQLCommandf( odbc, WIDE("insert into board_info (board_name) values (\'%s\')"), EscapeString( boardname ) );
		save_struct.iBoard = FetchLastInsertID( odbc, NULL, NULL );
	}
	//SQLCommandf( odbc, WIDE("update board_info ") );
	SQLCommandf( odbc, WIDE("delete from board_layer_link where board_info_id = %lu"), save_struct.iBoard );

	ForAllInSet( LAYER, LayerPool, BeginSaveLayer, (uintptr_t)&save_struct );
	ForAllInSet( LAYER, LayerPool, SaveLayer, (uintptr_t)&save_struct );

	//LayerPool->forall( BeginSaveLayer, (uintptr_t)&save_struct );
	//LayerPool->forall( SaveLayer, (uintptr_t)&save_struct );
	return save_struct.iBoard;
}

uintptr_t CPROC DeleteSaveLayer( POINTER p, uintptr_t psv )
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
				PLAYER pl = (PLAYER)ForAllInSet( LAYER, this->LayerPool, CheckIsLayer, (uintptr_t)iLayer );
				//(PLAYER)this->LayerPool->forall( CheckIsLayer, iLayer );
				if( !pl )
				{
					PushSQLQueryEx( odbc );
					pl = new(&LayerPool,&LayerDataPool) LAYER( odbc, peices, (INDEX)atoi(results[0]) );
					//LAYER( peice_type, psv );

					//pl->Load( odbc, atoi( results[0] ) );
					PopODBCEx( odbc );
				}
			}
		}
		LeaveCriticalSec( &cs );
		return TRUE;
	}
	return FALSE;
}


void BOARD::ConfigSetCellSize( arg_list args )
{
	PARAM( args, int64_t, x );
	PARAM( args, int64_t, y );
	SetCellSize( (int)x, (int)y );
}
uintptr_t CPROC ConfigSetCellSize( uintptr_t psv, arg_list args )
{
	((BOARD*)psv)->ConfigSetCellSize( args );
	return psv;
}

//---------------------------------------------------
void BOARD::DefinePeiceColors( arg_list args )
{
	PARAM( args, char *, type );
	PARAM( args, uint64_t, input_or_threshold );
	PARAM( args, CDATA, c1 );
	PARAM( args, CDATA, c2 );
	PARAM( args, CDATA, c3 );
//   PPEICE peice = GetPeice( type );
}
uintptr_t CPROC DefinePeiceColors( uintptr_t psv, arg_list args )
{
	BOARD *brainboard = (BOARD*)psv;
	brainboard->DefinePeiceColors( args );
	return psv;
}



#define GenericInvoke( root, method, ret_type, args, ... )   \
	{\
		ret_type (CPROC *f)args;\
		TEXTCHAR keyname[256];\
		PCLASSROOT data = NULL;\
		CTEXTSTR name;\
		tnprintf( keyname, sizeof( keyname ), root WIDE("/%s/") method, method_name );\
		for( name = GetFirstRegisteredName( keyname, &data );\
			 name;\
			  name = GetNextRegisteredName( &data ) )\
		{\
			f = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,ret_type,name,args);\
			if( f )\
			{\
				return f(__VA_ARGS__);\
			}\
		}\
		return 0;\
	}\


#define GenericVoidInvoke( root, method, args, ... )   \
	{\
		void (CPROC *f)args;\
		TEXTCHAR keyname[256];\
		PCLASSROOT data = NULL;\
		CTEXTSTR name;\
		tnprintf( keyname, sizeof( keyname ), root WIDE("/%s/") method, method_name );\
		for( name = GetFirstRegisteredName( keyname, &data );\
			 name;\
			  name = GetNextRegisteredName( &data ) )\
		{\
			f = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,void,name,args);\
			if( f )\
			{\
				f(__VA_ARGS__);\
			}\
		}\
	}\

#define GenericVoidInvokeEx( result, root, method, args, ... )   \
	{\
		void (CPROC *f)args;\
		TEXTCHAR keyname[256];\
		PCLASSROOT data = NULL;\
		CTEXTSTR name;\
		tnprintf( keyname, sizeof( keyname ), root WIDE("/%s/") method, method_name );\
		for( name = GetFirstRegisteredName( keyname, &data );\
			 name;\
			  name = GetNextRegisteredName( &data ) )\
		{\
			f = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,void,name,args);\
			if( f )\
			{\
				f(__VA_ARGS__);\
				result = true; \
			}\
		}\
	}\




class PROCREG_VIA_INVOKE: public VIA_METHODS
{
	uintptr_t psv;
	CTEXTSTR method_name;
	PIBOARD board;
	//BRAINBOARD * brainboard;
public:
	PROCREG_VIA_INVOKE( PIBOARD board, CTEXTSTR type )
	{
      this->board = board;
		method_name = StrDup( type );

		///brainboard = newbrainboard;
	}
private:
	//PSYNAPSE synapse;
public:
	uintptr_t Create(uintptr_t psvExtra, PLAYER_DATA layer )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnCreate"), uintptr_t, (uintptr_t,PLAYER_DATA), psvExtra, layer );
		//return (uintptr_t)(brainboard->brain->DupSynapse( brainboard->DefaultSynapse ));
	}
	void Destroy( uintptr_t psv )
	{
		GenericVoidInvoke( WIDE("/automaton/board"), WIDE("OnDestroy"), (uintptr_t), psv );
	}
	int Disconnect( uintptr_t psv )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnDisconnect"), int, (uintptr_t), psv );
		return TRUE;
	}
	int OnRightClick( uintptr_t psv, int32_t x, int32_t y )
	{
      GenericVoidInvoke( WIDE("/automaton/board"), WIDE("Properties"), (uintptr_t,PSI_CONTROL), psv, board->GetControl() );
		//ShowSynapseDialog( (PSYNAPSE)psv );
		return 1;
	}
//	PEICE_PROC( int, OnClick )( uintptr_t psv, int x, int y )
//	{
//		lprintf( WIDE("syn") );
 //     return 0;
 //  }
	void SaveBegin( PODBC odbc, uintptr_t psvInstance )
	{
		//SQLCommandf( odbc, WIDE("delete from board_layer_neuron where board_layer_id=%lu"), iParent );
		//PSYNAPSE synapse = (PSYNAPSE)psvInstance;
		//synapse->SaveBegin( odbc );
	}
	INDEX Save( PODBC odbc, INDEX iParent, uintptr_t psvInstance )
	{
		//SQLCommandf( odbc, WIDE("delete from board_layer_synapse where board_layer_id=%lu"), iParent );
		//INDEX iBrainSynapse = 
		//return ((PSYNAPSE)psvInstance)->Save( odbc, iParent );
		//SQLInsert( odbc, WIDE("board_layer_synapse")
		//			, WIDE("board_layer_id"), 2, iParent
		//			, WIDE("brain_synapse_id"), 2, iBrainSynapse
		//			, NULL, 0, NULL );
		//return FetchLastInsertID( odbc, NULL, NULL );
      return INVALID_INDEX;
	}
	uintptr_t Load( PODBC odbc, INDEX iInstance )
	{
		//PSYNAPSE synapse = brainboard->brain->GetSynapse();
		//synapse->Load( odbc, 0, iInstance );
		//return (uintptr_t)synapse;
      return INVALID_INDEX;
	}
};

class PROCREG_INVOKE: public PEICE_METHODS
{
	uintptr_t psv;
	CTEXTSTR method_name;
   PIBOARD board;
public:
	PROCREG_INVOKE( PIBOARD board, CTEXTSTR name, uintptr_t psv )
	{
		this->board = board;
		this->method_name = StrDup( name );
      this->psv = psv;
	}
private:
public:

	uintptr_t Create(uintptr_t psvExtra, PLAYER_DATA layer )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnCreate"), uintptr_t, (uintptr_t,PLAYER_DATA), psvExtra, layer );
	}

	void Destroy( uintptr_t asdf )
	{
      GenericVoidInvoke( WIDE("/automaton/board"), WIDE("OnDestroy"), (uintptr_t), asdf );
	}
	PEICE_PROC( void, Properties )( uintptr_t psv, PSI_CONTROL parent )
	{
      	GenericVoidInvoke( WIDE("/automaton/board"), WIDE("Properties"), (uintptr_t,PSI_CONTROL), psv, parent );

	}
	PEICE_PROC( int, Connect )( uintptr_t psvTo
				  , int rowto, int colto
				  , uintptr_t psvFrom
				  , int rowfrom, int colfrom )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("connect"), int
						 , (uintptr_t,int,int,uintptr_t,int,int)
						 , psvTo, rowto, colto, psvFrom, rowfrom, colfrom );
	}

	void Update( uintptr_t psv, uint32_t cycle )
	{
		GenericVoidInvoke( WIDE("/automaton/board"), WIDE("update"), (uintptr_t,uint32_t), psv, cycle );
	}

	int OnBeginDrag( uintptr_t psv, int32_t x, int32_t y )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnMouseBeginDrag"), int, (uintptr_t,int32_t,int32_t), psv, x, y );
      	return 0;
	}

	int OnClick( uintptr_t psv, int32_t x, int32_t y )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnMouseDown"), int, (uintptr_t,int32_t,int32_t), psv, x, y );
		//board->LockDrag();
		return 0;
	}
	int OnTap( uintptr_t psv, int32_t x, int32_t y )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnMouseTap"), int, (uintptr_t,int32_t,int32_t), psv, x, y );
		//board->LockDrag();
		return 0;
	}
	int OnRightClick( uintptr_t psv, int32_t x, int32_t y )
	{
		GenericVoidInvoke( WIDE("/automaton/board"), WIDE("Properties"), (uintptr_t,PSI_CONTROL), psv,board->GetControl() );
		return 1;
	}
	void  Draw( uintptr_t psvInstance, Image surface, Image peice, int32_t x, int32_t y )
	{
		bool result = false;
		GenericVoidInvokeEx( result, WIDE("/automaton/board"), WIDE("OnDraw"), (uintptr_t,Image,Image,int32_t,int32_t), psvInstance, surface, peice, x, y );
		if( !result )
		{
			PEICE_METHODS::Draw( psvInstance, surface, peice, x, y );
		}
	}

	int OnDoubleClick( uintptr_t psv, int32_t x, int32_t y )
	{
		return 0;
	}

	int ConnectEnd( uintptr_t psv_to_instance, int32_t x, int32_t y
									  , PIPEICE peice_from, uintptr_t psv_from_instance )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnEndConnect"), int, (uintptr_t,int32_t,int32_t,PIPEICE,uintptr_t)
						 , psv_to_instance, x, y, peice_from, psv_from_instance );
	}
	int ConnectBegin( uintptr_t psv_to_instance, int32_t x, int32_t y
									  , PIPEICE peice_from, uintptr_t psv_from_instance )
	{
		GenericInvoke( WIDE("/automaton/board"), WIDE("OnBeginConnect"), int, (uintptr_t,int32_t,int32_t,PIPEICE,uintptr_t)
						 , psv_to_instance, x, y, peice_from, psv_from_instance );
	}

};
//static class PROCREG_INVOKE *procreg_invoke;
//static class PROCREG_VIA_INVOKE *procreg_via_invoke;



//---------------------------------------------------
void BOARD::DefineABlock( arg_list args )
{
	PARAM( args, TEXTCHAR *, type );
	PARAM( args, uint64_t, cx );
	PARAM( args, uint64_t, cy );
	PARAM( args, TEXTCHAR *, filename );
	//PPEICE_METHODS methods = FindPeiceMethods( type );
	Image image = LoadImageFile( filename );
	if( image )
	{
		lprintf( WIDE("Make block %s : %s"), type, filename );
		PIPEICE pip = CreatePeice( type, image
										, (int)cx, (int)cy
										, ((int)cx-1)/2, ((int)cy-1)/2
										, new PROCREG_INVOKE( this, type, 0 )
										, (uintptr_t)this /* brainboard*/
										 );
	}
	else
		lprintf( WIDE("Failed to open %s"), filename );
}
uintptr_t CPROC DefineABlock( uintptr_t psv, arg_list args )
{
	BOARD *brainboard = (BOARD*)psv;
	brainboard->DefineABlock( args );
	return psv;
}

//---------------------------------------------------
void BOARD::DefineABlockNoOpt( arg_list args )
{
	PARAM( args, TEXTCHAR *, type );
	PARAM( args, TEXTCHAR *, filename );
	//PPEICE_METHODS methods = FindPeiceMethods( type );
	Image image = LoadImageFile( filename );
	lprintf( WIDE("Attempt to define via with %s"), filename );
	if( image )
		CreateVia( type, image, new PROCREG_VIA_INVOKE( this, type ), (uintptr_t)this );
}
uintptr_t CPROC DefineABlockNoOpt( uintptr_t psv, arg_list args )
{
	BOARD *brainboard = (BOARD*)psv;
	brainboard->DefineABlockNoOpt( args );
	return psv;
}

void BOARD::SetSelectedTool( PIPEICE peice )
{
   selected_peice = peice;
}

LOGICAL BOARD::LoadPeiceConfiguration( CTEXTSTR file )
{
	int result;
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	AddConfigurationMethod( pch, WIDE("cell size %i by %i"), ::ConfigSetCellSize );
	//#AddConfigurationMethod( pch, WIDE("color %w %c"), DefineAColor );
	AddConfigurationMethod( pch, WIDE("block %w (%i by %i) %p"), ::DefineABlock );
	AddConfigurationMethod( pch, WIDE("color %w %i %c %c %c"), ::DefinePeiceColors );
	AddConfigurationMethod( pch, WIDE("pathway %w %p"), ::DefineABlockNoOpt );
	lprintf( WIDE("Load %s"), file );
	result = ProcessConfigurationFile( pch, file, (uintptr_t)this );
	DestroyConfigurationHandler( pch );
	return result;
}


PUBLIC( void, ExportThis)( void )
{
}
