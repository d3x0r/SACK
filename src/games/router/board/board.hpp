#ifndef BOARD_DEFINED
#define BOARD_DEFINED
#include <controls.h>
#include "peice.hpp"
#include "layer.hpp"

//typedef class IBOARD *PIBOARD;
class IBOARD
{
public:
#undef SetCloseHandler
	virtual void SetCloseHandler( void (CPROC*)(uintptr_t,class IBOARD*), uintptr_t ) = 0;
	virtual void Close( void ) = 0;
	// put a peice defined by PPEICE with part referenced by
	// row, col at the position x, y

	// add a avaiable peice to the board...
	//virtual void AddPeice( PIPEICE peice );
	virtual PIPEICE CreatePeice( CTEXTSTR name
										, Image image = NULL
										, int rows = 1, int cols = 1
										, int hotspot_x = 0
										, int hotspot_y = 0
										 // force images to this size
										, PPEICE_METHODS methods = NULL 
										, uintptr_t psvCreate = 0 ) = 0;

	virtual PIVIA CreateVia( CTEXTSTR name
											, Image image = NULL
											, PVIA_METHODS methods = NULL 
										, uintptr_t psvCreate = 0 ) = 0;
	virtual PIPEICE GetFirstPeice( INDEX *idx ) = 0;
	virtual PIPEICE GetNextPeice( INDEX *idx ) = 0;
	virtual PIPEICE GetPeice( CTEXTSTR name ) = 0;
	virtual void GetSize( uint32_t* cx, uint32_t* cy ) = 0;
	virtual void SetCellSize( uint32_t cx, uint32_t cy ) = 0;
	virtual void GetCellSize( uint32_t* cx, uint32_t* cy, int scale ) = 0;

   virtual void GetCurrentPeiceSize( int32_t *wX, int32_t *wY ) = 0;
	virtual void GetCurrentPeiceHotSpot( int32_t *wX, int32_t *wY ) = 0;

//   virtual void SetPeiceText( P

	// define the graphic for the background...
	// without a background the application can get no events.
	virtual void SetBackground( PIPEICE peice, uintptr_t psvCreate ) = 0;
	virtual void SetScale( int scale ) = 0;
	virtual int GetScale( void ) = 0;

	virtual PLAYER PutPeice( PIPEICE peice, int32_t x, int32_t y, uintptr_t psv ) = 0;
	virtual void SetSelectedTool( PIPEICE peice ) = 0;
   virtual PLAYER CreateActivePeice( int32_t x, int32_t y, uintptr_t psvCreate ) = 0;
   virtual PLAYER CreatePeice( CTEXTSTR name, int32_t x, int32_t y, uintptr_t psvCreate ) = 0;


	// begin path invokes some things like connect_to_peice...
	// failure can result from invalid conditions from the connection
	// methods... this reuslts back to the application here.
	virtual int BeginPath( PIVIA peice/*, int32_t x, int32_t y*/, uintptr_t psv ) = 0;
	virtual void EndPath( int32_t x, int32_t y ) = 0;
	virtual void UnendPath( void ) = 0;
	virtual PLAYER GetLayerAt( int32_t *wX, int32_t *wY, PLAYER notlayer = NULL ) = 0;

	virtual void BoardRefresh( void ) = 0;  // put current board on screen.

	// this method uses the currently selected peice passed to OnMouse method.
	// only valid from within an OnMouse event...
	virtual void LockDrag( void ) = 0;
	virtual void BeginSize( int dir ) = 0;

	// currently dispatched peiced is locked into a drag mode...
	// peices which are attached receive events?
	virtual void LockPeiceDrag( void ) = 0;
	virtual INDEX Save( PODBC odbc, CTEXTSTR name ) = 0;
	virtual LOGICAL Load( PODBC odbc, CTEXTSTR name ) = 0;
   virtual PSI_CONTROL GetControl( void ) = 0;


	virtual LOGICAL LoadPeiceConfiguration( CTEXTSTR file ) = 0;


};

PEICE_PROC( PIBOARD, CreateBoard )();
PEICE_PROC( PSI_CONTROL, CreateBoardControl )(PSI_CONTROL, int32_t,int32_t,uint32_t,uint32_t);
PEICE_PROC( PIBOARD, GetBoardFromControl )( PSI_CONTROL board );

PIPEICE GetPeice( PLIST peices, CTEXTSTR peice_name );

extern "C" void CheckTables( PODBC odbc );

#endif
