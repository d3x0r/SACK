#ifndef PEICES_DEFINED
#define PEICES_DEFINED

#include <sack_types.h>
#include <pssql.h>
#include <image.h>
//#include "matter.hpp"
//technically this should come from board.hpp
// but since board.hpp inclues this - this si a better place to define it.
typedef class IBOARD *PIBOARD;


#if !defined(__STATIC__) && !defined( __UNIX__ )
#ifdef PEICE_SOURCE 
//#define PEICE_PROC(type,name) __declspec(dllexport) type CPROC name
#define PEICE_PROC(type,name) EXPORT_METHOD type name
#define PEICE_EXTERN(type,name) EXPORT_METHOD type name
#else
//#define PEICE_PROC(type,name) __declspec(dllimport) type CPROC name
#ifdef __GNUC__
#define PEICE_PROC(type,name) type name
#define PEICE_EXTERN(type,name) type name
#else
#define PEICE_PROC(type,name) IMPORT_METHOD type name
#define PEICE_EXTERN(type,name) IMPORT_METHOD type name
#endif
#endif
#else
#ifdef PEICE_SOURCE
#define PEICE_PROC(type,name) type CPROC name
#define PEICE_EXTERN(type,name) type CPROC name
#else
#define PEICE_PROC(type,name) type CPROC name
#define PEICE_EXTERN(type,name) type CPROC name
#endif
#endif
//#undef PEICE_PROC
//#define PEICE_PROC(n,t) n t


#define NOWHERE -1
#define UP 0
#define UP_RIGHT 1
#define RIGHT 2
#define DOWN_RIGHT 3
#define DOWN 4
#define DOWN_LEFT 5
#define LEFT 6
#define UP_LEFT 7

typedef class LAYER_DATA *PLAYER_DATA;

typedef struct delta_tag
{
	int x, y;
} DIR_DELTA;

#if defined( PEICE_SOURCE ) && !defined( PEICE_DEF_DEFINED )
extern DIR_DELTA DirDeltaMap[8];
#else
#ifndef PEICE_DEF_DEFINED
// this used to be defined - think someone uses this?
//PEICE_EXTERN(DIR_DELTA, DirDeltaMap[8]);
#endif
#endif

typedef struct {
	// peice - full peice definition, has
	// one method - getcell.
	// the col, row expressed here is the specific
	// portion of the peice.
	class IPEICE *peice;
	int col, row;
} CELL, *PCELL;

typedef class IPEICE *PIPEICE;
typedef class PEICE_METHODS *PPEICE_METHODS;
typedef struct PEICE_DEFINITION *PPEICE_DEFINITION;

class IPEICE
{
public:
	PPEICE_METHODS methods;
	uintptr_t psvCreate;
	virtual void Destroy( void ) = 0;
	virtual CTEXTSTR name(void) = 0;
	virtual Image getimage(void) = 0;
	virtual Image getimage(int scale) = 0;
	virtual Image getcell( int32_t x, int32_t y ) = 0;
	virtual Image getcell( int32_t x, int32_t y, int scale ) = 0;
	virtual void gethotspot( int32_t* x, int32_t* y ) = 0;
	virtual void getsize( uint32_t* rows, uint32_t* cols ) = 0;
	virtual void SaveBegin( PODBC odbc, uintptr_t psvInstance ) = 0;
	virtual INDEX Save( PODBC odbc, INDEX iParent, uintptr_t psvInstance ) = 0;
	virtual uintptr_t Load( PODBC odbc, INDEX iInstance ) = 0;
};

class PEICE_METHODS {
	// interface to peice gives access to
	// standard graphical elements... the image, size etc...
public:
	PIPEICE master;
	PEICE_PROC( CTEXTSTR, name )(void);
	PEICE_PROC( void, SetPeice )( PIPEICE peice ) { master = peice; }
	PEICE_PROC( Image, getimage )(void) { return master->getimage(); };
	PEICE_PROC( Image, getcell )( int32_t x, int32_t y ) { return master->getcell(x,y); };
	PEICE_PROC( Image, getimage )(int scale) { return master->getimage(scale); };
	PEICE_PROC( Image, getcell )( int32_t x, int32_t y, int scale ) { return master->getcell(x,y,scale); };
	PEICE_PROC( void, gethotspot )( int32_t* x, int32_t* y ) { master->gethotspot(x,y); };
	PEICE_PROC( void, getsize )( uint32_t* rows, uint32_t* cols ) { master->getsize(rows,cols); };

	// --- these are intended to be overridden.
	// the above are sufficient for static usage, and should not be
	// defined by derived peices...

