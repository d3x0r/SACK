#ifndef PEICES_DEFINED
#define PEICES_DEFINED

#include <sack_types.h>
#include <pssql.h>
#include <image.h>
//#include "matter.hpp"
//technically this should come from board.hpp
// but since board.hpp inclues this - this si a better place to define it.
typedef class IBOARD *PIBOARD;


#ifdef PEICE_SOURCE 
//#define PEICE_PROC(type,name) __declspec(dllexport) type CPROC name
#define PEICE_PROC(type,name) EXPORT_METHOD type name
#define PEICE_EXTERN(type,name) EXPORT_METHOD type name
#else
//#define PEICE_PROC(type,name) __declspec(dllimport) type CPROC name
#ifdef __LINUX__
#    define PEICE_PROC(type,name) type name
#    define PEICE_EXTERN(type,name) IMPORT_METHOD type name
#  else
#    define PEICE_PROC(type,name) IMPORT_METHOD type name
#    define PEICE_EXTERN(type,name) IMPORT_METHOD type name
#  endif
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

typedef struct delta_tag
{
	int x, y;
} DIR_DELTA;

#if defined( PEICE_SOURCE ) && !defined( PEICE_DEF_DEFINED )
IMPORT_METHOD
	DIR_DELTA DirDeltaMap[8];
#else
#ifndef PEICE_DEF_DEFINED
PEICE_EXTERN(DIR_DELTA, DirDeltaMap[8]);
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
	PTRSZVAL psvCreate;
	virtual void Destroy( void ) = 0;
	virtual CTEXTSTR name(void) = 0;
	virtual Image getimage(void) = 0;
	virtual Image getimage(int scale) = 0;
	virtual Image getcell( S_32 x, S_32 y ) = 0;
	virtual Image getcell( S_32 x, S_32 y, int scale ) = 0;
	virtual void gethotspot( PS_32 x, PS_32 y ) = 0;
	virtual void getsize( P_32 rows, P_32 cols ) = 0;
	virtual void SaveBegin( PODBC odbc, PTRSZVAL psvInstance ) = 0;
	virtual INDEX Save( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance ) = 0;
	virtual PTRSZVAL Load( PODBC odbc, INDEX iInstance ) = 0;
};

class PEICE_METHODS {
	// interface to peice gives access to
	// standard graphical elements... the image, size etc...
public:
	PIPEICE master;
	PEICE_PROC( CTEXTSTR, name )(void);
	PEICE_PROC( void, SetPeice )( PIPEICE peice ) { master = peice; }
	PEICE_PROC( Image, getimage )(void) { return master->getimage(); };
	PEICE_PROC( Image, getcell )( S_32 x, S_32 y ) { return master->getcell(x,y); };
	PEICE_PROC( Image, getimage )(int scale) { return master->getimage(scale); };
	PEICE_PROC( Image, getcell )( S_32 x, S_32 y, int scale ) { return master->getcell(x,y,scale); };
	PEICE_PROC( void, gethotspot )( PS_32 x, PS_32 y ) { master->gethotspot(x,y); };
	PEICE_PROC( void, getsize )( P_32 rows, P_32 cols ) { master->getsize(rows,cols); };

	// --- these are intended to be overridden.
	// the above are sufficient for static usage, and should not be
	// defined by derived peices...

	virtual PEICE_PROC( PTRSZVAL, Create )( PTRSZVAL psv_userdata );
	virtual PEICE_PROC( void, Destroy )( PTRSZVAL );
	// return 0 to disallow beginning of a connection, current path never really exists...
	virtual PEICE_PROC( int, ConnectBegin )( PTRSZVAL psv_to_instance, S_32 x, S_32 y
														, PIPEICE peice_from, PTRSZVAL psv_from_instance );
	// return 0 to disallow connection, current path dissappears.
	virtual PEICE_PROC( int, ConnectEnd )( PTRSZVAL psv_to_instance, S_32 x, S_32 y
													 , PIPEICE peice_from, PTRSZVAL psv_from_instance );
	// can return 0 to disallow disconnect...
	virtual PEICE_PROC( int, Disconnect )( PTRSZVAL psv_to_instance );
													 //, PIPEICE peice_to_disconnect, PTRSZVAL psv_to_disconnect_instance );
	virtual PEICE_PROC( void, OnMove )( PTRSZVAL psvInstance );
	virtual PEICE_PROC( int, OnClick )( PTRSZVAL psvInstance, S_32 x, S_32 y );
	virtual PEICE_PROC( int, OnRightClick )( PTRSZVAL psvInstance, S_32 x, S_32 y );
	virtual PEICE_PROC( int, OnDoubleClick )( PTRSZVAL psvInstance, S_32 x, S_32 y );

	virtual PEICE_PROC( void, Update )( PTRSZVAL psvInstance, _32 cycle );
	virtual PEICE_PROC( void, Draw )( PTRSZVAL psvInstance, Image surface, Image peice, S_32 x, S_32 y );

	// result with the instance ID in the database
	virtual PEICE_PROC( void, SaveBegin )( PODBC odbc, PTRSZVAL psvInstance );
	virtual PEICE_PROC( INDEX, Save )( PODBC odbc, INDEX iParent, PTRSZVAL psvInstance );
	virtual PEICE_PROC( PTRSZVAL, Load )( PODBC odbc, INDEX iInstance );
	//virtual PEICE_PROC( void, Save )( FILE *file, PTRSZVAL psvInstance );
	//virtual PEICE_PROC( void, Draw )( PTRSZVAL psvInstance
	//										  , Image surface
	//										  , int x, int y );
	//virtual PEICE_PROC( void, Draw )( PTRSZVAL psvInstance
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
	//virtual int OnRelease( PTRSZVAL psv );
	//virtual int OnRouteClick( PTRSZVAL psv );
	//virtual int Reroute();
	PEICE_PROC( void, SetPeice )( PIVIA peice ) { via_master = peice; PEICE_METHODS::master = (PIPEICE)peice; }
	virtual PEICE_PROC( int, OnClick )( PTRSZVAL psvInstance, S_32 x, S_32 y );
	virtual PEICE_PROC( int, OnRightClick )( PTRSZVAL psvInstance, S_32 x, S_32 y );
	virtual PEICE_PROC( int, OnDoubleClick )( PTRSZVAL psvInstance, S_32 x, S_32 y );
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
	PTRSZVAL psvInstance;
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
							 , PTRSZVAL psv
							 );
PEICE_PROC(PIVIA,DoCreateVia)( PIBOARD board, CTEXTSTR name //= "A Peice"
											 , Image image //= NULL
											 , PVIA_METHODS methods //= NULL
											 , PTRSZVAL psv
											 );

}

#endif
//--------------------------------------------------------------------
//
// $Log :$
//