	virtual PEICE_PROC( uintptr_t, Create )( uintptr_t psv_userdata, PLAYER_DATA layer );
	virtual PEICE_PROC( void, Destroy )( uintptr_t );
	// return 0 to disallow beginning of a connection, current path never really exists...
	virtual PEICE_PROC( int, ConnectBegin )( uintptr_t psv_to_instance, int32_t x, int32_t y
														, PIPEICE peice_from, uintptr_t psv_from_instance );
	// return 0 to disallow connection, current path dissappears.
	virtual PEICE_PROC( int, ConnectEnd )( uintptr_t psv_to_instance, int32_t x, int32_t y
													 , PIPEICE peice_from, uintptr_t psv_from_instance );
	// can return 0 to disallow disconnect...
	virtual PEICE_PROC( int, Disconnect )( uintptr_t psv_to_instance );
													 //, PIPEICE peice_to_disconnect, uintptr_t psv_to_disconnect_instance );
	virtual PEICE_PROC( void, OnMove )( uintptr_t psvInstance );
	virtual PEICE_PROC( int, OnClick )( uintptr_t psvInstance, int32_t x, int32_t y );
	virtual PEICE_PROC( int, OnTap )( uintptr_t psvInstance, int32_t x, int32_t y );
	virtual PEICE_PROC( int, OnBeginDrag )( uintptr_t psvInstance, int32_t x, int32_t y );
	virtual PEICE_PROC( int, OnRightClick )( uintptr_t psvInstance, int32_t x, int32_t y );
	virtual PEICE_PROC( int, OnDoubleClick )( uintptr_t psvInstance, int32_t x, int32_t y );

	virtual PEICE_PROC( void, Update )( uintptr_t psvInstance, uint32_t cycle );
	virtual PEICE_PROC( void, Draw )( uintptr_t psvInstance, Image surface, Image peice, int32_t x, int32_t y );
	//virtual void DrawExtra( uintptr_t psv, Image surface, int32_t x, int32_t y, uint32_t w, uint32_t h ) = 0;

	// result with the instance ID in the database
	virtual PEICE_PROC( void, SaveBegin )( PODBC odbc, uintptr_t psvInstance );
	virtual PEICE_PROC( INDEX, Save )( PODBC odbc, INDEX iParent, uintptr_t psvInstance );
	virtual PEICE_PROC( uintptr_t, Load )( PODBC odbc, INDEX iInstance );
	//virtual PEICE_PROC( void, Save )( FILE *file, uintptr_t psvInstance );
	//virtual PEICE_PROC( void, Draw )( uintptr_t psvInstance
	//										  , Image surface
	//										  , int x, int y );
	//virtual PEICE_PROC( void, Draw )( uintptr_t psvInstance
	//										  , Image surface
	//										  , int x, int y
												// these two are the cell offset if cells
												// 2, 2 - 3, 3 are to be drawn at x, y
                                    // cellx = 2, celly = 2, rows = 1, cols = 1
	//										  , int cellx, int celly
	//										  );
};


typedef class VIA_METHODS *PVIA_METHODS;
typedef class IVIA *PIVIA;
class VIA_METHODS: public PEICE_METHODS {
public:
	PIVIA via_master;
	//virtual int OnRelease( uintptr_t psv );
	//virtual int OnRouteClick( uintptr_t psv );
	//virtual int Reroute();
	PEICE_PROC( void, SetPeice )( PIVIA peice ) { via_master = peice; PEICE_METHODS::master = (PIPEICE)peice; }
	virtual PEICE_PROC( int, OnClick )( uintptr_t psvInstance, int32_t x, int32_t y );
	virtual PEICE_PROC( int, OnRightClick )( uintptr_t psvInstance, int32_t x, int32_t y );
	virtual PEICE_PROC( int, OnDoubleClick )( uintptr_t psvInstance, int32_t x, int32_t y );
};

class IVIA: public IPEICE
{
public:
	//virtual ~IVIA();
	virtual CTEXTSTR name( void )=0;
	virtual Image GetViaStart( int direction, int scale = 0 )=0;// { return NULL; }
	virtual Image GetViaEnd( int direction, int scale = 0 )=0;//{ return NULL; }
	// getviafromto will result in start or end if from or to is NOWHERE respectively
	virtual Image GetViaFromTo( int from, int to, int scale = 0 ){ return NULL; }

	virtual Image GetViaFill1( int *xofs, int *yofs, int direction, int scale = 0 ){ return NULL; }
	virtual Image GetViaFill2( int *xofs, int *yofs, int direction, int scale = 0 ){ return NULL; }
	virtual int Move( void ) { return 0; } // Begin, Start
	virtual int Stop( void ) { return 0; } // end
	PVIA_METHODS via_methods;
};


struct PEICE_INSTANCE {
	uintptr_t psvInstance;
	PIPEICE peice;
	PEICE_METHODS methods;
};

extern "C" {
	PEICE_PROC( PIPEICE, DoCreatePeice )( PIBOARD board, CTEXTSTR name //= "A Peice"
							 , Image image //= NULL
							 , int rows //= 1
							 , int cols //= 1
							 , int hotspot_x
							 , int hotspot_y
							 , PPEICE_METHODS methods //= NULL
							 , uintptr_t psv
							 );
PEICE_PROC(PIVIA,DoCreateVia)( PIBOARD board, CTEXTSTR name //= "A Peice"
											 , Image image //= NULL
											 , PVIA_METHODS methods //= NULL
											 , uintptr_t psv
											 );

}

#endif
//--------------------------------------------------------------------
//
// $Log :$
//
